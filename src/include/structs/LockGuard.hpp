#pragma once

#include <zephyr/kernel.h>
#include <pthread.h>

class LockGuard
{
public:
	explicit LockGuard(struct k_mutex &mutex) : _mutex(&mutex), _is_kernel(true)
	{
		k_mutex_lock(_mutex, K_FOREVER);
	}

	explicit LockGuard(pthread_mutex_t &mutex) : _pmutex(&mutex), _is_kernel(false)
	{
		pthread_mutex_lock(_pmutex);
	}

	LockGuard(const LockGuard &) = delete;
	LockGuard &operator=(const LockGuard &) = delete;

	~LockGuard()
	{
		if (_is_kernel) {
			k_mutex_unlock(_mutex);
		} else {
			pthread_mutex_unlock(_pmutex);
		}
	}

private:
	struct k_mutex *_mutex{nullptr};
	pthread_mutex_t *_pmutex{nullptr};
	bool _is_kernel{false};
};
