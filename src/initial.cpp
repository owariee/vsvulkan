#include "vulkanrender.h"
#include "imguiloader.h"
#include "sdlwindow.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

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
    glm::mat4 projection;
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

    PipelineDescription pipelineDesc = {
        .perVertexStride = sizeof(Vertex),
        .perInstanceStride = sizeof(QuadInstance),
        .vertexInputs = {
            {.location = 0, .format = VULKAN_FORMAT_R32G32_SFLOAT,       .offset = offsetof(Vertex, pos),           .rate = VULKAN_RATE_PER_VERTEX },
            {.location = 1, .format = VULKAN_FORMAT_R32G32_SFLOAT,       .offset = offsetof(QuadInstance, pos),     .rate = VULKAN_RATE_PER_INSTANCE },
            {.location = 2, .format = VULKAN_FORMAT_R32G32_SFLOAT,       .offset = offsetof(QuadInstance, size),    .rate = VULKAN_RATE_PER_INSTANCE },
            {.location = 3, .format = VULKAN_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(QuadInstance, color),   .rate = VULKAN_RATE_PER_INSTANCE }
        },
        .pushConstants = {
            {.offset = 0, .size = sizeof(glm::mat4), .stages = { VULKAN_STAGE_VERTEX } }
        },
        .descriptorSets = {
            {
                .set = 0,
                .bindings = {
                    {.binding = 0, .type = VULKAN_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = { VULKAN_STAGE_VERTEX } }
                }
            }
        },
    };

    PrintPipelineDescription(pipelineDesc);

    PipelineDescription pipelineDescTest = {
        .pushConstants = VulkanProducePushConstants("../shaders/2d.vert.spv", "../shaders/2d.frag.spv"),
		.descriptorSets = VulkanProduceDescriptorSet("../shaders/2d.vert.spv", "../shaders/2d.frag.spv"),
    };

    PrintPipelineDescription(pipelineDescTest);

    // create pipeline
	iRenderer.pipelineIndex = VulkanSetupPipeline(vkContext, "2d", &pipelineDesc);

    //iRenderer.projection = glm::ortho(0.0f, (float)vkContext->surfaceSize.width, 0.0f, (float)vkContext->surfaceSize.height);
    iRenderer.projection = glm::ortho(-1.0f, (float)vkContext->surfaceSize.width / (float)vkContext->surfaceSize.height, -1.0f, 1.0f);

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

    vkCmdPushConstants(
        commandBuffer,
        vkContext->pipelines[iRenderer->pipelineIndex]->layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(glm::mat4),
        &iRenderer->projection
    );

    // Draw all quads in one call
    vkCmdDrawIndexed(commandBuffer, 6, iRenderer->maxQuads, 0, 0, 0);
}

void RendererInstancedStart(InstancedRenderer* iRenderer) {
    iRenderer->quads.clear();
}

