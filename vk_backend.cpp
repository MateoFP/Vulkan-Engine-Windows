#define STB_IMAGE_IMPLEMENTATION 
#define CGLTF_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#include "windows.h"
#include "vulkan/vulkan.h"
#include "stb_image.h"
#include "cgltf.h"
#include "miniaudio.h"
#include "font.h"
#include "vk_backend.h"
#include "game.h"
#ifdef DEBUG
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_win32.h"

void InitImgui(HWND win32_handle)
{
	VkDescriptorPoolSize pool_sizes[] =
	{ { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
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
	pool_info.flags = 0;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	vkCreateDescriptorPool(vk.logical_device, &pool_info, nullptr, &imguiPool);

	ImGui_ImplWin32_Init(win32_handle);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vk.instance;
	init_info.PhysicalDevice = vk.physical_device;
	init_info.Device = vk.logical_device;
	init_info.Queue = vk.graphics_q;
	init_info.QueueFamily = vk.graphics_f;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &vk.surface_format.format;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.RenderPass = nullptr;
	ImGui_ImplVulkan_Init(&init_info);
}
#endif
void InitMA()
{
	ma_engine_init(NULL, &vk.audio_engine) == MA_SUCCESS;
	ma_sound_init_from_file(&vk.audio_engine, "resources/sounds/forest.wav", MA_SOUND_FLAG_STREAM, 0, 0, &vk.forest);
	ma_sound_start(&vk.forest);
	ma_engine_set_volume(&vk.audio_engine, 0.04f);
}

ReadEntireFile ReadFile(const char* file_path)
{
	ReadEntireFile ref = {};

	FILE* file = fopen(file_path, "rb");
	if (file == nullptr)
	{
		printf("Failed to load file %s, \n", file_path);
		return ref;
	}

	fseek(file, 0, SEEK_END);
	ref.contents_size = (size_t)ftell(file);
	fseek(file, 0, SEEK_SET);

	ref.contents = (char*)malloc(ref.contents_size);
	fread(ref.contents, 1, ref.contents_size, file);
	fclose(file);

	return ref;
}

void Create3DPipeline(VkDescriptorSetLayout* set_layouts)
{
	const char* vert_file = "resources/shaders/3d_vert_file.spv";
	const char* frag_file = "resources/shaders/3d_frag_file.spv";

	ReadEntireFile vert_shader_code = ReadFile(vert_file);
	ReadEntireFile frag_shader_code = ReadFile(frag_file);

	VkShaderModuleCreateInfo vertShaderCreateInfo{};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertShaderCreateInfo.codeSize = vert_shader_code.contents_size;
	vertShaderCreateInfo.pCode = (uint32_t*)(vert_shader_code.contents);

	VkShaderModuleCreateInfo fragShaderCreateInfo{};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragShaderCreateInfo.codeSize = frag_shader_code.contents_size;
	fragShaderCreateInfo.pCode = (uint32_t*)(frag_shader_code.contents);

	VkShaderModule vert_module;
	VkShaderModule frag_module;

	VK_CHECK(vkCreateShaderModule(vk.logical_device, &vertShaderCreateInfo, nullptr, &vert_module));
	VK_CHECK(vkCreateShaderModule(vk.logical_device, &fragShaderCreateInfo, nullptr, &frag_module));
	
	free(vert_shader_code.contents);
	free(frag_shader_code.contents);

	VkPipelineShaderStageCreateInfo vert_stage{};
	vert_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_stage.module = vert_module;
	vert_stage.pName = "main";

	VkPipelineShaderStageCreateInfo frag_stage{};
	frag_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage.module = frag_module;
	frag_stage.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { frag_stage, vert_stage };

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)vk.extent.width;
	viewport.height = (float)vk.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = vk.extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkVertexInputBindingDescription binding_description = Vertex::getBindingDescription();
	VkVertexInputAttributeDescription* attribute_descriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexBindingDescriptions = &binding_description;
	vertexInputInfo.pVertexAttributeDescriptions = attribute_descriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.lineWidth = 5.0f;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCreateInfo.depthBiasClamp = 0.0f;
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingCreateInfo.minSampleShading = 1.0f;
	multisamplingCreateInfo.pSampleMask = nullptr;
	multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable		 = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp		 = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp		 = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendingCreateInfo.blendConstants[0] = 0.0f;
	colorBlendingCreateInfo.blendConstants[1] = 0.0f;
	colorBlendingCreateInfo.blendConstants[2] = 0.0f;
	colorBlendingCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pipelineLayoutCreateInfo.pSetLayouts = set_layouts;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = 0;

	VK_CHECK(vkCreatePipelineLayout(vk.logical_device, &pipelineLayoutCreateInfo, nullptr, &vk.layout_3d));

	VkPipelineRenderingCreateInfoKHR pipeline_rendering_info{};
	pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
	pipeline_rendering_info.colorAttachmentCount = 1,
	pipeline_rendering_info.pColorAttachmentFormats = &vk.surface_format.format;
	pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &pipeline_rendering_info;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineInfo.pViewportState = &viewportStateCreateInfo;
	pipelineInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlendingCreateInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = vk.layout_3d;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	VK_CHECK(vkCreateGraphicsPipelines(vk.logical_device, nullptr, 1, &pipelineInfo, nullptr, &vk.pipeline_3d));

	vkDestroyShaderModule(vk.logical_device, frag_module, 0);
	vkDestroyShaderModule(vk.logical_device, vert_module, 0);
}
void Create2DPipeline(VkDescriptorSetLayout* set_layouts)
{
	const char* vert_file = "resources/shaders/2d_vert_file.spv";
	const char* frag_file = "resources/shaders/2d_frag_file.spv";

	ReadEntireFile vert_shader_code = ReadFile(vert_file);
	ReadEntireFile frag_shader_code = ReadFile(frag_file);

	VkShaderModuleCreateInfo vertShaderCreateInfo{};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertShaderCreateInfo.codeSize = vert_shader_code.contents_size;
	vertShaderCreateInfo.pCode = (uint32_t*)(vert_shader_code.contents);

	VkShaderModuleCreateInfo fragShaderCreateInfo{};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragShaderCreateInfo.codeSize = frag_shader_code.contents_size;
	fragShaderCreateInfo.pCode = (uint32_t*)(frag_shader_code.contents);

	VkShaderModule vert_module;
	VkShaderModule frag_module;

	VK_CHECK(vkCreateShaderModule(vk.logical_device, &vertShaderCreateInfo, nullptr, &vert_module));
	VK_CHECK(vkCreateShaderModule(vk.logical_device, &fragShaderCreateInfo, nullptr, &frag_module));

	VkPipelineShaderStageCreateInfo vert_stage{};
	vert_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_stage.module = vert_module;
	vert_stage.pName = "main";

	VkPipelineShaderStageCreateInfo frag_stage{};
	frag_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage.module = frag_module;
	frag_stage.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { frag_stage, vert_stage };

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)vk.extent.width;
	viewport.height = (float)vk.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = vk.extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkVertexInputBindingDescription binding_description_2d = Vertex2D::getBindingDescription();
	VkVertexInputAttributeDescription* attribute_descriptions_2d = Vertex2D::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = 3;
	vertexInputInfo.pVertexBindingDescriptions = &binding_description_2d;
	vertexInputInfo.pVertexAttributeDescriptions = attribute_descriptions_2d;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.lineWidth = 5.0f;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCreateInfo.depthBiasClamp = 0.0f;
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingCreateInfo.minSampleShading = 1.0f;
	multisamplingCreateInfo.pSampleMask = nullptr;
	multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendingCreateInfo.blendConstants[0] = 0.0f;
	colorBlendingCreateInfo.blendConstants[1] = 0.0f;
	colorBlendingCreateInfo.blendConstants[2] = 0.0f;
	colorBlendingCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = set_layouts;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = 0;

	VK_CHECK(vkCreatePipelineLayout(vk.logical_device, &pipelineLayoutCreateInfo, nullptr, &vk.layout_2d));

	VkPipelineRenderingCreateInfoKHR pipeline_rendering_info{};
	pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		pipeline_rendering_info.colorAttachmentCount = 1,
		pipeline_rendering_info.pColorAttachmentFormats = &vk.surface_format.format;
	pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &pipeline_rendering_info;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineInfo.pViewportState = &viewportStateCreateInfo;
	pipelineInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlendingCreateInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = vk.layout_2d;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	VK_CHECK(vkCreateGraphicsPipelines(vk.logical_device, nullptr, 1, &pipelineInfo, nullptr, &vk.pipeline_2d));

	vkDestroyShaderModule(vk.logical_device, frag_module, 0);
	vkDestroyShaderModule(vk.logical_device, vert_module, 0);
}

