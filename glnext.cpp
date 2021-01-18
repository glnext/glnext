#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <vulkan/vulkan_core.h>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

enum PackType {
    PACK_FLOAT,
    PACK_INT,
    PACK_BYTE,
    PACK_PAD,
};

enum ImageMode {
    IMG_PROTECTED,
    IMG_TEXTURE,
    IMG_OUTPUT,
};

struct Format {
    VkFormat format;
    int size;
    PackType pack_type;
    int pack_count;
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

struct Instance;
struct Renderer;
struct Pipeline;
struct Memory;
struct Buffer;
struct Image;
struct Sampler;

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
    Memory * device_memory;
    Memory * host_memory;
    Buffer * host_buffer;
    Memory * staging_memory;
    Buffer * staging_buffer;
    PyObject * staging_memoryview;
    PyObject * renderer_list;
    PyObject * memory_list;
    PyObject * buffer_list;
    PyObject * image_list;
};

struct Renderer {
    PyObject_HEAD
    Instance * instance;
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    VkRenderPass render_pass;
    PyObject * framebuffers;
    PyObject * resolve_images;
    PyObject * clear_values;
    Buffer * uniform_buffer;
    PyObject * output;
    PyObject * pipeline_list;
};

struct Pipeline {
    PyObject_HEAD
    Instance * instance;
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
};

struct Image {
    PyObject_HEAD
    Instance * instance;
    Memory * memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkExtent3D extent;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    ImageMode mode;
    VkFormat format;
    VkImage image;
};

struct Sampler {
    PyObject_HEAD
    Image * image;
    VkSamplerCreateInfo info;
    VkImageSubresourceRange subresource_range;
    VkComponentMapping component_mapping;
};

PyTypeObject * Instance_type;
PyTypeObject * Renderer_type;
PyTypeObject * Pipeline_type;
PyTypeObject * Memory_type;
PyTypeObject * Buffer_type;
PyTypeObject * Image_type;
PyTypeObject * Sampler_type;

PyObject * empty_str;
PyObject * empty_list;
PyObject * default_topology;
PyObject * default_front_face;
PyObject * default_border_color;
PyObject * default_format;
PyObject * default_filter;
PyObject * default_address_mode;
PyObject * default_swizzle;
PyObject * texture_str;
PyObject * output_str;

template <typename T>
void realloc_array(T ** array, uint32_t size) {
    *array = (T *)PyMem_Realloc(*array, sizeof(T) * size);
}

void presenter_resize(Presenter * presenter) {
    realloc_array(&presenter->surface_array, presenter->surface_count);
    realloc_array(&presenter->swapchain_array, presenter->surface_count);
    realloc_array(&presenter->wait_stage_array, presenter->surface_count);
    realloc_array(&presenter->semaphore_array, presenter->surface_count);
    realloc_array(&presenter->image_source_array, presenter->surface_count);
    realloc_array(&presenter->image_copy_array, presenter->surface_count);
    realloc_array(&presenter->image_count_array, presenter->surface_count);
    realloc_array(&presenter->image_array, presenter->surface_count);
    realloc_array(&presenter->result_array, presenter->surface_count);
    realloc_array(&presenter->index_array, presenter->surface_count);
}

void presenter_remove(Presenter * presenter, uint32_t index) {
    PyMem_Free(presenter->image_array[index]);
    presenter->surface_count -= 1;
    for (uint32_t i = index; i < presenter->surface_count; ++i) {
        presenter->surface_array[i] = presenter->surface_array[i + 1];
        presenter->swapchain_array[i] = presenter->swapchain_array[i + 1];
        presenter->wait_stage_array[i] = presenter->wait_stage_array[i + 1];
        presenter->semaphore_array[i] = presenter->semaphore_array[i + 1];
        presenter->image_source_array[i] = presenter->image_source_array[i + 1];
        presenter->image_copy_array[i] = presenter->image_copy_array[i + 1];
        presenter->image_count_array[i] = presenter->image_count_array[i + 1];
        presenter->image_array[i] = presenter->image_array[i + 1];
        presenter->result_array[i] = presenter->result_array[i + 1];
        presenter->index_array[i] = presenter->index_array[i + 1];
    }
    presenter_resize(presenter);
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

VkDeviceSize take_memory(Memory * self, VkMemoryRequirements requirements) {
    if (VkDeviceSize padding = self->offset % requirements.alignment) {
        self->offset += requirements.alignment - padding;
    }
    VkDeviceSize res = self->offset;
    self->offset += requirements.size;
    return res;
}

VkCommandBuffer begin_commands(Instance * instance) {
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        NULL,
    };

    vkBeginCommandBuffer(instance->command_buffer, &command_buffer_begin_info);
    return instance->command_buffer;
}

void end_commands(Instance * instance) {
    vkEndCommandBuffer(instance->command_buffer);

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        0,
        NULL,
        NULL,
        1,
        &instance->command_buffer,
        0,
        NULL,
    };

    vkQueueSubmit(instance->queue, 1, &submit_info, instance->fence);
    vkWaitForFences(instance->device, 1, &instance->fence, true, UINT64_MAX);
    vkResetFences(instance->device, 1, &instance->fence);
}

void allocate_memory(Memory * self, VkDeviceSize size = 0) {
    if (self->size) {
        return;
    }

    if (size < self->offset) {
        size = self->offset;
    }

    if (!size) {
        return;
    }

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        NULL,
        size,
        self->host ? self->instance->host_memory_type_index : self->instance->device_memory_type_index,
    };

    self->size = size;
    vkAllocateMemory(self->instance->device, &memory_allocate_info, NULL, &self->memory);

    if (self->host) {
        vkMapMemory(self->instance->device, self->memory, 0, VK_WHOLE_SIZE, 0, &self->ptr);
    }
}

void free_memory(Memory * self) {
    if (self->host) {
        vkUnmapMemory(self->instance->device, self->memory);
    }
    vkFreeMemory(self->instance->device, self->memory, NULL);
    self->memory = NULL;
    self->ptr = NULL;
    self->offset = 0;
    self->size = 0;
}

void bind_buffer(Buffer * self) {
    vkBindBufferMemory(self->instance->device, self->buffer, self->memory->memory, self->offset);
}

void bind_image(Image * self) {
    vkBindImageMemory(self->instance->device, self->image, self->memory->memory, self->offset);
}

