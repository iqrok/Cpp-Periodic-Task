#ifndef _MAIN_ROUTINE_HPP_
#define _MAIN_ROUTINE_HPP_

#include <sys/resource.h>
#include <sys/types.h>

#include <Statistics.hpp>
#include <sleep.hpp>
#include <timestamp.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {

constexpr int schedule_priority = SCHED_RR;

const uint64_t period_ns_busy = 500'000;
const uint64_t period_ns_sleep = 500'000;

const uint64_t offset_ns = 0;
const uint32_t max_sample_size = 5'000;

bool isRunning = false;

std::vector<double> sample_sleep;
std::vector<double> sample_busy;

std::thread thrd_sleep;
std::thread thrd_busy;

void routine_sleep()
{
	pid_t tid = gettid();

#if DEBUG > 0
	printf("Starting Sleep Loop thread %d\n", tid);
#endif

	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(schedule_priority);

	if (sched_setscheduler(tid, schedule_priority, &param) == -1) {
		perror("sched_setscheduler failed");
	}

	setpriority(PRIO_PROCESS, tid, -1);

	struct timespec wakeup_time;
	uint32_t index = 0;
	uint64_t period_ns = period_ns_sleep - offset_ns;

	while (isRunning) {
		Sleep::start_timer(&wakeup_time);
		uint64_t time_start = Timestamp::now_ns();

		double x = 1000 * 0.1234325325;

		uint64_t exec_time = Sleep::wait(wakeup_time, period_ns);
		uint64_t pause_time = Timestamp::now_ns() - time_start;

		Statistics::push(&sample_sleep, pause_time, &index, max_sample_size);
	}
}

void routine_busy()
{
	pid_t tid = gettid();

#if DEBUG > 0
	printf("Starting Busy Loop thread %d\n", tid);
#endif

	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(schedule_priority);

	if (sched_setscheduler(tid, schedule_priority, &param) == -1) {
		perror("sched_setscheduler failed");
	}

	setpriority(PRIO_PROCESS, tid, -1);

	struct timespec wakeup_time;
	uint32_t index = 0;
	uint64_t period_ns = period_ns_busy - offset_ns;

	while (isRunning) {
		Sleep::start_timer(&wakeup_time);
		uint64_t time_start = Timestamp::now_ns();

		double x = 1000 * 0.1234325325;

		uint64_t exec_time = Sleep::busy_wait(wakeup_time, period_ns);
		uint64_t pause_time = Timestamp::now_ns() - time_start;

		Statistics::push(&sample_busy, pause_time, &index, max_sample_size);
	}
}

void start()
{
	isRunning = true;
	thrd_sleep = std::thread(&routine_sleep);
	thrd_busy = std::thread(&routine_busy);
}

void stop()
{
	isRunning = false;

	thrd_sleep.join();
	thrd_busy.join();

	Statistics::print_stats(sample_sleep, "Sleep loop", true);
	Statistics::print_stats(sample_busy, "Busy loop", false);
}

}

#endif
