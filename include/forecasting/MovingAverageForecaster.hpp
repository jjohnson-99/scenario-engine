#pragma once

#include "forecasting/Forecaster.hpp"

class MovingAverageForecaster : public Forecaster
{
public:
    explicit MovingAverageForecaster(std::size_t window, std::size_t horizon = 1);

    ForecastResult forecast(const TimeSeriesView& series) const override;

    std::string_view name() const override;
    std::string label() const override;
    std::size_t minimum_observations() const override;
    std::size_t horizon() const override;

private:
    std::size_t window_;
    std::size_t horizon_;
};
