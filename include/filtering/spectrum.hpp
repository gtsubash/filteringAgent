#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace filtering {

struct SpectrumPoint {
    float frequency_hz{0.0f};
    float magnitude_linear{0.0f};
    float magnitude_db{-120.0f};
};

struct SpectrumResult {
    float sample_rate{0.0f};
    std::vector<SpectrumPoint> points;
    float peak_frequency_hz{0.0f};
    float peak_magnitude_db{-120.0f};
};

/**
 * @brief Discrete spectrum analyzer and terminal visualization tools.
 */
class SpectrumAnalyzer {
public:
    static constexpr double kPi = 3.141592653589793238462643383279502884;

    /**
     * @brief Compute magnitude spectrum from 0 Hz up to Nyquist (sample_rate / 2).
     * @param signal Input discrete-time samples.
     * @param sample_rate Sampling rate in Hz.
     * @param num_bins Number of discrete frequency bins.
     */
    template <std::floating_point T = float>
    [[nodiscard]] static SpectrumResult compute_spectrum(
        std::span<const T> signal,
        float sample_rate,
        std::size_t num_bins = 128)
    {
        SpectrumResult result;
        result.sample_rate = sample_rate;

        if (signal.empty() || sample_rate <= 0.0f || num_bins < 2) {
            return result;
        }

        result.points.resize(num_bins);
        const float nyquist = sample_rate * 0.5f;
        const double n_samples = static_cast<double>(signal.size());
        const double norm_factor = (n_samples > 0.0) ? (2.0 / n_samples) : 1.0;

        float max_mag_db = -120.0f;
        float max_freq = 0.0f;

        for (std::size_t b = 0; b < num_bins; ++b) {
            const float freq = (static_cast<float>(b) / static_cast<float>(num_bins - 1)) * nyquist;
            const double omega = 2.0 * kPi * static_cast<double>(freq) / static_cast<double>(sample_rate);

            double real_sum = 0.0;
            double imag_sum = 0.0;

            for (std::size_t n = 0; n < signal.size(); ++n) {
                const double angle = omega * static_cast<double>(n);
                const double val = static_cast<double>(signal[n]);
                real_sum += val * std::cos(angle);
                imag_sum -= val * std::sin(angle);
            }

            double mag = std::sqrt(real_sum * real_sum + imag_sum * imag_sum) * norm_factor;
            // DC bin normalization
            if (b == 0) {
                mag *= 0.5;
            }

            const float mag_linear = static_cast<float>(mag);
            // Dynamic range floor at -120 dB to avoid log(0)
            const float clamped_mag = std::max(mag_linear, 1e-6f);
            const float mag_db = 20.0f * std::log10(clamped_mag);

            result.points[b] = {freq, mag_linear, mag_db};

            if (mag_db > max_mag_db) {
                max_mag_db = mag_db;
                max_freq = freq;
            }
        }

        result.peak_frequency_hz = max_freq;
        result.peak_magnitude_db = max_mag_db;
        return result;
    }

    /**
     * @brief Print side-by-side ASCII frequency comparison bar meters.
     */
    static void print_comparison(
        const SpectrumResult& input_spec,
        const SpectrumResult& output_spec,
        std::size_t display_bins = 20,
        std::ostream& out = std::cout);
};

} // namespace filtering
