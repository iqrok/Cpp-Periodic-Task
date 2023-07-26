#ifndef _CYCLE_TEST_HPP_
#define _CYCLE_TEST_HPP_

#include <TaskCycle.hpp>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {

constexpr uint32_t sample_size = 750;

struct TaskCycle::distribution_summary_s summary_busy;
float samples_busy[sample_size];
TaskCycle::task_config_t config_busy;
std::thread thrd_busy;

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

struct TaskCycle::distribution_summary_s summary_sleep;
float samples_sleep[sample_size];
TaskCycle::task_config_t config_sleep;
std::thread thrd_sleep;

double result_sleep = 0;
std::vector<double> data_sleep = {
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

void workload_sleep(void)
{
	workload_sin(data_sleep, result_sleep);
}

void start()
{
	config_busy.affinity = 1;
	config_busy.step_sleep = 350;
	config_busy.schedule_priority = SCHED_FIFO;
	config_busy.nice_value = -20;
	config_busy.tolerance = -1;
	config_busy.lazy_sleep = 650'000;
	config_busy.period_ns = 1'000'000;
	config_busy.offset_ns = 750,
	config_busy.fptr = workload_busy;

	config_sleep.affinity = 0;
	config_sleep.schedule_priority = SCHED_FIFO;
	config_sleep.nice_value = -20;
	config_busy.tolerance = 0.05;
	config_sleep.period_ns = 1'000'000;
	config_sleep.offset_ns = 700,
	config_sleep.fptr = workload_sleep;

	thrd_sleep = std::thread(&TaskCycle::routine_sleep, &config_sleep,
		samples_sleep, sample_size);
	thrd_busy = std::thread(&TaskCycle::routine_busy, &config_busy,
		samples_busy, sample_size);
}

void stop()
{
	config_busy.is_running = false;
	config_sleep.is_running = false;

	thrd_sleep.join();
	thrd_busy.join();

	TaskCycle::stats_summarize(&summary_busy, samples_busy, sample_size,
		config_busy.period_ns);

	TaskCycle::stats_summarize(&summary_sleep, samples_sleep, sample_size,
		config_sleep.period_ns);

	printf("\n");

	TaskCycle::stats_print("BUSY WAIT", summary_busy);
	TaskCycle::stats_print("SLEEP", summary_sleep);

	printf("workload busy %lf\n", result_busy);
	printf("workload sleep %lf\n", result_sleep);
}

}

#endif
