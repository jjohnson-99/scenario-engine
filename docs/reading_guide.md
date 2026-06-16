# Statistical Reading Guide

This guide maps the statistical concepts in this project to specific chapters and sections of
*All of Statistics* by Larry Wasserman (AoS), starting from Chapter 6. It also identifies
concepts that AoS does not cover and points to the standard alternative sources for those gaps.

Chapters 1–5 (probability foundations) are assumed known. The guide is organized by project
stage, not by chapter order, so you can read in the sequence that is most useful to the code
you are about to write.

---

## Part 1 — What AoS Covers, and Where

---

### Stage 1b/1c — Residual Statistics, Skewness, Kurtosis

**AoS Chapter 6: Models, Statistical Inference and Learning**

The `ResidualAnalyzer` computes the mean error, variance, and standard deviation of residuals.
These are *estimators* of the corresponding population quantities. Chapter 6 gives the framework
for thinking about what that means:

- **Bias and MSE of an estimator** (§6.3 area): the expected gap between an estimator and its
  target. This is the statistical foundation for the `bias` field in `ModelEvaluationResult` —
  a model with non-zero mean residual is biased. Understanding why an unbiased estimator is not
  always the best one (bias-variance tradeoff) connects directly to why MAE and RMSE can rank
  models differently.
- **Statistical functionals** (§6.4 area): mean, variance, and quantiles are all *functionals*
  of the error distribution — functions of the distribution itself, not just of the data. This
  framing becomes important in Chapter 7 and for understanding what a percentile of a scenario
  fan really is.

*What to skip in Ch 6:* the sections on sufficiency and completeness are mathematically rich but
not needed for this project at its current stage.

**AoS Chapter 9: Parametric Inference — specifically the MLE sections**

When `calibrate()` estimates `variance_` as the population variance of residuals, it is
implicitly computing the Maximum Likelihood Estimate of the variance parameter of a normal
distribution. Chapter 9 justifies this:

- **MLE (§9.3–9.5 area)**: why the sample mean and sample variance are the "right" estimators
  under a Gaussian assumption. The MLE of μ is the sample mean; the MLE of σ² is the
  *population* variance (divide by n), not the sample variance (divide by n−1). This is exactly
  what `ResidualAnalyzer` computes — and now you know why.
- **Asymptotic properties of MLEs** (§9.6 area): consistency (the estimate converges to the
  truth as n grows) and asymptotic normality. These properties matter when deciding how many
  historical observations are enough to trust a calibrated `variance_`.

*What to skip in Ch 9:* method of moments is a useful alternative estimator but not needed
immediately. The delta method is advanced — worth noting but not critical right now.

**On skewness and kurtosis:** AoS defines these as moments of a distribution (they appear
naturally as part of the moment-generating function discussion in Part 1) but does not develop
them as diagnostic tools in depth. The statistical test that uses skewness and kurtosis jointly
is the **Jarque-Bera test** — not covered in AoS. See the gap section below.

---

### Stage 1c — Ljung-Box Test

**AoS Chapter 10: Hypothesis Testing and p-values**

The Ljung-Box test is a specific hypothesis test, but its mechanics depend entirely on the
general framework that Chapter 10 builds. Read this chapter to understand what the test is
doing before you implement it — the Q statistic is just a number until you understand what
"p-value below 0.05" actually means:

- **The testing framework (§10.1–10.3)**: null hypothesis, test statistic, significance level,
  and p-value. Understand these precisely. A p-value is not the probability that the null is
  true — it is the probability of observing a test statistic at least as extreme as the one you
  computed, *assuming the null is true*. This distinction matters when interpreting a Ljung-Box
  result.
- **Type I and Type II errors, power (§10.4)**: Type I error is falsely concluding
  autocorrelation exists (false positive — you would unnecessarily reject a good model). Type II
  error is missing real autocorrelation (false negative — you use a mis-specified model for
  simulation). Choosing a significance level of 0.05 sets the Type I rate; the power determines
  how detectable autocorrelation is.
- **The chi-squared distribution** (review from Part 1 if needed): the Ljung-Box Q statistic
  follows a chi-squared distribution under the null. AoS covers chi-squared goodness-of-fit
  testing (§10.8 area), which uses the same distributional machinery. Reading this section gives
  you the intuition for why Q ~ χ²(K).

