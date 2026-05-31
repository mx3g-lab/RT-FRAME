#pragma once

#include <zephyr/kernel.h>

class AtomicTransaction
{
public:
	AtomicTransaction()  { k_mutex_lock(&_mutex, K_FOREVER); }
	~AtomicTransaction() { k_mutex_unlock(&_mutex); }

	void lock()   { k_mutex_lock(&_mutex, K_FOREVER); }
	void unlock() { k_mutex_unlock(&_mutex); }

private:
	static struct k_mutex _mutex;
};
