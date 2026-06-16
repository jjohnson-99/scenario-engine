# C++ Features — Interview Reference

This document covers every modern C++ feature used in this project, why each was a natural choice,
and how to discuss them at depth in a technical interview. The second half covers features not yet
used, with discussion of when they would arise naturally here and what interviewers probe on each.

A theme runs through everything: **zero-cost abstractions**. C++ repeatedly offers you the ability
to express intent clearly — ownership, immutability, type safety, lifetime — with no runtime penalty
over equivalent hand-written C. The language features below are that principle made concrete.

---

# Part 1 — Features Already in Use

---

## 1. Non-Owning Views: `std::string_view` and `std::span`

These two types share a philosophy: a *view* is a non-owning, read-only window into data that
someone else owns. The view itself is cheap to copy (two words: pointer + size) and involves no
heap allocation.

### `std::string_view` — `Forecaster::name()`

`name()` returns `std::string_view` pointing to a string literal in static storage. The literal
lives for the entire program lifetime, so the view is always valid.

**Why not `const std::string&`?**
A `const std::string&` parameter binds to a `std::string`, potentially triggering Small String
Optimization (SSO) or a heap allocation if the string is long. Returning `const std::string&` from
a function is dangerous: the referenced object must outlive the caller. `string_view` sidesteps
both: it copies two words and never allocates.

**Why not `const char*`?**
`const char*` does not carry length; computing `strlen` is O(N) and error-prone. `string_view` has
`size()` and does not require null termination.

**What the old call sites looked like.**
Before `string_view`, a function accepting a string-like argument had no good option:

```cpp
// C++03 — you had to pick one; none handles all callers cheaply:
void print_name(const std::string& name);  // copies if caller passes a literal
void print_name(const char* name);         // no length, must null-terminate
void print_name(std::string name);         // always copies

// C++17 string_view — one overload, handles all callers, zero allocation:
void print_name(std::string_view name);

print_name("MovingAverage");           // OK: points to literal, no allocation
print_name(std::string{"formatted"});  // OK: binds to string's buffer directly
print_name(some_string_view);          // OK: copied by value (two words)
```

**The hard interview question: dangling views.**

```cpp
std::string_view dangerous()
{
    std::string s = "hello";
    return s;          // UB: view outlives the string
}

std::string_view also_dangerous(std::string s)
{
    return s;          // UB: parameter destroyed at end of call
}
```

The rule: a `string_view` is only safe as long as its underlying data lives. Never store a
`string_view` received as a parameter beyond the function's scope without verifying the source's
lifetime. This is why `string_view` member variables are almost always wrong.

**Follow-up: why does `std::string` have SSO?**
For strings under ~15 characters (ABI-dependent), `std::string` stores the data inline in the
object rather than heap-allocating. This makes short strings free to copy and avoids cache misses.
`string_view` is better still — it avoids the object entirely.

---

### `std::span<const Observation>` — `TimeSeriesView`, Benchmark parameters

`std::span` is `string_view` generalised to any contiguous sequence of `T`. `TimeSeriesView` wraps
a `span<const Observation>` to provide a zero-copy slice of a `TimeSeries`.

**What the alternative looked like.**
Before `std::span`, passing a contiguous range into a function required a raw pointer plus a
separate size — two parameters that the type system cannot bind together:

```cpp
// C++03/11/14 — caller must pass pointer and size separately:
void process(const Observation* data, std::size_t count);

process(ts.data(), ts.size());               // OK
process(ts.data() + 5, ts.size());           // BUG: size not adjusted — silent overread
process(ts.data() + 5, ts.size() - 5);       // correct, but manual and fragile
```

The two parameters can fall out of sync. The compiler cannot catch the mismatch. `std::span`
bundles the pointer and size into one type, making it impossible to pass an inconsistent pair:

```cpp
void process(std::span<const Observation> view);

process(ts.observations());         // size is bundled; can't desync
process(ts.observations().last(5)); // subspan arithmetic is inside span; caller can't get it wrong
```

**The hard interview question: `span<const T>` vs `const span<T>`.**

```cpp
std::span<const Observation> a;   // span over immutable Observations; you can reseat 'a'
const std::span<Observation>  b;  // span over mutable Observations; you cannot reseat 'b'
```

`const` on the `span` object means you cannot rebind the span. `const` on the element type means
you cannot modify the elements through the span. In practice you almost always want
`span<const T>` — you want to prevent mutation of the data, not rebinding of the view.

**`span::last(window_)`** — used in `forecast()`:

```cpp
for (const auto& obs : series.observations().last(window_))
{
    sum += obs.value;
}
```

Before `span::last`, the same loop required manual pointer arithmetic:

```cpp
// Old style — arithmetic is correct but the intent is buried:
const auto& obs_vec = series.observations();
const std::size_t start = obs_vec.size() - window_;
for (std::size_t i = start; i < obs_vec.size(); ++i)
    sum += obs_vec[i].value;

// span::last — the intent "last N elements" is stated directly:
for (const auto& obs : series.observations().last(window_))
    sum += obs.value;
```

Both compile to identical pointer arithmetic. The difference is readability and the elimination
of the off-by-one risk in manually computing `start`.

**Follow-up: static vs dynamic extent.**
`std::span<T, N>` (static extent) bakes the size into the type, enabling the compiler to elide the
size field entirely. `std::span<T>` (dynamic extent, which we use) stores pointer + size. Static
extent is useful when sizes are known at compile time — e.g., a fixed-size sliding window — but
dynamic extent is the right default for runtime-sized data.

**Follow-up: how does span relate to the aliasing rules?**
`span` is a pointer + size; accessing through it is subject to the strict aliasing rule just like
any pointer. `span<const T>` does not promise the data is const in physical memory — it only
prevents you from mutating it through *this view*. Another `span<T>` to the same data can still
modify it. This matters in concurrent code.

---

## 2. Runtime Polymorphism: Virtual Dispatch, Abstract Base Classes, `override`

`Forecaster` is the central abstraction. Every component that evaluates or generates scenarios
takes a `const Forecaster&` — it does not know whether it holds a `MovingAverageForecaster`,
`ExponentialSmoothingForecaster`, or `StochasticMovingAverageForecaster`.

