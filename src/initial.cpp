#include "vulkanrender.h"
#include "imguiloader.h"
#include "sdlwindow.h"

//int EngineSDLVulkanStart(std::function<void(VulkanContext* vkContext)> initLambda, std::function<void(SDL_Event* event)> eventLambda, std::function<void(VulkanContext* vkContext)> logicLambda, std::function<void(void)> imguiLambda) {
//	SDL_Window* window = SDLWindowInit("Vulkan Engine", 1280, 720, (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));
//
//	VulkanContext vkContext{};
//	VulkanInit(&vkContext, [window](VulkanContext* ctx) -> VkSurfaceKHR {
//		return SDLVulkanCreateSurface(ctx->instance, window);
//		});
//
//	VkDescriptorPool imguiPool = ImGuiSDLVulkanStartup(window, &vkContext);
//
//	initLambda(&vkContext);
//
//	while (1) {
//		if (SDLHandleEvents([=](SDL_Event* event) {
//			ImGui_ImplSDL2_ProcessEvent(event);
//			eventLambda(event);
//		}) < 0) break;
//		logicLambda(&vkContext);
//		ImGuiSDLVulkanDraw([=] {
//			imguiLambda();
//		});
//		VulkanDraw(&vkContext);
//	}
//
//	vkDeviceWaitIdle(vkContext.device);
//	ImGuiSDLVulkanShutdown(&vkContext, imguiPool);
//	SDL_DestroyWindow(window);
//	VulkanShutdown(&vkContext);
//	return 0;
//}
//
//int main(int argc, char* argv[]) {
//	//Renderer2D* renderer;
//
//	auto initLambda = [=](VulkanContext* vkContext) {
//		// Define triangle vertices and indices
//		Vertex triangleVertices[] = {
//			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // bottom-center, red
//			{{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}}, // top-right, green
//			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // top-left, blue
//		};
//		uint16_t indices[] = { 0, 1, 2 };
//
//		// Create buffers
//		int32_t vertexBufferId = VulkanCreateVertexBuffer(vkContext, triangleVertices, sizeof(triangleVertices));
//		int32_t indexBufferId = CreateIndexBuffer(vkContext, indices, sizeof(indices));
//	
//		// Create pipeline
//		int32_t pipelineIndex = VulkanCreateGraphicsPipeline(vkContext, "base", [=](VertexInputDescription* vertexInputDesc) {
//			VulkanCreateVertexBinding(vertexInputDesc, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
//			VulkanCreateVertexAttribute(vertexInputDesc, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
//			VulkanCreateVertexAttribute(vertexInputDesc, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
//		});
//
//
//
//		// Create commands
//		VulkanBindCommandBuffers(vkContext, [=](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {
//			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext->pipelines[pipelineIndex]->pipeline);
//			VkDeviceSize offsets[] = { 0 };
//			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkContext->buffers[vertexBufferId]->buffer, offsets);
//			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
//
//			Renderer2D renderer = Renderer2D_Init(vkContext);
//
//			Renderer2D_BeginFrame(&renderer);
//			Renderer2D_DrawQuad(&renderer, { 100, 100 }, { 200, 100 }, { 1, 0, 0, 1 });
//			Renderer2D_DrawBorder(&renderer, { 110, 110 }, { 180, 80 }, { 0, 1, 0, 1 }, 2.0f);
//
//			Renderer2D_EndFrame(&renderer, commandBuffer);
//			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
//
//
//		});
//
//	};
//
//	auto eventLambda = [=](SDL_Event* event) {
//
//	};
//
//	auto logicLambda = [=](VulkanContext* vkContext) {
//
//	};
//
//	auto imguiLambda = [=] {
//		ImGui::Button("teste");
//	};
//
//	return EngineSDLVulkanStart(initLambda, eventLambda, logicLambda, imguiLambda);
//}

//typedef struct {
//	float pos[2];  // 2D position (x, y)
//	float color[3]; // RGB color
//} Vertex;



struct Vertex {
    float pos[2];   // unit quad: (0,0),(1,0),(1,1),(0,1)
};

struct QuadInstance {
    float pos[2];   // position in NDC or pixels
    float size[2];  // width and height
    float color[4]; // RGBA
};

typedef struct {
    int32_t maxQuads;
    int32_t vertexBufferId;
    int32_t indexBufferId;
    int32_t instanceBufferId;
    int32_t pipelineIndex;
    std::vector<QuadInstance> quads;
} InstancedRenderer;

InstancedRenderer RendererInstanced(VulkanContext* vkContext) {
    InstancedRenderer iRenderer;

    // create base quad
    Vertex quadVertices[] = {
        {{0.0f, 0.0f}},
        {{1.0f, 0.0f}},
        {{1.0f, 1.0f}},
        {{0.0f, 1.0f}}
    };

    uint16_t quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

    iRenderer.vertexBufferId = VulkanCreateVertexBuffer(vkContext, quadVertices, sizeof(quadVertices));
    iRenderer.indexBufferId = CreateIndexBuffer(vkContext, quadIndices, sizeof(quadIndices));

    // create instanced
    iRenderer.maxQuads = 10000;
    iRenderer.instanceBufferId = VulkanCreateVertexBuffer(vkContext, NULL, 10000 * sizeof(QuadInstance));

    // create pipeline
    iRenderer.pipelineIndex = VulkanCreateGraphicsPipeline(vkContext, "2d", [=](VertexInputDescription* vertexInputDesc) {
        VulkanCreateVertexBinding(vertexInputDesc, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);      // unit quad
        VulkanCreateVertexAttribute(vertexInputDesc, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
        VulkanCreateVertexBinding(vertexInputDesc, 1, sizeof(QuadInstance), VK_VERTEX_INPUT_RATE_INSTANCE); // per-instance
        VulkanCreateVertexAttribute(vertexInputDesc, 1, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadInstance, pos));
        VulkanCreateVertexAttribute(vertexInputDesc, 1, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadInstance, size));
        VulkanCreateVertexAttribute(vertexInputDesc, 1, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(QuadInstance, color));
    });

    return iRenderer;
}

