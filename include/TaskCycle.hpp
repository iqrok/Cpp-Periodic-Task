#ifndef _TASK_CYCLE_HPP_
#define _TASK_CYCLE_HPP_

#include "Sleep.hpp"
#include "StatisticsStatic.hpp"
#include "TimespecHelper.hpp"

#include <linux/sched.h>
#include <sys/syscall.h>

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

struct sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

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

typedef struct task_config_s {
	bool is_running = false;
	int8_t schedule_priority = -1;
	int8_t affinity = -1;
	int8_t nice_value = 20;
	pid_t tid;
	float tolerance = -1;
	int32_t priority_offset = 0;
	uint64_t ncycle = 0;
	uint64_t period_ns;
	int64_t offset_ns = 0;
	int64_t deadline_time = -1;
	void (*fptr)(void);
	Sleep::sleep_task_s cycle;
} task_config_t;

uint8_t get_last_cpu(const uint8_t& n)
{
	uint8_t ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	return (ncpu - 1) - (n % ncpu);
}

void set_thread_properties(const task_config_t* task)
{
	if (task->affinity > -1) {
		uint8_t target_affinity = get_last_cpu(task->affinity);

		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(target_affinity, &cpuset);

		if (sched_setaffinity(task->tid, sizeof(cpu_set_t), &cpuset) == -1) {
			fprintf(stderr, "(%d) sched_setaffinity: %s\n", task->tid, strerror(errno));
		}
#if DEBUG > 0
		else {
			fprintf(stderr, "(%d) sched_setaffinity: CPU %d\n", task->tid, target_affinity);
		}
#endif
	}

	if (task->schedule_priority > -1) {
		struct sched_param param = {};
		param.sched_priority = sched_get_priority_max(task->schedule_priority) - task->priority_offset;

		if (sched_setscheduler(task->tid, task->schedule_priority, &param) == -1) {
			fprintf(stderr, "(%d) sched_setscheduler: %s\n", task->tid, strerror(errno));
		}
#if DEBUG > 0
		else {
			fprintf(stderr, "(%d) Schedule priority: %d\n", task->tid, param.sched_priority);
		}
#endif
	}

	if (task->nice_value != 20) {
		if (setpriority(PRIO_PROCESS, task->tid, task->nice_value) == -1) {
			fprintf(stderr, "(%d) setpriority: %s\n", task->tid, strerror(errno));
		}
#if DEBUG > 0
		else {
			fprintf(stderr, "(%d) Nice Value: %d\n", task->tid, task->nice_value);
		}
#endif
	}

	if (prctl(PR_SET_TIMERSLACK, 1, task->tid, 0, 0) == -1) {
		fprintf(stderr, "(%d) prctl: %s\n", task->tid, strerror(errno));
	}
}

void routine_sleep(task_config_t* task, float* samples, const uint32_t& sample_size)
{
	task->tid = gettid();
	uint32_t index = 0;
	float diff;
	task->cycle.period_cmp = task->period_ns - task->offset_ns;

	set_thread_properties(task);

#if DEBUG > 0
	printf("(%d) Starting Sleep Loop thread\n", task->tid);
#endif

	task->is_running = true;
	Timespec::now(&task->cycle.timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::wait(&task->cycle);
		Timespec::now(&task->cycle.timer);

		task->ncycle++;

		if (StatisticsStatic::push(samples, task->cycle.cycle_time, &index, sample_size)) {
			if (task->tolerance < 0) {
				continue;
			}

			diff = StatisticsStatic::average(samples, sample_size) - task->period_ns;
			if (fabs(diff / task->period_ns) > task->tolerance) {
				task->cycle.period_cmp = task->period_ns - diff;
			}
		}
	}
}

void routine_busy(task_config_t* task, float* samples, const uint32_t& sample_size)
{
	task->tid = gettid();
	uint32_t index = 0;
	float diff;
	task->cycle.period_cmp = task->period_ns - task->offset_ns;

	set_thread_properties(task);

#if DEBUG > 0
	printf("(%d) Starting Busy Loop thread\n", task->tid);
#endif

	// disable lazy_sleep if the value is greater than the period
	if (task->cycle.lazy_sleep >= task->cycle.period_cmp) {
		task->cycle.lazy_sleep = -1;
	}

	task->is_running = true;
	Timespec::now(&task->cycle.timer);

	while (task->is_running) {
		(*task->fptr)();

		Sleep::busy_wait(&task->cycle);
		Timespec::now(&task->cycle.timer);

		task->ncycle++;

		if (StatisticsStatic::push(samples, task->cycle.cycle_time, &index, sample_size)) {
			if (task->tolerance < 0) {
				continue;
			}

			diff = StatisticsStatic::average(samples, sample_size) - task->period_ns;
			if (fabs(diff / task->period_ns) > task->tolerance) {
				task->cycle.period_cmp = task->period_ns - diff;
			}
		}
	}
}

void routine_deadline(task_config_t* task, float* samples, const uint32_t& sample_size)
{
	task->tid = gettid();
	uint32_t index = 0;

	struct sched_attr attr;
    attr.size = sizeof(attr);
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_nice = task->nice_value;
    attr.sched_runtime = task->cycle.exec_time;
    attr.sched_period = task->period_ns - task->offset_ns;
    attr.sched_deadline = task->deadline_time > -1
		? task->deadline_time
		: attr.sched_period >> 1;

    if(syscall(__NR_sched_setattr, task->tid, &attr, 0) == -1){
		fprintf(stderr, "(%d) Failed to set sched_attr: %s\n", task->tid, strerror(errno));
		return;
	}

	if (setpriority(PRIO_PROCESS, task->tid, task->nice_value) == -1) {
		fprintf(stderr, "(%d) setpriority: %s\n", task->tid, strerror(errno));
	}

#if DEBUG > 0
	printf("(%d) Starting Sched Deadline thread\n", task->tid);
#endif

	task->is_running = true;

	while(task->is_running){
		Timespec::now(&task->cycle.timer);
		(*task->fptr)();
		Timespec::diff(task->cycle.timer, task->cycle.deadline, &task->cycle.cycle_time);
		StatisticsStatic::push(samples, task->cycle.cycle_time, &index, sample_size);
		Timespec::copy(&task->cycle.deadline, task->cycle.timer, 0);

		task->ncycle++;

		sched_yield();
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
