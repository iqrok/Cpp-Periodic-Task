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