void resize_buffer(Buffer * self, VkDeviceSize size) {
    if (size <= self->size) {
        return;
    }

    self->size = size;

    vkDestroyBuffer(self->instance->device, self->buffer, NULL);
    free_memory(self->memory);

    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        size,
        self->usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    vkCreateBuffer(self->instance->device, &buffer_create_info, NULL, &self->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(self->instance->device, self->buffer, &requirements);
    self->offset = take_memory(self->memory, requirements);
    allocate_memory(self->memory);
    bind_buffer(self);
}

void update_buffer(Buffer * self, PyObject * data) {
    if (data == Py_None) {
        return;
    }

    Py_buffer view = {};
    PyObject_GetBuffer(data, &view, PyBUF_STRIDED_RO);
    resize_buffer(self->instance->host_buffer, (VkDeviceSize)view.len);
    PyBuffer_ToContiguous(self->instance->host_memory->ptr, &view, view.len, 'C');

    VkCommandBuffer cmd = begin_commands(self->instance);

    VkBufferCopy copy = {0, 0, (VkDeviceSize)view.len};
    vkCmdCopyBuffer(cmd, self->instance->host_buffer->buffer, self->buffer, 1, &copy);

    end_commands(self->instance);
    PyBuffer_Release(&view);
}

struct new_memory_args {
    Instance * instance;
    VkBool32 host;
};

Memory * new_memory(new_memory_args args) {
    Memory * res = PyObject_New(Memory, Memory_type);
    res->instance = args.instance;
    res->memory = NULL;
    res->offset = 0;
    res->size = 0;
    res->host = args.host;
    res->ptr = NULL;
    PyList_Append(args.instance->memory_list, (PyObject *)res);
    return res;
}

struct new_buffer_args {
    Instance * instance;
    Memory * memory;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
};

Buffer * new_buffer(new_buffer_args args) {
    Buffer * res = PyObject_New(Buffer, Buffer_type);
    res->instance = args.instance;
    res->memory = args.memory;
    res->offset = 0;
    res->size = args.size;
    res->usage = args.usage;
    res->buffer = NULL;

    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        args.size,
        args.usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    vkCreateBuffer(args.instance->device, &buffer_create_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(args.instance->device, res->buffer, &requirements);
    res->offset = take_memory(args.memory, requirements);

    PyList_Append(args.instance->buffer_list, (PyObject *)res);
    return res;
}

struct new_image_args {
    Instance * instance;
    Memory * memory;
    VkDeviceSize size;
    VkImageUsageFlags usage;
    VkExtent3D extent;
    uint32_t samples;
    uint32_t levels;
    uint32_t layers;
    ImageMode mode;
    VkFormat format;
};

Image * new_image(new_image_args args) {
    Image * res = PyObject_New(Image, Image_type);
    res->instance = args.instance;
    res->memory = args.memory;
    res->offset = 0;
    res->size = args.size;
    res->extent = args.extent;
    res->samples = args.samples;
    res->levels = args.levels;
    res->layers = args.layers;
    res->mode = args.mode;
    res->format = args.format;
    res->image = NULL;

    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        0,
        VK_IMAGE_TYPE_2D,
        args.format,
        args.extent,
        args.levels,
        args.layers,
        (VkSampleCountFlagBits)args.samples,
        VK_IMAGE_TILING_OPTIMAL,
        args.usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    vkCreateImage(args.instance->device, &image_create_info, NULL, &res->image);

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(args.instance->device, res->image, &requirements);
    res->offset = take_memory(args.memory, requirements);

    PyList_Append(args.instance->image_list, (PyObject *)res);
    return res;
}

Memory * get_memory(Instance * instance, PyObject * memory) {
    if (memory == Py_None) {
        return new_memory({instance, false});
    }
    if (PyObject_Type(memory) == (PyObject *)Memory_type) {
        return (Memory *)memory;
    }
    PyErr_Format(PyExc_TypeError, "memory");
    return NULL;
}

Format get_format(PyObject * name) {
    const char * s = PyUnicode_AsUTF8(name);
    if (!strcmp(s, "1f")) return {VK_FORMAT_R32_SFLOAT, 4, PACK_FLOAT, 1};
    if (!strcmp(s, "2f")) return {VK_FORMAT_R32G32_SFLOAT, 8, PACK_FLOAT, 2};
    if (!strcmp(s, "3f")) return {VK_FORMAT_R32G32B32_SFLOAT, 12, PACK_FLOAT, 3};
    if (!strcmp(s, "4f")) return {VK_FORMAT_R32G32B32A32_SFLOAT, 16, PACK_FLOAT, 4};
    if (!strcmp(s, "1i")) return {VK_FORMAT_R32_SINT, 4, PACK_INT, 1};
    if (!strcmp(s, "2i")) return {VK_FORMAT_R32G32_SINT, 8, PACK_INT, 2};
    if (!strcmp(s, "3i")) return {VK_FORMAT_R32G32B32_SINT, 12, PACK_INT, 3};
    if (!strcmp(s, "4i")) return {VK_FORMAT_R32G32B32A32_SINT, 16, PACK_INT, 4};
    if (!strcmp(s, "1b")) return {VK_FORMAT_R8_UNORM, 1, PACK_BYTE, 1};
    if (!strcmp(s, "2b")) return {VK_FORMAT_R8G8_UNORM, 2, PACK_BYTE, 2};
    if (!strcmp(s, "3b")) return {VK_FORMAT_R8G8B8_UNORM, 3, PACK_BYTE, 3};
    if (!strcmp(s, "4b")) return {VK_FORMAT_B8G8R8A8_UNORM, 4, PACK_BYTE, 4};
    if (!strcmp(s, "1x")) return {VK_FORMAT_UNDEFINED, 1, PACK_PAD, 1};
    if (!strcmp(s, "2x")) return {VK_FORMAT_UNDEFINED, 2, PACK_PAD, 2};
    if (!strcmp(s, "3x")) return {VK_FORMAT_UNDEFINED, 3, PACK_PAD, 3};
    if (!strcmp(s, "4x")) return {VK_FORMAT_UNDEFINED, 4, PACK_PAD, 4};
    PyErr_Format(PyExc_ValueError, "format");
    return {};
}

VkCullModeFlags get_cull_mode(PyObject * name) {
    if (name == Py_None) {
        return VK_CULL_MODE_NONE;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "front")) {
        return VK_CULL_MODE_FRONT_BIT;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "back")) {
        return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
}

VkFrontFace get_front_face(PyObject * name) {
    if (!PyUnicode_CompareWithASCIIString(name, "counter_clockwise")) {
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "clockwise")) {
        return VK_FRONT_FACE_CLOCKWISE;
    }
    return VK_FRONT_FACE_MAX_ENUM;
}

VkPrimitiveTopology get_topology(PyObject * name) {
    if (!PyUnicode_CompareWithASCIIString(name, "points")) {
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "lines")) {
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "line_strip")) {
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "triangles")) {
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "triangle_strip")) {
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "triangle_fan")) {
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    }
    PyErr_Format(PyExc_ValueError, "topology");
    return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

VkFilter get_filter(PyObject * name) {
    if (!PyUnicode_CompareWithASCIIString(name, "nearest")) {
        return VK_FILTER_NEAREST;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "linear")) {
        return VK_FILTER_LINEAR;
    }
    PyErr_Format(PyExc_ValueError, "filter");
    return VK_FILTER_MAX_ENUM;
}

VkSamplerMipmapMode get_mipmap_filter(PyObject * name) {
    if (!PyUnicode_CompareWithASCIIString(name, "nearest")) {
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "linear")) {
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    PyErr_Format(PyExc_ValueError, "filter");
    return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
}

VkSamplerAddressMode get_address_mode(PyObject * name) {
    if (!PyUnicode_CompareWithASCIIString(name, "repeat")) {
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "mirrored_repeat")) {
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "clamp_to_edge")) {
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "clamp_to_border")) {
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "mirror_clamp_to_edge")) {
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    }
    PyErr_Format(PyExc_ValueError, "address_mode");
    return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
}

ImageMode get_image_mode(PyObject * name) {
    if (!PyUnicode_CompareWithASCIIString(name, "texture")) {
        return IMG_TEXTURE;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "output")) {
        return IMG_OUTPUT;
    }
    return IMG_PROTECTED;
}

VkBorderColor get_border_color(PyObject * name, bool is_float) {
    if (!PyUnicode_CompareWithASCIIString(name, "transparent")) {
        return is_float ? VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK : VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "black")) {
        return is_float ? VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK : VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    }
    if (!PyUnicode_CompareWithASCIIString(name, "white")) {
        return is_float ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    }
    PyErr_Format(PyExc_ValueError, "border_color");
    return VK_BORDER_COLOR_MAX_ENUM;
}

struct build_mipmaps_args {
    VkCommandBuffer cmd;
    uint32_t width;
    uint32_t height;
    uint32_t levels;
    uint32_t layers;
    uint32_t image_count;
    Image * image_array;
};

void build_mipmaps(build_mipmaps_args args) {
    for (uint32_t level = 1; level < args.levels; ++level) {
        uint32_t parent = level - 1;
        VkImageBlit image_blit = {
            {VK_IMAGE_ASPECT_COLOR_BIT, parent, 0, args.layers},
            {{0, 0, 0}, {(int32_t)args.width >> parent, (int32_t)args.height >> parent, 1}},
            {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, args.layers},
            {{0, 0, 0}, {(int32_t)args.width >> level, (int32_t)args.height >> level, 1}},
        };

        for (uint32_t i = 0; i < args.image_count; ++i) {
            vkCmdBlitImage(
                args.cmd,
                args.image_array[i].image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                args.image_array[i].image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &image_blit,
                VK_FILTER_LINEAR
            );
        }

        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[64];

        for (uint32_t i = 0; i < args.image_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                args.image_array[i].image,
                {VK_IMAGE_ASPECT_COLOR_BIT, level, 1, 0, args.layers},
            };
        }

        vkCmdPipelineBarrier(
            args.cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            image_barrier_count,
            image_barrier_array
        );
    }

    if (args.levels > 1) {
        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[128];

        for (uint32_t i = 0; i < args.image_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                args.image_array[i].image,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, args.levels - 1, 0, args.layers},
            };

            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                args.image_array[i].image,
                {VK_IMAGE_ASPECT_COLOR_BIT, args.levels - 1, 1, 0, args.layers},
            };
        }

        vkCmdPipelineBarrier(
            args.cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            image_barrier_count,
            image_barrier_array
        );
    }
}

