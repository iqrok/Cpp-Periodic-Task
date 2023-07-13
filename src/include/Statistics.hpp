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

namespace Statistics {

template <typename T>
T average(const std::vector<T>& distributionVector)
{
	if (distributionVector.size() == 0) {
		throw std::invalid_argument("Statistics::average - The distribution provided is empty");
	}
	return std::accumulate(distributionVector.begin(), distributionVector.end(), T())
		/ (distributionVector.size());
}

template <typename T>
T variance(const std::vector<T>& distributionVector)
{
	if (distributionVector.size() == 0) {
		throw std::invalid_argument("Statistics::expectation - The distribution provided is empty");
	}
	T meanOfSquare = average(distributionVector);
	return (std::accumulate(distributionVector.begin(), distributionVector.end(), T(), [=](T a, T b) { return a + (b - meanOfSquare) * (b - meanOfSquare); }) / distributionVector.size());
}

template <typename T>
T standardDeviation(const std::vector<T>& distributionVector)
{
	return std::sqrt(variance(distributionVector));
}

template <typename T>
T mode(const std::vector<T>& distributionVector)
{
	if (distributionVector.size() == 0) {
		throw std::invalid_argument("Statistics::mode - The distribution provided is empty.");
	}
	std::unordered_map<T, int> frequencyMap;
	std::for_each(distributionVector.begin(), distributionVector.end(), [&](T a) { frequencyMap[a]++; });
	return std::max_element(frequencyMap.begin(), frequencyMap.end(), [](auto a, auto b) { return (a.second < b.second); })->first;
}

template <typename T>
void minmax(const std::vector<T>& distributionVector, T* _min, T* _max)
{
	if (distributionVector.size() == 0) {
		throw std::invalid_argument("Statistics::minmax - The distribution provided is empty.");
	}

	auto result = minmax_element(
		distributionVector.begin(),
		distributionVector.end());

	*_min = *result.first;
	*_max = *result.second;
}

template <typename T, typename U>
void calculate(const std::vector<T>& distributionVector, U* _average, U* _deviation)
{
	uint32_t length = distributionVector.size();

	if (length == 0) {
		throw std::invalid_argument("Statistics::calculate - The distribution provided is empty.");
	}

	*_average = average(distributionVector);

	// variance
	T _accumulation = std::accumulate(
		distributionVector.begin(),
		distributionVector.end(),
		T(),
		[=](T a, T b) {
			return a + (b - (T)*_average) * (b - (T)*_average);
		});

	T _variance = _accumulation / length;

	*_deviation = std::sqrt(_variance);
}

template <typename T, typename U>
bool push(std::vector<T>* sample, const U& value, uint32_t* index, const uint32_t& max_size)
{
	uint32_t length = (*sample).size();

	if (length < max_size) {
		(*sample).push_back((T)value);
		*index = length + 1;
		return *index == (length - 1);
	}

	if (*index < max_size)
		*index += 1;
	else
		*index = 0;

	(*sample)[*index] = (T)value;

	return *index == (length - 1);
}

template <typename T>
void print_stats(std::vector<T>& sample, std::string row_name, bool header)
{
	uint32_t _length = sample.size();
	T _average, _deviation, _min, _max;

	calculate(sample, &_average, &_deviation);
	minmax(sample, &_min, &_max);

	const char sep = ' ';
	const int width = 20;

	if (header) {
		std::cerr
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
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _length
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _average
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _deviation
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _min
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << _max
		<< std::right << std::setw(width) << std::setfill(sep) << std::fixed << (_max - _min)
		<< "\n";
}

}
