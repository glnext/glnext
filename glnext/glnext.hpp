#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#define VK_NO_PROTOTYPES
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
    VkImageBlit * image_blit_array;
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

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkDestroyInstance vkDestroyInstance;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
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
    PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
    PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
    PFN_vkCreateImageView vkCreateImageView;
    PFN_vkCmdDispatch vkCmdDispatch;
    PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
    PFN_vkBindImageMemory vkBindImageMemory;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkCmdSetViewport vkCmdSetViewport;
    PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
    PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
    PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
    PFN_vkCmdPushConstants vkCmdPushConstants;
    PFN_vkUnmapMemory vkUnmapMemory;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
    PFN_vkCreateRenderPass vkCreateRenderPass;
    PFN_vkCmdDraw vkCmdDraw;
    PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
    PFN_vkCmdSetScissor vkCmdSetScissor;
    PFN_vkCmdBindPipeline vkCmdBindPipeline;
    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
    PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
    PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
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

struct Image;
struct Buffer;

struct BufferBinding {
    PyObject * name;
    uint32_t binding;
    Buffer * buffer;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkDescriptorType descriptor_type;
    VkDescriptorBufferInfo descriptor_buffer_info;
    BufferMode mode;
};

struct ImageBinding {
    PyObject * name;
    uint32_t binding;
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
    VkDescriptorSetLayoutBinding * descriptor_binding_array;
    VkDescriptorPoolSize * descriptor_pool_size_array;
    VkWriteDescriptorSet * write_descriptor_set_array;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    uint32_t attribute_count;
    VkBuffer * attribute_buffer_array;
    VkDeviceSize * attribute_offset_array;
    VkPipeline pipeline;
    PyObject * members;
};

struct ComputePipeline {
    PyObject_HEAD
    Instance * instance;
    ComputeCount compute_count;
    uint32_t buffer_count;
    uint32_t image_count;
    BufferBinding * buffer_array;
    ImageBinding * image_array;
    VkDescriptorSetLayoutBinding * descriptor_binding_array;
    VkDescriptorPoolSize * descriptor_pool_size_array;
    VkWriteDescriptorSet * write_descriptor_set_array;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule compute_shader_module;
    VkPipeline pipeline;
    Buffer * uniform_buffer;
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

template <typename T>
T * allocate(uint32_t count) {
    return (T *)PyMem_Malloc(sizeof(T) * count);
}

PFN_vkGetInstanceProcAddr get_instance_proc_addr(const char * backend);
void load_library_methods(Instance * instance);
void load_instance_methods(Instance * instance);
void load_device_methods(Instance * instance);

void install_debug_messenger(Instance * instance);

BufferBinding parse_buffer_binding(PyObject * obj);
ImageBinding parse_image_binding(PyObject * obj);

void execute_framebuffer(Framebuffer * self);
void execute_render_pipeline(RenderPipeline * self);
void execute_compute_pipeline(ComputePipeline * self);

VkCommandBuffer begin_commands(Instance * instance);
void end_commands(Instance * instance);
void end_commands_with_present(Instance * instance);

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

void presenter_resize(Presenter * presenter);
void presenter_remove(Presenter * presenter, uint32_t index);
