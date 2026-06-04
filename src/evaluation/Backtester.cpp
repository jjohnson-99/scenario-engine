#include "evaluation/Backtester.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

ModelEvaluationResult Backtester::evaluate(
    const Forecaster& model,
    const TimeSeries& series) const
{
    return evaluate(model, series, model.horizon());
}

ModelEvaluationResult Backtester::evaluate(
    const Forecaster& model,
    const TimeSeries& series,
    std::size_t horizon) const
{
    if (series.size() < model.minimum_observations() + horizon)
    {
        throw std::runtime_error("Insufficient data for backtest");
    }

    double abs_error_sum     = 0.0;
    double squared_error_sum = 0.0;
    double signed_error_sum  = 0.0;
    double max_abs_error     = 0.0;
    double ape_sum           = 0.0;

    std::size_t evaluation_count = 0;
    std::size_t ape_count        = 0;

    const auto start = model.minimum_observations();
    for (std::size_t t = start; t + horizon <= series.size(); ++t)
    {
        TimeSeries history = series.first_n(t);

        double prediction = model.forecast(history).value;
        double actual     = series.value_at(t + horizon - 1);
        double error      = actual - prediction;

        abs_error_sum     += std::abs(error);
        squared_error_sum += error * error;
        signed_error_sum  += error;
        max_abs_error      = std::max(max_abs_error, std::abs(error));

        if (actual != 0.0)
        {
            ape_sum += std::abs(error) / std::abs(actual);
            ++ape_count;
        }

        ++evaluation_count;
    }

    const double n     = static_cast<double>(evaluation_count);
    const double mean_e = signed_error_sum / n;

    return {
        .model_name       = model.label(),
        .mae              = abs_error_sum / n,
        .rmse             = std::sqrt(squared_error_sum / n),
        .bias             = mean_e,
        .max_error        = max_abs_error,
        .mape             = ape_count > 0
                                ? (ape_sum / static_cast<double>(ape_count)) * 100.0
                                : std::numeric_limits<double>::quiet_NaN(),
        .error_std        = std::sqrt(squared_error_sum / n - mean_e * mean_e),
        .evaluation_count = evaluation_count,
        .horizon          = horizon
    };
}
