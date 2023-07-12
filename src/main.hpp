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

const uint64_t period_ns_io = 500'000;

const uint64_t offset_ns = 7'500;
//~ const uint64_t offset_ns = 10000;
const uint32_t max_sample_size = 5'000;

std::vector<double> sample_io;
std::vector<double> diff_sample_io;

bool isRunning = false;

std::thread thrd_io;

void routine()
{
	pid_t tid = gettid();

#if DEBUG > 0
	printf("Starting Ventilation IO thread %d\n", tid);
#endif

	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);

	if (sched_setscheduler(tid, SCHED_FIFO, &param) == -1) {
		perror("sched_setscheduler failed");
	}

	setpriority(PRIO_PROCESS, tid, -1);

	struct timespec wakeup_time;
	uint32_t index = 0;
	uint64_t period_ns = period_ns_io - offset_ns;

	while (isRunning) {
		Sleep::start_timer(&wakeup_time);
		uint64_t time_start = Timestamp::now_ns();

		double x = 1000 * 0.1234325325;

		//~ uint64_t exec_time = Sleep::wait_for_given_period(wakeup_time, period_ns);
		uint64_t exec_time = Sleep::busy_wait_for_given_period(wakeup_time, period_ns);
		uint64_t pause_time = Timestamp::now_ns() - time_start;
		//~ printf(" %ld\n", pause_time);

		Statistics::push(&sample_io, pause_time, &index, max_sample_size);
		Statistics::push(&diff_sample_io, exec_time, &index, max_sample_size);
	}
}

void start()
{
	isRunning = true;
	thrd_io = std::thread(&routine);
}

void stop()
{
	isRunning = false;
	thrd_io.join();

	Statistics::print_stats(sample_io, "IO & Control loop", true);
	Statistics::print_stats(diff_sample_io, "IO & Control exec time", false);
}

}

#endif
