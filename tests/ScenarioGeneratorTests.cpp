#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "data/TimeSeries.hpp"
#include "forecasting/MovingAverageForecaster.hpp"
#include "forecasting/StochasticMovingAverageForecaster.hpp"
#include "simulation/ScenarioGenerator.hpp"

using Catch::Approx;

static TimeSeries make_series(int n, double value = 10.0)
{
    TimeSeries ts;
    for (int i = 0; i < n; ++i)
        ts.add({{}, value});
    return ts;
}

TEST_CASE("ScenarioGenerator produces the correct number of scenarios")
{
    auto ts = make_series(10);
    MovingAverageForecaster model(3);

    ScenarioGenerator sg{42};
    auto scenarios = sg.generate(model, ts, 10, 3);

    REQUIRE(scenarios.size() == 10);
}

TEST_CASE("Each scenario has history length plus horizon_steps observations")
{
    auto ts = make_series(10);
    MovingAverageForecaster model(3);

    ScenarioGenerator sg{42};
    auto scenarios = sg.generate(model, ts, 5, 4);

    for (const auto& s : scenarios)
    {
        REQUIRE(s.size() == 14);
    }
}

TEST_CASE("Deterministic model produces identical scenarios")
{
    auto ts = make_series(10);
    MovingAverageForecaster model(3);

    ScenarioGenerator sg{42};
    auto scenarios = sg.generate(model, ts, 5, 3);

    for (std::size_t i = 1; i < scenarios.size(); ++i)
    {
        for (std::size_t t = 0; t < scenarios[i].size(); ++t)
        {
            REQUIRE(scenarios[i].value_at(t) == Approx(scenarios[0].value_at(t)));
        }
    }
}

TEST_CASE("Stochastic model with fixed seed produces reproducible scenarios")
{
    TimeSeries ts;
    for (double v : {10.0, 12.0, 8.0, 14.0, 11.0, 9.0, 13.0, 10.0})
        ts.add({{}, v});

    StochasticMovingAverageForecaster model(3);
    model.calibrate(ts);

    ScenarioGenerator sg1{99};
    auto scenarios1 = sg1.generate(model, ts, 5, 3);

    ScenarioGenerator sg2{99};
    auto scenarios2 = sg2.generate(model, ts, 5, 3);

    for (std::size_t i = 0; i < scenarios1.size(); ++i)
    {
        for (std::size_t t = 0; t < scenarios1[i].size(); ++t)
        {
            REQUIRE(scenarios1[i].value_at(t) == Approx(scenarios2[i].value_at(t)));
        }
    }
}

TEST_CASE("Stochastic model produces varied scenarios")
{
    TimeSeries ts;
    for (double v : {10.0, 12.0, 8.0, 14.0, 11.0, 9.0, 13.0, 10.0})
        ts.add({{}, v});

    StochasticMovingAverageForecaster model(3);
    model.calibrate(ts);

    ScenarioGenerator sg{42};
    auto scenarios = sg.generate(model, ts, 20, 3);

    // With positive variance, the last simulated values should not all be identical
    double first_terminal = scenarios.front().value_at(scenarios.front().size() - 1);
    bool any_differ = false;
    for (const auto& s : scenarios)
    {
        if (s.value_at(s.size() - 1) != first_terminal)
        {
            any_differ = true;
            break;
        }
    }
    REQUIRE(any_differ);
}

TEST_CASE("Insufficient history throws")
{
    TimeSeries ts;
    ts.add({{}, 1.0});
    ts.add({{}, 2.0});

    MovingAverageForecaster model(5);
    ScenarioGenerator sg{42};

    REQUIRE_THROWS(sg.generate(model, ts, 1, 1));
}

TEST_CASE("Zero horizon_steps returns copies of history")
{
    auto ts = make_series(10);
    MovingAverageForecaster model(3);

    ScenarioGenerator sg{42};
    auto scenarios = sg.generate(model, ts, 3, 0);

    REQUIRE(scenarios.size() == 3);
    for (const auto& s : scenarios)
    {
        REQUIRE(s.size() == ts.size());
    }
}
