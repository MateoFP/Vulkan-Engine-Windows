#pragma once
#include <vector>
#include "windows.h"
#include "vulkan/vulkan.h"
#include "mateo_math.h"
#include "miniaudio.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))
#define VK_CHECK(res) if (res != VK_SUCCESS) { __debugbreak(); }
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#define TEXTURE_NUM 7
#define MODEL_NUM	4
#define GRID_SIZE	101
#define TILE_SIZE	1
#define FIF			2
#define DEBUG

enum bit_flags : uint8_t
{
	FLAG_STONE		= 1 << 0,	// 0x01 (bit0)
	FLAG_FOG		= 1 << 0,	// 0x02 (bit1)
	FLAG_VISITED	= 1 << 1,	// 0x03 (bit2)
};

struct GUBO
{
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 projView;
	v4 dest;
	int is_debug;
	uint32_t frame_num;
	float pad[2];      
};
struct Buffer
{
	VkDeviceMemory	mem;
	VkBuffer		buffer;
};
struct Image
{
	VkImage			image;
	VkImageView		view;
	VkDeviceMemory	memory;
	VkFormat		format;
	int				width, height, depth, channels;
};
struct ReadEntireFile
{
	char* contents;
	size_t	contents_size;
};
struct Vertex
{
	float pos[3];
	float uv[2];
	uint32_t model_id;
	uint32_t tex_id;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static VkVertexInputAttributeDescription* getAttributeDescriptions()
	{
		static VkVertexInputAttributeDescription attributeDescriptions[4] = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, uv);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[2].offset = offsetof(Vertex, model_id);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[3].offset = offsetof(Vertex, tex_id);

		return attributeDescriptions;
	}
};
struct Vertex2D
{
	float pos[2];
	float uv[2];
	uint32_t id;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex2D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static VkVertexInputAttributeDescription* getAttributeDescriptions()
	{
		static VkVertexInputAttributeDescription attributeDescriptions[3] = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex2D, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex2D, uv);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[2].offset = offsetof(Vertex2D, id);

		return attributeDescriptions;
	}
};
struct VkContext
{
	VkInstance					instance;
	VkPhysicalDevice			physical_device;
	VkDevice					logical_device;
	VkExtent2D					extent;
	VkSurfaceCapabilitiesKHR	surface_cap;
	VkSurfaceFormatKHR			surface_format;
	VkPipelineLayout			layout_2d;
	VkPipeline					pipeline_2d;
	VkPipelineLayout			layout_3d;
	VkPipeline					pipeline_3d;
	VkCommandPool				cmd_pool;
	VkDescriptorPool			desc_pool;
	VkQueue						graphics_q;
	uint32_t					graphics_f;

	VkPhysicalDeviceMemoryProperties device_mem_prop;

	VkSampler linear_sampler;
	VkSampler nearest_sampler;

	VkDescriptorSet	buffer_set[FIF];
	VkDescriptorSet	texture_set;

	Image textures[TEXTURE_NUM];
	Image tiles;
	Image depth_image;

	VkSwapchainKHR	swapchain;
	VkImage*		swapchain_images;
	VkImageView*	swapchain_image_views;

	std::vector<Vertex>		global_vert;
	std::vector<uint32_t>	global_indices;
	std::vector<Vertex2D>	global_vert_2d;
	std::vector<uint32_t>	global_indices_2d;
	uint8_t					vertex_grid[GRID_SIZE][GRID_SIZE];
	uint8_t					fog_map[100][100];

	VkCommandBuffer global_cmd_buffers[FIF];

	Buffer		fog_map_buffer;
	void*		fog_map_mapped;
	Buffer		vertex_grid_buffer;
	void*		vertex_grid_mapped;
	Buffer		global_vert_buffer;
	Buffer		global_index_buffer;
	Buffer		global_vert_2d_buffer;
	void*		global_vert_2d_buffer_mapped;
	Buffer		global_index_buffer_2d;
	Buffer		model_uniform_buffer[FIF];
	Buffer		global_uniform_buffer[FIF];
	void*		model_uniform_buffer_mapped[FIF];
	void*		global_uniform_buffer_mapped[FIF];

	VkSemaphore	render_finished_semaphores[5];
	VkSemaphore	image_available_semaphores[FIF];
	VkFence		in_flight_fences[FIF];

	ma_engine audio_engine;
	ma_sound  forest;
};

extern VkContext vk;
extern GUBO gubo;
void InitVulkan(HWND win32_handle);
#ifdef DEBUG
void InitImgui(HWND win32_handle);
#endif
void InitMA();

ReadEntireFile ReadFile(const char* file_path);

void Create2DPipeline(VkDescriptorSetLayout* set_layouts);
void Create3DPipeline(VkDescriptorSetLayout* set_layouts);

void CreateImage(Image* image, VkImageType image_type, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags prop, VkImageAspectFlags flags, uint32_t array_layers);
void SetImageLayout(VkCommandBuffer cmd_buffer, VkImage images, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t array_layers);
void LoadModel(const char* path, uint32_t model_id, uint32_t tex_id);
void LoadTextures(Image* image, const char** texture_paths, uint32_t array_layers, VkFormat image_format);
void LoadPixels(Image* image, uint8_t* raw_pixels, int w, int h, uint32_t array_layers, VkFormat image_format, uint32_t bytes_per_pixel);

VkCommandBuffer BeginCMDBuffer();
void EndCMDBuffer(VkCommandBuffer cmd_buffer);
void CopyBufferToImage(VkCommandBuffer cmd_buffer, VkBuffer buffer, Image* image, uint32_t array_layers);
void CreateBuffer(VkDeviceSize	size, VkBufferUsageFlags usage, VkMemoryPropertyFlags prop, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void CreateVertexGrid();
void CreateFogMap();
void CreateIndexBuffer();
void CreateVertexBuffer();
void CreateVertex2DBuffer();
void CreateIndex2DBuffer();
void CreateGUBOBuffer();
void CreateMUBOBuffer();

void CreateDescriptorSet(VkDescriptorPool my_pool, VkDescriptorSetLayout* layout, VkDescriptorSet* set);
void WriteSets();

void CreateQuad(float x, float y, float width, float height, uint32_t id);
void Render(VkCommandBuffer cmd_buffer, uint32_t image_index);
void DrawFrame();

