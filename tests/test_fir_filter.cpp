#include "filtering/fir_filter.hpp"
#include "filtering/filter_design.hpp"
#include "filtering/spectrum.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {

int g_tests_passed = 0;
int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[-] FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            g_tests_failed++; \
        } else { \
            std::cout << "[+] PASS: " << msg << "\n"; \
            g_tests_passed++; \
        } \
    } while (0)

constexpr float kTolerance = 1e-5f;

// 1. Impulse Response Test
void test_impulse_response() {
    std::vector<float> coeffs = {0.1f, 0.25f, 0.3f, 0.25f, 0.1f};
    filtering::FirFilter<float> filter(coeffs);

    std::vector<float> impulse(10, 0.0f);
    impulse[0] = 1.0f; // Delta impulse

    std::vector<float> output(impulse.size());
    filter.process(impulse, output);

    for (std::size_t i = 0; i < coeffs.size(); ++i) {
        TEST_ASSERT(std::abs(output[i] - coeffs[i]) < kTolerance,
                    "Impulse response output matches coefficient at index " + std::to_string(i));
    }
    for (std::size_t i = coeffs.size(); i < impulse.size(); ++i) {
        TEST_ASSERT(std::abs(output[i]) < kTolerance,
                    "Output is zero after impulse response duration");
    }
}

// 2. Step Response & DC Gain Test
void test_step_response() {
    std::vector<float> coeffs = {0.1f, 0.2f, 0.4f, 0.2f, 0.1f};
    float expected_dc_gain = 0.0f;
    for (float c : coeffs) expected_dc_gain += c;

    filtering::FirFilter<float> filter(coeffs);
    std::vector<float> step(30, 1.0f);
    std::vector<float> output(step.size());
    filter.process(step, output);

    for (std::size_t i = coeffs.size() - 1; i < step.size(); ++i) {
        TEST_ASSERT(std::abs(output[i] - expected_dc_gain) < kTolerance,
                    "Step response converges to DC gain (" + std::to_string(expected_dc_gain) + ")");
    }
}

// 3. Block Streaming State Continuity Test
void test_streaming_continuity() {
    auto coeffs = filtering::FilterDesign::create_lowpass_fir<float>(21, 0.1f);
    const std::size_t total_samples = 256;
    std::vector<float> signal(total_samples);
    for (std::size_t i = 0; i < total_samples; ++i) {
        signal[i] = std::sin(0.05f * static_cast<float>(i));
    }

    auto batch_output = filtering::FirFilter<float>::filter_batch(signal, coeffs);

    filtering::FirFilter<float> streaming_filter(coeffs);
    std::vector<float> streamed_output(total_samples);
    const std::size_t chunk_size = 16;
    for (std::size_t i = 0; i < total_samples; i += chunk_size) {
        std::span<const float> in_chunk(&signal[i], chunk_size);
        std::span<float> out_chunk(&streamed_output[i], chunk_size);
        streaming_filter.process(in_chunk, out_chunk);
    }

    bool matched = true;
    for (std::size_t i = 0; i < total_samples; ++i) {
        if (std::abs(batch_output[i] - streamed_output[i]) > 1e-6f) {
            matched = false;
            break;
        }
    }
    TEST_ASSERT(matched, "Streaming chunk-by-chunk produces identical output to batch filtering");
}

// 4. Frequency Attenuation Test
void test_frequency_attenuation() {
    const std::size_t taps = 41;
    const float cutoff = 0.05f; // 50 Hz at 1000 Hz Fs
    auto coeffs = filtering::FilterDesign::create_lowpass_fir<float>(taps, cutoff);
    filtering::FirFilter<float> filter(coeffs);

    const std::size_t n = 500;
    std::vector<float> low_freq(n);
    std::vector<float> high_freq(n);

    for (std::size_t i = 0; i < n; ++i) {
        low_freq[i] = std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * 0.01f * static_cast<float>(i));
        high_freq[i] = std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * 0.25f * static_cast<float>(i));
    }

    std::vector<float> low_out(n);
    std::vector<float> high_out(n);

    filter.process(low_freq, low_out);
    filter.reset();
    filter.process(high_freq, high_out);

    float low_energy = 0.0f;
    float high_energy = 0.0f;
    for (std::size_t i = taps; i < n; ++i) {
        low_energy += low_out[i] * low_out[i];
        high_energy += high_out[i] * high_out[i];
    }

    TEST_ASSERT(low_energy > 50.0f, "Passband frequency is preserved");
    TEST_ASSERT(high_energy < 0.5f, "Stopband frequency is heavily attenuated (> 20 dB)");
}