void render(VkCommandBuffer cmd, Renderer * renderer) {
    VkClearValue * clear_value_array = NULL;
    uint32_t clear_value_count = retreive_array(renderer->clear_values, &clear_value_array);
    VkFramebuffer * framebuffer_array = NULL;
    retreive_array(renderer->framebuffers, &framebuffer_array);

    for (uint32_t layer = 0; layer < renderer->layers; ++layer) {
        VkRenderPassBeginInfo render_pass_begin_info = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            NULL,
            renderer->render_pass,
            framebuffer_array[layer],
            {{0, 0}, {renderer->width, renderer->height}},
            clear_value_count,
            clear_value_array,
        };

        vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {0.0f, 0.0f, (float)renderer->width, (float)renderer->height, 0.0f, 1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {{0, 0}, {renderer->width, renderer->height}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        for (int i = 0; i < PyList_Size(renderer->pipeline_list); ++i) {
            Pipeline * pipeline = (Pipeline *)PyList_GetItem(renderer->pipeline_list, i);

            VkBuffer * vertex_buffer_array = NULL;
            uint32_t vertex_buffer_count = retreive_array(pipeline->vertex_buffers, &vertex_buffer_array);
            const VkDeviceSize vertex_buffer_offset_array[64] = {};

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

            if (vertex_buffer_count) {
                vkCmdBindVertexBuffers(
                    cmd,
                    0,
                    vertex_buffer_count,
                    vertex_buffer_array,
                    vertex_buffer_offset_array
                );
            }

            if (pipeline->descriptor_set) {
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->pipeline_layout,
                    0,
                    1,
                    &pipeline->descriptor_set,
                    0,
                    NULL
                );
            }

            if (pipeline->index_buffer) {
                vkCmdBindIndexBuffer(cmd, pipeline->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            }

            vkCmdPushConstants(
                cmd,
                pipeline->pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                4,
                &layer
            );

            if (pipeline->indirect_count && pipeline->index_count) {
                vkCmdDrawIndexedIndirect(cmd, pipeline->indirect_buffer->buffer, 0, pipeline->indirect_count, 20);
            } else if (pipeline->indirect_count) {
                vkCmdDrawIndirect(cmd, pipeline->indirect_buffer->buffer, 0, pipeline->indirect_count, 16);
            } else if (pipeline->index_count) {
                vkCmdDrawIndexed(cmd, pipeline->index_count, pipeline->instance_count, 0, 0, 0);
            } else {
                vkCmdDraw(cmd, pipeline->vertex_count, pipeline->instance_count, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd);
    }

    if (renderer->levels > 1) {
        Image * resolve_image_array = NULL;
        uint32_t resolve_image_count = retreive_array(renderer->resolve_images, &resolve_image_array);

        build_mipmaps({
            cmd,
            renderer->width,
            renderer->height,
            renderer->levels,
            renderer->layers,
            resolve_image_count,
            resolve_image_array,
        });
    }
}

Instance * glnext_meth_instance(PyObject * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"physical_device", "host_memory", "device_memory", "staging_buffer", NULL};

    struct {
        uint32_t physical_device = 0;
        VkDeviceSize host_memory_size = 0;
        VkDeviceSize device_memory_size = 0;
        VkDeviceSize staging_buffer_size = 0;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$Ikkk",
        keywords,
        &args.physical_device,
        &args.host_memory_size,
        &args.device_memory_size,
        &args.staging_buffer_size
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.host_memory_size < 64 * 1024) {
        args.host_memory_size = 64 * 1024;
    }

    Instance * res = PyObject_New(Instance, Instance_type);

    res->instance = NULL;
    res->physical_device = NULL;
    res->device = NULL;
    res->queue = NULL;
    res->command_pool = NULL;
    res->command_buffer = NULL;
    res->fence = NULL;

    res->presenter = {};

    res->renderer_list = PyList_New(0);
    res->memory_list = PyList_New(0);
    res->buffer_list = PyList_New(0);
    res->image_list = PyList_New(0);

    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        NULL,
        "application",
        0,
        "engine",
        0,
        VK_API_VERSION_1_0,
    };

    const char * instance_layer_array[] = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_gfxreconstruct"};
    const char * instance_extension_array[] = {"VK_EXT_debug_utils", "VK_KHR_surface", "VK_KHR_win32_surface"};

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        NULL,
        0,
        &application_info,
        1,
        instance_layer_array,
        3,
        instance_extension_array,
    };

    vkCreateInstance(&instance_create_info, NULL, &res->instance);

    uint32_t physical_device_count = 0;
    VkPhysicalDevice physical_device_array[64] = {};
    vkEnumeratePhysicalDevices(res->instance, &physical_device_count, NULL);
    vkEnumeratePhysicalDevices(res->instance, &physical_device_count, physical_device_array);
    res->physical_device = physical_device_array[args.physical_device];

    VkPhysicalDeviceMemoryProperties device_memory_properties = {};
    vkGetPhysicalDeviceMemoryProperties(res->physical_device, &device_memory_properties);

    VkPhysicalDeviceFeatures supported_features = {};
    vkGetPhysicalDeviceFeatures(res->physical_device, &supported_features);

    res->host_memory_type_index = 0;
    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->host_memory_type_index = i;
            break;
        }
    }

    res->device_memory_type_index = 0;
    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->device_memory_type_index = i;
            break;
        }
    }

    VkFormat depth_formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    for (uint32_t i = 0; i < 3; ++i) {
        VkFormatProperties format_properties = {};
        vkGetPhysicalDeviceFormatProperties(res->physical_device, depth_formats[i], &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            res->depth_format = depth_formats[i];
        }
    }

    uint32_t queue_family_properties_count = 0;
    VkQueueFamilyProperties queue_family_properties_array[64];
    vkGetPhysicalDeviceQueueFamilyProperties(res->physical_device, &queue_family_properties_count, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(res->physical_device, &queue_family_properties_count, queue_family_properties_array);

    res->queue_family_index = 0;
    for (uint32_t i = 0; i < queue_family_properties_count; ++i) {
        if (queue_family_properties_array[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            res->queue_family_index = i;
            break;
        }
    }

    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo device_queue_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        res->queue_family_index,
        1,
        &queue_priority,
    };

    VkPhysicalDeviceFeatures physical_device_features = {};
    physical_device_features.multiDrawIndirect = supported_features.multiDrawIndirect;
    physical_device_features.samplerAnisotropy = supported_features.samplerAnisotropy;

    const char * device_extension_array[] = {"VK_KHR_swapchain"};

    VkDeviceCreateInfo device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        NULL,
        0,
        1,
        &device_queue_create_info,
        0,
        NULL,
        1,
        device_extension_array,
        &physical_device_features,
    };

    vkCreateDevice(res->physical_device, &device_create_info, NULL, &res->device);
    vkGetDeviceQueue(res->device, res->queue_family_index, 0, &res->queue);

    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0};
    vkCreateFence(res->device, &fence_create_info, NULL, &res->fence);

    VkCommandPoolCreateInfo command_pool_create_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        res->queue_family_index,
    };

    vkCreateCommandPool(res->device, &command_pool_create_info, NULL, &res->command_pool);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        res->command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1,
    };

    vkAllocateCommandBuffers(res->device, &command_buffer_allocate_info, &res->command_buffer);

    res->device_memory = NULL;
    if (args.device_memory_size) {
        res->device_memory = new_memory({res, false});
    }

    res->host_memory = new_memory({res, true});
    res->host_buffer = new_buffer({
        res,
        res->host_memory,
        args.host_memory_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    });

    allocate_memory(res->host_memory);
    bind_buffer(res->host_buffer);

    res->staging_memory = NULL;
    res->staging_buffer = NULL;
    res->staging_memoryview = NULL;

    if (args.staging_buffer_size) {
        res->staging_memory = new_memory({res, true});
        res->staging_buffer = new_buffer({
            res,
            res->staging_memory,
            args.staging_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        });

        allocate_memory(res->staging_memory);
        bind_buffer(res->staging_buffer);

        res->staging_memoryview = PyMemoryView_FromMemory((char *)res->staging_memory->ptr, args.staging_buffer_size, PyBUF_WRITE);
    }

    return res;
}

