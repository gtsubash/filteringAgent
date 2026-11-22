#include "filtering/html_report.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace filtering {

namespace {

std::string generate_svg_polyline(const std::vector<float>& x_vals,
                                  const std::vector<float>& y_vals,
                                  float min_x, float max_x,
                                  float min_y, float max_y,
                                  float width, float height,
                                  float pad_left, float pad_top)
{
    if (x_vals.empty() || y_vals.empty() || x_vals.size() != y_vals.size()) {
        return "";
    }

    const float dx = (max_x - min_x) > 1e-6f ? (max_x - min_x) : 1.0f;
    const float dy = (max_y - min_y) > 1e-6f ? (max_y - min_y) : 1.0f;

    std::ostringstream oss;
    for (std::size_t i = 0; i < x_vals.size(); ++i) {
        float px = pad_left + ((x_vals[i] - min_x) / dx) * width;
        float py = pad_top + height - ((y_vals[i] - min_y) / dy) * height;
        oss << std::fixed << std::setprecision(1) << px << "," << py << " ";
    }
    return oss.str();
}

} // namespace

bool HtmlReportGenerator::generate_csv(const std::string& filepath, const VisualizationData& data) {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        return false;
    }

    out << "time_sec,raw_signal,filtered_signal\n";
    const std::size_t n = std::min({data.time.size(), data.raw_signal.size(), data.filtered_signal.size()});
    for (std::size_t i = 0; i < n; ++i) {
        out << std::fixed << std::setprecision(6)
            << data.time[i] << ","
            << data.raw_signal[i] << ","
            << data.filtered_signal[i] << "\n";
    }
    return true;
}

