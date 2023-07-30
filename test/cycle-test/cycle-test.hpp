#ifndef _CYCLE_TEST_HPP_
#define _CYCLE_TEST_HPP_

#include <TaskCycle.hpp>

#include <sys/mman.h>
#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {

void workload_sin(const std::vector<double>& data, double& result);
void workload_busy(void);
void workload_busy2(void);
void workload_deadline(void);

constexpr uint32_t sample_size = 500;

struct TaskConfigurations {
	struct TaskCycle::distribution_summary_s summary;
	float samples[sample_size];
	TaskCycle::task_config_t config;
	std::thread thread;
	std::string name;
};

struct TaskConfigurations tBusy, tBusy2, tDeadline, tDeadline2;

double result_busy = 0;
std::vector<double> data_busy = {
	1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.0, 1.1,
	1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.0, 1.1, 1.2,
	1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.0, 1.1, 1.2, 1.3,
	1.4, 1.5, 1.6, 1.7, 1.8, 1.0, 1.1, 1.2, 1.3, 1.4,
	1.5, 1.6, 1.7, 1.8, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5,
	1.6, 1.7, 1.8, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6,
	1.7, 1.8, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7,
	1.8, 1.0, 1.5, 1.6, 1.7, 1.5, 1.6, 1.7, 1.8, 1.0,
	1.1, 1.2, 1.3, 1.4, 1.5, 1.7, 1.8, 2.0, 1.1, 1.2,
	1.7, 1.8, 2.0, 1.1, 1.2, 1.1, 1.2, 1.5, 1.6, 1.7,
};

double result_busy2 = 0;
std::vector<double> data_busy2 = {
	0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1,
	0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2,
	0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3,
	0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4,
	0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5,
	0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
	0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7,
	0.8, 1.0, 0.5, 0.6, 0.7, 0.6, 0.7, 0.8, 1.0, 0.1,
	0.7, 0.8, 1.0, 0.1, 0.2, 0.5, 0.6, 0.7, 0.6, 0.7,
	0.4, 0.5, 0.6, 0.7, 0.8, 0.7, 0.6, 0.7, 0.8, 1.0,
};

double result_deadline = 0;
double result_deadline2 = 0;
std::vector<double> data_deadline = {
	0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1,
	0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2,
	0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3,
	0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4,
	0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5,
	0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
	0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7,
	0.8, 1.0, 0.5, 0.6, 0.7, 0.6, 0.7, 0.8, 1.0, 0.1,
	0.7, 0.8, 1.0, 0.1, 0.2, 0.5, 0.6, 0.7, 0.6, 0.7,
	0.4, 0.5, 0.6, 0.7, 0.8, 0.7, 0.6, 0.7, 0.8, 1.0,
};

void workload_sin(const std::vector<double>& data, double& result)
{
	double rt = 0;
	for (size_t i = 0; i < data.size(); ++i) {
		rt += data[i] * std::sin(data[i]);
	}
	result = rt;
}

void workload_busy(void)
{
	workload_sin(data_busy, result_busy);
}

void workload_busy2(void)
{
	workload_sin(data_busy2, result_busy2);
}

void workload_deadline(void)
{
	workload_sin(data_deadline, result_deadline);
}

void workload_deadline2(void)
{
	workload_sin(data_deadline, result_deadline2);
}

