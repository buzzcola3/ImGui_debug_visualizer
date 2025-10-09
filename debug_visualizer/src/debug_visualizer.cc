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
#include <memory>
#include <sstream>
#include <utility>

#include <imgui/imgui.h>

namespace dbgvis {
namespace {
template <class... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

std::string ScalarToString(const ScalarValue& value) {
    return std::visit(
        Overload{
            [](int64_t v) {
                return std::to_string(v);
            },
            [](double v) {
                std::ostringstream oss;
                oss.setf(std::ios::fixed, std::ios::floatfield);
                oss.precision(3);
                oss << v;
                return oss.str();
            },
            [](bool v) {
                return v ? std::string("true") : std::string("false");
            },
            [](const std::string& v) {
                return v;
            }},
        value);
}
}  // namespace

StructureBuilder::StructureBuilder(std::vector<StructureNode>* nodes) : nodes_(nodes) {}

void StructureBuilder::field(const std::string& label, int value) {
    field(label, static_cast<int64_t>(value));
}

void StructureBuilder::field(const std::string& label, int64_t value) {
    if (!nodes_) {
        return;
    }
    nodes_->push_back(StructureNode{label, ScalarValue{value}, {}});
}

void StructureBuilder::field(const std::string& label, float value) {
    field(label, static_cast<double>(value));
}

void StructureBuilder::field(const std::string& label, double value) {
    if (!nodes_) {
        return;
    }
    nodes_->push_back(StructureNode{label, ScalarValue{value}, {}});
}

void StructureBuilder::field(const std::string& label, bool value) {
    if (!nodes_) {
        return;
    }
    nodes_->push_back(StructureNode{label, ScalarValue{value}, {}});
}

void StructureBuilder::field(const std::string& label, std::string value) {
    if (!nodes_) {
        return;
    }
    nodes_->push_back(StructureNode{label, ScalarValue{std::move(value)}, {}});
}

void StructureBuilder::field(const std::string& label, const char* value) {
    field(label, std::string(value));
}

StructureBuilder StructureBuilder::nested(const std::string& label) {
    if (!nodes_) {
        return StructureBuilder(nullptr);
    }
    nodes_->push_back(StructureNode{label, std::nullopt, {}});
    return StructureBuilder(&nodes_->back().children);
}

DebugVisualizer::Graph::AssignmentProxy::AssignmentProxy(Graph* owner) : owner_(owner) {}

DebugVisualizer::Graph::AssignmentProxy& DebugVisualizer::Graph::AssignmentProxy::operator=(float sample) {
    if (owner_) {
        owner_->push(sample);
    }
    return *this;
}

DebugVisualizer::Graph::AssignmentProxy::operator float() const {
    return owner_ ? owner_->latest() : 0.0f;
}

void DebugVisualizer::Graph::AssignmentProxy::reset(Graph* owner) {
    owner_ = owner;
}

DebugVisualizer::Graph::Graph() : x(this), config_(), samples_(), latest_sample_(0.0f) {}

DebugVisualizer::Graph::Graph(GraphConfig config) : x(this), config_(config), samples_(), latest_sample_(0.0f) {}

DebugVisualizer::Graph::Graph(const Graph& other)
        : x(this),
          config_(other.config_),
          samples_(other.samples_),
          latest_sample_(other.latest_sample_) {}

DebugVisualizer::Graph::Graph(Graph&& other) noexcept
        : x(this),
          config_(std::move(other.config_)),
          samples_(std::move(other.samples_)),
          latest_sample_(other.latest_sample_) {}

DebugVisualizer::Graph& DebugVisualizer::Graph::operator=(const Graph& other) {
    if (this == &other) {
        return *this;
    }
    config_ = other.config_;
    samples_ = other.samples_;
    latest_sample_ = other.latest_sample_;
    x.reset(this);
    return *this;
}

DebugVisualizer::Graph& DebugVisualizer::Graph::operator=(Graph&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    config_ = std::move(other.config_);
    samples_ = std::move(other.samples_);
    latest_sample_ = other.latest_sample_;
    x.reset(this);
    return *this;
}

DebugVisualizer::Graph& DebugVisualizer::Graph::configure(const GraphConfig& config) {
    config_ = config;
    trim_to_config();
    return *this;
}

const GraphConfig& DebugVisualizer::Graph::config() const {
    return config_;
}

GraphConfig& DebugVisualizer::Graph::config() {
    return config_;
}

void DebugVisualizer::Graph::push(float sample) {
    latest_sample_ = sample;
    samples_.push_back(sample);
    trim_to_config();
}

void DebugVisualizer::Graph::add_samples(const std::vector<float>& samples) {
    for (float sample : samples) {
        push(sample);
    }
}

const std::vector<float>& DebugVisualizer::Graph::samples() const {
    return samples_;
}

std::vector<float>& DebugVisualizer::Graph::samples() {
    return samples_;
}

float DebugVisualizer::Graph::latest() const {
    return latest_sample_;
}

void DebugVisualizer::Graph::trim_to_config() {
    if (config_.max_samples == 0) {
        samples_.clear();
        return;
    }
    if (samples_.size() > config_.max_samples) {
        const size_t excess = samples_.size() - config_.max_samples;
        using difference_type = std::vector<float>::difference_type;
        samples_.erase(samples_.begin(), samples_.begin() + static_cast<difference_type>(excess));
    }
}

DebugVisualizer::Tab::Tab(std::string id, std::string title)
        : Graph(this),
          id_(std::move(id)),
          title_(title.empty() ? id_ : std::move(title)) {}

DebugVisualizer::TabCollection::TabCollection(DebugVisualizer* owner) : owner_(owner) {}

DebugVisualizer::Tab& DebugVisualizer::TabCollection::operator[](const std::string& id) {
    return owner_->ensure_tab(id, {});
}

DebugVisualizer::Tab& DebugVisualizer::TabCollection::add(const std::string& id, const std::string& title) {
    return owner_->ensure_tab(id, title);
}

bool DebugVisualizer::TabCollection::contains(const std::string& id) const {
    return owner_ && owner_->find_tab_internal(id) != nullptr;
}

size_t DebugVisualizer::TabCollection::size() const {
    return owner_ ? owner_->tabs_.size() : 0U;
}

std::vector<std::string> DebugVisualizer::TabCollection::ids() const {
    if (!owner_) {
        return {};
    }
    return owner_->tab_ids();
}

DebugVisualizer::Tab::GraphCollection::GraphCollection(Tab* owner) : owner_(owner) {}

DebugVisualizer::Graph& DebugVisualizer::Tab::GraphCollection::operator[](const std::string& key) {
    auto it = owner_->graphs_.find(key);
    if (it == owner_->graphs_.end()) {
        it = owner_->graphs_.try_emplace(key, DebugVisualizer::Graph()).first;
    }
    return it->second;
}

DebugVisualizer::Graph& DebugVisualizer::Tab::GraphCollection::add(const std::string& key,
                                                                   const GraphConfig& config) {
    return owner_->ensure_graph(key, config);
}

bool DebugVisualizer::Tab::GraphCollection::contains(const std::string& key) const {
    return owner_ && owner_->graphs_.find(key) != owner_->graphs_.end();
}

size_t DebugVisualizer::Tab::GraphCollection::size() const {
    return owner_ ? owner_->graphs_.size() : 0U;
}

auto DebugVisualizer::Tab::GraphCollection::begin() {
    return owner_->graphs_.begin();
}

auto DebugVisualizer::Tab::GraphCollection::end() {
    return owner_->graphs_.end();
}

auto DebugVisualizer::Tab::GraphCollection::begin() const {
    return owner_->graphs_.cbegin();
}

auto DebugVisualizer::Tab::GraphCollection::end() const {
    return owner_->graphs_.cend();
}

DebugVisualizer::Graph& DebugVisualizer::Tab::addGraph(const std::string& key, const GraphConfig& config) {
    return ensure_graph(key, config);
}

const std::string& DebugVisualizer::Tab::id() const {
    return id_;
}

const std::string& DebugVisualizer::Tab::title() const {
    return title_;
}

void DebugVisualizer::Tab::set_title(std::string title) {
    if (!title.empty()) {
        title_ = std::move(title);
    }
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_value(const std::string& key, int value) {
    return update_value(key, static_cast<int64_t>(value));
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_value(const std::string& key, int64_t value) {
    scalars_[key] = value;
    return *this;
}

DebugVisualizer::Graph& DebugVisualizer::Tab::ensure_graph(const std::string& key, const GraphConfig& config) {
    auto [it, inserted] = graphs_.try_emplace(key, DebugVisualizer::Graph(config));
    if (!inserted) {
        const GraphConfig& existing = it->second.config();
        if (existing.max_samples != config.max_samples || existing.auto_scale != config.auto_scale ||
            existing.manual_min != config.manual_min || existing.manual_max != config.manual_max) {
            it->second.configure(config);
        }
    }
    return it->second;
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_value(const std::string& key, float value) {
    return update_value(key, static_cast<double>(value));
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_value(const std::string& key, double value) {
    scalars_[key] = value;
    return *this;
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_value(const std::string& key, bool value) {
    scalars_[key] = value;
    return *this;
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_value(const std::string& key, std::string value) {
    scalars_[key] = std::move(value);
    return *this;
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_value(const std::string& key, const char* value) {
    scalars_[key] = std::string(value);
    return *this;
}

DebugVisualizer::Tab& DebugVisualizer::Tab::push_graph_sample(const std::string& key, float sample, const GraphConfig& config) {
    DebugVisualizer::Graph& graph = ensure_graph(key, config);
    graph.push(sample);
    return *this;
}

DebugVisualizer::Tab& DebugVisualizer::Tab::add_graph_samples(const std::string& key,
                                                              const std::vector<float>& samples,
                                                              const GraphConfig& config) {
    DebugVisualizer::Graph& graph = ensure_graph(key, config);
    graph.add_samples(samples);
    return *this;
}

DebugVisualizer::Tab& DebugVisualizer::Tab::update_structure(const std::string& key,
                                                             const std::function<void(StructureBuilder&)>& builder) {
    StructureEntry& entry = structures_[key];
    entry.root.label = key;
    entry.root.value.reset();
    entry.root.children.clear();

    StructureBuilder root_builder(&entry.root.children);
    if (builder) {
        builder(root_builder);
    }
    entry.has_content = !entry.root.children.empty();
    return *this;
}

std::optional<ScalarValue> DebugVisualizer::Tab::get_scalar(const std::string& key) const {
    auto it = scalars_.find(key);
    if (it == scalars_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<float> DebugVisualizer::Tab::get_graph_samples(const std::string& key) const {
    auto it = graphs_.find(key);
    if (it == graphs_.end()) {
        return {};
    }
    return it->second.samples();
}

std::optional<StructureNode> DebugVisualizer::Tab::get_structure(const std::string& key) const {
    auto it = structures_.find(key);
    if (it == structures_.end() || !it->second.has_content) {
        return std::nullopt;
    }
    return it->second.root;
}

void DebugVisualizer::Tab::clear() {
    scalars_.clear();
    graphs_.clear();
    structures_.clear();
}

DebugVisualizer::DebugVisualizer()
                : tabs(this),
                    window_title_("Debug Window"),
          window_flags_(ImGuiWindowFlags_None),
          visible_(true),
          default_tab_id_("overview") {
    add_tab(default_tab_id_);
}

void DebugVisualizer::set_window_title(std::string title) {
    window_title_ = std::move(title);
}

void DebugVisualizer::set_window_flags(int flags) {
    window_flags_ = flags;
}

const std::string& DebugVisualizer::window_title() const {
    return window_title_;
}

void DebugVisualizer::set_visible(bool visible) {
    visible_ = visible;
}

bool DebugVisualizer::is_visible() const {
    return visible_;
}

DebugVisualizer::Tab& DebugVisualizer::add_tab(const std::string& id) {
    return add_tab(id, id);
}

DebugVisualizer::Tab& DebugVisualizer::add_tab(const std::string& id, const std::string& title) {
    return ensure_tab(id, title);
}

DebugVisualizer::Tab* DebugVisualizer::find_tab(const std::string& id) {
    return const_cast<Tab*>(find_tab_internal(id));
}

const DebugVisualizer::Tab* DebugVisualizer::find_tab(const std::string& id) const {
    return find_tab_internal(id);
}

bool DebugVisualizer::remove_tab(const std::string& id) {
    if (id == default_tab_id_) {
        return false;
    }
    auto it = std::find_if(tabs_.begin(), tabs_.end(), [&](const std::unique_ptr<Tab>& tab) {
        return tab->id() == id;
    });
    if (it == tabs_.end()) {
        return false;
    }
    tabs_.erase(it);
    return true;
}

std::vector<std::string> DebugVisualizer::tab_ids() const {
    std::vector<std::string> ids;
    ids.reserve(tabs_.size());
    for (const auto& tab : tabs_) {
        ids.push_back(tab->id());
    }
    return ids;
}

DebugVisualizer::Tab& DebugVisualizer::default_tab() {
    return ensure_tab(default_tab_id_, {});
}

const DebugVisualizer::Tab& DebugVisualizer::default_tab() const {
    return const_cast<DebugVisualizer*>(this)->default_tab();
}

DebugVisualizer& DebugVisualizer::window_tile(const std::string& id) {
    return window_tile(id, id);
}

DebugVisualizer& DebugVisualizer::window_tile(const std::string& id, const std::string& title) {
    if (auto* existing = find_window_tile(id)) {
        if (!title.empty() && existing->window_title() != title) {
            existing->set_window_title(title);
        }
        return *existing;
    }

    WindowTile tile;
    tile.id = id;
    tile.visualizer = std::make_unique<DebugVisualizer>();
    tile.visualizer->set_window_title(title.empty() ? id : title);
    tile.visualizer->set_visible(true);

    DebugVisualizer& reference = *tile.visualizer;
    window_tiles_.push_back(std::move(tile));
    return reference;
}

DebugVisualizer* DebugVisualizer::find_window_tile(const std::string& id) {
    auto it = std::find_if(window_tiles_.begin(), window_tiles_.end(), [&](const WindowTile& entry) {
        return entry.id == id;
    });
    if (it == window_tiles_.end()) {
        return nullptr;
    }
    return it->visualizer.get();
}

const DebugVisualizer* DebugVisualizer::find_window_tile(const std::string& id) const {
    auto it = std::find_if(window_tiles_.begin(), window_tiles_.end(), [&](const WindowTile& entry) {
        return entry.id == id;
    });
    if (it == window_tiles_.end()) {
        return nullptr;
    }
    return it->visualizer.get();
}

bool DebugVisualizer::remove_window_tile(const std::string& id) {
    auto it = std::find_if(window_tiles_.begin(), window_tiles_.end(), [&](const WindowTile& entry) {
        return entry.id == id;
    });
    if (it == window_tiles_.end()) {
        return false;
    }
    window_tiles_.erase(it);
    return true;
}

std::vector<std::string> DebugVisualizer::window_tile_ids() const {
    std::vector<std::string> ids;
    ids.reserve(window_tiles_.size());
    for (const auto& entry : window_tiles_) {
        ids.push_back(entry.id);
    }
    return ids;
}

void DebugVisualizer::render() {
    if (visible_) {
        bool keep_open = true;
        const std::string title = window_title_.empty() ? std::string("Debug Visualizer") : window_title_;
        if (ImGui::Begin(title.c_str(), &keep_open, window_flags_)) {
            if (tabs_.empty()) {
                ImGui::TextUnformatted("No tabs added yet. Call add_tab() to begin.");
            } else if (ImGui::BeginTabBar("DebugVisualizerTabs")) {
                for (auto& tab_ptr : tabs_) {
                    Tab& tab = *tab_ptr;
                    if (ImGui::BeginTabItem(tab.title().c_str())) {
                        render_tab_contents(tab);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
        visible_ = keep_open;
    }

    for (auto& entry : window_tiles_) {
        if (entry.visualizer) {
            entry.visualizer->render();
        }
    }
}

void DebugVisualizer::render_tab_contents(const Tab& tab) const {
    bool rendered_any = false;

    if (!tab.scalars_.empty()) {
        ImGui::SeparatorText("Variables");
        for (const auto& [key, value] : tab.scalars_) {
            render_scalar(key, value);
        }
        rendered_any = true;
    }

    if (!tab.graphs_.empty()) {
        if (rendered_any) {
            ImGui::Spacing();
        }
        ImGui::SeparatorText("Graphs");
        for (const auto& [key, graph] : tab.graphs_) {
            render_graph(key, graph);
        }
        rendered_any = true;
    }

    bool structures_rendered = false;
    if (!tab.structures_.empty()) {
        if (rendered_any) {
            ImGui::Spacing();
        }
        ImGui::SeparatorText("Structures");
        for (const auto& [key, entry] : tab.structures_) {
            if (!entry.has_content) {
                continue;
            }
            if (ImGui::TreeNode(key.c_str())) {
                for (const auto& child : entry.root.children) {
                    render_structure_node(child);
                }
                ImGui::TreePop();
                structures_rendered = true;
            }
        }
        rendered_any = rendered_any || structures_rendered;
    }

    if (!rendered_any) {
        ImGui::TextUnformatted("This tab has no data yet.");
    }
}

void DebugVisualizer::render_scalar(const std::string& key, const ScalarValue& value) const {
    std::visit(
        Overload{
            [&](int64_t v) {
                ImGui::Text("%s: %lld", key.c_str(), static_cast<long long>(v));
            },
            [&](double v) {
                ImGui::Text("%s: %.3f", key.c_str(), v);
            },
            [&](bool v) {
                ImGui::Text("%s: %s", key.c_str(), v ? "true" : "false");
            },
            [&](const std::string& v) {
                ImGui::Text("%s: %s", key.c_str(), v.c_str());
            }},
        value);
}

void DebugVisualizer::render_graph(const std::string& key, const DebugVisualizer::Graph& graph) const {
    const auto& samples = graph.samples();
    if (samples.empty()) {
        ImGui::Text("%s: <no samples>", key.c_str());
        return;
    }

    const GraphConfig& cfg = graph.config();

    float min_value = cfg.manual_min;
    float max_value = cfg.manual_max;
    if (cfg.auto_scale) {
        auto [min_it, max_it] = std::minmax_element(samples.begin(), samples.end());
        min_value = *min_it;
        max_value = *max_it;
        if (min_value == max_value) {
            min_value -= 1.0f;
            max_value += 1.0f;
        }
    }

    ImGui::PlotLines(
        key.c_str(),
        samples.data(),
        static_cast<int>(samples.size()),
        0,
        nullptr,
        min_value,
        max_value,
        ImVec2(0.0f, 80.0f));
}

void DebugVisualizer::render_structure_node(const StructureNode& node) const {
    if (!node.children.empty()) {
        if (ImGui::TreeNode(node.label.c_str())) {
            if (node.value.has_value()) {
                ImGui::Text("%s", ScalarToString(*node.value).c_str());
            }
            for (const auto& child : node.children) {
                render_structure_node(child);
            }
            ImGui::TreePop();
        }
        return;
    }

    if (node.value.has_value()) {
        ImGui::Text("%s: %s", node.label.c_str(), ScalarToString(*node.value).c_str());
    } else {
        ImGui::Text("%s", node.label.c_str());
    }
}

void DebugVisualizer::clear() {
    for (auto& tab : tabs_) {
        tab->clear();
    }
    for (auto& entry : window_tiles_) {
        if (entry.visualizer) {
            entry.visualizer->clear();
        }
    }
}

void DebugVisualizer::update_value(const std::string& key, int value) {
    default_tab().update_value(key, value);
}

void DebugVisualizer::update_value(const std::string& key, int64_t value) {
    default_tab().update_value(key, value);
}

void DebugVisualizer::update_value(const std::string& key, float value) {
    default_tab().update_value(key, value);
}

void DebugVisualizer::update_value(const std::string& key, double value) {
    default_tab().update_value(key, value);
}

void DebugVisualizer::update_value(const std::string& key, bool value) {
    default_tab().update_value(key, value);
}

void DebugVisualizer::update_value(const std::string& key, std::string value) {
    default_tab().update_value(key, std::move(value));
}

void DebugVisualizer::update_value(const std::string& key, const char* value) {
    default_tab().update_value(key, value);
}

void DebugVisualizer::push_graph_sample(const std::string& key, float sample, const GraphConfig& config) {
    default_tab().push_graph_sample(key, sample, config);
}

void DebugVisualizer::add_graph_samples(const std::string& key,
                                         const std::vector<float>& samples,
                                         const GraphConfig& config) {
    default_tab().add_graph_samples(key, samples, config);
}

void DebugVisualizer::update_structure(const std::string& key,
                                       const std::function<void(StructureBuilder&)>& builder) {
    default_tab().update_structure(key, builder);
}

std::optional<ScalarValue> DebugVisualizer::get_scalar(const std::string& key) const {
    if (const Tab* tab = find_tab_internal(default_tab_id_)) {
        return tab->get_scalar(key);
    }
    return std::nullopt;
}

std::vector<float> DebugVisualizer::get_graph_samples(const std::string& key) const {
    if (const Tab* tab = find_tab_internal(default_tab_id_)) {
        return tab->get_graph_samples(key);
    }
    return {};
}

std::optional<StructureNode> DebugVisualizer::get_structure(const std::string& key) const {
    if (const Tab* tab = find_tab_internal(default_tab_id_)) {
        return tab->get_structure(key);
    }
    return std::nullopt;
}

DebugVisualizer::Tab& DebugVisualizer::ensure_tab(const std::string& id, const std::string& title) {
    if (Tab* existing = const_cast<Tab*>(find_tab_internal(id))) {
        if (!title.empty()) {
            existing->set_title(title);
        }
        return *existing;
    }

    auto tab = std::make_unique<Tab>(id, title.empty() ? id : title);
    Tab& ref = *tab;
    tabs_.push_back(std::move(tab));
    return ref;
}

const DebugVisualizer::Tab* DebugVisualizer::find_tab_internal(const std::string& id) const {
    auto it = std::find_if(tabs_.begin(), tabs_.end(), [&](const std::unique_ptr<Tab>& tab) {
        return tab->id() == id;
    });
    if (it == tabs_.end()) {
        return nullptr;
    }
    return it->get();
}

}  // namespace dbgvis
