#include "vulkanrender.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

SDL_Window* WindowInit(VkExtent2D swapchainExtent) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

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

int32_t SDLHandleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT) {
			return -1;
		}
	}
}

VkDescriptorPool ImGuiStartup(SDL_Window* window, VulkanContext* vkContext) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // optional
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // optional
	ImGui_ImplSDL2_InitForVulkan(window);
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
	{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
	{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
	{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	VkDescriptorPool imguiPool;
	CHECK_VULKAN_RESULT(vkCreateDescriptorPool(vkContext->device, &pool_info, nullptr, &imguiPool));
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.RenderPass = vkContext->renderPass;
	init_info.Instance = vkContext->instance;
	init_info.PhysicalDevice = vkContext->physicalDevice;
	init_info.Device = vkContext->device;
	init_info.QueueFamily = vkContext->queueFamilyIndex;
	init_info.Queue = vkContext->graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = imguiPool; // create a pool with IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE
	init_info.MinImageCount = VK_REQUIRED_IMAGE_COUNT;
	init_info.ImageCount = VK_REQUIRED_IMAGE_COUNT;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	//dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	VkFormat pipelineformat = VK_FORMAT_R8G8B8A8_UNORM;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &pipelineformat;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();
	return imguiPool;
}

void ImGuiDraw() {
	// imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	//some imgui UI to test
	ImGui::ShowDemoWindow();

	//make imgui calculate internal draw structures
	ImGui::Render();
}

void ImGuiShutdown(VulkanContext *vkContext, VkDescriptorPool imguiPool) {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(vkContext->device, imguiPool, nullptr);
}

typedef struct Vertex {
	float pos[2];  // 2D position (x, y)
	float color[3]; // RGB color
} Vertex;

int main(int argc, char* argv[]) {
	VkExtent2D swapchainExtent = { 1280, 720 };
	SDL_Window* window = WindowInit(swapchainExtent);

	VulkanContext vkContext{};
	VulkanInit(&vkContext, [window](VulkanContext* ctx) -> VkSurfaceKHR {
		return VulkanCreateSurface(ctx->instance, window);
	});

	VkDescriptorPool imguiPool = ImGuiStartup(window, &vkContext);

	// Define triangle vertices and indices
	Vertex triangleVertices[] = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // bottom-center, red
		{{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}}, // top-right, green
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // top-left, blue
	};
	uint16_t indices[] = { 0, 1, 2 };

	// Create buffers
	int32_t vertexBufferId = VulkanCreateVertexBuffer(&vkContext, triangleVertices, sizeof(triangleVertices));
	int32_t indexBufferId = CreateIndexBuffer(&vkContext, indices, sizeof(indices));
	
	// Create pipeline
	int32_t pipelineIndex = VulkanCreateGraphicsPipeline(&vkContext, "base", [=](VertexInputDescription* vertexInputDesc) {
		VulkanCreateVertexBinding(vertexInputDesc, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		VulkanCreateVertexAttribute(vertexInputDesc, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
		VulkanCreateVertexAttribute(vertexInputDesc, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
	});

	VkViewport viewportFixed = { 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
	VkRect2D scissorFixed = { {0, 0}, {800, 600} };
	auto commandBufferSetup = [=](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.pipelines[pipelineIndex]->pipeline);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkContext.buffers[vertexBufferId]->buffer, offsets);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	};
	VulkanBindCommandBuffers(&vkContext, commandBufferSetup);

	while (1) {
		if (SDLHandleEvents() < 0) break;
		ImGuiDraw();
		VulkanDraw(&vkContext);
	}

	vkDeviceWaitIdle(vkContext.device);
	ImGuiShutdown(&vkContext, imguiPool);
	SDL_DestroyWindow(window);
	VulkanShutdown(&vkContext);
	return 0;
}

//Uniform buffers(e.g., for MVP transforms)
//Descriptor sets for textures or more complex shaders
//Depth buffer if you move beyond 2D rendering
