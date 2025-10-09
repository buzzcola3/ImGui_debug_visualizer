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

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace dbgvis {
namespace {
using UpdateFn = std::function<void(DebugVisualizerApp&)>;

struct ServiceState {
    std::mutex mutex;
    std::vector<UpdateFn> pending_updates;
    std::thread thread;
    DebugVisualizerAppOptions options;
    std::string tile_id = "Main";
    std::atomic<bool> thread_started{false};
    std::atomic<bool> running{false};
    std::atomic<bool> stop_requested{false};

    ServiceState() = default;

    ~ServiceState() {
        stop_requested.store(true, std::memory_order_release);
        if (thread_started.load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(mutex);
            pending_updates.emplace_back([](DebugVisualizerApp& app) {
                app.request_close();
            });
        }
        if (thread.joinable()) {
            thread.join();
        }
    }
};

ServiceState& GetState() {
    static ServiceState state;
    return state;
}

void FlushUpdates(DebugVisualizerApp& app) {
    ServiceState& state = GetState();
    std::vector<UpdateFn> updates;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        updates.swap(state.pending_updates);
    }
    for (auto& update : updates) {
        update(app);
    }
}

void PrepareDefaultTab(DebugVisualizerApp& app) {
    ServiceState& state = GetState();
    DebugVisualizer& tile = app.Tiles[state.tile_id];
    tile.default_tab();
}

void ServiceThread() {
    ServiceState& state = GetState();

    DebugVisualizerAppOptions options;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        options = state.options;
    }

    DebugVisualizerApp app(std::move(options));
    state.running.store(true, std::memory_order_release);

    auto frame_callback = [&](DebugVisualizerApp& ctx, float /*elapsed*/, float /*delta*/) {
        if (state.stop_requested.load(std::memory_order_acquire)) {
            ctx.request_close();
        }
        PrepareDefaultTab(ctx);
        FlushUpdates(ctx);
    };

    app.run(frame_callback);

    state.running.store(false, std::memory_order_release);
    state.thread_started.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.pending_updates.clear();
    }
    state.stop_requested.store(false, std::memory_order_release);
}

void EnsureThreadStarted() {
    ServiceState& state = GetState();
    if (state.thread_started.load(std::memory_order_acquire)) {
        return;
    }

    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.thread_started.load(std::memory_order_acquire)) {
        return;
    }

    if (state.thread.joinable()) {
        state.thread.join();
    }

    state.stop_requested.store(false, std::memory_order_release);
    state.thread = std::thread(ServiceThread);
    state.thread_started.store(true, std::memory_order_release);
}

void EnqueueUpdate(UpdateFn update) {
    ServiceState& state = GetState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.pending_updates.emplace_back(std::move(update));
}

void PostUpdate(UpdateFn update) {
    EnsureThreadStarted();
    EnqueueUpdate(std::move(update));
}

DebugVisualizer::Tab& EnsureTab(DebugVisualizerApp& app, const std::string& tab_id) {
    ServiceState& state = GetState();
    DebugVisualizer& tile = app.Tiles[state.tile_id];
    return tile.tabs[tab_id];
}

}  // namespace

void StartBackgroundVisualizer() {
    StartBackgroundVisualizer(DebugVisualizerAppOptions{});
}

void StartBackgroundVisualizer(bool enable_docking) {
    DebugVisualizerAppOptions options;
    options.enable_docking = enable_docking;
    StartBackgroundVisualizer(std::move(options));
}

void StartBackgroundVisualizer(DebugVisualizerAppOptions options) {
    ServiceState& state = GetState();
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.options = std::move(options);
    }
    EnsureThreadStarted();
}

void ShutdownBackgroundVisualizer() {
    ServiceState& state = GetState();
    if (!state.thread_started.load(std::memory_order_acquire)) {
        return;
    }

    state.stop_requested.store(true, std::memory_order_release);
    EnqueueUpdate([](DebugVisualizerApp& app) {
        app.request_close();
    });

    if (state.thread.joinable()) {
        state.thread.join();
        state.thread = std::thread();
    }

    state.thread_started.store(false, std::memory_order_release);
    state.running.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.pending_updates.clear();
    }
}

bool IsBackgroundVisualizerRunning() {
    ServiceState& state = GetState();
    return state.running.load(std::memory_order_acquire);
}

void value(const std::string& tab_id, const std::string& key, int value) {
    PostUpdate([tab_id, key, value](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_value(key, value);
    });
}

void value(const std::string& tab_id, const std::string& key, int64_t value) {
    PostUpdate([tab_id, key, value](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_value(key, value);
    });
}

void value(const std::string& tab_id, const std::string& key, float value) {
    PostUpdate([tab_id, key, value](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_value(key, value);
    });
}

void value(const std::string& tab_id, const std::string& key, double value) {
    PostUpdate([tab_id, key, value](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_value(key, value);
    });
}

void value(const std::string& tab_id, const std::string& key, bool value) {
    PostUpdate([tab_id, key, value](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_value(key, value);
    });
}

void value(const std::string& tab_id, const std::string& key, std::string value) {
    PostUpdate([tab_id, key, value = std::move(value)](DebugVisualizerApp& app) mutable {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_value(key, std::move(value));
    });
}

void value(const std::string& tab_id, const std::string& key, const char* value) {
    PostUpdate([tab_id, key, value = std::string(value)](DebugVisualizerApp& app) mutable {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_value(key, std::move(value));
    });
}

void graph_sample(const std::string& tab_id, const std::string& key, float sample, const GraphConfig& config) {
    PostUpdate([tab_id, key, sample, config](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        DebugVisualizer::Graph& graph = tab.Graph.add(key, config);
        graph.x = sample;
    });
}

void graph_samples(const std::string& tab_id,
                   const std::string& key,
                   const std::vector<float>& samples,
                   const GraphConfig& config) {
    PostUpdate([tab_id, key, samples = std::vector<float>(samples), config](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.add_graph_samples(key, samples, config);
    });
}

void structure(const std::string& tab_id,
               const std::string& key,
               const std::function<void(StructureBuilder&)>& builder) {
    PostUpdate([tab_id, key, builder](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.update_structure(key, builder);
    });
}

void clear_tab(const std::string& tab_id) {
    PostUpdate([tab_id](DebugVisualizerApp& app) {
        DebugVisualizer::Tab& tab = EnsureTab(app, tab_id);
        tab.clear();
    });
}

void set_window_title(std::string title) {
    PostUpdate([title = std::move(title)](DebugVisualizerApp& app) mutable {
        ServiceState& state = GetState();
        DebugVisualizer& tile = app.Tiles[state.tile_id];
        tile.set_window_title(title);
    });
}

void show_window(bool visible) {
    PostUpdate([visible](DebugVisualizerApp& app) {
        ServiceState& state = GetState();
        DebugVisualizer& tile = app.Tiles[state.tile_id];
        tile.set_visible(visible);
    });
}

}  // namespace dbgvis
