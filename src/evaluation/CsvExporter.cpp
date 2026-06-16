#include "evaluation/CsvExporter.hpp"

#include <format>
#include <fstream>
#include <stdexcept>

void CsvExporter::write(std::span<const ModelEvaluationResult> results, std::ostream& out) const
{
    out << "model_name,horizon,mae,rmse,bias,max_error,mape,error_std,evaluation_count\n";

    for (const auto& r : results) {
        out << std::format(
            "{},{},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{}\n", r.model_name, r.horizon, r.mae,
            r.rmse, r.bias, r.max_error, r.mape, r.error_std, r.evaluation_count
        );
    }
}

void CsvExporter::write(
    std::span<const ModelEvaluationResult> results, const std::filesystem::path& path
) const
{
    std::ofstream file(path);

    if (!file) {
        throw std::runtime_error(std::format("Failed to open file for writing: {}", path.string()));
    }

    write(results, file);
}