Image * Instance_meth_image(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "size",
        "format",
        "samples",
        "levels",
        "layers",
        "mode",
        "memory",
        NULL,
    };

    struct {
        uint32_t width;
        uint32_t height;
        PyObject * format = default_format;
        uint32_t samples = 1;
        uint32_t levels = 1;
        uint32_t layers = 1;
        PyObject * mode = texture_str;
        PyObject * memory = Py_None;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "(II)|O$IIIOO",
        keywords,
        &args.width,
        &args.height,
        &args.format,
        &args.samples,
        &args.levels,
        &args.layers,
        &args.mode,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    Memory * memory = get_memory(self, args.memory);
    ImageMode image_mode = get_image_mode(args.mode);
    Format format = get_format(args.format);

    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (image_mode == IMG_TEXTURE) {
        image_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    Image * res = new_image({
        self,
        memory,
        args.width * args.height * args.layers * format.size,
        image_usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        {args.width, args.height, 1},
        args.samples,
        args.levels,
        args.layers,
        image_mode,
        format.format,
    });

    allocate_memory(memory);
    bind_image(res);

    VkCommandBuffer cmd = begin_commands(self);

    VkImageLayout image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if (image_mode == IMG_TEXTURE) {
        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkImageMemoryBarrier image_barrier_transfer = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        0,
        VK_IMAGE_LAYOUT_UNDEFINED,
        image_layout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        res->image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, args.layers},
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &image_barrier_transfer
    );

    end_commands(self);

    PyList_Append(self->image_list, (PyObject *)res);
    return res;
}

Sampler * Instance_meth_sampler(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "image",
        "min_filter",
        "mag_filter",
        "mipmap_filter",
        "address_mode_x",
        "address_mode_y",
        "address_mode_z",
        "normalized",
        "anisotropy",
        "max_anisotrpoy",
        "min_lod",
        "max_lod",
        "lod_bias",
        "border_color",
        "swizzle",
        "layer",
        "layers",
        NULL,
    };

    struct {
        Image * image;
        PyObject * min_filter = default_filter;
        PyObject * mag_filter = default_filter;
        PyObject * mipmap_filter = default_filter;
        PyObject * address_mode_x = default_address_mode;
        PyObject * address_mode_y = default_address_mode;
        PyObject * address_mode_z = default_address_mode;
        VkBool32 normalized = true;
        VkBool32 anisotropy = false;
        float max_anisotrpoy = 1.0f;
        float min_lod = 0.0f;
        float max_lod = 1000.0f;
        float lod_bias = 0.0f;
        PyObject * border_color = default_border_color;
        PyObject * swizzle = default_swizzle;
        uint32_t layer = 0;
        uint32_t layers = 1;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "O!|$OOOOOOppffffOOII",
        keywords,
        Image_type,
        &args.image,
        &args.address_mode_x,
        &args.address_mode_y,
        &args.address_mode_z,
        &args.min_filter,
        &args.mag_filter,
        &args.mipmap_filter,
        &args.normalized,
        &args.anisotropy,
        &args.max_anisotrpoy,
        &args.min_lod,
        &args.max_lod,
        &args.lod_bias,
        &args.border_color,
        &args.swizzle,
        &args.layer,
        &args.layers
    );

    if (!args_ok) {
        return NULL;
    }

    Sampler * res = PyObject_New(Sampler, Sampler_type);
    res->image = args.image;
    res->info = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        NULL,
        0,
        get_filter(args.min_filter),
        get_filter(args.mag_filter),
        get_mipmap_filter(args.mipmap_filter),
        get_address_mode(args.address_mode_x),
        get_address_mode(args.address_mode_y),
        get_address_mode(args.address_mode_z),
        args.lod_bias,
        args.anisotropy,
        args.max_anisotrpoy,
        false,
        VK_COMPARE_OP_NEVER,
        args.min_lod,
        args.max_lod,
        get_border_color(args.border_color, true),
        !args.normalized,
    };
    res->subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        1,
        args.layer,
        args.layers,
    };
    res->component_mapping = {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
    };
    return res;
}

PyObject * Instance_meth_surface(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "window",
        "image",
        NULL
    };

    struct {
        void * window;
        Image * image;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "n|O!",
        keywords,
        &args.window,
        Image_type,
        &args.image
    );

    if (!args_ok) {
        return NULL;
    }

    #if defined(_WIN32) || defined(_WIN64)

    VkWin32SurfaceCreateInfoKHR surface_create_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        NULL,
        0,
        NULL,
        (HWND)args.window,
    };

    VkSurfaceKHR surface = NULL;
    vkCreateWin32SurfaceKHR(self->instance, &surface_create_info, NULL, &surface);

    #elif defined(__APPLE__)

    // TODO:

    #else

    // TODO:

    #endif

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(self->physical_device, self->queue_family_index, surface, &present_support);

    uint32_t num_formats = 1;
    VkSurfaceFormatKHR surface_format = {};
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, surface, &num_formats, &surface_format);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self->physical_device, surface, &surface_capabilities);

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        NULL,
        0,
        surface,
        surface_capabilities.minImageCount,
        args.image->format,
        surface_format.colorSpace,
        surface_capabilities.currentExtent,
        1,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        surface_capabilities.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_PRESENT_MODE_FIFO_KHR,
        true,
        NULL,
    };

    VkSwapchainKHR swapchain = NULL;
    vkCreateSwapchainKHR(self->device, &swapchain_create_info, NULL, &swapchain);

    VkSemaphore semaphore = NULL;
    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0};
    vkCreateSemaphore(self->device, &semaphore_create_info, NULL, &semaphore);

    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(self->device, swapchain, &swapchain_image_count, NULL);
    VkImage * swapchain_image_array = (VkImage *)PyMem_Malloc(sizeof(VkImage) * swapchain_image_count);
    vkGetSwapchainImagesKHR(self->device, swapchain, &swapchain_image_count, swapchain_image_array);

    uint32_t idx = self->presenter.surface_count++;

    presenter_resize(&self->presenter);

    self->presenter.surface_array[idx] = surface;
    self->presenter.swapchain_array[idx] = swapchain;
    self->presenter.semaphore_array[idx] = semaphore;
    self->presenter.image_source_array[idx] = args.image->image;
    self->presenter.image_count_array[idx] = swapchain_image_count;
    self->presenter.image_array[idx] = swapchain_image_array;
    self->presenter.wait_stage_array[idx] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    self->presenter.result_array[idx] = VK_SUCCESS;
    self->presenter.index_array[idx] = 0;

    self->presenter.image_copy_array[idx] = {
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        args.image->extent,
    };

    Py_RETURN_NONE;
}

