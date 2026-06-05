#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "evaluation/ResidualAnalyzer.hpp"
#include "evaluation/ResidualSeries.hpp"

using Catch::Approx;

static ResidualSeries make_series(std::initializer_list<double> errors)
{
    ResidualSeries rs;
    for (double e : errors)
    {
        rs.add({.actual = 0.0, .predicted = -e, .error = e});
    }
    return rs;
}

TEST_CASE("Zero-error series yields all zeros")
{
    ResidualSeries rs = make_series({0.0, 0.0, 0.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.mean_error        == Approx(0.0));
    REQUIRE(stats.variance          == Approx(0.0));
    REQUIRE(stats.standard_deviation == Approx(0.0));
}

TEST_CASE("Known errors yield correct statistics")
{
    // errors: {1, 2, 3}
    // mean = 2.0
    // variance = (1 + 4 + 9)/3 - 4 = 14/3 - 4 = 2/3
    // std_dev = sqrt(2/3)
    ResidualSeries rs = make_series({1.0, 2.0, 3.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.mean_error        == Approx(2.0));
    REQUIRE(stats.variance          == Approx(2.0 / 3.0));
    REQUIRE(stats.standard_deviation == Approx(std::sqrt(2.0 / 3.0)));
}

TEST_CASE("Symmetric errors yield zero mean")
{
    ResidualSeries rs = make_series({-3.0, -1.0, 1.0, 3.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.mean_error == Approx(0.0));
    REQUIRE(stats.variance   > 0.0);
}

TEST_CASE("Empty series throws")
{
    ResidualSeries rs;
    ResidualAnalyzer analyzer;

    REQUIRE_THROWS(analyzer.analyze(rs));
}
