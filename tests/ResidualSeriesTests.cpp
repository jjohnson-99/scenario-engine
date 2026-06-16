#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "evaluation/ResidualSeries.hpp"

using Catch::Approx;

TEST_CASE("ResidualSeries add and size")
{
    ResidualSeries rs;

    REQUIRE(rs.size() == 0);

    rs.add({.actual = 10.0, .predicted = 8.0, .error = 2.0});
    rs.add({.actual = 12.0, .predicted = 11.0, .error = 1.0});

    REQUIRE(rs.size() == 2);
}

TEST_CASE("ResidualSeries stores fields correctly")
{
    ResidualSeries rs;
    rs.add({.actual = 100.0, .predicted = 95.0, .error = 5.0});

    const auto& r = rs.residuals()[0];

    REQUIRE(r.actual == Approx(100.0));
    REQUIRE(r.predicted == Approx(95.0));
    REQUIRE(r.error == Approx(5.0));
}
