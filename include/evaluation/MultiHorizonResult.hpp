#pragma once

#include <string>
#include <vector>

#include "evaluation/ModelEvaluationResult.hpp"

struct MultiHorizonResult
{
    std::string                        model_label{};
    std::vector<ModelEvaluationResult> by_horizon{};
};
