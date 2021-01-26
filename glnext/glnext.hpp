#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <vulkan/vulkan_core.h>

#ifdef BUILD_WINDOWS
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#define SURFACE_EXTENSION "VK_KHR_win32_surface"
#endif

#ifdef BUILD_DARWIN
#include <QuartzCore/CAMetalLayer.h>
#include <vulkan/vulkan_metal.h>
#define SURFACE_EXTENSION "VK_EXT_metal_surface"
#endif

#ifdef BUILD_LINUX
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#define SURFACE_EXTENSION "VK_KHR_xlib_surface"
#endif

enum ImageMode {
    IMG_PROTECTED,
    IMG_TEXTURE,
    IMG_OUTPUT,
    IMG_STORAGE,
};

enum BufferMode {
    BUF_STORAGE,
    BUF_UNIFORM,
};

typedef void (* Packer)(char * ptr, PyObject ** obj);

struct Format {
    VkFormat format;
    uint32_t size;
    Packer packer;
    uint32_t items;
};

struct HostBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void * ptr;
};

struct ComputeCount {
    uint32_t x, y, z;
};

struct Presenter {
    uint32_t surface_count;
    VkSurfaceKHR * surface_array;
    VkSwapchainKHR * swapchain_array;
    VkPipelineStageFlags * wait_stage_array;
    VkSemaphore * semaphore_array;
    VkImage * image_source_array;
    VkImageCopy * image_copy_array;
    uint32_t * image_count_array;
    VkImage ** image_array;
    VkResult * result_array;
    uint32_t * index_array;
};

struct ModuleState {
    PyTypeObject * Instance_type;
    PyTypeObject * Framebuffer_type;
    PyTypeObject * RenderPipeline_type;
    PyTypeObject * ComputePipeline_type;
    PyTypeObject * Memory_type;
    PyTypeObject * Buffer_type;
    PyTypeObject * Image_type;

    PyObject * empty_str;
    PyObject * empty_list;

    PyObject * default_topology;
    PyObject * default_front_face;
    PyObject * default_format;
    PyObject * one_float_str;
    PyObject * one_int_str;
    PyObject * texture_str;
    PyObject * output_str;
};

struct Instance {
    PyObject_HEAD

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    VkFence fence;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    VkDebugUtilsMessengerEXT debug_messenger;

    uint32_t queue_family_index;
    uint32_t host_memory_type_index;
    uint32_t device_memory_type_index;
    VkFormat depth_format;

    Presenter presenter;

    PyObject * task_list;
    PyObject * memory_list;
    PyObject * buffer_list;
    PyObject * image_list;
    PyObject * log_list;

    ModuleState * state;
};

struct Image;
struct Buffer;

struct BufferBinding {
    Buffer * buffer;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkDescriptorType descriptor_type;
};

struct ImageBinding {
    VkBool32 sampled;
    uint32_t image_count;
    Image ** image_array;
    VkDescriptorImageInfo * descriptor_image_info_array;
    VkImageViewCreateInfo * image_view_create_info_array;
    VkSamplerCreateInfo * sampler_create_info_array;
    VkImageView * image_view_array;
    VkSampler * sampler_array;
    VkDescriptorType descriptor_type;
    VkImageLayout layout;
};

struct Framebuffer {
    PyObject_HEAD
    Instance * instance;
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    VkBool32 depth;
    VkBool32 compute;
    ImageMode mode;
    Image ** image_array;
    VkImageView * image_view_array;
    VkFramebuffer * framebuffer_array;
    VkClearValue * clear_value_array;
    VkAttachmentDescription * description_array;
    VkAttachmentReference * reference_array;
    VkImageMemoryBarrier * image_barrier_array;
    uint32_t image_barrier_count;
    uint32_t attachment_count;
    uint32_t output_count;
    VkRenderPass render_pass;
    PyObject * render_pipeline_list;
    PyObject * compute_pipeline_list;
    PyObject * output;
};

struct RenderPipeline {
    PyObject_HEAD
    Instance * instance;
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t index_count;
    uint32_t indirect_count;
    Buffer * vertex_buffer;
    Buffer * instance_buffer;
    Buffer * index_buffer;
    Buffer * indirect_buffer;
    uint32_t buffer_count;
    uint32_t image_count;
    BufferBinding * buffer_array;
    ImageBinding * image_array;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    uint32_t vertex_attribute_count;
    VkBuffer * vertex_attribute_buffer_array;
    VkDeviceSize * vertex_attribute_offset_array;
    VkPipeline pipeline;
    PyObject * buffer;
};

struct ComputePipeline {
    PyObject_HEAD
    Instance * instance;
    ComputeCount compute_count;
    uint32_t buffer_count;
    uint32_t image_count;
    BufferBinding * buffer_array;
    ImageBinding * image_array;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule compute_shader_module;
    VkPipeline pipeline;
    Buffer * uniform_buffer;
    PyObject * buffer;
};

struct Memory {
    PyObject_HEAD
    Instance * instance;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkDeviceMemory memory;
    VkBool32 host;
    void * ptr;
};

struct Buffer {
    PyObject_HEAD
    Instance * instance;
    Memory * memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkBuffer buffer;
    // StagingBuffer * staging_buffer;
    // VkDeviceSize staging_offset;
};

struct Image {
    PyObject_HEAD
    Instance * instance;
    Memory * memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkImageAspectFlags aspect;
    VkExtent3D extent;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    ImageMode mode;
    VkFormat format;
    VkImage image;
    // StagingBuffer * staging_buffer;
    // VkDeviceSize staging_offset;
};

struct BufferCreateInfo {
    Instance * instance;
    Memory * memory;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
};

struct ImageCreateInfo {
    Instance * instance;
    Memory * memory;
    VkDeviceSize size;
    VkImageUsageFlags usage;
    VkImageAspectFlags aspect;
    VkExtent3D extent;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    ImageMode mode;
    VkFormat format;
};

void install_debug_messenger(Instance * instance);

BufferBinding parse_buffer_binding(PyObject * obj);
ImageBinding parse_image_binding(PyObject * obj);

void execute_framebuffer(VkCommandBuffer command_buffer, Framebuffer * framebuffer);
void execute_render_pipeline(VkCommandBuffer command_buffer, RenderPipeline * pipeline);
void execute_compute_pipeline(VkCommandBuffer cmd, ComputePipeline * pipeline);

VkCommandBuffer begin_commands(Instance * instance);
void end_commands(Instance * instance);

Memory * new_memory(Instance * instance, VkBool32 host = false);
Memory * get_memory(Instance * instance, PyObject * memory);

VkDeviceSize take_memory(Memory * self, VkMemoryRequirements * requirements);

void allocate_memory(Memory * self);
void free_memory(Memory * self);

Image * new_image(ImageCreateInfo info);
Buffer * new_buffer(BufferCreateInfo info);

void bind_image(Image * image);
void bind_buffer(Buffer * buffer);

void new_temp_buffer(Instance * instance, HostBuffer * temp, VkDeviceSize size);
void free_temp_buffer(Instance * instance, HostBuffer * temp);

VkPrimitiveTopology get_topology(PyObject * name);
ImageMode get_image_mode(PyObject * name);
Format get_format(PyObject * name);
