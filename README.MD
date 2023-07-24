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

## Test Results

### Test 1

- CPU AMD Ryzen 7 5700U with Radeon Graphics
- kernel 6.2.0-1008-lowlatency
- OS Ubuntu 23.04

```bash
============ BUSY WAIT ============
Sample Size  :             500
Task Period  :     1000.000000 us (1000.000 Hz)
mean         :     1000.085205 us (999.915 Hz)
deviation    :        0.421931 us
min          :      999.366028 us
max          :     1002.229980 us
diff min max :        2.864000 us
% deviation  :        0.042193 %
% minmax     :        0.286400 %
------------------------------
```
```bash
============ SLEEP ============
Sample Size  :             500
Task Period  :     7500.000000 us (133.333 Hz)
mean         :     7519.482910 us (132.988 Hz)
deviation    :        3.744515 us
min          :     7510.787109 us
max          :     7592.222168 us
diff min max :       81.434998 us
% deviation  :        0.049927 %
% minmax     :        1.085800 %
------------------------------
```

### Test 2-1
- CPU Intel(R) Celeron(R) CPU N3350 @ 1.10GHz
- kernel 6.3.5-lowlatency (ubuntu patch)
- OS Debian 12
- with root permission

```bash
============ BUSY WAIT ============
Sample Size  :             500
Task Period  :     1000.000000 us (1000.000 Hz)
mean         :     1001.044189 us (998.957 Hz)
deviation    :        2.246352 us
min          :      999.299988 us
max          :     1020.755005 us
diff min max :       21.455000 us
% deviation  :        0.224635 %
% minmax     :        2.145500 %
------------------------------
```
```bash
============ SLEEP ============
Sample Size  :             500
Task Period  :     7500.000000 us (133.333 Hz)
mean         :     7499.888184 us (133.335 Hz)
deviation    :       19.588785 us
min          :     7426.754883 us
max          :     7675.051758 us
diff min max :      248.296997 us
% deviation  :        0.261184 %
% minmax     :        3.310627 %
------------------------------
```

### Test 2-2
- CPU Intel(R) Celeron(R) CPU N3350 @ 1.10GHz
- kernel 6.1.0-10-rt-amd64
- OS Debian 12
- without root permission (No RT thread)

```bash
============ BUSY WAIT ============
Sample Size  :             500
Task Period  :     1000.000000 us (1000.000 Hz)
mean         :     1000.076050 us (999.924 Hz)
deviation    :       19.110909 us
min          :      994.033997 us
max          :     1316.558960 us
diff min max :      322.524994 us
% deviation  :        1.911091 %
% minmax     :       32.252499 %
------------------------------
```
```bash
============ SLEEP ============
Sample Size  :             500
Task Period  :     7500.000000 us (133.333 Hz)
mean         :     7526.804199 us (132.859 Hz)
deviation    :      504.612976 us
min          :     7381.754883 us
max          :    17631.429688 us
diff min max :    10249.674805 us
% deviation  :        6.728173 %
% minmax     :      136.662323 %
------------------------------
```

### Test 2-3
- CPU Intel(R) Celeron(R) CPU N3350 @ 1.10GHz
- kernel 6.1.0-10-rt-amd64
- OS Debian 12
- with root permission

```bash
============ BUSY WAIT ============
Sample Size  :             500
Task Period  :     1000.000000 us (1000.000 Hz)
mean         :     1745.976074 us (572.746 Hz)
deviation    :     2333.716553 us
min          :        2.047000 us
max          :    51031.531250 us
diff min max :    51029.484375 us
% deviation  :      233.371643 %
% minmax     :     5102.948242 %
------------------------------
```
```bash
============ SLEEP ============
Sample Size  :             500
Task Period  :     7500.000000 us (133.333 Hz)
mean         :     7502.569824 us (133.288 Hz)
deviation    :       29.913811 us
min          :     7403.983887 us
max          :     7639.036133 us
diff min max :      235.052002 us
% deviation  :        0.398851 %
% minmax     :        3.134027 %
------------------------------

```

### Test 3
- CPU Intel(R) Core(TM) i5-8265U CPU @ 1.60GHz
- kernel 5.19.0-1028-lowlatency
- OS Ubuntu 22.04.2 LTS

```bash
============ BUSY WAIT ============
Sample Size  :             500
Task Period  :     1000.000000 us (1000.000 Hz)
mean         :     1000.343262 us (999.657 Hz)
deviation    :        2.094338 us
min          :      999.299988 us
max          :     1029.064941 us
diff min max :       29.764999 us
% deviation  :        0.209434 %
% minmax     :        2.976500 %
------------------------------
```
```bash
============ SLEEP ============
Sample Size  :             500
Task Period  :     7500.000000 us (133.333 Hz)
mean         :     7525.872559 us (132.875 Hz)
deviation    :       16.548178 us
min          :     7509.259766 us
max          :     7594.020020 us
diff min max :       84.760002 us
% deviation  :        0.220642 %
% minmax     :        1.130133 %
------------------------------
```