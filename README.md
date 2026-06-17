# Scenario Engine

A modern C++ platform for forecasting, stochastic simulation, and optimization.

## Overview

Scenario Engine is an experimental project focused on building a production-style quantitative modeling platform from first principles using modern C++.

The long-term goal is to provide a framework for:

- Time series analysis
- Forecasting and predictive modeling
- Backtesting and model evaluation
- Stochastic scenario generation
- Monte Carlo simulation
- Optimization under uncertainty
- Quantitative decision support

The project is intentionally being developed incrementally, with a strong emphasis on software engineering, testing, maintainability, and modern C++ practices.

## Vision

Many quantitative systems evolve into collections of scripts, notebooks, and disconnected utilities.

Scenario Engine aims to explore a different approach:

```text
Data
  ↓
Time Series
  ↓
Forecasting Models
  ↓
Backtesting & Residual Diagnostics
  ↓
Scenario Generation
  ↓
Optimization
  ↓
Decision Support
```

The end result should resemble a small-scale quantitative research platform rather than a collection of individual algorithms.

Potential applications include:

- Quantitative finance
- Demand forecasting
- Supply chain optimization
- Risk analysis
- Scientific modeling
- Operations research
- Energy systems planning

## Current Status

The pipeline runs end-to-end from raw data through Monte Carlo scenario generation:

```text
TimeSeries → Forecaster → calibrate() → ScenarioGenerator → vector<TimeSeries>
                 ↓
        ResidualGenerator → ResidualAnalyzer → ResidualStatistics
                 ↓
        Backtester → ModelEvaluationResult → Benchmark → CsvExporter
```

Implemented components:

- Core `TimeSeries` abstraction with descriptive statistics
- CSV data loading
- Forecasting interface (`Forecaster` abstract base class)
- Moving average and exponential smoothing forecasters
- Stochastic forecaster with `calibrate()` (learns residual variance from walk-forward backtest)
- Multi-horizon backtesting framework (RMSE, MAE, MAPE, error std dev)
- Multi-model benchmark with CSV export
- Residual analysis: mean, variance, skewness, excess kurtosis
- Autocorrelation function (ACF) and Ljung-Box test statistic
- Monte Carlo scenario generation (`ScenarioGenerator`)
- 55 unit tests (Catch2)

## Design Principles

### Modern C++

The project is an opportunity to explore modern C++ development practices, including:

- C++23 language features
- RAII
- Value semantics
- Strong typing
- Generic programming
- Test-driven development

### Clear Separation of Responsibilities

Components have well-defined responsibilities:

```text
Data Layer       — TimeSeries, Observation, CsvLoader
Analysis Layer   — Backtester, Benchmark, ResidualAnalyzer, Metrics
Forecasting Layer — Forecaster interface, concrete models
Simulation Layer  — ScenarioGenerator
Optimization Layer — (planned)
```

### Extensibility

New forecasting models, simulation engines, and optimization strategies are addable without modifying existing infrastructure. The `Forecaster` interface is the primary extension point.

## Roadmap

### Phase 1 — Foundations

- [x] Time series abstraction
- [x] CSV loading
- [x] Statistics
- [x] Forecasting interface
- [x] Initial forecasting models
- [x] Unit testing

### Phase 2 — Evaluation

- [x] Multi-horizon backtesting
- [x] Additional metrics (RMSE, MAE, MAPE, error std dev)
- [x] Model comparison and benchmarking
- [x] CSV export of benchmark results

### Phase 3 — Simulation

- [x] Stochastic forecaster with calibration
- [x] Monte Carlo scenario generation
- [x] Residual diagnostics (skewness, kurtosis, ACF, Ljung-Box)
- [ ] NoiseModel abstraction (Gaussian, Bootstrap)
- [ ] Scenario fan analysis (percentile bands, VaR, CVaR)

### Phase 4 — Optimization

- [ ] Optimization abstractions
- [ ] Constraint definitions
- [ ] Scenario-based optimization (SAA)
- [ ] End-to-end workflows

## Example End-State Workflow

```text
Historical Data
        ↓
Forecast Model (calibrated)
        ↓
Residual Diagnostics (validate noise assumptions)
        ↓
Scenario Generation (Monte Carlo fan)
        ↓
Scenario Fan Analysis (VaR, CVaR, percentile bands)
        ↓
Optimization Engine
        ↓
Recommended Decision
```

## Motivation

This project serves two purposes:

1. Build a practical quantitative modeling platform.
2. Develop expertise in modern C++, software architecture, and quantitative systems engineering.

The emphasis is not only on implementing algorithms, but on constructing a maintainable and extensible system that could support increasingly sophisticated quantitative methods over time.

## License

TBD