### How virtual dispatch works

The compiler builds a **vtable** (virtual dispatch table) for each class that has at least one
virtual function. The vtable is an array of function pointers, one per virtual function. Every
object of a polymorphic class carries a hidden pointer (the **vptr**) to its class's vtable,
typically as the first word of the object.

When you call `model.forecast(view)`:

```
1. Load vptr from model's memory (one dereference)
2. Index into vtable at the forecast slot (one add + load)
3. Call the function pointer (indirect call — cannot be inlined by default)
```

Cost: one extra memory load per virtual call. On modern hardware with a warm cache, this is
~1–3 ns. The real cost is **inhibited inlining**: the compiler cannot see through the indirect
call to fold constants or vectorise the body.

**Why `virtual ~Forecaster() = default` is not optional.**

If you `delete` a derived object through a base pointer without a virtual destructor, the
behaviour is undefined. The base destructor runs, the derived destructor does not. Any
resources the derived class owns (heap memory, file handles, locks) leak silently. This is one
of the most common C++ bugs in codebases that use inheritance. The fix is always: if a class is
designed to be used polymorphically via pointer or reference, its destructor must be virtual.

**`= 0` — pure virtual functions.**

A pure virtual function declares an interface contract. The abstract class itself cannot be
instantiated (the vtable slot has no implementation). Any concrete class that fails to implement
all pure virtuals is also abstract and cannot be instantiated — the compiler enforces this.

Note: pure virtual functions *can* have a body, which derived classes can call explicitly via
`Base::fn()`. This is rare but useful for providing a default implementation that subclasses
can optionally invoke.

**The `override` keyword — always use it.**

```cpp
ForecastResult forecast(const TimeSeriesView& series) override;
```

Without `override`, a signature mismatch silently creates a *new* virtual function rather than
overriding the base — and the compiler says nothing:

```cpp
class Forecaster {
public:
    virtual ForecastResult forecast(const TimeSeriesView& v) const = 0;
};

class MovingAverageForecaster : public Forecaster {
public:
    // Missing 'const' — this is NOT an override. It is a brand-new virtual function.
    // The compiler accepts this without warning.
    ForecastResult forecast(const TimeSeriesView& v) { ... }
};

const MovingAverageForecaster ma(3);
const Forecaster& f = ma;
f.forecast(view);   // dispatches to Forecaster::forecast — pure virtual, runtime abort
ma.forecast(view);  // dispatches to MovingAverageForecaster::forecast — works
// Two calls that look the same produce completely different results. Silent.

// With override, the compiler catches this at compile time:
ForecastResult forecast(const TimeSeriesView& v) override { ... }
// error: 'forecast' marked 'override' but does not override any member function
```

`override` makes the compiler reject the mismatch at compile time. This is not optional safety — it is correct C++.

**The hard interview question: the slicing problem.**

```cpp
MovingAverageForecaster ma(3);
Forecaster f = ma;   // slices: copies only the Forecaster base subobject
f.forecast(view);    // calls... which forecast()?
```

The copy drops the derived part. `f.forecast()` calls `Forecaster::forecast()`, which is pure
virtual and therefore undefined behaviour (or a pure virtual call abort at runtime). Polymorphism
only works through pointers or references — value semantics and inheritance do not mix. This is
why `Backtester` and `ScenarioGenerator` take `const Forecaster&`, never `Forecaster`.

---

## 3. Designated Initializers

```cpp
return {
    .mean     = sum / static_cast<double>(window_),
    .variance = 0.0,
    .horizon  = horizon_
};
```

**What they are.** For aggregate types (no user-provided constructors, no private members,
no virtual functions), you can name the fields you are initialising. This requires C++20; C99
had them but C++03/11/14/17 did not.

**Why they matter.** Without designated initializers:

```cpp
return {sum / window, 0.0, horizon_};  // positional: what is 0.0? which field is horizon_?
```

The positional form is fragile: if you add a field or reorder the struct, every call site silently
initialises the wrong field with the wrong value. Named initializers tie the code to the struct's
field names, not its layout.

**The hard constraint: order.**
Designated initializers must appear in the same order as the struct declaration. You cannot
skip to a later field and then come back. The compiler enforces this; it is not a limitation of
the implementation but a deliberate language rule to prevent misuse.

**Follow-up: what is an aggregate?**
An aggregate (C++20 definition) is a class with: no user-provided constructors, no private or
protected non-static data members, no virtual functions, no virtual, private, or protected base
classes. Standard library types like `std::array` are aggregates; `std::vector` is not. Aggregates
support aggregate initialisation (braced-init with positional or designated fields).

---

## 4. Default Member Initialization

```cpp
// In StochasticMovingAverageForecaster.hpp
double variance_{0.0};
```

Initializes `variance_` to `0.0` for every object, before the constructor body runs, regardless
of which constructor is used.

**Why this matters for correctness.** Before C++11, every constructor had to explicitly initialize
every member, or the member would hold an indeterminate value (a read would be UB). Default member
initializers make the "safe default" part of the type definition, not a burden on every constructor
author.

**Interaction with the constructor initializer list.**

```cpp
StochasticMovingAverageForecaster::StochasticMovingAverageForecaster(
    std::size_t window, std::size_t horizon)
    : window_(window)
    , horizon_(horizon)
    // variance_ is NOT in the initializer list — default applies: 0.0
{
}
```

If a member appears in the constructor's member initializer list, that value wins over the default
member initializer. If it does not appear, the default applies. This is deterministic and
well-defined.