void start()
{
	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "Failed to lock memory: %s\n", strerror(errno));
	}

	tBusy.name = "BUSY 1000 us";
	tBusy.config.affinity = 0;
	tBusy.config.schedule_priority = SCHED_RR;
	tBusy.config.nice_value = -20;
	tBusy.config.tolerance = -1;
	tBusy.config.period_ns = 1'000'000;
	tBusy.config.cycle.lazy_sleep = tBusy.config.period_ns - 350'000;
	tBusy.config.cycle.step_sleep = 350;
	tBusy.config.offset_ns = 750,
	tBusy.config.fptr = workload_busy;

	tBusy2.name = "BUSY 5000 us";
	tBusy2.config.affinity = 0;
	tBusy2.config.schedule_priority = SCHED_RR;
	tBusy2.config.priority_offset = 1;
	tBusy2.config.nice_value = -20;
	tBusy2.config.tolerance = -1;
	tBusy2.config.period_ns = 5'000'000;
	tBusy2.config.cycle.lazy_sleep = tBusy2.config.period_ns - 1'000'000;
	tBusy2.config.cycle.step_sleep = 50;
	tBusy2.config.offset_ns = 750,
	tBusy2.config.fptr = workload_busy2;

	tDeadline.name = "DEADLINE 1000 us";
	tDeadline.config.nice_value = -20;
	tDeadline.config.period_ns = 1'000'000;
	tDeadline.config.offset_ns = 0;
	tDeadline.config.cycle.exec_time = 100'000;
	tDeadline.config.deadline_time = tDeadline.config.period_ns - 500'000;
	tDeadline.config.fptr = workload_deadline;

	tDeadline2.name = "DEADLINE 5000 us";
	tDeadline2.config.nice_value = -20;
	tDeadline2.config.period_ns = 5'000'000;
	tDeadline2.config.offset_ns = 0;
	tDeadline2.config.cycle.exec_time = 100'000;
	tDeadline2.config.deadline_time = tDeadline2.config.period_ns - 250'000;
	tDeadline2.config.fptr = workload_deadline2;

	tDeadline.thread = std::thread(&TaskCycle::routine_deadline, &tDeadline.config,
		tDeadline.samples, sample_size);
	pthread_setname_np(tDeadline.thread.native_handle(), tDeadline.name.c_str());

	tBusy.thread = std::thread(&TaskCycle::routine_busy, &tBusy.config,
		tBusy.samples, sample_size);
	pthread_setname_np(tBusy.thread.native_handle(), tBusy.name.c_str());

	tDeadline2.thread = std::thread(&TaskCycle::routine_deadline, &tDeadline2.config,
		tDeadline2.samples, sample_size);
	pthread_setname_np(tDeadline2.thread.native_handle(), tDeadline2.name.c_str());

	tBusy2.thread = std::thread(&TaskCycle::routine_busy, &tBusy2.config,
		tBusy2.samples, sample_size);
	pthread_setname_np(tBusy2.thread.native_handle(), tBusy2.name.c_str());
}

void stop()
{
	tBusy.config.is_running = false;
	tBusy2.config.is_running = false;
	tDeadline.config.is_running = false;
	tDeadline2.config.is_running = false;

	tDeadline.thread.join();
	tDeadline2.thread.join();
	tBusy2.thread.join();
	tBusy.thread.join();

	TaskCycle::stats_summarize(&tBusy.summary, tBusy.samples, sample_size,
		tBusy.config.period_ns);

	TaskCycle::stats_summarize(&tBusy2.summary, tBusy2.samples, sample_size,
		tBusy2.config.period_ns);

	TaskCycle::stats_summarize(&tDeadline.summary, tDeadline.samples, sample_size,
		tDeadline.config.period_ns);

	TaskCycle::stats_summarize(&tDeadline2.summary, tDeadline2.samples, sample_size,
		tDeadline2.config.period_ns);

	printf("\n");

	TaskCycle::stats_print(tBusy.name.c_str(), tBusy.summary);
	TaskCycle::stats_print(tDeadline.name.c_str(), tDeadline.summary);
	TaskCycle::stats_print(tBusy2.name.c_str(), tBusy2.summary);
	TaskCycle::stats_print(tDeadline2.name.c_str(), tDeadline2.summary);

	printf("(%d) %20s %16.6lf\t%16ld loop\n", tBusy.config.tid, tBusy.name.c_str(), result_busy, tBusy.config.ncycle);
	printf("(%d) %20s %16.6lf\t%16ld loop\n", tDeadline.config.tid, tDeadline.name.c_str(), result_deadline, tDeadline.config.ncycle);
	printf("(%d) %20s %16.6lf\t%16ld loop\n", tBusy2.config.tid, tBusy2.name.c_str(), result_busy2, tBusy2.config.ncycle);
	printf("(%d) %20s %16.6lf\t%16ld loop\n", tDeadline2.config.tid, tDeadline2.name.c_str(), result_deadline2, tDeadline2.config.ncycle);
}

}

#endif
