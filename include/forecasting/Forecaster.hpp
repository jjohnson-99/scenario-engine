#pragma once

#include <string>
#include <string_view>

#include "data/TimeSeries.hpp"
#include "forecasting/ForecastResult.hpp"

class Forecaster
{
public:
    virtual ~Forecaster() = default;

    virtual std::string_view name() const = 0;

    virtual std::string label() const = 0;

    virtual ForecastResult forecast(const TimeSeries& series) const = 0;

    virtual std::size_t minimum_observations() const = 0;

    virtual std::size_t horizon() const = 0;
};
