#ifndef _MAIN_ROUTINE_HPP_
#define _MAIN_ROUTINE_HPP_

#include <TaskCycle.hpp>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {

constexpr uint32_t sample_size = 500;

float samples_sleep[sample_size];
float samples_busy[sample_size];

TaskCycle::task_config_t config_busy = {
	false,
	0,
	250,
	SCHED_FIFO,
	-20,
	0.005,
	300'000,
	500'000,
	700,
};

TaskCycle::task_config_t config_sleep = {
	false,
	1,
	150,
	SCHED_FIFO,
	-20,
	0.005,
	300'000,
	1'000'000,
	735,
};

double result_sleep = 0;
std::vector<double> data_sleep = {
	0.1, 0.2, 0.3, 0.4, 0.5,
	0.6, 0.7, 0.8, 1.0, 0.1,
	0.2, 0.3, 0.4, 0.5, 0.6,
	0.7, 0.8, 1.0, 0.1, 0.2,
	0.3, 0.4, 0.5, 0.6, 0.7,
	0.8, 1.0, 0.1, 0.2, 0.3,
	0.4, 0.5, 0.6, 0.7, 0.8,
	1.0, 0.1, 0.2, 0.3, 0.4,
	0.5, 0.6, 0.7, 0.8, 1.0,
	0.1, 0.2, 0.3, 0.4, 0.5,
	0.6, 0.7, 0.8, 1.0, 0.1,
	0.2, 0.3, 0.4, 0.5, 0.6,
	0.7, 0.8, 1.0, 0.1, 0.2,
	0.3, 0.4, 0.5, 0.6, 0.7,
	0.8, 1.0, 0.5, 0.6, 0.7,
};

double result_busy = 0;
std::vector<double> data_busy = {
	1.1, 1.2, 1.3, 1.4, 1.5,
	1.6, 1.7, 1.8, 1.0, 1.1,
	1.2, 1.3, 1.4, 1.5, 1.6,
	1.7, 1.8, 1.0, 1.1, 1.2,
	1.3, 1.4, 1.5, 1.6, 1.7,
	1.8, 1.0, 1.1, 1.2, 1.3,
	1.4, 1.5, 1.6, 1.7, 1.8,
	1.0, 1.1, 1.2, 1.3, 1.4,
	1.5, 1.6, 1.7, 1.8, 1.0,
	1.1, 1.2, 1.3, 1.4, 1.5,
	1.6, 1.7, 1.8, 1.0, 1.1,
	1.2, 1.3, 1.4, 1.5, 1.6,
	1.7, 1.8, 1.0, 1.1, 1.2,
	1.3, 1.4, 1.5, 1.6, 1.7,
	1.8, 1.0, 1.5, 1.6, 1.7,
};

std::thread thrd_sleep;
std::thread thrd_busy;

void workload_sin(const std::vector<double>& data, double& result)
{
	double rt = 0;
	for (size_t i = 0; i < data.size(); ++i) {
		rt *= std::sin(data[i]);
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
	config_sleep.fptr = workload_sleep;
	config_busy.fptr = workload_busy;

	thrd_sleep = std::thread(&TaskCycle::routine_sleep, &config_sleep, samples_sleep, sample_size);
	thrd_busy = std::thread(&TaskCycle::routine_busy, &config_busy, samples_busy, sample_size);
}

void stop()
{
	config_busy.is_running = false;
	config_sleep.is_running = false;

	thrd_sleep.join();
	thrd_busy.join();

	printf("\n");

	TaskCycle::print_statistics("BUSY WAIT", samples_busy, sample_size, config_busy.period_ns);
	TaskCycle::print_statistics("SLEEP", samples_sleep, sample_size, config_sleep.period_ns);

	printf("workload_busy %lf\n", result_busy);
	printf("workload_sleep %lf\n", result_sleep);
}

}

#endif
