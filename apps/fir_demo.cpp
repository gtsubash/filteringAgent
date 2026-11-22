#include "filtering/fir_filter.hpp"
#include "filtering/filter_design.hpp"
#include "filtering/spectrum.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void render_ascii_waveform(const std::string& title, const std::vector<float>& signal, std::size_t count, int height = 11) {
    std::cout << "\n=== " << title << " (First " << count << " samples) ===\n";
    float min_val = signal[0];
    float max_val = signal[0];
    for (std::size_t i = 0; i < count; ++i) {
        min_val = std::min(min_val, signal[i]);
        max_val = std::max(max_val, signal[i]);
    }
    if (std::abs(max_val - min_val) < 1e-6f) {
        max_val += 1.0f;
    }

    const int width = static_cast<int>(count);
    std::vector<std::string> grid(height, std::string(width, ' '));

    for (int col = 0; col < width; ++col) {
        float norm = (signal[col] - min_val) / (max_val - min_val);
        int row = height - 1 - static_cast<int>(norm * (height - 1));
        row = std::clamp(row, 0, height - 1);
        grid[row][col] = '*';
    }

    for (int r = 0; r < height; ++r) {
        std::cout << "|" << grid[r] << "|\n";
    }
    std::cout << "Range: [" << std::fixed << std::setprecision(2) << min_val << " to " << max_val << "]\n";
}

float compute_rms(const std::vector<float>& vec, std::size_t start_idx = 0) {
    if (vec.size() <= start_idx) return 0.0f;
    double sum_sq = 0.0;
    for (std::size_t i = start_idx; i < vec.size(); ++i) {
        sum_sq += vec[i] * vec[i];
    }
    return static_cast<float>(std::sqrt(sum_sq / static_cast<double>(vec.size() - start_idx)));
}

} // namespace

int main() {
    std::cout << "=========================================================\n";
    std::cout << "    1D FIR Filtering Engine Demonstration (C++20)\n";
    std::cout << "=========================================================\n\n";

    // 1. Setup Signal Parameters
    const float sample_rate = 1000.0f; // 1 kHz sampling rate
    const float duration = 0.5f;       // 0.5 seconds
    const std::size_t total_samples = static_cast<std::size_t>(sample_rate * duration);

    const float clean_freq = 5.0f;     // 5 Hz low frequency carrier
    const float noise_freq = 150.0f;   // 150 Hz high-frequency interference

    std::vector<float> time(total_samples);
    std::vector<float> clean_signal(total_samples);
    std::vector<float> noisy_signal(total_samples);

    for (std::size_t i = 0; i < total_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        time[i] = t;
        float s_clean = std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * clean_freq * t);
        float s_noise = 0.6f * std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * noise_freq * t);
        clean_signal[i] = s_clean;
        noisy_signal[i] = s_clean + s_noise;
    }

    std::cout << "Generated " << total_samples << " samples at " << sample_rate << " Hz.\n";
    std::cout << "  - Carrier signal: " << clean_freq << " Hz (amplitude 1.0)\n";
    std::cout << "  - Interference:   " << noise_freq << " Hz (amplitude 0.6)\n\n";

    // 2. Design Low-Pass FIR Filter
    const std::size_t taps = 41;
    const float cutoff_ratio = 30.0f / sample_rate; // 30 Hz cutoff
    std::cout << "Designing " << taps << "-tap Windowed-Sinc Low-Pass FIR Filter:\n";
    std::cout << "  - Cutoff frequency: 30.0 Hz (Normalized: " << cutoff_ratio << ")\n";

    auto coeffs = filtering::FilterDesign::create_lowpass_fir<float>(taps, cutoff_ratio);

    // 3. Instantiate and execute FIR filter
    filtering::FirFilter<float> fir(coeffs);
    std::vector<float> filtered_signal(total_samples);
    fir.process(noisy_signal, filtered_signal);

    // 4. Analysis and Comparison
    const std::size_t warmup = taps;
    float raw_rms = compute_rms(noisy_signal, warmup);
    float filtered_rms = compute_rms(filtered_signal, warmup);
    float clean_rms = compute_rms(clean_signal, warmup);

    std::cout << "\nResults:\n";
    std::cout << "  - Raw Noisy Signal RMS: " << std::fixed << std::setprecision(4) << raw_rms << "\n";
    std::cout << "  - Pure Clean Signal RMS:" << clean_rms << "\n";
    std::cout << "  - Filtered Signal RMS:  " << filtered_rms << "\n";

    // 5. Render waveforms
    render_ascii_waveform("Raw Signal (Carrier + 150Hz Noise)", noisy_signal, 80);
    render_ascii_waveform("Filtered Output (Low-Pass Filtered)", filtered_signal, 80);

    // 6. Frequency Spectrum Analysis and Comparison Visualization
    auto input_spec = filtering::SpectrumAnalyzer::compute_spectrum<float>(noisy_signal, sample_rate, 100);
    auto output_spec = filtering::SpectrumAnalyzer::compute_spectrum<float>(filtered_signal, sample_rate, 100);
    filtering::SpectrumAnalyzer::print_comparison(input_spec, output_spec, 20);

    std::cout << "FIR filtering demonstration completed successfully!\n";
    return 0;
}
