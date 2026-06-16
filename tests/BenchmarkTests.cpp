#include <array>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "evaluation/Benchmark.hpp"
#include "forecasting/MovingAverageForecaster.hpp"
#include "forecasting/ExponentialSmoothingForecaster.hpp"
#include "data/TimeSeries.hpp"

using Catch::Approx;

TEST_CASE("Benchmark evaluates all models")
{
    TimeSeries ts;

    for (int i = 0; i < 20; ++i) {
        ts.add({{}, 100.0});
    }

    MovingAverageForecaster ma(3);
    ExponentialSmoothingForecaster es(0.5);

    std::array<const Forecaster*, 2> models{&ma, &es};

    Benchmark benchmark;

    auto results = benchmark.evaluate(models, ts);

    REQUIRE(results.size() == 2);
}

TEST_CASE("Benchmark results are ranked by RMSE ascending")
{
    TimeSeries ts;

    for (int i = 0; i < 20; ++i) {
        ts.add({{}, 100.0});
    }

    MovingAverageForecaster ma(3);
    ExponentialSmoothingForecaster es(0.5);

    std::array<const Forecaster*, 2> models{&ma, &es};

    Benchmark benchmark;

    auto results = benchmark.evaluate(models, ts);

    for (std::size_t i = 1; i < results.size(); ++i) {
        REQUIRE(results[i - 1].rmse <= results[i].rmse);
    }
}

TEST_CASE("Benchmark result carries model name")
{
    TimeSeries ts;

    for (int i = 0; i < 10; ++i) {
        ts.add({{}, 100.0});
    }

    MovingAverageForecaster ma(3);

    std::array<const Forecaster*, 1> models{&ma};

    Benchmark benchmark;

    auto results = benchmark.evaluate(models, ts);

    REQUIRE(results[0].model_name == "MovingAverage(3)");
}
