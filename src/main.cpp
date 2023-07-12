#include "main.hpp"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int code)
{
	MainRoutine::stop();
	printf("SIGNAL %d\n", code);
	exit(code);
}

int main()
{
	printf("CLOCKS_PER_SEC %ld\n", CLOCKS_PER_SEC);
	struct sched_param param = {};
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);

	if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
		perror("sched_setscheduler failed");
	}

	MainRoutine::start();

	struct sigaction signalHandler;

	signalHandler.sa_handler = handler;
	sigemptyset(&signalHandler.sa_mask);
	signalHandler.sa_flags = 0;

	sigaction(SIGINT, &signalHandler, NULL);
	sigaction(SIGTERM, &signalHandler, NULL);
	sigaction(SIGKILL, &signalHandler, NULL);
	sigaction(SIGABRT, &signalHandler, NULL);

	pause();

	return 0;
}
