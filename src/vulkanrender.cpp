/**
 * @file    vulkanrender.cpp
 * @brief   Vulkan rendering module implementation.
 *
 * @details  This file contains all the necessary Vulkan functions needed in 
 *          order to operate an Vulkan Renderer. This is capable of Shader,
 *          Textures, Buffers, and all other necessary Vulkan objects.
 *
 * @author  João Gabriel Sabatini
 * @date    2025-09-27
 * @version 1.0
 *
 * @copyright
 *          Proprietary License, any use of this code is subject to
 *          charges and restrictions set forth by its author.
 *
 * @note    Optional: You can add @note, @warning, or @todo tags here.
 *
 * @see     vulkanrender.h
 */

#include "vulkanrender.h"

#include <spirv_reflect.h>
#include <stdexcept>
#include <fstream>
#include <string>
#include <iostream>

/**
 * @brief Creates a Vulkan instance with required extensions and validation layers.
 * @return VkInstance The created Vulkan instance.
 */
static VkInstance VulkanCreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // Platform-specific surface extension
    const char* extensions[] = {
        "VK_KHR_surface",
    #ifdef GRAPHICS_SERVER_DWM_WIN32
        "VK_KHR_win32_surface"
    #elif GRAPHICS_SERVER_X11_LINUX
        "VK_KHR_xcb_surface"
    #elif GRAPHICS_SERVER_WAYLAND_LINUX
		"VK_KHR_wayland_surface"
    #endif
    };

    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = layers;

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    CHECK_VULKAN_RESULT(result);

    return instance;
}

/**
 * @brief Selects first physical device (GPU) from the available devices.
 * @return VkPhysicalDevice The selected physical device.
 */
static VkPhysicalDevice VulkanSelectPhysicalDevice(VkInstance instance) {
    // Query the number of physical devices:
    uint32_t count;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    assert(count > 0);

    // Get all physical device handles:
    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[count];
    vkEnumeratePhysicalDevices(instance, &count, physicalDevices);
    VkPhysicalDevice physicalDevice = physicalDevices[0];
    delete[] physicalDevices;

    // Select a physical device:
    return physicalDevice;
}

/**
 * @brief Select first queue family that supports graphics operations.
 * @return uint32_t The index of the selected queue family.
 */
static uint32_t VulkanGetGraphicsQueueFamilyIndex(VkPhysicalDevice physicalDevice) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
    // Find a queue family that supports graphics:
    uint32_t i;
    for (i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    delete[] queueFamilies;
    return i;
}

/**
 * @brief Creates a logical device from the selected physical device.
 * @return VkDevice The created logical device.
 */
static VkDevice VulkanCreateLogicalDevice(VkPhysicalDevice physicalDevice) {
    float priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = 0;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &priority;
    const char* enabled_extensions[1] = { "VK_KHR_swapchain" };

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = enabled_extensions;

    VkDevice device;
    VkResult result = vkCreateDevice(physicalDevice, &create_info, nullptr, &device);
    CHECK_VULKAN_RESULT(result);

    return device;
}

/**
 * @brief Retrieves the graphics queue from the logical device.
 * @return VkQueue The graphics queue.
 */
static VkQueue VulkanGetGraphicsQueue(VkDevice device, uint32_t queueFamilyIndex) {
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    return queue;
}

/**
 * @brief Creates a command pool for allocating command buffers.
 * @return VkCommandPool The created command pool.
 */
static VkCommandPool VulkanCreateCommandPool(VkDevice device, uint32_t queueFamilyIndex) {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool!\n");
        exit(1);
    }
    return commandPool;
}

/**
 * @brief Creates a simple render pass with one color attachment.
 * @return VkRenderPass The created render pass.
 */
static VkRenderPass VulkanCreateRenderPass(VkDevice device, VkFormat format) {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    VkRenderPass renderPass;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass!\n");
        exit(1);
    }
    return renderPass;
}

/**
 * @brief Allocates a single command buffer from the command pool.
 * @return VkCommandBuffer The allocated command buffer.
 */
static VkCommandBuffer VulkanAllocateCommandBuffer(VkDevice device, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffer!\n");
        exit(1);
    }
    return commandBuffer;
}

/**
 * @brief Allocates multiple command buffers from the command pool.
 * @note This is used so you dont have to allocate one command buffer per Swapchain image manually.
 * @return VkCommandBuffer* An array of allocated command buffers.
 */
static void VulkanAllocateCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* commandBuffers, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        commandBuffers[i] = VulkanAllocateCommandBuffer(device, commandPool);
    }
}

/**
 * @brief Frees multiple command buffers and releases their memory.
 */
static void VulkanDestroyCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* commandBuffers, uint32_t count) {
    vkFreeCommandBuffers(device, commandPool, count, commandBuffers);
}

/**
 * @brief Creates a binary semaphore for GPU synchronization.
 * @return VkSemaphore The created semaphore.
 */
static VkSemaphore VulkanCreateSemaphore(VkDevice device) {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore semaphore;
    VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
    CHECK_VULKAN_RESULT(result);
    return semaphore;
}

/**
 * @brief Creates multiple semaphores for synchronizing rendering operations.
 */
static void VulkanCreateSemaphores(VkDevice device, uint32_t swapchainImageCount, VkSemaphore* semaphores) {
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        semaphores[i] = VulkanCreateSemaphore(device);
    }
}

