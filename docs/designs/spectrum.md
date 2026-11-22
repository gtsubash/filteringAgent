# Design Specification: Frequency Spectrum Analysis & Visualization

- **Status:** Approved
- **Author:** Architect Subagent
- **Target Standard:** C++20
- **Scope:** Frequency magnitude response computation and ASCII comparison visualization for discrete signals

---

## 1. Overview & Goals

Provides tools to analyze the frequency content of discrete signals before and after filtering.
It enables engineers and automated tests to verify:
1. Signal frequencies in the filter passband are preserved with $\approx 0\text{ dB}$ attenuation.
2. Signal frequencies in the filter stopband are attenuated by $\ge 20\text{ dB}$.
3. Side-by-side terminal visualization using ASCII bar meters.

---

## 2. API & Data Contracts

```cpp
namespace filtering {

struct SpectrumPoint {
    float frequency_hz{0.0f};
    float magnitude_linear{0.0f};
    float magnitude_db{0.0f}; // 20 * log10(magnitude_linear)
};

struct SpectrumResult {
    float sample_rate{0.0f};
    std::vector<SpectrumPoint> points;
    float peak_frequency_hz{0.0f};
    float peak_magnitude_db{-100.0f};
};

class SpectrumAnalyzer {
public:
    // Computes discrete frequency spectrum up to Nyquist (Fs / 2)
    template <std::floating_point T = float>
    [[nodiscard]] static SpectrumResult compute_spectrum(
        std::span<const T> signal,
        float sample_rate,
        std::size_t num_bins = 128);

    // Renders formatted side-by-side ASCII comparison
    static void print_comparison(
        const SpectrumResult& input_spec,
        const SpectrumResult& output_spec,
        std::size_t display_bins = 20,
        std::ostream& out = std::cout);
};

} // namespace filtering
```

---

## 3. Numeric Safety & Invariants
- Minimum floor of $10^{-6}$ ($-120\text{ dB}$) before computing `std::log10` to avoid $\log(0) = -\infty$.
- Pre-allocation of spectrum points vector to avoid dynamic reallocations.
- Bounds checking on `sample_rate > 0` and non-empty input spans.
