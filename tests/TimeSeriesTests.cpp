#include <catch2/catch_test_macros.hpp>

#include "data/TimeSeries.hpp"

TEST_CASE("Empty series")
{
    TimeSeries ts;

    REQUIRE(ts.size() == 0);
    REQUIRE(ts.mean() == 0.0);
}

TEST_CASE("Mean calculation")
{
    TimeSeries ts;

    ts.add({{}, 100.0});
    ts.add({{}, 200.0});

    REQUIRE(ts.mean() == 150.0);
}
