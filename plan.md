# Scenario Engine — Development Plan

## Vision

A production-style quantitative modeling platform built from first principles in modern C++. The intended end-to-end pipeline is:

```
Historical Data
      ↓
TimeSeries + TimeSeriesView
      ↓
Forecaster  →  ForecastResult
      ↓
Backtester  →  ModelEvaluationResult + ResidualSeries
      ↓
ResidualAnalyzer  →  ResidualStatistics
      ↓
Benchmark  →  MultiHorizonResult (ranked)
      ↓
[ScenarioGenerator  →  vector<TimeSeries>]    ← Phase 3
      ↓
[OptimizationEngine  →  Decision / Policy]    ← Phase 4
```

---

## What Has Been Built

### Infrastructure
- C++23, CMake 3.20+, vcpkg (Catch2 for tests)
- Directory layout: `include/{data,forecasting,evaluation,statistics,config}` mirrored by `src/`
- 32 passing tests across all layers
- CLAUDE.md in-repo guidance for future sessions

### Data Layer (`include/data/`, `src/data/`)
- `Observation` — timestamped double value (`std::chrono::sys_days`)
- `TimeSeries` — owning sequence with summary statistics (`mean`, `variance`, `min`, `max`, `standard_deviation`, `summary`)
- `TimeSeriesView` — zero-copy non-owning view over `std::span<const Observation>`; produced by `TimeSeries::view()` and `TimeSeries::first_n_view(n)` — eliminates the O(N²) copy cost in the walk-forward backtest loop
- `CsvLoader` — parses `date,value` CSV into `TimeSeries`
- `SummaryStatistics` (`include/statistics/`) — plain struct returned by `TimeSeries::summary()`

### Forecasting Layer (`include/forecasting/`, `src/forecasting/`)
- `Forecaster` — abstract base: `name()` (static id), `label()` (param-annotated display string), `forecast(TimeSeriesView)`, `minimum_observations()`, `horizon()`
- `ForecastResult` — `{mean, variance, horizon}`; `variance = 0.0` for current models, ready for uncertainty quantification
- `MovingAverageForecaster(window, horizon=1)` — direct horizon, not recursive
- `ExponentialSmoothingForecaster(alpha, horizon=1)` — direct horizon

### Evaluation Layer (`include/evaluation/`, `src/evaluation/`)
- `ModelEvaluationResult` — `{model_name, mae, rmse, bias, max_error, mape, error_std, evaluation_count, horizon}`
- `Backtester::evaluate(model, series[, horizon])` — walk-forward validation using `TimeSeriesView` slices (zero-copy); explicit horizon overload allows sweeping without model mutation
- `Backtester::residuals(model, series[, horizon])` — same walk-forward loop, returns `ResidualSeries` instead of aggregates
- `Benchmark::evaluate(models, series)` — single horizon, results ranked by RMSE ascending
- `Benchmark::run(models, series, horizons)` — sweeps all models × all horizons, returns `vector<MultiHorizonResult>`
- `CsvExporter` — flat CSV (one row per model × horizon) via `std::ostream&` or `std::filesystem::path`

### Residual Analysis Layer (`include/evaluation/`, `src/evaluation/`)
- `Residual` — `{actual, predicted, error}`
- `ResidualSeries` — thin `std::vector<Residual>` wrapper with `add()`, `size()`, `residuals()`
- `ResidualStatistics` — `{mean_error, variance, standard_deviation}`
- `ResidualAnalyzer::analyze(series)` — single-pass population statistics using `Var(e) = E[e²] − E[e]²`

### Modern C++ features introduced (incrementally)
| Feature | Where |
|---|---|
| `std::print` / `std::println` | `main.cpp` |
| `std::string_view` | `Forecaster::name()` return type |
| `std::span` | `TimeSeriesView`, `Benchmark::run` horizons parameter |
| `std::ranges::sort` with projection | `Benchmark::evaluate`, `Benchmark::run` |
| `std::format` | `label()` on both forecasters, `main.cpp` table formatting |
| `std::filesystem::path` | `CsvExporter` file overload |

---

## What Comes Next

### Phase 2 — Evaluation (in progress)

#### Residual diagnostics (immediate next)
The `ResidualAnalyzer` is the entry point; extend it with:
- **Autocorrelation function (ACF)** at lags 1…k — detects whether residuals are serially correlated (a sign the model is leaving structure in the errors)
- **Ljung-Box test** — formal hypothesis test for residual autocorrelation
- **Normality check** — skewness + kurtosis of the error distribution

#### Model comparison
- `ModelComparator` — given a `vector<ModelEvaluationResult>` for the same series, rank by multiple criteria (RMSE, MAE, MAPE) and report relative performance gaps

#### Benchmark datasets
- Add more `data/` CSV files (e.g., a seasonal series, a trending series) to make benchmarks meaningful

---

### Phase 3 — Simulation

#### Noise model abstraction
```
NoiseModel (abstract)
  GaussianNoise(mean, std_dev)
  BootstrapNoise(residual_series)   ← resamples from observed residuals
```

#### ScenarioGenerator
```cpp
ScenarioGenerator::generate(
    const Forecaster& model,
    const TimeSeries& history,
    const NoiseModel& noise,
    std::size_t n_scenarios,
    std::size_t horizon
) -> vector<TimeSeries>
```

Uses `Config::num_scenarios` and `Config::forecast_horizon` (currently stub fields). Each scenario is a simulated future path: forecast mean + sampled noise at each step.

#### C++23 opportunity
`std::generator<TimeSeries>` — yield scenarios lazily rather than materialising all `n_scenarios` at once. Natural fit once `<generator>` is available in the toolchain.

---

### Phase 4 — Optimization

Long-term; depends on Phase 3 producing scenario fans.

- **Objective** — define a decision problem (e.g., minimise expected cost subject to constraints)
- **Scenario-based optimisation** — solve the objective across all scenarios, report a robust policy
- **Constraint programming** — capacity limits, budget constraints, etc.

---

### Deferred design decisions

| Decision | Status |
|---|---|
| `TimeSeriesLike` concept | Deferred until a third "time-series-shaped" type appears (e.g., scenario matrix slices) |
| `std::expected<T, E>` for error returns | Deferred; exceptions are fine at current scale |
| `std::ranges` algorithms in `TimeSeries` internals | Deferred; manual loops are clear for now |
| `ForecastResult::variance` population | Currently always `0.0`; will be non-zero once probabilistic forecasters are added |
