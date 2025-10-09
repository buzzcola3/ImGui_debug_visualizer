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

#pragma once

#include <functional>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace dbgvis {

using ScalarValue = std::variant<int64_t, double, bool, std::string>;

struct StructureNode {
    std::string label;
    std::optional<ScalarValue> value;
    std::vector<StructureNode> children;
};

class StructureBuilder {
public:
    explicit StructureBuilder(std::vector<StructureNode>* nodes);

    void field(const std::string& label, int value);
    void field(const std::string& label, int64_t value);
    void field(const std::string& label, float value);
    void field(const std::string& label, double value);
    void field(const std::string& label, bool value);
    void field(const std::string& label, std::string value);
    void field(const std::string& label, const char* value);

    StructureBuilder nested(const std::string& label);

private:
    std::vector<StructureNode>* nodes_;
};

struct GraphConfig {
    size_t max_samples = 240;
    bool auto_scale = true;
    float manual_min = 0.0f;
    float manual_max = 1.0f;
};

class DebugVisualizer {
public:
    class Graph {
    public:
        class AssignmentProxy {
        public:
            explicit AssignmentProxy(Graph* owner);

            AssignmentProxy& operator=(float sample);
            operator float() const;

            void reset(Graph* owner);

        private:
            Graph* owner_;
        };

        Graph();
        explicit Graph(GraphConfig config);
        Graph(const Graph& other);
        Graph(Graph&& other) noexcept;

        Graph& operator=(const Graph& other);
        Graph& operator=(Graph&& other) noexcept;

        AssignmentProxy x;

        Graph& configure(const GraphConfig& config);
        const GraphConfig& config() const;
        GraphConfig& config();

        void push(float sample);
        void add_samples(const std::vector<float>& samples);

        const std::vector<float>& samples() const;
        std::vector<float>& samples();
        float latest() const;

    private:
        friend class Tab;
        void trim_to_config();

        GraphConfig config_;
        std::vector<float> samples_;
        float latest_sample_;
    };

    struct StructureEntry {
        StructureNode root;
        bool has_content = false;
    };

    class Tab {
    public:
        class GraphCollection {
        public:
            explicit GraphCollection(Tab* owner);

            DebugVisualizer::Graph& operator[](const std::string& key);
            DebugVisualizer::Graph& add(const std::string& key, const GraphConfig& config = {});
            bool contains(const std::string& key) const;
            size_t size() const;

            auto begin();
            auto end();
            auto begin() const;
            auto end() const;

        private:
            Tab* owner_;
        };

        Tab(std::string id, std::string title);

        const std::string& id() const;
        const std::string& title() const;
        void set_title(std::string title);

    DebugVisualizer::Graph& addGraph(const std::string& key, const GraphConfig& config = {});
        GraphCollection Graph;

        Tab& update_value(const std::string& key, int value);
        Tab& update_value(const std::string& key, int64_t value);
        Tab& update_value(const std::string& key, float value);
        Tab& update_value(const std::string& key, double value);
        Tab& update_value(const std::string& key, bool value);
        Tab& update_value(const std::string& key, std::string value);
        Tab& update_value(const std::string& key, const char* value);

        Tab& push_graph_sample(const std::string& key, float sample, const GraphConfig& config = {});
        Tab& add_graph_samples(const std::string& key, const std::vector<float>& samples, const GraphConfig& config = {});

        Tab& update_structure(const std::string& key, const std::function<void(StructureBuilder&)>& builder);

        std::optional<ScalarValue> get_scalar(const std::string& key) const;
        std::vector<float> get_graph_samples(const std::string& key) const;
        std::optional<StructureNode> get_structure(const std::string& key) const;

        void clear();

    private:
        friend class DebugVisualizer;
        friend class GraphCollection;
    DebugVisualizer::Graph& ensure_graph(const std::string& key, const GraphConfig& config = {});

        std::string id_;
        std::string title_;
        std::map<std::string, ScalarValue> scalars_;
        std::map<std::string, DebugVisualizer::Graph> graphs_;
        std::map<std::string, StructureEntry> structures_;
    };

    class TabCollection {
    public:
        explicit TabCollection(DebugVisualizer* owner);

        Tab& operator[](const std::string& id);
        Tab& add(const std::string& id, const std::string& title = {});
        bool contains(const std::string& id) const;
        size_t size() const;
        std::vector<std::string> ids() const;

    private:
        DebugVisualizer* owner_;
    };

    DebugVisualizer();

    TabCollection tabs;

    void set_window_title(std::string title);
    void set_window_flags(int flags);

    const std::string& window_title() const;

    void set_visible(bool visible);
    bool is_visible() const;

    Tab& add_tab(const std::string& id);
    Tab& add_tab(const std::string& id, const std::string& title);
    Tab* find_tab(const std::string& id);
    const Tab* find_tab(const std::string& id) const;
    bool remove_tab(const std::string& id);
    std::vector<std::string> tab_ids() const;
    Tab& default_tab();
    const Tab& default_tab() const;

    DebugVisualizer& window_tile(const std::string& id);
    DebugVisualizer& window_tile(const std::string& id, const std::string& title);
    DebugVisualizer* find_window_tile(const std::string& id);
    const DebugVisualizer* find_window_tile(const std::string& id) const;
    bool remove_window_tile(const std::string& id);
    std::vector<std::string> window_tile_ids() const;

    void clear();

    void update_value(const std::string& key, int value);
    void update_value(const std::string& key, int64_t value);
    void update_value(const std::string& key, float value);
    void update_value(const std::string& key, double value);
    void update_value(const std::string& key, bool value);
    void update_value(const std::string& key, std::string value);
    void update_value(const std::string& key, const char* value);

