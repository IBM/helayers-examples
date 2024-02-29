#ifndef ___SIMPLE_THREAD_POOL___
#define ___SIMPLE_THREAD_POOL___

#include <assert.h>
#include <unistd.h>
#include <semaphore.h>
#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

class ThreadPool;

struct Thread {
	enum State { Waiting, Running, Zombie, Done };

	ThreadPool *_pool;
	std::function<void(void)> _f;
	State _state;
	std::thread _thread;

	Thread(const std::function<void(void)> &f, ThreadPool *pool) : _pool(pool), _f(f), _state(Waiting) { }
	Thread(const Thread &a) = delete;
	Thread(Thread &&a) : _pool(a._pool), _f(a._f), _state(a._state) {
		_thread = std::move(a._thread);
	}

	~Thread() {
		assert(!_thread.joinable());
	}

	Thread &operator=(Thread &&t) {
		_pool = t._pool;
		_f = t._f;
		_state = t._state;
		assert(!_thread.joinable());
		_thread = std::move(t._thread);
		return *this;
	}

	void set_pool(ThreadPool *p) { _pool = p; }

	inline void run();
};


class ThreadPool {
private:
	static std::mutex _global_mutex;
	static sem_t _threads;
	static bool _inited;

	std::vector<Thread *> _jobs;
public:

	static void init(int no = 1) {
		if (_inited) {
			sem_destroy(&_threads);
		}
		sem_init(&_threads, 0, no);
		_inited = true;
	}

	ThreadPool() {
		_global_mutex.lock();
		if (!_inited)
			init(1);
		_global_mutex.unlock();
	}

	~ThreadPool() {
		for (auto i = _jobs.begin(); i < _jobs.end(); ++i)
			delete (*i);
	}


	void notify_thread_finished() {
		sem_post(&_threads);
	}

	void submit_job(const std::function<void(void)> &f) {
		Thread *t = new Thread(f, this);
		_jobs.push_back(t);
		process_jobs(0);
	}

	int get_free_cpu() {
		int ret;
		sem_getvalue(&_threads, &ret);
		return ret;
	}

	bool has_free_cpu() {
		return get_free_cpu() > 0;
	}

	void process_jobs(int iterations = -1) {
		int iter = iterations;
		while ((iterations == -1) || (iter >= 0)) {
			bool all_is_done = false;

			int waiting = 0;
			int running = 0;

			all_is_done = true;
			auto i = _jobs.begin();
			while (i != _jobs.end()) {
				if ((*i)->_state != Thread::Done)
					all_is_done = false;

				if ((*i)->_state == Thread::Running)
					++running;
				if ((*i)->_state == Thread::Waiting)
					++waiting;

				if ((*i)->_state == Thread::Zombie) {
					(*i)->_thread.join();
					(*i)->_state = Thread::Done;
					delete (*i);
					_jobs.erase(i);
					i = _jobs.begin();
				} else
					++i;
			}

//			std::cerr << "Waiting: " << waiting << "    running: " << running << "     semaphore: " << get_free_cpu() << std::endl;

			if (all_is_done)
				return;

			i = _jobs.begin();
			while ((i != _jobs.end()) && ((*i)->_state != Thread::Waiting))
				++i;

			if (i != _jobs.end()) {
				int ret;
				if (iter == 0)
					ret = sem_trywait(&_threads);
				else
					ret = sem_wait(&_threads);

				if (ret == 0)
					(*i)->run();
			}


			--iter;
		}
	}
};

inline void Thread::run() {
	_state = Running;
	_thread = std::thread( [this](){
//		std::cout << "starting thread" << std::endl;
		_f();
//		std::cout << "ending thread" << std::endl;
		_state = Zombie;
		_pool->notify_thread_finished();
	});
}

#endif
