#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "evaluation/ModelEvaluationResult.hpp"
#include "evaluation/MultiHorizonResult.hpp"
#include "forecasting/Forecaster.hpp"
#include "data/TimeSeries.hpp"

class Benchmark
{
public:
    std::vector<ModelEvaluationResult>
    evaluate(std::span<const Forecaster*> models, const TimeSeries& series) const;

    std::vector<MultiHorizonResult>
    run(std::span<const Forecaster*> models, const TimeSeries& series,
        std::span<const std::size_t> horizons) const;
};