// 5. Frequency Spectrum Analysis and Visualization Test
void test_frequency_spectrum_visualization() {
    std::cout << "\n--- Testing Frequency Spectrum Computation & Visualization ---\n";
    const float sample_rate = 1000.0f;
    const std::size_t total_samples = 500;
    const float carrier_freq = 10.0f; // 10 Hz in passband
    const float noise_freq = 120.0f;  // 120 Hz in stopband

    std::vector<float> noisy(total_samples);
    for (std::size_t i = 0; i < total_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        noisy[i] = std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * carrier_freq * t)
                 + 0.5f * std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * noise_freq * t);
    }

    // 41-tap Lowpass FIR with cutoff at 30 Hz
    const std::size_t taps = 41;
    auto coeffs = filtering::FilterDesign::create_lowpass_fir<float>(taps, 30.0f / sample_rate);
    filtering::FirFilter<float> filter(coeffs);
    std::vector<float> filtered(total_samples);
    filter.process(noisy, filtered);

    // Compute frequency spectrum for steady-state (skipping filter startup transient)
    std::span<const float> steady_in(noisy.data() + taps, total_samples - taps);
    std::span<const float> steady_out(filtered.data() + taps, total_samples - taps);

    auto in_spec = filtering::SpectrumAnalyzer::compute_spectrum<float>(steady_in, sample_rate, 100);
    auto out_spec = filtering::SpectrumAnalyzer::compute_spectrum<float>(steady_out, sample_rate, 100);

    TEST_ASSERT(!in_spec.points.empty(), "Input spectrum points computed");
    TEST_ASSERT(!out_spec.points.empty(), "Output spectrum points computed");

    // Peak frequency of both should be the carrier (10 Hz)
    TEST_ASSERT(std::abs(in_spec.peak_frequency_hz - carrier_freq) < 6.0f,
                "Input peak frequency detected near 10 Hz (" + std::to_string(in_spec.peak_frequency_hz) + " Hz)");
    TEST_ASSERT(std::abs(out_spec.peak_frequency_hz - carrier_freq) < 6.0f,
                "Filtered peak frequency detected near 10 Hz (" + std::to_string(out_spec.peak_frequency_hz) + " Hz)");

    // Find the 120 Hz noise bin
    float in_noise_db = -120.0f;
    float out_noise_db = -120.0f;
    for (std::size_t i = 0; i < in_spec.points.size(); ++i) {
        if (std::abs(in_spec.points[i].frequency_hz - noise_freq) < 6.0f) {
            in_noise_db = in_spec.points[i].magnitude_db;
            out_noise_db = out_spec.points[i].magnitude_db;
            break;
        }
    }

    float noise_attenuation = in_noise_db - out_noise_db;
    TEST_ASSERT(noise_attenuation > 20.0f,
                "120 Hz noise is attenuated by at least 20 dB (Actual: " + std::to_string(noise_attenuation) + " dB)");

    // Render the visual comparison table
    filtering::SpectrumAnalyzer::print_comparison(in_spec, out_spec, 20);
}

// 6. State Reset Test
void test_reset() {
    std::vector<float> coeffs = {0.2f, 0.5f, 0.3f};
    filtering::FirFilter<float> filter(coeffs);

    (void)filter.process_sample(10.0f);
    filter.reset();

    float out = filter.process_sample(0.0f);
    TEST_ASSERT(std::abs(out) < kTolerance, "Filter reset correctly zeroes internal delay line");
}

// 7. Parameter Validation Test
void test_validation() {
    bool caught_empty = false;
    try {
        std::vector<float> empty_coeffs;
        filtering::FirFilter<float> f(empty_coeffs);
    } catch (const std::invalid_argument&) {
        caught_empty = true;
    }
    TEST_ASSERT(caught_empty, "Empty coefficients throws std::invalid_argument");

    filtering::FirFilter<float> f(std::vector<float>{1.0f, 2.0f});
    std::vector<float> in = {1.0f, 2.0f};
    std::vector<float> small_out = {0.0f};
    bool caught_size = false;
    try {
        f.process(in, small_out);
    } catch (const std::invalid_argument&) {
        caught_size = true;
    }
    TEST_ASSERT(caught_size, "Small output buffer throws std::invalid_argument");
}

// 8. Single Tap Filter
void test_single_tap() {
    filtering::FirFilter<float> scalar({3.5f});
    TEST_ASSERT(scalar.num_taps() == 1, "Single tap filter has num_taps == 1");
    float out = scalar.process_sample(2.0f);
    TEST_ASSERT(std::abs(out - 7.0f) < kTolerance, "Single tap behaves as exact scalar multiplier");
}

} // namespace

int main() {
    std::cout << "=========================================\n";
    std::cout << " Running 1D FIR Filtering Test Suite\n";
    std::cout << "=========================================\n";

    test_impulse_response();
    test_step_response();
    test_streaming_continuity();
    test_frequency_attenuation();
    test_frequency_spectrum_visualization();
    test_reset();
    test_validation();
    test_single_tap();

    std::cout << "\n=========================================\n";
    std::cout << " Test Summary: " << g_tests_passed << " passed, "
              << g_tests_failed << " failed.\n";
    std::cout << "=========================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
