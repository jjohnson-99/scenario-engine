#include "evaluation/Metrics.hpp"

double Metrics::squared_error(double actual, double predicted)
{
    const double diff = actual - predicted;

    return diff * diff;
}
