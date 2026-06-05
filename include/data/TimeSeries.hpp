#pragma once

#include <vector>

#include "data/Observation.hpp"
#include "data/TimeSeriesView.hpp"
#include "statistics/SummaryStatistics.hpp"

class TimeSeries
{
public:
    void add(Observation obs);

    std::size_t size() const;

    double value_at(std::size_t index) const;

    double mean() const;
    double variance() const;
    double standard_deviation() const;

    double min() const;
    double max() const;

    const std::vector<Observation>& observations() const;

    SummaryStatistics summary() const;

    TimeSeries first_n(std::size_t n) const;

    TimeSeriesView view() const;

    TimeSeriesView first_n_view(std::size_t n) const;

private:
    std::vector<Observation> data_;
};

