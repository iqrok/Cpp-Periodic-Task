#ifndef _MAIN_ROUTINE_HPP_
#define _MAIN_ROUTINE_HPP_

#include <errno.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include <Statistics.hpp>
#include <StatisticsStatic.hpp>
#include <Sleep.hpp>
#include <Timestamp.hpp>

#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {

constexpr int schedule_priority = SCHED_FIFO;
constexpr uint32_t distribution_size = 2'500;

const uint64_t period_ns_busy = 1'000'000;
const uint64_t period_ns_sleep = period_ns_busy;

const uint64_t offset_ns = 0;
const uint32_t max_sample_size = distribution_size;
const uint32_t max_loop = 4;

bool isRunning = false;

std::vector<double> sample_sleep;
std::vector<double> sample_busy;

double distribution_sleep[distribution_size];
double distribution_busy[distribution_size];

std::thread thrd_sleep;
std::thread thrd_busy;

void workload_sin(const std::vector<float>& data, float& result) {
  float rt = 0;
  for (size_t i = 0; i < data.size(); ++i) {
    rt += std::sin(data[i]);
  }
  result = rt;
}

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

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "Warning: Failed to lock memory: %s\n", strerror(errno));
	}

	prctl(PR_SET_TIMERSLACK, 1, tid, 0, 0);

	uint32_t loop_counter = 0;

	struct timespec wakeup_time;
	uint32_t index = 0;
	uint64_t period_ns = period_ns_sleep - offset_ns;

	std::vector<float> data = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0 };
	float result = 0;

	while (isRunning) {
		Sleep::start_timer(&wakeup_time);
		uint64_t time_start = Timestamp::now_ns();

		workload_sin(data, result);
		/* double x = 1000 * 0.1234325325; */

		uint64_t exec_time = Sleep::wait(wakeup_time, period_ns);
		uint64_t pause_time = Timestamp::now_ns() - time_start;

		//~ Statistics::push(&sample_sleep, pause_time, &index, max_sample_size);

		if(StatisticsStatic::push(distribution_sleep, pause_time, &index, max_sample_size)){
			float_t diff = StatisticsStatic::average(distribution_sleep, max_sample_size) - period_ns_sleep;

			if(diff / period_ns_sleep > 0.005){
				period_ns = period_ns_sleep - diff;
			}

			//~ StatisticsStatic::print_stats(distribution_sleep, max_sample_size, "Sleep loop", false);
			//~ if(++loop_counter > max_loop){
				//~ std::cerr << "Sleep Loop Finished! " << diff << "\n";
				//~ break;
			//~ }
		}
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

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "Warning: Failed to lock memory: %s\n", strerror(errno));
	}

	prctl(PR_SET_TIMERSLACK, 1, tid, 0, 0);

	uint32_t loop_counter = 0;

	struct timespec wakeup_time;
	uint32_t index = 0;
	uint64_t period_ns = period_ns_busy - offset_ns;

	std::vector<float> data = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0 };
	float result = 0;

	while (isRunning) {
		Sleep::start_timer(&wakeup_time);
		uint64_t time_start = Timestamp::now_ns();

		workload_sin(data, result);

		uint64_t exec_time = Sleep::busy_wait(wakeup_time, period_ns);
		uint64_t pause_time = Timestamp::now_ns() - time_start;

		//~ if(Statistics::push(&sample_busy, pause_time, &index, max_sample_size)){
			//~ Statistics::print_stats(sample_busy, "Busy loop", false);
			//~ float_t diff = Statistics::average(sample_busy) - period_ns_busy;

			//~ if(diff / period_ns_busy > 0.005){
				//~ period_ns = period_ns_busy - diff;
			//~ }
		//~ }

		if(StatisticsStatic::push(distribution_busy, pause_time, &index, max_sample_size)){
			float_t diff = StatisticsStatic::average(distribution_busy, max_sample_size) - period_ns_busy;

			if(diff / period_ns_busy > 0.005){
				period_ns = period_ns_busy - diff;
			}

			StatisticsStatic::print_stats(distribution_busy, max_sample_size, "Busy loop", false);

			//~ if(++loop_counter > max_loop){
				//~ std::cerr << "Busy Loop Finished! " << diff << "\n";
				//~ break;
			//~ }
		}
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

	//~ Statistics::print_stats(sample_sleep, "Sleep loop", true);
	//~ Statistics::print_stats(sample_busy, "Busy loop", false);

	StatisticsStatic::print_stats(distribution_sleep, max_sample_size, "Sleep loop", true);
	StatisticsStatic::print_stats(distribution_busy, max_sample_size, "Busy loop", false);
}

}

#endif
