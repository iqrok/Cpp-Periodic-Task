#ifndef _SLEEP_HELPER_HPP_
#define _SLEEP_HELPER_HPP_

#include <cstdint>
#include <time.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

namespace Sleep {

struct task_config_s {
	bool is_running;
	int8_t affinity;
	uint16_t step_sleep;
	int schedule_priority;
	int nice_value;
	float tolerance;
	float laziness;
	uint64_t period_ns;
	int64_t offset_ns;
	int64_t exec_time;
	int64_t cycle_time;
	uint64_t _period_cmp;
	struct timespec timer;
	struct timespec deadline;
	struct timespec current;
	void (*fptr)(void);
};

void timespec_diff(const timespec& a, const timespec& b, int64_t* ns)
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

bool timespec_compare(const timespec& left, const timespec& right)
{
	return (left.tv_sec == right.tv_sec)
		? left.tv_nsec < right.tv_nsec
		: left.tv_sec < right.tv_sec;
}

void start_timer(timespec* start)
{
	clock_gettime(CLOCK_MONOTONIC, start);
}

void wait(struct task_config_s* task)
{
	// calculate execution time
	clock_gettime(CLOCK_MONOTONIC, &task->current);
	timespec_diff(task->current, task->timer, &task->exec_time);

	// reuse task->current to set deadline
	task->deadline.tv_sec = task->timer.tv_sec;
	task->deadline.tv_nsec = task->timer.tv_nsec + task->_period_cmp;

	// normalize timespec, if nsec is overflowed
	timespec_normalize(&task->deadline);

	// sleep for given duration
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->deadline, NULL);

	// calculate cycle time
	clock_gettime(CLOCK_MONOTONIC, &task->deadline);
	timespec_diff(task->deadline, task->timer, &task->cycle_time);
}

void busy_wait(struct task_config_s* task)
{
	uint16_t counter = 0;
	//~ timespec deadline, timer;

	// calculate execution time
	clock_gettime(CLOCK_MONOTONIC, &task->current);
	timespec_diff(task->current, task->timer, &task->exec_time);

	if(task->laziness > 0){
		// set deadline
		task->deadline.tv_sec = task->timer.tv_sec;
		task->deadline.tv_nsec = task->timer.tv_nsec + task->laziness;

		// normalize timespec, if nsec is overflowed
		timespec_normalize(&task->deadline);

		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->deadline, NULL);
	}

	// set deadline
	task->deadline.tv_sec = task->timer.tv_sec;
	task->deadline.tv_nsec = task->timer.tv_nsec + task->_period_cmp;

	// normalize timespec, if nsec is overflowed
	timespec_normalize(&task->deadline);

	// busy wait until timer > deadline
	while (timespec_compare(task->current, task->deadline)) {
		// update timer
		clock_gettime(CLOCK_MONOTONIC, &task->current);

		// need to add sleep to avoid throttling being activated by OS
		if (++counter > task->step_sleep) {
			counter = 0;
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &task->current, NULL);
		}
	}

	// calculate cycle time
	timespec_diff(task->current, task->timer, &task->cycle_time);
}

}

#endif
