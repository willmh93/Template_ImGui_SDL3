#pragma once

/// SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_touch.h>

/// emscripten
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

/// Windows
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

/// OpenGL
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <filesystem>
#include <functional>

struct GLCaps
{
    int max_texture_size = 0;
    int max_renderbuffer_size = 0;
    int max_samples = 0;
    int max_viewport_w = 0;
    int max_viewport_h = 0;
};

class PlatformManager
{
    static PlatformManager* singleton;

    SDL_Window* window = nullptr;

    double css_w = 0, css_h = 0;
    int gl_w = 0, gl_h = 0;
    int win_w = 0, win_h = 0;
    int fb_w = 0, fb_h = 0;
    float win_dpr = 1.0f;

    bool is_mobile_device = false;

    GLuint offscreen_fbo = 0;
    GLuint offscreen_color = 0;
    GLuint offscreen_depth = 0;
    int    offscreen_w = 0;
    int    offscreen_h = 0;

    GLCaps gl_caps;
    GLCaps query_gl_caps();

public:

    static constexpr PlatformManager* instance()
    {
        return singleton;
    }

    PlatformManager(SDL_Window* _window)
    {
        singleton = this;
        window = _window;
    }

    // Required function calls
    void init();
    void update();
    void resized();

    [[nodiscard]] SDL_Window* sdlWindow() const { return window; }

    [[nodiscard]] int glWidth()      const { return gl_w; }
    [[nodiscard]] int glHeight()     const { return gl_h; }
    [[nodiscard]] int fboWidth()     const { return fb_w; }
    [[nodiscard]] int fboHeight()    const { return fb_h; }
    [[nodiscard]] int windowWidth()  const { return win_w; }
    [[nodiscard]] int windowHeight() const { return win_h; }

    [[nodiscard]] ImVec2 glSize()     const { return ImVec2((float)gl_w, (float)gl_h); }
    [[nodiscard]] ImVec2 windowSize() const { return ImVec2((float)win_w, (float)win_h); }

    // Device Info
    [[nodiscard]] float dpr() const { return win_dpr; }

    // what the app should render to (offscreen, or direct)
    [[nodiscard]] ImVec2 fboSize() const
    {
        return { (float)fb_w, (float)fb_h };
    }

    GLCaps glCaps() const { return gl_caps; }
    bool glValidTextureSize(int internal_w, int internal_h);

    [[nodiscard]] bool deviceVertical();
    void deviceOrientation(int* orientation_angle, int* orientation_index = nullptr);
    bool deviceOrientationChanged(std::function<void(int, int)> onChanged);

    // Platform detection
    [[nodiscard]] bool isMobile() const;
    [[nodiscard]] bool isDesktopNative() const;
    [[nodiscard]] bool isDesktopBrowser() const;

    // Scale
    [[nodiscard]] float fontScale() const;
    [[nodiscard]] float thumbScale(float extra_mobile_mult = 1.0f) const;

    // Layout helpers
    [[nodiscard]] float lineHeight() const;
    [[nodiscard]] float inputHeight() const;
    [[nodiscard]] float maxCharRows() const;
    [[nodiscard]] float maxCharCols() const;

    // File system helpers
    [[nodiscard]] std::filesystem::path executableDir() const;
    [[nodiscard]] std::filesystem::path resourceDir() const;
    [[nodiscard]] std::string path(std::string_view virtual_path) const;
};

[[nodiscard]] constexpr PlatformManager* platform()
{
    return PlatformManager::instance();
}