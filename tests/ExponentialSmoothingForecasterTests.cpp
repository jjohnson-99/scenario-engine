#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "forecasting/ExponentialSmoothingForecaster.hpp"

using Catch::Approx;

TEST_CASE("Constant series")
{
    TimeSeries ts;

    ts.add({{}, 100.0});
    ts.add({{}, 100.0});
    ts.add({{}, 100.0});

    ExponentialSmoothingForecaster model(0.5);

    REQUIRE(
        model.forecast(ts).value == Approx(100.0));
}

TEST_CASE("Invalid alpha")
{
    REQUIRE_THROWS(
        ExponentialSmoothingForecaster(0.0));
}

TEST_CASE("ExponentialSmoothingForecaster Empty series")
{
    TimeSeries ts;

    ExponentialSmoothingForecaster model(0.5);

    REQUIRE_THROWS(
        model.forecast(ts));
}
