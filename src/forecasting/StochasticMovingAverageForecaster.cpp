#include "forecasting/StochasticMovingAverageForecaster.hpp"
#include "evaluation/ResidualAnalyzer.hpp"
#include "evaluation/ResidualGenerator.hpp"

#include <format>
#include <stdexcept>

StochasticMovingAverageForecaster::StochasticMovingAverageForecaster(
    std::size_t window,
    std::size_t horizon)
    : window_(window)
    , horizon_(horizon)
{
    if (window_ == 0)
    {
        throw std::invalid_argument(
            "Moving average window must be positive");
    }

    if (horizon_ == 0)
    {
        throw std::invalid_argument(
            "Forecast horizon must be positive");
    }
}

void StochasticMovingAverageForecaster::calibrate(const TimeSeries& series)
{
    ResidualGenerator generator;
    ResidualSeries    rs = generator.generate(*this, series);

    ResidualAnalyzer analyzer;
    variance_ = analyzer.analyze(rs).variance;
}

ForecastResult StochasticMovingAverageForecaster::forecast(
    const TimeSeriesView& view) const
{
    if (view.size() < window_)
    {
        throw std::runtime_error(
            "Insufficient observations for forecast");
    }

    double sum = 0.0;

    for (const auto& obs : view.observations().last(window_))
    {
        sum += obs.value;
    }

    return {
        .mean     = sum / static_cast<double>(window_),
        .variance = variance_,
        .horizon  = horizon_
    };
}

std::string_view StochasticMovingAverageForecaster::name() const
{
    return "StochasticMovingAverage";
}

std::string StochasticMovingAverageForecaster::label() const
{
    return std::format("StochasticMovingAverage({})", window_);
}

std::size_t StochasticMovingAverageForecaster::minimum_observations() const
{
    return window_;
}

std::size_t StochasticMovingAverageForecaster::horizon() const
{
    return horizon_;
}
