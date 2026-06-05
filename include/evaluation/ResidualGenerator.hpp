#pragma once

#include "evaluation/ResidualSeries.hpp"
#include "forecasting/Forecaster.hpp"
#include "data/TimeSeries.hpp"

class ResidualGenerator
{
public:
    ResidualSeries generate(
        const Forecaster& model,
        const TimeSeries& series
    ) const;

    ResidualSeries generate(
        const Forecaster& model,
        const TimeSeries& series,
        std::size_t horizon
    ) const;
};
