/* This is an internal header, which will not be installed. */
#pragma once
#include <mutex>

#include "pthread.h"
#include <config.h>

// #define USE_LOCK_GUARD_D

#if defined(DEBUG)
void debug_print_mutex_info(const char* name, const char* func, const char* file, int line, char icon);
void debug_pring_worker_start(const char* file, int line, char icon);
#endif

#if defined(DEBUG)
    class LockGuardWithDebug {
  private:
    const char*                  mutex_name;
    const char*                  func_name;
    const char*                  file_name;
    int                          line;
    std::lock_guard<std::mutex>* lock;

  public:
    LockGuardWithDebug(const char* mutex_name, const char* func_name, std::mutex& mutex, const char* file_name, int line) : mutex_name(mutex_name), func_name(func_name), file_name(file_name), line(line){
        debug_print_mutex_info(mutex_name, func_name, file_name, line, '*');
        lock = new std::lock_guard(mutex);
        debug_print_mutex_info(mutex_name, func_name, file_name, line, '+');
    }
    ~LockGuardWithDebug(){
        delete lock;
        debug_print_mutex_info(mutex_name, func_name, file_name, line, '-');
    }
};

#if defined(USE_LOCK_GUARD_D)
#define LOCK_GUARD_D(mutex, name) LockGuardWithDebug name(#mutex, __func__, mutex, __FILE__, __LINE__)
#else
#define LOCK_GUARD_D(m, name) std::lock_guard<std::mutex> name(m)
#endif

#define WORKER_BEGIN debug_pring_worker_start(__FILE__, __LINE__, '{')
#define WORKER_FINISH debug_pring_worker_start(__FILE__, __LINE__, '}')
#define DEBUG_OUT(message) std::cout << message << __FILE__ << " : " << __func__ << "(" << __LINE__ << ")" << std::endl;
#else
#define WORKER_BEGIN
#define WORKER_FINISH
#define DEBUG_OUT(m)
#endif
