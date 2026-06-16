#include "config/ConfigLoader.hpp"

Config ConfigLoader::load(const std::string&)
{
    return {.forecast_horizon = 30, .num_scenarios = 1000};
}
