#ifndef _TIMESPEC_HELPER_HPP_
#define _TIMESPEC_HELPER_HPP_

#include <cstdint>
#include <time.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

namespace Timespec {

void diff(const struct timespec& a, const struct timespec& b, int64_t* ns)
{
	*ns = ((a.tv_sec - b.tv_sec) * NSEC_PER_SEC) + (a.tv_nsec - b.tv_nsec);
}

void normalize_upper(struct timespec* a)
{
	while (a->tv_nsec >= NSEC_PER_SEC) {
		a->tv_nsec -= NSEC_PER_SEC;
		a->tv_sec++;
	}
}

void normalize_lower(struct timespec* a)
{
	while (a->tv_nsec < 0) {
		a->tv_nsec += NSEC_PER_SEC;
		a->tv_sec--;
	}
}

void copy(
	struct timespec* dst, const struct timespec& src, const int64_t& offset_ns)
{
	dst->tv_sec = src.tv_sec;
	dst->tv_nsec = src.tv_nsec + offset_ns;

	if (offset_ns >= 0) {
		normalize_upper(dst);
	} else {
		normalize_lower(dst);
	}
}

bool compare(const struct timespec& left, const struct timespec& right)
{
	return (left.tv_sec == right.tv_sec) ? left.tv_nsec < right.tv_nsec
										 : left.tv_sec < right.tv_sec;
}

void now(struct timespec* start)
{
	clock_gettime(CLOCK_MONOTONIC, start);
}

uint64_t to_ns(const struct timespec& a)
{
	return (a.tv_sec * NSEC_PER_SEC) + a.tv_nsec;
}

}

#endif