Renderer * Instance_meth_renderer(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "size",
        "format",
        "samples",
        "levels",
        "layers",
        "depth",
        "uniform_buffer",
        "mode",
        "memory",
        NULL
    };

    struct {
        uint32_t width;
        uint32_t height;
        PyObject * format = default_format;
        uint32_t samples = 4;
        uint32_t levels = 1;
        uint32_t layers = 1;
        VkBool32 depth = true;
        VkBool32 uniform_buffer = true;
        PyObject * mode = output_str;
        PyObject * memory = Py_None;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "(II)|O!$IIIppOO",
        keywords,
        &args.width,
        &args.height,
        &PyUnicode_Type,
        &args.format,
        &args.samples,
        &args.levels,
        &args.layers,
        &args.depth,
        &args.uniform_buffer,
        &args.mode,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    Memory * memory = get_memory(self, args.memory);
    PyObject * format_list = PyUnicode_Split(args.format, NULL, -1);
    uint32_t output_count = (uint32_t)PyList_Size(format_list);
    ImageMode image_mode = get_image_mode(args.mode);

    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (image_mode == IMG_TEXTURE) {
        image_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    Renderer * res = PyObject_New(Renderer, Renderer_type);

    res->instance = self;
    res->width = args.width;
    res->height = args.height;
    res->samples = args.samples;
    res->levels = args.levels;
    res->layers = args.layers;

    res->pipeline_list = PyList_New(0);

    res->uniform_buffer = NULL;
    if (args.uniform_buffer) {
        res->uniform_buffer = new_buffer({
            self,
            memory,
            64 * 1024,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        });
    }

    Image * depth_image;
    Image * color_image_array[64];
    Image * resolve_image_array[64];

    if (args.depth) {
        depth_image = new_image({
            self,
            memory,
            0,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            {args.width, args.height, 1},
            args.samples,
            1,
            args.layers,
            IMG_PROTECTED,
            self->depth_format,
        });
    }

    if (args.samples) {
        for (uint32_t i = 0; i < output_count; ++i) {
            Format format = get_format(PyList_GetItem(format_list, i));
            color_image_array[i] = new_image({
                self,
                memory,
                0,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                {args.width, args.height, 1},
                args.samples,
                1,
                args.layers,
                IMG_PROTECTED,
                format.format,
            });
        }
    }

    for (uint32_t i = 0; i < output_count; ++i) {
        Format format = get_format(PyList_GetItem(format_list, i));
        resolve_image_array[i] = new_image({
            self,
            memory,
            args.width * args.height * args.layers * format.size,
            image_usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            {args.width, args.height, 1},
            VK_SAMPLE_COUNT_1_BIT,
            args.levels,
            args.layers,
            image_mode,
            format.format,
        });
    }

    allocate_memory(memory);

    if (args.uniform_buffer) {
        bind_buffer(res->uniform_buffer);
    }

    if (args.depth) {
        bind_image(depth_image);
    }

    if (args.samples) {
        for (uint32_t i = 0; i < output_count; ++i) {
            bind_image(color_image_array[i]);
        }
    }

    for (uint32_t i = 0; i < output_count; ++i) {
        bind_image(resolve_image_array[i]);
    }

    VkImageLayout final_image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if (image_mode == IMG_TEXTURE && args.levels == 1) {
        final_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    uint32_t description_count = 0;
    VkAttachmentDescription description_array[130];

    if (args.depth) {
        description_array[description_count++] = {
            0,
            depth_image->format,
            (VkSampleCountFlagBits)depth_image->samples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    if (args.format) {
        for (uint32_t i = 0; i < output_count; ++i) {
            description_array[description_count++] = {
                0,
                color_image_array[i]->format,
                (VkSampleCountFlagBits)color_image_array[i]->samples,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };
        }
    }

    for (uint32_t i = 0; i < output_count; ++i) {
            description_array[description_count++] = {
            0,
            resolve_image_array[i]->format,
            (VkSampleCountFlagBits)resolve_image_array[i]->samples,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            final_image_layout,
        };
    }

    uint32_t reference_count = 0;
    VkAttachmentReference depth_reference;
    VkAttachmentReference color_reference_array[64];
    VkAttachmentReference resolve_reference_array[64];

    if (args.depth) {
        depth_reference = {
            reference_count++,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    if (args.format) {
        for (uint32_t i = 0; i < output_count; ++i) {
            color_reference_array[i] = {
                reference_count++,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };
        }
    }

    for (uint32_t i = 0; i < output_count; ++i) {
        resolve_reference_array[i] = {
            reference_count++,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
    }

    VkSubpassDescription subpass_description = {
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0,
        NULL,
        output_count,
        color_reference_array,
        args.samples ? resolve_reference_array : NULL,
        args.depth ? &depth_reference : NULL,
        0,
        NULL,
    };

    VkSubpassDependency subpass_dependency = {
        VK_SUBPASS_EXTERNAL,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0,
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        NULL,
        0,
        description_count,
        description_array,
        1,
        &subpass_description,
        1,
        &subpass_dependency,
    };

    vkCreateRenderPass(res->instance->device, &render_pass_create_info, NULL, &res->render_pass);

    VkFramebuffer framebuffer_array[64];

    for (uint32_t k = 0; k < args.layers; ++k) {
        uint32_t image_view_count = 0;
        VkImageView image_view_array[130];

        if (args.depth) {
            VkImageViewCreateInfo image_view_create_info = {
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                NULL,
                0,
                depth_image->image,
                VK_IMAGE_VIEW_TYPE_2D,
                self->depth_format,
                {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
                {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, k, 1},
            };

            VkImageView image_view = NULL;
            vkCreateImageView(res->instance->device, &image_view_create_info, NULL, &image_view);
            image_view_array[image_view_count++] = image_view;
        }

        if (args.samples) {
            for (uint32_t i = 0; i < output_count; ++i) {
                VkImageViewCreateInfo image_view_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    NULL,
                    0,
                    color_image_array[i]->image,
                    VK_IMAGE_VIEW_TYPE_2D,
                    color_image_array[i]->format,
                    {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, k, 1},
                };

                VkImageView image_view = NULL;
                vkCreateImageView(res->instance->device, &image_view_create_info, NULL, &image_view);
                image_view_array[image_view_count++] = image_view;
            }
        }

        for (uint32_t i = 0; i < output_count; ++i) {
            VkImageViewCreateInfo image_view_create_info = {
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                NULL,
                0,
                resolve_image_array[i]->image,
                VK_IMAGE_VIEW_TYPE_2D,
                resolve_image_array[i]->format,
                {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, k, 1},
            };

            VkImageView image_view = NULL;
            vkCreateImageView(res->instance->device, &image_view_create_info, NULL, &image_view);
            image_view_array[image_view_count++] = image_view;
        }

        VkFramebufferCreateInfo framebuffer_create_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            NULL,
            0,
            res->render_pass,
            image_view_count,
            image_view_array,
            args.width,
            args.height,
            1,
        };

        vkCreateFramebuffer(res->instance->device, &framebuffer_create_info, NULL, &framebuffer_array[k]);
    }

    res->framebuffers = preserve_array(args.layers, framebuffer_array);
    res->resolve_images = preserve_array(output_count, resolve_image_array);

    uint32_t clear_value_count = output_count + args.depth;
    VkClearValue clear_value_array[65] = {};
    if (args.depth) {
        clear_value_array[0] = {1.0f, 0};
    }

    res->clear_values = preserve_array(clear_value_count, clear_value_array);

    res->output = PyTuple_New(output_count);
    for (uint32_t i = 0; i < output_count; ++i) {
        PyTuple_SetItem(res->output, i, (PyObject *)resolve_image_array[i]);
        Py_INCREF(resolve_image_array[i]);
    }

    PyList_Append(self->renderer_list, (PyObject *)res);
    return res;
}

Pipeline * Renderer_meth_pipeline(Renderer * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_shader",
        "fragment_shader",
        "vertex_format",
        "instance_format",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        "front_face",
        "cull_mode",
        "depth_test",
        "depth_write",
        "blending",
        "primitive_restart",
        "topology",
        "samplers",
        "memory",
        NULL,
    };

    struct {
        PyObject * vertex_shader = Py_None;
        PyObject * fragment_shader = Py_None;
        PyObject * vertex_format = empty_str;
        PyObject * instance_format = empty_str;
        uint32_t vertex_count = 0;
        uint32_t instance_count = 1;
        uint32_t index_count = 0;
        uint32_t indirect_count = 0;
        PyObject * front_face = default_front_face;
        PyObject * cull_mode = Py_None;
        VkBool32 depth_test = true;
        VkBool32 depth_write = true;
        VkBool32 blending = false;
        VkBool32 primitive_restart = false;
        PyObject * topology = default_topology;
        PyObject * samplers = Py_None;
        PyObject * memory = Py_None;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$O!O!OOIIIIOOppppOOO",
        keywords,
        &PyBytes_Type,
        &args.vertex_shader,
        &PyBytes_Type,
        &args.fragment_shader,
        &args.vertex_format,
        &args.instance_format,
        &args.vertex_count,
        &args.instance_count,
        &args.index_count,
        &args.indirect_count,
        &args.front_face,
        &args.cull_mode,
        &args.depth_test,
        &args.depth_write,
        &args.blending,
        &args.primitive_restart,
        &args.topology,
        &args.samplers,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.samplers == Py_None) {
        args.samplers = empty_list;
    }

    Memory * memory = get_memory(self->instance, args.memory);

    Pipeline * res = PyObject_New(Pipeline, Pipeline_type);

    res->instance = self->instance;
    res->vertex_count = args.vertex_count;
    res->instance_count = args.instance_count;
    res->index_count = args.index_count;
    res->indirect_count = args.indirect_count;

    PyObject * vertex_format = PyUnicode_Split(args.vertex_format, NULL, -1);
    PyObject * instance_format = PyUnicode_Split(args.instance_format, NULL, -1);

    uint32_t attribute_count = 0;
    VkVertexInputAttributeDescription attribute_array[64];
    VkVertexInputBindingDescription binding_array[64];

    uint32_t vstride = 0;
    uint32_t istride = 0;

    for (uint32_t i = 0; i < PyList_Size(vertex_format); ++i) {
        Format format = get_format(PyList_GetItem(vertex_format, i));
        if (format.format) {
            uint32_t location = attribute_count++;
            attribute_array[location] = {location, location, format.format, vstride};
        }
        vstride += format.size;
    }

    uint32_t vertex_attribute_count = attribute_count;

    for (uint32_t i = 0; i < PyList_Size(instance_format); ++i) {
        Format format = get_format(PyList_GetItem(instance_format, i));
        if (format.format) {
            uint32_t location = attribute_count++;
            attribute_array[location] = {location, location, format.format, istride};
        }
        istride += format.size;
    }

    for (uint32_t i = 0; i < vertex_attribute_count; ++i) {
        binding_array[i] = {i, vstride, VK_VERTEX_INPUT_RATE_VERTEX};
    }

    for (uint32_t i = vertex_attribute_count; i < attribute_count; ++i) {
        binding_array[i] = {i, istride, VK_VERTEX_INPUT_RATE_INSTANCE};
    }

    VkBool32 short_index = false;
    uint32_t index_size = short_index ? 2 : 4;
    uint32_t indirect_size = args.index_count ? 20 : 16;

    res->vertex_buffer = NULL;
    res->instance_buffer = NULL;
    res->index_buffer = NULL;
    res->indirect_buffer = NULL;

    if (vstride * args.vertex_count) {
        res->vertex_buffer = new_buffer({
            self->instance,
            memory,
            vstride * args.vertex_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        });
    }

    if (istride * args.instance_count) {
        res->instance_buffer = new_buffer({
            self->instance,
            memory,
            istride * args.instance_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        });
    }

    if (args.index_count) {
        res->index_buffer = new_buffer({
            self->instance,
            memory,
            args.index_count * index_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        });
    }

    if (args.indirect_count) {
        res->indirect_buffer = new_buffer({
            self->instance,
            memory,
            args.indirect_count * indirect_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        });
    }

    allocate_memory(memory);

    if (res->vertex_buffer) {
        bind_buffer(res->vertex_buffer);
    }
    if (res->instance_buffer) {
        bind_buffer(res->instance_buffer);
    }
    if (res->index_buffer) {
        bind_buffer(res->index_buffer);
    }
    if (res->indirect_buffer) {
        bind_buffer(res->indirect_buffer);
    }

    VkBuffer vertex_buffer_array[64];

    for (uint32_t i = 0; i < vertex_attribute_count; ++i) {
        vertex_buffer_array[i] = res->vertex_buffer->buffer;
    }

    for (uint32_t i = vertex_attribute_count; i < attribute_count; ++i) {
        vertex_buffer_array[i] = res->instance_buffer->buffer;
    }

    res->vertex_buffers = preserve_array(attribute_count, vertex_buffer_array);

    uint32_t sampler_count = (uint32_t)PyList_Size(args.samplers);
    VkSampler sampler_array[64];
    VkImageView image_view_array[64];
    VkDescriptorImageInfo sampler_binding_array[64];

    for (uint32_t i = 0; i < sampler_count; ++i) {
        Sampler * obj = (Sampler *)PyList_GetItem(args.samplers, i);

        VkImageViewCreateInfo image_view_create_info = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            obj->image->image,
            VK_IMAGE_VIEW_TYPE_2D,
            obj->image->format,
            obj->component_mapping,
            obj->subresource_range,
        };

        VkImageView image_view = NULL;
        vkCreateImageView(self->instance->device, &image_view_create_info, NULL, &image_view);

        VkSampler sampler = NULL;
        vkCreateSampler(self->instance->device, &obj->info, NULL, &sampler);

        sampler_array[i] = sampler;
        image_view_array[i] = image_view;
        sampler_binding_array[i] = {
            NULL,
            image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    uint32_t descriptor_buffer_binding = 0;
    uint32_t descriptor_sampler_binding = 0;
    uint32_t descriptor_binding_count = 0;
    VkDescriptorSetLayoutBinding descriptor_binding_array[2];
    VkDescriptorPoolSize descriptor_pool_size_array[2];

    if (self->uniform_buffer) {
        descriptor_buffer_binding = descriptor_binding_count++;
        descriptor_binding_array[descriptor_buffer_binding] = {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        };
        descriptor_pool_size_array[descriptor_buffer_binding] = {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
        };
    }

    if (sampler_count) {
        descriptor_sampler_binding = descriptor_binding_count++;
        descriptor_binding_array[descriptor_sampler_binding] = {
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            sampler_count,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            sampler_array,
        };
        descriptor_pool_size_array[descriptor_sampler_binding] = {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            sampler_count,
        };
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        NULL,
        0,
        descriptor_binding_count,
        descriptor_binding_array,
    };

    vkCreateDescriptorSetLayout(self->instance->device, &descriptor_set_layout_create_info, NULL, &res->descriptor_set_layout);

    VkPushConstantRange push_constant_range = {
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        4,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        NULL,
        0,
        1,
        &res->descriptor_set_layout,
        1,
        &push_constant_range,
    };

    vkCreatePipelineLayout(self->instance->device, &pipeline_layout_create_info, NULL, &res->pipeline_layout);

    res->descriptor_pool = NULL;
    res->descriptor_set = NULL;

    if (descriptor_binding_count) {
        VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            NULL,
            0,
            1,
            descriptor_binding_count,
            descriptor_pool_size_array,
        };

        vkCreateDescriptorPool(self->instance->device, &descriptor_pool_create_info, NULL, &res->descriptor_pool);

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            NULL,
            res->descriptor_pool,
            1,
            &res->descriptor_set_layout,
        };

        vkAllocateDescriptorSets(self->instance->device, &descriptor_set_allocate_info, &res->descriptor_set);
    }

    if (self->uniform_buffer) {
        VkDescriptorBufferInfo descriptor_buffer_info = {
            self->uniform_buffer->buffer,
            0,
            VK_WHOLE_SIZE,
        };

        VkWriteDescriptorSet write_buffer_descriptor_set = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            res->descriptor_set,
            descriptor_buffer_binding,
            0,
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            NULL,
            &descriptor_buffer_info,
            NULL,
        };

        vkUpdateDescriptorSets(self->instance->device, 1, &write_buffer_descriptor_set, 0, NULL);
    }

    if (sampler_count) {
        VkWriteDescriptorSet write_sampler_descriptor_set = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            res->descriptor_set,
            descriptor_sampler_binding,
            0,
            sampler_count,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            sampler_binding_array,
            NULL,
            NULL,
        };

        vkUpdateDescriptorSets(self->instance->device, 1, &write_sampler_descriptor_set, 0, NULL);
    }

    VkShaderModuleCreateInfo vertex_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.vertex_shader),
        (uint32_t *)PyBytes_AsString(args.vertex_shader),
    };

    vkCreateShaderModule(self->instance->device, &vertex_shader_module_create_info, NULL, &res->vertex_shader_module);

    VkShaderModuleCreateInfo fragment_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.fragment_shader),
        (uint32_t *)PyBytes_AsString(args.fragment_shader),
    };

    vkCreateShaderModule(self->instance->device, &fragment_shader_module_create_info, NULL, &res->fragment_shader_module);

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_array[] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            NULL,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            res->vertex_shader_module,
            "main",
            NULL,
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            NULL,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            res->fragment_shader_module,
            "main",
            NULL,
        },
    };

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        NULL,
        0,
        attribute_count,
        binding_array,
        attribute_count,
        attribute_array,
    };

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        NULL,
        0,
        get_topology(args.topology),
        args.primitive_restart,
    };

    VkPipelineViewportStateCreateInfo pipeline_viewport_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        NULL,
        0,
        1,
        NULL,
        1,
        NULL,
    };

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        NULL,
        0,
        false,
        false,
        VK_POLYGON_MODE_FILL,
        get_cull_mode(args.cull_mode),
        get_front_face(args.front_face),
        false,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        NULL,
        0,
        (VkSampleCountFlagBits)self->samples,
        false,
        0.0f,
        NULL,
        false,
        false,
    };

    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        NULL,
        0,
        args.depth_test,
        args.depth_write,
        VK_COMPARE_OP_LESS,
        false,
        false,
        {},
        {},
        0.0f,
        0.0f,
    };

    uint32_t color_attachment_count = (uint32_t)PyTuple_Size(self->output);
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_array[64];

    for (uint32_t i = 0; i < color_attachment_count; ++i) {
        pipeline_color_blend_attachment_array[i] = {
            args.blending,
            VK_BLEND_FACTOR_SRC_ALPHA,
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_FACTOR_ZERO,
            VK_BLEND_OP_ADD,
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
    }

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        NULL,
        0,
        false,
        VK_LOGIC_OP_CLEAR,
        color_attachment_count,
        pipeline_color_blend_attachment_array,
        {0.0f, 0.0f, 0.0f, 0.0f},
    };

    VkDynamicState dynamic_state_array[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        NULL,
        0,
        2,
        dynamic_state_array,
    };

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        NULL,
        0,
        2,
        pipeline_shader_stage_array,
        &pipeline_vertex_input_state,
        &pipeline_input_assembly_state,
        NULL,
        &pipeline_viewport_state,
        &pipeline_rasterization_state,
        &pipeline_multisample_state,
        &pipeline_depth_stencil_state,
        &pipeline_color_blend_state,
        &pipeline_dynamic_state,
        res->pipeline_layout,
        self->render_pass,
        0,
        NULL,
        0,
    };

    vkCreateGraphicsPipelines(self->instance->device, NULL, 1, &graphics_pipeline_create_info, NULL, &res->pipeline);

    PyList_Append(self->pipeline_list, (PyObject *)res);
    return res;
}

