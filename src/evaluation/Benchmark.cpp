#include "evaluation/Benchmark.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>

#include "evaluation/Backtester.hpp"

std::vector<ModelEvaluationResult> Benchmark::evaluate(
    std::span<const Forecaster*> models,
    const TimeSeries& series) const
{
    if (models.empty())
    {
        throw std::invalid_argument("Benchmark requires at least one model");
    }

    Backtester backtester;

    std::vector<ModelEvaluationResult> results;
    results.reserve(models.size());

    for (const Forecaster* model : models)
    {
        results.push_back(backtester.evaluate(*model, series));
    }

    std::ranges::sort(results, {}, &ModelEvaluationResult::rmse);

    return results;
}

std::vector<MultiHorizonResult> Benchmark::run(
    std::span<const Forecaster*>  models,
    const TimeSeries&             series,
    std::span<const std::size_t>  horizons) const
{
    if (models.empty())
    {
        throw std::invalid_argument("Benchmark requires at least one model");
    }

    if (horizons.empty())
    {
        throw std::invalid_argument("Benchmark requires at least one horizon");
    }

    Backtester backtester;

    std::vector<MultiHorizonResult> table;
    table.reserve(models.size());

    for (const Forecaster* model : models)
    {
        MultiHorizonResult row;
        row.model_label = model->label();
        row.by_horizon.reserve(horizons.size());

        for (const std::size_t h : horizons)
        {
            row.by_horizon.push_back(
                backtester.evaluate(*model, series, h));
        }

        std::ranges::sort(row.by_horizon, {}, &ModelEvaluationResult::horizon);

        table.push_back(std::move(row));
    }

    return table;
}
