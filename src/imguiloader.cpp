#include "imguiloader.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

VkDescriptorPool ImGuiSDLVulkanStartup(SDL_Window* window, VulkanContext* vkContext) {
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

void ImGuiSDLVulkanDraw(std::function<void(void)> imguiWindowsLambda) {
	// imgui new frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	//some imgui UI to test
	imguiWindowsLambda();
	ImGui::ShowDemoWindow();

	//make imgui calculate internal draw structures
	ImGui::Render();
}

void ImGuiSDLVulkanShutdown(VulkanContext* vkContext, VkDescriptorPool imguiPool) {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(vkContext->device, imguiPool, nullptr);
}