void CreateImage(Image* image, VkImageType image_type, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags prop, VkImageAspectFlags flags, uint32_t array_layers)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = image_type;
	imageInfo.extent.width = (uint32_t)(image->width);
	imageInfo.extent.height = (uint32_t)(image->height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = array_layers;
	imageInfo.format = image->format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	VK_CHECK(vkCreateImage(vk.logical_device, &imageInfo, nullptr, &image->image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(vk.logical_device, image->image, &mem_req);

	uint32_t mem_property = 0;
	for (uint32_t i = 0; i < vk.device_mem_prop.memoryTypeCount; i++)
	{
		if ((mem_req.memoryTypeBits & (1 << i)) &&
			(vk.device_mem_prop.memoryTypes[i].propertyFlags & prop) == prop)
		{
			mem_property = i;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = mem_req.size;
	allocInfo.memoryTypeIndex = mem_property;

	VK_CHECK(vkAllocateMemory(vk.logical_device, &allocInfo, nullptr, &image->memory));
	VK_CHECK(vkBindImageMemory(vk.logical_device, image->image, image->memory, 0));

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image->image;
	if (array_layers > 1)
	{
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	else
	{
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	}
	viewInfo.format = image->format;
	viewInfo.subresourceRange.aspectMask = flags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = array_layers;

	VK_CHECK(vkCreateImageView(vk.logical_device, &viewInfo, nullptr, &image->view));
}
void SetImageLayout(VkCommandBuffer cmd_buffer, VkImage images, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t array_layers)
{
	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.image = images;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = array_layers;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else
	{
		printf("gg");
	}

	vkCmdPipelineBarrier(cmd_buffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

ModelMeshes load_gltf(const char* path)
{
	cgltf_options options = {};
	cgltf_data* data = nullptr;

	cgltf_result result = cgltf_parse_file(&options, path, &data);
	if (result != cgltf_result_success)
	{
		OutputDebugStringA("failed at parsing model file.\n");
	}
	result = cgltf_load_buffers(&options, data, path);
	if (result != cgltf_result_success)
	{
		OutputDebugStringA("failed at loading model buffers.\n");
	}

	ModelMeshes model = {};
	model.submesh_index = static_cast<uint32_t>(vk.meshes_alloc.size());
	for (cgltf_size m = 0; m < data->meshes_count; ++m)
	{
		for (cgltf_size p = 0; p < data->meshes[m].primitives_count; ++p)
		{
			cgltf_primitive* prim = &data->meshes[m].primitives[p];

			cgltf_accessor* pos_acc = nullptr;
			cgltf_accessor* joint_acc = nullptr;
			cgltf_accessor* weight_acc = nullptr;
			cgltf_accessor* uv_acc = nullptr;

			for (size_t i = 0; i < prim->attributes_count; ++i)
			{
				if (prim->attributes[i].type == cgltf_attribute_type_position) pos_acc = prim->attributes[i].data;
				if (prim->attributes[i].type == cgltf_attribute_type_joints)   joint_acc = prim->attributes[i].data;
				if (prim->attributes[i].type == cgltf_attribute_type_weights)  weight_acc = prim->attributes[i].data;
				if (prim->attributes[i].type == cgltf_attribute_type_texcoord) uv_acc = prim->attributes[i].data;
			}

			if (!pos_acc) continue;

			Submesh alloc = {};
			alloc.vertex_offset = static_cast<int32_t>(vk.global_vert.size());

			size_t vertex_count = pos_acc->count;
			for (size_t v = 0; v < vertex_count; ++v)
			{
				Vertex vert = {};

				if (pos_acc)
				{
					float p[3];
					cgltf_accessor_read_float(pos_acc, v, p, 3);
					vert.pos[0] = p[0];
					vert.pos[1] = p[1];
					vert.pos[2] = p[2];
				}
				if (uv_acc)
				{
					float u[2];
					cgltf_accessor_read_float(uv_acc, v, u, 2);
					vert.uv[0] = u[0];
					vert.uv[1] = u[1];
				}

				vk.global_vert.push_back(vert);
			}

			if (prim->indices)
			{
				alloc.first_index = static_cast<uint32_t>(vk.global_indices.size());
				alloc.index_count = prim->indices->count;
				for (size_t i = 0; i < alloc.index_count; ++i)
				{
					uint32_t index = (uint32_t)cgltf_accessor_read_index(prim->indices, i);
					vk.global_indices.push_back(index);
				}
			}
			vk.meshes_alloc.push_back(alloc);
		}
	}
	model.submesh_count = static_cast<uint32_t>(vk.meshes_alloc.size()) - model.submesh_index;

	cgltf_free(data);
	return model;
}
void IndirectCMDs(ModelMeshes model, uint32_t instance_count, uint32_t first_instance)
{
	for (uint32_t i = 0; i < model.submesh_count; ++i)
	{
		uint32_t index = model.submesh_index + i;

		VkDrawIndexedIndirectCommand cmd{};
		cmd.indexCount		= vk.meshes_alloc[index].index_count;
		cmd.instanceCount	= instance_count;
		cmd.firstIndex		= vk.meshes_alloc[index].first_index;
		cmd.vertexOffset	= vk.meshes_alloc[index].vertex_offset;
		cmd.firstInstance	= first_instance;

		vk.indirect_cmds[vk.active_ind_cmds++] = cmd;
	}
}

void LoadTextures(Image* image, const char** texture_paths, uint32_t array_layers, VkFormat image_format)
{
	Assert(array_layers > 0 && texture_paths != nullptr);

	int stbi_format;
	int bytes_per_pixel;

	if (image_format == VK_FORMAT_R8G8B8A8_SRGB)
	{
		stbi_format = 4;
		bytes_per_pixel = 4;
	}
	else if (image_format == VK_FORMAT_R8_UNORM || image_format == VK_FORMAT_R8_UINT)
	{
		stbi_format = 1;
		bytes_per_pixel = 1;
	}
	else { OutputDebugStringA("\nwrong image format\n"); }

	int w, h, c;
	stbi_uc* first_layer = stbi_load(texture_paths[0], &w, &h, &c, stbi_format);
	Assert(first_layer);

	size_t single_layer_bytes = (size_t)w * h * bytes_per_pixel;
	size_t total_bytes = single_layer_bytes * array_layers;

	uint8_t* raw_pixels = (uint8_t*)malloc(total_bytes);
	memcpy(raw_pixels, first_layer, single_layer_bytes);
	stbi_image_free(first_layer);

	for (uint32_t i = 1; i < array_layers; i++)
	{
		int layer_w, layer_h, layer_c;
		stbi_uc* layer_pixels = stbi_load(texture_paths[i], &layer_w, &layer_h, &layer_c, stbi_format);
		Assert(layer_pixels && layer_w == w && layer_h == h);

		memcpy(raw_pixels + (i * single_layer_bytes), layer_pixels, single_layer_bytes);
		stbi_image_free(layer_pixels);
	}

	LoadPixels(image, raw_pixels, w, h, array_layers, image_format, bytes_per_pixel);

	free(raw_pixels);
}
void LoadPixels(Image* image, uint8_t* raw_pixels, int w, int h, uint32_t array_layers, VkFormat image_format, uint32_t bytes_per_pixel)
{
	image->width = w;
	image->height = h;
	image->format = image_format;

	VkDeviceSize image_size = (uint64_t)image->width * (uint64_t)image->height * bytes_per_pixel;
	VkDeviceSize total_image_size = image_size * array_layers;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(total_image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	VK_CHECK(vkMapMemory(vk.logical_device, stagingBufferMemory, 0, total_image_size, 0, &data));
	memcpy(data, raw_pixels, (size_t)total_image_size);
	vkUnmapMemory(vk.logical_device, stagingBufferMemory);

	CreateImage(image, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, array_layers);

	VkCommandBuffer cmd_buffer = BeginCMDBuffer();

	SetImageLayout(cmd_buffer, image->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, array_layers);
	CopyBufferToImage(cmd_buffer, stagingBuffer, image, array_layers);
	SetImageLayout(cmd_buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, array_layers);

	EndCMDBuffer(cmd_buffer);

	vkDestroyBuffer(vk.logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk.logical_device, stagingBufferMemory, nullptr);
}

VkCommandBuffer BeginCMDBuffer()
{
	VkCommandBufferAllocateInfo allocCreateInfo{};
	allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocCreateInfo.commandPool = vk.cmd_pool;
	allocCreateInfo.commandBufferCount = 1;

	VkCommandBuffer cmd_buffer;
	vkAllocateCommandBuffers(vk.logical_device, &allocCreateInfo, &cmd_buffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd_buffer, &beginInfo);

	return cmd_buffer;
}
void EndCMDBuffer(VkCommandBuffer cmd_buffer)
{
	vkEndCommandBuffer(cmd_buffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd_buffer;

	vkQueueSubmit(vk.graphics_q, 1, &submitInfo, nullptr);
	vkQueueWaitIdle(vk.graphics_q);
	vkFreeCommandBuffers(vk.logical_device, vk.cmd_pool, 1, &cmd_buffer);
}
void CopyBufferToImage(VkCommandBuffer cmd_buffer, VkBuffer buffer, Image* image, uint32_t array_layers)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = array_layers;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { (uint32_t)image->width, (uint32_t)image->height, 1 };

	vkCmdCopyBufferToImage(cmd_buffer, buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
void CreateBuffer(VkDeviceSize	size, VkBufferUsageFlags usage, VkMemoryPropertyFlags prop, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(vk.logical_device, &bufferCreateInfo, 0, &buffer));

	VkMemoryRequirements mem_req;
	vkGetBufferMemoryRequirements(vk.logical_device, buffer, &mem_req);

	uint32_t mem_property = 0;
	for (uint32_t i = 0; i < vk.device_mem_prop.memoryTypeCount; i++)
	{
		if ((mem_req.memoryTypeBits & (1 << i)) &&
			(vk.device_mem_prop.memoryTypes[i].propertyFlags & prop) == prop)
		{
			mem_property = i;
			break;
		}
	}

	VkMemoryAllocateInfo memoryAllocInfo{};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = mem_req.size;
	memoryAllocInfo.memoryTypeIndex = mem_property;

	VK_CHECK(vkAllocateMemory(vk.logical_device, &memoryAllocInfo, 0, &bufferMemory));
	VK_CHECK(vkBindBufferMemory(vk.logical_device, buffer, bufferMemory, 0));
}
void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer cmd_buffer = BeginCMDBuffer();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd_buffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndCMDBuffer(cmd_buffer);
}

void CreateVertexGrid()
{
	VkDeviceSize buffer_size = sizeof(vk.vertex_grid);

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk.vertex_grid_buffer.buffer, vk.vertex_grid_buffer.mem);

	VK_CHECK(vkMapMemory(vk.logical_device, vk.vertex_grid_buffer.mem, 0, buffer_size, 0, &vk.vertex_grid_mapped));
}
void CreateFogMap()
{
	VkDeviceSize buffer_size = sizeof(vk.fog_map);

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk.fog_map_buffer.buffer, vk.fog_map_buffer.mem);

	VK_CHECK(vkMapMemory(vk.logical_device, vk.fog_map_buffer.mem, 0, buffer_size, 0, &vk.fog_map_mapped));
}
void CreateIndexBuffer()
{
	VkDeviceSize buffer_size = sizeof(vk.global_indices[0]) * vk.global_indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	VK_CHECK(vkMapMemory(vk.logical_device, stagingBufferMemory, 0, buffer_size, 0, &data));
	memcpy(data, vk.global_indices.data(), buffer_size);
	vkUnmapMemory(vk.logical_device, stagingBufferMemory);

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk.global_index_buffer.buffer, vk.global_index_buffer.mem);

	CopyBuffer(stagingBuffer, vk.global_index_buffer.buffer, buffer_size);

	vkDestroyBuffer(vk.logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk.logical_device, stagingBufferMemory, nullptr);
}
void CreateVertexBuffer()
{
	VkDeviceSize buffer_size = sizeof(vk.global_vert[0]) * vk.global_vert.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	VK_CHECK(vkMapMemory(vk.logical_device, stagingBufferMemory, 0, buffer_size, 0, &data));
	memcpy(data, vk.global_vert.data(), buffer_size);
	vkUnmapMemory(vk.logical_device, stagingBufferMemory);

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk.global_vert_buffer.buffer, vk.global_vert_buffer.mem);

	CopyBuffer(stagingBuffer, vk.global_vert_buffer.buffer, buffer_size);

	vkDestroyBuffer(vk.logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk.logical_device, stagingBufferMemory, nullptr);
}
void CreateVertex2DBuffer()
{
	VkDeviceSize buffer_size = MAX_QUADS;

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk.global_vert_2d_buffer.buffer, vk.global_vert_2d_buffer.mem);

	VK_CHECK(vkMapMemory(vk.logical_device, vk.global_vert_2d_buffer.mem, 0, buffer_size, 0, &vk.global_vert_2d_buffer_mapped));
}
void CreateIndex2DBuffer()
{
	VkDeviceSize buffer_size = sizeof(vk.global_indices_2d[0]) * vk.global_indices_2d.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	VK_CHECK(vkMapMemory(vk.logical_device, stagingBufferMemory, 0, buffer_size, 0, &data));
	memcpy(data, vk.global_indices_2d.data(), buffer_size);
	vkUnmapMemory(vk.logical_device, stagingBufferMemory);

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk.global_index_buffer_2d.buffer, vk.global_index_buffer_2d.mem);

	CopyBuffer(stagingBuffer, vk.global_index_buffer_2d.buffer, buffer_size);

	vkDestroyBuffer(vk.logical_device, stagingBuffer, nullptr);
	vkFreeMemory(vk.logical_device, stagingBufferMemory, nullptr);
}
void CreateGUBOBuffer()
{
	VkDeviceSize buffer_size = sizeof(GUBO);

	for (uint32_t i = 0; i < FIF; i++)
	{
		CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vk.global_uniform_buffer[i].buffer, vk.global_uniform_buffer[i].mem);

		VK_CHECK(vkMapMemory(vk.logical_device, vk.global_uniform_buffer[i].mem, 0, buffer_size, 0, &vk.global_uniform_buffer_mapped[i]));
	}
}
void CreateIndirectBuffer()
{
	VkDeviceSize buffer_size = sizeof(VkDrawIndexedIndirectCommand) * MAX_INDIRECT_CMDS;

	CreateBuffer(buffer_size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk.indirect_buffer.buffer, vk.indirect_buffer.mem);

	VK_CHECK(vkMapMemory(vk.logical_device, vk.indirect_buffer.mem, 0, buffer_size, 0, &vk.indirect_buffer_mapped));
}
void CreateInstanceBuffer()
{
	VkDeviceSize buffer_size = sizeof(InstanceData) * MAX_INSTANCES;

	for (uint32_t i = 0; i < FIF; i++)
	{
		CreateBuffer(buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vk.instance_buffer[i].buffer, vk.instance_buffer[i].mem);

		VK_CHECK(vkMapMemory(vk.logical_device, vk.instance_buffer[i].mem, 0, buffer_size, 0, &vk.instance_buffer_mapped[i]));
	}
}

void CreateDescriptorSet(VkDescriptorPool my_pool, VkDescriptorSetLayout* layout, VkDescriptorSet* set)
{
	VkDescriptorSetAllocateInfo setAllocInfo{};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = my_pool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = layout;

	VK_CHECK(vkAllocateDescriptorSets(vk.logical_device, &setAllocInfo, set));
}
void WriteSets()
{
	//gubo
	for (uint32_t i = 0; i < FIF; i++)
	{
		VkDescriptorBufferInfo gubo_info{};
		gubo_info.buffer = vk.global_uniform_buffer[i].buffer;
		gubo_info.offset = 0;
		gubo_info.range = sizeof(GUBO);

		VkWriteDescriptorSet write_gubo = {};
		write_gubo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_gubo.dstSet = vk.buffer_set[i];
		write_gubo.dstBinding = 0;
		write_gubo.dstArrayElement = 0;
		write_gubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_gubo.descriptorCount = 1;
		write_gubo.pBufferInfo = &gubo_info;
		vkUpdateDescriptorSets(vk.logical_device, 1, &write_gubo, 0, nullptr);
	}

	//instance buffer
	for (uint32_t i = 0; i < FIF; i++)
	{
		VkDescriptorBufferInfo instance_buffer_info{};
		instance_buffer_info.buffer = vk.instance_buffer[i].buffer;
		instance_buffer_info.offset = 0;
		instance_buffer_info.range = sizeof(InstanceData) * 3;

		VkWriteDescriptorSet write_instance_buffer = {};
		write_instance_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_instance_buffer.dstSet = vk.buffer_set[i];
		write_instance_buffer.dstBinding = 1;
		write_instance_buffer.dstArrayElement = 0;
		write_instance_buffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write_instance_buffer.descriptorCount = 1;
		write_instance_buffer.pBufferInfo = &instance_buffer_info;
		vkUpdateDescriptorSets(vk.logical_device, 1, &write_instance_buffer, 0, nullptr);
	}

	//vertex grid
	for (uint32_t i = 0; i < FIF; i++)
	{
		VkDescriptorBufferInfo grid_info{};
		grid_info.buffer = vk.vertex_grid_buffer.buffer;
		grid_info.offset = 0;
		grid_info.range = sizeof(vk.vertex_grid);

		VkWriteDescriptorSet grid_write = {};
		grid_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		grid_write.dstSet = vk.buffer_set[i];
		grid_write.dstBinding = 2;
		grid_write.dstArrayElement = 0;
		grid_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		grid_write.descriptorCount = 1;
		grid_write.pBufferInfo = &grid_info;
		vkUpdateDescriptorSets(vk.logical_device, 1, &grid_write, 0, nullptr);
	}

	//fog grid
	for (uint32_t i = 0; i < FIF; i++)
	{
		VkDescriptorBufferInfo grid_info{};
		grid_info.buffer = vk.fog_map_buffer.buffer;
		grid_info.offset = 0;
		grid_info.range = sizeof(vk.fog_map);

		VkWriteDescriptorSet grid_write = {};
		grid_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		grid_write.dstSet = vk.buffer_set[i];
		grid_write.dstBinding = 3;
		grid_write.dstArrayElement = 0;
		grid_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		grid_write.descriptorCount = 1;
		grid_write.pBufferInfo = &grid_info;
		vkUpdateDescriptorSets(vk.logical_device, 1, &grid_write, 0, nullptr);
	}
	//single global textures
	VkDescriptorImageInfo image_infos[TEXTURE_NUM];
	for (uint32_t i = 0; i < TEXTURE_NUM; i++)
	{
		image_infos[i].imageView = vk.textures[i].view;
		image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_infos[i].sampler = vk.linear_sampler;
	}

	VkDescriptorImageInfo tiles_image_info = {};
	tiles_image_info.imageView = vk.tiles.view;
	tiles_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	tiles_image_info.sampler = vk.linear_sampler;

	VkWriteDescriptorSet write_tex[2] = {};
	write_tex[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_tex[0].dstSet = vk.texture_set;
	write_tex[0].dstBinding = 0;
	write_tex[0].dstArrayElement = 0;
	write_tex[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_tex[0].descriptorCount = TEXTURE_NUM;
	write_tex[0].pImageInfo = image_infos;
	//tiles array
	write_tex[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_tex[1].dstSet = vk.texture_set;
	write_tex[1].dstBinding = 1;
	write_tex[1].dstArrayElement = 0;
	write_tex[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_tex[1].descriptorCount = 1;
	write_tex[1].pImageInfo = &tiles_image_info;

	vkUpdateDescriptorSets(vk.logical_device, 2, write_tex, 0, nullptr);
}

void CreateQuad(float x, float y, float width, float height, uint32_t id)
{
	vk.global_vert_2d.push_back({ { x,         y          }, { 0.0f, 1.0f }, id});
	vk.global_vert_2d.push_back({ { x + width, y          }, { 1.0f, 1.0f }, id});
	vk.global_vert_2d.push_back({ { x + width, y + height }, { 1.0f, 0.0f }, id});
	vk.global_vert_2d.push_back({ { x,         y + height }, { 0.0f, 0.0f }, id});
}
void Render(VkCommandBuffer cmd_buffer, uint32_t image_index)
{
	VkCommandBufferBeginInfo beingInfo{};
	beingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &beingInfo));

	SetImageLayout(cmd_buffer, vk.swapchain_images[image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

	VkClearValue colorClear;
	colorClear.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	VkClearValue depthClear;
	depthClear.depthStencil = { 1.0f, 0 };

	VkRenderingAttachmentInfo color_attachment_info = {};
	color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	color_attachment_info.imageView = vk.swapchain_image_views[image_index];
	color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
	color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_info.clearValue = colorClear;

	VkRenderingAttachmentInfo depth_attachment_info = {};
	depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depth_attachment_info.imageView = vk.depth_image.view;
	depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
	depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment_info.clearValue = depthClear;

	VkRenderingInfo render_info = {};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	render_info.renderArea.offset = { 0 };
	render_info.renderArea.extent = vk.extent;
	render_info.layerCount = 1;
	render_info.colorAttachmentCount = 1;
	render_info.pColorAttachments = &color_attachment_info;
	render_info.pDepthAttachment = &depth_attachment_info;

	VkDeviceSize offsets[] = { 0 };

	vkCmdBeginRendering(cmd_buffer, &render_info);

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_3d);
		vkCmdBindIndexBuffer(cmd_buffer, vk.global_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vk.global_vert_buffer.buffer, offsets);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.layout_3d, 0, 1, &vk.buffer_set[current_frame], 
			0, nullptr);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.layout_3d, 1, 1, &vk.texture_set, 
			0, nullptr);

		vkCmdDrawIndexedIndirect(cmd_buffer, vk.indirect_buffer.buffer, 0, vk.active_ind_cmds, sizeof(VkDrawIndexedIndirectCommand));

		vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_2d);
		vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vk.global_vert_2d_buffer.buffer, offsets);
		vkCmdBindIndexBuffer(cmd_buffer, vk.global_index_buffer_2d.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.layout_2d, 0, 1, &vk.texture_set, 
			0, nullptr);

		vkCmdDrawIndexed(cmd_buffer, vk.global_indices_2d.size(), 1, 0, 0, 0);

		#ifdef DEBUG
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd_buffer);
		#endif

	vkCmdEndRendering(cmd_buffer);

	SetImageLayout(cmd_buffer, vk.swapchain_images[image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
	VK_CHECK(vkEndCommandBuffer(cmd_buffer));
}
void DrawFrame()
{
	vkWaitForFences(vk.logical_device, 1, &vk.in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

	uint32_t image_index = 0;

	vkAcquireNextImageKHR(vk.logical_device, vk.swapchain, UINT64_MAX, vk.image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

	vkResetFences(vk.logical_device, 1, &vk.in_flight_fences[current_frame]);
	vkResetCommandBuffer(vk.global_cmd_buffers[current_frame], 0);

	Render(vk.global_cmd_buffers[current_frame], image_index);

	VkSemaphore waitSemaphores[] = { vk.image_available_semaphores[current_frame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { vk.render_finished_semaphores[image_index]};

	VkSubmitInfo submitCreateInfo{};
	submitCreateInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitCreateInfo.waitSemaphoreCount = 1;
	submitCreateInfo.pWaitSemaphores = waitSemaphores;
	submitCreateInfo.pWaitDstStageMask = waitStages;
	submitCreateInfo.commandBufferCount = 1;
	submitCreateInfo.pCommandBuffers = &vk.global_cmd_buffers[current_frame];
	submitCreateInfo.signalSemaphoreCount = 1;
	submitCreateInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK(vkQueueSubmit(vk.graphics_q, 1, &submitCreateInfo, vk.in_flight_fences[current_frame]));

	VkSwapchainKHR swapchains[] = { vk.swapchain };

	VkPresentInfoKHR presentCreateInfo{};
	presentCreateInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentCreateInfo.waitSemaphoreCount = 1;
	presentCreateInfo.pWaitSemaphores = signalSemaphores;
	presentCreateInfo.swapchainCount = 1;
	presentCreateInfo.pSwapchains = swapchains;
	presentCreateInfo.pImageIndices = &image_index;
	presentCreateInfo.pResults = nullptr; // Optional

	VK_CHECK(vkQueuePresentKHR(vk.graphics_q, &presentCreateInfo));

	current_frame = (current_frame + 1) % FIF;
}

void InitModels(const char** model_paths, uint32_t array_size)
{
	uint32_t first_instance = 0;

	for (uint32_t i = 0; i < array_size; i++)
	{
		ModelMeshes model = load_gltf(model_paths[i]);
		IndirectCMDs(model, 1, first_instance);
		first_instance++;
		vk.total_submesh_count += model.submesh_count;
		vk.model_meshes.push_back(model);
	}
}
void InitQuads()
{
	uint32_t quad_num = MAX_QUADS / 4;
	vk.global_indices_2d.resize(quad_num * 6);
	uint32_t vertex_offset = 0;
	for (size_t i = 0; i < vk.global_indices_2d.size(); i += 6)
	{
		vk.global_indices_2d[i + 0] = vertex_offset + 0;
		vk.global_indices_2d[i + 1] = vertex_offset + 1;
		vk.global_indices_2d[i + 2] = vertex_offset + 2;
		vk.global_indices_2d[i + 3] = vertex_offset + 2;
		vk.global_indices_2d[i + 4] = vertex_offset + 3;
		vk.global_indices_2d[i + 5] = vertex_offset + 0;

		vertex_offset += 4;
	}
}

