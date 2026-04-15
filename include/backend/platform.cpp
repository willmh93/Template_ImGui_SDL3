//#include <bitloop/core/config.h>
//#include <bitloop/core/project.h>
#include "platform.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif


#include <imgui_internal.h>
#include <imgui_freetype.h>
#include <imgui_stdlib.h>



PlatformManager* PlatformManager::singleton = nullptr;

GLCaps PlatformManager::query_gl_caps()
{
    GLCaps caps{};

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.max_texture_size);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &caps.max_renderbuffer_size);

    GLint vp[2] = {};
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, vp);
    caps.max_viewport_w = vp[0];
    caps.max_viewport_h = vp[1];

    caps.max_samples = 0;
    if (glGetError() == GL_NO_ERROR) {
        glGetIntegerv(GL_MAX_SAMPLES, &caps.max_samples);
        glGetError(); // avoid stale error affecting later checks
    }

    return caps;
}

bool PlatformManager::glValidTextureSize(int internal_w, int internal_h)
{
    const int max_w = std::min(gl_caps.max_texture_size, gl_caps.max_renderbuffer_size);
    const int max_h = std::min(gl_caps.max_texture_size, gl_caps.max_renderbuffer_size);

    if (internal_w <= 0 || internal_h <= 0) return false;
    if (internal_w > max_w || internal_h > max_h) return false;
    if (internal_w > gl_caps.max_viewport_w || internal_h > gl_caps.max_viewport_h) return false;
    return true;
}

#ifdef __EMSCRIPTEN__
EM_JS(int, _is_mobile_device, (), {
  // 1. Prefer UA-Client-Hints when they exist (Chromium >= 89)
  if (navigator.userAgentData && navigator.userAgentData.mobile !== undefined) {
    return navigator.userAgentData.mobile ? 1 : 0;
  }

  // 2. Pointer/hover media-queries – quick and cheap
  if (window.matchMedia('(pointer: coarse)').matches &&
      !window.matchMedia('(hover: hover)').matches) {
    return 1;
  }

  // 3. Fallback to a light regex on the UA string
  return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini|Windows Phone/i
           .test(navigator.userAgent) ? 1 : 0;
});
#else
int _is_mobile_device()
{
    return 0;
}
#endif

void PlatformManager::init()
{
    is_mobile_device = _is_mobile_device();
    gl_caps = query_gl_caps();

    #ifdef __EMSCRIPTEN__
    EM_ASM({
        window.addEventListener('keydown', function(event)
        {
            if (event.ctrlKey && event.key == 'v')
                event.stopImmediatePropagation();
        }, true);
        });
    #endif
}

void PlatformManager::update()
{
    SDL_GetWindowSizeInPixels(window, &gl_w, &gl_h);
    SDL_GetWindowSize(window, &win_w, &win_h);
    win_dpr = SDL_GetWindowDisplayScale(window);
}

void PlatformManager::resized()
{
    #ifdef __EMSCRIPTEN__
    emscripten_get_element_css_size("#canvas", &css_w, &css_h);
    float device_dpr = emscripten_get_device_pixel_ratio();
    fb_w = css_w * device_dpr;
    fb_h = css_h * device_dpr;
    SDL_SetWindowSize(window, fb_w, fb_h);
    emscripten_set_canvas_element_size("#canvas", fb_w, fb_h);
    #else
    SDL_GetWindowSizeInPixels(window, &fb_w, &fb_h);
    #endif

    platform()->update();
}

static void destroy_offscreen_fbo(GLuint& fbo, GLuint& color, GLuint& depth)
{
    if (depth) { glDeleteRenderbuffers(1, &depth); depth = 0; }
    if (color) { glDeleteTextures(1, &color);      color = 0; }
    if (fbo) { glDeleteFramebuffers(1, &fbo);    fbo = 0; }
}

