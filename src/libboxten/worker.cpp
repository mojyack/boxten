#include <list>

#include <console.hpp>
#include <worker.hpp>
#include <worker_internal.hpp>

#include "debug.hpp"

namespace boxten{
namespace {
std::thread master_thread;
bool exit_master_thread;
SafeVar<std::list<boxten::WorkerThread>> workers;

SafeVar<bool>           workers_changed = false;
std::condition_variable refresh_workers;
void notify_workers_changed(){
    std::lock_guard<std::mutex> lock(workers_changed.lock);
    workers_changed = true;
    refresh_workers.notify_one();
}
} // namespace

void WorkerThread::start() {
    auto new_work = [&]() {
        WORKER_BEGIN;
        function();
        WORKER_FINISH;
        {
            std::lock_guard<std::mutex> lock(state.lock);
            state = STATE::FINISHED;
            work_finished.notify_one();
        }
        notify_workers_changed();
    };
    state = STATE::RUNNING;
    thread = std::thread(new_work);
}
void WorkerThread::join(){
    std::unique_lock<std::mutex> lock(state.lock);
    work_finished.wait(lock, [&]() {
        return state == STATE::FINISHED;
    });
    state = STATE::JOINED;
}

WorkerThread::STATE WorkerThread::get_state() {
    std::lock_guard<std::mutex> lock(state.lock);
    return state;
}
WorkerThread::WorkerThread(std::function<void(void)> function)
    : function(function){}
WorkerThread ::~WorkerThread() {
    thread.join();
}

void start_master_thread(){
    exit_master_thread = false;
    master_thread      = std::thread([&]() {
        while(!exit_master_thread){
            {
                std::unique_lock<std::mutex> lock(workers_changed.lock);
                refresh_workers.wait(lock, [&]() { return workers_changed || exit_master_thread; });
                if(exit_master_thread) break;
                workers_changed = false;
            }

            std::lock_guard<std::mutex> lock(workers.lock);
            for(auto w = workers->begin(); w != workers->end();) {
                auto state = w->get_state();
                if(state == WorkerThread::STATE::PREPARED){
                    w->start();
                } else if(state == WorkerThread::STATE::JOINED){
                    
                    w = workers->erase(w);
                }
                if(state != WorkerThread::STATE::JOINED) w++;
            }
        }
    });
}
std::thread& finish_master_thread() {
    exit_master_thread = true;
    notify_workers_changed();
    return master_thread;
}
WorkerThread* queue_worker_thread(std::function<void(void)> function){
    std::lock_guard<std::mutex> lock(workers.lock);
    workers->emplace_back(function);
    notify_workers_changed();
    return &workers->back();
}

void Worker::join(){
    worker_thread->join();
    worker_thread = nullptr;
}
Worker& Worker::operator=(Worker&& old){
    worker_thread = old.worker_thread;
    old.worker_thread = nullptr;

    return *this;
}
Worker::operator bool() {
    return worker_thread != nullptr;
}
Worker::Worker(std::function<void(void)> function){
    worker_thread = queue_worker_thread(function);
}
Worker::Worker() {}
Worker::~Worker(){
    if(worker_thread != nullptr){
        DEBUG_OUT("Worker not joined!");
        std::terminate();
    };
}
}