/* This is an internal header, which will not be installed. */
#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#include "type.hpp"

namespace boxten {
class WorkerThread {
  public:
    enum STATE {
        PREPARED,
        RUNNING,
        FINISHED,
        JOINED,
    };

  private:
    SafeVar<STATE> state = STATE::PREPARED;

    std::condition_variable work_finished;
    std::thread thread;

    std::function<void(void)> function;

  public:
    void start();
    void join();
    STATE get_state();
    WorkerThread(std::function<void(void)> function);
    ~WorkerThread();
};
void start_master_thread();
std::thread& finish_master_thread();
WorkerThread* queue_worker_thread(std::function<void(void)> function);
} // namespace boxten