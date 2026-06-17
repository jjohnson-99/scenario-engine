#pragma once

#include "evaluation/ResidualSeries.hpp"
#include "evaluation/ResidualStatistics.hpp"

class ResidualAnalyzer
{
public:
    ResidualStatistics analyze(const ResidualSeries& series) const;

    AutocorrelationResult acf(const ResidualSeries& series, std::size_t n_lags) const;

    LjungBoxResult ljung_box(const AutocorrelationResult& acf_result, std::size_t n) const;
};
