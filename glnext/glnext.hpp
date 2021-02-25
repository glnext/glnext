#pragma once

#include <Python.h>
#include <structmember.h>

#include <vulkan/vulkan_core.h>

#ifdef BUILD_WINDOWS
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#define DEFAULT_SURFACE "VK_KHR_win32_surface"
#define DEFAULT_BACKEND "vulkan-1.dll"
#endif

#ifdef BUILD_LINUX
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#define DEFAULT_SURFACE "VK_KHR_xlib_surface"
#define DEFAULT_BACKEND "libvulkan.so"
#endif

#ifdef BUILD_DARWIN
#include <QuartzCore/CAMetalLayer.h>
#include <vulkan/vulkan_metal.h>
#define DEFAULT_SURFACE "VK_EXT_metal_surface"
#define DEFAULT_BACKEND NULL
#endif

typedef VkResult (VKAPI_PTR * PFN_vkCreateWin32SurfaceKHR)(VkInstance, const struct VkWin32SurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
typedef VkResult (VKAPI_PTR * PFN_vkCreateXlibSurfaceKHR)(VkInstance, const struct VkXlibSurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
typedef VkResult (VKAPI_PTR * PFN_vkCreateXcbSurfaceKHR)(VkInstance, const struct VkXcbSurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
typedef VkResult (VKAPI_PTR * PFN_vkCreateWaylandSurfaceKHR)(VkInstance, const struct VkWaylandSurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *);
typedef VkResult (VKAPI_PTR * PFN_vkCreateMetalSurfaceEXT)(VkInstance, const struct VkMetalSurfaceCreateInfoEXT *, const VkAllocationCallbacks *, VkSurfaceKHR *);

enum ImageMode {
    IMG_PROTECTED,
    IMG_TEXTURE,
    IMG_OUTPUT,
    IMG_STORAGE,
};

enum BufferMode {
    BUF_UNIFORM,
    BUF_STORAGE,
    BUF_INPUT,
    BUF_OUTPUT,
};

struct Image;
struct Buffer;
struct RenderPipeline;
struct ComputePipeline;
struct StagingBuffer;

struct RenderParameters {
    VkBool32 enabled;
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t index_count;
    uint32_t indirect_count;
    uint32_t max_draw_count;
    VkDeviceSize vertex_buffer_offset;
    VkDeviceSize instance_buffer_offset;
    VkDeviceSize index_buffer_offset;
    VkDeviceSize indirect_buffer_offset;
    VkDeviceSize count_buffer_offset;
};

struct ComputeParameters {
    VkBool32 enabled;
    uint32_t x, y, z;
};

typedef void (* RenderCommand)(RenderPipeline * self, VkCommandBuffer command_buffer);
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

struct StagingBufferBinding {
    union {
        PyObject * obj;
        Buffer * buffer;
        Image * image;
        RenderPipeline * render_pipeline;
        ComputePipeline * compute_pipeline;
    };
    VkDeviceSize offset;
    VkBool32 is_input;
    VkBool32 is_output;
};

struct SwapChainImages {
    uint32_t image_count;
    VkImage image_array[8];
};

struct Presenter {
    VkBool32 supported;
    uint32_t surface_count;
    VkSurfaceKHR surface_array[64];
    VkSwapchainKHR swapchain_array[64];
    VkPipelineStageFlags wait_stage_array[64];
    VkImageMemoryBarrier copy_image_barrier_array[64];
    VkImageMemoryBarrier present_image_barrier_array[64];
    VkSemaphore semaphore_array[64];
    VkImage image_source_array[64];
    VkImageBlit image_blit_array[64];
    SwapChainImages image_array[64];
    VkResult result_array[64];
    uint32_t index_array[64];
};

struct ModuleState {
    PyTypeObject * Instance_type;
    PyTypeObject * Batch_type;
    PyTypeObject * Framebuffer_type;
    PyTypeObject * RenderPipeline_type;
    PyTypeObject * ComputePipeline_type;
    PyTypeObject * Memory_type;
    PyTypeObject * Buffer_type;
    PyTypeObject * Image_type;
    PyTypeObject * StagingBuffer_type;

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

struct Extension {
    VkBool32 acceleration_structure;
    VkBool32 dedicated_allocation;
    VkBool32 deferred_host_operations;
    VkBool32 draw_indirect_count;
    VkBool32 mesh_shader;
    VkBool32 pipeline_library;
    VkBool32 ray_query;
    VkBool32 ray_tracing_pipeline;
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

    VkPipelineCache pipeline_cache;
    VkDebugUtilsMessengerEXT debug_messenger;

    VkBool32 debug;
    uint32_t api_version;
    uint32_t queue_family_index;
    uint32_t host_memory_type_index;
    uint32_t device_memory_type_index;
    VkFormat depth_format;

    Extension extension;
    Presenter presenter;

    PyObject * task_list;
    PyObject * staging_list;
    PyObject * memory_list;
    PyObject * buffer_list;
    PyObject * image_list;
    PyObject * log_list;

    ModuleState * state;

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkDestroyInstance vkDestroyInstance;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkGetDeviceQueue vkGetDeviceQueue;
    PFN_vkCreateFence vkCreateFence;
    PFN_vkCreateCommandPool vkCreateCommandPool;
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
    PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
    PFN_vkCreateShaderModule vkCreateShaderModule;
    PFN_vkDestroyShaderModule vkDestroyShaderModule;
    PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
    PFN_vkCreateImageView vkCreateImageView;
    PFN_vkCmdDispatch vkCmdDispatch;
    PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
    PFN_vkBindImageMemory vkBindImageMemory;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkCmdSetViewport vkCmdSetViewport;
    PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
    PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
    PFN_vkCmdPushConstants vkCmdPushConstants;
    PFN_vkUnmapMemory vkUnmapMemory;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
    PFN_vkCreateRenderPass vkCreateRenderPass;
    PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
    PFN_vkCreatePipelineCache vkCreatePipelineCache;
    PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
    PFN_vkCmdSetScissor vkCmdSetScissor;
    PFN_vkCmdBindPipeline vkCmdBindPipeline;
    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
    PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
    PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
    PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
    PFN_vkCreateImage vkCreateImage;
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
    PFN_vkCreateComputePipelines vkCreateComputePipelines;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkCreateFramebuffer vkCreateFramebuffer;
    PFN_vkMapMemory vkMapMemory;
    PFN_vkQueueSubmit vkQueueSubmit;
    PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
    PFN_vkCreateSampler vkCreateSampler;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkWaitForFences vkWaitForFences;
    PFN_vkResetFences vkResetFences;
    PFN_vkCreateSemaphore vkCreateSemaphore;
    PFN_vkDestroySemaphore vkDestroySemaphore;
    PFN_vkCmdCopyImage vkCmdCopyImage;
    PFN_vkCmdBlitImage vkCmdBlitImage;

    PFN_vkCmdDraw vkCmdDraw;
    PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
    PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
    PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
    PFN_vkCmdDrawIndirectCount vkCmdDrawIndirectCount;
    PFN_vkCmdDrawIndexedIndirectCount vkCmdDrawIndexedIndirectCount;
    PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV;
    PFN_vkCmdDrawMeshTasksIndirectNV vkCmdDrawMeshTasksIndirectNV;
    PFN_vkCmdDrawMeshTasksIndirectCountNV vkCmdDrawMeshTasksIndirectCountNV;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
    PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
    PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
    PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
    PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
};

struct Batch {
    PyObject_HEAD
    Instance * instance;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkSemaphore semaphore;
    VkBool32 present;
};

struct DescriptorBinding {
    PyObject * type;
    PyObject * name;
    uint32_t binding;
    VkDescriptorType descriptor_type;
    bool is_buffer;
    bool is_image;
    bool is_new;
    struct {
        Buffer * buffer;
        VkDeviceSize size;
        VkBufferUsageFlags usage;
        VkDescriptorBufferInfo descriptor_buffer_info;
        BufferMode mode;
    } buffer;
    struct {
        VkBool32 sampled;
        uint32_t image_count;
        Image ** image_array;
        VkDescriptorImageInfo * descriptor_image_info_array;
        VkImageViewCreateInfo * image_view_create_info_array;
        VkSamplerCreateInfo * sampler_create_info_array;
        VkImageView * image_view_array;
        VkSampler * sampler_array;
        VkImageLayout layout;
    } image;
    VkDescriptorSetLayoutBinding descriptor_binding;
    VkDescriptorPoolSize descriptor_pool_size;
    VkWriteDescriptorSet write_descriptor_set;
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
    RenderParameters parameters;
    RenderCommand render_command;
    Buffer * vertex_buffer;
    Buffer * instance_buffer;
    Buffer * index_buffer;
    Buffer * indirect_buffer;
    Buffer * count_buffer;
    uint32_t binding_count;
    DescriptorBinding * binding_array;
    VkDescriptorSetLayoutBinding * descriptor_binding_array;
    VkDescriptorPoolSize * descriptor_pool_size_array;
    VkWriteDescriptorSet * write_descriptor_set_array;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    uint32_t attribute_count;
    VkBuffer * attribute_buffer_array;
    VkDeviceSize * attribute_offset_array;
    VkPipeline pipeline;
    PyObject * members;
};

struct ComputePipeline {
    PyObject_HEAD
    Instance * instance;
    ComputeParameters parameters;
    uint32_t binding_count;
    DescriptorBinding * binding_array;
    VkDescriptorSetLayoutBinding * descriptor_binding_array;
    VkDescriptorPoolSize * descriptor_pool_size_array;
    VkWriteDescriptorSet * write_descriptor_set_array;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkPipeline pipeline;
    PyObject * members;
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
    VkBool32 bound;
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
    VkBool32 bound;
};

struct StagingBuffer {
    PyObject_HEAD
    Instance * instance;
    VkDeviceSize size;
    VkBuffer buffer;
    VkDeviceMemory memory;
    char * ptr;
    uint32_t binding_count;
    StagingBufferBinding * binding_array;
    PyObject * mem;
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

struct BuildMipmapsInfo {
    Instance * instance;
    VkCommandBuffer command_buffer;
    uint32_t width;
    uint32_t height;
    uint32_t levels;
    uint32_t layers;
    uint32_t image_count;
    Image ** image_array;
};

template <typename T>
T * allocate(uint32_t count) {
    return (T *)PyMem_Malloc(sizeof(T) * count);
}

inline bool has_key(PyObject * dict, const char * key) {
    return !!PyDict_GetItemString(dict, key);
}

PyObject * Buffer_meth_write(Buffer * self, PyObject * arg);

PFN_vkGetInstanceProcAddr get_instance_proc_addr(const char * backend);

void load_library_methods(Instance * instance);
void load_instance_methods(Instance * instance);
void load_device_methods(Instance * instance);

PyObject * get_instance_layers(PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties);
PyObject * get_instance_extensions(PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties);
PyObject * get_device_extensions(VkPhysicalDevice physical_device, PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties);

uint32_t load_instance_layers(Instance * instance, const char ** array, PyObject * extra_layers);
uint32_t load_instance_extensions(Instance * instance, const char ** array, const char * surface);
uint32_t load_device_extensions(Instance * instance, const char ** array);

void install_debug_messenger(Instance * instance);

int parse_descriptor_binding(Instance * instance, DescriptorBinding * binding, PyObject * obj);
void create_descriptor_binding_objects(Instance * instance, DescriptorBinding * binding, Memory * memory);
void bind_descriptor_binding_objects(Instance * instance, DescriptorBinding * binding);

void execute_instance(Instance * self);
void execute_framebuffer(Framebuffer * self, VkCommandBuffer command_buffer);
void execute_render_pipeline(RenderPipeline * self, VkCommandBuffer command_buffer);
void execute_compute_pipeline(ComputePipeline * self, VkCommandBuffer command_buffer);
void execute_staging_buffer_input(StagingBuffer * self, VkCommandBuffer command_buffer);
void execute_staging_buffer_output(StagingBuffer * self, VkCommandBuffer command_buffer);

VkCommandBuffer begin_commands(Instance * instance);
void end_commands(Instance * instance);
void end_commands_with_present(Instance * instance);

Memory * new_memory(Instance * instance, VkBool32 host = false);
Memory * get_memory(Instance * instance, PyObject * memory);

VkDeviceSize take_memory(Memory * self, VkMemoryRequirements * requirements);

void allocate_memory(Memory * self, VkMemoryDedicatedAllocateInfo * dedicated = NULL);
void free_memory(Memory * self);

Image * new_image(ImageCreateInfo info);
Buffer * new_buffer(BufferCreateInfo info);

void bind_image(Image * image);
void bind_buffer(Buffer * buffer);

void new_temp_buffer(Instance * instance, HostBuffer * temp, VkDeviceSize size);
void free_temp_buffer(Instance * instance, HostBuffer * temp);

void copy_present_images(Instance * self);
void present_images(Instance * self);

void build_mipmaps(BuildMipmapsInfo args);

VkPrimitiveTopology get_topology(PyObject * name);
ImageMode get_image_mode(PyObject * name);
Format get_format(PyObject * name);

void presenter_remove(Presenter * presenter, uint32_t index);
