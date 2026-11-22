# Design Specification: Dedicated Signal & Filter Output Visualization Tool

- **Status:** Approved
- **Author:** Architect Subagent
- **Target Standard:** C++20
- **Scope:** Standalone visualization application (`fir_viewer`), HTML/SVG report generator, and CSV export

---

## 1. Overview & Goals

The `fir_viewer` tool provides a comprehensive visualization suite for inspecting discrete-time signals, filter impulse responses, and frequency attenuation.
It supports:
1. **Interactive Terminal View**: High-resolution ASCII rendering for headless environments.
2. **Self-Contained HTML/SVG Dashboard**: Zero-dependency single-file HTML export with embedded SVG line charts and frequency spectrum bars.
3. **Structured CSV Export**: Comma-separated values for integration with external analysis tools (Python, MATLAB, Excel).

---

## 2. Architecture & File Layout

```text
filteringAgent/
├── include/filtering/
│   └── html_report.hpp     # HTML/SVG report generation interface
├── src/
│   └── html_report.cpp     # SVG chart drawing and HTML dashboard template
├── apps/
│   └── signal_viewer.cpp   # Standalone CLI tool entry point (fir_viewer)
├── scripts/
│   └── plot_filter.py      # Optional Python visualization script
└── docs/designs/
    └── visualization.md    # This specification
```

---

## 3. CLI Interface Contract

```text
Usage: fir_viewer [options]

Options:
  --html <filepath>   Export interactive self-contained HTML report (e.g. build/report.html)
  --csv <filepath>    Export raw and filtered signal data points to CSV
  --carrier <float>   Carrier signal frequency in Hz (default: 5.0)
  --noise <float>     Interference noise frequency in Hz (default: 150.0)
  --cutoff <float>    Filter cutoff frequency in Hz (default: 30.0)
  --taps <int>        Number of FIR filter taps (default: 41)
  --samples <int>     Total number of samples (default: 500)
  --rate <float>      Sampling rate in Hz (default: 1000.0)
  --help              Display help message
```

---

## 4. HTML/SVG Generator Architecture

`HtmlReportGenerator` produces a standalone HTML file containing:
- Dark theme styled with embedded CSS (no internet CDN calls required).
- Embedded vector SVG line plots:
  - Chart 1: Time Domain (Raw Noisy vs. Filtered Output overlay).
  - Chart 2: Frequency Domain Spectrum (Input vs. Filtered Magnitude in dB).
  - Chart 3: FIR Impulse Response ($h[k]$ taps).
- Metrics table:
  - Input RMS, Filtered RMS, Carrier Preservation (dB), Noise Attenuation (dB).
