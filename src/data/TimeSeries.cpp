#include "data/TimeSeries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

void TimeSeries::add(Observation obs)
{
    data_.push_back(obs);
}

std::size_t TimeSeries::size() const
{
    return data_.size();
}

double TimeSeries::value_at(std::size_t index) const
{
    return data_.at(index).value;
}

double TimeSeries::mean() const
{
    if (data_.empty())
    {
        return 0.0;
    }

    double total = 0.0;

    for (const auto& obs : data_)
    {
        total += obs.value;
    }

    return total / static_cast<double>(data_.size());
}

double TimeSeries::variance() const
{
    if (data_.size() < 2)
    {
        return 0.0;
    }

    const double mu = mean();

    double sum = 0.0;

    for (const auto& obs : data_)
    {
        const double diff = obs.value - mu;
        sum += diff * diff;
    }

    return sum / static_cast<double>(data_.size() - 1);
}

double TimeSeries::standard_deviation() const
{
    return std::sqrt(variance());
}

double TimeSeries::min() const
{
    if (data_.empty())
    {
        return 0.0;
    }

    double result = std::numeric_limits<double>::max();

    for (const auto& obs : data_)
    {
        result = std::min(result, obs.value);
    }

    return result;
}

double TimeSeries::max() const
{
    if (data_.empty())
    {
        return 0.0;
    }

    double result = std::numeric_limits<double>::lowest();

    for (const auto& obs : data_)
    {
        result = std::max(result, obs.value);
    }

    return result;
}

const std::vector<Observation>& TimeSeries::observations() const
{
    return data_;
}

SummaryStatistics TimeSeries::summary() const
{
    return {
        .mean = mean(),
        .variance = variance(),
        .minimum = min(),
        .maximum = max()
    };
}

TimeSeries TimeSeries::first_n(std::size_t n) const
{
    TimeSeries result;

    n = std::min(n, data_.size());

    for (std::size_t i = 0; i < n; ++i)
    {
        result.add(data_[i]);
    }

    return result;
}

TimeSeriesView TimeSeries::view() const
{
    return TimeSeriesView{data_};
}

TimeSeriesView TimeSeries::first_n_view(std::size_t n) const
{
    n = std::min(n, data_.size());
    return TimeSeriesView{std::span<const Observation>{data_.data(), n}};
}
