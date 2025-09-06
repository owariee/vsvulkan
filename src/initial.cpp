#include "vulkanrender.h"

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
	VkExtent2D swapchainExtent = { 1280, 720 };
	SDL_Window* window = WindowInit(swapchainExtent);

	VulkanContext vkContext{};
	VulkanInit(&vkContext, [window](VulkanContext* ctx) {
		ctx->surface = VulkanCreateSurface(ctx->instance, window);
		});
	

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
	CHECK_VULKAN_RESULT(vkCreateDescriptorPool(vkContext.device, &pool_info, nullptr, &imguiPool));
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.RenderPass = vkContext.renderPass;
	init_info.Instance = vkContext.instance;
	init_info.PhysicalDevice = vkContext.physicalDevice;
	init_info.Device = vkContext.device;
	init_info.QueueFamily = vkContext.queueFamilyIndex;
	init_info.Queue = vkContext.graphicsQueue;
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

	// Define triangle vertices and indices
	Vertex triangleVertices[] = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // bottom-center, red
		{{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}}, // top-right, green
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // top-left, blue
	};
	uint16_t indices[] = { 0, 1, 2 };

	// Create vertex buffer with actual data
	VkDeviceMemory vertexBufferMemory;
	VkBuffer vertexBuffer = VulkanCreateVertexBuffer(vkContext.device, vkContext.physicalDevice, triangleVertices, sizeof(triangleVertices), &vertexBufferMemory);

	// Create index buffer with actual data
	VkDeviceMemory indexBufferMemory;
	VkBuffer indexBuffer = CreateIndexBuffer(vkContext.device, vkContext.physicalDevice, indices, sizeof(indices), &indexBufferMemory);
	
	VkPipelineLayout pipelineLayout; // we need this later to destroy the pipeline layout
	VkPipeline graphicsPipeline = VulkanCreateGraphicsPipeline(vkContext.device, vkContext.physicalDevice, vkContext.renderPass, vertexBuffer, indexBuffer, &pipelineLayout);
	auto commandBufferSetup = [=](VkExtent2D surfaceSize, VkCommandBuffer commandBuffer) {
		// Set viewport (the region of the framebuffer you want to draw to)
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)surfaceSize.width;
		viewport.height = (float)surfaceSize.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Set scissor (cut-off area, usually matches viewport)
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = surfaceSize;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Bind your graphics pipeline if you have one
		if (graphicsPipeline != VK_NULL_HANDLE) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		}

		// **Bind your vertex buffer here**
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	};
	VulkanBindCommandBuffers(&vkContext, commandBufferSetup);

	//uint32_t currentFrame = 0;
	//const uint32_t MAX_FRAMES_IN_FLIGHT = VK_REQUIRED_IMAGE_COUNT;
	//bool shouldRecreateSwapchain = false;

	while (1) {
		if (SDLHandleEvents() < 0) break;

		// imgui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		//some imgui UI to test
		ImGui::ShowDemoWindow();

		//make imgui calculate internal draw structures
		ImGui::Render();

		VulkanDraw(&vkContext);

		//vkWaitForFences(vkContext.device, 1, &vkContext.inFlightFence, VK_TRUE, UINT64_MAX);
		//vkResetFences(vkContext.device, 1, &vkContext.inFlightFence);

		//if (shouldRecreateSwapchain) {
		//	VulkanRecreateSwapchain(&vkContext);
		//	// Rerecord command buffers after new swapchain is created!
		//	VulkanRecordCommandBuffers(&vkContext, commandBufferSetup);
		//	shouldRecreateSwapchain = false;
		//}

		//VkSemaphore imageAvailableSemaphore = vkContext.imageAvailableSemaphores[currentFrame];
		//VkSemaphore renderFinishedSemaphore = vkContext.renderFinishedSemaphores[currentFrame];

		//// Acquire next image from swapchain
		//uint32_t imageIndex = VulkanAcquireNextImage(vkContext.device, vkContext.swapchain.instance, imageAvailableSemaphore);

		//VulkanRecordCommandBuffers(&vkContext, commandBufferSetup);
		//
		//VulkanSubmitCommandBuffer(
		//	vkContext.graphicsQueue,
		//	vkContext.commandBuffers,
		//	imageAvailableSemaphore,
		//	renderFinishedSemaphore,
		//	vkContext.inFlightFence,
		//	imageIndex);

		//VkResult presentResult = VulkanQueuePresent(
		//	vkContext.graphicsQueue,
		//	vkContext.swapchain.instance,
		//	renderFinishedSemaphore,
		//	imageIndex);

		//if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
		//	shouldRecreateSwapchain = true;
		//}

		//// Advance to next frame
		//currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		//SDL_Delay(16); // Simulate ~60 FPS
	}

	vkDeviceWaitIdle(vkContext.device);
	
	vkDestroyBuffer(vkContext.device, vertexBuffer, nullptr);
	vkDestroyBuffer(vkContext.device, indexBuffer, nullptr);
	vkFreeMemory(vkContext.device, vertexBufferMemory, nullptr);
	vkFreeMemory(vkContext.device, indexBufferMemory, nullptr);

	vkDestroyPipeline(vkContext.device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(vkContext.device, pipelineLayout, nullptr);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(vkContext.device, imguiPool, nullptr);

	SDL_DestroyWindow(window);
	VulkanShutdown(&vkContext);
	return 0;
}

//Uniform buffers(e.g., for MVP transforms)
//Descriptor sets for textures or more complex shaders
//Depth buffer if you move beyond 2D rendering
