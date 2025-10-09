#include <cstdint>
#include <string>
#include <variant>

#include "debug_visualizer/debug_visualizer.h"

int main() {
    dbgvis::DebugVisualizer visualizer;

    auto& metrics_tab = visualizer.tabs["metrics"];
    metrics_tab.update_value("score", 42);
    metrics_tab.update_value("accuracy", 0.95);
    metrics_tab.update_value("alive", true);

    auto score = metrics_tab.get_scalar("score");
    if (!score || !std::holds_alternative<int64_t>(*score) || std::get<int64_t>(*score) != 42) {
        return 1;
    }

    dbgvis::GraphConfig graph_config;
    graph_config.max_samples = 4;
    graph_config.auto_scale = true;

    metrics_tab.addGraph("fps", graph_config);
    metrics_tab.Graph["fps"].x = 60.0f;
    metrics_tab.Graph["fps"].x = 58.0f;
    metrics_tab.Graph["fps"].x = 59.0f;
    metrics_tab.Graph["fps"].x = 61.0f;
    metrics_tab.Graph["fps"].x = 62.0f;

    auto samples = metrics_tab.get_graph_samples("fps");
    if (samples.size() != 4 || samples.front() != 58.0f || samples.back() != 62.0f) {
        return 2;
    }

    metrics_tab.update_structure("player", [](dbgvis::StructureBuilder& builder) {
        builder.field("health", 97);
        builder.field("mana", 44);
        auto position = builder.nested("position");
        position.field("x", 1.0f);
        position.field("y", 2.0f);
        position.field("z", 3.0f);
    });

    auto player = metrics_tab.get_structure("player");
    if (!player || player->children.size() != 3) {
        return 3;
    }

    auto& ai_tile = visualizer.window_tile("ai", "AI Debug");
    auto& ai_tab = ai_tile.tabs["state"];
    ai_tab.update_value("state", "searching");
    ai_tab.addGraph("threat");
    ai_tab.Graph["threat"].x = 0.5f;

    auto ai_state = ai_tab.get_scalar("state");
    if (!ai_state || !std::holds_alternative<std::string>(*ai_state) || std::get<std::string>(*ai_state) != "searching") {
        return 4;
    }

    auto tiles = visualizer.window_tile_ids();
    if (tiles.size() != 1 || tiles.front() != "ai") {
        return 5;
    }

    if (!visualizer.remove_window_tile("ai") || !visualizer.window_tile_ids().empty()) {
        return 6;
    }

    return 0;
}
