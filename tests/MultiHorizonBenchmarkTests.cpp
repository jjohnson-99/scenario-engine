#include <array>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "evaluation/Benchmark.hpp"
#include "forecasting/MovingAverageForecaster.hpp"
#include "forecasting/ExponentialSmoothingForecaster.hpp"
#include "data/TimeSeries.hpp"

using Catch::Approx;

static TimeSeries make_constant_series(int n, double value = 100.0)
{
    TimeSeries ts;
    for (int i = 0; i < n; ++i)
        ts.add({{}, value});
    return ts;
}

TEST_CASE("Benchmark::run produces one row per model")
{
    TimeSeries ts = make_constant_series(30);

    MovingAverageForecaster ma(3);
    ExponentialSmoothingForecaster es(0.5);

    std::array<const Forecaster*, 2> models{&ma, &es};
    std::array<std::size_t, 3>       horizons{1, 2, 3};

    Benchmark benchmark;

    auto table = benchmark.run(models, ts, horizons);

    REQUIRE(table.size() == 2);
}

TEST_CASE("Benchmark::run produces one column per horizon")
{
    TimeSeries ts = make_constant_series(30);

    MovingAverageForecaster ma(3);

    std::array<const Forecaster*, 1> models{&ma};
    std::array<std::size_t, 4>       horizons{1, 2, 5, 10};

    Benchmark benchmark;

    auto table = benchmark.run(models, ts, horizons);

    REQUIRE(table[0].by_horizon.size() == 4);
}

TEST_CASE("Benchmark::run columns are sorted by horizon ascending")
{
    TimeSeries ts = make_constant_series(30);

    MovingAverageForecaster ma(3);

    std::array<const Forecaster*, 1> models{&ma};
    std::array<std::size_t, 3>       horizons{5, 1, 3};

    Benchmark benchmark;

    auto table = benchmark.run(models, ts, horizons);

    const auto& cols = table[0].by_horizon;
    for (std::size_t i = 1; i < cols.size(); ++i)
    {
        REQUIRE(cols[i - 1].horizon < cols[i].horizon);
    }
}

TEST_CASE("Benchmark::run model_label uses label() with parameters")
{
    TimeSeries ts = make_constant_series(20);

    MovingAverageForecaster ma(5);

    std::array<const Forecaster*, 1> models{&ma};
    std::array<std::size_t, 1>       horizons{1};

    Benchmark benchmark;

    auto table = benchmark.run(models, ts, horizons);

    REQUIRE(table[0].model_label == "MovingAverage(5)");
}

TEST_CASE("Benchmark::run constant series gives zero RMSE at every horizon")
{
    TimeSeries ts = make_constant_series(30);

    MovingAverageForecaster ma(3);

    std::array<const Forecaster*, 1> models{&ma};
    std::array<std::size_t, 3>       horizons{1, 2, 3};

    Benchmark benchmark;

    auto table = benchmark.run(models, ts, horizons);

    for (const auto& r : table[0].by_horizon)
    {
        REQUIRE(r.rmse == Approx(0.0));
    }
}
