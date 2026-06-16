# Scenario Engine — Roadmap

## Where We Are

The pipeline currently runs end-to-end:

```
TimeSeries → Forecaster → calibrate() → ScenarioGenerator → vector<TimeSeries>
                ↓
           ResidualGenerator → ResidualAnalyzer → ResidualStatistics
                ↓
           Backtester → ModelEvaluationResult → Benchmark → CsvExporter
```

The foundation is solid. What follows are the natural next layers, ordered from most immediate to longest-term,
with reasoning for each decision.

---

## Stage 1 — Residual Diagnostics (Complete the Evaluation Layer)

**Why now:** Before trusting any scenario fan, we need to validate that the model's residuals are well-behaved.
The `ResidualAnalyzer` currently gives mean, variance, and standard deviation. That tells us the *size* of the
errors but nothing about their *structure*. Two problems with undiagnosed structure:

1. If residuals are **autocorrelated**, the model is systematically missing a pattern — yesterday's
   error predicts today's error. Scenarios built on top of such a model will systematically under- or
   overshoot reality.
2. If residuals are **non-normal** (heavy tails, skewness), the Gaussian noise assumption in
   `ScenarioGenerator` is wrong. The scenario fan will misrepresent the true distribution of outcomes.

### ACF — Autocorrelation Function

The autocorrelation at lag *k* measures the correlation between `error[t]` and `error[t-k]`. For a
well-specified model, all lags should be near zero (errors should be unpredictable from their own history).

Computing ACF for lags 1…K:

```
mean_e = mean of all errors
for each lag k in 1..K:
    numerator   = Σ (e[t] - mean_e)(e[t-k] - mean_e)   for t = k..n
    denominator = Σ (e[t] - mean_e)²                    for t = 0..n
    acf[k]      = numerator / denominator
```

The denominator is the same for every lag (total variance), so it can be computed once. This is O(N·K).

A new struct `AutocorrelationResult { std::vector<double> acf; std::size_t n_lags; }` would be returned
by an `acf(const ResidualSeries&, std::size_t n_lags)` method added to `ResidualAnalyzer`.

### Ljung-Box Test

Given the ACF at lags 1…K, the Ljung-Box statistic tests the *joint* null hypothesis that all
autocorrelations are zero:

```
Q = n(n+2) * Σ_{k=1}^{K} acf[k]² / (n - k)
```

Under the null, Q ~ χ²(K). The p-value is `1 - CDF_chi2(Q, K)`. A p-value below 0.05 is evidence of
autocorrelation — the model is mis-specified.

Computing χ² CDF without an external library is feasible using the incomplete gamma function, which can be
implemented in a small `include/statistics/` helper. Alternatively, the raw Q statistic and its degrees of
freedom can be returned and the user can look up the critical value — simpler to start.

### Normality Metrics — Skewness and Kurtosis

```
skewness = E[(e - mean)³] / std_dev³     (0 = symmetric)
kurtosis = E[(e - mean)⁴] / std_dev⁴    (3 = normal; >3 = heavy tails)
excess_kurtosis = kurtosis - 3           (0 = normal)
```

These can be added to `ResidualStatistics` directly. They require a third and fourth power accumulation
in the `ResidualAnalyzer` loop — still a single pass.

A `kurtosis >> 3` tells you that extreme errors occur more often than Gaussian predicts. This matters
enormously for risk: Gaussian scenarios will understate tail events.

---

## Stage 2 — NoiseModel Abstraction

**Why:** `ScenarioGenerator` currently hardcodes `N(0, √variance)`. That is just one possible noise model,
and it rests on the normality assumption diagnosed in Stage 1. Decoupling the noise model from the generator
makes it easy to swap in better alternatives.

### The abstraction

```cpp
// include/simulation/NoiseModel.hpp
class NoiseModel
{
public:
    virtual ~NoiseModel() = default;
    virtual double sample(std::mt19937& rng) const = 0;
};
```

`ScenarioGenerator` would take a `const NoiseModel&` and call `noise.sample(rng_)` at each step,
replacing the current inline `normal_distribution` construction.

### GaussianNoise

```cpp
class GaussianNoise : public NoiseModel
{
public:
    GaussianNoise(double mean, double std_dev);
    double sample(std::mt19937& rng) const override;
private:
    std::normal_distribution<double> dist_;
};
```

Equivalent to current behaviour. Note that `std::normal_distribution` is mutable-on-sample, so `dist_`
would need to be `mutable` (the same pattern as `rng_` in `ScenarioGenerator`).

### BootstrapNoise

The non-parametric alternative: instead of fitting a distribution, resample directly from the observed
residuals. No distributional assumption required.

```cpp
class BootstrapNoise : public NoiseModel
{
public:
    explicit BootstrapNoise(const ResidualSeries& series);
    double sample(std::mt19937& rng) const override;
private:
    std::vector<double> errors_;
    mutable std::uniform_int_distribution<std::size_t> dist_;
};
```

