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

#include <cstdio>
#include <utility>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#ifndef IMGUI_IMPL_OPENGL_LOADER_GLEW
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#endif
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

namespace dbgvis {
namespace {
void GlfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW Error (%d): %s\n", error, description);
}

void GlfwKeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}
}  // namespace

struct DebugVisualizerApp::Impl {
    explicit Impl(DebugVisualizerAppOptions opts) : options(std::move(opts)) {
        visualizer.set_window_title(options.window_title);
    }

    ~Impl() {
        Shutdown();
    }

    bool Initialize();
    void Shutdown();
    void ApplyWindowTitle();

    DebugVisualizerAppOptions options;
    GLFWwindow* window = nullptr;
    bool glfw_initialized = false;
    bool imgui_initialized = false;
    double last_time = 0.0;
    DebugVisualizer visualizer;
    std::string applied_window_title;
};

bool DebugVisualizerApp::Impl::Initialize() {
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }
    glfw_initialized = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, options.gl_context_major_version);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, options.gl_context_minor_version);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(options.width, options.height, options.window_title.c_str(), nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        Shutdown();
        return false;
    }

    glfwSetKeyCallback(window, GlfwKeyCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(options.vsync ? 1 : 0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::fprintf(stderr, "Failed to initialize GLEW\n");
        Shutdown();
        return false;
    }

    glGetError();  // Clear spurious error from GLEW initialization.

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    if (options.enable_keyboard_navigation) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }
    if (options.enable_docking) {
#ifdef ImGuiConfigFlags_DockingEnable
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    }

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(options.glsl_version.c_str());
    imgui_initialized = true;

    last_time = glfwGetTime();
    ApplyWindowTitle();
    return true;
}

void DebugVisualizerApp::Impl::Shutdown() {
    if (imgui_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized = false;
    }

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    if (glfw_initialized) {
        glfwTerminate();
        glfw_initialized = false;
    }
}

void DebugVisualizerApp::Impl::ApplyWindowTitle() {
    if (!window) {
        return;
    }
    const std::string& desired_title = visualizer.window_title().empty() ? options.window_title : visualizer.window_title();
    if (desired_title != applied_window_title) {
        glfwSetWindowTitle(window, desired_title.c_str());
        applied_window_title = desired_title;
    }
}

DebugVisualizerApp::TileCollection::TileCollection(DebugVisualizerApp* owner) : owner_(owner) {}

DebugVisualizer& DebugVisualizerApp::TileCollection::operator[](const std::string& id) {
    if (owner_) {
        return owner_->ensure_tile(id);
    }
    static DebugVisualizer placeholder;
    return placeholder;
}

DebugVisualizer& DebugVisualizerApp::TileCollection::add(const std::string& id, const std::string& title) {
    if (owner_) {
        return owner_->ensure_tile(id, title);
    }
    static DebugVisualizer placeholder;
    return placeholder;
}

bool DebugVisualizerApp::TileCollection::contains(const std::string& id) const {
    return owner_ && owner_->find_tile_internal(id) != nullptr;
}

size_t DebugVisualizerApp::TileCollection::size() const {
    if (!owner_ || !owner_->impl_) {
        return 0;
    }
    return owner_->impl_->visualizer.window_tile_ids().size();
}

std::vector<std::string> DebugVisualizerApp::TileCollection::ids() const {
    if (!owner_ || !owner_->impl_) {
        return {};
    }
    return owner_->impl_->visualizer.window_tile_ids();
}

void DebugVisualizerApp::TileCollection::reset_owner(DebugVisualizerApp* owner) {
        owner_ = owner;
}

namespace {
DebugVisualizerAppOptions OptionsWithDocking(bool enable_docking) {
        DebugVisualizerAppOptions options;
        options.enable_docking = enable_docking;
        return options;
}
}  // namespace

DebugVisualizerApp::DebugVisualizerApp()
                : DebugVisualizerApp(DebugVisualizerAppOptions{}) {}

DebugVisualizerApp::DebugVisualizerApp(bool enable_docking)
                : DebugVisualizerApp(OptionsWithDocking(enable_docking)) {}

DebugVisualizerApp::DebugVisualizerApp(DebugVisualizerAppOptions options)
                : Tiles(this),
                    impl_(std::make_unique<Impl>(std::move(options))) {}

DebugVisualizerApp::~DebugVisualizerApp() = default;

DebugVisualizerApp::DebugVisualizerApp(DebugVisualizerApp&& other) noexcept
        : Tiles(this),
          impl_(std::move(other.impl_)) {
    Tiles.reset_owner(this);
    other.Tiles.reset_owner(nullptr);
}

DebugVisualizerApp& DebugVisualizerApp::operator=(DebugVisualizerApp&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
        Tiles.reset_owner(this);
        other.Tiles.reset_owner(nullptr);
    }
    return *this;
}

int DebugVisualizerApp::run(const UpdateCallback& callback) {
    if (!impl_) {
        return 1;
    }

    if (!impl_->Initialize()) {
        impl_->Shutdown();
        return 1;
    }

    int exit_code = 0;
    while (!glfwWindowShouldClose(impl_->window)) {
        glfwPollEvents();

        double current_time = glfwGetTime();
        float delta_time = static_cast<float>(current_time - impl_->last_time);
        if (delta_time <= 0.0f) {
            delta_time = 1.0f / 60.0f;
        }
        impl_->last_time = current_time;

        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = delta_time;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (callback) {
            callback(*this, static_cast<float>(current_time), delta_time);
        }

        impl_->ApplyWindowTitle();

        impl_->visualizer.render();

        ImGui::Render();
        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(impl_->window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(impl_->window);
    }

    impl_->Shutdown();
    return exit_code;
}

DebugVisualizer& DebugVisualizerApp::addTile(const std::string& id, const std::string& title) {
    return ensure_tile(id, title);
}

DebugVisualizer* DebugVisualizerApp::findTile(const std::string& id) {
    return const_cast<DebugVisualizer*>(find_tile_internal(id));
}

const DebugVisualizer* DebugVisualizerApp::findTile(const std::string& id) const {
    return find_tile_internal(id);
}

DebugVisualizer& DebugVisualizerApp::ensure_tile(const std::string& id, const std::string& title) {
    if (!impl_) {
        static DebugVisualizer dummy;
        return dummy;
    }
    return impl_->visualizer.window_tile(id, title);
}

const DebugVisualizer* DebugVisualizerApp::find_tile_internal(const std::string& id) const {
    if (!impl_) {
        return nullptr;
    }
    return impl_->visualizer.find_window_tile(id);
}

}  // namespace dbgvis
