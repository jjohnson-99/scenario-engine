#pragma once

#include "data/TimeSeries.hpp"
#include "forecasting/Forecaster.hpp"

class StochasticMovingAverageForecaster : public Forecaster
{
public:
    explicit StochasticMovingAverageForecaster(std::size_t window, std::size_t horizon = 1);

    void calibrate(const TimeSeries& series);

    ForecastResult forecast(const TimeSeriesView& series) const override;

    std::string_view name() const override;
    std::string label() const override;
    std::size_t minimum_observations() const override;
    std::size_t horizon() const override;

private:
    std::size_t window_;
    std::size_t horizon_;
    double variance_{0.0};
};
