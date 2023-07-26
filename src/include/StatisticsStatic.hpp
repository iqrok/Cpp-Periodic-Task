/**
 * original author: Suryasis Paul
 * url: https://github.com/suryasis-hub/MathLibrary/blob/main/MathLibrary/Statistics.h
 * **/
#ifndef _STATISTICS_STATIC_HPP_
#define _STATISTICS_STATIC_HPP_

#include <cmath>
#include <stdexcept>

namespace StatisticsStatic {

template <typename T, typename V>
T average(T* distribution, const V& size)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::expectation - The distribution provided is empty");
	}

	T sum = 0;
	for (V idx = 0; idx < size; idx++) {
		sum += distribution[idx];
	}

	return sum / size;
}

template <typename T, typename V>
T variance(T* distribution, const V& size)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::expectation - The distribution provided is empty");
	}

	T meanOfSquare = average(distribution, size);

	T sum = 0;
	for (V idx = 0; idx < size; idx++) {
		sum += ((distribution[idx] - meanOfSquare) * (distribution[idx] - meanOfSquare));
	}

	return sum / size;
}

template <typename T, typename V>
T standardDeviation(T* distribution, const V& size)
{
	return std::sqrt(variance(distribution, size));
}

template <typename T, typename V>
void minmax(T* distribution, const V& size, T* _min, T* _max)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::minmax - The distribution provided is empty.");
	}

	*_min = distribution[0];
	*_max = distribution[0];

	for (V index = 1; index < size; index++) {
		if (*_min > distribution[index]) {
			*_min = distribution[index];
		}

		if (*_max < distribution[index]) {
			*_max = distribution[index];
		}
	}
}

template <typename T, typename U, typename V>
void calculate(T* distribution, const V& size, const U& target, U* _average,
	U* _standard_deviation, U* _periodic_deviation, U* _min, U* _max)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::calculate - The distribution provided is empty.");
	}

	*_average = average(distribution, size);

	*_max = distribution[0];
	*_min = distribution[0];

	// variance
	T _accumulation = 0;
	T _paccumulation = 0;
	for (V index = 0; index < size; index++) {
		_accumulation += ((distribution[index] - *_average) * (distribution[index] - *_average));
		_paccumulation += ((distribution[index] - target) * (distribution[index] - target));

		if (*_min > distribution[index]) {
			*_min = distribution[index];
		}

		if (*_max < distribution[index]) {
			*_max = distribution[index];
		}
	}

	*_standard_deviation = std::sqrt(_accumulation / size);
	*_periodic_deviation = std::sqrt(_paccumulation / size);
}

template <typename T, typename U, typename V>
bool push(T* distribution, const U& value, V* index, const V& size)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::push - The distribution provided is empty.");
	}

	if (*index >= size) {
		*index = 0;
	}

	distribution[(*index)++] = (T)value;

	return *index == size;
}

}

#endif
