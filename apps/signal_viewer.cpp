#include "filtering/filter_design.hpp"
#include "filtering/fir_filter.hpp"
#include "filtering/html_report.hpp"
#include "filtering/spectrum.hpp"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void print_help(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  --html <file>     Export self-contained interactive HTML/SVG report\n"
              << "  --csv <file>      Export raw and filtered signal samples to CSV\n"
              << "  --carrier <float> Carrier frequency in Hz (default: 5.0)\n"
              << "  --noise <float>   Noise frequency in Hz (default: 150.0)\n"
              << "  --cutoff <float>  Filter cutoff frequency in Hz (default: 30.0)\n"
              << "  --taps <int>      Filter taps (default: 41)\n"
              << "  --samples <int>   Number of samples (default: 500)\n"
              << "  --rate <float>    Sample rate in Hz (default: 1000.0)\n"
              << "  --help            Show this help message\n";
}

} // namespace

int main(int argc, char* argv[]) {
    std::string html_path = "build/visualization.html";
    std::string csv_path = "";
    float carrier_freq = 5.0f;
    float noise_freq = 150.0f;
    float cutoff_freq = 30.0f;
    int taps = 41;
    int total_samples = 500;
    float sample_rate = 1000.0f;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "--html" && i + 1 < argc) {
            html_path = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            csv_path = argv[++i];
        } else if (arg == "--carrier" && i + 1 < argc) {
            carrier_freq = std::stof(argv[++i]);
        } else if (arg == "--noise" && i + 1 < argc) {
            noise_freq = std::stof(argv[++i]);
        } else if (arg == "--cutoff" && i + 1 < argc) {
            cutoff_freq = std::stof(argv[++i]);
        } else if (arg == "--taps" && i + 1 < argc) {
            taps = std::stoi(argv[++i]);
        } else if (arg == "--samples" && i + 1 < argc) {
            total_samples = std::stoi(argv[++i]);
        } else if (arg == "--rate" && i + 1 < argc) {
            sample_rate = std::stof(argv[++i]);
        }
    }

    std::cout << "=========================================================\n";
    std::cout << "        FIR Signal & Spectrum Visualization Tool\n";
    std::cout << "=========================================================\n";
    std::cout << "Signal Configuration:\n"
              << "  - Sampling Rate:      " << sample_rate << " Hz\n"
              << "  - Carrier Frequency:  " << carrier_freq << " Hz\n"
              << "  - Noise Frequency:    " << noise_freq << " Hz\n"
              << "  - FIR Filter Cutoff:  " << cutoff_freq << " Hz (" << taps << " taps)\n"
              << "  - Total Samples:      " << total_samples << "\n\n";

    // 1. Generate Signal
    std::vector<float> time(total_samples);
    std::vector<float> raw_signal(total_samples);
    std::vector<float> clean_signal(total_samples);

    for (int i = 0; i < total_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        time[i] = t;
        float s_clean = std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * carrier_freq * t);
        float s_noise = 0.6f * std::sin(2.0f * static_cast<float>(filtering::FilterDesign::kPi) * noise_freq * t);
        clean_signal[i] = s_clean;
        raw_signal[i] = s_clean + s_noise;
    }

    // 2. Filter Signal
    auto coeffs = filtering::FilterDesign::create_lowpass_fir<float>(static_cast<std::size_t>(taps), cutoff_freq / sample_rate);
    filtering::FirFilter<float> filter(coeffs);
    std::vector<float> filtered_signal(total_samples);
    filter.process(raw_signal, filtered_signal);

    // 3. Frequency Spectrum Analysis
    // Skip transient startup
    const std::size_t warmup = static_cast<std::size_t>(taps);
    std::span<const float> steady_raw(raw_signal.data() + warmup, total_samples - warmup);
    std::span<const float> steady_filt(filtered_signal.data() + warmup, total_samples - warmup);

    auto in_spec = filtering::SpectrumAnalyzer::compute_spectrum<float>(steady_raw, sample_rate, 100);
    auto out_spec = filtering::SpectrumAnalyzer::compute_spectrum<float>(steady_filt, sample_rate, 100);

    // Find noise attenuation
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

    // 4. Print Terminal Visualizations
    filtering::SpectrumAnalyzer::print_comparison(in_spec, out_spec, 20);

    // 5. Generate Interactive Visualizations & Exports
    filtering::VisualizationData vis_data;
    vis_data.title = "FIR Filter Performance & Spectrum Analysis";
    vis_data.sample_rate = sample_rate;
    vis_data.time = time;
    vis_data.raw_signal = raw_signal;
    vis_data.filtered_signal = filtered_signal;
    vis_data.coefficients = coeffs;
    vis_data.input_spectrum = in_spec;
    vis_data.output_spectrum = out_spec;
    vis_data.carrier_freq = carrier_freq;
    vis_data.noise_freq = noise_freq;
    vis_data.cutoff_freq = cutoff_freq;
    vis_data.noise_attenuation_db = noise_attenuation;

    if (!html_path.empty()) {
        if (filtering::HtmlReportGenerator::generate_html(html_path, vis_data)) {
            std::cout << "[+] Interactive HTML/SVG report generated: " << html_path << "\n";
        } else {
            std::cerr << "[-] Failed to write HTML report to " << html_path << "\n";
        }
    }

    if (!csv_path.empty()) {
        if (filtering::HtmlReportGenerator::generate_csv(csv_path, vis_data)) {
            std::cout << "[+] Signal samples exported to CSV: " << csv_path << "\n";
        } else {
            std::cerr << "[-] Failed to write CSV to " << csv_path << "\n";
        }
    }

    std::cout << "\nVisualization completed successfully!\n";
    return 0;
}
