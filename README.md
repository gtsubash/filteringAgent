# Filtering Agent

A high-performance C++20 signal processing and computer vision library developed using a 6-stage automated agent pipeline.

## Features

- **C++20 1D FIR Filtering Engine**: `FirFilter<T>` supporting `float` and `double`.
- **Stateful Streaming & One-Shot Batching**: Ring-buffer delay line preserves continuous phase between block calls without boundary artifacts.
- **Windowed-Sinc Design Utilities**: Hamming-windowed low-pass, high-pass, and moving average coefficient generators.
- **Zero Raw Pointers & Memory Safe**: RAII semantics and non-owning `std::span` views with compile-time concepts.
- **CPU First with GPU Decoupling**: Pure C++20 standard library implementation runnable on any CPU without GPU dependencies (`ENABLE_GPU=OFF`).

---

## Building and Testing

### Prerequisites
- CMake >= 3.18
- C++20 compatible compiler (MSVC 2019/2022, GCC 10+, or Clang 11+)

### Build Commands

```powershell
# Configure build
cmake -B build -G "Visual Studio 16 2019" -A x64

# Compile all targets (core library, demo, test suite)
cmake --build build --config Debug --parallel

# Run unit tests
ctest --test-dir build -C Debug --output-on-failure
# Or directly:
.\build\Debug\fir_tests.exe

# Run interactive CLI demonstration
.\build\Debug\fir_demo.exe

# Run the dedicated visualization tool (generates interactive HTML & CSV)
.\build\Debug\fir_viewer.exe --html build/visualization.html --csv build/signals.csv
```

---

## 6-Stage Engineering Pipeline

The repository uses an automated 6-stage subagent lifecycle:

1. **[Planner](.gemini/agents/planner.md)**: Scopes work, ordered tasks, CPU vs. GPU requirements.
2. **[Architect](.gemini/agents/architect.md)**: Defines contracts, data structures, and specs under `docs/designs/`. See [docs/designs/fir_filter.md](docs/designs/fir_filter.md).
3. **[Developer](.gemini/agents/developer.md)**: Writes C++20 implementations and builds with `cpu-debug`.
4. **[Reviewer](.gemini/agents/reviewer.md)**: Performs memory, numeric safety, and CPU/GPU isolation audits.
5. **[Tester](.gemini/agents/tester.md)**: Authors unit tests and executes ASan/UBSan test suites.
6. **[Releaser](.gemini/agents/releaser.md)**: Manages release readiness, changelogs, and CI triggers.

Detailed pipeline documentation is available in [docs/pipeline.md](docs/pipeline.md).
