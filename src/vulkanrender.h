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
#include <vector>
#include <optional>
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
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    std::vector<VkDescriptorSetLayoutBinding> layoutBinding;
    std::vector<VkPushConstantRange> constantRange;
} VertexInputDescription;

typedef struct {
    VkDeviceMemory memory;
    VkBuffer buffer;
} VulkanBuffer;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptorSetLayout;
} VulkanPipeline;

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
    std::vector<std::optional<VulkanPipeline>> pipelines;
    std::vector<std::optional<VulkanBuffer>> buffers;
} VulkanContext;

void VulkanInit(VulkanContext* vkContext, std::function<VkSurfaceKHR(VulkanContext* vkContext)> createSurface);
void VulkanShutdown(VulkanContext* vkContext);
void VulkanDraw(VulkanContext* vkContext);
void VulkanBindCommandBuffers(VulkanContext* vkContext, std::function<void(VkExtent2D surfaceSize, VkCommandBuffer commandBuffer)> commandsLambda);

int32_t VulkanCreateGraphicsPipeline(
    VulkanContext* vkContext,
    const char* shaderBaseName,
    std::function<void(VertexInputDescription* vertexInputDescription)> vertexInputDescriptionLambda);

int32_t VulkanCreateVertexBuffer(VulkanContext* vkContext, const void* vertexData, VkDeviceSize size);
int32_t CreateIndexBuffer(VulkanContext* vkContext, const void* indexData, VkDeviceSize size);

void VulkanCreateVertexAttribute(
    VertexInputDescription* vertexInputDescription,
    uint32_t binding,
    uint32_t location,
    VkFormat format,
    uint32_t offset);

void VulkanCreateVertexBinding(
    VertexInputDescription* vertexInputDescription,
    uint32_t binding,
    uint32_t stride,
    VkVertexInputRate inputRate);

void VulkanDeleteBuffer(VulkanContext* vkContext, uint32_t index);
void VulkanDeletePipeline(VulkanContext* vkContext, uint32_t index);

void VulkanCreatePushConstant(
    VertexInputDescription* vertexInputDescription,
    VkShaderStageFlags stageFlags,
    uint32_t offset,
    uint32_t size);

void VulkanCreateDescriptorSetLayoutBinding(
    VertexInputDescription* vertexInputDescription,
    uint32_t binding,
    VkDescriptorType descriptorType,
    uint32_t descriptorCount,
    VkShaderStageFlags stageFlags);

void VulkanUpdateVertexBuffer(VulkanContext* vkContext, int32_t bufferId, const void* vertexData, VkDeviceSize size);
