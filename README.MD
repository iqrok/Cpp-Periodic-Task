# Consistent loop timing

There are 2 approaches to achieve consistent timed loop, either use sleep or by busy waiting until the deadline is arrived;

## Sleep Method

This method is straightforward, sleep for given duration then do the next loop.
[clock_nanosleep(2)](https://linux.die.net/man/2/clock_nanosleep) is used to delay the exection in nanoseconds. Use __clock_id__ `CLOCK_MONOTONIC` to avoid time adjustment by OS, and __flag__ `TIMER_ABSTIME` in order to make sure if the deadline is already passed the caller won't be suspended.

The downsides of this method are:

1. Depends heavily on kernel type
	rt or lowlatency kernel will perform better compared to generic kernel. Basically we ask the kernel to do the suspension by calling `clock_nanosleep`, which is an API by the kernel to user space.

2. Different Hardware might give very different result

3. Jitter value is relatively higher (compared to busy wait)
	depending on the kernel and hardware type, the deviation varied between 10us to hundreds of microseconds.

4. Relatively inconsistent between run
	depending on how high the CPU usage on the core which the thread is running

The advantage of using this method is:

1. CPU usage is low
	

## Busy Wait Method

This method uses loop to compare current time with the deadline.

```c++
while (timespec_compare(timer, deadline)) {
	clock_gettime(CLOCK_MONOTONIC, &timer);

	// need to add sleep to avoid throttling being activated by OS
	if(++counter > step_sleep){
		counter = 0;
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timer, NULL);
	}
}
```

But looping without delay might delay the time comparation (loop execution) much later. That's why we put `clock_nanosleep` inside the loop. `step_sleep` is used to only execute `clock_nanosleep` at certain `counter` only.

The bigger `step_sleep` value, the better the accuracy but might trigger throttling if it starves other processes resources.

This method will hog the CPU usage to almost 100%, so it's not possible to run more than 1 thread with this method on the same CPU core, without sacrificing the consistency.

The Advantages/Disadvantages of Busy Wait method are the opposite of the Sleep method's.