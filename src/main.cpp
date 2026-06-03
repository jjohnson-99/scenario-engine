#include <print>

#include "data/CsvLoader.hpp"
#include "forecasting/MovingAverageForecaster.hpp"
#include "forecasting/ExponentialSmoothingForecaster.hpp"
#include "forecasting/Backtester.hpp"

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

    MovingAverageForecaster ma(3);
    ExponentialSmoothingForecaster es(0.3);

    std::println("Moving Average Forecast:          {}",
        ma.forecast(series).value);

    std::println("Exponential Smoothing Forecast:   {}",
        es.forecast(series).value);

    Backtester backtester;

    auto ma_result = backtester.run(ma, series);
    auto es_result = backtester.run(es, series);

    std::println("Moving Average RMSE:              {}", ma_result.rmse);
    std::println("Exponential Smoothing RMSE:       {}", es_result.rmse);

    return 0;
}