/**
 * @brief Destroys multiple semaphores and releases their memory.
 */
static void VulkanDestroySemaphores(VkDevice device, VkSemaphore* semaphores, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroySemaphore(device, semaphores[i], nullptr);
    }
}

/**
 * @brief Creates a fence for CPU-GPU synchronization.
 * @return VkFence The created fence.
 */
static VkFence VulkanCreateFence(VkDevice device, bool signaled) {
    VkFence fence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create fence!\n");
        exit(1);
    }
    return fence;
}

/**
 * @brief Creates a swapchain for presenting images to the surface.
 * @return VkSwapchainKHR The created swapchain.
 */
static VkSwapchainKHR VulkanCreateSwapchainInstance(VkDevice device, VkSurfaceKHR surface, VkExtent2D extent) {
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = 4;
    createInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    // Use vkGetPhysicalDeviceSurfaceFormatsKHR to find out ^
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // further settings: images queue ownership, alpha, ...
    VkSwapchainKHR swapchain;
    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
    CHECK_VULKAN_RESULT(result);
    return swapchain;
}

/**
 * @brief Retrieves the images from the swapchain.
 * @return VkImage* An array of swapchain images.
 */
static VkImage* VulkanGetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, uint32_t* imageCountOut, VkImage* swapchainImages) {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr); // Get count
    printf("Swapchain image count: %u\n", imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages); // Get images
    *imageCountOut = imageCount;
    return swapchainImages;
}

/**
 * @brief Creates a 2D image view for the given image and format.
 * @return VkImageView The created image view.
 */
static VkImageView VulkanCreateImageView(VkDevice device, VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    // Image subresource info
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image view!\n");
        exit(1);
    }

    return imageView;
}

/**
 * @brief Creates image views for all swapchain images.
 */
static void VulkanCreateImageViews(VkDevice device, VkImage* swapchainImages, uint32_t swapchainImageCount, VkImageView* swapchainImageViews) {
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        swapchainImageViews[i] = VulkanCreateImageView(device, swapchainImages[i], VK_FORMAT_R8G8B8A8_UNORM);
    }
}

/**
 * @brief Creates a framebuffer for the given render pass and image view.
 * @return VkFramebuffer The created framebuffer.
 */
static VkFramebuffer VulkanCreateFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, VkExtent2D extent) {
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &imageView;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    VkFramebuffer framebuffer;
    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create framebuffer!\n");
        exit(1);
    }
    return framebuffer;
}

/**
 * @brief Creates framebuffers for all swapchain image views.
 */
static void VulkanCreateFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView* imageViews, VkExtent2D extent, uint32_t count, VkFramebuffer* framebuffers) {
    for (uint32_t i = 0; i < count; i++) {
        framebuffers[i] = VulkanCreateFramebuffer(device, renderPass, imageViews[i], extent);
    }
}

/**
 * @brief Destroys multiple image views and releases their memory.
 */
static void VulkanDestroyImageViews(VkDevice device, VkImageView* imageViews, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyImageView(device, imageViews[i], nullptr);
    }
}

/**
 * @brief Destroys multiple framebuffers and releases their memory.
 */
static void VulkanDestroyFramebuffers(VkDevice device, VkFramebuffer* framebuffers, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }
}

/**
 * @brief Determines the appropriate swapchain extent based on surface capabilities.
 * @return VkExtent2D The determined swapchain extent.
 */
static VkExtent2D GetSurfaceExtent(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get physical device surface capabilities!\n");
        exit(1);
    }

    // If currentExtent.width is UINT32_MAX, then the extent is undefined and
    // the swapchain extent must be set by the application.
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    }
    else {
        // In this case, the application needs to specify the extent.
        // You could clamp your desired extent to the allowed min/max.
        VkExtent2D actualExtent = {
            // For example, some desired width and height, or fallback values:
            1280, // desired width
            720   // desired height
        };

        if (actualExtent.width < surfaceCapabilities.minImageExtent.width) {
            actualExtent.width = surfaceCapabilities.minImageExtent.width;
        }
        else if (actualExtent.width > surfaceCapabilities.maxImageExtent.width) {
            actualExtent.width = surfaceCapabilities.maxImageExtent.width;
        }

        if (actualExtent.height < surfaceCapabilities.minImageExtent.height) {
            actualExtent.height = surfaceCapabilities.minImageExtent.height;
        }
        else if (actualExtent.height > surfaceCapabilities.maxImageExtent.height) {
            actualExtent.height = surfaceCapabilities.maxImageExtent.height;
        }

        return actualExtent;
    }
}

/**
 * @brief Creates the swapchain and its associated resources.
 */
static void VulkanCreateSwapchain(VulkanContext* vkContext) {
    uint32_t swapchainImageCount;
	vkContext->surfaceSize = GetSurfaceExtent(vkContext->physicalDevice, vkContext->surface);
    vkContext->swapchain.instance = VulkanCreateSwapchainInstance(vkContext->device, vkContext->surface, vkContext->surfaceSize);
    VulkanGetSwapchainImages(vkContext->device, vkContext->swapchain.instance, &swapchainImageCount, vkContext->swapchain.images);
    VulkanCreateImageViews(vkContext->device, vkContext->swapchain.images, VK_REQUIRED_IMAGE_COUNT, vkContext->swapchain.imageViews);
    VulkanCreateFramebuffers(vkContext->device, vkContext->renderPass, vkContext->swapchain.imageViews, vkContext->surfaceSize, VK_REQUIRED_IMAGE_COUNT, vkContext->swapchain.framebuffers);
}

