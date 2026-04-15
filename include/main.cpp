#include <thread>
#include <cmath>
#include <imgui.h>

/// SDL3
#include <SDL3/SDL.h>

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
#define SDL_MAIN_HANDLED
#include <GLES3/gl3.h>
#include "backend/emscripten_browser_clipboard.h"
#else
#include "glad/glad.h"
#endif

/// ImGui
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "backend/platform.h"
#include "backend/image_loader.h"
#include "app.h"

SDL_Window* window = nullptr;
bool quitting = false;

ImFont* main_font = nullptr;
ImFont* mono_font = nullptr;

#ifdef __EMSCRIPTEN__
bool simulated_imgui_paste = false;
#endif

void gui_loop()
{
    // ======== Poll SDL events ========
    ImGuiIO& io = ImGui::GetIO();

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        SDL_Event es = e;
        ImGui_ImplSDL3_ProcessEvent(&es);

        switch (es.type)
        {
        case SDL_EVENT_QUIT:
            quitting = true;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            platform()->resized();
            break;

        default:
            break;
        }

        app()->onEvent(es);
    }

    // ======== Prepare frame ========
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    ImGui::GetIO().DisplaySize = ImVec2((float)platform()->fboWidth(), (float)platform()->fboHeight());
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    ImGui::NewFrame();

    // ======== Draw window ========
    app()->onFrame();

    // ======== Render ========
    ImGui::Render();

    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);

    #ifdef __EMSCRIPTEN__
    // Release simulated paste keys
    if (simulated_imgui_paste) {
        simulated_imgui_paste = false;
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, false);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_V, false);
    }
    #endif

    #ifndef __EMSCRIPTEN__
    std::this_thread::sleep_for(std::chrono::milliseconds{ 16 });
    #endif
}


#ifdef __EMSCRIPTEN__
std::string clipboard_content;  // this stores the content for our internal clipboard
bool simulatedImguiPaste = false;

char const* get_content_for_imgui(ImGuiContext*)
{
    /// Callback for imgui, to return clipboard content
    return clipboard_content.c_str();
}

void set_content_from_imgui(ImGuiContext*, char const* text)
{
    /// Callback for imgui, to set clipboard content
    clipboard_content = text;
    emscripten_browser_clipboard::copy(clipboard_content);  // send clipboard data to the browser
}
void clipboard_paste_callback(std::string&& paste_data, void* callback_data)
{
    clipboard_content = std::move(paste_data);
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_V, true);
    simulated_imgui_paste = true;
}
#endif

void initStyles()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // base sizes
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.PopupRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 6.0f;

    style.ScrollbarRounding = 6.0f * platform()->thumbScale();
    style.ScrollbarSize = 20.0f * platform()->thumbScale(0.85f);
    style.GrabMinSize = 35.0f * platform()->thumbScale(0.85f);

    //style.ScrollbarRounding = platform()->isMobile() ? 12.0f : 6.0f; // Extra scrollbar size for mobile
    //style.ScrollbarSize = platform()->isMobile() ? 30.0f : 20.0f;    // Extra scrollbar size for mobile

    // update by dpr
    style.ScaleAllSizes(platform()->dpr());

    // colors
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);                  // Light grey text for readability
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);          // Subtle grey for disabled text
    colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);              // Dark background with a hint of blue
    colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);               // Slightly lighter for child elements
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);               // Popup background
    colors[ImGuiCol_Border] = ImVec4(0.28f, 0.29f, 0.30f, 0.60f);                // Soft border color
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);          // No border shadow
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);               // Frame background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);        // Frame hover effect
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);         // Active frame background
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);               // Title background
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);         // Active title background
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);      // Collapsed title background
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);             // Menu bar background
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);           // Scrollbar background
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);         // Dark accent for scrollbar grab
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);  // Scrollbar grab hover
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.34f, 0.36f, 1.00f);   // Scrollbar grab active
    colors[ImGuiCol_CheckMark] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);             // Dark blue checkmark
    colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);            // Dark blue slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);      // Active slider grab
    colors[ImGuiCol_Button] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);                // Dark blue button
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);         // Button hover effect
    colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);          // Active button
    colors[ImGuiCol_Header] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);                // Header color similar to button
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);         // Header hover effect
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);          // Active header
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);             // Separator color
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);      // Hover effect for separator
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);       // Active separator
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);            // Resize grip
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);     // Hover effect for resize grip
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.44f, 0.54f, 0.64f, 1.00f);      // Active resize grip
    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);                   // Inactive tab
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);            // Hover effect for tab
    colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);             // Active tab color
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);          // Unfocused tab
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);    // Active but unfocused tab
    colors[ImGuiCol_PlotLines] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);             // Plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);      // Hover effect for plot lines
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);         // Histogram color
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);  // Hover effect for histogram
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);         // Table header background
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);     // Strong border for tables
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.24f, 0.25f, 0.26f, 1.00f);      // Light border for tables
    colors[ImGuiCol_TableRowBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);            // Table row background
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);         // Alternate row background
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.34f, 0.64f, 0.85f);        // Selected text background
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.46f, 0.56f, 0.66f, 0.90f);        // Drag and drop target
    colors[ImGuiCol_NavHighlight] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);          // Navigation highlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);     // Dim background for windowing
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);      // Dim background for modal windows
}

