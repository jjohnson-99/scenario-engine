#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "forecasting/MovingAverageForecaster.hpp"

using Catch::Approx;

TEST_CASE("Moving average forecast")
{
    TimeSeries ts;

    ts.add({{}, 100});
    ts.add({{}, 110});
    ts.add({{}, 120});

    MovingAverageForecaster model(3);

    REQUIRE(
        model.forecast(ts).value == Approx(110.0)
    );
}

TEST_CASE("Window of one")
{
    TimeSeries ts;

    ts.add({{}, 100.0});
    ts.add({{}, 110.0});
    ts.add({{}, 120.0});

    MovingAverageForecaster model(1);

    REQUIRE(
        model.forecast(ts).value == Approx(120.0)
    );
}

TEST_CASE("Insufficient observations")
{
    TimeSeries ts;

    ts.add({{}, 100.0});
    ts.add({{}, 110.0});

    MovingAverageForecaster model(3);

    REQUIRE_THROWS(
        model.forecast(ts)
    );
}

TEST_CASE("ForecastResult carries horizon")
{
    TimeSeries ts;

    ts.add({{}, 100.0});
    ts.add({{}, 110.0});
    ts.add({{}, 120.0});

    MovingAverageForecaster model(3, 5);

    REQUIRE(model.forecast(ts).horizon == 5);
}
