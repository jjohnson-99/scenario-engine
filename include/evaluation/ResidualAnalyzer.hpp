#pragma once

#include "evaluation/ResidualSeries.hpp"
#include "evaluation/ResidualStatistics.hpp"

class ResidualAnalyzer
{
public:
    ResidualStatistics analyze(const ResidualSeries& series) const;
};
