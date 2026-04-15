#pragma once
#include <SDL3/SDL.h>

class App
{
    static App* singleton;

public:

    static constexpr App* instance() {
        return singleton;
    }

    void onStartup();
    void onFrame();
    void onShutdown();
    void onEvent(SDL_Event e);
};

[[nodiscard]] constexpr App* app()
{
    return App::instance();
}
