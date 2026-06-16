#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "evaluation/ResidualGenerator.hpp"
#include "evaluation/Backtester.hpp"
#include "forecasting/MovingAverageForecaster.hpp"
#include "data/TimeSeries.hpp"

using Catch::Approx;

TEST_CASE("ResidualGenerator::generate error equals actual minus predicted")
{
    TimeSeries ts;

    for (int i = 0; i < 10; ++i) {
        ts.add({{}, 100.0});
    }

    MovingAverageForecaster model(3);
    ResidualGenerator generator;

    auto rs = generator.generate(model, ts);

    for (const auto& r : rs.residuals()) {
        REQUIRE(r.error == Approx(r.actual - r.predicted));
    }
}

TEST_CASE("ResidualGenerator::generate count matches evaluate evaluation_count")
{
    TimeSeries ts;

    for (int i = 0; i < 15; ++i) {
        ts.add({{}, 100.0 + static_cast<double>(i)});
    }

    MovingAverageForecaster model(3);
    ResidualGenerator generator;
    Backtester backtester;

    auto result = backtester.evaluate(model, ts);
    auto rs = generator.generate(model, ts);

    REQUIRE(rs.size() == result.evaluation_count);
}

TEST_CASE("ResidualGenerator::generate explicit horizon overload")
{
    TimeSeries ts;

    for (int i = 0; i < 20; ++i) {
        ts.add({{}, 100.0});
    }

    MovingAverageForecaster model(3);
    ResidualGenerator generator;

    auto rs = generator.generate(model, ts, 2);

    REQUIRE(rs.size() > 0);

    for (const auto& r : rs.residuals()) {
        REQUIRE(r.error == Approx(r.actual - r.predicted));
    }
}
