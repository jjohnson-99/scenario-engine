#pragma once

#include "evaluation/ModelEvaluationResult.hpp"
#include "forecasting/Forecaster.hpp"
#include "data/TimeSeries.hpp"

class Backtester
{
public:
    ModelEvaluationResult evaluate(const Forecaster& model, const TimeSeries& series) const;

    ModelEvaluationResult
    evaluate(const Forecaster& model, const TimeSeries& series, std::size_t horizon) const;
};
