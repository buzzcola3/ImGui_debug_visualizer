# ImGui Debug Visualizer

A lightweight Dear ImGui-based debug instrumentation library that can be embedded into any C++ application. It lets you register scalar values, live graphs, and hierarchical structures keyed by name, then renders them in a ready-to-use ImGui window so you can monitor and tune your systems in real time.

## Features

- Register scalar values (ints, floats, booleans, strings) keyed by name.
- Stream samples into time-series line graphs with automatic or manual scaling.
- Build hierarchical structures with a fluent builder API to visualize complex state.
- Organize your telemetry into tabs and spawn additional window tiles for subsystem-specific dashboards.
- Minimal dependencies for the data API, with built-in GLFW/OpenGL window management when you need it.

## Getting Started

### Prerequisites

- Bazel 6.0 or later.
- A C++17 toolchain.
- Internet access on the first build so Bazel can fetch Dear ImGui.

### Build the library and example

```bash
cd /home/buzz/Projects/ImGui_debug_visualizer
bazel build //debug_visualizer:debug_visualizer

# Interactive demo with GLFW + OpenGL renderer
bazel run //debug_visualizer:debug_visualizer_demo
```

The interactive demo spins up a GLFW window, loads OpenGL through GLEW, and renders the debug UI in real time—everything is encapsulated in the library so the example only updates values.

### Run the tests

```bash
bazel test //...
```

## Integrating into Your Project

1. Add this repository as a Bazel dependency (e.g., via `local_repository`).
2. Depend on `//debug_visualizer:debug_visualizer` from your `cc_binary` or `cc_library` targets.
3. Instantiate the provided `DebugVisualizerApp` and update your values inside the callback—window creation, OpenGL, and ImGui backends are handled for you.

```cpp
#include <cmath>
#include "debug_visualizer/debug_visualizer.h"

int main() {
    dbgvis::DebugVisualizerApp app(true);  // enable docking
    float phase = 0.0f;

    return app.run([&](dbgvis::DebugVisualizerApp& ctx, float elapsed, float delta) {
        phase += delta;

        auto& tile = ctx.Tiles["Main"];  // window id doubles as title by default

        auto& overview = tile.tabs["Overview"];
        auto& waveforms = tile.tabs["Waveforms"];
        if (!waveforms.Graph.contains("sine")) {
            waveforms.addGraph("sine");
            waveforms.addGraph("cosine");
        }

        overview.update_value("elapsed", elapsed);
        overview.update_value("delta", delta);

        waveforms.Graph["sine"].x = std::sin(phase);
        waveforms.Graph["cosine"].x = std::cos(phase);

        overview.update_structure("player", [&](dbgvis::StructureBuilder& builder) {
            builder.field("health", 87);
            auto position = builder.nested("position");
            position.field("x", std::sin(phase));
            position.field("y", std::cos(phase));
            position.field("z", 0.0f);
        });
    });
}
```

If you already have an ImGui frame inside your engine, you can still construct `dbgvis::DebugVisualizer` directly and call `render()` just like before; the helper app simply keeps the boilerplate out of your way for quick diagnostics and standalone tooling.

Need a second tabbed window dedicated to another system? Add another tile and drive it with the same API:

```cpp
auto& network_tile = ctx.Tiles["Network Monitor"];
auto& metrics = network_tile.tabs["Metrics"];
if (!metrics.Graph.contains("Trends/Bandwidth")) {
    metrics.addGraph("Trends/Bandwidth");
}
metrics.update_value("Traffic/Bandwidth", bandwidth_kbps);
metrics.Graph["Trends/Bandwidth"].x = bandwidth_kbps;
```

## Project Layout

- `debug_visualizer/` – Library headers, sources, and Bazel target.
- `tests/` – Lightweight regression tests covering core data flows.

## Next Steps

- Extend the graph API with multiple series per plot and annotations.
- Add persistence so you can record snapshots and compare frames.
- Experiment with additional renderer bindings (e.g., Vulkan, DirectX) by swapping the ImGui backend targets in the demo.

## License

The debug visualizer code is distributed under the GNU General Public License v3.0 (or later). Dear ImGui remains available under its original MIT license and is fetched directly from upstream.
