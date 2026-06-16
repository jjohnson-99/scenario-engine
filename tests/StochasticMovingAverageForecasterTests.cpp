#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "data/TimeSeries.hpp"
#include "forecasting/StochasticMovingAverageForecaster.hpp"

using Catch::Approx;

static TimeSeries make_constant(int n, double value)
{
    TimeSeries ts;
    for (int i = 0; i < n; ++i)
        ts.add({{}, value});
    return ts;
}

TEST_CASE("StochasticMA uncalibrated variance is zero")
{
    auto ts = make_constant(10, 50.0);

    StochasticMovingAverageForecaster model(3);

    REQUIRE(model.forecast(ts.view()).variance == Approx(0.0));
}

TEST_CASE("StochasticMA constant series has zero variance after calibration")
{
    auto ts = make_constant(10, 50.0);

    StochasticMovingAverageForecaster model(3);
    model.calibrate(ts);

    auto result = model.forecast(ts.view());

    REQUIRE(result.mean == Approx(50.0));
    REQUIRE(result.variance == Approx(0.0));
}

TEST_CASE("StochasticMA variable series has positive variance after calibration")
{
    TimeSeries ts;
    for (int i = 0; i < 10; ++i)
        ts.add({{}, i % 2 == 0 ? 10.0 : 20.0});

    StochasticMovingAverageForecaster model(3);
    model.calibrate(ts);

    REQUIRE(model.forecast(ts.view()).variance > 0.0);
}

TEST_CASE("StochasticMA mean matches plain MA mean after calibration")
{
    TimeSeries ts;
    for (double v : {10.0, 20.0, 30.0, 40.0, 50.0})
        ts.add({{}, v});

    // Window 3: last three values are 30, 40, 50 → mean = 40
    StochasticMovingAverageForecaster model(3);
    model.calibrate(ts);

    REQUIRE(model.forecast(ts.view()).mean == Approx(40.0));
}

TEST_CASE("StochasticMA insufficient observations throws")
{
    TimeSeries ts;
    ts.add({{}, 10.0});
    ts.add({{}, 20.0});

    StochasticMovingAverageForecaster model(3);

    REQUIRE_THROWS(model.forecast(ts.view()));
}

TEST_CASE("StochasticMA horizon is preserved")
{
    auto ts = make_constant(10, 1.0);

    StochasticMovingAverageForecaster model(3, 4);
    model.calibrate(ts);

    REQUIRE(model.forecast(ts.view()).horizon == 4);
    REQUIRE(model.horizon() == 4);
}

TEST_CASE("StochasticMA label and name")
{
    StochasticMovingAverageForecaster model(5);

    REQUIRE(model.label() == "StochasticMovingAverage(5)");
    REQUIRE(model.name() == "StochasticMovingAverage");
}
