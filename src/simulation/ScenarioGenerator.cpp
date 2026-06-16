#include "simulation/ScenarioGenerator.hpp"

#include <cmath>
#include <stdexcept>

ScenarioGenerator::ScenarioGenerator(unsigned seed) : rng_(seed) {}

std::vector<TimeSeries> ScenarioGenerator::generate(
    const Forecaster& model, const TimeSeries& history, std::size_t n_scenarios,
    std::size_t horizon_steps
) const
{
    if (history.size() < model.minimum_observations()) {
        throw std::runtime_error("Insufficient history for scenario generation");
    }

    std::vector<TimeSeries> scenarios;
    scenarios.reserve(n_scenarios);

    const auto last_timestamp = history.observations().back().timestamp;

    for (std::size_t s = 0; s < n_scenarios; ++s) {
        TimeSeries path = history;

        for (std::size_t step = 1; step <= horizon_steps; ++step) {
            ForecastResult result = model.forecast(path.view());

            double next_value = result.mean;

            if (result.variance > 0.0) {
                std::normal_distribution<double> noise{0.0, std::sqrt(result.variance)};
                next_value += noise(rng_);
            }

            const auto timestamp = last_timestamp + std::chrono::days{static_cast<int>(step)};

            path.add({timestamp, next_value});
        }

        scenarios.push_back(std::move(path));
    }

    return scenarios;
}