**Follow-up: initialization order.**
Members are always initialized in *declaration order*, not in the order they appear in the
initializer list. A well-configured compiler (`-Wreorder` / `-Wall`) will warn if your initializer
list order does not match declaration order, because out-of-order initialization can cause subtle
bugs (a member that depends on another member that isn't initialized yet).

---

## 5. `mutable` — Logical Const vs Bitwise Const

```cpp
// ScenarioGenerator.hpp
mutable std::mt19937 rng_;
```

A `const` member function promises not to modify the object's observable state — its *logical*
state. But `mt19937` must advance its internal state every time you draw a sample. That internal
state is an *implementation detail*, not part of the logical state of the generator. `mutable`
communicates this precisely: "this member may change in const methods, but it does not affect the
object's observable behaviour."

**Legitimate uses of `mutable`:**
- RNG state (as here)
- Mutex members (locking is not logical mutation)
- Cached/lazy-computed values (compute once, store)

**The alternatives and why they are worse.**

*Option 1 — make `generate()` non-const:*
```cpp
std::vector<TimeSeries> generate(...);  // non-const member
```
A caller holding `const ScenarioGenerator& sg` cannot call this. But the generator has no
observable state that changes — only its internal RNG position advances. Removing `const` leaks
an implementation detail into the public API and forces unnecessary non-const access everywhere.

*Option 2 — pass the RNG as a parameter:*
```cpp
std::vector<TimeSeries> generate(..., std::mt19937& rng) const;
```
Legitimate in contexts where the caller owns the RNG and needs deterministic control across
multiple generators. The downside: `ScenarioGenerator` no longer encapsulates its randomness source;
every call site must supply an engine. For a class whose purpose is generating random scenarios,
this is an unnatural burden on callers.

*Option 3 — store a pointer to an external RNG:*
```cpp
std::mt19937* rng_;  // non-owning pointer
```
Introduces lifetime complexity — the pointed-to engine must outlive the generator. Ownership is
unclear. `mutable` with an owned engine is simpler and safer.

**The hard interview question: `mutable` and thread safety.**

`const` on a member function is a promise to the type system, not a thread-safety guarantee.
Two threads calling `const` methods concurrently is expected to be safe — the standard says const
methods must be *thread-safe* (§[res.on.data.races]). But `mutable rng_` breaks this: two threads
calling `generate()` concurrently would race on `rng_`. The fix is to make `rng_` thread-local,
pass it as a parameter, or protect it with `mutable std::mutex mtx_`.

This is one of the sharpest interview questions on `mutable` — the answer reveals whether a
candidate knows the connection between `const` and the memory model.

---

## 6. Move Semantics — `std::move` and Rvalue References

```cpp
scenarios.push_back(std::move(path));
```

After the inner loop builds a complete `TimeSeries` path (copying history + appending simulated
values), `std::move` transfers ownership of path's internal `vector<Observation>` buffer into
the `scenarios` vector — no element-by-element copy. `path` is left in a "valid but unspecified
state" (typically empty).

**What the world looked like before move semantics.**
In C++03, there was no way to transfer ownership of a heap buffer — you could only copy it.
Returning a `vector` from a function copied every element. The standard workaround was output
parameters:

```cpp
// C++03 idiom — pass the result container in by reference to avoid copying:
void generate_scenarios(const Forecaster& model, const TimeSeries& history,
                        std::vector<TimeSeries>& result);   // output parameter

// The calling code was explicit but ugly:
std::vector<TimeSeries> scenarios;
generate_scenarios(model, history, scenarios);
```

With move semantics, returning by value is free (either NRVO eliminates the move entirely, or
the move constructor transfers the buffer in O(1)):

```cpp
// C++11 — return by value; no copy, no output parameter needed:
std::vector<TimeSeries> scenarios = generate(model, history, 100, 5);
```

Similarly, `push_back` before move semantics copied the element into the vector. For a `TimeSeries`
holding 1000 observations, that was 1000 `Observation` copies per `push_back`. Move makes it O(1)
regardless of how many observations the series holds.

**What `std::move` actually does.**
`std::move(x)` is a cast to `T&&` — an rvalue reference. It does not move anything itself.
It tells the compiler to prefer a move constructor or move assignment operator over the copy
versions. The actual data transfer happens in `TimeSeries`'s move constructor (compiler-generated
since we follow Rule of Zero).

**The Rule of Zero.**
If your class manages no resources directly (it holds only types that already manage their own
resources, like `std::vector`), you should declare no destructor, copy constructor, copy assignment,
move constructor, or move assignment. The compiler generates correct, efficient versions of all
five. `TimeSeries` follows this rule — its only data member is `std::vector<Observation> data_`,
which already has correct move semantics.

**The hard interview question: "valid but unspecified state".**
After `std::move(v)`, `v` is in a state where: (1) its destructor will not UB, (2) it can be
assigned to, (3) but you cannot make any other assumption about its contents. For `std::vector`,
the moved-from object is guaranteed to be empty. This is a stronger guarantee than the standard
requires — the standard only promises "valid but unspecified." Always check the specific type's
documentation.

**NRVO and when you should NOT use `std::move` on a return.**

```cpp
std::vector<TimeSeries> generate(...) const
{
    std::vector<TimeSeries> scenarios;
    // ...
    return scenarios;    // CORRECT: NRVO applies, no copy or move
    return std::move(scenarios);  // WRONG: suppresses NRVO, forces a move instead
}
```

Named Return Value Optimisation (NRVO) constructs the return value directly in the caller's storage
— zero copies, zero moves. Adding `std::move` to a local variable in a return statement *disables*
NRVO (the expression is no longer an lvalue) and forces a move instead. The move is cheaper than
a copy but still worse than NRVO. The rule: never `std::move` a local variable in a return
statement.

---

## 7. `std::format`, `std::print`, `std::println` — Type-Safe Formatting

```cpp
std::string label = std::format("MovingAverage({})", window_);
std::println("Terminal mean:  {:.4f}", terminal_mean);
```

**Why `printf` is dangerous.**
`printf`'s format string is parsed at runtime. The compiler cannot verify that `%d` matches an
`int` argument. Mismatches are undefined behaviour (frequently crashes or wrong output). GCC/Clang
add `-Wformat` to catch obvious cases, but it only works for known format strings — not for
strings built at runtime.

**Why `ostringstream` is verbose.**
`std::ostringstream oss; oss << "MovingAverage(" << window_ << ")";` is correct but noisy.
It constructs a stream object, manages a string buffer, and requires `.str()` to extract the result.

**`std::format` is type-safe and fast.**
The format string is parsed at compile time (via `consteval` machinery in the standard library
implementation). If `{}` does not match the argument type, the code does not compile. The generated
code is also typically faster than `ostringstream` because it writes directly into a preallocated
buffer.