*What to skip in Ch 10:* the Neyman-Pearson lemma and uniformly most powerful tests are
theoretically elegant but not needed for implementing or interpreting Ljung-Box.

---

### Stage 2c — BootstrapNoise

**AoS Chapter 8: The Bootstrap**

This is the most directly applicable chapter in the entire book to the project. `BootstrapNoise`
*is* the nonparametric bootstrap applied to residuals. Read this chapter in full — it is one of
the shorter chapters and every section is relevant:

- **The plug-in principle (§8.1)**: the idea that you estimate a population quantity by computing
  the same quantity on the empirical distribution. `BootstrapNoise` computes the empirical
  distribution of residuals (a discrete uniform over the observed errors) and samples from it.
  That is the plug-in principle applied to the error distribution.
- **The nonparametric bootstrap (§8.2–8.3)**: resampling with replacement from observed data to
  approximate the sampling distribution of a statistic. The direct analogue: resampling from
  observed residuals to simulate future noise without assuming a parametric form.
- **When the bootstrap fails (§8.5 or equivalent)**: the bootstrap breaks down when the statistic
  of interest is sensitive to the tails of the distribution, or when the sample size is very
  small. For `BootstrapNoise`, this means: if your residual series is short (< 30 or so),
  the bootstrap will not explore the tail well. In that case, `GaussianNoise` may actually
  be more reliable despite making a stronger assumption.
- **Bootstrap confidence intervals (§8.4 area)**: this connects Stage 2 to Stage 3 —
  the percentile confidence interval computed by the bootstrap is the same machinery as the
  percentile bands in `ScenarioFan`.

Also read **Chapter 7: Estimating the CDF and Statistical Functionals** alongside Chapter 8,
since Chapter 8 builds directly on it:

- **The empirical CDF (§7.1)**: this is the distribution that `BootstrapNoise` is sampling from.
  Understanding the ECDF makes the bootstrap feel natural rather than like a black box.
- **Statistical functionals and the plug-in estimator (§7.2)**: formalizes what we are doing
  when we estimate the mean of future noise from the sample mean of past residuals.
- **Quantiles and their estimation (§7.3 area)**: directly relevant to `ScenarioFan` percentiles
  and VaR. The α-th quantile of the terminal distribution is VaR(α).

---

### Stage 3 — ScenarioFan, VaR, CVaR

**AoS Chapter 7: Estimating the CDF and Statistical Functionals (continued)**

The `ScenarioFan` computes percentile bands at each future time step. Each percentile is a
quantile of the empirical distribution of scenario values at that step. Chapter 7 covers:

- **Quantile estimation (§7.3 area)**: the plug-in estimate of the p-th quantile is the p-th
  quantile of the empirical distribution — exactly what `nth_element` gives you when you
  partially sort the scenario values at each step.
- **The Glivenko-Cantelli theorem**: the ECDF converges uniformly to the true CDF as n → ∞.
  For `ScenarioFan`, this means that percentile estimates improve as `n_scenarios` grows. It
  also tells you *how* they improve: the estimation error in quantiles decreases as O(1/√n).
  So 100 scenarios gives roughly 10% quantile estimation error; 10,000 gives ~1%.

**VaR and CVaR:** AoS covers quantiles thoroughly, and VaR is simply the α-th quantile of the
loss distribution. The CVaR calculation — the conditional expectation below the VaR threshold —
is also computable from AoS's treatment of conditional expectations. However, VaR and CVaR
as *risk management* quantities with their specific financial and regulatory interpretations
are not in AoS. See the gap section below.

---

### Stage 4 — LinearTrendForecaster

**AoS Chapter 13: Linear Regression**

The `LinearTrendForecaster` fits a line through the last `window` observations using OLS. This
is exactly what Chapter 13 covers:

- **OLS estimator derivation (§13.1–13.2)**: the slope and intercept formulas in the roadmap
  come directly from minimizing the sum of squared residuals. Chapter 13 derives this from
  first principles. After reading this, the formula in the roadmap is no longer a recipe —
  you know *why* it is the correct estimator.
