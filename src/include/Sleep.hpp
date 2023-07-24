#ifndef _SLEEP_HELPER_HPP_
#define _SLEEP_HELPER_HPP_

#include <time.h>
#include <cstdint>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

namespace Sleep {

void timespec_diff(const timespec& a, const timespec& b, int64_t* ns)
{
	*ns = ((a.tv_sec - b.tv_sec) * NSEC_PER_SEC)
		+ (a.tv_nsec - b.tv_nsec);
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

void wait(const timespec& start, const uint64_t& period_ns,
	int64_t* exec_time, int64_t* cycle_time)
{
	timespec timer;

	// calculate execution time
	clock_gettime(CLOCK_MONOTONIC, &timer);
	timespec_diff(timer, start, exec_time);

	// reuse timer to set deadline
	timer.tv_sec = start.tv_sec;
	timer.tv_nsec = start.tv_nsec + period_ns;

	// normalize timespec, if nsec is overflowed
	while (timer.tv_nsec >= NSEC_PER_SEC) {
		timer.tv_nsec -= NSEC_PER_SEC;
		timer.tv_sec++;
	}

	// sleep for given duration
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timer, NULL);

	// calculate cycle time
	clock_gettime(CLOCK_MONOTONIC, &timer);
	timespec_diff(timer, start, cycle_time);
}

void busy_wait(const timespec& start, const uint64_t& period_ns,
	int64_t* exec_time, int64_t* cycle_time, const uint16_t& step_sleep)
{
	uint16_t counter = 0;
	timespec deadline, timer;

	// calculate execution time
	clock_gettime(CLOCK_MONOTONIC, &timer);
	timespec_diff(timer, start, exec_time);

	// set deadline
	deadline.tv_sec = start.tv_sec;
	deadline.tv_nsec = start.tv_nsec + period_ns;

	// normalize timespec, if nsec is overflowed
	while (deadline.tv_nsec >= NSEC_PER_SEC) {
		deadline.tv_nsec -= NSEC_PER_SEC;
		deadline.tv_sec++;
	}

	// busy wait until timer > deadline
	while (timespec_compare(timer, deadline)) {
		// update timer
		clock_gettime(CLOCK_MONOTONIC, &timer);

		// need to add sleep to avoid throttling being activated by OS
		if (++counter > step_sleep) {
			counter = 0;
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timer, NULL);
		}
	}

	// calculate cycle time
	timespec_diff(timer, start, cycle_time);
}

}

#endif
