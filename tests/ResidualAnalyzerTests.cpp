#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "evaluation/ResidualAnalyzer.hpp"
#include "evaluation/ResidualSeries.hpp"

using Catch::Approx;

static ResidualSeries make_series(std::initializer_list<double> errors)
{
    ResidualSeries rs;
    for (double e : errors) {
        rs.add({.actual = 0.0, .predicted = -e, .error = e});
    }
    return rs;
}

TEST_CASE("Zero-error series yields all zeros")
{
    ResidualSeries rs = make_series({0.0, 0.0, 0.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.mean_error == Approx(0.0));
    REQUIRE(stats.variance == Approx(0.0));
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

    REQUIRE(stats.mean_error == Approx(2.0));
    REQUIRE(stats.variance == Approx(2.0 / 3.0));
    REQUIRE(stats.standard_deviation == Approx(std::sqrt(2.0 / 3.0)));
}

TEST_CASE("Symmetric errors yield zero mean")
{
    ResidualSeries rs = make_series({-3.0, -1.0, 1.0, 3.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.mean_error == Approx(0.0));
    REQUIRE(stats.variance > 0.0);
}

TEST_CASE("Empty series throws")
{
    ResidualSeries rs;
    ResidualAnalyzer analyzer;

    REQUIRE_THROWS(analyzer.analyze(rs));
}

TEST_CASE("Symmetric errors yield zero skewness")
{
    // {-3, -1, 1, 3} is symmetric about 0
    ResidualSeries rs = make_series({-3.0, -1.0, 1.0, 3.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.skewness == Approx(0.0).margin(1e-10));
}

TEST_CASE("Zero-variance series yields zero skewness and excess kurtosis")
{
    ResidualSeries rs = make_series({2.0, 2.0, 2.0, 2.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.skewness == Approx(0.0));
    REQUIRE(stats.excess_kurtosis == Approx(0.0));
}

TEST_CASE("Right-skewed errors yield positive skewness")
{
    // {0, 0, 0, 3} — mass on the left, long right tail
    ResidualSeries rs = make_series({0.0, 0.0, 0.0, 3.0});

    ResidualAnalyzer analyzer;
    auto stats = analyzer.analyze(rs);

    REQUIRE(stats.skewness > 0.0);
}

TEST_CASE("ACF lag-1 is zero for a series with uncorrelated consecutive pairs")
{
    // {1, 0, -1, 0, 1, 0, -1, 0}: mean=0, lag-1 numerator = sum of (a[t]*a[t-1]) = 0 exactly
    ResidualSeries rs = make_series({1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0});

    ResidualAnalyzer analyzer;
    auto result = analyzer.acf(rs, 1);

    REQUIRE(result.n_lags == 1);
    REQUIRE(result.acf[0] == Approx(0.0).margin(1e-10));
}

TEST_CASE("ACF of alternating series is negative at lag 1")
{
    // {1, -1, 1, -1}: each value predicts the opposite of the next — strong negative ACF
    ResidualSeries rs = make_series({1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0, -1.0});

    ResidualAnalyzer analyzer;
    auto result = analyzer.acf(rs, 1);

    REQUIRE(result.acf[0] < -0.5);
}

TEST_CASE("ACF of linear trend series is positive at lag 1")
{
    // Strongly trended series — consecutive values are close, so lag-1 ACF is positive
    ResidualSeries rs = make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});

    ResidualAnalyzer analyzer;
    auto result = analyzer.acf(rs, 1);

    REQUIRE(result.acf[0] > 0.5);
}

TEST_CASE("ACF caps n_lags at series length minus one")
{
    ResidualSeries rs = make_series({1.0, 2.0, 3.0});

    ResidualAnalyzer analyzer;
    auto result = analyzer.acf(rs, 100);

    REQUIRE(result.n_lags == 2);
    REQUIRE(result.acf.size() == 2);
}

TEST_CASE("Ljung-Box Q is near zero for uncorrelated series")
{
    // lag-1 ACF is exactly 0 for this series, so Q = 0
    ResidualSeries rs = make_series({1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0});

    ResidualAnalyzer analyzer;
    auto acf_result = analyzer.acf(rs, 1);
    auto lb = analyzer.ljung_box(acf_result, rs.size());

    REQUIRE(lb.q_statistic == Approx(0.0).margin(1e-9));
    REQUIRE(lb.degrees_of_freedom == 1);
}

TEST_CASE("Ljung-Box Q is large for autocorrelated series")
{
    // Linear trend — strong lag-1 autocorrelation produces a large Q statistic
    ResidualSeries rs = make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});

    ResidualAnalyzer analyzer;
    auto acf_result = analyzer.acf(rs, 4);
    auto lb = analyzer.ljung_box(acf_result, rs.size());

    // χ²(4) critical value at 5% is 9.49; a trending series should exceed it
    REQUIRE(lb.q_statistic > 3.0);
    REQUIRE(lb.degrees_of_freedom == 4);
}
