#pragma once

#include <vector>

#include "evaluation/Residual.hpp"

class ResidualSeries
{
public:
    void add(Residual r)
    {
        residuals_.push_back(r);
    }

    std::size_t size() const
    {
        return residuals_.size();
    }

    const std::vector<Residual>& residuals() const
    {
        return residuals_;
    }

private:
    std::vector<Residual> residuals_;
};
