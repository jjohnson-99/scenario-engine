#pragma once

#include <chrono>

struct Observation
{
    std::chrono::sys_days timestamp;
    double value{};
};
