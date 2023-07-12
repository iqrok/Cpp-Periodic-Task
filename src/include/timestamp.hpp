#ifndef _TIMESTAMP_CPP_H_
#define _TIMESTAMP_CPP_H_

#include <chrono>

namespace Timestamp {

inline uint64_t now()
{
	auto now = std::chrono::steady_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto epoch = now_ms.time_since_epoch();
	auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

	return value.count();
}

inline uint64_t now_us()
{
	auto now = std::chrono::steady_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::microseconds>(now);
	auto epoch = now_ms.time_since_epoch();
	auto value = std::chrono::duration_cast<std::chrono::microseconds>(epoch);

	return value.count();
}

inline uint64_t now_ns()
{
	auto now = std::chrono::steady_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
	auto epoch = now_ms.time_since_epoch();
	auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch);

	return value.count();
}

}

#endif
