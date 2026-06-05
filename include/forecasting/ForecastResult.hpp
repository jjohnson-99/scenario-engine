#pragma once

#include <cstddef>

struct ForecastResult
{
    double      mean{};
    double      variance{};
    std::size_t horizon{1};
};
