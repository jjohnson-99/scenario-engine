#include "forecasting/MovingAverageForecaster.hpp"

#include <format>
#include <stdexcept>

MovingAverageForecaster::MovingAverageForecaster(
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

ForecastResult MovingAverageForecaster::forecast(const TimeSeries& series) const
{
    const auto& observations = series.observations();

    if (observations.size() < window_)
    {
        throw std::runtime_error(
            "Insufficient observations for forecast");
    }

    double sum = 0.0;

    const std::size_t start =
        observations.size() - window_;

    for (std::size_t i = start; i < observations.size(); ++i)
    {
        sum += observations[i].value;
    }

    return {sum / static_cast<double>(window_), horizon_};
}

std::string_view MovingAverageForecaster::name() const
{
    return "MovingAverage";
}

std::string MovingAverageForecaster::label() const
{
    return std::format("MovingAverage({})", window_);
}

std::size_t MovingAverageForecaster::minimum_observations() const
{
    return window_;
}

std::size_t MovingAverageForecaster::horizon() const
{
    return horizon_;
}
