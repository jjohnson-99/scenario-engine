#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include "data/TimeSeries.hpp"
#include "forecasting/Forecaster.hpp"

class ScenarioGenerator
{
public:
    explicit ScenarioGenerator(unsigned seed = std::random_device{}());

    std::vector<TimeSeries> generate(
        const Forecaster& model, const TimeSeries& history, std::size_t n_scenarios,
        std::size_t horizon_steps
    ) const;

private:
    mutable std::mt19937 rng_;
};
