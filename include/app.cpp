#include "app.h"
#include <imgui.h>

App* App::singleton = nullptr;

void App::onStartup()
{
}

void App::onFrame()
{
    ImGui::ShowDemoWindow();
}

void App::onShutdown()
{

}

void App::onEvent(SDL_Event e)
{
    switch (e.type)
    {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_MOTION:
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        break;

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_TEXT_INPUT:
        break;
    }
}