**The hard interview question: custom formatters.**
`std::format` is extensible. Specialise `std::formatter<T>` for your type and it becomes
formattable with `{}`. This replaces the `operator<<` pattern for formatting output. Custom
formatters can support format specifiers (e.g., `{:.4f}` for floating point precision).

---

## 8. `std::ranges::sort` with Projection

```cpp
std::ranges::sort(results, {}, &ModelEvaluationResult::rmse);
```

**What a projection is.**
A projection is a callable applied to each element before comparison. Here, `&ModelEvaluationResult::rmse`
is a pointer-to-member, which `ranges::sort` dereferences on each element before comparing. The
sort orders `ModelEvaluationResult` objects by their `.rmse` field, ascending, without a manual
lambda.

**How this differs from `std::sort` with a comparator.**
```cpp
// Old style
std::sort(results.begin(), results.end(),
    [](const auto& a, const auto& b){ return a.rmse < b.rmse; });

// Ranges style
std::ranges::sort(results, {}, &ModelEvaluationResult::rmse);
```

The ranges version: (1) takes the container directly, not iterator pairs; (2) separates the
comparator from the projection, so both can be supplied independently; (3) is constrained —
the template requires the range and comparator to satisfy `sortable`, producing better error
messages if you pass the wrong type.

**The hard interview question: why `{}` as the comparator?**
`{}` constructs `std::ranges::less{}` — the default comparator. The three-argument form of
`ranges::sort` is `(range, comp, proj)`. Passing `{}` for `comp` gives the default
less-than comparison applied to the projected values. If you wanted descending order:
`std::ranges::greater{}` as the comparator.

**Follow-up: are ranges lazy?**
`std::ranges::sort` is *not* lazy — like `std::sort`, it sorts in-place immediately. Laziness
in the ranges library comes from *views* (e.g., `std::views::filter`, `std::views::transform`),
which compute nothing until iterated. `ranges::sort` is a range *algorithm*, not a view.

---

## 9. `std::chrono` — Type-Safe Time

```cpp
// Observation.hpp
std::chrono::sys_days timestamp;

// ScenarioGenerator.cpp
const auto timestamp = last_timestamp + std::chrono::days{static_cast<int>(step)};
```

**The problem chrono solves.**
Pre-chrono code stores time as `int64_t` (Unix timestamp), `double` (seconds), or `std::string`.
These types carry no semantic meaning — nothing prevents adding a "date" to a "duration" or
comparing seconds to milliseconds without conversion. Bugs are silent.

**`sys_days` is a strong type.**
`std::chrono::sys_days` is `std::chrono::time_point<std::chrono::system_clock, std::chrono::days>`.
The type encodes both the clock (system_clock — UTC-based) and the resolution (days). You cannot
accidentally add a `sys_days` to a `seconds` without an explicit conversion — the type system
prevents it.

**`std::chrono::days` is a duration.**
`std::chrono::days{1}` is a duration of exactly 86400 seconds. Adding it to a `sys_days`
time point gives the next calendar day. The arithmetic is exact — no floating-point involved.

**The hard interview question: `sys_days` vs `local_days`.**
`sys_days` is a UTC time point. `local_days` is a time point in an unspecified local timezone.
They are distinct types — you cannot mix them without an explicit conversion through a timezone.
This prevents the common bug of treating a local date as UTC. C++20 adds a full calendar and
timezone library (`std::chrono::year_month_day`, `std::chrono::zoned_time`) on top of this.

---

## 10. `<random>` — Engine/Distribution Separation

```cpp
explicit ScenarioGenerator(unsigned seed = std::random_device{}());
// ...
mutable std::mt19937 rng_;
// ...
std::normal_distribution<double> noise{0.0, std::sqrt(result.variance)};
double shock = noise(rng_);
```

**What `rand()` and `srand()` got wrong.**

```cpp
// C-style — global, limited, and broken for serious use:
srand(42);                              // seeds a single global generator for the entire process
int r = rand();                         // [0, RAND_MAX]; RAND_MAX may be as low as 32767
double d = r / (double)RAND_MAX;        // uniform in [0,1] — but subtly biased
double in_range = r % 100;              // modulo bias: not uniformly distributed in [0,99]
```

Five problems with this:
1. **Global state** — `srand` affects every call to `rand()` in the process. Two threads calling
   `rand()` simultaneously is a data race (undefined behaviour on most implementations).
2. **Too few bits** — `RAND_MAX` may be 32767, giving only 15 bits of randomness per draw.
   A Monte Carlo simulation needs millions of independent samples; 15-bit granularity is
   statistically catastrophic.
3. **Modulo bias** — `rand() % N` is not uniformly distributed unless `RAND_MAX + 1` is exactly
   divisible by `N`. Correct rejection sampling is non-trivial to implement correctly.
4. **No independent streams** — there is one global generator. You cannot have two independent
   random processes running in the same program.
5. **Unspecified algorithm** — the standard does not require any particular distribution quality.
   Some implementations have visible patterns at small scales.

**The design: engines and distributions are separate.**
The C++ random library separates the *source of randomness* (the engine) from the *distribution*
of the output. The engine produces a stream of uniform pseudorandom bits. The distribution maps
those bits into a desired shape (normal, uniform real, Poisson, etc.). This separation lets you
reuse one engine across many distributions, or swap distributions without changing the seeding logic.

```cpp
// C++11 — local, seedable, statistically sound:
std::mt19937 rng{42};                              // local engine; 19937 bits of state
std::normal_distribution<double> dist{0.0, 1.0};  // distribution is a separate object
double sample = dist(rng);                         // draw one sample; no global state
std::uniform_int_distribution<int> die{1, 6};      // reuse same engine for a different distribution
int roll = die(rng);
```

**`std::mt19937`** — the Mersenne Twister with a period of 2^19937 - 1. Fast, well-studied, not
cryptographically secure (its state can be reconstructed from ~624 consecutive outputs). For
simulation this is fine; for security it is not.

**`std::random_device`** — reads from a hardware entropy source (e.g., `/dev/urandom` on Linux).
Used once to seed the engine. On some platforms (notably some MinGW builds on Windows)
`random_device` has historically been non-random — always check `random_device::entropy()` before
trusting it.

