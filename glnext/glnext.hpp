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

typedef void (* Packer)(char * ptr, PyObject ** obj);

struct Format {
    VkFormat format;
    uint32_t size;
    Packer packer;
    uint32_t items;
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

struct HostBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void * ptr;
};

struct ModuleState {
    PyTypeObject * Instance_type;
    PyTypeObject * ComputeSet_type;
    PyTypeObject * RenderSet_type;
    PyTypeObject * Pipeline_type;
    PyTypeObject * Transform_type;
    PyTypeObject * Encoder_type;
    PyTypeObject * Image_type;
    PyTypeObject * Buffer_type;
    PyTypeObject * Memory_type;
    PyTypeObject * StagingBuffer_type;

    PyObject * empty_str;
    PyObject * empty_list;
    PyObject * default_topology;
    PyObject * default_front_face;
    PyObject * default_border_color;
    PyObject * default_format;
    PyObject * default_filter;
    PyObject * default_address_mode;
    PyObject * default_swizzle;
    PyObject * one_float_str;
    PyObject * one_int_str;
    PyObject * texture_str;
    PyObject * output_str;
};

struct StagingBuffer;
struct Buffer;
struct Image;
struct Memory;

struct Instance {
    PyObject_HEAD

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    VkFence fence;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    uint32_t queue_family_index;
    uint32_t host_memory_type_index;
    uint32_t device_memory_type_index;
    VkFormat depth_format;

    Presenter presenter;

    PyObject * transform_list;
    PyObject * encoder_list;
    PyObject * task_list;
    PyObject * memory_list;
    PyObject * buffer_list;
    PyObject * user_image_list;
    PyObject * image_list;

    ModuleState * state;
};

struct ComputeSet {
    PyObject_HEAD
    Instance * instance;
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule compute_shader_module;
    VkPipeline pipeline;
    PyObject * result_images;
    Buffer * uniform_buffer;
    Buffer * storage_buffer;
    PyObject * output;
};

struct RenderSet {
    PyObject_HEAD
    Instance * instance;
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    VkBool32 depth;
    Image ** image_array;
    VkImageView * image_view_array;
    VkFramebuffer * framebuffer_array;
    VkClearValue * clear_value_array;
    VkAttachmentDescription * description_array;
    VkAttachmentReference * reference_array;
    uint32_t attachment_count;
    uint32_t output_count;
    VkRenderPass render_pass;
    Buffer * uniform_buffer;
    Buffer * storage_buffer;
    PyObject * pipeline_list;
    PyObject * output;
};

struct Pipeline {
    PyObject_HEAD
    Instance * instance;
    Buffer * uniform_buffer;
    Buffer * storage_buffer;
    Buffer * vertex_buffer;
    Buffer * instance_buffer;
    Buffer * index_buffer;
    Buffer * indirect_buffer;
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t index_count;
    uint32_t indirect_count;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    PyObject * vertex_buffers;
    VkPipeline pipeline;
};

struct Transform {
    PyObject_HEAD
    Instance * instance;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule compute_shader_module;
    VkPipeline pipeline;
    uint32_t compute_groups_x;
    uint32_t compute_groups_y;
    uint32_t compute_groups_z;
    Buffer * uniform_buffer;
    Buffer * storage_buffer;
    Buffer * output_buffer;
};

struct Encoder {
    PyObject_HEAD
    Instance * instance;
    Image * image;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule compute_shader_module;
    VkPipeline pipeline;
    uint32_t compute_groups_x;
    uint32_t compute_groups_y;
    uint32_t compute_groups_z;
    Buffer * uniform_buffer;
    Buffer * storage_buffer;
    Buffer * output_buffer;
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
    StagingBuffer * staging_buffer;
    VkDeviceSize staging_offset;
};

