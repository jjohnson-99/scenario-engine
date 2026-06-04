#pragma once

#include "forecasting/Forecaster.hpp"

class ExponentialSmoothingForecaster
    : public Forecaster
{
public:
    explicit ExponentialSmoothingForecaster(
        double alpha,
        std::size_t horizon = 1);

    std::string_view name() const override;

    std::string label() const override;

    ForecastResult forecast(const TimeSeries& series) const override;

    std::size_t minimum_observations() const override;

    std::size_t horizon() const override;

private:
    double alpha_;
    std::size_t horizon_;
};
