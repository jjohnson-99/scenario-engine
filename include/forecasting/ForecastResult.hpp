#pragma once

#include <cstddef>

struct ForecastResult
{
    double value{};
    std::size_t horizon{1};
};
