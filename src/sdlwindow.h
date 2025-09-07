#pragma once

#include <functional>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

SDL_Window* SDLWindowInit(const char* name, uint32_t width, uint32_t height, SDL_WindowFlags flags);
VkSurfaceKHR SDLVulkanCreateSurface(VkInstance instance, SDL_Window* window);
int32_t SDLHandleEvents(std::function<void(SDL_Event* event)> eventHandlerLambda);
