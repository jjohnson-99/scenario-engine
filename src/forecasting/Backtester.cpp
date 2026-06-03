#include "forecasting/Backtester.hpp"

#include <cmath>

BacktestResult Backtester::run(
    const Forecaster& model,
    const TimeSeries& series) const
{
    const std::size_t h = model.horizon();

    if (series.size() < model.minimum_observations() + h)
    {
        throw std::runtime_error("Insufficient data for backtest");
    }

    double abs_error_sum = 0.0;
    double squared_error_sum = 0.0;

    std::size_t forecasts = 0;

    const auto start = model.minimum_observations();
    for (std::size_t t = start; t + h <= series.size(); ++t)
    {
        TimeSeries history = series.first_n(t);

        double prediction =
            model.forecast(history).value;

        double actual =
            series.value_at(t + h - 1);

        double error =
            actual - prediction;

        abs_error_sum += std::abs(error);
        squared_error_sum += error * error;

        ++forecasts;
    }

    return {
        .mae = abs_error_sum /
               static_cast<double>(forecasts),

        .rmse = std::sqrt(
            squared_error_sum /
            static_cast<double>(forecasts)),

        .forecasts = forecasts,

        .horizon = h
    };
}
