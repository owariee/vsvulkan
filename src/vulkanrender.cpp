#include "vulkanrender.h"

/*
 * Creates a Vulkan instance with required extensions and validation layers.
 */
VkInstance VulkanCreateInstance() {
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

/*
 * Selects first physical device (GPU) from the available devices.
 */
VkPhysicalDevice VulkanSelectPhysicalDevice(VkInstance instance) {
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

/*
 * Select first queue family that supports graphics operations.
 */
uint32_t VulkanGetGraphicsQueueFamilyIndex(VkPhysicalDevice physicalDevice) {
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

/*
 * Creates a logical device from the selected physical device.
 */
VkDevice VulkanCreateLogicalDevice(VkPhysicalDevice physicalDevice)
{
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

/*
 * Retrieves the graphics queue from the logical device.
 */
VkQueue VulkanGetGraphicsQueue(VkDevice device, uint32_t queueFamilyIndex)
{
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    return queue;
}

/*
 * Creates a command pool for allocating command buffers.
 */
VkCommandPool VulkanCreateCommandPool(VkDevice device, uint32_t queueFamilyIndex) {
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

/*
 * Creates a simple render pass with one color attachment.
 */
VkRenderPass VulkanCreateRenderPass(VkDevice device, VkFormat format) {
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

/*
 * Allocates a single command buffer from the command pool.
 */
VkCommandBuffer VulkanAllocateCommandBuffer(VkDevice device, VkCommandPool commandPool) {
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

/*
 * Allocates multiple command buffers from the command pool.
 */
void VulkanAllocateCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* commandBuffers, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        commandBuffers[i] = VulkanAllocateCommandBuffer(device, commandPool);
    }
}

/*
 * Frees multiple command buffers and releases their memory.
 */
void VulkanDestroyCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* commandBuffers, uint32_t count) {
    vkFreeCommandBuffers(device, commandPool, count, commandBuffers);
}

/*
 * Creates a binary semaphore for GPU synchronization.
 */
VkSemaphore VulkanCreateSemaphore(VkDevice device) {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore semaphore;
    VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
    CHECK_VULKAN_RESULT(result);
    return semaphore;
}

/*
 * Creates multiple semaphores for synchronizing rendering operations.
 */
void VulkanCreateSemaphores(VkDevice device, uint32_t swapchainImageCount, VkSemaphore* semaphores) {
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        semaphores[i] = VulkanCreateSemaphore(device);
    }
}

/*
 * Destroys multiple semaphores and releases their memory.
 */
void VulkanDestroySemaphores(VkDevice device, VkSemaphore* semaphores, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroySemaphore(device, semaphores[i], nullptr);
    }
}

/*
 * Creates a fence for CPU-GPU synchronization.
 */
VkFence VulkanCreateFence(VkDevice device, bool signaled) {
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

/*
 * Creates a swapchain for presenting images to the surface.
 */
VkSwapchainKHR VulkanCreateSwapchainInstance(VkDevice device, VkSurfaceKHR surface, VkExtent2D extent) {
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

/*
 * Retrieves the images from the swapchain.
 */
VkImage* VulkanGetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, uint32_t* imageCountOut, VkImage* swapchainImages) {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr); // Get count
    printf("Swapchain image count: %u\n", imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages); // Get images
    *imageCountOut = imageCount;
    return swapchainImages;
}

/*
 * Creates a 2D image view for the given image and format.
 */
VkImageView VulkanCreateImageView(VkDevice device, VkImage image, VkFormat format) {
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

/*
 * Creates image views for all swapchain images.
 */
void VulkanCreateImageViews(VkDevice device, VkImage* swapchainImages, uint32_t swapchainImageCount, VkImageView* swapchainImageViews) {
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        swapchainImageViews[i] = VulkanCreateImageView(device, swapchainImages[i], VK_FORMAT_R8G8B8A8_UNORM);
    }
}

/*
 * Creates a framebuffer for the given render pass and image view.
 */
VkFramebuffer VulkanCreateFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, VkExtent2D extent) {
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

/*
 * Creates framebuffers for all swapchain image views.
 */
void VulkanCreateFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView* imageViews, VkExtent2D extent, uint32_t count, VkFramebuffer* framebuffers) {
    for (uint32_t i = 0; i < count; i++) {
        framebuffers[i] = VulkanCreateFramebuffer(device, renderPass, imageViews[i], extent);
    }
}

/*
 * Destroys multiple image views and releases their memory.
 */
void VulkanDestroyImageViews(VkDevice device, VkImageView* imageViews, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyImageView(device, imageViews[i], nullptr);
    }
}

