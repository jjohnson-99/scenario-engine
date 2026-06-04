#pragma once

#include <filesystem>
#include <ostream>
#include <span>

#include "evaluation/ModelEvaluationResult.hpp"

class CsvExporter
{
public:
    void write(
        std::span<const ModelEvaluationResult> results,
        std::ostream& out
    ) const;

    void write(
        std::span<const ModelEvaluationResult> results,
        const std::filesystem::path& path
    ) const;
};
