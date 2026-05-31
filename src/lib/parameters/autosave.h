#pragma once

#include <zephyr/kernel.h>
#include <px4_atomic.h>
#include <hrt/hrt.h>
#include <vwork_config.h>
#include <work_queue_manager.h>

class ParamAutosave
{
public:
	ParamAutosave();
	~ParamAutosave();

	void request();
	void enable(bool enable);
	bool enabled() const { return !_disabled; }
	hrt_abstime lastAutosave() const { return _last_timestamp; }

private:
	static void work_handler(struct k_work *work);
	void run();

	struct k_work_delayable _dwork;
	struct k_work_q        *_wq{nullptr};
	hrt_abstime _last_timestamp{0};
	px4::atomic_bool _scheduled{false};
	int _retry_count{0};
	bool _disabled{false};
};
