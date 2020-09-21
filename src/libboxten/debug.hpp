/* This is an internal header, which will not be installed. */
#pragma once
#include <mutex>

#include "pthread.h"
#include <config.h>

// #define USE_LOCK_GUARD_D

#ifdef DEBUG
void debug_print_mutex_info(const char* name, const char* func, const char* file, int line, char icon);
void debug_pring_worker_start(const char* file, int line, char icon);
#endif

#ifdef DEBUG
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

#ifdef USE_LOCK_GUARD_D
#define LOCK_GUARD_D(mutex, func, name) LockGuardWithDebug name(#mutex, #func, mutex, __FILE__, __LINE__)
#else
#define LOCK_GUARD_D(m, f, name) std::lock_guard<std::mutex> name(m)
#endif

#define WORKER_BEGIN debug_pring_worker_start(__FILE__, __LINE__, '{')
#define WORKER_FINISH debug_pring_worker_start(__FILE__, __LINE__, '}')
#define DEBUG_OUT(message) std::cout << message << __FILE__ << __LINE__ << std::endl;
#elif
#define WORKER_BEGIN
#define WORKER_FINISH
#deinfe DEBUG_OUT(m)
#endif