#include "evaluation/ResidualAnalyzer.hpp"

#include <cmath>
#include <stdexcept>

ResidualStatistics ResidualAnalyzer::analyze(const ResidualSeries& series) const
{
    if (series.size() == 0) {
        throw std::runtime_error("Cannot analyze an empty ResidualSeries");
    }

    double error_sum = 0.0;
    double squared_error_sum = 0.0;

    for (const auto& r : series.residuals()) {
        error_sum += r.error;
        squared_error_sum += r.error * r.error;
    }

    const double n = static_cast<double>(series.size());
    const double mean_error = error_sum / n;
    const double variance = squared_error_sum / n - mean_error * mean_error;

    return {.mean_error = mean_error, .variance = variance, .standard_deviation = std::sqrt(variance)};
}