/*
 * Destroys multiple framebuffers and releases their memory.
 */
void VulkanDestroyFramebuffers(VkDevice device, VkFramebuffer* framebuffers, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }
}

/*
 * Determines the appropriate swapchain extent based on surface capabilities.
 */
VkExtent2D GetSurfaceExtent(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
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

/*
 * Creates the swapchain and its associated resources.
 */
void VulkanCreateSwapchain(VulkanContext* vkContext) {
    uint32_t swapchainImageCount;
	vkContext->surfaceSize = GetSurfaceExtent(vkContext->physicalDevice, vkContext->surface);
    vkContext->swapchain.instance = VulkanCreateSwapchainInstance(vkContext->device, vkContext->surface, vkContext->surfaceSize);
    VulkanGetSwapchainImages(vkContext->device, vkContext->swapchain.instance, &swapchainImageCount, vkContext->swapchain.images);
    VulkanCreateImageViews(vkContext->device, vkContext->swapchain.images, VK_REQUIRED_IMAGE_COUNT, vkContext->swapchain.imageViews);
    VulkanCreateFramebuffers(vkContext->device, vkContext->renderPass, vkContext->swapchain.imageViews, vkContext->surfaceSize, VK_REQUIRED_IMAGE_COUNT, vkContext->swapchain.framebuffers);
}

/*
 * Destroys the swapchain and its associated resources.
 */
void VulkanDestroySwapchain(VulkanContext* vkContext) {
    VulkanDestroyFramebuffers(vkContext->device, vkContext->swapchain.framebuffers, VK_REQUIRED_IMAGE_COUNT);
    VulkanDestroyImageViews(vkContext->device, vkContext->swapchain.imageViews, VK_REQUIRED_IMAGE_COUNT);
    vkDestroySwapchainKHR(vkContext->device, vkContext->swapchain.instance, nullptr);
}

/*
 * record commands into an command buffer
 */
void VulkanRecordCommandBuffer(
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

	commandsLambda(vkContext->surfaceSize, commandBuffer); // Call the provided lambda to record custom commands

    // End the render pass
    vkCmdEndRenderPass(commandBuffer);

    // Finish recording commands
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record command buffer!\n");
        exit(1);
    }
}

/*
 * Records multiple command buffers for rendering.
 */
void VulkanRecordCommandBuffers(VulkanContext* vkContext)
{
    for (uint32_t i = 0; i < VK_REQUIRED_IMAGE_COUNT; i++) {
        VulkanRecordCommandBuffer(vkContext, vkContext->commandBuffers[i], vkContext->swapchain.framebuffers[i], vkContext->commandsLambda);
    }
}

/*
 * Recreates the swapchain, typically in response to window resizing.
 */
void VulkanRecreateSwapchain(VulkanContext* vkContext) {
    vkDeviceWaitIdle(vkContext->device);
    VulkanDestroySwapchain(vkContext);
    VulkanCreateSwapchain(vkContext);
    VulkanRecordCommandBuffers(vkContext);
    vkDeviceWaitIdle(vkContext->device);
}

/*
 * Bind a new set of commands and records multiple command buffers for rendering.
 */
void VulkanBindCommandBuffers(VulkanContext* vkContext, std::function<void(VkExtent2D surfaceSize, VkCommandBuffer commandBuffer)> commandsLambda)
{
    vkContext->commandsLambda = commandsLambda;
}

/*
 * Present queue
 */
VkResult VulkanQueuePresent(
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

/*
 * Submit command buffers to queue
 */
void VulkanSubmitCommandBuffer(
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

/*
 * Acquire next swapchain image 
 */
uint32_t VulkanAcquireNextImage(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore) {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);
    //CHECK_VULKAN_RESULT(result);
    printf("Acquired image index: %u\n", imageIndex);
    return imageIndex;
}

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

void VulkanInit(VulkanContext* vkContext, std::function<void(VulkanContext* vkContext)> createSurface) {
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
	createSurface(vkContext); // will set surface
    VulkanCreateSwapchain(vkContext);
    vkContext->currentFrame = 0;
    vkContext->MAX_FRAMES_IN_FLIGHT = VK_REQUIRED_IMAGE_COUNT;
    vkContext->shouldRecreateSwapchain = false;
    vkContext->commandsLambda = [=](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {};
}

void VulkanShutdown(VulkanContext* vkContext) {
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