**The hard interview question: thread safety.**
`std::mt19937` is not thread-safe. Two threads sharing one generator without synchronization
produce a data race — undefined behaviour. Solutions: one engine per thread (thread-local storage),
or a mutex around each draw. Thread-local is usually faster:

```cpp
thread_local std::mt19937 tl_rng{std::random_device{}()};
```

**Why construct `normal_distribution` inside the loop rather than once as a member?**
`std::normal_distribution` is cheap to construct — it stores only the parameters. The state that
*is* expensive to maintain is in the engine. Constructing a new `normal_distribution` per call is
fine. We guard against `σ = 0` (when variance is zero) because the standard says constructing a
`normal_distribution` with non-positive sigma is undefined behaviour.

---

## 11. `static_cast` and Explicit Conversions

```cpp
const double n = static_cast<double>(series.size());
```

**Why named casts exist.**
C-style casts — `(double)n` — can silently perform any of: static_cast, reinterpret_cast,
const_cast, or a combination. The reader (and the compiler) cannot distinguish which. Named casts
document intent and allow the compiler to reject invalid conversions.

- `static_cast<T>`: compile-time checked conversion (numeric conversions, base-to-derived with
  known type, void* to T*)
- `reinterpret_cast<T>`: bitwise reinterpretation; almost never correct
- `const_cast<T>`: remove constness; legitimate only when you know the original object was
  non-const
- `dynamic_cast<T>`: runtime-checked downcast through a virtual hierarchy

**`-Wconversion` in this project** means the compiler will warn on implicit narrowing conversions
(e.g., `size_t` silently truncated to `int`). `static_cast` silences the warning deliberately —
the cast is the documentation that the programmer knows about and accepts the conversion.

**The hard interview question: `static_cast` for downcast safety.**
`static_cast<Derived*>(base_ptr)` is unchecked — it blindly reinterprets the pointer. If the
object is not actually a `Derived`, you get UB silently. `dynamic_cast<Derived*>(base_ptr)`
performs a runtime type check via the vtable and returns `nullptr` on failure (for pointers) or
throws `std::bad_cast` (for references). Use `dynamic_cast` when you are not certain of the
dynamic type; use `static_cast` only when you are.

---

## 12. `std::vector::reserve` and Container Growth

```cpp
scenarios.reserve(n_scenarios);
```

**How `std::vector` grows.**
`push_back` on a full vector allocates a new buffer (typically 1.5× or 2× the current capacity),
moves all elements, and releases the old buffer. Each reallocation is O(N). Over N pushes,
the amortised cost is O(1) per push, but there are O(log N) reallocations, each invalidating
all iterators and pointers into the vector.

**`reserve` avoids reallocations.**
`reserve(n)` allocates at least `n` slots upfront. All subsequent `push_back` calls (up to `n`)
do not reallocate. This matters here because moving a `TimeSeries` is cheap (move the internal
vector), but the bookkeeping of repeated reallocations still costs.

Without `reserve`, 1000 scenarios trigger roughly 10 reallocations — and each reallocation
invalidates every pointer and iterator into the vector:

```cpp
// Without reserve:
std::vector<TimeSeries> scenarios;
TimeSeries* first = nullptr;

for (std::size_t s = 0; s < 1000; ++s)
{
    scenarios.push_back(std::move(path));
    if (s == 0) first = &scenarios[0];
    // After any push_back that triggers reallocation, 'first' is a dangling pointer.
    // Dereferencing it is undefined behaviour — silent corruption.
}

// With reserve — the buffer never moves; pointers remain valid:
std::vector<TimeSeries> scenarios;
scenarios.reserve(1000);
TimeSeries* first = nullptr;

for (std::size_t s = 0; s < 1000; ++s)
{
    scenarios.push_back(std::move(path));
    if (s == 0) first = &scenarios[0];
    // 'first' remains valid for the entire loop.
}
```

**The hard interview question: `reserve` vs `resize`.**
`reserve(n)` changes `capacity()` but not `size()` — no elements are constructed.
`resize(n)` changes `size()`, value-initialising new elements if growing, destroying elements
if shrinking. Confusing the two is a common bug: using `resize` when you want `reserve` leaves you
with a vector of zeroed elements before you fill them, which can also mask reading from
uninitialised positions.

---

## 13. RAII — `std::ofstream`

```cpp
// CsvExporter.cpp — implicitly via the path overload
std::ofstream file{path};
exporter.write(results, file);
// file closes and flushes here when it goes out of scope
```

**RAII — Resource Acquisition Is Initialization.**
A resource (file handle, mutex lock, heap memory) is tied to an object's lifetime. The constructor
acquires the resource; the destructor releases it. The resource is released even if an exception
is thrown — the stack unwinds, destructors run, the resource is freed.

**What the C alternative looks like — and why it leaks.**

```cpp
// C-style file handling — manual open and close:
FILE* file = fopen(path.c_str(), "w");
if (!file) { /* handle open failure */ }

write_results(results, file);   // what if this throws?

fclose(file);   // never reached if write_results throws — file handle leaked forever
```

C does not have exceptions, so this pattern is safe there. In C++ it is a latent leak: any
exception between `fopen` and `fclose` skips the close. The C++ fix before RAII was
`try/catch` around everything:

```cpp
FILE* file = fopen(path.c_str(), "w");
try {
    write_results(results, file);
} catch (...) {
    fclose(file);
    throw;   // re-throw after cleanup
}
fclose(file);
```

This is error-prone — every resource needs its own catch block, and if you add a second resource
you need a nested structure. C++ does not have `finally`, so you must use catch-and-rethrow.

RAII replaces all of this:

```cpp
// C++ RAII — no explicit close, leak-free regardless of exceptions:
{
    std::ofstream file{path};       // constructor opens the file
    write_results(results, file);   // throws? doesn't matter
}   // file.~ofstream() runs here — file is closed and flushed unconditionally
```

`std::ofstream` closes and flushes the file in its destructor. This means: no explicit `close()`,
no `try/catch`, no leaked file handle.

**The hard interview question: exception safety guarantees.**
- *Basic guarantee*: if an exception is thrown, the program is in a valid state and no resources
  are leaked. RAII provides this automatically.
