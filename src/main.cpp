#include "stdafx.h"
#include "Window.h"

int main(int argc, char* argv[]) {
    //Create a window
    try{
        Window window;
        if (!window.initialize("VulkanEngine", 600, 600)) return EXIT_FAILURE;

        // Start the rendering loop
        window.startRenderingLoop();
        
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}