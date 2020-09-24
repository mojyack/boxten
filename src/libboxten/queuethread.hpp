/* This is an internal header, which will not be installed. */
#pragma once
#include <mutex>
#include <condition_variable>

#include "worker.hpp"
#include "debug.hpp"

namespace boxten{
template <class T>
class QueueThread {
  protected:
    virtual void proc(std::vector<T> queue_to_proc) = 0;

  private:
    std::vector<T>          queue;
    std::mutex              queue_lock;
    bool                    queue_processing = false;

    bool                    queue_changed;
    std::mutex              queue_changed_lock;
    std::condition_variable queue_changed_cond;

    Worker                  thread;
    bool                    finish_thread;
    void         loop() {
        while(!finish_thread) {
            std::vector<T> queue_to_proc;
            {
                std::unique_lock<std::mutex> clock(queue_changed_lock);
                queue_changed_cond.wait(clock, [&]() { return queue_changed || finish_thread; });
                LOCK_GUARD_D(queue_lock, lock);
                queue_processing = true;
                queue_to_proc = queue;
                queue.clear();
                queue_changed = false;
            }
            proc(queue_to_proc);
            queue_processing = false;
        }
    }

  public:
    void start(){
        queue_changed = false;
        finish_thread = false;
        thread        = Worker(std::bind(&QueueThread::loop, this));
    }
    void finish(){
        finish_thread = true;
        queue_changed_cond.notify_one();
        thread.join();
    }
    void enqueue(T data){
        {
            LOCK_GUARD_D(queue_lock, lock);
            queue.emplace_back(data);
        }
        {
            LOCK_GUARD_D(queue_changed_lock, lock);
            queue_changed = true;
            queue_changed_cond.notify_one();
        }
    }
    std::mutex& wait_empty() {
        while(!finish_thread) {
            LOCK_GUARD_D(queue_lock, lock);
            if(queue.empty() && !queue_processing) {
                break;
            }
        }
        return queue_lock;
    }
    virtual ~QueueThread(){}
};
}