#pragma once
#include "stdafx.h"
#include "Renderer.h"

class Window
{

protected:
    SDL_Window* _window = nullptr;
    Renderer* _renderer = nullptr;

    static bool eventCallback(void *userdata, SDL_Event *event);

    // Event handlers
    void onWindowResized(int width, int height);
    void onMouseMotion(int x, int y);
    void onMouseButtonDown(int button, int x, int y);
    void onMouseButtonUp(int button, int x, int y);
    void onMouseWheel(int x, int y);

private:
    bool _isMouseDown = false;
    int _lastMouseX = 0;
    int _lastMouseY = 0;

public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool initialize(const std::string& title, const uint16_t width, const uint16_t height);
    void startRenderingLoop();
};