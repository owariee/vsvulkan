#pragma once

#include "vulkanrender.h"
#include "sdlwindow.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

VkDescriptorPool ImGuiSDLVulkanStartup(SDL_Window* window, VulkanContext* vkContext);
void ImGuiSDLVulkanDraw(std::function<void(void)> imguiWindowsLambda);
void ImGuiSDLVulkanShutdown(VulkanContext* vkContext, VkDescriptorPool imguiPool);
