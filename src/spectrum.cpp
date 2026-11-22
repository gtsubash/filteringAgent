#include "filtering/spectrum.hpp"

#include <algorithm>
#include <iomanip>

namespace filtering {

namespace {

std::string make_meter(float db, int bar_width = 16, float min_db = -60.0f, float max_db = 0.0f) {
    float clamped = std::clamp(db, min_db, max_db);
    float norm = (clamped - min_db) / (max_db - min_db);
    int num_filled = static_cast<int>(norm * static_cast<float>(bar_width));
    num_filled = std::clamp(num_filled, 0, bar_width);

    std::string bar = "[";
    bar.append(num_filled, '#');
    bar.append(bar_width - num_filled, ' ');
    bar += "]";
    return bar;
}

} // namespace

void SpectrumAnalyzer::print_comparison(
    const SpectrumResult& input_spec,
    const SpectrumResult& output_spec,
    std::size_t display_bins,
    std::ostream& out)
{
    if (input_spec.points.empty() || output_spec.points.empty()) {
        out << "Empty spectrum data.\n";
        return;
    }

    const std::size_t total_points = std::min(input_spec.points.size(), output_spec.points.size());
    const std::size_t step = std::max<std::size_t>(1, total_points / display_bins);

    out << "\n========================================================================================\n";
    out << "                     Frequency Spectrum Comparison (Input vs. Filtered)\n";
    out << "========================================================================================\n";
    out << std::setw(10) << "Freq (Hz)" << " | "
        << std::setw(25) << "Input Spectrum [dB]" << " | "
        << std::setw(25) << "Filtered Spectrum [dB]" << " | "
        << "Attenuation\n";
    out << "----------------------------------------------------------------------------------------\n";

    for (std::size_t i = 0; i < total_points; i += step) {
        const auto& in_pt = input_spec.points[i];
        const auto& out_pt = output_spec.points[i];

        float delta_db = out_pt.magnitude_db - in_pt.magnitude_db;

        std::string in_meter = make_meter(in_pt.magnitude_db);
        std::string out_meter = make_meter(out_pt.magnitude_db);

        out << std::fixed << std::setprecision(1)
            << std::setw(9) << in_pt.frequency_hz << "Hz | "
            << in_meter << " " << std::setw(6) << std::setprecision(1) << in_pt.magnitude_db << "dB | "
            << out_meter << " " << std::setw(6) << std::setprecision(1) << out_pt.magnitude_db << "dB | "
            << std::setw(6) << std::setprecision(1) << delta_db << " dB\n";
    }

    out << "========================================================================================\n";
    out << " Peak (Input):    " << input_spec.peak_frequency_hz << " Hz (" << input_spec.peak_magnitude_db << " dB)\n";
    out << " Peak (Filtered): " << output_spec.peak_frequency_hz << " Hz (" << output_spec.peak_magnitude_db << " dB)\n\n";
}

// Explicit template instantiations
template SpectrumResult SpectrumAnalyzer::compute_spectrum<float>(std::span<const float>, float, std::size_t);
template SpectrumResult SpectrumAnalyzer::compute_spectrum<double>(std::span<const double>, float, std::size_t);

} // namespace filtering