`sample()` draws a random index into `errors_` and returns that error. This preserves the actual shape
of the error distribution — fat tails, skewness, and all — at the cost of assuming the past error
distribution is representative of future errors.

**When to prefer Bootstrap over Gaussian:** when the Ljung-Box test passes (no autocorrelation) but the
kurtosis diagnostic shows heavy tails. Gaussian scenarios would understate extreme outcomes; bootstrap
scenarios sample from the actually-observed extremes.

### Connecting calibration to noise model selection

A natural workflow after Stage 1 + Stage 2:

```
calibrate()
    ↓
ResidualAnalyzer → skewness, kurtosis, Ljung-Box p-value
    ↓
if p_value < 0.05:
    model is mis-specified — diagnose before proceeding
elif abs(excess_kurtosis) < 1.0:
    GaussianNoise(0, std_dev)   // approximately normal
else:
    BootstrapNoise(residual_series)   // heavy tails, use empirical distribution
```

This could be automated in a future `NoiseModelSelector` or left as a manual decision — the diagnostics
give the user what they need to choose.

---

## Stage 3 — Scenario Fan Analysis

**Why:** `vector<TimeSeries>` is the raw output of `ScenarioGenerator`, but to make decisions you need
summary statistics *across* scenarios at each future time step, not just at the terminal point.

### ScenarioFan / ScenarioStatistics

A thin wrapper that takes `const vector<TimeSeries>&` and computes, at each future step `t`:

- Mean across scenarios: `E[path[t]]`
- Standard deviation across scenarios
- Percentile band (5th, 25th, 50th, 75th, 95th) — the "fan" shape

```cpp
struct StepStatistics
{
    double mean{};
    double std_dev{};
    double p05{};
    double p25{};
    double p50{};
    double p75{};
    double p95{};
};

class ScenarioFan
{
public:
    explicit ScenarioFan(const std::vector<TimeSeries>& scenarios, std::size_t history_length);
    const std::vector<StepStatistics>& steps() const;
    std::size_t horizon() const;
};
```

Computing percentiles requires sorting the values at each step across all `n_scenarios` paths —
`O(n_scenarios * log(n_scenarios))` per step. `std::ranges::nth_element` (partial sort) is more
efficient if you only need a few fixed percentiles.

### Risk Metrics

On top of the fan, two standard risk measures on the terminal distribution:

**Value at Risk (VaR):** the α-th percentile of terminal values. `VaR(5%) = p05` of the terminal
distribution. "With 95% probability, the terminal value will be above this level."

**Expected Shortfall / CVaR:** the *expected* value conditional on being in the worst α% of outcomes.
More informative than VaR because it averages the tail rather than just marking its boundary.

```
sort terminal values ascending
CVaR(5%) = mean of the bottom 5% of sorted terminals
```

Both can live on a `RiskMetrics` struct returned by a `RiskAnalyzer::analyze(ScenarioFan)` method.
These are the quantities that feed directly into Phase 4 optimization — the optimizer minimizes
expected cost subject to a CVaR constraint, for example.

---

## Stage 4 — More Forecasters

**Why:** A single model (StochasticMA) is not useful for benchmarking. The value of the evaluation layer
is comparing models. More forecasters also exercise the `Forecaster` interface and reveal whether the
abstraction holds up.

### StochasticExponentialSmoothingForecaster

The direct mirror of `StochasticMovingAverageForecaster`. Inherits from `Forecaster`, adds `calibrate()`,
stores `variance_`. The `forecast()` body is the same as `ExponentialSmoothingForecaster` — only
`variance` in the result changes.

### LinearTrendForecaster

Fits an ordinary least-squares linear regression to the last `window` observations and extrapolates.

```
Given values y_0 ... y_{w-1} at positions t = 0 ... w-1:
    slope     = (Σ t*y - w * mean_t * mean_y) / (Σ t² - w * mean_t²)
    intercept = mean_y - slope * mean_t
    forecast  = intercept + slope * (w - 1 + horizon)
```

This can be computed in two passes (or one pass with accumulated sums). It is deterministic by default
and can be made stochastic via the same `calibrate()` pattern. Linear trend is a natural counterpart to
moving average when the series has a clear slope.

### Drift Forecaster (Random Walk with Drift)

The simplest possible non-trivial model: the forecast is the last observed value plus the average
period-over-period change.

```
drift   = (y_{n-1} - y_0) / (n - 1)
forecast = y_{n-1} + horizon * drift
```

Useful as a baseline — if a more sophisticated model cannot beat a drift model, it likely is not
capturing anything beyond the trend.

---

## Stage 5 — Config Integration

**Why:** `Config` has `num_scenarios` and `forecast_horizon` as stubs since the initial commit.
Now that `ScenarioGenerator` exists, these fields have concrete meaning.

`ConfigLoader` presumably parses a config file. Once the fields are wired up, `main.cpp` can read:

```cpp
Config cfg = ConfigLoader::load("config.json");
auto scenarios = sg.generate(model, series, cfg.num_scenarios, cfg.forecast_horizon);
```