static void ensure_offscreen_fbo(GLuint& fbo, GLuint& color, GLuint& depth, int w, int h)
{
    destroy_offscreen_fbo(fbo, color, depth);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool PlatformManager::deviceVertical()
{
    #ifdef __EMSCRIPTEN__
    EmscriptenOrientationChangeEvent orientation;
    if (emscripten_get_orientation_status(&orientation) == EMSCRIPTEN_RESULT_SUCCESS)
        return abs(orientation.orientationAngle % 180) < 90;
    #endif

    return false;
}

void PlatformManager::deviceOrientation(int* orientation_angle, int* orientation_index)
{
    #ifdef __EMSCRIPTEN__
    EmscriptenOrientationChangeEvent orientation;
    if (emscripten_get_orientation_status(&orientation) == EMSCRIPTEN_RESULT_SUCCESS)
    {
        if (orientation_angle) *orientation_angle = orientation.orientationAngle;
        if (orientation_index) *orientation_index = orientation.orientationIndex;
    }
    #else
    if (orientation_angle) *orientation_angle = 0;
    if (orientation_index) *orientation_index = 0;
    #endif
}

#ifdef __EMSCRIPTEN__
std::function<void(int, int)> _onOrientationChangedCB;
EM_BOOL _orientationChanged(int type, const EmscriptenOrientationChangeEvent* e, void* data)
{
    if (_onOrientationChangedCB)
        _onOrientationChangedCB(e->orientationIndex, e->orientationAngle);
    return EM_TRUE;
}
#endif

bool PlatformManager::deviceOrientationChanged(std::function<void(int, int)> onChanged [[maybe_unused]] )
{
    #ifdef __EMSCRIPTEN__
    _onOrientationChangedCB = onChanged;
    EMSCRIPTEN_RESULT res = emscripten_set_orientationchange_callback(NULL, EM_FALSE, _orientationChanged);
    return (res == EMSCRIPTEN_RESULT_SUCCESS);
    #else
    return false;
    #endif
}

bool PlatformManager::isMobile() const
{
    #ifdef BL_SIMULATE_MOBILE
    return true;
    #else
    return is_mobile_device;
    #endif
}

bool PlatformManager::isDesktopNative() const
{
    #if defined BL_WEB_BUILD
    return false;
    #else
    return !isMobile();
    #endif
}

bool PlatformManager::isDesktopBrowser() const
{
    // detect real environment
    #ifdef __EMSCRIPTEN__
    return !isMobile(); // Assumed desktop browser if mobile screen not detected
    #else
    return false; // Native desktop application
    #endif
}

float PlatformManager::fontScale() const
{
    return isMobile() ? 1.3f : 1.0f;
}

float PlatformManager::thumbScale(float extra_mobile_mult) const
{
    return isMobile() ? (2.0f * extra_mobile_mult) : 1.0f;
}

float PlatformManager::lineHeight() const
{
    return ImGui::GetFontSize();
}

float PlatformManager::inputHeight() const
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& s = ImGui::GetStyle();
    return io.DisplaySize.y / ImGui::GetFontSize() + s.FramePadding.y * 2.0f;
}

float PlatformManager::maxCharRows() const
{
    ImGuiIO& io = ImGui::GetIO();
    return io.DisplaySize.y / ImGui::GetFontSize();
}

float PlatformManager::maxCharCols() const
{
    ImGuiIO& io = ImGui::GetIO();
    return io.DisplaySize.x / ImGui::GetFontSize();
}

std::filesystem::path PlatformManager::executableDir() const
{
    #if defined(_WIN32)
    std::wstring buf(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(nullptr, buf.data(), (DWORD)buf.size());
    if (len == 0)
        throw std::runtime_error("GetModuleFileNameW failed");
    buf.resize(len);
    return std::filesystem::path(buf).parent_path();

    #elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0)
        throw std::runtime_error("_NSGetExecutablePath failed");
    return std::filesystem::canonical(buf).parent_path();

    #elif defined(__linux__)
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len == -1)
        throw std::runtime_error("readlink failed");
    buf[len] = '\0';
    return std::filesystem::canonical(buf).parent_path();

    #else // fallback
    return std::filesystem::current_path();
    #endif
}

std::filesystem::path PlatformManager::resourceDir() const
{
    #ifdef __EMSCRIPTEN__
    return "/";
    #else
    return executableDir();
    #endif
}

std::string PlatformManager::path(std::string_view virtual_path) const
{
    std::filesystem::path p = resourceDir();

    if (!virtual_path.empty() && virtual_path.front() == '/')
        virtual_path.remove_prefix(1); // trim leading '/'

    p /= virtual_path; // join
    p = p.lexically_normal(); // clean up
    return p.string();
}