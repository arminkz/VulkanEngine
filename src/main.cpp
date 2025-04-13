#include "stdafx.h"
#include "Window.h"

int main(int argc, char* argv[]) {
    //Create a window
    Window window;
    if (!window.initialize("VulkanEngine", 600, 600)) return EXIT_FAILURE;

    // Start the rendering loop
    window.startRenderingLoop();

    return EXIT_SUCCESS;
}