- **Statistical inference on coefficients (§13.3–13.4)**: confidence intervals and hypothesis
  tests on the slope. A slope not significantly different from zero suggests the series has no
  trend — the `LinearTrendForecaster` would degrade to the mean, and a `MovingAverageForecaster`
  would be a better model. This gives you a criterion for when to use each model.
- **Residual analysis in regression (§13.5 area)**: the assumptions of OLS are: linearity,
  independence of errors (no autocorrelation — tested by Ljung-Box), constant variance
  (homoscedasticity — related to kurtosis), and normality of errors (tested by the
  skewness/kurtosis/Jarque-Bera diagnostics). Reading this section connects the residual
  diagnostics from Stage 1 to their regression-theoretic motivation — they test exactly whether
  OLS assumptions hold.
- **Prediction intervals vs confidence intervals**: the residual variance from OLS gives you a
  prediction interval for a new observation. This is the regression-theoretic version of what
  `calibrate()` does — it estimates the uncertainty around a forecast. In Stage 4, a stochastic
  `LinearTrendForecaster` would use this directly.

*What to skip in Ch 13:* the multivariate section (multiple regression with many predictors) is
not needed yet — our regression is always univariate (time index as the single predictor).

---

### Stage 6 — Optimization

**AoS Chapter 12: Statistical Decision Theory**

The optimizer's objective — `minimize E[cost(decision, scenario)]` — *is* statistical decision
theory. Chapter 12 is the theoretical foundation for Phase 4:

- **Loss functions and risk (§12.1–12.2)**: a loss function L(θ, δ) maps a true state θ
  and a decision δ to a real cost. Risk is the expected loss: R(θ, δ) = E[L(θ, δ(X))].
  The `SimpleOptimizer` minimizes exactly this, with the scenarios playing the role of X
  (observations of the uncertain future) and the decision variable playing the role of δ.
- **Bayes risk and the Bayes estimator (§12.3)**: the Bayes estimator minimizes expected loss
  under a prior distribution. When you average the cost function over scenarios, you are
  computing the Bayes risk under the empirical prior given by the scenario distribution.
  Understanding this connection clarifies why Sample Average Approximation is statistically
  principled.
- **Minimax risk (§12.4)**: instead of minimizing expected loss, minimize worst-case loss.
  This is the theoretical foundation for CVaR-constrained optimization — the CVaR constraint
  is a softer version of minimax that focuses on the worst α% rather than the absolute worst.

*What to skip in Ch 12:* the admissibility and complete class theorems are deep theory —
interesting but not needed for implementation.

---

## Part 2 — What AoS Does Not Cover

These topics appear in the roadmap or codebase but are outside the scope of AoS. The
recommendations below point to the most accessible sources, ordered by difficulty.

---

### Autocorrelation, ACF, and Time Series (Stage 1a)

**Not in AoS.** AoS is a cross-sectional statistics textbook — it assumes observations are
independent. Time series analysis relaxes this assumption and builds a theory of dependence
over time. Everything in Stage 1 (ACF, PACF, Ljung-Box, white noise tests) and Stage 4
(exponential smoothing theory, ARIMA) comes from this literature.

**Primary reference:**
*Forecasting: Principles and Practice* (3rd ed.) by Hyndman & Athanasopoulos.
Available free online at otexts.com/fpp3. Chapters 2 (time plots, autocorrelation),
5 (exponential smoothing), and 9–10 (ARIMA) are most directly relevant. This book is
deliberately applied — it covers the ACF, Ljung-Box, and model selection criteria (AIC, BIC)
without requiring measure theory.

**For more depth:**
*Introduction to Time Series and Forecasting* by Brockwell & Davis (2nd ed.). More rigorous
than Hyndman & Athanasopoulos. Chapters 1–3 (stationarity, ARMA models, ACF) and Chapter 5
(forecasting with ARIMA) map directly to Stage 1 and Stage 4 of the roadmap.

**Specific topics:**
- ACF and PACF — Hyndman Ch 2, Brockwell Ch 1
- Ljung-Box test — Hyndman Ch 2.8 (labeled as "portmanteau tests"), Brockwell Ch 1
- Exponential smoothing (ETS framework) — Hyndman Ch 5
- White noise and stationarity — Brockwell Ch 1

---

