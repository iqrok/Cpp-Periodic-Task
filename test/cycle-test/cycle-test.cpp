#include "cycle-test.hpp"

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

int main(int argc, char* argv[])
{
	MainRoutine::start();

	struct sigaction signalHandler;

	signalHandler.sa_handler = handler;
	sigemptyset(&signalHandler.sa_mask);
	signalHandler.sa_flags = 0;

	sigaction(SIGINT, &signalHandler, NULL);
	sigaction(SIGTERM, &signalHandler, NULL);
	sigaction(SIGKILL, &signalHandler, NULL);
	sigaction(SIGABRT, &signalHandler, NULL);

	if (argc <= 1) {
		printf("Press Ctrl+C to exit...\n");
		pause();
	} else {
		uint64_t us = atol(argv[1]) * 1000;
		printf("Interrupt in %s ms...\n", argv[1]);
		usleep(us);
		raise(SIGINT);
	}

	return 0;
}
