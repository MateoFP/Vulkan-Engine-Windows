#define VK_USE_PLATFORM_WIN32_KHR
#include "win32_backend.h"
#include "vk_backend.h"

VkContext vk = {};

void InitVulkan(HWND win32_handle)
{
	//instance
	const char* instance_layers[] = { "VK_LAYER_KHRONOS_validation" };
	const char* instance_extensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
									VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Hello Triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instance_info{};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = ArrayCount(instance_extensions);
	instance_info.ppEnabledExtensionNames = instance_extensions;
	instance_info.enabledLayerCount = 1;
	instance_info.ppEnabledLayerNames = instance_layers;

	VK_CHECK(vkCreateInstance(&instance_info, 0, &vk.instance));

	//surface
	VkSurfaceKHR surface;
	VkWin32SurfaceCreateInfoKHR surface_info = {};
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.hinstance = GetModuleHandleW(NULL);
	surface_info.hwnd = win32_handle;

	VK_CHECK(vkCreateWin32SurfaceKHR(vk.instance, &surface_info, NULL, &surface));

	//device
	uint32_t						 device_count = 1;
	uint32_t						 queue_family_count = 0;
	VkPhysicalDeviceProperties		 physical_device_prop;
	VkPhysicalDeviceFeatures		 physical_device_feat;
	VkQueueFamilyProperties			 *q_fam_prop;

	vkEnumeratePhysicalDevices(vk.instance, &device_count, &vk.physical_device);
	vkGetPhysicalDeviceProperties(vk.physical_device, &physical_device_prop);
	vkGetPhysicalDeviceFeatures(vk.physical_device, &physical_device_feat);
	vkGetPhysicalDeviceMemoryProperties(vk.physical_device, &vk.device_mem_prop);

	vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_family_count, 0);
	q_fam_prop = (VkQueueFamilyProperties*)malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_family_count, q_fam_prop);

	VkBool32 present_support = false;
	for (uint32_t i = 0; i < queue_family_count; i++)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(vk.physical_device, i, surface, &present_support);
		if (present_support && (q_fam_prop[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			vk.graphics_f = i;
			break;
		}
	}

	//extensions
	const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feat = {};
	dynamic_rendering_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamic_rendering_feat.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceVulkan12Features vulkan12_feat = {};
	vulkan12_feat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12_feat.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	vulkan12_feat.runtimeDescriptorArray = VK_TRUE;
	vulkan12_feat.storageBuffer8BitAccess = VK_TRUE;
	vulkan12_feat.uniformAndStorageBuffer8BitAccess = VK_TRUE;

	vulkan12_feat.pNext = &dynamic_rendering_feat;

	VkDeviceQueueCreateInfo device_queue_info = {};
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.queueFamilyIndex = vk.graphics_f;
	device_queue_info.queueCount = 1;
	float queuePriority = 1.0f;
	device_queue_info.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &device_queue_info;
	device_info.queueCreateInfoCount = 1;
	device_info.pEnabledFeatures = &physical_device_feat;
	device_info.pNext = &vulkan12_feat;
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = nullptr;
	device_info.enabledExtensionCount = ArrayCount(device_extensions);
	device_info.ppEnabledExtensionNames = device_extensions;

	VK_CHECK(vkCreateDevice(vk.physical_device, &device_info, 0, &vk.logical_device));
	vkGetDeviceQueue(vk.logical_device, vk.graphics_f, 0, &vk.graphics_q);

	//swapchain
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, surface, &vk.surface_cap);

	uint32_t			format_count = 0;
	uint32_t			present_mode_count = 0;

	vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, surface, &format_count, 0);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, surface, &present_mode_count, 0);

	VkSurfaceFormatKHR* surface_formats = (VkSurfaceFormatKHR*)malloc(format_count * sizeof(VkSurfaceFormatKHR));
	VkPresentModeKHR* present_modes = (VkPresentModeKHR*)malloc(present_mode_count * sizeof(VkPresentModeKHR));

	vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, surface, &format_count, surface_formats);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physical_device, surface, &present_mode_count, present_modes);

	vk.surface_format = surface_formats[0];
	for (uint32_t i = 0; i < format_count; i++)
	{
		if ((surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
			surface_formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) &&
			surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			vk.surface_format = surface_formats[i];
			break;
		}
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;;
	for (uint32_t i = 0; i < present_mode_count; i++)
	{
		if (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR)
		{
			present_mode = VK_PRESENT_MODE_FIFO_KHR;
			break;
		}
	}

	//swapchain images/views
	vk.extent = vk.surface_cap.currentExtent;
	uint32_t image_count = vk.surface_cap.minImageCount + 1;

	if (vk.surface_cap.maxImageCount > 0 && image_count > vk.surface_cap.maxImageCount)
	{
		image_count = vk.surface_cap.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = image_count;
	swapchainCreateInfo.imageFormat = vk.surface_format.format;
	swapchainCreateInfo.imageColorSpace = vk.surface_format.colorSpace;
	swapchainCreateInfo.imageExtent = vk.extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = vk.surface_cap.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = present_mode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = nullptr;

	VK_CHECK(vkCreateSwapchainKHR(vk.logical_device, &swapchainCreateInfo, nullptr, &vk.swapchain));

	vkGetSwapchainImagesKHR(vk.logical_device, vk.swapchain, &image_count, nullptr);
	vk.swapchain_images = (VkImage*)malloc(image_count * sizeof(VkImage));
	vkGetSwapchainImagesKHR(vk.logical_device, vk.swapchain, &image_count, vk.swapchain_images);
	vk.swapchain_image_views = (VkImageView*)malloc(image_count * sizeof(VkImageView));

	for (uint32_t i = 0; i < image_count; i++)
	{
		VkImageViewCreateInfo ImageViewCreateInfo{};
		ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.image = vk.swapchain_images[i];
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = vk.surface_format.format;
		ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		VK_CHECK(vkCreateImageView(vk.logical_device, &ImageViewCreateInfo, 0, &vk.swapchain_image_views[i]));
	}

	//descriptor set layouts
	VkDescriptorSetLayout buffer_layout;
	VkDescriptorSetLayout texture_layout;

	//gubo
	VkDescriptorSetLayoutBinding buffer_bindings[4]{};
	buffer_bindings[0].binding = 0;
	buffer_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	buffer_bindings[0].descriptorCount = 1;
	buffer_bindings[0].pImmutableSamplers = nullptr;
	buffer_bindings[0].stageFlags = VK_SHADER_STAGE_ALL;

	//mubo
	buffer_bindings[1].binding = 1;
	buffer_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	buffer_bindings[1].descriptorCount = 1;
	buffer_bindings[1].pImmutableSamplers = nullptr;
	buffer_bindings[1].stageFlags = VK_SHADER_STAGE_ALL;

	//grid 
	buffer_bindings[2].binding = 2;
	buffer_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	buffer_bindings[2].descriptorCount = 1;
	buffer_bindings[2].pImmutableSamplers = nullptr;
	buffer_bindings[2].stageFlags = VK_SHADER_STAGE_ALL;

	//fog map 
	buffer_bindings[3].binding = 3;
	buffer_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	buffer_bindings[3].descriptorCount = 1;
	buffer_bindings[3].pImmutableSamplers = nullptr;
	buffer_bindings[3].stageFlags = VK_SHADER_STAGE_ALL;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
	descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_info.bindingCount = 4;
	descriptor_set_layout_info.pBindings = buffer_bindings;

	VK_CHECK(vkCreateDescriptorSetLayout(vk.logical_device, &descriptor_set_layout_info, nullptr, &buffer_layout));

	VkDescriptorSetLayoutBinding texture_bindings[2] = {};
	
	//single global textures
	texture_bindings[0].binding = 0;
	texture_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_bindings[0].descriptorCount = TEXTURE_NUM;
	texture_bindings[0].stageFlags = VK_SHADER_STAGE_ALL;

	//texture tiles
	texture_bindings[1].binding = 1;
	texture_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_bindings[1].descriptorCount = 1;
	texture_bindings[1].stageFlags = VK_SHADER_STAGE_ALL;

	VkDescriptorSetLayoutCreateInfo texture_layout_info = {};
	texture_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	texture_layout_info.bindingCount = 2;
	texture_layout_info.pBindings = texture_bindings;

	VK_CHECK(vkCreateDescriptorSetLayout(vk.logical_device, &texture_layout_info, nullptr, &texture_layout));

	//graphics pipelines
	VkDescriptorSetLayout layouts_3d[2] = { buffer_layout, texture_layout };
	Create3DPipeline(layouts_3d);
	Create2DPipeline(&texture_layout);

	//cmd pool
	VkCommandPoolCreateInfo cmd_pool_info{};
	cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cmd_pool_info.queueFamilyIndex = vk.graphics_f;

	VK_CHECK(vkCreateCommandPool(vk.logical_device, &cmd_pool_info, NULL, &vk.cmd_pool));

	//depth image
	vk.depth_image.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	vk.depth_image.width = vk.extent.width;
	vk.depth_image.height = vk.extent.height;
	CreateImage(&vk.depth_image, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	VkCommandBuffer cmd_buffer_depth_image = BeginCMDBuffer();
	SetImageLayout(cmd_buffer_depth_image, vk.depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	EndCMDBuffer(cmd_buffer_depth_image);

	//texture samplers
	VkSamplerCreateInfo linear_info{};
	linear_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	linear_info.magFilter = VK_FILTER_LINEAR;
	linear_info.minFilter = VK_FILTER_LINEAR;
	linear_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	linear_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	linear_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	linear_info.anisotropyEnable = VK_FALSE;
	linear_info.maxAnisotropy = physical_device_prop.limits.maxSamplerAnisotropy;
	linear_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	linear_info.unnormalizedCoordinates = VK_FALSE;
	linear_info.compareEnable = VK_FALSE;
	linear_info.compareOp = VK_COMPARE_OP_ALWAYS;
	linear_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	linear_info.mipLodBias = 0.0f;
	linear_info.minLod = 0.0f;
	linear_info.maxLod = 0.0f;

	VkSamplerCreateInfo nearest_info{};
	nearest_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	nearest_info.magFilter = VK_FILTER_NEAREST;
	nearest_info.minFilter = VK_FILTER_NEAREST;
	nearest_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	nearest_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	nearest_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	nearest_info.anisotropyEnable = VK_FALSE;
	nearest_info.maxAnisotropy = physical_device_prop.limits.maxSamplerAnisotropy;
	nearest_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	nearest_info.unnormalizedCoordinates = VK_FALSE;
	nearest_info.compareEnable = VK_FALSE;
	nearest_info.compareOp = VK_COMPARE_OP_ALWAYS;
	nearest_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	nearest_info.mipLodBias = 0.0f;
	nearest_info.minLod = 0.0f;
	nearest_info.maxLod = 0.0f;

	VK_CHECK(vkCreateSampler(vk.logical_device, &linear_info, nullptr, &vk.linear_sampler));
	VK_CHECK(vkCreateSampler(vk.logical_device, &nearest_info, nullptr, &vk.nearest_sampler));

	//textures/models
	const char* p1tex_path[] =		{ "resources/textures/Image_0.png" };
	const char* alphamask_path[] =	{ "resources/textures/am_stone_grey.tga" };
	const char* hp_path[] =			{ "resources/textures/hp_ui.png" };
	const char* mana_path[] =		{ "resources/textures/mana_ui.png" };
	const char* hp_bg_path[] =		{ "resources/textures/hp_bg.png" };
	const char* chest_path[] =		{ "resources/textures/chest.png" };
	const char* tiles_path[] =
	{	"resources/textures/stone.png",
		"resources/textures/stone2.png",
		"resources/textures/dirt.png",
		"resources/textures/dirt2.png",
		"resources/textures/grass.png" };

	LoadTextures(&vk.textures[0], alphamask_path,	1, VK_FORMAT_R8_UNORM);
	LoadTextures(&vk.textures[1], p1tex_path,		1, VK_FORMAT_R8G8B8A8_SRGB);
	LoadTextures(&vk.textures[2], p1tex_path,		1, VK_FORMAT_R8G8B8A8_SRGB);
	LoadTextures(&vk.textures[3], hp_path,			1, VK_FORMAT_R8G8B8A8_SRGB);
	LoadTextures(&vk.textures[4], mana_path,		1, VK_FORMAT_R8G8B8A8_SRGB);
	LoadTextures(&vk.textures[5], hp_bg_path,		1, VK_FORMAT_R8G8B8A8_SRGB);
	LoadTextures(&vk.textures[6], chest_path,		1, VK_FORMAT_R8G8B8A8_SRGB);
	LoadTextures(&vk.tiles,		  tiles_path,		5, VK_FORMAT_R8G8B8A8_SRGB);

	LoadModel("resources/models/plane.glb",		0,0);
	LoadModel("resources/models/monk_idle.glb", 1,1);
	LoadModel("resources/models/monk_idle.glb", 2,2);
	LoadModel("resources/models/chest.glb",		3,6);

	//2D quads
	CreateQuad(0.0, 0.0, 0.0, 0.0, 0);
	CreateQuad(0.0, 0.0, 0.0, 0.0, 0);
	CreateQuad(0.0, 0.0, 0.0, 0.0, 0);

	uint32_t quad_num = vk.global_vert_2d.size() / 4;
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

	//Buffers
	CreateFogMap();
	CreateVertexGrid();
	CreateIndexBuffer();
	CreateVertexBuffer();
	CreateIndex2DBuffer();
	CreateVertex2DBuffer();
	CreateGUBOBuffer();
	CreateMUBOBuffer();

	//descriptor pool and descriptor sets
	VkDescriptorPoolSize desc_pool_size[3]{};
	desc_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	desc_pool_size[0].descriptorCount = 2 * FIF;
	desc_pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_pool_size[1].descriptorCount = TEXTURE_NUM + 1;
	desc_pool_size[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc_pool_size[2].descriptorCount = 2 * FIF;

	VkDescriptorPoolCreateInfo desc_pool_info{};
	desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desc_pool_info.poolSizeCount = 3;
	desc_pool_info.pPoolSizes = desc_pool_size;
	desc_pool_info.maxSets = 3;

	VK_CHECK(vkCreateDescriptorPool(vk.logical_device, &desc_pool_info, nullptr, &vk.desc_pool));

	CreateDescriptorSet(vk.desc_pool, &buffer_layout, &vk.buffer_set[0]);
	CreateDescriptorSet(vk.desc_pool, &buffer_layout, &vk.buffer_set[1]);
	CreateDescriptorSet(vk.desc_pool, &texture_layout, &vk.texture_set);
	WriteSets();

	//global cmd buffer
	VkCommandBufferAllocateInfo allocCreateInfo{};
	allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocCreateInfo.commandPool = vk.cmd_pool;
	allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocCreateInfo.commandBufferCount = FIF;
	VK_CHECK(vkAllocateCommandBuffers(vk.logical_device, &allocCreateInfo, vk.global_cmd_buffers));

	//sync objects
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < FIF; i++)
	{
		VK_CHECK(vkCreateSemaphore(vk.logical_device, &semaphoreCreateInfo, nullptr, &vk.image_available_semaphores[i]));
		VK_CHECK(vkCreateFence(vk.logical_device, &fenceCreateInfo, nullptr, &vk.in_flight_fences[i]));
		VK_CHECK(vkCreateSemaphore(vk.logical_device, &semaphoreCreateInfo, nullptr, &vk.render_finished_semaphores[i]));
	}
	for (uint32_t i = 0; i < 5; i++)
	{
		VK_CHECK(vkCreateSemaphore(vk.logical_device, &semaphoreCreateInfo, nullptr, &vk.render_finished_semaphores[i]));
	}

	InitMA();
}