void RendererInstancedCmd(const InstancedRenderer* iRenderer, const VulkanContext* vkContext, VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext->pipelines[iRenderer->pipelineIndex]->pipeline);

    VkDeviceSize offsets[] = { 0, 0 };
    VkBuffer vertexBuffers[] = {
        vkContext->buffers[iRenderer->vertexBufferId]->buffer,    // quad
        vkContext->buffers[iRenderer->instanceBufferId]->buffer   // instance
    };
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, vkContext->buffers[iRenderer->indexBufferId]->buffer, 0, VK_INDEX_TYPE_UINT16);

    // Draw all quads in one call
    vkCmdDrawIndexed(commandBuffer, 6, iRenderer->maxQuads, 0, 0, 0);
}

void RendererInstancedStart(InstancedRenderer* iRenderer) {
    iRenderer->quads.clear();
}

void RendererInstancedEnd(InstancedRenderer* iRenderer, VulkanContext* vkContext) {
    VulkanUpdateVertexBuffer(vkContext, iRenderer->instanceBufferId, iRenderer->quads.data(), iRenderer->quads.size() * sizeof(QuadInstance));
}

int main(int argc, char* argv[]) {
	SDL_Window* window = SDLWindowInit("Vulkan Engine", 1280, 720, (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));

	VulkanContext vkContext{};
	VulkanInit(&vkContext, [window](VulkanContext* ctx) -> VkSurfaceKHR {
		return SDLVulkanCreateSurface(ctx->instance, window);
	});

	VkDescriptorPool imguiPool = ImGuiSDLVulkanStartup(window, &vkContext);

    struct VertexPosColor {
        float pos[2];   // unit quad: (0,0),(1,0),(1,1),(0,1)
        float color[3];
    };

    // Rectangle in NDC coordinates (-1..1 range)
    VertexPosColor rectVertices[4] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // bottom-left, red
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // bottom-right, green
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // top-right, blue
        {{-0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // top-left, yellow
    };

    // Two triangles → 6 indices
    uint16_t rectIndices[] = {
        0, 1, 2,   // first triangle
        2, 3, 0    // second triangle
    };

    // Create buffers
    int32_t vertexBufferId = VulkanCreateVertexBuffer(&vkContext, rectVertices, sizeof(rectVertices));
    int32_t indexBufferId = CreateIndexBuffer(&vkContext, rectIndices, sizeof(rectIndices));

    // Create pipeline
    int32_t pipelineIndex = VulkanCreateGraphicsPipeline(&vkContext, "base",
        [=](VertexInputDescription* vertexInputDesc) {
            VulkanCreateVertexBinding(vertexInputDesc, 0, sizeof(VertexPosColor), VK_VERTEX_INPUT_RATE_VERTEX);
            VulkanCreateVertexAttribute(vertexInputDesc, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexPosColor, pos));
            VulkanCreateVertexAttribute(vertexInputDesc, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPosColor, color));
        });

    InstancedRenderer iRenderer = RendererInstanced(&vkContext);

    // Create commands
    VulkanBindCommandBuffers(&vkContext, [=](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            vkContext.pipelines[pipelineIndex]->pipeline);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1,
            &vkContext.buffers[vertexBufferId]->buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer,
            vkContext.buffers[indexBufferId]->buffer, 0, VK_INDEX_TYPE_UINT16);

        // draw 6 indices = 2 triangles = rectangle
        vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

        RendererInstancedCmd(&iRenderer, &vkContext, commandBuffer);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    });

    float pos = -0.2f;

	while (1) {
        RendererInstancedStart(&iRenderer);

        pos += 0.00005f;

        iRenderer.quads.push_back({ { pos, -0.5f }, { 0.4f, 0.4f }, {1,0,0,1} }); // example rectangle
        iRenderer.quads.push_back({ {  0.3f,  0.0f }, { 0.2f, 0.3f }, {0,1,0,1} }); // another rectangle
        iRenderer.quads.push_back({ { -0.6f, -0.5f }, { 0.5f, 0.5f }, {0,0,1,1} }); // example rectangle
        iRenderer.quads.push_back({ {  0.0f,  0.0f }, { 0.3f, 0.4f }, {0,1,1,1} }); // another rectangle

        RendererInstancedEnd(&iRenderer, &vkContext);
        //rectVertices[0].pos[1] -= 0.001f;
        //VulkanUpdateVertexBuffer(&vkContext, vertexBufferId, rectVertices, sizeof(rectVertices));

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
