#include "evaluation/Backtester.hpp"
#include "evaluation/ResidualGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

ModelEvaluationResult Backtester::evaluate(
    const Forecaster& model,
    const TimeSeries& series) const
{
    return evaluate(model, series, model.horizon());
}

ModelEvaluationResult Backtester::evaluate(
    const Forecaster& model,
    const TimeSeries& series,
    std::size_t horizon) const
{
    ResidualGenerator generator;
    ResidualSeries rs = generator.generate(model, series, horizon);

    double abs_error_sum     = 0.0;
    double squared_error_sum = 0.0;
    double signed_error_sum  = 0.0;
    double max_abs_error     = 0.0;
    double ape_sum           = 0.0;

    std::size_t evaluation_count = 0;
    std::size_t ape_count        = 0;

    for (const auto& r : rs.residuals())
    {
        const double abs_e = std::abs(r.error);

        abs_error_sum     += abs_e;
        squared_error_sum += r.error * r.error;
        signed_error_sum  += r.error;
        max_abs_error      = std::max(max_abs_error, abs_e);

        if (r.actual != 0.0)
        {
            ape_sum += abs_e / std::abs(r.actual);
            ++ape_count;
        }

        ++evaluation_count;
    }

    const double n      = static_cast<double>(evaluation_count);
    const double mean_e = signed_error_sum / n;

    return {
        .model_name       = model.label(),
        .mae              = abs_error_sum / n,
        .rmse             = std::sqrt(squared_error_sum / n),
        .bias             = mean_e,
        .max_error        = max_abs_error,
        .mape             = ape_count > 0
                                ? (ape_sum / static_cast<double>(ape_count)) * 100.0
                                : std::numeric_limits<double>::quiet_NaN(),
        .error_std        = std::sqrt(squared_error_sum / n - mean_e * mean_e),
        .evaluation_count = evaluation_count,
        .horizon          = horizon
    };
}
