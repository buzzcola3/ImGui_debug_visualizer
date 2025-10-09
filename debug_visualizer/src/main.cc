/*
 *  This file is part of ImGui Debug Visualizer project.
 *  Copyright (C) 2025 buzzcola3 (Samuel Betak)
 *
 *  ImGui Debug Visualizer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ImGui Debug Visualizer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ImGui Debug Visualizer. If not, see <http://www.gnu.org/licenses/>.
 *
 *  Author: buzzcola3 (Samuel Betak)
 *  Email: buzzcola3@gmail.com
 */

#include <cmath>

#include "debug_visualizer/debug_visualizer.h"

namespace {
constexpr float kTargetFrameTimeMs = 16.67f;

dbgvis::GraphConfig MakeGraphConfig(size_t max_samples, bool auto_scale = true) {
    dbgvis::GraphConfig config;
    config.max_samples = max_samples;
    config.auto_scale = auto_scale;
    return config;
}

}  // namespace

int main() {
    dbgvis::DebugVisualizerApp app(true);

    float phase = 0.0f;
    const auto frame_graph_config = MakeGraphConfig(240);
    const auto waveform_config = MakeGraphConfig(360);

    return app.run([&](dbgvis::DebugVisualizerApp& app_ctx, float elapsed_time, float delta_time) {
        phase += delta_time;

        dbgvis::DebugVisualizer& debug_tile = app_ctx.Tiles["Main"];
        debug_tile.set_window_title("Debug Window");

        dbgvis::DebugVisualizer::Tab& overview = debug_tile.tabs["Overview"];
        dbgvis::DebugVisualizer::Tab& graphs = debug_tile.tabs["Graphs"];
        dbgvis::DebugVisualizer::Tab& systems = debug_tile.tabs["Systems"];

        if (!graphs.Graph.contains("Frame Time (ms)")) {
            graphs.addGraph("Frame Time (ms)", frame_graph_config);
        }
        if (!graphs.Graph.contains("Sine Wave")) {
            graphs.addGraph("Sine Wave", waveform_config);
        }

        const float frame_time_ms = delta_time * 1000.0f;
        const float fps = delta_time > 0.0f ? 1.0f / delta_time : 0.0f;
        const float budget_used = frame_time_ms / kTargetFrameTimeMs;
        const float cpu_load = 0.35f + 0.20f * std::sin(phase * 0.5f);
        const bool is_paused = std::sin(phase * 0.17f) > 0.8f;

        overview.update_value("Timing/FPS", fps);
        overview.update_value("Timing/Frame time (ms)", frame_time_ms);
        overview.update_value("Timing/Budget used", budget_used);
        overview.update_value("Systems/CPU load", cpu_load * 100.0f);
        overview.update_value("Systems/Paused", is_paused);

        graphs.Graph["Frame Time (ms)"].x = frame_time_ms;
        graphs.Graph["Sine Wave"].x = std::sin(phase);

        systems.update_structure("Player", [&](dbgvis::StructureBuilder& builder) {
            builder.field("health", 92);
            builder.field("energy", 58.0f + 10.0f * std::sin(phase * 0.3f));

            auto position = builder.nested("position");
            position.field("x", std::cos(phase) * 6.0f);
            position.field("y", std::sin(phase) * 6.0f);
            position.field("z", 1.5f);

            auto inventory = builder.nested("inventory");
            inventory.field("potions", 2);
            inventory.field("keys", 1);
            inventory.field("gold", 128);
        });

        systems.update_structure("World", [&](dbgvis::StructureBuilder& builder) {
            builder.field("time_of_day", "Sunset");
            builder.field("weather", "Clear");
            builder.field("temperature_c", 22.0f + 2.0f * std::sin(phase * 0.1f));
        });
    });
}
