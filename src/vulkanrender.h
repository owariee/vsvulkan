#pragma once

#ifdef _WIN32
#define GRAPHICS_SERVER_DWM_WIN32
#elif defined(__linux__)
#define GRAPHICS_SERVER_X11_LINUX
#endif

#ifdef GRAPHICS_SERVER_DWM_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif GRAPHICS_SERVER_X11_LINUX
#define VK_USE_PLATFORM_XCB_KHR
#elif GRAPHICS_SERVER_WAYLAND_LINUX
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <functional>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define CHECK_VULKAN_RESULT(res) \
        do { \
            if ((res) != VK_SUCCESS) { \
                fprintf(stderr, "Vulkan error: %d at %s:%d\n", (res), __FILE__, __LINE__); \
                abort(); \
            } \
        } while (0)

#define VK_REQUIRED_IMAGE_COUNT 4

typedef struct {
    VkSwapchainKHR instance;
    VkImage images[VK_REQUIRED_IMAGE_COUNT];
    VkImageView imageViews[VK_REQUIRED_IMAGE_COUNT];
    VkFramebuffer framebuffers[VK_REQUIRED_IMAGE_COUNT];
} VulkanSwapchain;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t queueFamilyIndex;
    VkCommandPool commandPool;
    VkRenderPass renderPass;
    VkQueue graphicsQueue;
	VkCommandBuffer commandBuffers[VK_REQUIRED_IMAGE_COUNT];
    VkFence inFlightFence;
	VkSemaphore renderFinishedSemaphores[VK_REQUIRED_IMAGE_COUNT];
	VkSemaphore imageAvailableSemaphores[VK_REQUIRED_IMAGE_COUNT];
    VkSurfaceKHR surface;
    VkExtent2D surfaceSize;
	VulkanSwapchain swapchain;
    uint32_t currentFrame;
    uint32_t MAX_FRAMES_IN_FLIGHT;
    bool shouldRecreateSwapchain;
    std::function<void(VkExtent2D surfaceSize, VkCommandBuffer commandBuffer)> commandsLambda;
} VulkanContext;

void VulkanInit(VulkanContext* vkContext, std::function<void(VulkanContext* vkContext)> createSurface);
void VulkanShutdown(VulkanContext* vkContext);
void VulkanDraw(VulkanContext* vkContext);
void VulkanBindCommandBuffers(VulkanContext* vkContext, std::function<void(VkExtent2D surfaceSize, VkCommandBuffer commandBuffer)> commandsLambda);
