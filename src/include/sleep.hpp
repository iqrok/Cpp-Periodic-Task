#ifndef _SLEEP_HELPER_HPP_
#define _SLEEP_HELPER_HPP_

#include <time.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

namespace Sleep {

bool timespec_compare(const timespec& left, const timespec& right)
{
	if (left.tv_sec == right.tv_sec)
		return left.tv_nsec < right.tv_nsec;

	return left.tv_sec < right.tv_sec;
}

void start_timer(timespec* start)
{
	clock_gettime(CLOCK_MONOTONIC, start);
}

uint64_t wait_for_given_period(const timespec& start, const uint64_t& period_ns)
{
	timespec stop, wakeup_time, remain;

	clock_gettime(CLOCK_MONOTONIC, &stop);

	uint64_t exec_time = (stop.tv_nsec + ((stop.tv_sec - start.tv_sec) * NSEC_PER_SEC)) - start.tv_nsec;
	uint64_t ns = period_ns - exec_time;

	if (ns < 0)
		return exec_time;

	clock_gettime(CLOCK_MONOTONIC, &wakeup_time);
	wakeup_time.tv_nsec += ns;

	while (wakeup_time.tv_nsec >= NSEC_PER_SEC) {
		wakeup_time.tv_nsec -= NSEC_PER_SEC;
		wakeup_time.tv_sec++;
	}

	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup_time, NULL);

	return exec_time;
}

uint64_t busy_wait_for_given_period(const timespec& start, const uint64_t& period_ns)
{
	timespec stop, timer;

	clock_gettime(CLOCK_MONOTONIC, &stop);
	clock_gettime(CLOCK_MONOTONIC, &timer);

	uint64_t exec_time = (stop.tv_nsec + ((stop.tv_sec - start.tv_sec) * NSEC_PER_SEC)) - start.tv_nsec;
	uint64_t ns = period_ns - exec_time;

	// cap ns at 0
	if (ns < 0)
		ns = 0;

	// reuse stop to determine the deadline
	stop.tv_sec = start.tv_sec;
	stop.tv_nsec = start.tv_nsec + ns;

	// normalize timespec, if nsec is overflow
	while (stop.tv_nsec >= NSEC_PER_SEC) {
		stop.tv_nsec -= NSEC_PER_SEC;
		stop.tv_sec++;
	}

	// busy wait until timer > stop
	while (timespec_compare(timer, stop)) {
		// update timer
		clock_gettime(CLOCK_MONOTONIC, &timer);

		// need to add sleep, otherwise some loop will be executed at much later time
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timer, NULL);
	}

	return exec_time;
}

}

#endif