- *Strong guarantee*: if an exception is thrown, the program state is unchanged (as if the
  operation never happened). This requires copy-then-swap or similar techniques.
- *No-throw guarantee* (`noexcept`): the operation never throws. Required for move constructors
  in containers (a move that throws during `vector::resize` would leave the vector in an invalid
  state; `std::vector` will use a copy instead of a move if the move constructor is not `noexcept`).

This last point is subtle and important: **mark move constructors `noexcept`**. If you do not,
`std::vector` and `std::move_if_noexcept` may copy instead of move, negating your performance intent.

---

## 14. Abstract Base Classes and the Non-Virtual Interface Pattern

`Forecaster` currently uses pure virtual public functions. A common refinement is the
**Non-Virtual Interface (NVI) pattern**:

```cpp
class Forecaster {
public:
    ForecastResult forecast(const TimeSeriesView& v) const { return do_forecast(v); }
private:
    virtual ForecastResult do_forecast(const TimeSeriesView& v) const = 0;
};
```

The public function is non-virtual and can contain pre/post-condition checks, logging, or caching.
The private virtual function is the customisation point. This is not used in this project but is
worth knowing — it is a common senior interview topic because it shows understanding of interface
design beyond "just make it virtual."

---

# Part 2 — Features Not Yet Used

These would arise naturally as the project grows. Each section describes the feature, when it would
appear here, and what interviewers probe.

---

## A. `std::expected<T, E>` (C++23) — when ConfigLoader grows up

**When it would arise.** `ConfigLoader::load()` currently either succeeds or presumably throws.
As the config format grows more complex (invalid fields, missing required keys, bad file path),
exceptions become heavyweight for *expected* failures. `std::expected` is the right tool:

```cpp
std::expected<Config, std::string> ConfigLoader::load(const std::filesystem::path& path);
```

On success: `.value()` returns the Config. On failure: `.error()` returns a description string.
No exception unwinding, no performance overhead for the common success path.

**What it is.**
`std::expected<T, E>` is like `std::optional<T>` but the "empty" state carries information.
It is a sum type — at any moment it holds either a `T` or an `E`, never both, never neither.

**Monadic operations (C++23).**

```cpp
auto result = ConfigLoader::load("config.json")
    .transform([](Config c){ c.num_scenarios *= 2; return c; })
    .or_else([](std::string err){ return default_config(); });
```

`transform` applies a function to the value if present (like `Optional.map` in other languages).
`and_then` chains operations that themselves return `expected` (flatMap). `or_else` handles the
error case. This composes cleanly without nested if-checks.

**The hard interview question: vs exceptions.**
Exceptions are for *unexpected* failures — things you do not anticipate and cannot recover from
locally. `std::expected` is for *expected* failures that are part of the function's normal contract.
"File not found" is expected and recoverable; it should be `expected`. Stack overflow is unexpected
and unrecoverable; it should throw (or terminate). Using exceptions for expected failures causes
flow-control-via-exceptions, which is slow and obscures logic. Using `expected` for unexpected
failures forces callers to handle error cases they cannot do anything about.

---

## B. Concepts (C++20) — when `TimeSeriesLike` becomes necessary

**When it would arise.** Right now the codebase has two time-series-shaped types: `TimeSeries`
(owning) and `TimeSeriesView` (non-owning). If `ScenarioFan::StepStatistics` or a scenario slice
becomes a third such type, you have a genuine constraint to express: "this function works on
anything that supports `.size()`, `.value_at(i)`, and `.observations()`."

```cpp
template <typename T>
concept TimeSeriesLike = requires(const T& t, std::size_t i) {
    { t.size() }           -> std::convertible_to<std::size_t>;
    { t.value_at(i) }      -> std::convertible_to<double>;
    { t.observations() }   -> std::ranges::contiguous_range;
};

template <TimeSeriesLike T>
ForecastResult MovingAverageForecaster::forecast(const T& series) const;
```

**What concepts replace.**
Before C++20, constraining templates required SFINAE — `std::enable_if`, `std::void_t`, or arcane
`decltype` expressions. These produce error messages that span hundreds of lines. A violated
concept produces a short, readable diagnostic: "T does not satisfy TimeSeriesLike because value_at
does not return something convertible to double."

**Concept subsumption.**
If `StochasticForecaster` is a refinement of `TimeSeriesLike`:

```cpp
template <typename T>
concept CalibrableForecaster = TimeSeriesLike<T> && requires(T& t, const TimeSeries& s) {
    { t.calibrate(s) };
};
```

The compiler understands that `CalibrableForecaster` is *more constrained* than `TimeSeriesLike`.
If two function overloads differ only in their concept constraints, the more constrained one is
preferred — this is concept subsumption. It enables clean, zero-overhead overload resolution
without SFINAE tricks.

**The hard interview question: `requires requires`.**
A concept can contain a `requires`-expression (testing whether certain expressions are valid):

```cpp
template <typename T>
concept Forecaster = requires(const T& t, const TimeSeriesView& v) {
    { t.forecast(v) } -> std::same_as<ForecastResult>;
    { t.minimum_observations() } -> std::convertible_to<std::size_t>;
};
```

The outer `requires` says "T satisfies this concept if..." The inner `requires(args){ exprs }` is
a requires-expression — a compile-time boolean that tests whether `exprs` are valid for arguments
of the given types. The doubled keyword is intentional and confusing; it is the most common source
of concept syntax errors.

---

## C. `std::generator<T>` and Coroutines (C++23) — lazy scenario generation

**When it would arise.** `ScenarioGenerator::generate()` currently materialises all `n_scenarios`
paths into a `vector<TimeSeries>` before returning. For `n_scenarios = 10000` with long horizons,
this is a large allocation. The `SimpleOptimizer` only needs one scenario at a time — materialising
all of them to iterate once is wasteful.

```cpp
std::generator<TimeSeries> ScenarioGenerator::stream(
    const Forecaster& model,
    const TimeSeries& history,
    std::size_t       n_scenarios,
    std::size_t       horizon_steps) const
{
    for (std::size_t s = 0; s < n_scenarios; ++s)
    {
        TimeSeries path = history;
        // ... build path ...
        co_yield std::move(path);   // suspends here, resumes on next pull
    }
}
```

