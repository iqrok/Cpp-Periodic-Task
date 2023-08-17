#ifndef _CYCLE_TEST_HPP_
#define _CYCLE_TEST_HPP_

#include <TaskCycle.hpp>

#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace MainRoutine {
constexpr uint32_t sample_size = 500;
constexpr uint32_t test_size = 4;

struct TaskConfigurations {
	struct TaskCycle::distribution_summary_s summary;
	float samples[sample_size];
	TaskCycle::task_config_t task;
	std::thread thread;
	std::string name;
	void (*routine)(TaskCycle::task_config_t*, float*, const uint32_t&);
	double result;
};

void workload_sin(const std::vector<double>& data, double& result);
void workload_busy(void);
void workload_busy2(void);
void workload_deadline(void);

std::vector<double> data_workload;
double result_control;
struct TaskConfigurations tTests[test_size];

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
	workload_sin(data_workload, tTests[0].result);
}

void workload_busy2(void)
{
	workload_sin(data_workload, tTests[1].result);
}

void workload_deadline(void)
{
	workload_sin(data_workload, tTests[2].result);
}

void workload_deadline2(void)
{
	workload_sin(data_workload, tTests[3].result);
}

void workload_control(void)
{
	workload_sin(data_workload, result_control);
	printf("Result = %lf\n", result_control);
}

void start()
{
	for(uint16_t i = 0; i < 0xaff; i++){
		time_t t;
		srand((unsigned) time(&t));
		data_workload.push_back((double) rand() / (double) rand());
	}

	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		fprintf(stderr, "Failed to lock memory: %s\n", strerror(errno));
	}

	tTests[0].name = "BUSY 1000 us";
	tTests[0].routine = TaskCycle::routine_busy;
	tTests[0].task.affinity = 0;
	tTests[0].task.schedule_priority = SCHED_FIFO;
	tTests[0].task.nice_value = -20;
	tTests[0].task.tolerance = -1;
	tTests[0].task.period_ns = 1'000'000;
	tTests[0].task.cycle.lazy_sleep = tTests[0].task.period_ns - 350'000;
	tTests[0].task.cycle.step_sleep = 350;
	tTests[0].task.offset_ns = 750,
	tTests[0].task.fptr = workload_busy;

	tTests[2].name = "BUSY 5000 us";
	tTests[2].routine = TaskCycle::routine_busy;
	tTests[2].task.affinity = 0;
	tTests[2].task.schedule_priority = SCHED_RR;
	tTests[2].task.priority_offset = 1;
	tTests[2].task.nice_value = -20;
	tTests[2].task.tolerance = -1;
	tTests[2].task.period_ns = 5'000'000;
	tTests[2].task.cycle.lazy_sleep = tTests[2].task.period_ns - 1'000'000;
	tTests[2].task.cycle.step_sleep = 50;
	tTests[2].task.offset_ns = 750,
	tTests[2].task.fptr = workload_busy2;

	tTests[1].name = "DEADLINE 1000 us";
	tTests[1].routine = TaskCycle::routine_deadline;
	tTests[1].task.nice_value = -20;
	tTests[1].task.period_ns = 1'000'000;
	tTests[1].task.offset_ns = 0;
	tTests[1].task.cycle.exec_time = 500'000;
	tTests[1].task.deadline_time = tTests[1].task.period_ns - 100'000;
	tTests[1].task.fptr = workload_deadline;

	tTests[3].name = "DEADLINE 5000 us";
	tTests[3].routine = TaskCycle::routine_deadline;
	tTests[3].task.nice_value = -20;
	tTests[3].task.period_ns = 5'000'000;
	tTests[3].task.offset_ns = 0;
	tTests[3].task.cycle.exec_time = 500'000;
	tTests[3].task.deadline_time = tTests[3].task.period_ns - 250'000;
	tTests[3].task.fptr = workload_deadline2;

	for(uint8_t i = 0; i < test_size; i++){
		tTests[i].thread = std::thread(
			tTests[i].routine, &tTests[i].task, tTests[i].samples,
			sample_size);

		pthread_setname_np(
			tTests[i].thread.native_handle(),
			tTests[i].name.c_str());
	}
}

void stop()
{
	for(uint8_t i = 0; i < test_size; i++){
		tTests[i].task.is_running = false;
		tTests[i].thread.join();
	}

	printf("\n");

	workload_control();

	for(uint8_t i = 0; i < test_size; i++){
		TaskCycle::stats_summarize(
			&tTests[i].summary, tTests[i].samples,
			sample_size, tTests[i].task.period_ns);

		TaskCycle::stats_print(tTests[i].name.c_str(),
			tTests[i].summary);

		printf("(%d) %20s %16lld loops in %lld ns => %lf [%d]\n\n",
			tTests[i].task.tid, tTests[i].name.c_str(),
			tTests[i].task.elapsed.ncycle,
			tTests[i].task.elapsed.ns,
			tTests[i].result, tTests[i].result == result_control);
	}
}

}

#endif
