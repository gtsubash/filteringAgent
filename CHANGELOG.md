# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - 2026-11-22

### Added
- **Dedicated Signal & Spectrum Visualization Tool (`fir_viewer`)**:
  - `apps/signal_viewer.cpp`: Standalone CLI visualization tool with customizable signal parameters (`--carrier`, `--noise`, `--cutoff`, `--taps`, `--rate`).
  - `HtmlReportGenerator` (`include/filtering/html_report.hpp`, `src/html_report.cpp`): Zero-dependency HTML/SVG dashboard generator creating interactive dark-theme reports with embedded vector waveforms and spectrum meters.
  - CSV export capability (`--csv <file>`) for downstream analysis.
  - `scripts/plot_filter.py`: Optional Python plotting script for rendering PNG plots with matplotlib.
  - Design specification in `docs/designs/visualization.md`.

## [1.1.0] - 2026-11-22

### Added
- **Frequency Spectrum Analysis & Visualization (`SpectrumAnalyzer`)**:
  - `compute_spectrum`: Discrete Fourier Transform magnitude spectrum calculation across linear frequency bins up to Nyquist ($F_s / 2$).
  - `print_comparison`: Side-by-side terminal ASCII spectrum meter with dB scale and attenuation calculation.
  - Frequency response unit test in `tests/test_fir_filter.cpp` verifying peak carrier preservation and stopband attenuation $> 20\text{ dB}$.
  - Integrated frequency response visualization table into `apps/fir_demo.cpp`.

## [1.0.0] - 2026-11-22

### Added
- **Core 1D FIR Filtering Engine (`filtering_core`)**:
  - `filtering::FirFilter<T>` C++20 class supporting both `float` and `double`.
  - Direct form FIR convolution with circular delay-line state for streaming block processing.
  - Stateless one-shot `filter_batch` helper.
  - Non-owning `std::span` buffer views for zero-allocation processing.
  - Comprehensive parameter validation (`std::invalid_argument` for empty taps or undersized buffers).
- **Filter Design Utilities (`FilterDesign`)**:
  - Windowed-Sinc Low-Pass FIR filter generator with Hamming window and DC gain normalization.
  - High-Pass FIR filter generator via spectral inversion.
  - Moving Average (boxcar) filter generator.
- **Applications & Verification**:
  - `fir_demo`: Interactive CLI application filtering a 150 Hz interference tone from a 5 Hz carrier signal, including ASCII waveform rendering.
  - `fir_tests`: Deterministic unit test suite covering impulse response, DC gain step response, streaming continuity, stopband attenuation (> 20 dB), and parameter validation (44/44 tests passed).
- **Architecture & Infrastructure**:
  - Formal design specification in `docs/designs/fir_filter.md`.
  - Top-level `CMakeLists.txt` configured with C++20 standard.
  - `.gitignore` for build artifacts.
