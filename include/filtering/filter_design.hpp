#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

namespace filtering {

/**
 * @brief Window function and standard FIR coefficient generation utilities.
 */
class FilterDesign {
public:
    static constexpr double kPi = 3.141592653589793238462643383279502884;

    /**
     * @brief Design a normalized Moving Average (boxcar) FIR filter.
     * @param taps Number of filter taps (must be >= 1).
     */
    template <std::floating_point T = float>
    [[nodiscard]] static std::vector<T> create_moving_average(std::size_t taps) {
        if (taps == 0) {
            throw std::invalid_argument("Taps must be greater than zero.");
        }
        return std::vector<T>(taps, static_cast<T>(1.0 / static_cast<double>(taps)));
    }

    /**
     * @brief Design a Windowed-Sinc Low-Pass FIR Filter using a Hamming window.
     *
     * @param taps Number of filter taps (odd number recommended for symmetric linear phase).
     * @param cutoff_ratio Normalized cutoff frequency fc / Fs (0 < cutoff_ratio < 0.5).
     * @return Normalized FIR coefficients with unity DC gain.
     */
    template <std::floating_point T = float>
    [[nodiscard]] static std::vector<T> create_lowpass_fir(std::size_t taps, T cutoff_ratio) {
        if (taps == 0) {
            throw std::invalid_argument("Taps must be greater than zero.");
        }
        if (cutoff_ratio <= static_cast<T>(0) || cutoff_ratio >= static_cast<T>(0.5)) {
            throw std::invalid_argument("Cutoff ratio must be in the range (0.0, 0.5).");
        }

        std::vector<T> coeffs(taps);
        const double fc = static_cast<double>(cutoff_ratio);
        const double center = static_cast<double>(taps - 1) / 2.0;

        double sum = 0.0;
        for (std::size_t i = 0; i < taps; ++i) {
            const double n = static_cast<double>(i) - center;

            // Ideal Sinc function
            double sinc_val = 0.0;
            if (std::abs(n) < 1e-9) {
                sinc_val = 2.0 * fc;
            } else {
                sinc_val = std::sin(2.0 * kPi * fc * n) / (kPi * n);
            }

            // Hamming window: w[i] = 0.54 - 0.46 * cos(2*pi*i / (M-1))
            double window = 1.0;
            if (taps > 1) {
                window = 0.54 - 0.46 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(taps - 1));
            }

            const double val = sinc_val * window;
            coeffs[i] = static_cast<T>(val);
            sum += val;
        }

        // Normalize for unity gain at DC (0 Hz)
        if (std::abs(sum) > 1e-12) {
            for (auto& c : coeffs) {
                c = static_cast<T>(static_cast<double>(c) / sum);
            }
        }

        return coeffs;
    }

    /**
     * @brief Design a Windowed-Sinc High-Pass FIR Filter.
     * @param taps Number of filter taps (must be odd).
     * @param cutoff_ratio Normalized cutoff frequency (0.0 to 0.5).
     */
    template <std::floating_point T = float>
    [[nodiscard]] static std::vector<T> create_highpass_fir(std::size_t taps, T cutoff_ratio) {
        if (taps % 2 == 0) {
            throw std::invalid_argument("High-pass FIR filters require an odd number of taps.");
        }
        auto lp = create_lowpass_fir<T>(taps, cutoff_ratio);
        const std::size_t center = (taps - 1) / 2;

        // Spectral inversion
        for (std::size_t i = 0; i < taps; ++i) {
            lp[i] = -lp[i];
        }
        lp[center] += static_cast<T>(1.0);
        return lp;
    }
};

} // namespace filtering
