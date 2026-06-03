#include "data/CsvLoader.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

TimeSeries CsvLoader::load(const std::string& path)
{
    std::ifstream file(path);

    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    TimeSeries series;

    std::string line;

    // Skip header row
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        std::string date_str;
        std::string value_str;

        std::getline(ss, date_str, ',');
        std::getline(ss, value_str);

        int y{};
        unsigned m{};
        unsigned d{};

        std::stringstream date_stream(date_str);

        char dash1{};
        char dash2{};

        date_stream
            >> y
            >> dash1
            >> m
            >> dash2
            >> d;

        Observation obs{
            std::chrono::sys_days{
                std::chrono::year_month_day{
                    std::chrono::year{y},
                    std::chrono::month{m},
                    std::chrono::day{d}
                }
            },
            std::stod(value_str)
        };

        series.add(obs);
    }

    return series;
}