/**
 * @brief Destroys the swapchain and its associated resources.
 */
static void VulkanDestroySwapchain(VulkanContext* vkContext) {
    VulkanDestroyFramebuffers(vkContext->device, vkContext->swapchain.framebuffers, VK_REQUIRED_IMAGE_COUNT);
    VulkanDestroyImageViews(vkContext->device, vkContext->swapchain.imageViews, VK_REQUIRED_IMAGE_COUNT);
    vkDestroySwapchainKHR(vkContext->device, vkContext->swapchain.instance, nullptr);
}

/**
 * @brief Record commands into an command buffer.
 * @todo Make this function accepts an vector of lambdas to record multiple sets of commmands.
 */
static void VulkanRecordCommandBuffer(
    VulkanContext* vkContext,
    VkCommandBuffer commandBuffer,
    VkFramebuffer framebuffer,
    std::function<void(VkExtent2D surfaceSize, VkCommandBuffer commandBuffer)> commandsLambda)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = NULL; // Only relevant for secondary command buffers

    // Start recording commands
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Specify clear color (background)
    VkClearValue clearColor = { .color = { {0.0f, 0.0f, 0.0f, 1.0f} } };

    // Info about the render pass begin
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vkContext->renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vkContext->surfaceSize;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    // Begin render pass
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    //// Set viewport (the region of the framebuffer you want to draw to)
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vkContext->surfaceSize.width;
    viewport.height = (float)vkContext->surfaceSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Set scissor (cut-off area, usually matches viewport)
    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = vkContext->surfaceSize;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	commandsLambda(vkContext->surfaceSize, commandBuffer); // Call the provided lambda to record custom commands

    // End the render pass
    vkCmdEndRenderPass(commandBuffer);

    // Finish recording commands
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record command buffer!\n");
        exit(1);
    }
}

/**
 * @brief Records multiple command buffers for rendering.
 */
static void VulkanRecordCommandBuffers(VulkanContext* vkContext) {
    for (uint32_t i = 0; i < VK_REQUIRED_IMAGE_COUNT; i++) {
        VulkanRecordCommandBuffer(vkContext, vkContext->commandBuffers[i], vkContext->swapchain.framebuffers[i], vkContext->commandsLambda);
    }
}

/**
 * @brief Recreates the swapchain, typically in response to window resizing.
 */
static void VulkanRecreateSwapchain(VulkanContext* vkContext) {
    vkDeviceWaitIdle(vkContext->device);
    VulkanDestroySwapchain(vkContext);
    VulkanCreateSwapchain(vkContext);
    VulkanRecordCommandBuffers(vkContext);
    vkDeviceWaitIdle(vkContext->device);
}

/**
 * @brief Bind a new set of commands and records multiple command buffers for rendering.
 */
void VulkanBindCommandBuffers(VulkanContext* vkContext, std::function<void(VkExtent2D surfaceSize, VkCommandBuffer commandBuffer)> commandsLambda) {
    vkContext->commandsLambda = commandsLambda;
}

/**
 * @brief Present queue.
 * @result VkResult Result of the presentation operation.
 */
static VkResult VulkanQueuePresent(
    VkQueue graphicsQueue,
    VkSwapchainKHR swapchain,
    VkSemaphore renderFinishedSemaphore,
    uint32_t imageIndex)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    return vkQueuePresentKHR(graphicsQueue, &presentInfo);
}

/**
 * @brief Submit command buffers to queue.
 */
static void VulkanSubmitCommandBuffer(
    VkQueue graphicsQueue,
    VkCommandBuffer* commandBuffers,
    VkSemaphore imageAvailableSemaphore,
    VkSemaphore renderFinishedSemaphore,
    VkFence inFlightFence,
    uint32_t imageIndex)
{
    // Submit recorded command buffer
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
}

/**
 * @brief Acquire next swapchain image.
 * @return uint32_t The index of the acquired image.
 */
static uint32_t VulkanAcquireNextImage(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore) {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);
    //CHECK_VULKAN_RESULT(result);
    printf("Acquired image index: %u\n", imageIndex);
    return imageIndex;
}

/**
 * @brief Load shader module from file.
 * @return VkShaderModule The created shader module.
 */
static VkShaderModule VulkanLoadShaderModule(VkDevice device, const char* filepath) {
    // Open the file
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filepath);
        return VK_NULL_HANDLE;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    // Allocate buffer and read file
    char* buffer = (char*)malloc(filesize);
    if (fread(buffer, 1, filesize, file) != filesize) {
        fprintf(stderr, "Failed to read shader file: %s\n", filepath);
        fclose(file);
        free(buffer);
        return VK_NULL_HANDLE;
    }
    fclose(file);

    // Create shader module
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)filesize,
        .pCode = (const uint32_t*)buffer
    };

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, NULL, &shaderModule);
    free(buffer);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module from %s\n", filepath);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

/**
 * @brief Create shader stage.
 * @return VkPipelineShaderStageCreateInfo The created shader stage info.
 */
