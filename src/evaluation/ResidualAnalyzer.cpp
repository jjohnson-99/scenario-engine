#include "evaluation/ResidualAnalyzer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <span>
#include <stdexcept>

ResidualStatistics ResidualAnalyzer::analyze(const ResidualSeries& series) const
{
    if (series.size() == 0) {
        throw std::runtime_error("Cannot analyze an empty ResidualSeries");
    }

    double sum = 0.0;
    double sum2 = 0.0;
    double sum3 = 0.0;
    double sum4 = 0.0;

    for (const auto& r : series.residuals()) {
        sum += r.error;
        sum2 += r.error * r.error;
        sum3 += r.error * r.error * r.error;
        sum4 += r.error * r.error * r.error * r.error;
    }

    const double n = static_cast<double>(series.size());
    const double mean = sum / n;
    const double variance = sum2 / n - mean * mean;
    const double std_dev = std::sqrt(variance);

    double skewness = 0.0;
    double excess_kurtosis = 0.0;

    if (std_dev > 0.0) {
        // Central moments computed from raw moments via binomial expansion
        const double m2 = variance;
        const double m3 = sum3 / n - 3.0 * mean * m2 - mean * mean * mean;
        const double m4 = sum4 / n - 4.0 * mean * (sum3 / n) + 6.0 * mean * mean * m2
                          + 3.0 * mean * mean * mean * mean;

        skewness = m3 / (std_dev * std_dev * std_dev);
        excess_kurtosis = m4 / (m2 * m2) - 3.0;
    }

    return {
        .mean_error = mean,
        .variance = variance,
        .standard_deviation = std_dev,
        .skewness = skewness,
        .excess_kurtosis = excess_kurtosis,
    };
}

AutocorrelationResult ResidualAnalyzer::acf(const ResidualSeries& series, std::size_t n_lags) const
{
    if (series.size() == 0) {
        throw std::runtime_error("Cannot compute ACF of an empty ResidualSeries");
    }

    const auto& residuals = series.residuals();
    const std::size_t n = residuals.size();

    const double mean =
        std::accumulate(residuals.begin(), residuals.end(), 0.0,
            [](double acc, const Residual& r) { return acc + r.error; })
        / static_cast<double>(n);

    // Project each residual onto its centred deviation from the mean
    std::vector<double> centred(n);
    std::ranges::transform(residuals, centred.begin(),
        [mean](const Residual& r) { return r.error - mean; });

    // Total variance — shared denominator for every lag
    const std::span<const double> c{centred};
    const double denom = std::inner_product(c.begin(), c.end(), c.begin(), 0.0);

    const std::size_t lags = std::min(n_lags, n - 1);
    std::vector<double> result(lags);

    for (std::size_t k = 1; k <= lags; ++k) {
        // Lag-k numerator: inner product of c[k:] and c[0:n-k]
        const auto lagged = c.subspan(k);
        const double num = std::inner_product(lagged.begin(), lagged.end(), c.begin(), 0.0);
        result[k - 1] = (denom > 0.0) ? num / denom : 0.0;
    }

    return {.acf = std::move(result), .n_lags = lags};
}

LjungBoxResult ResidualAnalyzer::ljung_box(const AutocorrelationResult& acf_result, std::size_t n) const
{
    const double nd = static_cast<double>(n);
    double q = 0.0;

    for (std::size_t k = 1; k <= acf_result.n_lags; ++k) {
        const double rk = acf_result.acf[k - 1];
        q += (rk * rk) / (nd - static_cast<double>(k));
    }

    q *= nd * (nd + 2.0);

    return {.q_statistic = q, .degrees_of_freedom = acf_result.n_lags};
}
