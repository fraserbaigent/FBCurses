#ifndef NAMESPACE_NUMERICAL
#define NAMESPACE_NUMERICAL

#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace Numerical {
    static uint64_t unique_rand_uint () {
	static std::vector<uint64_t> drawn_values;
	while (true) {
	    uint64_t val = 0;
	    if (std::find(drawn_values.cbegin(),
			 drawn_values.cend(),
			 val) == drawn_values.cend())
		drawn_values.push_back(val);
	    return val;
	};
    };

    template <typename T>
    struct Statistics {
	T _min;
	T _max;
	T _mean;
	T _median;
	T _std_dev;
    };

    template <typename T>
    static T
    average_mean
    (std::vector<T> const& values) {
	T total = std::accumulate(values.begin(),
				  values.end(),
				  static_cast<T>(0));
	return total / static_cast<T>(values.size());
    };

    template <typename T>
    static T
    average_median
    (std::vector<T> const& values, Statistics<T>* stats = nullptr) {
	std::vector<T> copy = values;
	std::sort(copy.begin(), copy.end());

	if (stats) {
	    stats->_min = values.front();
	    stats->_max = values.back();
	};
	
	if (copy.size() % 2 == 0) {
	    const size_t i = copy.size() / 2;
	    const size_t j = i - 1;
	    return (copy[i] + copy[j]) / static_cast<T>(2);
	} else {
	    return copy[static_cast<size_t>(std::floor(copy.size() / 2.0))];
	};
    };

    template <typename T>
    static T
    average_std_dev
    (std::vector<T> const& values, T const mean) {
	T var = 0;
	for (const auto& val : values) {
	    var += pow((val - mean), 2);
	};
	var *= (1.0 / (values.size() - 1) );
	return static_cast<T>(pow(var, 0.5));
    };
    
    template <typename T>
    static Statistics<T>
    get_stats
    (std::vector<T> const& values) {
	Statistics<T> stats;
	stats._mean = average_mean(values);
	stats._median = average_median(values, &stats);
	stats._std_dev = average_std_dev(values, stats._mean);
	return stats;
    };
};

#endif
