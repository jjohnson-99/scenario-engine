#pragma once

#include <string>

#include "config/Config.hpp"

class ConfigLoader
{
public:
    static Config load(
        const std::string& path
    );
};
