#include <array>
#include <filesystem>
#include <format>
#include <print>
#include <vector>

#include "data/CsvLoader.hpp"
#include "forecasting/MovingAverageForecaster.hpp"
#include "forecasting/ExponentialSmoothingForecaster.hpp"
#include "evaluation/Benchmark.hpp"
#include "evaluation/CsvExporter.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::println(stderr, "Usage: scenario_engine <csv_file>");

        return 1;
    }

    TimeSeries series =
        CsvLoader::load(argv[1]);

    std::println("Observations: {}", series.size());
    std::println("Mean:         {}", series.mean());
    std::println("Variance:     {}", series.variance());
    std::println("Min:          {}", series.min());
    std::println("Max:          {}", series.max());
    std::println("");

    MovingAverageForecaster ma(3);
    ExponentialSmoothingForecaster es(0.3);

    std::println("Moving Average Forecast:        {}", ma.forecast(series.view()).mean);
    std::println("Exponential Smoothing Forecast: {}", es.forecast(series.view()).mean);
    std::println("");

    std::array<const Forecaster*, 2> models{&ma, &es};
    std::array<std::size_t, 4>       horizons{1, 2, 3, 4};

    Benchmark benchmark;

    auto table = benchmark.run(models, series, horizons);

    // Print multi-horizon RMSE table
    std::string header = std::format("{:<30}", "Model");
    for (const auto& row : table.front().by_horizon)
    {
        header += std::format("  H={:<6}", row.horizon);
    }
    std::println("{}", header);
    std::println("{}", std::string(header.size(), '-'));

    for (const auto& row : table)
    {
        std::string line = std::format("{:<30}", row.model_label);
        for (const auto& r : row.by_horizon)
        {
            line += std::format("  {:<8.4f}", r.rmse);
        }
        std::println("{}", line);
    }

    std::println("");

    // Flatten all results and export to CSV
    std::vector<ModelEvaluationResult> flat;
    for (const auto& row : table)
    {
        for (const auto& r : row.by_horizon)
        {
            flat.push_back(r);
        }
    }

    const std::filesystem::path csv_path{"results.csv"};

    CsvExporter exporter;
    exporter.write(flat, csv_path);

    std::println("Results written to {}", csv_path.string());

    return 0;
}