**What coroutines are.**
A coroutine is a function that can suspend and resume. The three coroutine keywords are:
- `co_yield value` — suspend and produce a value to the caller
- `co_await awaitable` — suspend until an asynchronous operation completes
- `co_return value` — final return from a coroutine

`std::generator` is a synchronous coroutine — it does not involve asynchrony. It is a lazy
sequence: the next value is computed only when the caller iterates to it. The coroutine frame
(its local variables) lives on the heap between suspensions.

**The hard interview question: symmetric transfer and stack overflow.**
Without symmetric transfer, `co_yield` returns through the caller's stack frame, which then calls
the next iteration, which calls `co_yield` again — potentially blowing the stack for deeply nested
coroutines. C++20 coroutines use symmetric transfer: `co_yield` jumps directly to the caller's
continuation without growing the stack. This is implemented via compiler-generated trampolines,
not recursive calls.

---

## D. `std::function<>` and Type Erasure — for `SimpleOptimizer`

**When it would arise.**

```cpp
using CostFn = std::function<double(double decision, const TimeSeries& scenario)>;
double SimpleOptimizer::optimize(CostFn cost, ...) const;
```

The optimizer should work with any callable: a lambda, a function pointer, a functor. `std::function`
erases the type of the callable behind a uniform interface.

**What type erasure costs.**
`std::function` stores the callable on the heap if it is larger than its internal small buffer
(typically ~24 bytes). Calling it involves an indirect function call through a stored pointer.
For a tight inner loop (calling `cost` millions of times across scenarios), this overhead matters.

**Alternatives.**
- **Template parameter**: `template<typename F> double optimize(F cost, ...)` — zero overhead,
  the call can be inlined, but the entire optimizer body is compiled once per callable type.
- **`std::move_only_function`** (C++23): like `std::function` but the callable can be move-only.
  `std::function` requires copyable callables; a lambda that captures a `unique_ptr` is not
  copyable and cannot be stored in `std::function`.

**The hard interview question: how does `std::function` implement type erasure?**
`std::function` holds a pointer to a virtual dispatch table (or equivalent) that it builds at
construction time from the callable's type. The stored table has slots for: `call`, `copy`,
`move`, `destroy`. This is the same mechanism as C++ virtual dispatch, but implemented
manually (without using `virtual`) to avoid the need for a base class. This pattern — storing
type-erased operations alongside the object — is called the *type erasure idiom* and is a
general technique independent of `std::function`.

---

## E. `std::optional<T>` — representing absent values

**When it would arise.**
A forecaster that requires more observations than the series contains cannot produce a result.
Currently it throws. An alternative is returning `std::optional<ForecastResult>`:

```cpp
std::optional<ForecastResult> MovingAverageForecaster::forecast(
    const TimeSeriesView& view) const;
```

Callers check `if (result)` before using the value, making the "no result" path explicit in the
type system rather than implicitly thrown.

**`std::optional` vs a sentinel value.**
Before `optional`, you might return `{NaN, NaN, 0}` to signal failure. The caller can forget to
check. `optional` forces the caller to unwrap — accessing `.value()` on an empty optional throws
`std::bad_optional_access`. Alternatively, `.value_or(default)` provides a default without
throwing.

**The hard interview question: `optional` of a reference.**
`std::optional<T&>` is ill-formed — the standard prohibits optional references. The rationale
is that optional reference semantics are ambiguous: does `opt = other_ref` rebind the optional
or assign through the reference? Use `std::optional<std::reference_wrapper<T>>` or a raw pointer
instead (a nullable pointer *is* an optional reference).

---

## F. `std::variant<>` and `std::visit` — heterogeneous results

**When it would arise.** If the `Backtester` should return either a `ModelEvaluationResult` or
a `std::string` error description (without exceptions), and you want the type system to enforce
handling both cases:

```cpp
using EvalResult = std::variant<ModelEvaluationResult, std::string>;
```

**`std::visit` with overloaded lambdas.**

```cpp
std::visit(overloaded{
    [](const ModelEvaluationResult& r) { /* use result */ },
    [](const std::string& err)          { /* handle error */ },
}, eval_result);
```

`overloaded` is a common utility (not in the standard library, but trivially written):

```cpp
template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
```

This creates a callable that inherits `operator()` from each lambda, and the compiler selects the
right one based on which type the variant currently holds. If you forget to handle a case, it
does not compile.

**The hard interview question: `variant` vs inheritance.**
`variant` is a value type — it can be copied, stored in a `vector`, compared. A virtual hierarchy
is a reference type — you must manage it through pointers, contend with lifetime, and pay for
heap allocation. `variant` is appropriate when the set of types is *closed* (known at compile time)
and you want value semantics. Inheritance is appropriate when the set of types is *open* (new
types can be added without modifying existing code) — the Open/Closed Principle.

---

## G. Smart Pointers — when polymorphic ownership is needed

**Currently.** The project stores `Forecaster` pointers as non-owning raw pointers in
`std::array<const Forecaster*, 2>`. The caller (main.cpp) owns the objects on the stack.
This is correct — raw pointers for non-owning handles are idiomatic C++.

**When `std::unique_ptr<Forecaster>` would arise.** If forecasters are heap-allocated (e.g.,
read from config and constructed dynamically), the owner needs a smart pointer:

```cpp
std::vector<std::unique_ptr<Forecaster>> models;
models.push_back(std::make_unique<MovingAverageForecaster>(3));
```

`unique_ptr` enforces single ownership. It is zero-overhead compared to a raw pointer — the
deleter is a template parameter, resolved at compile time, and the destructor call is inlined.

**The hard interview question: `make_unique` vs `new`.**
Before C++17, `f(std::unique_ptr<T>(new T{}), g())` was a memory leak: the compiler was allowed
to evaluate `new T`, then `g()`, then construct the `unique_ptr`. If `g()` threw, the `unique_ptr`
was never constructed and the raw pointer leaked. `std::make_unique<T>()` is a single expression —
the allocation and the `unique_ptr` construction are atomic with respect to exception ordering.
In C++17, evaluation order is stricter, but `make_unique` remains the idiomatic choice.

