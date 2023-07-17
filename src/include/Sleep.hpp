#ifndef _SLEEP_HELPER_HPP_
#define _SLEEP_HELPER_HPP_

#include <time.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

namespace Sleep {

uint32_t step_sleep = 500;

void set_step_sleep(uint32_t num){
	step_sleep = num;
}

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

uint64_t wait(const timespec& start, const uint64_t& period_ns)
{
	timespec stop;

	clock_gettime(CLOCK_MONOTONIC, &stop);

	uint64_t exec_time = (stop.tv_nsec + ((stop.tv_sec - start.tv_sec) * NSEC_PER_SEC)) - start.tv_nsec;
	uint64_t ns = period_ns - exec_time;

	// cap ns at 0
	if (ns < 0){
		ns = 0;
	}

	stop.tv_nsec += ns;

	while (stop.tv_nsec >= NSEC_PER_SEC) {
		stop.tv_nsec -= NSEC_PER_SEC;
		stop.tv_sec++;
	}

	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &stop, NULL);

	return exec_time;
}

uint64_t busy_wait(const timespec& start, const uint64_t& period_ns)
{
	uint32_t counter = 0;
	timespec stop, timer;

	clock_gettime(CLOCK_MONOTONIC, &stop);
	clock_gettime(CLOCK_MONOTONIC, &timer);

	uint64_t exec_time = (stop.tv_nsec + ((stop.tv_sec - start.tv_sec) * NSEC_PER_SEC)) - start.tv_nsec;
	uint64_t ns = period_ns - exec_time;

	// cap ns at 0
	if (ns < 0){
		ns = 0;
	}

	// reuse stop to determine the deadline
	stop.tv_nsec += ns;

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
		if(++counter > step_sleep){
			counter = 0;
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timer, NULL);
		}
	}

	return exec_time;
}

}

#endif
