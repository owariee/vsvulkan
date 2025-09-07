#include "sdlwindow.h"

// "Vulkan Engine", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
SDL_Window* SDLWindowInit(const char* name, uint32_t width, uint32_t height, SDL_WindowFlags flags) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(flags);

	SDL_Window* _window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		window_flags
	);

	return _window;
}

VkSurfaceKHR SDLVulkanCreateSurface(VkInstance instance, SDL_Window* window) {
	VkSurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
		fprintf(stderr, "Failed to create Vulkan surface.\n");
		return VK_NULL_HANDLE;
	}
	return surface;
}

int32_t SDLHandleEvents(std::function<void(SDL_Event *event)> eventHandlerLambda) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		eventHandlerLambda(&event);
		if (event.type == SDL_QUIT) {
			return -1;
		}
	}
}
