#include "evaluation/ResidualGenerator.hpp"

#include <stdexcept>

ResidualSeries ResidualGenerator::generate(
    const Forecaster& model,
    const TimeSeries& series) const
{
    return generate(model, series, model.horizon());
}

ResidualSeries ResidualGenerator::generate(
    const Forecaster& model,
    const TimeSeries& series,
    std::size_t horizon) const
{
    if (series.size() < model.minimum_observations() + horizon)
    {
        throw std::runtime_error("Insufficient data for residual generation");
    }

    ResidualSeries result;

    const auto start = model.minimum_observations();
    for (std::size_t t = start; t + horizon <= series.size(); ++t)
    {
        TimeSeriesView history = series.first_n_view(t);
        double prediction      = model.forecast(history).mean;
        double actual          = series.value_at(t + horizon - 1);

        result.add({.actual = actual, .predicted = prediction, .error = actual - prediction});
    }

    return result;
}