struct Buffer {
    PyObject_HEAD
    Instance * instance;
    Memory * memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkBuffer buffer;
    StagingBuffer * staging_buffer;
    VkDeviceSize staging_offset;
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

struct StagingBuffer {
    PyObject_HEAD
    Instance * instance;
    VkDeviceSize size;
    VkBuffer buffer;
    VkDeviceMemory memory;
    void * ptr;
    PyObject * mem;
};

template <typename T>
void realloc_array(T ** array, uint32_t size) {
    *array = (T *)PyMem_Realloc(*array, sizeof(T) * size);
}

template <typename T>
PyObject * preserve_array(uint32_t size, T * array) {
    return PyBytes_FromStringAndSize((char *)array, sizeof(T) * size);
}

template <typename T>
uint32_t retreive_array(PyObject * obj, T ** array) {
    *array = (T *)PyBytes_AsString(obj);
    return (uint32_t)PyBytes_Size(obj) / sizeof(T);
}

template <typename T>
PyObject * alloc_array(uint32_t size, T ** array) {
    PyObject * res = PyBytes_FromStringAndSize(NULL, sizeof(T) * size);
    retreive_array(res, array);
    return res;
}

VkCommandBuffer begin_commands(Instance * instance);
void end_commands(Instance * instance);
void end_commands_with_present(Instance * instance);

Memory * new_memory(Instance * instance, VkBool32 host);
Memory * get_memory(Instance * instance, PyObject * memory);
VkDeviceSize take_memory(Memory * self, VkMemoryRequirements * requirements);
void allocate_memory(Memory * self);
void free_memory(Memory * self);

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

struct BufferCreateInfo {
    Instance * instance;
    Memory * memory;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
};

Image * new_image(ImageCreateInfo * create_info);
Buffer * new_buffer(BufferCreateInfo * create_info);

void bind_image(Image * image);
void bind_buffer(Buffer * buffer);

Format get_format(PyObject * name);
VkCullModeFlags get_cull_mode(PyObject * name);
VkFrontFace get_front_face(PyObject * name);
VkPrimitiveTopology get_topology(PyObject * name);
VkFilter get_filter(PyObject * name);
VkSamplerMipmapMode get_mipmap_filter(PyObject * name);
VkSamplerAddressMode get_address_mode(PyObject * name);
ImageMode get_image_mode(PyObject * name);
VkBorderColor get_border_color(PyObject * name, bool is_float);

void presenter_resize(Presenter * presenter);
void presenter_remove(Presenter * presenter, uint32_t index);

void new_temp_buffer(Instance * instance, HostBuffer * temp, VkDeviceSize size);
void free_temp_buffer(Instance * instance, HostBuffer * temp);

void staging_input_buffer(VkCommandBuffer command_buffer, Buffer * buffer);
void staging_output_buffer(VkCommandBuffer command_buffer, Buffer * buffer);
void staging_input_image(VkCommandBuffer command_buffer, Image * image);
void staging_output_image(VkCommandBuffer command_buffer, Image * image);

void execute_transform(VkCommandBuffer command_buffer, Transform * transform);
void execute_encoder(VkCommandBuffer command_buffer, Encoder * encoder);
void execute_compute_set(VkCommandBuffer command_buffer, ComputeSet * compute_set);
void execute_render_set(VkCommandBuffer cmd, RenderSet * render_set);

void get_compute_local_size(PyObject * shader, uint32_t * size);

struct build_mipmaps_args {
    VkCommandBuffer cmd;
    uint32_t width;
    uint32_t height;
    uint32_t levels;
    uint32_t layers;
    uint32_t image_count;
    Image ** image_array;
};

void build_mipmaps(build_mipmaps_args args);

PyObject * Buffer_meth_write(Buffer * self, PyObject * arg);

struct BufferDescriptorUpdateInfo {
    Buffer * buffer;
    VkDescriptorType descriptor_type;
    VkDescriptorSet descriptor_set;
    uint32_t binding;
};

void buffer_descriptor_update(BufferDescriptorUpdateInfo info);