This also opens the door to a command-line driven workflow: different config files for different
experiments, without recompiling.

The config struct can be extended incrementally:

```cpp
struct Config
{
    int         forecast_horizon{1};
    int         num_scenarios{100};
    int         ma_window{3};
    double      es_alpha{0.3};
    std::string noise_model{"gaussian"};   // "gaussian" | "bootstrap"
    unsigned    rng_seed{42};
};
```

---

## Stage 6 — Phase 4: Optimization

**Why:** The stated end-goal of the platform is *decision-making under uncertainty*. The scenario fan
is the input; the optimizer is the output. This is the furthest-future stage but worth describing so
the earlier design decisions can be made with it in mind.

### The Problem Structure

A stochastic optimization problem has the form:

```
minimize   E[cost(decision, scenario)]
subject to constraints(decision)
```

where the expectation is approximated by averaging over the scenario fan. This is called **Sample
Average Approximation (SAA)** and is the standard approach for Monte Carlo-based optimization.

### SimpleOptimizer

The entry point: a scalar decision variable and a user-supplied cost function.

```cpp
class SimpleOptimizer
{
public:
    using CostFn = std::function<double(double decision, const TimeSeries& scenario)>;

    double optimize(
        CostFn                          cost,
        const std::vector<TimeSeries>&  scenarios,
        double                          lower_bound,
        double                          upper_bound,
        std::size_t                     grid_points = 100) const;
};
```

Grid search over `[lower_bound, upper_bound]` with `grid_points` candidates. For each candidate
decision, evaluate `cost(decision, scenario)` across all scenarios and take the mean. Return the
decision with the lowest expected cost. Simple, no dependencies, and useful for problems where
the decision space is one-dimensional (inventory level, hedge ratio, etc.).

### CVaR-Constrained Optimization

Once `RiskAnalyzer` exists (Stage 3), add a constraint:

```
minimize   E[cost(d, scenario)]
subject to CVaR_alpha(cost(d, scenario)) <= limit
```

This ensures the optimizer does not choose decisions that minimize expected cost at the expense of
catastrophic tail outcomes. Requires a two-level solve (binary search on the constraint, inner
minimize) but is still implementable without a solver library.

---

## Modern C++ Opportunities by Stage

| Stage | Feature | Where |
|---|---|---|
| Stage 1 | `std::ranges::transform` + accumulation | ACF computation |
| Stage 2 | `std::uniform_int_distribution` | BootstrapNoise |
| Stage 3 | `std::ranges::nth_element` | Percentile computation in ScenarioFan |
| Stage 3 | `std::mdspan` (C++23) | Representing the scenario matrix as a 2D view |
| Stage 4 | Concepts (`TimeSeriesLike`) | When LinearTrendForecaster makes it three types |
| Stage 5 | `std::expected<Config, std::string>` | ConfigLoader error returns |
| Stage 6 | `std::function` | CostFn in SimpleOptimizer |
| Stage 6 | `std::generator<TimeSeries>` (C++23) | Lazy scenario generation in ScenarioGenerator |

`std::mdspan` in Stage 3 is worth calling out. The scenario matrix is naturally a 2D structure:
`n_scenarios` rows × `horizon_steps` columns. Currently it is `vector<TimeSeries>`, which is fine,
but `std::mdspan` would let you take a view over the raw value data without any restructuring —
useful if computing percentiles across all scenarios at a given step becomes a hot path.

`std::generator` in Stage 6 would change `ScenarioGenerator::generate()` from materialising all
`n_scenarios` at once to yielding them lazily. The optimizer only needs one scenario in memory at a
time when computing expected cost, so this could eliminate the `vector<TimeSeries>` allocation
entirely for large scenario counts.

---

## Recommended Order of Attack

```
Stage 1a  →  ACF in ResidualAnalyzer          (small, validates existing infrastructure)
Stage 1b  →  Skewness + kurtosis              (single-pass addition to existing loop)
Stage 1c  →  Ljung-Box statistic              (builds on ACF)
Stage 2a  →  NoiseModel base class            (small, unlocks everything below)
Stage 2b  →  GaussianNoise refactor           (moves current inline logic into a class)
Stage 2c  →  BootstrapNoise                   (new capability, non-parametric)
Stage 3a  →  ScenarioFan + StepStatistics     (analysis of existing generator output)
Stage 3b  →  RiskAnalyzer (VaR, CVaR)         (builds on ScenarioFan)
Stage 4   →  LinearTrendForecaster + Drift    (more models for meaningful benchmarks)
Stage 5   →  Config wiring                    (connects UI to engine)
Stage 6   →  SimpleOptimizer                  (the payoff)
```

Stages 1–3 are entirely within the evaluation and simulation layers — no new abstraction layers needed,
just extensions to existing classes. They are the safest to implement incrementally. Stage 2 introduces
the first new virtual hierarchy since `Forecaster`. Stage 6 introduces `std::function` and closes the
loop on the platform's stated purpose.
