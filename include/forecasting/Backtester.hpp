#pragma once

#include "forecasting/BacktestResult.hpp"
#include "forecasting/Forecaster.hpp"
#include "data/TimeSeries.hpp"

class Backtester
{
public:
    BacktestResult run(
        const Forecaster& model,
        const TimeSeries& series
    ) const;
};