bool HtmlReportGenerator::generate_html(const std::string& filepath, const VisualizationData& data) {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        return false;
    }

    // Chart dimensions
    const float chart_w = 750.0f;
    const float chart_h = 240.0f;
    const float pad_l = 60.0f;
    const float pad_t = 30.0f;
    const float svg_w = 840.0f;
    const float svg_h = 300.0f;

    // 1. Time-Domain Range
    float min_time = 0.0f;
    float max_time = data.time.empty() ? 1.0f : data.time.back();
    float min_sig = -1.5f;
    float max_sig = 1.5f;
    for (float v : data.raw_signal) {
        min_sig = std::min(min_sig, v);
        max_sig = std::max(max_sig, v);
    }
    for (float v : data.filtered_signal) {
        min_sig = std::min(min_sig, v);
        max_sig = std::max(max_sig, v);
    }

    std::string raw_poly = generate_svg_polyline(data.time, data.raw_signal,
                                                 min_time, max_time, min_sig, max_sig,
                                                 chart_w, chart_h, pad_l, pad_t);
    std::string filt_poly = generate_svg_polyline(data.time, data.filtered_signal,
                                                  min_time, max_time, min_sig, max_sig,
                                                  chart_w, chart_h, pad_l, pad_t);

    // 2. Frequency-Domain Range
    std::vector<float> freq_x;
    std::vector<float> in_db_y;
    std::vector<float> out_db_y;
    for (const auto& pt : data.input_spectrum.points) {
        freq_x.push_back(pt.frequency_hz);
        in_db_y.push_back(pt.magnitude_db);
    }
    for (const auto& pt : data.output_spectrum.points) {
        out_db_y.push_back(pt.magnitude_db);
    }

    float max_freq = freq_x.empty() ? 500.0f : freq_x.back();
    float min_db = -80.0f;
    float max_db = 10.0f;

    std::string in_spec_poly = generate_svg_polyline(freq_x, in_db_y, 0.0f, max_freq, min_db, max_db,
                                                     chart_w, chart_h, pad_l, pad_t);
    std::string out_spec_poly = generate_svg_polyline(freq_x, out_db_y, 0.0f, max_freq, min_db, max_db,
                                                      chart_w, chart_h, pad_l, pad_t);

    out << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)HTML" << data.title << R"HTML(</title>
    <style>
        :root {
            --bg-color: #0f172a;
            --card-bg: #1e293b;
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
            --accent-cyan: #38bdf8;
            --accent-orange: #fb923c;
            --accent-green: #4ade80;
            --border-color: #334155;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-main);
            margin: 0;
            padding: 24px;
        }
        .container { max-width: 900px; margin: 0 auto; }
        header { margin-bottom: 24px; border-bottom: 1px solid var(--border-color); padding-bottom: 12px; }
        h1 { margin: 0 0 8px 0; font-size: 26px; font-weight: 700; color: var(--accent-cyan); }
        .subtitle { color: var(--text-muted); font-size: 14px; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 16px; margin-bottom: 24px; }
        .card { background: var(--card-bg); border-radius: 8px; border: 1px solid var(--border-color); padding: 16px; }
        .metric-title { font-size: 12px; text-transform: uppercase; color: var(--text-muted); letter-spacing: 0.05em; }
        .metric-value { font-size: 24px; font-weight: 700; margin-top: 4px; color: var(--text-main); }
        .chart-box { background: var(--card-bg); border-radius: 8px; border: 1px solid var(--border-color); padding: 20px; margin-bottom: 24px; }
        .chart-title { font-size: 18px; font-weight: 600; margin-bottom: 12px; display: flex; align-items: center; justify-content: space-between; }
        .legend { font-size: 13px; display: flex; gap: 16px; }
        .legend-item { display: flex; align-items: center; gap: 6px; }
        .dot { width: 10px; height: 10px; border-radius: 50%; display: inline-block; }
        svg text { fill: var(--text-muted); font-size: 11px; }
    </style>
</head>
<body>
<div class="container">
    <header>
        <h1>)HTML" << data.title << R"HTML(</h1>
        <div class="subtitle">Self-Contained C++20 FIR Signal & Spectrum Visualizer Report</div>
    </header>

    <div class="grid">
        <div class="card">
            <div class="metric-title">Sampling Rate</div>
            <div class="metric-value">)HTML" << static_cast<int>(data.sample_rate) << R"HTML( Hz</div>
        </div>
        <div class="card">
            <div class="metric-title">Cutoff Frequency</div>
            <div class="metric-value" style="color:var(--accent-cyan)">)HTML" << static_cast<int>(data.cutoff_freq) << R"HTML( Hz</div>
        </div>
        <div class="card">
            <div class="metric-title">Noise Attenuation</div>
            <div class="metric-value" style="color:var(--accent-green)">-)HTML"
        << std::fixed << std::setprecision(1) << data.noise_attenuation_db << R"HTML( dB</div>
        </div>
        <div class="card">
            <div class="metric-title">Filter Taps</div>
            <div class="metric-value">)HTML" << data.coefficients.size() << R"HTML(</div>
        </div>
    </div>

    <!-- Chart 1: Time Domain -->
    <div class="chart-box">
        <div class="chart-title">
            <span>Time-Domain Signal Comparison</span>
            <div class="legend">
                <span class="legend-item"><span class="dot" style="background:var(--accent-orange)"></span> Raw Noisy Input</span>
                <span class="legend-item"><span class="dot" style="background:var(--accent-cyan)"></span> Filtered Output</span>
            </div>
        </div>
        <svg viewBox="0 0 )HTML" << svg_w << " " << svg_h << R"HTML(" width="100%" height="auto">
            <!-- Grid lines -->
            <line x1=")HTML" << pad_l << R"HTML(" y1=")HTML" << pad_t + chart_h/2.0f << R"HTML(" x2=")HTML" << pad_l + chart_w << R"HTML(" y2=")HTML" << pad_t + chart_h/2.0f << R"HTML(" stroke="#334155" stroke-dasharray="4,4"/>
            <rect x=")HTML" << pad_l << R"HTML(" y=")HTML" << pad_t << R"HTML(" width=")HTML" << chart_w << R"HTML(" height=")HTML" << chart_h << R"HTML(" fill="none" stroke="#334155"/>
            <!-- Waveforms -->
            <polyline points=")HTML" << raw_poly << R"HTML(" fill="none" stroke="#fb923c" stroke-width="1.2" opacity="0.7"/>
            <polyline points=")HTML" << filt_poly << R"HTML(" fill="none" stroke="#38bdf8" stroke-width="2.2"/>
            <!-- Labels -->
            <text x=")HTML" << pad_l << R"HTML(" y=")HTML" << pad_t + chart_h + 18 << R"HTML(">0.00s</text>
            <text x=")HTML" << pad_l + chart_w/2.0f << R"HTML(" y=")HTML" << pad_t + chart_h + 18 << R"HTML(" text-anchor="middle">Time (seconds)</text>
            <text x=")HTML" << pad_l + chart_w << R"HTML(" y=")HTML" << pad_t + chart_h + 18 << R"HTML(" text-anchor="end">)HTML" << max_time << R"HTML(s</text>
            <text x=")HTML" << pad_l - 10 << R"HTML(" y=")HTML" << pad_t + 5 << R"HTML(" text-anchor="end">)HTML" << std::setprecision(1) << max_sig << R"HTML(</text>
            <text x=")HTML" << pad_l - 10 << R"HTML(" y=")HTML" << pad_t + chart_h << R"HTML(" text-anchor="end">)HTML" << min_sig << R"HTML(</text>
        </svg>
    </div>

    <!-- Chart 2: Frequency Domain -->
    <div class="chart-box">
        <div class="chart-title">
            <span>Frequency-Domain Spectrum Comparison</span>
            <div class="legend">
                <span class="legend-item"><span class="dot" style="background:var(--accent-orange)"></span> Input Spectrum</span>
                <span class="legend-item"><span class="dot" style="background:var(--accent-cyan)"></span> Filtered Spectrum</span>
            </div>
        </div>
        <svg viewBox="0 0 )HTML" << svg_w << " " << svg_h << R"HTML(" width="100%" height="auto">
            <!-- Grid lines -->
            <line x1=")HTML" << pad_l << R"HTML(" y1=")HTML" << pad_t + chart_h * 0.5f << R"HTML(" x2=")HTML" << pad_l + chart_w << R"HTML(" y2=")HTML" << pad_t + chart_h * 0.5f << R"HTML(" stroke="#334155" stroke-dasharray="4,4"/>
            <rect x=")HTML" << pad_l << R"HTML(" y=")HTML" << pad_t << R"HTML(" width=")HTML" << chart_w << R"HTML(" height=")HTML" << chart_h << R"HTML(" fill="none" stroke="#334155"/>
            <!-- Spectrum lines -->
            <polyline points=")HTML" << in_spec_poly << R"HTML(" fill="none" stroke="#fb923c" stroke-width="1.6" opacity="0.8"/>
            <polyline points=")HTML" << out_spec_poly << R"HTML(" fill="none" stroke="#38bdf8" stroke-width="2.4"/>
            <!-- Labels -->
            <text x=")HTML" << pad_l << R"HTML(" y=")HTML" << pad_t + chart_h + 18 << R"HTML(">0 Hz</text>
            <text x=")HTML" << pad_l + chart_w/2.0f << R"HTML(" y=")HTML" << pad_t + chart_h + 18 << R"HTML(" text-anchor="middle">Frequency (Hz)</text>
            <text x=")HTML" << pad_l + chart_w << R"HTML(" y=")HTML" << pad_t + chart_h + 18 << R"HTML(" text-anchor="end">)HTML" << static_cast<int>(max_freq) << R"HTML( Hz</text>
            <text x=")HTML" << pad_l - 10 << R"HTML(" y=")HTML" << pad_t + 5 << R"HTML(" text-anchor="end">0 dB</text>
            <text x=")HTML" << pad_l - 10 << R"HTML(" y=")HTML" << pad_t + chart_h << R"HTML(" text-anchor="end">-80 dB</text>
        </svg>
    </div>
</div>
</body>
</html>)HTML";

    return true;
}

} // namespace filtering
