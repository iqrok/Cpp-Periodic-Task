#ifndef _MAIN_ROUTINE_HPP_
#define _MAIN_ROUTINE_HPP_

#include <errno.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sched.h>
#include <unistd.h>

#include <Statistics.hpp>
#include <StatisticsStatic.hpp>
#include <Sleep.hpp>
#include <Timestamp.hpp>

#include <iostream>
#include <cstring>
#include <string>
#include <cstdio>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {

struct DistributionSummary {
	double target;
	double mean;
	double stdDev;
	double min;
	double max;
};

struct routine_data_s {
	uint16_t step_sleep;
	int16_t affinity;
	int schedule_priority;
	int nice_value;
	uint64_t period_ns;
	int64_t offset_ns;
	int64_t exec_time;
	int64_t cycle_time;
	struct timespec timer;
};

constexpr int schedule_priority = SCHED_FIFO;
constexpr int nice_value = -20;
constexpr uint32_t distribution_size = 	500;

const uint16_t step_sleep = 50;

const uint64_t period_ns_busy = 1000'000;
const uint64_t period_ns_sleep = period_ns_busy;

const uint64_t offset_ns = 0;
const uint32_t max_sample_size = distribution_size;
const uint32_t max_loop = 5;

bool isRunning = false;

std::vector<double> sample_sleep;
std::vector<double> sample_busy;

double distribution_sleep[distribution_size];
double distribution_busy[distribution_size];

struct DistributionSummary summary_sleep;
struct DistributionSummary summary_busy;

std::thread thrd_sleep;
std::thread thrd_busy;

uint8_t get_last_cpu(const uint8_t& n)
{
	uint8_t ncpu = sysconf(_SC_NPROCESSORS_CONF);
	return (ncpu - 1) - (n % ncpu);
}

void workload_sin(const std::vector<float>& data, float& result)
{
	float rt = 0;
	for (size_t i = 0; i < data.size(); ++i) {
		rt += std::sin(data[i]);
	}
	result = rt;
}

void routine_sleep(routine_data_s* task)
{
	pid_t tid = gettid();
	uint8_t target_affinity = get_last_cpu(task->affinity);

	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(target_affinity, &cpuset);

    sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);

	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(task->schedule_priority);

	if (sched_setscheduler(tid, task->schedule_priority, &param) == -1) {
		perror("sched_setscheduler failed");
	}

	setpriority(PRIO_PROCESS, tid, task->nice_value);

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "Warning: Failed to lock memory: %s\n", strerror(errno));
	}

	prctl(PR_SET_TIMERSLACK, 1, tid, 0, 0);

	uint32_t index = 0;
	float diff;
	uint64_t period = task->period_ns - task->offset_ns;

	std::vector<float> data = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, };
	float result = 0;
	uint32_t loop_counter = 0;

#if DEBUG > 0
	printf("Starting Sleep Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	Sleep::start_timer(&task->timer);

	while (isRunning) {
		workload_sin(data, result);

		Sleep::wait(task->timer, period, &task->exec_time, &task->cycle_time);
		Sleep::start_timer(&task->timer);

		if(StatisticsStatic::push(distribution_sleep, task->cycle_time, &index, max_sample_size)){
			diff = StatisticsStatic::average(distribution_sleep, max_sample_size) - task->period_ns;

			if(diff / task->period_ns > 0.0025){
				period = task->period_ns - diff;
			}

			if(++loop_counter > max_loop){
				std::cerr << "Sleep Loop Finished! " << diff << "\n";
				break;
			}
		}
	}
}

void routine_busy(routine_data_s* task)
{
	pid_t tid = gettid();
	uint8_t target_affinity = get_last_cpu(task->affinity);

	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(target_affinity, &cpuset);

    sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);

	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(task->schedule_priority);

	if (sched_setscheduler(tid, task->schedule_priority, &param) == -1) {
		perror("sched_setscheduler failed");
	}

	setpriority(PRIO_PROCESS, tid, nice_value);

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "Warning: Failed to lock memory: %s\n", strerror(errno));
	}

	prctl(PR_SET_TIMERSLACK, 1, tid, 0, 0);

	uint32_t index = 0;
	float diff;
	uint64_t period = task->period_ns - task->offset_ns;

	std::vector<float> data = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, };
	float result = 0;
	uint32_t loop_counter = 0;

#if DEBUG > 0
	printf("Starting Busy Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	Sleep::start_timer(&task->timer);

	while (isRunning) {
		workload_sin(data, result);

		Sleep::busy_wait(task->timer, period, &task->exec_time, &task->cycle_time, task->step_sleep);
		Sleep::start_timer(&task->timer);

		if(StatisticsStatic::push(distribution_busy, task->cycle_time, &index, max_sample_size)){
			diff = StatisticsStatic::average(distribution_busy, max_sample_size) - task->period_ns;

			if(diff < 0) diff = -diff;

			if(diff / task->period_ns > 0.0025){
				period = task->period_ns - diff;
			}

			if(++loop_counter > max_loop){
				std::cerr << "Busy Loop Finished! " << diff << "\n";
				break;
			}
		}
	}
}

void print_statistics(const char* title, double* distribution,
							const uint32_t& size, const uint64_t& target_period)
{
	DistributionSummary summary;

	summary.target = target_period;
	StatisticsStatic::calculate(distribution, size, &summary.mean, &summary.stdDev);
	StatisticsStatic::minmax(distribution, size, &summary.min, &summary.max);

	printf("============ %s ============\n", title);
	printf("Sample Size  : %15d\n", size);
	printf("Task Period  : %15.6lf us (%.3lf Hz)\n", summary.target / 1000, 1 / summary.target * 1E9);
	printf("mean         : %15.6lf us (%.3lf Hz)\n", summary.mean / 1000, 1 / summary.mean * 1E9);
	printf("deviation    : %15.6lf us\n", summary.stdDev / 1000);
	printf("min          : %15.6lf us\n", summary.min / 1000);
	printf("max          : %15.6lf us\n", summary.max / 1000);
	printf("diff min max : %15.6lf us\n", (summary.max - summary.min) / 1000);
	printf("%% deviation  : %15.6lf %%\n", 100 * (summary.stdDev / summary.target));
	printf("%% minmax     : %15.6lf %%\n", 100 * ((summary.max - summary.min) / summary.target));
	printf("------------------------------\n");
}

void start()
{
	isRunning = true;

	routine_data_s config_busy = {
		250,
		0,
		SCHED_FIFO,
		-20,
		1'000'000,
		700,
	};

	routine_data_s config_sleep = {
		250,
		1,
		SCHED_FIFO,
		-20,
		1'000'000,
		0,
	};

	thrd_sleep = std::thread(&routine_sleep, &config_sleep);
	thrd_busy = std::thread(&routine_busy, &config_busy);
}

void stop()
{
	isRunning = false;

	printf("\n");

	thrd_sleep.join();
	thrd_busy.join();

	print_statistics("BUSY WAIT", distribution_busy, max_sample_size, period_ns_busy);
	print_statistics("SLEEP", distribution_sleep, max_sample_size, period_ns_busy);
}

}

#endif