void initFonts()
{
    // Update FreeType fonts
    ImGuiIO& io = ImGui::GetIO();

    float base_pt = 16.0f;
    std::string font_path = platform()->path("/data/fonts/DroidSans.ttf");
    std::string font_path_mono = platform()->path("/data/fonts/UbuntuMono.ttf");
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;

    io.Fonts->Clear();
    main_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), base_pt * platform()->dpr() * platform()->fontScale(), &config);
    mono_font = io.Fonts->AddFontFromFileTTF(font_path_mono.c_str(), base_pt * platform()->dpr() * platform()->fontScale(), &config);
    io.Fonts->Build();
}

void checkChangedDPR()
{
    platform()->update();

    static float last_dpr = -1.0f;
    if (std::fabs(platform()->dpr() - last_dpr) > 0.01f)
    {
        // DPR changed
        last_dpr = platform()->dpr();

        initStyles();
        initFonts();
    }
}

void init_window()
{
    ImGui::LoadIniSettingsFromMemory("");

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Always initializes window on first call
    checkChangedDPR();
}


int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
    SDL_SetEventEnabled(SDL_EVENT_DROP_TEXT, true);
    SDL_SetEventEnabled(SDL_EVENT_DROP_BEGIN, true);
    SDL_SetEventEnabled(SDL_EVENT_DROP_COMPLETE, true);

    int fb_w = 1280, fb_h = 720;

    #ifdef __EMSCRIPTEN__
    emscripten_get_canvas_element_size("#canvas", &fb_w, &fb_h);
    #endif

    const char* window_name = "App";
    auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

    window = SDL_CreateWindow(window_name, fb_w, fb_h, window_flags);

    if (!window)
        return 1;

    // Set icon
    std::string icon_path = platform()->path("/data/icon/app.png");
    int w = 0, h = 0, comp = 0;
    unsigned char* icon_rgba = stbi_load(icon_path.c_str(), &w, &h, &comp, 4);
    if (icon_rgba) {
        SDL_Surface* icon = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, icon_rgba, w * 4);
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    }

    SDL_GLContext gl_context = nullptr;

    // ======== OpenGL setup ========
    {
        gl_context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, gl_context);

        //SDL_GL_SetSwapInterval(1); // enforces 60fps / v-sync
        SDL_GL_SetSwapInterval(0);

        #ifndef __EMSCRIPTEN__
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            //blPrint() << "Failed to initialize GLAD\n";
            return 1;
        }

        // Make colours consistent on desktop build
        glDisable(GL_FRAMEBUFFER_SRGB);
        #endif
    }

    // Begin application lifecycle
    {
        auto _platform_manager = std::make_unique<PlatformManager>(window);
        auto _app = std::make_unique<App>();

        // ======== ImGui setup ========
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            //ImPlot::CreateContext();
            ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
            ImGui_ImplOpenGL3_Init();

            #ifdef __EMSCRIPTEN__
            emscripten_browser_clipboard::paste(clipboard_paste_callback);

            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            platform_io.Platform_GetClipboardTextFn = get_content_for_imgui;
            platform_io.Platform_SetClipboardTextFn = set_content_from_imgui;
            #endif
        }

        platform()->init();
        init_window();
        app()->onStartup();

        // ======== Start main gui loop ========
        #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(gui_loop, 0, true);
        #else
        while (!quitting)
            gui_loop();
        #endif

        app()->onShutdown();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#ifdef _WIN32
int WINAPI CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv); // Redirect entry point for WIN32 build
}
#endif