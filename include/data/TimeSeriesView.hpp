#pragma once

#include <cstddef>
#include <span>

#include "data/Observation.hpp"

class TimeSeriesView
{
public:
    explicit TimeSeriesView(std::span<const Observation> data)
        : data_{data}
    {}

    std::size_t size() const
    {
        return data_.size();
    }

    double value_at(std::size_t index) const
    {
        return data_[index].value;
    }

    std::span<const Observation> observations() const
    {
        return data_;
    }

private:
    std::span<const Observation> data_;
};