PyObject * Renderer_meth_update(Renderer * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"size", "uniform_buffer", "clear_values", NULL};

    struct {
        uint32_t width;
        uint32_t height;
        PyObject * uniform_buffer = Py_None;
        PyObject * clear_values = Py_None;
    } args;

    args.width = self->width;
    args.height = self->height;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$(II)OO",
        keywords,
        &args.width,
        &args.height,
        &args.uniform_buffer,
        &args.clear_values
    );

    if (!args_ok) {
        return NULL;
    }

    update_buffer(self->uniform_buffer, args.uniform_buffer);

    if (args.clear_values != Py_None) {
        Py_buffer view = {};
        PyObject_GetBuffer(args.clear_values, &view, PyBUF_STRIDED_RO);
        PyBuffer_ToContiguous(PyBytes_AsString(self->clear_values), &view, view.len, 'C');
        PyBuffer_Release(&view);
    }

    Py_RETURN_NONE;
}

PyObject * Renderer_meth_render(Renderer * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"size", NULL};

    struct {
        uint32_t width;
        uint32_t height;
    } args;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwargs, "|(II)", keywords, &args.width, &args.height)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyObject * Pipeline_meth_update(Pipeline * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_buffer",
        "instance_buffer",
        "index_buffer",
        "indirect_buffer",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        NULL,
    };

    struct {
        PyObject * vertex_buffer = Py_None;
        PyObject * instance_buffer = Py_None;
        PyObject * index_buffer = Py_None;
        PyObject * indirect_buffer = Py_None;
        uint32_t vertex_count;
        uint32_t instance_count;
        uint32_t index_count;
        uint32_t indirect_count;
    } args;

    args.vertex_count = self->vertex_count;
    args.instance_count = self->instance_count;
    args.index_count = self->index_count;
    args.indirect_count = self->indirect_count;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$OOOOIIII",
        keywords,
        &args.vertex_buffer,
        &args.instance_buffer,
        &args.index_buffer,
        &args.indirect_buffer,
        &args.vertex_count,
        &args.instance_count,
        &args.index_count,
        &args.indirect_count
    );

    if (!args_ok) {
        return NULL;
    }

    update_buffer(self->vertex_buffer, args.vertex_buffer);
    update_buffer(self->instance_buffer, args.instance_buffer);
    update_buffer(self->index_buffer, args.index_buffer);
    update_buffer(self->indirect_buffer, args.indirect_buffer);

    self->vertex_count = args.vertex_count;
    self->instance_count = args.instance_count;
    self->index_count = args.index_count;
    self->indirect_count = args.indirect_count;

    Py_RETURN_NONE;
}

