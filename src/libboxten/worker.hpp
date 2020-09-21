#pragma once
#include <functional>

namespace boxten {
class WorkerThread;
class Worker {
  private:
    WorkerThread* worker_thread = nullptr;
  public:
    void    join();
    Worker& operator=(Worker&& old);
    operator bool();
    Worker(std::function<void(void)> function);
    Worker();
    ~Worker();
};
} // namespace boxten