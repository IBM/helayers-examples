#include "liphe/thread_pool.h"

std::mutex ThreadPool::_global_mutex;
sem_t ThreadPool::_threads;
bool ThreadPool::_inited = false;