void RendererInstancedEnd(InstancedRenderer* iRenderer, VulkanContext* vkContext) {
    VulkanUpdateVertexBuffer(vkContext, iRenderer->instanceBufferId, iRenderer->quads.data(), iRenderer->quads.size() * sizeof(QuadInstance));
    //iRenderer->projection = glm::ortho(0.0f, (float)vkContext->surfaceSize.width, 0.0f, (float)vkContext->surfaceSize.height);

    float width = (float)vkContext->surfaceSize.width / (float)vkContext->surfaceSize.height;
    float height = 1.0f;

    if (width < 1.0f)
    {
        width = 1.0f;
        height = (float)vkContext->surfaceSize.height / (float)vkContext->surfaceSize.width;
    }

    iRenderer->projection = glm::ortho(-width, width, -height, height);
    printf("Surface size: %u %u\n", vkContext->surfaceSize.width, vkContext->surfaceSize.height);
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
    VulkanBindCommandBuffers(&vkContext, [&](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            vkContext.pipelines[pipelineIndex]->pipeline);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1,
            &vkContext.buffers[vertexBufferId]->buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer,
            vkContext.buffers[indexBufferId]->buffer, 0, VK_INDEX_TYPE_UINT16);

        // draw 6 indices = 2 triangles = rectangle
        vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        RendererInstancedCmd(&iRenderer, &vkContext, commandBuffer);
        });


    //float pos = -0.2f;

	while (1) {
        RendererInstancedStart(&iRenderer);

        //pos += 0.00005f;

        //iRenderer.quads.push_back({ { pos, -0.5f }, { 0.4f, 0.4f }, {1,0,0,1} }); // example rectangle
        //iRenderer.quads.push_back({ {  0.3f,  0.0f }, { 0.2f, 0.3f }, {0,1,0,1} }); // another rectangle
        //iRenderer.quads.push_back({ { -0.6f, -0.5f }, { 0.5f, 0.5f }, {0,0,1,1} }); // example rectangle
        //iRenderer.quads.push_back({ {  0.0f,  0.0f }, { 0.3f, 0.4f }, {0,1,1,1} }); // another rectangle


        float widthRatio = (float)vkContext.surfaceSize.width / (float)vkContext.surfaceSize.height;
        float heightRatio = 1.0f;

        if (widthRatio < 1.0f)
        {
            widthRatio = 1.0f;
            heightRatio = (float)vkContext.surfaceSize.height / (float)vkContext.surfaceSize.width;
        }


        float virtualWidthRatio = (float)1920 / (float)1080;
        float virtualHeightRatio = 1.0f;

        //iRenderer.quads.push_back({ { -widthRatio, -heightRatio }, { (float)vkContext.surfaceSize.width, (float)vkContext.surfaceSize.height }, {1,1,1,1} }); // example rectangle
        //iRenderer.quads.push_back({ { -virtualWidthRatio, -heightRatio }, { (float)virtualWidthRatio*2, (float)vkContext.surfaceSize.height }, {0,0,0,1} }); // example rectangle


        iRenderer.quads.push_back({ { -widthRatio, -heightRatio }, { (float)widthRatio - virtualWidthRatio, (float)heightRatio*2 }, {1,1,1,1} }); // example rectangle
        iRenderer.quads.push_back({ { virtualWidthRatio, -heightRatio }, { (float)widthRatio - virtualWidthRatio, (float)heightRatio*2 }, {1,1,1,1} }); // example rectangle

        iRenderer.quads.push_back({ { -widthRatio, -heightRatio }, { (float)widthRatio*2, (float)heightRatio - virtualHeightRatio }, {1,1,1,1} }); // example rectangle
        iRenderer.quads.push_back({ { -widthRatio, virtualHeightRatio }, { (float)widthRatio*2, (float)heightRatio - virtualHeightRatio }, {1,1,1,1} }); // example rectangle


        //widthRatio == width
        //virtualWidthRatio == x

        //virtualWidthRatio * width = widthRatio * x
        //(virtualWidthRatio * width) / widthRatio = x

        if (widthRatio > virtualWidthRatio) {
            printf("Real game resolution width limited %f %f\n", (virtualWidthRatio * (float)vkContext.surfaceSize.width) / widthRatio, (float)vkContext.surfaceSize.height);
        }

        if (heightRatio > virtualHeightRatio) {
            printf("Real game resolution height limited %f %f\n", (float)vkContext.surfaceSize.width, (virtualHeightRatio* (float)vkContext.surfaceSize.height) / heightRatio);
        }


      

        //iRenderer.quads.push_back({ { virtualWidthRatio, -heightRatio }, { (float)widthRatio - virtualWidthRatio, (float)heightRatio * 2 }, {1,1,1,1} }); // example rectangle

        //iRenderer.quads.push_back({ { -virtualWidthRatio, -heightRatio }, { (float)virtualWidthRatio * 2, (float)vkContext.surfaceSize.height }, {0,0,0,1} }); // example rectangle


        iRenderer.quads.push_back({ { 0.0f, 0.0f }, { 1.0f, 1.0f }, {1,0,0,1} }); // example rectangle
        //iRenderer.quads.push_back({ {  0.3f,  0.0f }, { 0.2f, 0.3f }, {0,1,0,1} }); // another rectangle
        //iRenderer.quads.push_back({ { -0.6f, -0.5f }, { 0.5f, 0.5f }, {0,0,1,1} }); // example rectangle
        //iRenderer.quads.push_back({ {  0.0f,  0.0f }, { 0.3f, 0.4f }, {0,1,1,1} }); // another rectangle

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
