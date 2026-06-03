# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

vcpkg manages dependencies. The toolchain file must be passed at configure time.

```bash
# Configure (required after cloning or changing CMakeLists.txt)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=/opt/homebrew/share/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

## Tests

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test case by name (Catch2 tag syntax)
./build/scenario_engine_tests "Moving average forecast"
```

## Run

```bash
./build/scenario_engine data/sample.csv
```

## Architecture

The project is an incremental build toward a quantitative modeling platform. The intended pipeline is:

```
Data → TimeSeries → Forecaster → Backtester → Scenario Generator → Optimizer
```

Only the first four stages are currently implemented. `Config` (`include/config/`) holds `forecast_horizon` and `num_scenarios`, which are placeholders for the simulation stage.

### Folder layout

Headers and sources mirror each other under `include/` and `src/`, split into three subdirectories:

| Subdirectory | Responsibility |
|---|---|
| `data/` | `TimeSeries`, `Observation`, `SummaryStatistics`, `CsvLoader` |
| `forecasting/` | `Forecaster` interface, concrete models, `Backtester`, `Metrics` |
| `config/` | `Config` struct, `ConfigLoader` |

`#include` paths are always relative to the `include/` root (e.g., `#include "forecasting/Backtester.hpp"`).

### CMake targets

- `scenario_engine_lib` — static library built from everything in `src/` except `main.cpp`
- `scenario_engine` — CLI executable; links `scenario_engine_lib`
- `scenario_engine_tests` — Catch2 test binary; links `scenario_engine_lib`

### Adding a forecasting model

1. Inherit from `Forecaster` (`include/forecasting/Forecaster.hpp`) and implement `forecast(const TimeSeries&)` and `minimum_observations()`.
2. Add the `.hpp` to `include/forecasting/` and the `.cpp` to `src/forecasting/`.
3. Add the `.cpp` path to `SCENARIO_ENGINE_SOURCES` in `CMakeLists.txt`.

### Backtester

`Backtester::run()` (`src/forecasting/Backtester.cpp`) performs walk-forward validation: for each time step `t` starting at `model.minimum_observations()`, it calls `model.forecast(series.first_n(t))` and compares the result to `series.value_at(t)`, accumulating MAE and RMSE.