### Normality Tests — Jarque-Bera (Stage 1b)

**Not in AoS.** AoS covers skewness and kurtosis as moments and covers the general hypothesis
testing framework (Ch 10), but does not describe the Jarque-Bera test or other specific
normality tests such as Shapiro-Wilk or Kolmogorov-Smirnov.

The **Jarque-Bera test** combines skewness and kurtosis into a single test statistic:

```
JB = n/6 * (S² + (K-3)²/4)
```

where S is sample skewness and K is sample kurtosis. Under normality, JB ~ χ²(2).
The original paper is Jarque & Bera (1980), "Efficient Tests for Normality, Homoscedasticity
and Serial Independence of Regression Residuals." It is short and readable.

For a textbook treatment: any econometrics text covers it. *Introduction to Econometrics* by
Stock & Watson (Ch 17) or *Econometric Analysis* by Greene (Ch 4) both cover normality
testing in the context of regression residuals — exactly our use case.

**Practical note:** the Jarque-Bera test is known to have low power in small samples (< 100
observations). For the residual series this project generates from short time series, reporting
skewness and excess kurtosis as raw numbers — as planned — may be more informative than a
formal test with low power.

---

### Monte Carlo Simulation Methodology

**Partially in AoS.** AoS mentions simulation as a computational tool but does not develop it
as a methodology for uncertainty quantification. The theory of how many scenarios are needed,
variance reduction techniques, and convergence guarantees are outside AoS.

**Primary reference:**
*Simulation* by Sheldon Ross (5th ed.). Chapters 1–5 cover random number generation,
generating samples from specific distributions (the Box-Muller transform used internally by
`std::normal_distribution` is in Ch 5), and the theoretical basis for Monte Carlo estimation.
The key result: the standard error of a Monte Carlo estimate of E[X] is σ/√n, where σ is the
standard deviation of X and n is the number of scenarios. This gives you a formula for choosing
`n_scenarios`.

**For the finance application:**
*Monte Carlo Methods in Financial Engineering* by Glasserman. The first two chapters cover
everything relevant to this project: generating scenarios, estimating expectations, and
confidence intervals on Monte Carlo estimates. Later chapters (variance reduction: antithetic
variates, control variates) are useful once the basic simulation is working.

---

### Value at Risk and CVaR as Risk Measures

**Partially in AoS.** The quantile computation underlying VaR is in Ch 7. The conditional
expectation underlying CVaR is in Ch 1–5. But VaR and CVaR as formal risk measures — with
their properties (coherence, convexity), their role in regulation, and their limitations —
are finance and operations research topics not covered in AoS.

**The foundational paper on CVaR:**
Rockafellar & Uryasev (2000), "Optimization of Conditional Value-at-Risk," *Journal of Risk*.
This paper introduces CVaR, proves it is a coherent risk measure (unlike VaR), and gives an
equivalent linear programming formulation. It is the basis for the CVaR-constrained optimizer
in Stage 6. It is readable without an operations research background.

**Why VaR is insufficient:**
Artzner et al. (1999), "Coherent Measures of Risk," *Mathematical Finance*. This paper
establishes the axioms a risk measure should satisfy (monotonicity, subadditivity,
homogeneity, translation invariance) and proves that VaR violates subadditivity — meaning
that a portfolio of two assets can have higher VaR than the sum of the individual VaRs.
CVaR does not have this problem. Worth reading to understand *why* Stage 3 targets CVaR
rather than just percentiles.

---

### Sample Average Approximation and Stochastic Programming (Stage 6)

**Not in AoS.** The optimizer structure — `minimize E[cost(d, scenario)]` approximated by
the average over a finite set of scenarios — is called Sample Average Approximation (SAA)
and belongs to the stochastic programming literature.

**The clearest introduction:**
Shapiro, Dentcheva & Ruszczyński, *Lectures on Stochastic Programming* (2nd ed.), SIAM.
Chapter 5 covers SAA: convergence of the optimal value and optimal solution as the number
of scenarios increases, and the statistical properties of the SAA estimator. This is the
theoretical basis for asking "how many scenarios do I need?"