PyObject * Image_meth_write(Image * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"data", NULL};

    struct {
        Py_buffer data = {};
    } args;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwargs, "y*", keywords, &args.data)) {
        return NULL;
    }

    resize_buffer(self->instance->host_buffer, (VkDeviceSize)args.data.len);

    PyBuffer_ToContiguous(self->instance->host_memory->ptr, &args.data, args.data.len, 'C');
    PyBuffer_Release(&args.data);

    VkCommandBuffer cmd = begin_commands(self->instance);

    VkImageLayout image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if (self->mode == IMG_TEXTURE) {
        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkImageMemoryBarrier image_barrier_transfer = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        0,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        self->image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, self->layers},
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &image_barrier_transfer
    );

    VkBufferImageCopy copy = {
        0,
        self->extent.width,
        self->extent.height,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, self->layers},
        {0, 0, 0},
        self->extent,
    };

    vkCmdCopyBufferToImage(
        cmd,
        self->instance->host_buffer->buffer,
        self->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy
    );

    if (self->samples == 1) {
        VkImageMemoryBarrier image_barrier_general = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            NULL,
            0,
            0,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            image_layout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            self->image,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, self->layers},
        };

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            1,
            &image_barrier_general
        );
    } else {
        build_mipmaps({
            cmd,
            self->extent.width,
            self->extent.height,
            self->levels,
            self->layers,
            1,
            self,
        });
    }

    end_commands(self->instance);
    Py_RETURN_NONE;
}

PyObject * Image_meth_read(Image * self) {
    resize_buffer(self->instance->host_buffer, self->size);

    VkCommandBuffer cmd = begin_commands(self->instance);

    VkBufferImageCopy copy = {
        0,
        self->extent.width,
        self->extent.height,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, self->layers},
        {0, 0, 0},
        self->extent,
    };

    vkCmdCopyImageToBuffer(
        cmd,
        self->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        self->instance->host_buffer->buffer,
        1,
        &copy
    );

    end_commands(self->instance);
    return PyBytes_FromStringAndSize((char *)self->instance->host_memory->ptr, self->size);
}

PyObject * Instance_meth_render(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "update",
        NULL,
    };

    struct {
        PyObject * update;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|O",
        keywords,
        &args.update
    );

    if (!args_ok) {
        return NULL;
    }

    // update draw count, instance count, ...

    // copy staging buffer -> vertex buffer, indirect buffer, ...
    // copy staging buffer -> uniform buffer
    // copy staging buffer -> image

    // render all

    // copy images -> staging buffer
    // copy images -> swapchain images

    for (uint32_t i = 0; i < self->presenter.surface_count; ++i) {
        vkAcquireNextImageKHR(
            self->device,
            self->presenter.swapchain_array[i],
            UINT64_MAX,
            self->presenter.semaphore_array[i],
            NULL,
            &self->presenter.index_array[i]
        );
    }

    VkCommandBuffer cmd = begin_commands(self);

    for (uint32_t i = 0; i < PyList_Size(self->renderer_list); ++i) {
        Renderer * renderer = (Renderer *)PyList_GetItem(self->renderer_list, i);
        render(cmd, renderer);
    }

    if (self->presenter.surface_count) {
        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[64];

        for (uint32_t i = 0; i < self->presenter.surface_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                self->presenter.image_array[i][self->presenter.index_array[i]],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
        }

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            image_barrier_count,
            image_barrier_array
        );
    }

    for (uint32_t i = 0; i < self->presenter.surface_count; ++i) {
        vkCmdCopyImage(
            cmd,
            self->presenter.image_source_array[i],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            self->presenter.image_array[i][self->presenter.index_array[i]],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &self->presenter.image_copy_array[i]
        );
    }

    if (self->presenter.surface_count) {
        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[64];

        for (uint32_t i = 0; i < self->presenter.surface_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                self->presenter.image_array[i][self->presenter.index_array[i]],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
        }

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            image_barrier_count,
            image_barrier_array
        );
    }

    vkEndCommandBuffer(self->command_buffer);

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        self->presenter.surface_count,
        self->presenter.semaphore_array,
        self->presenter.wait_stage_array,
        1,
        &self->command_buffer,
        0,
        NULL,
    };

    vkQueueSubmit(self->queue, 1, &submit_info, self->fence);
    vkWaitForFences(self->device, 1, &self->fence, true, UINT64_MAX);
    vkResetFences(self->device, 1, &self->fence);

    if (self->presenter.surface_count) {
        VkPresentInfoKHR present_info = {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            NULL,
            0,
            NULL,
            self->presenter.surface_count,
            self->presenter.swapchain_array,
            self->presenter.index_array,
            self->presenter.result_array,
        };

        vkQueuePresentKHR(self->queue, &present_info);

        uint32_t idx = 0;
        while (idx < self->presenter.surface_count) {
            if (self->presenter.result_array[idx] == VK_ERROR_OUT_OF_DATE_KHR) {
                vkDestroySemaphore(self->device, self->presenter.semaphore_array[idx], NULL);
                vkDestroySwapchainKHR(self->device, self->presenter.swapchain_array[idx], NULL);
                vkDestroySurfaceKHR(self->instance, self->presenter.surface_array[idx], NULL);
                presenter_remove(&self->presenter, idx);
                continue;
            }
            idx += 1;
        }
    }

    Py_RETURN_NONE;
}

struct vec3 {
    double x, y, z;
};

