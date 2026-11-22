#pragma once

#include "filtering/spectrum.hpp"
#include <span>
#include <string>
#include <vector>

namespace filtering {

struct VisualizationData {
    std::string title{"FIR Filter Analysis Report"};
    float sample_rate{1000.0f};
    std::vector<float> time;
    std::vector<float> raw_signal;
    std::vector<float> filtered_signal;
    std::vector<float> coefficients;
    SpectrumResult input_spectrum;
    SpectrumResult output_spectrum;
    float raw_rms{0.0f};
    float filtered_rms{0.0f};
    float noise_attenuation_db{0.0f};
    float carrier_freq{5.0f};
    float noise_freq{150.0f};
    float cutoff_freq{30.0f};
};

/**
 * @brief Zero-dependency generator for interactive HTML/SVG reports and CSV exports.
 */
class HtmlReportGenerator {
public:
    /**
     * @brief Generate a self-contained HTML report with embedded SVG charts.
     */
    static bool generate_html(const std::string& filepath, const VisualizationData& data);

    /**
     * @brief Export time-domain signals to CSV format.
     */
    static bool generate_csv(const std::string& filepath, const VisualizationData& data);
};

} // namespace filtering
