#ifndef _TASK_CYCLE_HPP_
#define _TASK_CYCLE_HPP_

#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include "Sleep.hpp"
#include "StatisticsStatic.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace TaskCycle {

struct distribution_summary_s {
	float target;
	float mean;
	float stdDev;
	float min;
	float max;
};

typedef Sleep::task_config_s task_config_t;

uint8_t get_last_cpu(const uint8_t& n)
{
	uint8_t ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	return (ncpu - 1) - (n % ncpu);
}

void routine_sleep(task_config_t* task, float* samples, const uint32_t& sample_size)
{
	pid_t tid = gettid();
	uint8_t target_affinity = get_last_cpu(task->affinity);

	uint32_t index = 0;
	float diff;
	task->_period_cmp = task->period_ns - task->offset_ns;

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

#if DEBUG > 0
	printf("Starting Sleep Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	task->is_running = true;
	Sleep::start_timer(&task->timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::wait(task);
		Sleep::start_timer(&task->timer);

		if (StatisticsStatic::push(samples, task->cycle_time, &index, sample_size)) {
			diff = StatisticsStatic::average(samples, sample_size) - task->period_ns;

			if (fabs(diff / task->period_ns) > task->tolerance) {
				task->_period_cmp = task->period_ns - diff;
			}
		}
	}
}

void routine_busy(task_config_t* task, float* samples, const uint32_t& sample_size)
{
	pid_t tid = gettid();
	uint8_t target_affinity = get_last_cpu(task->affinity);

	uint32_t index = 0;
	float diff;
	task->_period_cmp = task->period_ns - task->offset_ns;

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

#if DEBUG > 0
	printf("Starting Busy Loop thread %d on CPU %d\n", tid, target_affinity);
#endif

	task->is_running = true;
	Sleep::start_timer(&task->timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::busy_wait(task);
		Sleep::start_timer(&task->timer);

		if (StatisticsStatic::push(samples, task->cycle_time, &index, sample_size)) {
			diff = StatisticsStatic::average(samples, sample_size) - task->period_ns;

			if (fabs(diff / task->period_ns) > task->tolerance) {
				task->_period_cmp = task->period_ns - diff;
			}
		}
	}
}

void print_statistics(const char* title, float* distribution,
	const uint32_t& sample_size, const uint64_t& target_period)
{
	distribution_summary_s summary;

	summary.target = target_period;
	StatisticsStatic::calculate(distribution, sample_size, &summary.mean, &summary.stdDev, &summary.min, &summary.max);

	printf("============ %s ============\n", title);
	printf("Sample Size  : %15d\n", sample_size);
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

}

#endif
