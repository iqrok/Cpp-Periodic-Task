#ifndef _SLEEP_HELPER_HPP_
#define _SLEEP_HELPER_HPP_

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

void timespec_diff(const struct timespec& a, const struct timespec& b,
	int64_t* ns)
{
	*ns = ((a.tv_sec - b.tv_sec) * NSEC_PER_SEC) + (a.tv_nsec - b.tv_nsec);
}

void timespec_normalize(struct timespec* a)
{
	while (a->tv_nsec >= NSEC_PER_SEC) {
		a->tv_nsec -= NSEC_PER_SEC;
		a->tv_sec++;
	}
}

void timespec_copy(struct timespec* dst, const struct timespec& src,
	const int64_t& offset_ns)
{
	dst->tv_sec = src.tv_sec;
	dst->tv_nsec = src.tv_nsec + offset_ns;

	timespec_normalize(dst);
}

bool timespec_compare(const struct timespec& left, const struct timespec& right)
{
	return (left.tv_sec == right.tv_sec)
		? left.tv_nsec < right.tv_nsec
		: left.tv_sec < right.tv_sec;
}

void start_timer(struct timespec* start)
{
	clock_gettime(CLOCK_MONOTONIC, start);
}

void wait(struct sleep_task_s* task)
{
	// calculate execution time
	clock_gettime(CLOCK_MONOTONIC, &task->current);
	timespec_diff(task->current, task->timer, &task->exec_time);

	// reuse task->current to set deadline
	timespec_copy(&task->deadline, task->timer, task->_period_cmp);

	// sleep for given duration
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->deadline, NULL);

	// calculate cycle time
	clock_gettime(CLOCK_MONOTONIC, &task->deadline);
	timespec_diff(task->deadline, task->timer, &task->cycle_time);
}

void busy_wait(struct sleep_task_s* task)
{
	task->_counter = 0;

	// calculate execution time
	clock_gettime(CLOCK_MONOTONIC, &task->current);
	timespec_diff(task->current, task->timer, &task->exec_time);

	// do lazy sleep, if it's valid, to reduce cpu usage on busy waiting
	if (task->lazy_sleep > 0) {
		// set deadline for lazy sleep
		timespec_copy(&task->deadline, task->timer, task->lazy_sleep);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->deadline, NULL);
	}

	// set deadline for busy wait
	timespec_copy(&task->deadline, task->timer, task->_period_cmp);

	// busy wait until timer > deadline
	while (timespec_compare(task->current, task->deadline)) {
		// update timer
		clock_gettime(CLOCK_MONOTONIC, &task->current);

		// need to add sleep to avoid throttling being activated by OS
		if (++task->_counter > task->step_sleep) {
			task->_counter = 0;
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->current, NULL);
		}
	}

	// calculate cycle time
	timespec_diff(task->current, task->timer, &task->cycle_time);
}

}

#endif
