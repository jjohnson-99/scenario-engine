#pragma once

#include "data/TimeSeries.hpp"
#include "forecasting/ForecastResult.hpp"

class Forecaster
{
public:
    virtual ~Forecaster() = default;

    virtual ForecastResult forecast(const TimeSeries& series) const = 0;

    virtual std::size_t minimum_observations() const = 0;

    virtual std::size_t horizon() const = 0;
};
