#ifndef _SLEEP_HELPER_HPP_
#define _SLEEP_HELPER_HPP_

#include "TimespecHelper.hpp"

#include <cstdint>
#include <time.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

namespace Sleep {

struct sleep_task_s {
	bool is_running = false;
	int8_t schedule_priority = -1;
	int8_t affinity = -1;
	int8_t nice_value = 20;
	uint16_t step_sleep = 1;
	uint16_t _counter = 0;
	float tolerance = -1;
	int32_t priority_offset = 0;
	uint64_t lazy_sleep = 0;
	uint64_t period_ns;
	int64_t offset_ns = 0;
	int64_t exec_time;
	int64_t cycle_time;
	uint64_t _period_cmp;
	struct timespec timer;
	struct timespec deadline;
	struct timespec current;
	void (*fptr)(void);
};

void wait(struct sleep_task_s* task)
{
	// calculate execution time
	Timespec::now(&task->current);
	Timespec::diff(task->current, task->timer, &task->exec_time);

	// reuse task->current to set deadline
	Timespec::copy(&task->deadline, task->timer, task->_period_cmp);

	// sleep for given duration
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->deadline, NULL);

	// calculate cycle time
	Timespec::now(&task->deadline);
	Timespec::diff(task->deadline, task->timer, &task->cycle_time);
}

void busy_wait(struct sleep_task_s* task)
{
	task->_counter = 0;

	// calculate execution time
	Timespec::now(&task->current);
	Timespec::diff(task->current, task->timer, &task->exec_time);

	// do lazy sleep, if it's valid, to reduce cpu usage on busy waiting
	if (task->lazy_sleep > 0) {
		// set deadline for lazy sleep
		Timespec::copy(&task->deadline, task->timer, task->lazy_sleep);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->deadline, NULL);
	}

	// set deadline for busy wait
	Timespec::copy(&task->deadline, task->timer, task->_period_cmp);

	// busy wait until timer > deadline
	while (Timespec::compare(task->current, task->deadline)) {
		// update timer
		Timespec::now(&task->current);

		// need to add sleep to avoid throttling being activated by OS
		if (++task->_counter > task->step_sleep) {
			task->_counter = 0;
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->current, NULL);
		}
	}

	// calculate cycle time
	Timespec::diff(task->current, task->timer, &task->cycle_time);
}

}

#endif
