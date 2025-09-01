#define VK_USE_PLATFORM_WIN32_KHR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

VkInstance VulkanCreateInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";
	appInfo.apiVersion = VK_API_VERSION_1_2;

	const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = 2;
	createInfo.ppEnabledExtensionNames = extensions;
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = layers;

	VkInstance instance;
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	CHECK_VULKAN_RESULT(result);

	return instance;
}

VkPhysicalDevice VulkanSelectPhysicalDevice(VkInstance instance)
{
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

uint32_t VulkanGetGraphicsQueueFamilyIndex(VkPhysicalDevice physicalDevice)
{
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

VkQueue VulkanGetGraphicsQueue(VkDevice device, uint32_t queueFamilyIndex)
{
	VkQueue queue;
	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
	return queue;
}

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

SDL_Window* WindowInit(VkExtent2D swapchainExtent) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	SDL_Window* _window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		swapchainExtent.width,
		swapchainExtent.height,
		window_flags
	);

	return _window;
}

VkSurfaceKHR VulkanCreateSurface(VkInstance instance, SDL_Window* window) {
	VkSurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
		fprintf(stderr, "Failed to create Vulkan surface.\n");
		return VK_NULL_HANDLE;
	}
	return surface;
}

VkSwapchainKHR VulkanCreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkExtent2D extent) {
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

uint32_t VulkanAcquireNextImage(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore) {
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);
	CHECK_VULKAN_RESULT(result);
	printf("Acquired image index: %u\n", imageIndex);
	return imageIndex;
}

VkSemaphore VulkanCreateSemaphore(VkDevice device) {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore);
	CHECK_VULKAN_RESULT(result);
	return semaphore;
}

int32_t SDLHandleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return -1;
		}
	}
}

VkImage* VulkanGetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain, uint32_t* imageCountOut) {
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr); // Get count
	printf("Swapchain image count: %u\n", imageCount);
	VkImage* swapchainImages = (VkImage*)malloc(sizeof(VkImage) * imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages); // Get images
	*imageCountOut = imageCount;
	return swapchainImages;
}

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

VkImageView* VulkanCreateImageViews(VkDevice device, VkImage* swapchainImages, uint32_t swapchainImageCount) {

	VkImageView* swapchainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		swapchainImageViews[i] = VulkanCreateImageView(device, swapchainImages[i], VK_FORMAT_R8G8B8A8_UNORM);
	}

	return swapchainImageViews;
}

void VulkanDestroyImageViews(VkDevice device, VkImageView* imageViews, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		vkDestroyImageView(device, imageViews[i], nullptr);
	}
	free(imageViews);
}

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

VkFramebuffer* VulkanCreateFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView* imageViews, VkExtent2D extent, uint32_t count) {
	VkFramebuffer* framebuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer) * count);
	for (uint32_t i = 0; i < count; i++) {
		framebuffers[i] = VulkanCreateFramebuffer(device, renderPass, imageViews[i], extent);
	}
	return framebuffers;
}

void VulkanDestroyFramebuffers(VkDevice device, VkFramebuffer* framebuffers, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		vkDestroyFramebuffer(device, framebuffers[i], nullptr);
	}
	free(framebuffers);
}

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

VkCommandBuffer* VulkanAllocateCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count) {
	VkCommandBuffer* commandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * count);
	for (uint32_t i = 0; i < count; i++) {
		commandBuffers[i] = VulkanAllocateCommandBuffer(device, commandPool);
	}
	return commandBuffers;
}

void VulkanDestroyCommandBuffers(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* commandBuffers, uint32_t count) {
	vkFreeCommandBuffers(device, commandPool, count, commandBuffers);
	free(commandBuffers);
}

void VulkanRecordCommandBuffer(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent, VkPipeline pipeline, VkBuffer vertexBuffer, VkBuffer indexBuffer) {
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
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	// Begin render pass
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Set viewport (the region of the framebuffer you want to draw to)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	// Set scissor (cut-off area, usually matches viewport)
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Bind your graphics pipeline if you have one
	if (pipeline != VK_NULL_HANDLE) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}

	// **Bind your vertex buffer here**
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	// End the render pass
	vkCmdEndRenderPass(commandBuffer);

	// Finish recording commands
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to record command buffer!\n");
		exit(1);
	}
}

void VulkanRecordCommandBuffers(
	VkCommandBuffer* commandBuffers,
	VkFramebuffer* framebuffers,
	VkRenderPass renderPass,
	uint32_t count,
	VkExtent2D extent,
	VkPipeline pipeline,
	VkBuffer vertexBuffer,
	VkBuffer indexBuffer) 
{
	for (uint32_t i = 0; i < count; i++) { 
		VulkanRecordCommandBuffer(commandBuffers[i], renderPass, framebuffers[i], extent, pipeline, vertexBuffer, indexBuffer);
	}
}

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

