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

constexpr int schedule_priority = SCHED_RR;
constexpr uint32_t distribution_size = 	1'500;

const uint64_t period_ns_busy = 1000'000;
const uint64_t period_ns_sleep = period_ns_busy;

const uint64_t offset_ns = 0;
const uint32_t max_sample_size = distribution_size;
const uint32_t max_loop = 3;

bool isRunning = false;

std::vector<double> sample_sleep;
std::vector<double> sample_busy;

double distribution_sleep[distribution_size];
double distribution_busy[distribution_size];

struct DistributionSummary summary_sleep;
struct DistributionSummary summary_busy;

std::thread thrd_sleep;
std::thread thrd_busy;

uint8_t ncpu = 0;

uint8_t get_last_cpu(const uint8_t& n, const uint8_t& max_cpu)
{
	return max_cpu - 1 - (n % max_cpu);
}

void workload_sin(const std::vector<float>& data, float& result)
{
  float rt = 0;
  for (size_t i = 0; i < data.size(); ++i) {
    rt += std::sin(data[i]);
  }
  result = rt;
}

void routine_sleep()
{
	pid_t tid = gettid();

	uint8_t affinity = 1;
	uint8_t target_affinity = get_last_cpu(affinity, ncpu);

#if DEBUG > 0
	printf("Starting Sleep Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(target_affinity, &cpuset);

    sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);

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

		uint64_t exec_time = Sleep::wait(wakeup_time, period_ns);
		uint64_t pause_time = Timestamp::now_ns() - time_start;

		if(StatisticsStatic::push(distribution_sleep, pause_time, &index, max_sample_size)){
			float_t diff = StatisticsStatic::average(distribution_sleep, max_sample_size) - period_ns_sleep;

			if(diff / period_ns_sleep > 0.005){
				period_ns = period_ns_sleep - diff;
			}

			if(++loop_counter > max_loop){
				std::cerr << "Sleep Loop Finished! " << diff << "\n";
				break;
			}
		}
	}
}

void routine_busy()
{
	pid_t tid = gettid();

	uint8_t affinity = 0;
	uint8_t target_affinity = get_last_cpu(affinity, ncpu);

#if DEBUG > 0
	printf("Starting Busy Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(target_affinity, &cpuset);

    sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset);

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

	std::vector<float> data = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 1.0, };
	float result = 0;

	while (isRunning) {
		Sleep::start_timer(&wakeup_time);
		uint64_t time_start = Timestamp::now_ns();

		workload_sin(data, result);

		uint64_t exec_time = Sleep::busy_wait(wakeup_time, period_ns);
		uint64_t pause_time = Timestamp::now_ns() - time_start;

		if(StatisticsStatic::push(distribution_busy, pause_time, &index, max_sample_size)){
			float diff = StatisticsStatic::average(distribution_busy, max_sample_size) - period_ns_busy;

			if(diff / period_ns_busy > 0.0001){
				period_ns = period_ns_busy - diff;
			}

			if(++loop_counter > max_loop){
				std::cerr << "Busy Loop Finished! " << diff << "\n";
				break;
			}
		}
	}
}

void print_statistics(const char* title, double* distribution, const uint32_t& size, const uint64_t& target_period){
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

	Sleep::set_step_sleep(750);

	ncpu = sysconf(_SC_NPROCESSORS_CONF);

	thrd_sleep = std::thread(&routine_sleep);
	thrd_busy = std::thread(&routine_busy);
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