vec3 operator - (const vec3 & a, const vec3 & b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3 normalize(const vec3 & a) {
    const double l = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    return {a.x / l, a.y / l, a.z / l};
}

vec3 cross(const vec3 & a, const vec3 & b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

double dot(const vec3 & a, const vec3 & b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

PyObject * glnext_meth_camera(PyObject * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"eye", "target", "up", "fov", "aspect", "near", "far", NULL};

    vec3 eye;
    vec3 target;
    vec3 up = {0.0, 0.0, 1.0};
    double fov = 75.0;
    double aspect = 1.0;
    double znear = 0.1;
    double zfar = 1000.0;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "(ddd)(ddd)|(ddd)dddd",
        keywords,
        &eye.x,
        &eye.y,
        &eye.z,
        &target.x,
        &target.y,
        &target.z,
        &up.x,
        &up.y,
        &up.z,
        &fov,
        &aspect,
        &znear,
        &zfar
    );

    if (!args_ok) {
        return NULL;
    }

    const double r1 = tan(fov * 0.01745329251994329576923690768489 / 2.0);
    const double r2 = r1 * aspect;
    const double r3 = (zfar + znear) / (zfar - znear);
    const double r4 = (2.0 * zfar * znear) / (zfar - znear);
    const vec3 f = normalize(target - eye);
    const vec3 s = normalize(cross(f, up));
    const vec3 u = cross(s, f);
    const vec3 t = {-dot(s, eye), -dot(u, eye), -dot(f, eye)};

    float res[] = {
        (float)(s.x / r2), (float)(u.x / r1), (float)(r3 * f.x), (float)f.x,
        (float)(s.y / r2), (float)(u.y / r1), (float)(r3 * f.y), (float)f.y,
        (float)(s.z / r2), (float)(u.z / r1), (float)(r3 * f.z), (float)f.z,
        (float)(t.x / r2), (float)(t.y / r1), (float)(r3 * t.z - r4), (float)t.z,
    };

    return PyBytes_FromStringAndSize((char *)res, 64);
}

PyObject * glnext_meth_pack(PyObject * self, PyObject ** args, Py_ssize_t nargs) {
    if (nargs != 1 && nargs != 2) {
        return NULL;
    }

    PyObject * seq = PySequence_Fast(args[nargs - 1], "");
    PyObject ** array = PySequence_Fast_ITEMS(seq);

    int row_size = 0;
    int format_count = 0;
    PackType format_array[256];

    if (nargs == 1) {
        format_array[format_count++] = PyLong_CheckExact(array[0]) ? PACK_INT : PACK_FLOAT;
        row_size = 4;
    } else {
        PyObject * vformat = PyUnicode_Split(args[0], NULL, -1);
        for (int k = 0; k < PyList_Size(vformat); ++k) {
            Format format = get_format(PyList_GetItem(vformat, k));
            row_size += format.size;
            for (int i = 0; i < format.pack_count; ++i) {
                format_array[format_count++] = format.pack_type;
            }
        }
        Py_DECREF(vformat);
    }

    int rows = (int)PySequence_Fast_GET_SIZE(seq) / format_count;

    PyObject * res = PyBytes_FromStringAndSize(NULL, rows * row_size);
    char * data = PyBytes_AsString(res);

    for (int i = 0; i < rows; ++i) {
        for (int k = 0; k < format_count; ++k) {
            switch (format_array[k]) {
                case PACK_FLOAT:
                    *(*(float **)&data)++ = (float)PyFloat_AsDouble(*array++);
                    break;
                case PACK_INT:
                    *(*(int **)&data)++ = PyLong_AsLong(*array++);
                    break;
                case PACK_BYTE:
                    *(*(uint8_t **)&data)++ = (uint8_t)PyLong_AsUnsignedLong(*array++);
                    break;
                case PACK_PAD:
                    *data++ = 0;
                    break;
            }
        }
    }

    return res;
}

void default_dealloc(PyObject * self) {
    Py_TYPE(self)->tp_free(self);
}

PyMemberDef Instance_members[] = {
    {"device_memory", T_OBJECT, offsetof(Instance, device_memory), READONLY, NULL},
    {"staging_buffer", T_OBJECT, offsetof(Instance, staging_memoryview), READONLY, NULL},
    {},
};

PyMethodDef Instance_methods[] = {
    {"surface", (PyCFunction)Instance_meth_surface, METH_VARARGS | METH_KEYWORDS, NULL},
    {"renderer", (PyCFunction)Instance_meth_renderer, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image", (PyCFunction)Instance_meth_image, METH_VARARGS | METH_KEYWORDS, NULL},
    {"sampler", (PyCFunction)Instance_meth_sampler, METH_VARARGS | METH_KEYWORDS, NULL},
    {"render", (PyCFunction)Instance_meth_render, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMemberDef Renderer_members[] = {
    {"output", T_OBJECT, offsetof(Renderer, output), READONLY, NULL},
    {},
};

PyMethodDef Renderer_methods[] = {
    {"pipeline", (PyCFunction)Renderer_meth_pipeline, METH_VARARGS | METH_KEYWORDS, NULL},
    {"update", (PyCFunction)Renderer_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {"render", (PyCFunction)Renderer_meth_render, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Pipeline_methods[] = {
    {"update", (PyCFunction)Pipeline_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Image_methods[] = {
    {"write", (PyCFunction)Image_meth_write, METH_VARARGS | METH_KEYWORDS, NULL},
    {"read", (PyCFunction)Image_meth_read, METH_NOARGS, NULL},
    {},
};

PyType_Slot Instance_slots[] = {
    {Py_tp_members, Instance_members},
    {Py_tp_methods, Instance_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Renderer_slots[] = {
    {Py_tp_members, Renderer_members},
    {Py_tp_methods, Renderer_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Pipeline_slots[] = {
    {Py_tp_methods, Pipeline_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Memory_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Buffer_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Image_slots[] = {
    {Py_tp_methods, Image_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Sampler_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Spec Instance_spec = {"glnext.Instance", sizeof(Instance), 0, Py_TPFLAGS_DEFAULT, Instance_slots};
PyType_Spec Renderer_spec = {"glnext.Renderer", sizeof(Renderer), 0, Py_TPFLAGS_DEFAULT, Renderer_slots};
PyType_Spec Pipeline_spec = {"glnext.Pipeline", sizeof(Pipeline), 0, Py_TPFLAGS_DEFAULT, Pipeline_slots};
PyType_Spec Memory_spec = {"glnext.Memory", sizeof(Memory), 0, Py_TPFLAGS_DEFAULT, Memory_slots};
PyType_Spec Buffer_spec = {"glnext.Buffer", sizeof(Buffer), 0, Py_TPFLAGS_DEFAULT, Buffer_slots};
PyType_Spec Image_spec = {"glnext.Image", sizeof(Image), 0, Py_TPFLAGS_DEFAULT, Image_slots};
PyType_Spec Sampler_spec = {"glnext.Sampler", sizeof(Sampler), 0, Py_TPFLAGS_DEFAULT, Sampler_slots};

PyMethodDef module_methods[] = {
    {"instance", (PyCFunction)glnext_meth_instance, METH_VARARGS | METH_KEYWORDS, NULL},
    {"camera", (PyCFunction)glnext_meth_camera, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pack", (PyCFunction)glnext_meth_pack, METH_FASTCALL, NULL},
    {},
};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "glnext", NULL, -1, module_methods};

extern "C" PyObject * PyInit_glnext() {
    PyObject * module = PyModule_Create(&module_def);

    Instance_type = (PyTypeObject *)PyType_FromSpec(&Instance_spec);
    Renderer_type = (PyTypeObject *)PyType_FromSpec(&Renderer_spec);
    Pipeline_type = (PyTypeObject *)PyType_FromSpec(&Pipeline_spec);
    Memory_type = (PyTypeObject *)PyType_FromSpec(&Memory_spec);
    Buffer_type = (PyTypeObject *)PyType_FromSpec(&Buffer_spec);
    Image_type = (PyTypeObject *)PyType_FromSpec(&Image_spec);
    Sampler_type = (PyTypeObject *)PyType_FromSpec(&Sampler_spec);

    empty_str = PyUnicode_FromString("");
    empty_list = PyList_New(0);

    default_topology = PyUnicode_FromString("triangles");
    default_front_face = PyUnicode_FromString("counter_clockwise");
    default_border_color = PyUnicode_FromString("transparent");
    default_format = PyUnicode_FromString("4b");
    default_filter = PyUnicode_FromString("nearest");
    default_address_mode = PyUnicode_FromString("repeat");
    default_swizzle = PyUnicode_FromString("rgba");
    texture_str = PyUnicode_FromString("texture");
    output_str = PyUnicode_FromString("output");

    return module;
}