void VulkanQueuePresent(
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
	vkQueuePresentKHR(graphicsQueue, &presentInfo);
}

VkSemaphore* VulkanCreateSemaphores(VkDevice device, uint32_t swapchainImageCount) {
	VkSemaphore* renderFinishedSemaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; ++i) {
		renderFinishedSemaphores[i] = VulkanCreateSemaphore(device);
	}

	return renderFinishedSemaphores;
}

void VulkanDestroySemaphores(VkDevice device, VkSemaphore* semaphores, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		vkDestroySemaphore(device, semaphores[i], nullptr);
	}
	free(semaphores);
}

VkShaderModule VulkanLoadShaderModule(VkDevice device, const char* filepath) {
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

VkPipelineShaderStageCreateInfo VulkanCreateShaderStage(VkDevice device, const char* filePath, VkShaderStageFlagBits stage, VkShaderModule* shaderModuleOut) {
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

VkBuffer VulkanCreateVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const void* vertexData, VkDeviceSize size, VkDeviceMemory* outMemory) {
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkBuffer vertexBuffer;
	if (vkCreateBuffer(device, &bufferInfo, NULL, &vertexBuffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create vertex buffer\n");
		return VK_NULL_HANDLE;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

	uint32_t memoryTypeIndex = 0;
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

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

	if (vkAllocateMemory(device, &allocInfo, NULL, outMemory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate vertex buffer memory\n");
		vkDestroyBuffer(device, vertexBuffer, NULL);
		return VK_NULL_HANDLE;
	}

	vkBindBufferMemory(device, vertexBuffer, *outMemory, 0);

	void* data;
	vkMapMemory(device, *outMemory, 0, size, 0, &data);
	memcpy(data, vertexData, (size_t)size);
	vkUnmapMemory(device, *outMemory);

	return vertexBuffer;
}

VkBuffer CreateIndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const void* indexData, VkDeviceSize size, VkDeviceMemory* outMemory) {
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VkBuffer indexBuffer;
	if (vkCreateBuffer(device, &bufferInfo, NULL, &indexBuffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create index buffer\n");
		return VK_NULL_HANDLE;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, indexBuffer, &memRequirements);

	uint32_t memoryTypeIndex = 0;
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

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

	if (vkAllocateMemory(device, &allocInfo, NULL, outMemory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate index buffer memory\n");
		vkDestroyBuffer(device, indexBuffer, NULL);
		return VK_NULL_HANDLE;
	}

	vkBindBufferMemory(device, indexBuffer, *outMemory, 0);

	void* data;
	vkMapMemory(device, *outMemory, 0, size, 0, &data);
	memcpy(data, indexData, (size_t)size);
	vkUnmapMemory(device, *outMemory);

	return indexBuffer;
}

typedef struct Vertex {
	float pos[2];  // 2D position (x, y)
	float color[3]; // RGB color
} Vertex;

VkPipeline VulkanCreateGraphicsPipeline(VkDevice device, VkPhysicalDevice physicalDevice, VkRenderPass renderPass, VkBuffer vertexBuffer, VkBuffer indexBuffer, VkPipelineLayout *pipelineLayoutOut) {
	// Load shaders
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	VkPipelineShaderStageCreateInfo vertShader = VulkanCreateShaderStage(device, "../shaders/base.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, &vertShaderModule);
	VkPipelineShaderStageCreateInfo fragShader = VulkanCreateShaderStage(device, "../shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, &fragShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShader, fragShader };

	// You will need to define your vertex input binding & attribute descriptions:
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[2] = {};

	// Position attribute (location = 0 in shader)
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	// Color attribute (location = 1 in shader)
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	// Setup vertex input state with these descriptions
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

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
	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = NULL;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = NULL;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create pipeline layout!\n");
		return VK_NULL_HANDLE;
	}

	// Viewport and scissor setup (fixed size for example)
	VkViewport viewport = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
	VkRect2D scissor = { {0, 0}, {800, 600} };
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

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
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline graphicsPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create graphics pipeline!\n");
		vkDestroyPipelineLayout(device, pipelineLayout, NULL);
		return VK_NULL_HANDLE;
	}

	*pipelineLayoutOut = pipelineLayout; // Return the created pipeline layout

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);

	return graphicsPipeline;
}

int main(int argc, char* argv[]) {
	VkInstance instance = VulkanCreateInstance();
	VkPhysicalDevice physicalDevice = VulkanSelectPhysicalDevice(instance);
	VkDevice device = VulkanCreateLogicalDevice(physicalDevice);
	VkExtent2D swapchainExtent = { 1280, 720 };
	SDL_Window* window = WindowInit(swapchainExtent);
	VkSurfaceKHR surface = VulkanCreateSurface(instance, window);
	VkSwapchainKHR swapchain = VulkanCreateSwapchain(device, surface, swapchainExtent);
	uint32_t swapchainImageCount;
	VkImage* swapchainImages = VulkanGetSwapchainImages(device, swapchain, &swapchainImageCount);
	VkImageView* swapchainImageViews = VulkanCreateImageViews(device, swapchainImages, swapchainImageCount);
	VkRenderPass renderPass = VulkanCreateRenderPass(device, VK_FORMAT_R8G8B8A8_UNORM);
	VkFramebuffer* framebuffers = VulkanCreateFramebuffers(device, renderPass, swapchainImageViews, swapchainExtent, swapchainImageCount);
	VkCommandPool commandPool = VulkanCreateCommandPool(device, 0); // we usually only need one command pool
	VkCommandBuffer* commandBuffers = VulkanAllocateCommandBuffers(device, commandPool, swapchainImageCount); // we need one command buffer per framebuffer
	
	
	// Define triangle vertices and indices
	Vertex triangleVertices[] = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // bottom-center, red
		{{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}}, // top-right, green
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // top-left, blue
	};
	uint16_t indices[] = { 0, 1, 2 };

	// Create vertex buffer with actual data
	VkDeviceMemory vertexBufferMemory;
	VkBuffer vertexBuffer = VulkanCreateVertexBuffer(device, physicalDevice, triangleVertices, sizeof(triangleVertices), &vertexBufferMemory);

	// Create index buffer with actual data
	VkDeviceMemory indexBufferMemory;
	VkBuffer indexBuffer = CreateIndexBuffer(device, physicalDevice, indices, sizeof(indices), &indexBufferMemory);
	
	
	
	
	VkPipelineLayout pipelineLayout; // we need this later to destroy the pipeline layout
	VkPipeline graphicsPipeline = VulkanCreateGraphicsPipeline(device, physicalDevice, renderPass, vertexBuffer, indexBuffer, &pipelineLayout);
	VulkanRecordCommandBuffers(commandBuffers, framebuffers, renderPass, swapchainImageCount, swapchainExtent, graphicsPipeline, vertexBuffer, indexBuffer);
	uint32_t queueFamilyIndex = VulkanGetGraphicsQueueFamilyIndex(physicalDevice);
	VkQueue graphicsQueue = VulkanGetGraphicsQueue(device, queueFamilyIndex);
	VkFence inFlightFence = VulkanCreateFence(device, false); // this fence is used to wait until a frame is finished rendering before starting a new one
	VkSemaphore* renderFinishedSemaphores = VulkanCreateSemaphores(device, swapchainImageCount);
	VkSemaphore* imagesAvailableSemaphores = VulkanCreateSemaphores(device, swapchainImageCount);

	uint32_t currentFrame = 0;
	const uint32_t MAX_FRAMES_IN_FLIGHT = swapchainImageCount;

	while (1) {
		if (SDLHandleEvents() < 0) break;
		vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &inFlightFence);

		VkSemaphore imageAvailableSemaphore = imagesAvailableSemaphores[currentFrame];
		VkSemaphore renderFinishedSemaphore = renderFinishedSemaphores[currentFrame];

		// Acquire next image from swapchain
		uint32_t imageIndex = VulkanAcquireNextImage(device, swapchain, imageAvailableSemaphore);
		
		VulkanSubmitCommandBuffer(
			graphicsQueue,
			commandBuffers,
			imageAvailableSemaphore,
			renderFinishedSemaphore,
			inFlightFence,
			imageIndex);

		VulkanQueuePresent(
			graphicsQueue,
			swapchain,
			renderFinishedSemaphore,
			imageIndex);

		// Advance to next frame
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		SDL_Delay(16); // Simulate ~60 FPS
	}

	vkDeviceWaitIdle(device);
	
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);


	VulkanDestroySemaphores(device, renderFinishedSemaphores, swapchainImageCount);
	VulkanDestroySemaphores(device, imagesAvailableSemaphores, swapchainImageCount);
	vkDestroyFence(device, inFlightFence, nullptr);
	VulkanDestroyCommandBuffers(device, commandPool, commandBuffers, swapchainImageCount);
	vkDestroyCommandPool(device, commandPool, nullptr);
	VulkanDestroyFramebuffers(device, framebuffers, swapchainImageCount);
	vkDestroyRenderPass(device, renderPass, nullptr);
	VulkanDestroyImageViews(device, swapchainImageViews, swapchainImageCount);
	free(swapchainImages);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	SDL_DestroyWindow(window);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
	return 0;
}

//Uniform buffers(e.g., for MVP transforms)
//Descriptor sets for textures or more complex shaders
//Depth buffer if you move beyond 2D rendering
//Swapchain recreation for window resizing
