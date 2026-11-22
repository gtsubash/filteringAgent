#pragma once

#include <concepts>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

namespace filtering {

/**
 * @brief 1D Finite Impulse Response (FIR) Filter for discrete-time signals.
 *
 * Supports both continuous streaming (maintaining delay-line history across calls)
 * and stateless batch filtering.
 *
 * @tparam T Floating-point precision (float or double).
 */
template <std::floating_point T = float>
class FirFilter {
public:
    /**
     * @brief Construct an FIR filter with given coefficients.
     * @param coefficients Impulse response taps h[0], h[1], ..., h[M-1].
     * @throws std::invalid_argument if coefficients span is empty.
     */
    explicit FirFilter(std::span<const T> coefficients) {
        set_coefficients(coefficients);
    }

    /**
     * @brief Construct an FIR filter with given coefficients vector.
     */
    explicit FirFilter(std::vector<T> coefficients)
        : FirFilter(std::span<const T>(coefficients.data(), coefficients.size())) {}

    /**
     * @brief Number of filter taps (order + 1).
     */
    [[nodiscard]] std::size_t num_taps() const noexcept {
        return coeffs_.size();
    }

    /**
     * @brief Read-only view of filter coefficients.
     */
    [[nodiscard]] std::span<const T> coefficients() const noexcept {
        return coeffs_;
    }

    /**
     * @brief Update filter coefficients and reset state.
     * @throws std::invalid_argument if coefficients span is empty.
     */
    void set_coefficients(std::span<const T> coefficients) {
        if (coefficients.empty()) {
            throw std::invalid_argument("Filter coefficients must not be empty.");
        }
        coeffs_.assign(coefficients.begin(), coefficients.end());
        reset();
    }

    /**
     * @brief Clear internal delay-line history to zero.
     */
    void reset() noexcept {
        const std::size_t state_size = (coeffs_.size() > 1) ? (coeffs_.size() - 1) : 0;
        state_.assign(state_size, static_cast<T>(0));
        state_index_ = 0;
    }

    /**
     * @brief Process a single input sample and update internal delay state.
     * @param sample Current input sample x[n].
     * @return Filtered output sample y[n].
     */
    [[nodiscard]] T process_sample(T sample) noexcept {
        const std::size_t num_taps_count = coeffs_.size();
        if (num_taps_count == 0) [[unlikely]] {
            return sample;
        }

        // Direct-Form FIR: y[n] = h[0]*x[n] + sum_{k=1}^{M-1} h[k]*state[k-1]
        T acc = coeffs_[0] * sample;

        const std::size_t state_size = state_.size();
        if (state_size > 0) {
            for (std::size_t k = 1; k < num_taps_count; ++k) {
                // Circular ring buffer indexing
                const std::size_t idx = (state_index_ - (k - 1) + (state_size - 1)) % state_size;
                acc += coeffs_[k] * state_[idx];
            }
            // Insert newest sample into delay line ring buffer
            state_[state_index_] = sample;
            state_index_ = (state_index_ + 1) % state_size;
        }

        return acc;
    }

    /**
     * @brief Stream an input buffer through the filter, updating delay-line history.
     * @param input Input samples span.
     * @param output Output buffer span (size must be >= input.size()).
     * @throws std::invalid_argument if output size is smaller than input size.
     */
    void process(std::span<const T> input, std::span<T> output) {
        if (output.size() < input.size()) {
            throw std::invalid_argument("Output buffer size must be greater than or equal to input size.");
        }
        for (std::size_t i = 0; i < input.size(); ++i) {
            output[i] = process_sample(input[i]);
        }
    }

    /**
     * @brief Convenience stateless one-shot batch filter.
     * @param input Input signal sequence.
     * @param coefficients Filter taps.
     * @return Filtered output signal vector.
     */
    [[nodiscard]] static std::vector<T> filter_batch(std::span<const T> input,
                                                     std::span<const T> coefficients) {
        FirFilter<T> temp_filter(coefficients);
        std::vector<T> result(input.size());
        temp_filter.process(input, std::span<T>(result.data(), result.size()));
        return result;
    }

private:
    std::vector<T> coeffs_;      ///< Filter taps h[0..M-1]
    std::vector<T> state_;       ///< Delay line buffer of size M-1
    std::size_t state_index_{0}; ///< Circular buffer write index
};

// Type aliases for common precisions
using FirFilterF = FirFilter<float>;
using FirFilterD = FirFilter<double>;

} // namespace filtering
