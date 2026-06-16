#include "forecasting/ExponentialSmoothingForecaster.hpp"

#include <format>
#include <stdexcept>

ExponentialSmoothingForecaster::ExponentialSmoothingForecaster(double alpha, std::size_t horizon)
    : alpha_(alpha), horizon_(horizon)
{
    if (alpha <= 0.0 || alpha > 1.0) {
        throw std::invalid_argument("alpha must be in (0, 1]");
    }

    if (horizon_ == 0) {
        throw std::invalid_argument("Forecast horizon must be positive");
    }
}

ForecastResult ExponentialSmoothingForecaster::forecast(const TimeSeriesView& series) const
{
    if (series.size() == 0) {
        throw std::runtime_error("empty series");
    }

    double smoothed = series.value_at(0);

    for (std::size_t i = 1; i < series.size(); ++i) {
        smoothed = alpha_ * series.value_at(i) + (1.0 - alpha_) * smoothed;
    }

    return {.mean = smoothed, .variance = 0.0, .horizon = horizon_};
}

std::string_view ExponentialSmoothingForecaster::name() const
{
    return "ExponentialSmoothing";
}

std::string ExponentialSmoothingForecaster::label() const
{
    return std::format("ExponentialSmoothing({:.2f})", alpha_);
}

std::size_t ExponentialSmoothingForecaster::minimum_observations() const
{
    return 1;
}

std::size_t ExponentialSmoothingForecaster::horizon() const
{
    return horizon_;
}
