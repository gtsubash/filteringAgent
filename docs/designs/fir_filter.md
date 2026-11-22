# Design Specification: 1D FIR Filtering Engine

- **Status:** Approved
- **Author:** Architect Subagent
- **Target Standard:** C++20
- **Scope:** 1D Finite Impulse Response (FIR) filtering library and validation application

---

## 1. Overview & Goals

The 1D Finite Impulse Response (FIR) filtering engine provides high-performance, deterministic digital signal filtering for discrete-time sequences. The design supports both **stateful streaming/block-based processing** (continuous delay line) and **one-shot batch processing**, adhering strictly to modern C++20 idioms, RAII, and zero-raw-pointer semantics.

---

## 2. File & Module Layout

```text
filteringAgent/
├── CMakeLists.txt                  # Top-level CMake build definition
├── include/
│   └── filtering/
│       ├── fir_filter.hpp          # Primary template & interface definitions
│       └── filter_design.hpp       # Window & coefficient generation utilities
├── src/
│   ├── fir_filter.cpp              # Explicit template instantiations & helpers
│   └── filter_design.cpp           # Window functions (Hamming, Hann, Sinc)
├── apps/
│   └── fir_demo.cpp                # Interactive/CLI demonstration app
├── tests/
│   └── test_fir_filter.cpp         # Unit test suite (impulse, step, sin waves)
└── docs/
    └── designs/
        └── fir_filter.md           # This specification document
```

### Targets:
1. `filtering_core` (STATIC library): Core FIR filtering and window generation.
2. `fir_demo` (EXECUTABLE): Demonstrates filtering on noisy multi-frequency synthetic signals.
3. `fir_tests` (EXECUTABLE): Deterministic test suite verifying impulse response, frequency attenuation, and streaming state continuity.

---

## 3. Data & Signal Contracts

### 3.1 Convolution Equation
For an $M$-tap filter with coefficients $h[0], h[1], \dots, h[M-1]$ and input signal $x[n]$:

$$y[n] = \sum_{k=0}^{M-1} h[k] \cdot x[n - k]$$

### 3.2 Types & Signatures
- **Numeric Types:** Supports standard floating-point types (`float` for high-throughput single precision, `double` for high-fidelity numeric precision).
- **Views:** Uses `std::span<const T>` for input signal views and `std::span<T>` for output signal views to avoid heap copies.
- **Coefficients:** Stored in `std::vector<T>` inside the filter class.

### 3.3 Core Class Architecture

```cpp
namespace filtering {

template <std::floating_point T = float>
class FirFilter {
public:
    // Lifecycle
    explicit FirFilter(std::span<const T> coefficients);
    explicit FirFilter(std::vector<T> coefficients);

    // Coefficients & taps
    [[nodiscard]] std::size_t num_taps() const noexcept;
    [[nodiscard]] std::span<const T> coefficients() const noexcept;
    void set_coefficients(std::span<const T> coefficients);

    // State management
    void reset() noexcept;

    // Single-sample streaming
    [[nodiscard]] T process_sample(T sample) noexcept;

    // Block streaming (updates delay-line state)
    void process(std::span<const T> input, std::span<T> output);

    // One-shot stateless batch filter
    static std::vector<T> filter_batch(std::span<const T> input, std::span<const T> coefficients);

private:
    std::vector<T> coeffs_;      // Size M
    std::vector<T> state_;       // Delay line of size M - 1
    std::size_t state_index_{0}; // Ring buffer head index
};

} // namespace filtering
```

### 3.4 Coefficient Design Helpers (`filter_design.hpp`)
- `create_moving_average<T>(std::size_t taps)`: Normalized moving average coefficients ($1/M$).
- `create_lowpass_fir<T>(std::size_t taps, T cutoff_ratio)`: Windowed sinc FIR filter with Hamming window.

---

## 4. C++20 Ownership & Error Handling

1. **RAII & Resource Safety:**
   - Zero raw owning pointers; all state is managed by standard containers (`std::vector<T>`).
2. **Bounds & Validation:**
   - Filter initialization validates that `coefficients` is non-empty (`coefficients.empty()` throws `std::invalid_argument`).
   - `process(input, output)` verifies that `output.size() >= input.size()`; throws `std::invalid_argument` if insufficient space is provided.
   - If `input.empty()`, processing is a safe no-op.
3. **Const-correctness & `noexcept`:**
   - Pure queries (`num_taps`, `coefficients`) are `const noexcept`.
   - Per-sample processing `process_sample` is `noexcept` and branchless/tightly bound for SIMD auto-vectorization.

---

## 5. CPU-First & GPU Isolation

1. **CPU Execution:**
   - Fully runnable on standard CPU using MSVC / Clang / GCC with `/std:c++20` or `-std=c++20`.
   - Auto-vectorization friendly ring-buffer or folded state representation for cache locality.
2. **GPU Decoupling:**
   - No GPU/CUDA headers or symbols leak into `include/filtering/fir_filter.hpp`.
   - Any future GPU acceleration (e.g. cuSignal / CUDA kernel) will implement a decoupled backend inheriting or matching this signature:
     ```cpp
     void process_gpu(GpuBufferView<const T> input, GpuBufferView<T> output);
     ```
   - Build system defaults `ENABLE_GPU=OFF`.

---

## 6. Verification & Acceptance Criteria

1. **Impulse Response:**
   - Input $\delta[n] = [1, 0, 0, \dots]$.
   - Output must match coefficients $h[0 \dots M-1]$ within numeric tolerance ($10^{-6}$ for float).
2. **Step Response & DC Gain:**
   - Input $u[n] = [1, 1, 1, \dots]$.
   - Steady-state output must match $\sum_{k=0}^{M-1} h[k]$.
3. **Streaming State Continuity:**
   - Filtering a signal in small blocks (e.g., 8 samples at a time) must yield identical output to filtering the entire signal at once.
4. **Noise Attenuation (Demo Application):**
   - High-frequency noise superimposed on a low-frequency carrier must be attenuated by $\ge 20\text{ dB}$ using a low-pass FIR filter.
