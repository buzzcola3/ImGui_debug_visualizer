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

#include "debug_visualizer/debug_visualizer.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <thread>

namespace {
constexpr float kTargetFrameTimeMs = 16.67f;
constexpr int kMaxCounterValue = 127;
constexpr int kCounterModulo = kMaxCounterValue + 1;
}  // namespace

int main() {
    dbgvis::StartBackgroundVisualizer();
    dbgvis::set_window_title("Debug Window");

    while (!dbgvis::IsBackgroundVisualizerRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    int counter = 0;
    uint64_t wraps = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto last_frame_time = start_time;

    while (dbgvis::IsBackgroundVisualizerRunning()) {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsed = now - start_time;
        const std::chrono::duration<float> delta = now - last_frame_time;
        last_frame_time = now;

        const float delta_seconds = std::max(delta.count(), 1.0f / 240.0f);
        const float frame_time_ms = delta_seconds * 1000.0f;
        const float fps = delta_seconds > 0.0f ? 1.0f / delta_seconds : 0.0f;

        const int previous_counter = counter;
        counter = (counter + 1) % kCounterModulo;
        if (counter == 0) {
            ++wraps;
        }

        const int ticks_this_frame = counter >= previous_counter ? counter - previous_counter : (counter + kCounterModulo) - previous_counter;
        const float rate_per_second = delta_seconds > 0.0f ? ticks_this_frame / delta_seconds : 0.0f;
        const float budget_used = frame_time_ms / kTargetFrameTimeMs;
        const int remaining = counter == 0 ? kMaxCounterValue : kMaxCounterValue - counter;

        dbgvis::value("Telemetry", "Counter/Current value", counter);
        dbgvis::value("Telemetry", "Counter/Wraps", static_cast<int64_t>(wraps));
        dbgvis::value("Telemetry", "Counter/Ticks this frame", ticks_this_frame);
        dbgvis::value("Telemetry", "Counter/Rate per second", rate_per_second);
        dbgvis::value("Telemetry", "Counter/Remaining to wrap", remaining);
        dbgvis::value("Telemetry", "Timing/Elapsed (s)", elapsed.count());
        dbgvis::value("Telemetry", "Timing/FPS", fps);
        dbgvis::value("Telemetry", "Timing/Frame time (ms)", frame_time_ms);
        dbgvis::value("Telemetry", "Timing/Budget used", budget_used);

        dbgvis::graph_sample("Telemetry", "Counter Value", static_cast<float>(counter));

        dbgvis::structure("Telemetry", "Counter/Progress", [counter, remaining, wraps](dbgvis::StructureBuilder& builder) {
            builder.field("current", counter);
            builder.field("wraps", static_cast<int64_t>(wraps));
            builder.field("remaining_to_wrap", remaining);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    dbgvis::ShutdownBackgroundVisualizer();
    return 0;
}
