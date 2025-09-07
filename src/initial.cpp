#include "vulkanrender.h"
#include "imguiloader.h"
#include "sdlwindow.h"

typedef struct Vertex {
	float pos[2];  // 2D position (x, y)
	float color[3]; // RGB color
} Vertex;

int main(int argc, char* argv[]) {
	SDL_Window* window = SDLWindowInit("Vulkan Engine", 1280, 720, (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));

	VulkanContext vkContext{};
	VulkanInit(&vkContext, [window](VulkanContext* ctx) -> VkSurfaceKHR {
		return SDLVulkanCreateSurface(ctx->instance, window);
	});

	VkDescriptorPool imguiPool = ImGuiSDLVulkanStartup(window, &vkContext);

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

	// Create commands
	VulkanBindCommandBuffers(&vkContext, [=](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.pipelines[pipelineIndex]->pipeline);
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkContext.buffers[vertexBufferId]->buffer, offsets);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	});

	while (1) {
		if (SDLHandleEvents([=](SDL_Event* event) {
			ImGui_ImplSDL2_ProcessEvent(event);
		}) < 0) break;
		ImGuiSDLVulkanDraw([=] {
			ImGui::Button("teste");
		});
		VulkanDraw(&vkContext);
	}

	vkDeviceWaitIdle(vkContext.device);
	ImGuiSDLVulkanShutdown(&vkContext, imguiPool);
	SDL_DestroyWindow(window);
	VulkanShutdown(&vkContext);
	return 0;
}

//Uniform buffers(e.g., for MVP transforms)
//Descriptor sets for textures or more complex shaders
//Depth buffer if you move beyond 2D rendering
