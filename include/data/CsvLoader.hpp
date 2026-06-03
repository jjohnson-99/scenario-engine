#pragma once

#include <string>

#include "data/TimeSeries.hpp"

class CsvLoader
{
public:
    static TimeSeries load(const std::string& path);
};