static VkPipelineShaderStageCreateInfo VulkanCreateShaderStage(VkDevice device, const char* filePath, VkShaderStageFlagBits stage, VkShaderModule* shaderModuleOut) {
    VkShaderModule shaderModule = VulkanLoadShaderModule(device, filePath);
    if (shaderModule == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to create shader module for %s\n", filePath);
        exit(EXIT_FAILURE); // Or handle error gracefully
    }

    VkPipelineShaderStageCreateInfo shaderStageInfo = {};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";

    *shaderModuleOut = shaderModule;

    return shaderStageInfo;
}

/**
 * @brief Create pipeline from 2 shaders.
 * @note The vertex input description is provided via a lambda function.
 * @return int32_t 0 on success, -1 on failure.
 */
int32_t VulkanCreateGraphicsPipeline(
    VulkanContext* vkContext,
    const char* shaderBaseName,
    std::function<void(VertexInputDescription* vertexInputDescription)> vertexInputDescriptionLambda)
{
    // Load shaders
    char vertShaderPath[256];
    char fragShaderPath[256];

    snprintf(vertShaderPath, sizeof(vertShaderPath), "../shaders/%s.vert.spv", shaderBaseName);
    snprintf(fragShaderPath, sizeof(fragShaderPath), "../shaders/%s.frag.spv", shaderBaseName);

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkPipelineShaderStageCreateInfo vertShader = VulkanCreateShaderStage(vkContext->device, vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT, &vertShaderModule);
    VkPipelineShaderStageCreateInfo fragShader = VulkanCreateShaderStage(vkContext->device, fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT, &fragShaderModule);
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShader, fragShader };

    // Setup vertex input state with these descriptions
    VertexInputDescription vertexInputDescription;
    vertexInputDescriptionLambda(&vertexInputDescription);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputDescription.bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexInputDescription.bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputDescription.attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputDescription.attributes.data();

    // --- Rest of the pipeline creation code ---
    // inputAssembly, viewport, rasterizer, etc. as before, just replace vertexInputInfo pointer in pipelineInfo

    // [Include or copy your previous pipeline creation code here, but use the new vertexInputInfo]

    // For brevity, here's an example for pipeline creation with updated vertexInputInfo:

    // Input assembly (triangle list)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport, rasterizer, multisampling, color blending as before (use your existing setup)...

    // Pipeline layout creation (no descriptors or push constants for now)
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(vertexInputDescription.layoutBinding.size());
    layoutInfo.pBindings = vertexInputDescription.layoutBinding.data();

    VkDescriptorSetLayout descriptorSetLayout;
    VkResult result = vkCreateDescriptorSetLayout(vkContext->device, &layoutInfo, nullptr, &descriptorSetLayout);
    CHECK_VULKAN_RESULT(result);

    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(vertexInputDescription.constantRange.size());
    pipelineLayoutInfo.pPushConstantRanges = vertexInputDescription.constantRange.data();

    if (vkCreatePipelineLayout(vkContext->device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout!\n");
        return -1;
    }

    // Viewport and scissor setup
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = NULL;
    viewportState.scissorCount = 1;
    viewportState.pScissors = NULL;

    VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = vkContext->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(vkContext->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline!\n");
        vkDestroyPipelineLayout(vkContext->device, pipelineLayout, NULL);
        return -1;
    }

    vkDestroyShaderModule(vkContext->device, vertShaderModule, nullptr);
    vkDestroyShaderModule(vkContext->device, fragShaderModule, nullptr);

    VulkanPipeline pipelineBuffer;
    pipelineBuffer.layout = pipelineLayout;
    pipelineBuffer.pipeline = graphicsPipeline;
    pipelineBuffer.descriptorSetLayout = descriptorSetLayout;
    size_t index = vkContext->pipelines.size();
    vkContext->pipelines.push_back(pipelineBuffer);

    return index;
}

/**
 * @brief Delete all available pipelines.
 */
static void VulkanDeletePipelines(VulkanContext* vkContext) {
    // Iterate and process existing pipelines
    for (size_t i = 0; i < vkContext->pipelines.size(); ++i) {
        if (vkContext->pipelines[i]) {
            vkDestroyDescriptorSetLayout(vkContext->device, vkContext->pipelines[i]->descriptorSetLayout, nullptr);
            vkDestroyPipeline(vkContext->device, vkContext->pipelines[i]->pipeline, nullptr);
            vkDestroyPipelineLayout(vkContext->device, vkContext->pipelines[i]->layout, nullptr);
        }
    }
}

/**
 * @brief Delete all available buffers
 */
static void VulkanDeleteBuffers(VulkanContext* vkContext) {
    // Iterate and process existing pipelines
    for (size_t i = 0; i < vkContext->buffers.size(); ++i) {
        if (vkContext->buffers[i]) {
            vkDestroyBuffer(vkContext->device, vkContext->buffers[i]->buffer, nullptr);
            vkFreeMemory(vkContext->device, vkContext->buffers[i]->memory, nullptr);
        }
    }
}

/**
 * @brief Delete a specific buffer by index.
 */
void VulkanDeleteBuffer(VulkanContext* vkContext, uint32_t index) {
    if (index >= vkContext->buffers.size()) {
        fprintf(stderr, "VulkanDeleteBuffer: invalid index %zu\n", index);
        return;
    }

    auto& optBuffer = vkContext->buffers[index];
    if (optBuffer.has_value()) {
        VulkanBuffer& buf = optBuffer.value();

        if (buf.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vkContext->device, buf.buffer, nullptr);
            buf.buffer = VK_NULL_HANDLE;
        }
        if (buf.memory != VK_NULL_HANDLE) {
            vkFreeMemory(vkContext->device, buf.memory, nullptr);
            buf.memory = VK_NULL_HANDLE;
        }

        optBuffer.reset(); // remove from vector
    }
}

