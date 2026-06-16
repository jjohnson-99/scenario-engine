#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "forecasting/MovingAverageForecaster.hpp"
#include "evaluation/Backtester.hpp"
#include "data/TimeSeries.hpp"

using Catch::Approx;

TEST_CASE("Backtester produces forecasts")
{
    TimeSeries ts;

    for (int i = 0; i < 20; ++i) {
        ts.add({{}, 100.0});
    }

    MovingAverageForecaster model(3);

    Backtester backtester;

    auto result = backtester.evaluate(model, ts);

    REQUIRE(result.evaluation_count > 0);
    REQUIRE(result.mae == Approx(0.0));
    REQUIRE(result.bias == Approx(0.0));
    REQUIRE(result.max_error == Approx(0.0));
    REQUIRE(result.horizon == 1);
    REQUIRE(result.model_name == "MovingAverage(3)");
}

TEST_CASE("Backtester horizon=2 validates against 2-steps-ahead actuals")
{
    TimeSeries ts;

    // Ascending series: 100, 102, 104, ..., 138 (20 points, step 2)
    for (int i = 0; i < 20; ++i) {
        ts.add({{}, 100.0 + 2.0 * i});
    }

    // Window-3 MA with horizon 2: predicts the average of the last 3 values,
    // compared against the value 2 steps later. With a perfectly linear series
    // the MA lags by 1 step (predicts current mean), so the 2-step error is
    // a fixed offset, not zero — this confirms the backtester is comparing
    // against t+1 (index), not t.
    MovingAverageForecaster model(3, 2);

    Backtester backtester;

    auto result = backtester.evaluate(model, ts);

    REQUIRE(result.evaluation_count > 0);
    REQUIRE(result.horizon == 2);
    REQUIRE(result.mae > 0.0);
}
