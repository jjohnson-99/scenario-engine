#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "evaluation/CsvExporter.hpp"
#include "evaluation/ModelEvaluationResult.hpp"

using Catch::Approx;

static ModelEvaluationResult make_result(
    std::string name, std::size_t horizon, double mae, double rmse, double bias, double max_error,
    double mape, double error_std, std::size_t count
)
{
    return {
        .model_name = std::move(name),
        .mae = mae,
        .rmse = rmse,
        .bias = bias,
        .max_error = max_error,
        .mape = mape,
        .error_std = error_std,
        .evaluation_count = count,
        .horizon = horizon
    };
}

TEST_CASE("CsvExporter writes correct header")
{
    ModelEvaluationResult r = make_result("MA(3)", 1, 1.0, 1.5, 0.5, 3.0, 2.0, 0.8, 10);

    std::ostringstream ss;
    CsvExporter exporter;
    exporter.write(std::span{&r, 1}, ss);

    const std::string output = ss.str();
    const std::string first_line = output.substr(0, output.find('\n'));

    REQUIRE(first_line == "model_name,horizon,mae,rmse,bias,max_error,mape,error_std,evaluation_count");
}

TEST_CASE("CsvExporter writes one data row per result")
{
    std::vector<ModelEvaluationResult> results = {
        make_result("MA(3)", 1, 1.0, 1.5, 0.1, 3.0, 2.0, 0.8, 10),
        make_result("MA(3)", 2, 1.2, 1.8, 0.2, 3.5, 2.2, 0.9, 9),
        make_result("ES(0.3)", 1, 0.9, 1.3, 0.0, 2.8, 1.8, 0.7, 10),
    };

    std::ostringstream ss;
    CsvExporter exporter;
    exporter.write(results, ss);

    // Count newlines: 1 header + 3 data rows = 4
    const std::string output = ss.str();
    const long newline_count = std::count(output.begin(), output.end(), '\n');

    REQUIRE(newline_count == 4);
}

TEST_CASE("CsvExporter round-trips field values")
{
    ModelEvaluationResult r = make_result("MovingAverage(3)", 2, 1.5, 2.0, -0.5, 4.0, 3.0, 1.1, 15);

    std::ostringstream ss;
    CsvExporter exporter;
    exporter.write(std::span{&r, 1}, ss);

    const std::string output = ss.str();

    REQUIRE(output.find("MovingAverage(3)") != std::string::npos);
    REQUIRE(output.find(",2,") != std::string::npos);   // horizon
    REQUIRE(output.find(",15\n") != std::string::npos); // evaluation_count at end of row
}

TEST_CASE("CsvExporter path overload creates a file")
{
    ModelEvaluationResult r = make_result("MA(3)", 1, 1.0, 1.5, 0.0, 3.0, 2.0, 0.8, 10);

    const std::filesystem::path tmp =
        std::filesystem::temp_directory_path() / "scenario_engine_test_export.csv";

    CsvExporter exporter;
    exporter.write(std::span{&r, 1}, tmp);

    REQUIRE(std::filesystem::exists(tmp));

    std::filesystem::remove(tmp);
}