/**
 * @brief Delete a specific pipeline by index.
 */
void VulkanDeletePipeline(VulkanContext* vkContext, uint32_t index) {
    if (index >= vkContext->pipelines.size()) {
        fprintf(stderr, "VulkanDeletePipeline: invalid index %zu\n", index);
        return;
    }

    auto& optPipeline = vkContext->pipelines[index];
    if (optPipeline.has_value()) {
        VulkanPipeline& pipeline = optPipeline.value();

        if (pipeline.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(vkContext->device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }
        if (pipeline.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(vkContext->device, pipeline.layout, nullptr);
            pipeline.layout = VK_NULL_HANDLE;
        }

        optPipeline.reset(); // mark slot as empty
    }
}

/**
 * @brief Create vertex buffer.
 * @return int32_t Index of the created buffer, or -1 on failure.
 */
int32_t VulkanCreateVertexBuffer(VulkanContext* vkContext, const void* vertexData, VkDeviceSize size) {
    VkDeviceMemory outMemory;
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkBuffer vertexBuffer;
    if (vkCreateBuffer(vkContext->device, &bufferInfo, NULL, &vertexBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        return -1;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkContext->device, vertexBuffer, &memRequirements);

    uint32_t memoryTypeIndex = 0;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkContext->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    if (vkAllocateMemory(vkContext->device, &allocInfo, NULL, &outMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate vertex buffer memory\n");
        vkDestroyBuffer(vkContext->device, vertexBuffer, NULL);
        return -1;
    }

    vkBindBufferMemory(vkContext->device, vertexBuffer, outMemory, 0);

    if (vertexData != NULL)
    {
        void* data;
        vkMapMemory(vkContext->device, outMemory, 0, size, 0, &data);
        memcpy(data, vertexData, (size_t)size);
        vkUnmapMemory(vkContext->device, outMemory);
    }

    VulkanBuffer bufferTemp;
    bufferTemp.buffer = vertexBuffer;
    bufferTemp.memory = outMemory;
    size_t index = vkContext->buffers.size();
    vkContext->buffers.push_back(bufferTemp);

    return index;
}

/**
 * @brief Update vertex buffer.
 */
void VulkanUpdateVertexBuffer(VulkanContext* vkContext, int32_t bufferId, const void* vertexData, VkDeviceSize size) {
    VulkanBuffer& buf = *vkContext->buffers[bufferId];

    void* data;
    vkMapMemory(vkContext->device, buf.memory, 0, size, 0, &data);
    memcpy(data, vertexData, (size_t)size);
    vkUnmapMemory(vkContext->device, buf.memory);
}

/**
 * @brief Create index buffer.
 * @return int32_t Index of the created buffer, or -1 on failure.
 */
int32_t CreateIndexBuffer(VulkanContext* vkContext, const void* indexData, VkDeviceSize size) {
    VkDeviceMemory outMemory;
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkBuffer indexBuffer;
    if (vkCreateBuffer(vkContext->device, &bufferInfo, NULL, &indexBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create index buffer\n");
        return -1;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkContext->device, indexBuffer, &memRequirements);

    uint32_t memoryTypeIndex = 0;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkContext->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    if (vkAllocateMemory(vkContext->device, &allocInfo, NULL, &outMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate index buffer memory\n");
        vkDestroyBuffer(vkContext->device, indexBuffer, NULL);
        return -1;
    }

    vkBindBufferMemory(vkContext->device, indexBuffer, outMemory, 0);

    void* data;
    vkMapMemory(vkContext->device, outMemory, 0, size, 0, &data);
    memcpy(data, indexData, (size_t)size);
    vkUnmapMemory(vkContext->device, outMemory);

    VulkanBuffer bufferTemp;
    bufferTemp.buffer = indexBuffer;
    bufferTemp.memory = outMemory;
    size_t index = vkContext->buffers.size();
    vkContext->buffers.push_back(bufferTemp);

    return index;
}

/**
 * @brief Creates an attribute description.
 */
void VulkanCreateVertexAttribute(
    VertexInputDescription* vertexInputDescription,
    uint32_t binding,
    uint32_t location,
    VkFormat format,
    uint32_t offset) 
{
    VkVertexInputAttributeDescription attributeDescription;
    attributeDescription.binding = binding;
    attributeDescription.location = location;
    attributeDescription.format = format;
    attributeDescription.offset = offset;
    vertexInputDescription->attributes.push_back(attributeDescription);
}

/**
 * @brief Creates an binding description.
 */
void VulkanCreateVertexBinding(
    VertexInputDescription* vertexInputDescription,
    uint32_t binding,
    uint32_t stride,
    VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = binding;
    bindingDescription.stride = stride;
    bindingDescription.inputRate = inputRate;
    vertexInputDescription->bindings.push_back(bindingDescription);
}

/**
 * @brief Creates an push constant.
 */
void VulkanCreatePushConstant(
    VertexInputDescription* vertexInputDescription,
    VkShaderStageFlags stageFlags,
    uint32_t offset,
    uint32_t size) 
{
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = stageFlags; // e.g., VK_SHADER_STAGE_VERTEX_BIT
    pushConstant.offset = offset;         // must be multiple of 4
    pushConstant.size = size;             // must be <= device limits
    vertexInputDescription->constantRange.push_back(pushConstant);
}

/**
 * @brief Creates a descriptor set layout binding.
 */
void VulkanCreateDescriptorSetLayoutBinding(
    VertexInputDescription* vertexInputDescription,
    uint32_t binding,
    VkDescriptorType descriptorType,
    uint32_t descriptorCount,
    VkShaderStageFlags stageFlags)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = descriptorCount; // e.g. 1 for single UBO, >1 for arrays
    layoutBinding.stageFlags = stageFlags;           // e.g. VK_SHADER_STAGE_VERTEX_BIT
    layoutBinding.pImmutableSamplers = nullptr;      // only used for immutable samplers
    vertexInputDescription->layoutBinding.push_back(layoutBinding);
}


/**
 * @brief Setup a pipeline with a given shader base name.
 * @return int32_t Index of the created pipeline, or -1 on failure.
 */
int32_t VulkanSetupPipeline(VulkanContext* vkContext, const char *shaderBasename, PipelineDescription *pipelineDesc) {
    auto vertexInputDescLambda = [=](VertexInputDescription* vertexInputDesc) {
        VulkanCreateVertexBinding(vertexInputDesc, 0, pipelineDesc->perVertexStride, VK_VERTEX_INPUT_RATE_VERTEX);
        VulkanCreateVertexBinding(vertexInputDesc, 1, pipelineDesc->perInstanceStride, VK_VERTEX_INPUT_RATE_INSTANCE);

        // setup vertex attributes
        for (const auto& input : pipelineDesc->vertexInputs) {
            VulkanCreateVertexAttribute(vertexInputDesc, input.rate == VULKAN_RATE_PER_VERTEX ? 0 : 1, input.location, (VkFormat)input.format, input.offset);
        }

		// setup push constants
        for (const auto& pc : pipelineDesc->pushConstants) {
            for(const auto& stage : pc.stages) {
                if (stage != VULKAN_STAGE_VERTEX && stage != VULKAN_STAGE_FRAGMENT) {
                    fprintf(stderr, "Unsupported push constant stage %d\n", stage);
                    continue;
                }
                VulkanCreatePushConstant(vertexInputDesc, stage, pc.offset, pc.size);
			}
		}

		// setup descriptor set layouts
        for (const auto& ds : pipelineDesc->descriptorSets) {
            for (const auto& binding : ds.bindings) {
                for (const auto& stage : binding.stages) {
                    if (stage != VULKAN_STAGE_VERTEX && stage != VULKAN_STAGE_FRAGMENT) {
                        fprintf(stderr, "Unsupported descriptor set layout binding stage %d\n", stage);
                        continue;
                    }
                    VulkanCreateDescriptorSetLayoutBinding(vertexInputDesc, binding.binding, (VkDescriptorType)binding.type, binding.count, stage);
                }
            }
		}
    };

    return VulkanCreateGraphicsPipeline(vkContext, shaderBasename, vertexInputDescLambda);
}

/**
 * @brief Convert SPIR-V state into VulkanStage.
 * @return VulkanStage The corresponding VulkanStage value.
 */
static inline VulkanStage ToVulkanStage(SpvReflectShaderStageFlagBits stage) {
    switch (stage) {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return VULKAN_STAGE_VERTEX;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return VULKAN_STAGE_FRAGMENT;
        default: return VULKAN_STAGE_NONE;
    }
}

/**
 * @brief Convert SPIR-V state into VulkanDescriptorSetType.
 * @return VulkanDescriptorSetType The corresponding VulkanDescriptorSetType value.
 */
static inline VulkanDescriptorSetType ToDescriptorType(SpvReflectDescriptorType t) {
    switch (t) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VULKAN_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        default: throw std::runtime_error("Unsupported descriptor type");
    }
}

/**
 * @brief Load SPIR-V binary from file.
 * @return std::vector<uint32_t> The loaded SPIR-V binary data.
 */
static std::vector<uint32_t> LoadSPV(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to open SPIR-V file");

    size_t size = file.tellg();
    file.seekg(0);

    if (size % 4 != 0) throw std::runtime_error("Invalid SPIR-V size");

    std::vector<uint32_t> buffer(size / 4);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

// Reflect push constants from a single module and merge
void ReflectPushConstantsModule(
    const std::string& spv_path,
    std::unordered_map<uint32_t, VulkanPushConstantEntry>& mergedPCs
) {
    auto spirv = LoadSPV(spv_path);
    SpvReflectShaderModule module;
    if (spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module) != SPV_REFLECT_RESULT_SUCCESS)
        throw std::runtime_error("Failed to create SPIRV reflection module");

    uint32_t count = 0;
    spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
    std::vector<SpvReflectBlockVariable*> blocks(count);
    spvReflectEnumeratePushConstantBlocks(&module, &count, blocks.data());

    for (auto* block : blocks) {
        auto it = mergedPCs.find(block->offset);
        if (it != mergedPCs.end()) {
            // Already exists: merge stages
            it->second.stages.push_back(ToVulkanStage(module.shader_stage));
        }
        else {
            VulkanPushConstantEntry e;
            e.offset = block->offset;
            e.size = block->size;
            e.stages = { ToVulkanStage(module.shader_stage) };
            mergedPCs[block->offset] = e;
        }
    }

    spvReflectDestroyShaderModule(&module);
}

// Merge vertex + fragment push constants
std::vector<VulkanPushConstantEntry> VulkanProducePushConstants(
    const std::string& vert_spv,
    const std::string& frag_spv
) {
    std::unordered_map<uint32_t, VulkanPushConstantEntry> mergedPCs;

    ReflectPushConstantsModule(vert_spv, mergedPCs);
    ReflectPushConstantsModule(frag_spv, mergedPCs);

    // Convert map to vector
    std::vector<VulkanPushConstantEntry> pcs;
    for (auto& kv : mergedPCs) {
        pcs.push_back(std::move(kv.second));
    }

    return pcs;
}

// Reflect a single SPIR-V module and populate descriptor set info
void ReflectSPVModule(const std::string& spv_path, std::unordered_map<uint32_t, VulkanDescriptorSetEntry>& mergedSets, VulkanStage stage) {
    auto spirv = LoadSPV(spv_path);
    SpvReflectShaderModule module;
    if (spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module) != SPV_REFLECT_RESULT_SUCCESS)
        throw std::runtime_error("Failed to create SPIRV reflection module");

    uint32_t set_count = 0;
    spvReflectEnumerateDescriptorSets(&module, &set_count, nullptr);
    std::vector<SpvReflectDescriptorSet*> set_ptrs(set_count);
    spvReflectEnumerateDescriptorSets(&module, &set_count, set_ptrs.data());

    for (auto* set : set_ptrs) {
        VulkanDescriptorSetEntry& entry = mergedSets[set->set];
        entry.set = set->set;

        for (uint32_t i = 0; i < set->binding_count; ++i) {
            const auto* b = set->bindings[i];
            auto it = std::find_if(entry.bindings.begin(), entry.bindings.end(),
                [&](const VulkanDescriptorSetEntryBinding& eb) { return eb.binding == b->binding; });

            if (it != entry.bindings.end()) {
                // Already exists: merge stages
                it->stages.push_back(stage);
            }
            else {
                VulkanDescriptorSetEntryBinding binding;
                binding.binding = b->binding;
                binding.count = b->count;
                binding.type = ToDescriptorType(b->descriptor_type);
                binding.stages = { stage };
                entry.bindings.push_back(binding);
            }
        }
    }

    spvReflectDestroyShaderModule(&module);
}

// Merge vertex and fragment SPIR-V reflection
std::vector<VulkanDescriptorSetEntry> VulkanProduceDescriptorSet(const std::string& vert_spv, const std::string& frag_spv) {
    std::unordered_map<uint32_t, VulkanDescriptorSetEntry> mergedSets;

    ReflectSPVModule(vert_spv, mergedSets, VULKAN_STAGE_VERTEX);
    ReflectSPVModule(frag_spv, mergedSets, VULKAN_STAGE_FRAGMENT);

    // Convert map to vector
    std::vector<VulkanDescriptorSetEntry> sets;
    for (auto& kv : mergedSets) {
        sets.push_back(std::move(kv.second));
    }
    return sets;
}

inline const char* VulkanFormatToString(VulkanFormat f) {
    switch (f) {
    case VULKAN_FORMAT_R32G32_SFLOAT:       return "R32G32_SFLOAT";
    case VULKAN_FORMAT_R32G32B32_SFLOAT:    return "R32G32B32_SFLOAT";
    case VULKAN_FORMAT_R32G32B32A32_SFLOAT: return "R32G32B32A32_SFLOAT";
    default: return "UNKNOWN_FORMAT";
    }
}

inline const char* VertexRateToString(VertexRate r) {
    switch (r) {
    case VULKAN_RATE_PER_VERTEX: return "PER_VERTEX";
    case VULKAN_RATE_PER_INSTANCE: return "PER_INSTANCE";
    default: return "UNKNOWN_RATE";
    }
}

inline const char* VulkanStageToString(VulkanStage s) {
    switch (s) {
    case VULKAN_STAGE_NONE: return "NONE";
    case VULKAN_STAGE_VERTEX: return "VERTEX";
    case VULKAN_STAGE_FRAGMENT: return "FRAGMENT";
    default: return "UNKNOWN_STAGE";
    }
}

inline const char* DescriptorTypeToString(VulkanDescriptorSetType t) {
    switch (t) {
    case VULKAN_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "UNIFORM_BUFFER";
    default: return "UNKNOWN_DESCRIPTOR";
    }
}

void PrintPipelineDescription(const PipelineDescription& pd) {
    std::cout << "PipelineDescription {\n";
    std::cout << "  perVertexStride: " << pd.perVertexStride << "\n";
    std::cout << "  perInstanceStride: " << pd.perInstanceStride << "\n";

    std::cout << "  vertexInputs:\n";
    for (const auto& v : pd.vertexInputs) {
        std::cout << "    location " << v.location
            << ", format: " << VulkanFormatToString(v.format)
            << ", offset: " << v.offset
            << ", rate: " << VertexRateToString(v.rate) << "\n";
    }

    std::cout << "  pushConstants:\n";
    for (const auto& pc : pd.pushConstants) {
        std::cout << "    offset: " << pc.offset
            << ", size: " << pc.size
            << ", stages: ";
        for (auto stage : pc.stages) std::cout << VulkanStageToString(stage) << " ";
        std::cout << "\n";
    }

    std::cout << "  descriptorSets:\n";
    for (const auto& ds : pd.descriptorSets) {
        std::cout << "    set: " << ds.set << "\n";
        for (const auto& b : ds.bindings) {
            std::cout << "      binding: " << b.binding
                << ", type: " << DescriptorTypeToString(b.type)
                << ", count: " << b.count
                << ", stages: ";
            for (auto stage : b.stages) std::cout << VulkanStageToString(stage) << " ";
            std::cout << "\n";
        }
    }

    std::cout << "}\n";
}

/**
 * @brief Main draw function to be called every frame.
 */
void VulkanDraw(VulkanContext* vkContext) {
    vkWaitForFences(vkContext->device, 1, &vkContext->inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkContext->device, 1, &vkContext->inFlightFence);

    if (vkContext->shouldRecreateSwapchain) {
        VulkanRecreateSwapchain(vkContext);
        vkContext->shouldRecreateSwapchain = false;
    }

    VkSemaphore imageAvailableSemaphore = vkContext->imageAvailableSemaphores[vkContext->currentFrame];
    VkSemaphore renderFinishedSemaphore = vkContext->renderFinishedSemaphores[vkContext->currentFrame];

    // Acquire next image from swapchain
    uint32_t imageIndex = VulkanAcquireNextImage(vkContext->device, vkContext->swapchain.instance, imageAvailableSemaphore);

    VulkanRecordCommandBuffers(vkContext);

    VulkanSubmitCommandBuffer(
        vkContext->graphicsQueue,
        vkContext->commandBuffers,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        vkContext->inFlightFence,
        imageIndex);

    VkResult presentResult = VulkanQueuePresent(
        vkContext->graphicsQueue,
        vkContext->swapchain.instance,
        renderFinishedSemaphore,
        imageIndex);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        vkContext->shouldRecreateSwapchain = true;
    }

    // Advance to next frame
    vkContext->currentFrame = (vkContext->currentFrame + 1) % vkContext->MAX_FRAMES_IN_FLIGHT;
}

/**
 * @brief Initializes the Vulkan context.
 */
void VulkanInit(VulkanContext* vkContext, std::function<VkSurfaceKHR(VulkanContext* vkContext)> createSurface) {
    vkContext->instance = VulkanCreateInstance();
    vkContext->physicalDevice = VulkanSelectPhysicalDevice(vkContext->instance);
    vkContext->device = VulkanCreateLogicalDevice(vkContext->physicalDevice);
    vkContext->queueFamilyIndex = VulkanGetGraphicsQueueFamilyIndex(vkContext->physicalDevice);
    vkContext->commandPool = VulkanCreateCommandPool(vkContext->device, vkContext->queueFamilyIndex); // we usually only need one command pool
    vkContext->renderPass = VulkanCreateRenderPass(vkContext->device, VK_FORMAT_R8G8B8A8_UNORM);
    vkContext->graphicsQueue = VulkanGetGraphicsQueue(vkContext->device, vkContext->queueFamilyIndex);
    VulkanAllocateCommandBuffers(vkContext->device, vkContext->commandPool, vkContext->commandBuffers, VK_REQUIRED_IMAGE_COUNT);
    vkContext->inFlightFence = VulkanCreateFence(vkContext->device, false);
    VulkanCreateSemaphores(vkContext->device, VK_REQUIRED_IMAGE_COUNT, vkContext->renderFinishedSemaphores);
    VulkanCreateSemaphores(vkContext->device, VK_REQUIRED_IMAGE_COUNT, vkContext->imageAvailableSemaphores);
	vkContext->surface = createSurface(vkContext);
    VulkanCreateSwapchain(vkContext);
    vkContext->currentFrame = 0;
    vkContext->MAX_FRAMES_IN_FLIGHT = VK_REQUIRED_IMAGE_COUNT;
    vkContext->shouldRecreateSwapchain = false;
    vkContext->commandsLambda = [=](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {};
}

/**
 * @brief Shuts down the Vulkan context and releases all resources.
 */
void VulkanShutdown(VulkanContext* vkContext) {
    VulkanDeleteBuffers(vkContext);
    VulkanDeletePipelines(vkContext);
	VulkanDestroySwapchain(vkContext);
    vkDestroySurfaceKHR(vkContext->instance, vkContext->surface, nullptr);
    VulkanDestroySemaphores(vkContext->device, vkContext->renderFinishedSemaphores, VK_REQUIRED_IMAGE_COUNT);
    VulkanDestroySemaphores(vkContext->device, vkContext->imageAvailableSemaphores, VK_REQUIRED_IMAGE_COUNT);
    vkDestroyFence(vkContext->device, vkContext->inFlightFence, nullptr);
	VulkanDestroyCommandBuffers(vkContext->device, vkContext->commandPool, vkContext->commandBuffers, VK_REQUIRED_IMAGE_COUNT);
    vkDestroyCommandPool(vkContext->device, vkContext->commandPool, nullptr);
    vkDestroyRenderPass(vkContext->device, vkContext->renderPass, nullptr);
    vkDestroyDevice(vkContext->device, nullptr);
    vkDestroyInstance(vkContext->instance, nullptr);
}