    void push_graph_sample(const std::string& key, float sample, const GraphConfig& config = {});
    void add_graph_samples(const std::string& key, const std::vector<float>& samples, const GraphConfig& config = {});

    void update_structure(const std::string& key, const std::function<void(StructureBuilder&)>& builder);

    void render();

    std::optional<ScalarValue> get_scalar(const std::string& key) const;
    std::vector<float> get_graph_samples(const std::string& key) const;
    std::optional<StructureNode> get_structure(const std::string& key) const;

private:
    friend class TabCollection;
    Tab& ensure_tab(const std::string& id, const std::string& title);
    const Tab* find_tab_internal(const std::string& id) const;

    std::string window_title_;
    int window_flags_;
    bool visible_;
    std::string default_tab_id_;
    std::vector<std::unique_ptr<Tab>> tabs_;
    struct WindowTile {
        std::string id;
        std::unique_ptr<DebugVisualizer> visualizer;
    };
    std::vector<WindowTile> window_tiles_;

    void render_tab_contents(const Tab& tab) const;
    void render_scalar(const std::string& key, const ScalarValue& value) const;
    void render_graph(const std::string& key, const DebugVisualizer::Graph& graph) const;
    void render_structure_node(const StructureNode& node) const;
};

struct DebugVisualizerAppOptions {
    int width = 1280;
    int height = 720;
    std::string window_title = "Debug Window";
    int gl_context_major_version = 3;
    int gl_context_minor_version = 3;
    std::string glsl_version = "#version 330";
    bool enable_keyboard_navigation = true;
    bool enable_docking = false;
    bool vsync = true;
};

class DebugVisualizerApp {
public:
    using UpdateCallback = std::function<void(DebugVisualizerApp&, float elapsed_time, float delta_time)>;

    class TileCollection {
    public:
        explicit TileCollection(DebugVisualizerApp* owner);

        DebugVisualizer& operator[](const std::string& id);
        DebugVisualizer& add(const std::string& id, const std::string& title = {});
        bool contains(const std::string& id) const;
        size_t size() const;
        std::vector<std::string> ids() const;

        void reset_owner(DebugVisualizerApp* owner);

    private:
        DebugVisualizerApp* owner_;
    };

    DebugVisualizerApp();
    explicit DebugVisualizerApp(bool enable_docking);
    explicit DebugVisualizerApp(DebugVisualizerAppOptions options);
    ~DebugVisualizerApp();

    DebugVisualizerApp(DebugVisualizerApp&&) noexcept;
    DebugVisualizerApp& operator=(DebugVisualizerApp&&) noexcept;

    DebugVisualizerApp(const DebugVisualizerApp&) = delete;
    DebugVisualizerApp& operator=(const DebugVisualizerApp&) = delete;

    int run(const UpdateCallback& callback);

    DebugVisualizer& addTile(const std::string& id, const std::string& title = {});
    TileCollection Tiles;

    DebugVisualizer* findTile(const std::string& id);
    const DebugVisualizer* findTile(const std::string& id) const;

    void request_close();
    bool is_running() const;

private:
    friend class TileCollection;
    struct Impl;
    std::unique_ptr<Impl> impl_;

    DebugVisualizer& ensure_tile(const std::string& id, const std::string& title = {});
    const DebugVisualizer* find_tile_internal(const std::string& id) const;
};

int RunVisualizerApp(bool enable_docking, const DebugVisualizerApp::UpdateCallback& callback);
int RunVisualizerApp(DebugVisualizerAppOptions options, const DebugVisualizerApp::UpdateCallback& callback);

void StartBackgroundVisualizer();
void StartBackgroundVisualizer(bool enable_docking);
void StartBackgroundVisualizer(DebugVisualizerAppOptions options);
void ShutdownBackgroundVisualizer();
bool IsBackgroundVisualizerRunning();

void value(const std::string& tab_id, const std::string& key, int value);
void value(const std::string& tab_id, const std::string& key, int64_t value);
void value(const std::string& tab_id, const std::string& key, float value);
void value(const std::string& tab_id, const std::string& key, double value);
void value(const std::string& tab_id, const std::string& key, bool value);
void value(const std::string& tab_id, const std::string& key, std::string value);
void value(const std::string& tab_id, const std::string& key, const char* value);

inline void value(const std::string& key, int v) {
    value("Telemetry", key, v);
}

inline void value(const std::string& key, int64_t v) {
    value("Telemetry", key, v);
}

inline void value(const std::string& key, float v) {
    value("Telemetry", key, v);
}

inline void value(const std::string& key, double v) {
    value("Telemetry", key, v);
}

inline void value(const std::string& key, bool v) {
    value("Telemetry", key, v);
}

inline void value(const std::string& key, std::string v) {
    value("Telemetry", key, std::move(v));
}

inline void value(const std::string& key, const char* v) {
    value("Telemetry", key, v);
}

void graph_sample(const std::string& tab_id, const std::string& key, float sample, const GraphConfig& config = {});
void graph_samples(const std::string& tab_id, const std::string& key, const std::vector<float>& samples, const GraphConfig& config = {});

inline void graph_sample(const std::string& key, float sample, const GraphConfig& config = {}) {
    graph_sample("Telemetry", key, sample, config);
}

inline void graph_samples(const std::string& key, const std::vector<float>& samples, const GraphConfig& config = {}) {
    graph_samples("Telemetry", key, samples, config);
}

void structure(const std::string& tab_id, const std::string& key, const std::function<void(StructureBuilder&)>& builder);

inline void structure(const std::string& key, const std::function<void(StructureBuilder&)>& builder) {
    structure("Telemetry", key, builder);
}

void clear_tab(const std::string& tab_id);

inline void clear_tab() {
    clear_tab("Telemetry");
}

void set_window_title(std::string title);
void show_window(bool visible);

}  // namespace dbgvis
