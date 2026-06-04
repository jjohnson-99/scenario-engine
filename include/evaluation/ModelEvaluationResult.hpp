#pragma once

#include <cstddef>
#include <string>

struct ModelEvaluationResult
{
    std::string model_name{};
    double      mae{};
    double      rmse{};
    double      bias{};
    double      max_error{};
    double      mape{};        // mean absolute percentage error (%), NaN if all actuals are zero
    double      error_std{};   // population std dev of signed errors
    std::size_t evaluation_count{};
    std::size_t horizon{};
};
