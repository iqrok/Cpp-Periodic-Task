#ifndef _MAIN_ROUTINE_HPP_
#define _MAIN_ROUTINE_HPP_

#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include <Sleep.hpp>
#include <StatisticsStatic.hpp>
#include <Timestamp.hpp>

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {

constexpr uint32_t sample_size = 500;

struct DistributionSummary {
	float target;
	float mean;
	float stdDev;
	float min;
	float max;
};

struct task_config_s {
	bool is_running;
	int8_t affinity;
	uint16_t step_sleep;
	int schedule_priority;
	int nice_value;
	float tolerance;
	uint64_t period_ns;
	int64_t offset_ns;
	int64_t exec_time;
	int64_t cycle_time;
	struct timespec timer;
	float distribution[sample_size];
	void (*fptr)(void);
};

task_config_s config_busy = {
	false,
	0,
	150,
	SCHED_FIFO,
	-20,
	0.005,
	1'000'000,
	700,
};

task_config_s config_sleep = {
	false,
	1,
	0,
	SCHED_FIFO,
	-20,
	0.005,
	10'000'000,
	1'000,
};

double result_sleep = 0;
double result_busy = 0;
std::vector<double> data = {
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

std::thread thrd_sleep;
std::thread thrd_busy;

uint8_t get_last_cpu(const uint8_t& n)
{
	uint8_t ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	return (ncpu - 1) - (n % ncpu);
}

void workload_sin(const std::vector<double>& data, double& result)
{
	double rt = 0;
	for (size_t i = 0; i < data.size(); ++i) {
		rt += std::sin(data[i]);
	}
	result = rt;
}

void routine_sleep(task_config_s* task)
{
	pid_t tid = gettid();
	uint8_t target_affinity = get_last_cpu(task->affinity);

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(target_affinity, &cpuset);

	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(task->schedule_priority);

	if (sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset) == -1) {
		fprintf(stderr, "(%d) sched_setaffinity: %s\n", tid, strerror(errno));
	}

	if (sched_setscheduler(tid, task->schedule_priority, &param) == -1) {
		fprintf(stderr, "(%d) sched_setscheduler: %s\n", tid, strerror(errno));
	}

	if (setpriority(PRIO_PROCESS, tid, task->nice_value) == -1) {
		fprintf(stderr, "(%d) setpriority: %s\n", tid, strerror(errno));
	}

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "(%d) Failed to lock memory: %s\n", tid, strerror(errno));
	}

	if (prctl(PR_SET_TIMERSLACK, 1, tid, 0, 0) == -1) {
		fprintf(stderr, "(%d) prctl: %s\n", tid, strerror(errno));
	}

	uint32_t index = 0;
	float diff;
	uint64_t period = task->period_ns - task->offset_ns;

#if DEBUG > 0
	printf("Starting Sleep Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	task->is_running = true;
	Sleep::start_timer(&task->timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::wait(task->timer, period, &task->exec_time, &task->cycle_time);
		Sleep::start_timer(&task->timer);

		if (StatisticsStatic::push(task->distribution, task->cycle_time, &index, sample_size)) {
			diff = StatisticsStatic::average(task->distribution, sample_size) - task->period_ns;

			if (fabs(diff / task->period_ns) > task->tolerance) {
				period = task->period_ns - diff;
			}
		}
	}
}

void routine_busy(task_config_s* task)
{
	pid_t tid = gettid();
	uint8_t target_affinity = get_last_cpu(task->affinity);

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(target_affinity, &cpuset);

	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(task->schedule_priority);

	if (sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset) == -1) {
		fprintf(stderr, "(%d) sched_setaffinity: %s\n", tid, strerror(errno));
	}

	if (sched_setscheduler(tid, task->schedule_priority, &param) == -1) {
		fprintf(stderr, "(%d) sched_setscheduler: %s\n", tid, strerror(errno));
	}

	if (setpriority(PRIO_PROCESS, tid, task->nice_value) == -1) {
		fprintf(stderr, "(%d) setpriority: %s\n", tid, strerror(errno));
	}

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "(%d) Failed to lock memory: %s\n", tid, strerror(errno));
	}

	if (prctl(PR_SET_TIMERSLACK, 1, tid, 0, 0) == -1) {
		fprintf(stderr, "(%d) prctl: %s\n", tid, strerror(errno));
	}

	uint32_t index = 0;
	float diff;
	uint64_t period = task->period_ns - task->offset_ns;

#if DEBUG > 0
	printf("Starting Busy Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	task->is_running = true;
	Sleep::start_timer(&task->timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::busy_wait(task->timer, period, &task->exec_time, &task->cycle_time, task->step_sleep);
		Sleep::start_timer(&task->timer);

		if (StatisticsStatic::push(task->distribution, task->cycle_time, &index, sample_size)) {
			diff = StatisticsStatic::average(task->distribution, sample_size) - task->period_ns;

			if (fabs(diff / task->period_ns) > task->tolerance) {
				period = task->period_ns - diff;
			}
		}
	}
}

void print_statistics(const char* title, float* distribution,
	const uint32_t& size, const uint64_t& target_period)
{
	DistributionSummary summary;

	summary.target = target_period;
	StatisticsStatic::calculate(distribution, size, &summary.mean, &summary.stdDev, &summary.min, &summary.max);

	printf("============ %s ============\n", title);
	printf("Sample Size  : %15d\n", size);
	printf("Task Period  : %15.6f us (%.3f Hz)\n", summary.target / 1000, 1 / summary.target * 1E9);
	printf("mean         : %15.6f us (%.3f Hz)\n", summary.mean / 1000, 1 / summary.mean * 1E9);
	printf("deviation    : %15.6f us\n", summary.stdDev / 1000);
	printf("min          : %15.6f us\n", summary.min / 1000);
	printf("max          : %15.6f us\n", summary.max / 1000);
	printf("diff min max : %15.6f us\n", (summary.max - summary.min) / 1000);
	printf("%% deviation  : %15.6f %%\n", 100 * (summary.stdDev / summary.target));
	printf("%% minmax     : %15.6f %%\n", 100 * ((summary.max - summary.min) / summary.target));
	printf("------------------------------\n");
}

void workload_busy(void){
	workload_sin(data, result_busy);
}

void workload_sleep(void){
	workload_sin(data, result_sleep);
}

void start()
{
	config_busy.fptr = workload_busy;
	config_sleep.fptr = workload_sleep;

	thrd_sleep = std::thread(&routine_sleep, &config_sleep);
	thrd_busy = std::thread(&routine_busy, &config_busy);
}

void stop()
{
	config_busy.is_running = false;
	config_sleep.is_running = false;

	thrd_sleep.join();
	thrd_busy.join();

	printf("\n");
	print_statistics("BUSY WAIT", config_busy.distribution, sample_size, config_busy.period_ns);
	print_statistics("SLEEP", config_sleep.distribution, sample_size, config_sleep.period_ns);

	printf("workload_busy %ld\n", config_busy.exec_time);
	printf("workload_sleep %ld\n", config_sleep.exec_time);
}

}

#endif
