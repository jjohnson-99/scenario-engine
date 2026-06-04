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
    std::size_t evaluation_count{};
    std::size_t horizon{};
};
