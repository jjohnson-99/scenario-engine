#pragma once

#include <cstddef>
#include <vector>

struct ResidualStatistics {
    double mean_error{};
    double variance{};
    double standard_deviation{};
    double skewness{};
    double excess_kurtosis{};
};

struct AutocorrelationResult {
    std::vector<double> acf;
    std::size_t n_lags{};
};

struct LjungBoxResult {
    double q_statistic{};
    std::size_t degrees_of_freedom{};
};