**More applied:**
Birge & Louveaux, *Introduction to Stochastic Programming* (2nd ed.). Chapter 1–3 gives
the problem formulation directly. The two-stage stochastic program (decide now, observe
uncertainty, recourse action) is the natural generalization of the `SimpleOptimizer`.

**Connection to AoS Ch 12:** AoS's treatment of Bayes risk (Ch 12) and SAA are the same
concept viewed from different traditions. In AoS's language, the scenario average is an
empirical Bayes estimate of the frequentist risk. Knowing both framings makes the connection
between statistics and operations research explicit.

---

### Heteroskedasticity and Volatility Clustering

**Not in AoS.** If residuals show variance that changes over time (large errors cluster
together, then small errors cluster), neither Gaussian nor bootstrap noise captures this.
This is *volatility clustering* and is modeled by GARCH (Generalized Autoregressive
Conditional Heteroskedasticity).

This is not in the current roadmap but is a natural Stage 4/5 extension: instead of a
constant `variance_` stored in the forecaster, the variance could be a function of recent
squared residuals.

**Reference:** Tsay, *Analysis of Financial Time Series* (3rd ed.), Chapters 3–4. The GARCH
model and its variants (EGARCH, GJR-GARCH) are covered with worked examples. The Engle
(1982) ARCH paper is also short and readable.

---

## Summary Table

| Project concept | In AoS? | Where in AoS | Alternative if not |
|---|---|---|---|
| Bias, MSE of estimators | Yes | Ch 6 | — |
| MLE, why sample variance is right | Yes | Ch 9 (MLE) | — |
| Hypothesis testing framework, p-values | Yes | Ch 10 | — |
| Chi-squared distribution (Ljung-Box) | Yes | Ch 10 | — |
| Empirical CDF, plug-in estimator | Yes | Ch 7 | — |
| Quantiles / percentiles (VaR, fan) | Yes | Ch 7 | — |
| The nonparametric bootstrap (BootstrapNoise) | Yes | Ch 8 | — |
| OLS linear regression (LinearTrendForecaster) | Yes | Ch 13 | — |
| Regression residual analysis | Yes | Ch 13 | — |
| Statistical decision theory (optimizer) | Yes | Ch 12 | — |
| Autocorrelation, ACF, PACF | **No** | — | Hyndman & Athanasopoulos (free); Brockwell & Davis |
| Ljung-Box test specifically | **No** | — | Hyndman Ch 2.8; Brockwell Ch 1 |
| Exponential smoothing theory | **No** | — | Hyndman & Athanasopoulos Ch 5 |
| Jarque-Bera normality test | **No** | — | Jarque & Bera (1980); Stock & Watson Ch 17 |
| Shapiro-Wilk, K-S normality tests | **No** | — | Any econometrics text |
| Monte Carlo simulation theory | **No** | — | Ross *Simulation*; Glasserman |
| VaR and CVaR as risk measures | **Partially** | Ch 7 (quantiles only) | Rockafellar & Uryasev (2000); Artzner et al. (1999) |
| Sample Average Approximation | **No** | — | Shapiro et al. *Lectures on Stochastic Programming* |
| GARCH / volatility modeling | **No** | — | Tsay *Analysis of Financial Time Series* Ch 3–4 |

---

## Recommended Reading Order

Given the project's stage-by-stage structure, the following sequence minimizes backtracking:

```
AoS Ch 6 (§ on bias, MSE)           ←  understand what ModelEvaluationResult fields mean
AoS Ch 7 (ECDF, quantiles)          ←  foundation for percentiles, VaR, and the bootstrap
AoS Ch 8 (Bootstrap, complete)      ←  directly implement BootstrapNoise after this
AoS Ch 9 (MLE sections only)        ←  understand why calibrate() uses population variance
AoS Ch 10 (testing framework + χ²)  ←  understand what the Ljung-Box p-value means
Hyndman FPP3 Ch 2                   ←  ACF, autocorrelation, white noise, Ljung-Box test
Hyndman FPP3 Ch 5                   ←  exponential smoothing theory
AoS Ch 13 (OLS + residual analysis) ←  LinearTrendForecaster and its regression diagnostics
AoS Ch 12 (decision theory)         ←  theoretical basis for the optimizer
Rockafellar & Uryasev (2000)        ←  CVaR-constrained optimization
```
