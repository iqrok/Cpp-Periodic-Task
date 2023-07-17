/**
 * original author: Suryasis Paul
 * url: https://github.com/suryasis-hub/MathLibrary/blob/main/MathLibrary/Statistics.h
 * **/

#include <cmath>
#include <cstdint>
#include <functional>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include <iomanip>
#include <iostream>

namespace StatisticsStatic {

template <typename T>
T average(T* distribution, const uint32_t& size)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::expectation - The distribution provided is empty");
	}

	T sum = 0;
	for(int idx = 0; idx < size; idx++){
		sum += distribution[idx];
	}

	return sum / size;
}

template <typename T>
T variance(T* distribution, const uint32_t& size)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::expectation - The distribution provided is empty");
	}

	T meanOfSquare = average(distribution, size);

	T sum = 0;
	for(int idx = 0; idx < size; idx++){
		sum += ((distribution[idx] - meanOfSquare) * (distribution[idx] - meanOfSquare));
	}

	return sum / size;
}

template <typename T>
T standardDeviation(T* distribution, const uint32_t& size)
{
	return std::sqrt(variance(distribution, size));
}

template <typename T>
void minmax(T* distribution, const uint32_t& size, T* _min, T* _max)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::minmax - The distribution provided is empty.");
	}

	*_min = distribution[0];
	*_max = distribution[0];

	for(int index = 1; index < size; index++){
		if(*_min > distribution[index]) *_min = distribution[index];
		if(*_max < distribution[index]) *_max = distribution[index];
	}
}

template <typename T, typename U>
void calculate(T* distribution, const uint32_t& size, U* _average, U* _deviation)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::calculate - The distribution provided is empty.");
	}

	*_average = average(distribution, size);

	// variance
	T _accumulation = 0;
	for(int idx = 0; idx < size; idx++){
		_accumulation += ((distribution[idx] - *_average) * (distribution[idx] - *_average));
	}

	T _variance = _accumulation / size;

	*_deviation = std::sqrt(_variance);
}

template <typename T, typename U>
bool push(T* distribution, const U& value, uint32_t* index, const uint32_t& size)
{
	if (size == 0) {
		throw std::invalid_argument("StatisticsStatic::push - The distribution provided is empty.");
	}

	if (*index >= size){
		*index = 0;
	}

	distribution[(*index)++] = (T)value;

	return *index == size;
}

template <typename T>
void print_stats(T* distribution, const uint32_t& size, std::string row_name, bool header)
{
	T _average, _deviation, _min, _max;

	calculate(distribution, size, &_average, &_deviation);
	minmax(distribution, size, &_min, &_max);

	const char sep = ' ';
	const int width = 20;

	if (header) {
		std::cerr << "\n"
			<< std::right << std::setw(35) << std::setfill(sep) << "Name"
			<< std::right << std::setw(width) << std::setfill(sep) << "Size"
			<< std::right << std::setw(width) << std::setfill(sep) << "Mean"
			<< std::right << std::setw(width) << std::setfill(sep) << "StdDev"
			<< std::right << std::setw(width) << std::setfill(sep) << "Min"
			<< std::right << std::setw(width) << std::setfill(sep) << "Max"
			<< std::right << std::setw(width) << std::setfill(sep) << "Diff Min-Max"
			<< "\n";
	}

	std::cerr
		<< std::right << std::setw(35) << std::setfill(sep) << row_name
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << size
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _average
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _deviation
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _min
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _max
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << (_max - _min)
		<< "\n";
}

}