**`shared_ptr` vs `unique_ptr`.**
`shared_ptr` uses reference counting (atomic increment/decrement, which is not cheap). Use it
only when ownership is genuinely shared — multiple objects need to keep the same resource alive.
In almost all cases in this codebase, ownership is clear and `unique_ptr` is correct.

---

## H. `[[nodiscard]]` — preventing silently ignored results

**When it would arise.** `ForecastResult`, `ModelEvaluationResult`, and `ResidualStatistics` are
all types that are meaningless to compute and then discard. Marking them (or the functions that
return them) `[[nodiscard]]` makes the compiler warn when the return value is thrown away:

```cpp
[[nodiscard]] ForecastResult forecast(const TimeSeriesView& series) const = 0;
```

```cpp
model.forecast(view);  // warning: ignoring return value of 'forecast'
```

**When it is most useful.** Error-returning functions: `[[nodiscard]]` on a function that returns
an error code ensures callers cannot silently ignore failures. This is why `std::expected` types
are often `[[nodiscard]]` — ignoring an `expected<T, E>` return means you did not check for
errors.

**The hard interview question: `[[nodiscard]]` on a class vs a function.**
If applied to the class itself (`[[nodiscard]] struct ForecastResult {...}`), any function
returning that type produces the warning, without having to mark each function individually.
If applied to the function, only that function's return is checked. The class-level attribute
is more powerful but more invasive.

---

## I. `constexpr` and `consteval` — compile-time computation

**`constexpr` functions** can run at compile time if all their arguments are compile-time
constants, or at runtime otherwise. C++20 significantly relaxes what is permitted in constexpr
context: heap allocation, virtual function calls, `try`/`catch`, and more.

**`consteval` functions** (C++20) *must* run at compile time. They cannot be called with
runtime arguments. `std::format`'s format-string parsing is `consteval` — this is why format
string errors are compile-time errors, not runtime panics.

**When it would arise in this project.**
Window size and horizon could be compile-time constants for a template specialisation:

```cpp
template <std::size_t Window, std::size_t Horizon = 1>
class MovingAverageForecaster { ... };
```

With fixed window, the compiler can unroll the summation loop, vectorise it, and completely
eliminate the bounds check. This is the "compile-time polymorphism" alternative to virtual
dispatch.

**The hard interview question: constant evaluation and UB.**
`constexpr` evaluation is stricter than runtime — undefined behaviour (signed overflow, out-of-
bounds access, null pointer dereference) is a *compile error* in a `constexpr` context rather than
silent undefined runtime behaviour. This makes `constexpr` functions effectively UB-free by
construction, which is a strong correctness guarantee.

---

## J. `std::mdspan` (C++23) — the scenario matrix as a 2D view

**When it would arise.**
The scenario fan is naturally a matrix: `n_scenarios` rows × `(history_length + horizon_steps)`
columns. Currently it is `vector<TimeSeries>`, which is a vector of vectors — cache-hostile for
column-wise access (reading the value at step `t` across all scenarios requires jumping between
`n_scenarios` non-contiguous memory locations).

A flat `vector<double>` with an `mdspan` view would lay all data contiguously:

```cpp
std::vector<double> data(n_scenarios * total_length);
std::mdspan<double, std::dextents<std::size_t, 2>> matrix{data.data(), n_scenarios, total_length};

double value_at_s3_t7 = matrix[3, 7];  // row 3, column 7
```

**Layout policies.**
`mdspan` supports pluggable layout policies:
- `layout_right` (default, C row-major): row elements are contiguous — fast to iterate across
  a scenario (all steps of one path)
- `layout_left` (Fortran column-major): column elements are contiguous — fast to iterate across
  all scenarios at a given step (computing percentiles)

The choice of layout matches the access pattern. For `ScenarioFan` percentile computation,
column-major would be optimal.

---

## K. `std::pmr` — Performance-Critical Allocation

**When it would arise.** Generating 10,000 scenarios, each copying a `TimeSeries` of 1,000
observations, triggers 10,000 heap allocations for the internal `vector<Observation>` buffers.
A monotonic buffer resource allocates all memory from a single upfront block:

```cpp
std::array<std::byte, 100 * 1024 * 1024> buffer;   // 100 MB stack/global arena
std::pmr::monotonic_buffer_resource pool{buffer.data(), buffer.size()};
std::pmr::vector<std::pmr::vector<double>> scenarios{&pool};
```

All allocations come from `pool` with O(1) cost (pointer bump). Deallocation is also O(1)
(the entire pool is released at once). This is standard in high-frequency trading and simulation
code where allocation latency matters.

**The hard interview question: the allocator model.**
C++'s allocator model is a template parameter on containers: `std::vector<T, Allocator>`.
Before C++17 `pmr`, writing allocator-aware code required threading the allocator type through
every container and every container-using class — an enormous ergonomic burden. `std::pmr`
containers (`std::pmr::vector`, `std::pmr::string`, etc.) fix the allocator type to
`std::polymorphic_allocator`, which dispatches to a runtime `memory_resource`. You swap
the allocator strategy by passing a different `memory_resource*` at construction, not by
changing any template parameters.

---

## Closing: The Throughline

Every feature in Part 1 and Part 2 serves one or more of these principles:

| Principle | Features |
|---|---|
| **Express ownership clearly** | `unique_ptr`, `shared_ptr`, `span` (non-owning), `string_view` (non-owning), `move` semantics |
| **Make illegal states unrepresentable** | `chrono` strong types, `optional`, `expected`, `variant`, `nodiscard` |
| **Zero-cost abstraction** | `span`, `string_view`, `ranges` projections, `constexpr`, `mdspan`, `unique_ptr` |
| **Generic, reusable code without sacrificing type safety** | concepts, `template`, `function`, `visit` |
| **Fail at compile time, not runtime** | `override`, designated initializers (order checked), `consteval` format strings, concepts |
| **Resource safety without manual management** | RAII (`ofstream`, `unique_ptr`), `noexcept` on moves, `make_unique` |

In an interview, the strongest answers tie a specific language feature back to one of these
principles and explain what the alternative (pre-feature) code looked like and why it was worse.
The features themselves are easy to look up; understanding *why the language added them* is what
separates candidates.
