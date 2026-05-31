/**
 * autosave.cpp — Parameter autosave on the vwork param_save work queue.
 *
 * Uses k_work_delayable submitted to vwork::configs::param_save queue
 * (PRIORITY_FS, 2 kB stack) instead of PX4 ScheduledWorkItem.
 *
 * actuator_armed check is preserved: when saving to FLASH (no default file),
 * defer save while armed to avoid CPU stalls from flash writes.
 */

#include "autosave.h"

#include <log/px4_log.h>
#include <uORB/topics/actuator_armed.h>
#include <uORB/Subscription.hpp>

#include "param.h"
#include "atomic_transaction.h"

using namespace time_literals;

ParamAutosave::ParamAutosave()
{
	k_work_init_delayable(&_dwork, work_handler);
	_wq = vwork::work_queue_find_or_create(vwork::configs::param_save);
}

ParamAutosave::~ParamAutosave()
{
	k_work_cancel_delayable(&_dwork);
}

void ParamAutosave::work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	ParamAutosave *self = CONTAINER_OF(dwork, ParamAutosave, _dwork);
	self->run();
}

void ParamAutosave::request()
{
	if (_scheduled.load() || _disabled) {
		return;
	}

	hrt_abstime delay = 300_ms;

	static constexpr hrt_abstime rate_limit = 2_s;
	const hrt_abstime last_save_elapsed = hrt_elapsed_time(&_last_timestamp);

	if (last_save_elapsed < rate_limit && rate_limit > last_save_elapsed + delay) {
		delay = rate_limit - last_save_elapsed;
	}

	_scheduled.store(true);
	k_work_schedule_for_queue(_wq, &_dwork, K_USEC(delay));
}

void ParamAutosave::enable(bool enable)
{
	AtomicTransaction transaction;
	_disabled = !enable;

	if (!enable && _scheduled.load()) {
		_scheduled.store(false);
		k_work_cancel_delayable(&_dwork);
	}
}

void ParamAutosave::run()
{
	bool disabled = false;

	if (!param_get_default_file()) {
		/* Saving to FLASH: defer while armed to avoid CPU stalls from flash writes */
		uORB::SubscriptionData<actuator_armed_s> armed_sub{ORB_ID(actuator_armed)};

		if (armed_sub.get().armed) {
			_scheduled.store(true);
			k_work_schedule_for_queue(_wq, &_dwork, K_SECONDS(1));
			return;
		}
	}

	{
		const AtomicTransaction transaction;
		_last_timestamp = hrt_absolute_time();
		_scheduled.store(false);
		disabled = _disabled;
	}

	if (disabled) {
		return;
	}

	PX4_DEBUG("Autosaving params");
	int ret = param_save_default(false);

	if (ret != PX4_OK) {
		if (_retry_count < 3) {
			_retry_count++;
			PX4_INFO("param auto save unavailable (%i), retrying..", ret);
			request();

		} else {
			PX4_ERR("param auto save failed (%i)", ret);
			_retry_count = 0;
		}

	} else {
		_retry_count = 0;
	}
}
