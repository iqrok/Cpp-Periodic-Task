#ifndef _TASK_CYCLE_HPP_
#define _TASK_CYCLE_HPP_

#include "Sleep.hpp"
#include "StatisticsStatic.hpp"

#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace TaskCycle {

struct distribution_summary_s {
	float target;
	float mean;
	float standard_deviation;
	float periodic_deviation;
	float min;
	float max;
	float percent_standard_deviation;
	float percent_periodic_deviation;
	float percent_min;
	float percent_max;
	uint32_t size;
};

typedef Sleep::sleep_task_s task_config_t;

uint8_t get_last_cpu(const uint8_t& n)
{
	uint8_t ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	return (ncpu - 1) - (n % ncpu);
}

void set_thread_properties(const pid_t& tid, const task_config_t* task)
{
	if (task->affinity > -1) {
		uint8_t target_affinity = get_last_cpu(task->affinity);

		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(target_affinity, &cpuset);

		if (sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset) == -1) {
			fprintf(stderr, "(%d) sched_setaffinity: %s\n", tid, strerror(errno));
		}
	}

	if (task->schedule_priority > -1) {
		struct sched_param param = {};
		param.sched_priority = sched_get_priority_max(task->schedule_priority) - task->priority_offset;

#if DEBUG > 0
		fprintf(stderr, "(%d) Schedule priority: (%d - %d = %d)\n", tid, sched_get_priority_max(task->schedule_priority), task->priority_offset, param.sched_priority);
#endif

		if (sched_setscheduler(tid, task->schedule_priority, &param) == -1) {
			fprintf(stderr, "(%d) sched_setscheduler: %s\n", tid, strerror(errno));
		}
	}

	if (task->nice_value != 20) {
		if (setpriority(PRIO_PROCESS, tid, task->nice_value) == -1) {
			fprintf(stderr, "(%d) setpriority: %s\n", tid, strerror(errno));
		}
	}

	if (prctl(PR_SET_TIMERSLACK, 1, tid, 0, 0) == -1) {
		fprintf(stderr, "(%d) prctl: %s\n", tid, strerror(errno));
	}
}

void routine_sleep(task_config_t* task, float* samples, const uint32_t& sample_size)
{
	pid_t tid = gettid();
	uint32_t index = 0;
	float diff;
	task->_period_cmp = task->period_ns - task->offset_ns;

	set_thread_properties(tid, task);

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "(%d) Failed to lock memory: %s\n", tid, strerror(errno));
	}

#if DEBUG > 0
	printf("(%d) Starting Sleep Loop thread\n", tid);
#endif

	task->is_running = true;
	Sleep::start_timer(&task->timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::wait(task);
		Sleep::start_timer(&task->timer);

		if (StatisticsStatic::push(samples, task->cycle_time, &index, sample_size)) {
			if (task->tolerance < 0) {
				continue;
			}

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
	uint32_t index = 0;
	float diff;
	task->_period_cmp = task->period_ns - task->offset_ns;

	set_thread_properties(tid, task);

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "(%d) Failed to lock memory: %s\n", tid, strerror(errno));
	}

#if DEBUG > 0
	printf("(%d) Starting Busy Loop thread\n", tid);
#endif

	// disable lazy_sleep if the value is greater than the period
	if (task->lazy_sleep >= task->_period_cmp) {
		task->lazy_sleep = -1;
	}

	task->is_running = true;
	Sleep::start_timer(&task->timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::busy_wait(task);
		Sleep::start_timer(&task->timer);

		if (StatisticsStatic::push(samples, task->cycle_time, &index, sample_size)) {
			if (task->tolerance < 0) {
				continue;
			}

			diff = StatisticsStatic::average(samples, sample_size) - task->period_ns;
			if (fabs(diff / task->period_ns) > task->tolerance) {
				task->_period_cmp = task->period_ns - diff;
			}
		}
	}
}

void stats_summarize(struct distribution_summary_s* summary, float* distribution,
	const uint32_t& sample_size, const uint64_t& target_period)
{
	summary->target = target_period;
	summary->size = sample_size;

	StatisticsStatic::calculate(
		distribution, sample_size, summary->target, &summary->mean,
		&summary->standard_deviation, &summary->periodic_deviation,
		&summary->min, &summary->max);

	summary->percent_periodic_deviation = summary->periodic_deviation / summary->target;
	summary->percent_standard_deviation = summary->periodic_deviation / summary->mean;
	summary->percent_min = fabs(summary->min - summary->target) / summary->target;
	summary->percent_max = fabs(summary->max - summary->target) / summary->target;
}

void stats_print(const char* title, const struct distribution_summary_s& summary)
{
	printf("============ %s ============\n", title);
	printf("Sample Size  : %15d\n", summary.size);
	printf("Task Period  : %15.3f us ± %9.3f us (%6.3f %%, %9.3f Hz )\n",
		summary.target / 1000, summary.periodic_deviation / 1000,
		100 * summary.percent_periodic_deviation, 1E9 / summary.target);

	printf("mean         : %15.3f us ± %9.3f us (%6.3f %%, %9.3f Hz )\n",
		summary.mean / 1000, summary.standard_deviation / 1000,
		100 * summary.percent_standard_deviation, 1E9 / summary.mean);

	printf("min          : %15.3f us (%9.3f %% )\n", summary.min / 1000,
		100 * summary.percent_min);

	printf("max          : %15.3f us (%9.3f %% )\n", summary.max / 1000,
		100 * summary.percent_max);

	printf("diff min max : %15.3f us (%9.3f %% )\n",
		(summary.max - summary.min) / 1000,
		100 * (
			(fabs(summary.min - summary.target) + (fabs(summary.max - summary.target)))
				/ summary.target));

	printf("------------------------------\n");
}

}

#endif
