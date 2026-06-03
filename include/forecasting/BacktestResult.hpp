#pragma once

#include <cstddef>

struct BacktestResult
{
    double mae{};
    double rmse{};
    std::size_t forecasts{};
    std::size_t horizon{};
};
