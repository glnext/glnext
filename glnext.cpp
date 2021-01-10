#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <vulkan/vulkan_core.h>

struct Context;
struct Memory;
struct Renderer;
struct Buffer;
struct Image;
struct Sampler;

enum PackType {
    PACK_FLOAT,
    PACK_INT,
    PACK_BYTE,
    PACK_PAD,
};

struct Instance {
    PyObject_HEAD

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkFence fence;

    uint32_t queue_family_index;
    uint32_t host_memory_type_index;
    uint32_t device_memory_type_index;

    Memory * device_memory;
    Memory * host_memory;
    Buffer * host_buffer;

    PyObject * context_list;
    PyObject * memory_list;
    PyObject * buffer_list;
    PyObject * image_list;
};

struct ColorAttachment {
    uint32_t size;
    PackType pack_type;
    uint32_t pack_count;
    Image * image;
    Image * resolve;
};

struct Context {
    PyObject_HEAD

    Instance * instance;

    uint32_t width;
    uint32_t height;
    VkSampleCountFlagBits samples;
    VkBool32 render_to_image;

    VkRenderPass render_pass;
    VkFramebuffer framebuffer;

    uint32_t attachment_count;
    VkImageView * attachment_image_view_array;

    uint32_t color_attachment_count;
    ColorAttachment * color_attachment_array;
    VkClearValue * clear_value_array;

    Image * depth_image;
    Buffer * uniform_buffer;

    PyObject * output;
    PyObject * renderer_list;
};

struct Memory {
    PyObject_HEAD

    Instance * instance;

    VkBool32 host;
    VkDeviceSize size;
    VkDeviceSize offset;
    VkDeviceMemory memory;
    void * ptr;
};

struct Renderer {
    PyObject_HEAD

    Context * ctx;
    Memory * memory;

    Buffer * vertex_buffer;
    Buffer * instance_buffer;
    Buffer * index_buffer;
    Buffer * indirect_buffer;

    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t index_count;
    uint32_t indirect_count;
    VkBool32 enabled;

    VkIndexType index_type;

    uint32_t vertex_buffer_count;
    VkBuffer * vertex_buffer_array;
    VkDeviceSize * vertex_offset_array;

    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    VkPipeline pipeline;

    uint32_t sampler_count;
    VkSampler * sampler_array;
    VkDescriptorImageInfo * sampler_binding_array;

    PyObject * image_list;
};

struct Buffer {
    PyObject_HEAD

    Instance * instance;
    Memory * memory;

    VkBufferUsageFlags usage;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkBuffer buffer;
};

struct Image {
    PyObject_HEAD

    Instance * instance;
    Memory * memory;

    VkDeviceSize offset;
    VkDeviceSize size;
    VkExtent3D extent;
    VkSampleCountFlagBits samples;
    VkFormat format;
    VkImage image;
};

struct Sampler {
    PyObject_HEAD

    Image * image;
    VkSamplerCreateInfo info;
    VkComponentMapping swizzle;
};

PyTypeObject * Instance_type;
PyTypeObject * Context_type;
PyTypeObject * Memory_type;
PyTypeObject * Renderer_type;
PyTypeObject * Buffer_type;
PyTypeObject * Image_type;
PyTypeObject * Sampler_type;

PyObject * default_framebuffer_format;
PyObject * default_border;
PyObject * empty_list;
PyObject * empty_str;
PyObject * sampler_address_mode_dict;
PyObject * sampler_filter_dict;
PyObject * mipmap_filter_dict;
PyObject * float_border_dict;
PyObject * int_border_dict;
PyObject * topology_dict;
PyObject * format_dict;

int convert_address_mode(PyObject * obj, VkSamplerAddressMode * value) {
    *value = (VkSamplerAddressMode)PyLong_AsUnsignedLong(PyDict_GetItem(sampler_address_mode_dict, obj));
    return 1;
}

int convert_filter(PyObject * obj, VkFilter * value) {
    *value = (VkFilter)PyLong_AsUnsignedLong(PyDict_GetItem(sampler_filter_dict, obj));
    return 1;
}

int convert_mipmap_filter(PyObject * obj, VkSamplerMipmapMode * value) {
    *value = (VkSamplerMipmapMode)PyLong_AsUnsignedLong(PyDict_GetItem(mipmap_filter_dict, obj));
    return 1;
}

void register_format(const char * name, VkFormat format, int size, PackType pack_type, int pack_count) {
    PyObject * item = Py_BuildValue("IIII", format, size, pack_type, pack_count);
    PyDict_SetItemString(format_dict, name, item);
    Py_DECREF(item);
}

int list_index(PyObject * lst, void * obj) {
    for (int i = 0; i < PyList_Size(lst); ++i) {
        if (PyList_GetItem(lst, i) == (PyObject *)obj) {
            return i;
        }
    }
    return -1;
}

VkCommandBuffer begin_commands(Instance * self) {
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        NULL,
    };

    vkBeginCommandBuffer(self->command_buffer, &command_buffer_begin_info);
    return self->command_buffer;
}

void end_commands(Instance * self) {
    vkEndCommandBuffer(self->command_buffer);

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        0,
        NULL,
        NULL,
        1,
        &self->command_buffer,
        0,
        NULL,
    };

    vkQueueSubmit(self->queue, 1, &submit_info, self->fence);
    vkWaitForFences(self->device, 1, &self->fence, true, UINT64_MAX);
    vkResetFences(self->device, 1, &self->fence);
}

Memory * new_memory(Instance * self, bool host) {
    Memory * res = PyObject_New(Memory, Memory_type);
    res->instance = self;
    res->host = host;
    res->memory = NULL;
    res->ptr = NULL;
    res->offset = 0;
    res->size = 0;
    PyList_Append(self->memory_list, (PyObject *)res);
    return res;
}

VkDeviceSize take_memory(Memory * self, VkMemoryRequirements requirements) {
    if (VkDeviceSize padding = self->offset % requirements.alignment) {
        self->offset += requirements.alignment - padding;
    }
    VkDeviceSize res = self->offset;
    self->offset += requirements.size;
    return res;
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
    if (self->memory) {
        if (self->host) {
            vkUnmapMemory(self->instance->device, self->memory);
        }
        vkFreeMemory(self->instance->device, self->memory, NULL);
        self->memory = NULL;
        self->ptr = NULL;
        self->offset = 0;
        self->size = 0;
    }
}

Buffer * new_buffer(Instance * self, VkDeviceSize size, VkBufferUsageFlags usage, Memory * memory) {
    Buffer * res = PyObject_New(Buffer, Buffer_type);
    res->instance = self;
    res->memory = memory;
    res->usage = usage;
    res->buffer = NULL;
    res->size = size;

    if (!size) {
        return res;
    }

    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        size,
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    vkCreateBuffer(self->device, &buffer_create_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(self->device, res->buffer, &requirements);
    res->offset = take_memory(memory, requirements);

    PyList_Append(self->buffer_list, (PyObject *)res);
    return res;
}

Image * new_image(Instance * self, VkDeviceSize size, VkExtent3D extent, VkFormat format, VkFlags usage, VkSampleCountFlagBits samples, Memory * memory) {
    Image * res = PyObject_New(Image, Image_type);
    res->instance = self;
    res->memory = memory;
    res->image = NULL;
    res->size = size;
    res->extent = extent;
    res->samples = samples;
    res->format = format;

    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        0,
        VK_IMAGE_TYPE_2D,
        format,
        extent,
        1,
        1,
        samples,
        VK_IMAGE_TILING_OPTIMAL,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    vkCreateImage(self->device, &image_create_info, NULL, &res->image);

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(self->device, res->image, &requirements);
    res->offset = take_memory(memory, requirements);

    PyList_Append(self->image_list, (PyObject *)res);
    return res;
}

void bind_buffer(Buffer * self) {
    if (self->buffer) {
        vkBindBufferMemory(self->instance->device, self->buffer, self->memory->memory, self->offset);
    }
}

void bind_image(Image * self) {
    if (self->image) {
        vkBindImageMemory(self->instance->device, self->image, self->memory->memory, self->offset);
    }
}

void resize_buffer(Buffer * self, VkDeviceSize size) {
    if (size <= self->size) {
        return;
    }
    self->size = size;

    if (self->buffer) {
        vkDestroyBuffer(self->instance->device, self->buffer, NULL);
    }
    free_memory(self->memory);

    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        size,
        self->usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

Instance * glnext_meth_instance(PyObject * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"host_memory", "device_memory", NULL};

    VkDeviceSize host_memory_size = 0;
    VkDeviceSize device_memory_size = 0;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|$kk",
        keywords,
        &host_memory_size,
        &device_memory_size
    );

    if (!args_ok) {
        return NULL;
    }

    if (host_memory_size < 64 * 1024) {
        host_memory_size = 64 * 1024;
    }

    Instance * res = PyObject_New(Instance, Instance_type);

    res->context_list = PyList_New(0);
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

    const char * instance_layer_array[] = {"VK_LAYER_KHRONOS_validation"};
    const char * instance_extension_array[] = {"VK_EXT_debug_utils"};

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        NULL,
        0,
        &application_info,
        1,
        instance_layer_array,
        1,
        instance_extension_array,
    };

    vkCreateInstance(&instance_create_info, NULL, &res->instance);

    uint32_t num_physical_devices = 1;
    vkEnumeratePhysicalDevices(res->instance, &num_physical_devices, &res->physical_device);

    VkPhysicalDeviceMemoryProperties device_memory_properties = {};
    vkGetPhysicalDeviceMemoryProperties(res->physical_device, &device_memory_properties);

    VkPhysicalDeviceFeatures supported_features = {};
    vkGetPhysicalDeviceFeatures(res->physical_device, &supported_features);

    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->host_memory_type_index = i;
            break;
        }
    }

    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->device_memory_type_index = i;
            break;
        }
    }

    res->queue_family_index = 0;
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

    VkDeviceCreateInfo device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        NULL,
        0,
        1,
        &device_queue_create_info,
        0,
        NULL,
        0,
        NULL,
        &physical_device_features,
    };

    vkCreateDevice(res->physical_device, &device_create_info, NULL, &res->device);
    vkGetDeviceQueue(res->device, res->queue_family_index, 0, &res->queue);

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

    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0};
    vkCreateFence(res->device, &fence_create_info, NULL, &res->fence);

    res->host_memory = new_memory(res, true);
    res->host_buffer = new_buffer(res, host_memory_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, res->host_memory);
    allocate_memory(res->host_memory);
    bind_buffer(res->host_buffer);

    res->device_memory = NULL;

    if (device_memory_size) {
        res->device_memory = new_memory(res, false);
        allocate_memory(res->device_memory, device_memory_size);
    }

    return res;
}

Context * Instance_meth_context(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", "format", "samples", "depth", "render_to_image", "memory", NULL};

    uint32_t width;
    uint32_t height;
    PyObject * framebuffer_format = default_framebuffer_format;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_4_BIT;
    VkBool32 has_depth = true;
    VkBool32 render_to_image = false;
    Memory * memory = NULL;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "(II)|OIkppO!",
        keywords,
        &width,
        &height,
        &framebuffer_format,
        &samples,
        &has_depth,
        &render_to_image,
        Memory_type,
        &memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (!memory) {
        memory = new_memory(self, false);
    }

    Context * res = PyObject_New(Context, Context_type);

    res->instance = self;
    res->render_to_image = render_to_image;
    res->renderer_list = PyList_New(0);

    res->uniform_buffer = new_buffer(self, 64 * 1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memory);

    VkBool32 has_color = true;
    VkBool32 has_resolve = has_color && samples != VK_SAMPLE_COUNT_1_BIT;
    VkFormat depth_format = VK_FORMAT_D24_UNORM_S8_UINT;

    VkImageUsageFlags output_image_usage = (render_to_image ? VK_IMAGE_USAGE_SAMPLED_BIT : VK_IMAGE_USAGE_TRANSFER_SRC_BIT) | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkImageLayout final_layout = render_to_image ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkExtent3D image_extent = {width, height, 1};
    VkExtent3D zero_extent = {};

    uint32_t pixel_size = 0;
    PyObject * format_list = PyUnicode_Split(framebuffer_format, NULL, -1);
    if (!format_list) {
        return NULL;
    }

    res->color_attachment_count = (uint32_t)PyList_Size(format_list);
    res->color_attachment_array = (ColorAttachment *)PyMem_Malloc(res->color_attachment_count * sizeof(ColorAttachment));

    for (uint32_t i = 0; i < res->color_attachment_count; ++i) {
        PyObject * obj = PyDict_GetItem(format_dict, PyList_GetItem(format_list, i));
        VkFormat format = (VkFormat)PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 0));
        uint32_t size = PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 1));

        res->color_attachment_array[i].size = size;
        res->color_attachment_array[i].pack_type = (PackType)PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 2));
        res->color_attachment_array[i].pack_count = PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 3));

        Image * color_image = new_image(
            self,
            width * height * samples * size,
            {width, height, 1},
            format,
            has_resolve ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : output_image_usage,
            samples,
            memory
        );

        res->color_attachment_array[i].image = color_image;
        res->color_attachment_array[i].resolve = color_image;

        if (has_resolve) {
            res->color_attachment_array[i].resolve = new_image(
                self,
                width * height * size,
                {width, height, has_resolve},
                format,
                output_image_usage,
                VK_SAMPLE_COUNT_1_BIT,
                memory
            );
        }

        pixel_size += size;
    }

    Py_DECREF(format_list);

    res->depth_image = new_image(
        self,
        width * height * samples * 4,
        {width, height, has_depth},
        depth_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        samples,
        memory
    );

    allocate_memory(memory);

    bind_buffer(res->uniform_buffer);

    for (uint32_t i = 0; i < res->color_attachment_count; ++i) {
        bind_image(res->color_attachment_array[i].image);
        if (has_resolve) {
            bind_image(res->color_attachment_array[i].resolve);
        }
    }

    bind_image(res->depth_image);

    VkAttachmentReference depth_attachment_reference;
    VkAttachmentReference color_attachment_reference_array[64];
    VkAttachmentReference resolve_attachment_reference_array[64];
    VkAttachmentDescription attachment_description_array[130];

    res->attachment_count = 0;
    uint32_t max_attachments = has_depth + (has_resolve + 1) * res->color_attachment_count;
    res->attachment_image_view_array = (VkImageView *)PyMem_Malloc(max_attachments * sizeof(VkImageView));
    res->clear_value_array = (VkClearValue *)PyMem_Malloc(max_attachments * sizeof(VkClearValue));
    memset(res->clear_value_array, 0, max_attachments * sizeof(VkClearValue));
    res->clear_value_array[0] = {1.0f, 0};

    if (has_depth) {
        VkImageViewCreateInfo depth_image_view_create_info = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            res->depth_image->image,
            VK_IMAGE_VIEW_TYPE_2D,
            depth_format,
            {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
            {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
        };

        VkImageView image_view = NULL;
        vkCreateImageView(res->instance->device, &depth_image_view_create_info, NULL, &image_view);

        res->attachment_image_view_array[res->attachment_count] = image_view;
        attachment_description_array[res->attachment_count] = {
            0,
            res->depth_image->format,
            res->depth_image->samples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        depth_attachment_reference = {
            res->attachment_count,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        res->attachment_count += 1;
    }

    for (uint32_t i = 0; i < res->color_attachment_count; ++i) {
        VkImageViewCreateInfo color_image_view_create_info = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            res->color_attachment_array[i].image->image,
            VK_IMAGE_VIEW_TYPE_2D,
            res->color_attachment_array[i].image->format,
            {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };

        VkImageView image_view = NULL;
        vkCreateImageView(res->instance->device, &color_image_view_create_info, NULL, &image_view);

        res->attachment_image_view_array[res->attachment_count] = image_view;
        attachment_description_array[res->attachment_count] = {
            0,
            res->color_attachment_array[i].image->format,
            res->color_attachment_array[i].image->samples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            has_resolve ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : final_layout,
        };
        color_attachment_reference_array[i] = {
            res->attachment_count,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        res->attachment_count += 1;

        if (has_resolve) {
            VkImageViewCreateInfo color_image_resolve_view_create_info = {
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                NULL,
                0,
                res->color_attachment_array[i].resolve->image,
                VK_IMAGE_VIEW_TYPE_2D,
                res->color_attachment_array[i].resolve->format,
                {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };

            VkImageView image_view = NULL;
            vkCreateImageView(res->instance->device, &color_image_resolve_view_create_info, NULL, &image_view);

            res->attachment_image_view_array[res->attachment_count] = image_view;
            attachment_description_array[res->attachment_count] = {
                0,
                res->color_attachment_array[i].resolve->format,
                res->color_attachment_array[i].resolve->samples,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                final_layout,
            };
            resolve_attachment_reference_array[i] = {
                res->attachment_count,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };

            res->attachment_count += 1;
        }
    }

    VkSubpassDescription subpass_description = {
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0,
        NULL,
        res->color_attachment_count,
        has_color ? color_attachment_reference_array : NULL,
        has_resolve ? resolve_attachment_reference_array : NULL,
        has_depth ? &depth_attachment_reference : NULL,
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
        res->attachment_count,
        attachment_description_array,
        1,
        &subpass_description,
        1,
        &subpass_dependency,
    };

    vkCreateRenderPass(res->instance->device, &render_pass_create_info, NULL, &res->render_pass);

    VkFramebufferCreateInfo framebuffer_create_info = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        NULL,
        0,
        res->render_pass,
        res->attachment_count,
        res->attachment_image_view_array,
        width,
        height,
        1,
    };

    vkCreateFramebuffer(res->instance->device, &framebuffer_create_info, NULL, &res->framebuffer);

    res->output = PyTuple_New(res->color_attachment_count);
    for (uint32_t i = 0; i < res->color_attachment_count; ++i) {
        PyObject * obj = (PyObject *)res->color_attachment_array[0].resolve;
        PyTuple_SetItem(res->output, 0, obj);
        Py_INCREF(obj);
    }

    res->width = width;
    res->height = height;
    res->samples = samples;

    if (!render_to_image) {
        resize_buffer(self->host_buffer, width * height * pixel_size);
    }

    PyList_Append(self->context_list, (PyObject *)res);
    return res;
}

Memory * Instance_meth_memory(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", NULL};

    VkDeviceSize size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k", keywords, &size)) {
        return NULL;
    }

    if (!self->instance) {
        return NULL;
    }

    Memory * res = new_memory(self, false);
    allocate_memory(res, size);
    return res;
}

Image * Instance_meth_image(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", "format", "memory", NULL};

    uint32_t width;
    uint32_t height;
    const char * format_str = "4b";
    Memory * memory = NULL;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "(II)|sO!",
        keywords,
        &width,
        &height,
        &format_str,
        Memory_type,
        &memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (!self->instance) {
        return NULL;
    }

    if (!memory) {
        memory = new_memory(self, false);
    }

    PyObject * format_obj = PyDict_GetItemString(format_dict, format_str);
    if (!format_obj) {
        return NULL;
    }
    VkFormat pixel_format = (VkFormat)PyLong_AsUnsignedLong(PyTuple_GetItem(format_obj, 0));
    uint32_t pixel_size = PyLong_AsUnsignedLong(PyTuple_GetItem(format_obj, 1));

    Image * res = new_image(
        self,
        width * height * pixel_size,
        {width, height, 1},
        pixel_format,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_SAMPLE_COUNT_1_BIT,
        memory
    );

    if (!res) {
        return NULL;
    }

    allocate_memory(memory);

    if (memory->offset > memory->size) {
        return NULL;
    }

    bind_image(res);

    VkCommandBuffer cmd = begin_commands(self);

    VkImageMemoryBarrier image_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        0,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        res->image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
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
        &image_barrier
    );

    end_commands(self);
    return res;
}

Sampler * Instance_meth_sampler(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {
        "image",
        "address_mode_x",
        "address_mode_y",
        "address_mode_z",
        "min_filter",
        "mag_filter",
        "mipmap_filter",
        "normalized",
        "anisotropy",
        "max_anisotrpoy",
        "min_lod",
        "max_lod",
        "lod_bias",
        "border",
        NULL,
    };

    Image * image;
    VkSamplerAddressMode address_mode_x = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode address_mode_y = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode address_mode_z = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkFilter min_filter = VK_FILTER_NEAREST;
    VkFilter mag_filter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode mipmap_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkBool32 normalized = true;
    VkBool32 anisotropy = false;
    float max_anisotrpoy = 0.0f;
    float min_lod = 0.0f;
    float max_lod = 1000.0f;
    float lod_bias = 0.0f;
    PyObject * border = default_border;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "O!|O&O&O&O&O&O&ppffffO",
        keywords,
        Image_type,
        &image,
        convert_address_mode,
        &address_mode_x,
        convert_address_mode,
        &address_mode_y,
        convert_address_mode,
        &address_mode_z,
        convert_filter,
        &min_filter,
        convert_filter,
        &mag_filter,
        convert_mipmap_filter,
        &mipmap_filter,
        &normalized,
        &anisotropy,
        &max_anisotrpoy,
        &min_lod,
        &max_lod,
        &lod_bias,
        &border
    );

    if (!args_ok) {
        return NULL;
    }

    if (!self->instance) {
        return NULL;
    }

    Sampler * res = PyObject_New(Sampler, Sampler_type);

    Py_INCREF(image);
    res->image = image;

    VkBorderColor border_color = (VkBorderColor)PyLong_AsUnsignedLong(PyDict_GetItem(float_border_dict, border));

    res->info = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        NULL,
        0,
        min_filter,
        mag_filter,
        mipmap_filter,
        address_mode_x,
        address_mode_y,
        address_mode_z,
        lod_bias,
        anisotropy,
        max_anisotrpoy,
        false,
        VK_COMPARE_OP_NEVER,
        min_lod,
        max_lod,
        border_color,
        !normalized,
    };

    res->swizzle = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

    return res;
}

Renderer * Context_meth_renderer(Context * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_shader",
        "fragment_shader",
        "vertex_format",
        "instance_format",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        "topology",
        "depth",
        "blend",
        "short_index",
        "samplers",
        "memory",
        NULL,
    };

    PyObject * vertex_shader = Py_None;
    PyObject * fragment_shader = Py_None;
    PyObject * vertex_format = empty_str;
    PyObject * instance_format = empty_str;
    uint32_t vertex_count = 0;
    uint32_t instance_count = 1;
    uint32_t index_count = 0;
    uint32_t indirect_count = 0;
    const char * topology_str = "triangles";
    VkBool32 depth = false;
    VkBool32 blend = false;
    VkBool32 short_index = false;
    PyObject * samplers = empty_list;
    Memory * memory = NULL;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|$O!O!OOIIIIspppOO!",
        keywords,
        &PyBytes_Type,
        &vertex_shader,
        &PyBytes_Type,
        &fragment_shader,
        &vertex_format,
        &instance_format,
        &vertex_count,
        &instance_count,
        &index_count,
        &indirect_count,
        &topology_str,
        &depth,
        &blend,
        &short_index,
        &samplers,
        Memory_type,
        &memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (!self->instance) {
        return NULL;
    }
    if (!memory) {
        memory = new_memory(self->instance, false);
    }

    PyObject * topology_int = PyDict_GetItemString(topology_dict, topology_str);
    VkPrimitiveTopology topology = (VkPrimitiveTopology)PyLong_AsUnsignedLong(topology_int);

    uint32_t bindings = 0;
    VkVertexInputBindingDescription binding_array[64];
    VkVertexInputAttributeDescription attribute_array[64];

    uint32_t vstride = 0;
    PyObject * vertex_format_list = PyUnicode_Split(vertex_format, NULL, -1);
    if (!vertex_format_list) {
        return NULL;
    }
    for (uint32_t i = 0; i < PyList_Size(vertex_format_list); ++i) {
        PyObject * obj = PyDict_GetItem(format_dict, PyList_GetItem(vertex_format_list, i));
        VkFormat format = (VkFormat)PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 0));
        uint32_t size = PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 1));
        if (format) {
            attribute_array[bindings] = {bindings, bindings, format, vstride};
            bindings += 1;
        }
        vstride += size;
    }
    Py_DECREF(vertex_format_list);

    uint32_t first_intance_binding = bindings;

    uint32_t istride = 0;
    PyObject * instance_format_list = PyUnicode_Split(instance_format, NULL, -1);
    if (!instance_format_list) {
        return NULL;
    }
    for (uint32_t i = 0; i < PyList_Size(instance_format_list); ++i) {
        PyObject * obj = PyDict_GetItem(format_dict, PyList_GetItem(instance_format_list, i));
        VkFormat format = (VkFormat)PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 0));
        uint32_t size = PyLong_AsUnsignedLong(PyTuple_GetItem(obj, 1));
        if (format) {
            attribute_array[bindings] = {bindings, bindings, format, istride};
            bindings += 1;
        }
        istride += size;
    }
    Py_DECREF(instance_format_list);

    for (uint32_t i = 0; i < first_intance_binding; ++i) {
        binding_array[i] = {i, vstride, VK_VERTEX_INPUT_RATE_VERTEX};
    }

    for (uint32_t i = first_intance_binding; i < bindings; ++i) {
        binding_array[i] = {i, istride, VK_VERTEX_INPUT_RATE_INSTANCE};
    }

    Renderer * res = PyObject_New(Renderer, Renderer_type);

    res->ctx = self;
    res->memory = memory;

    uint32_t index_size = short_index ? 2 : 4;
    uint32_t indirect_size = index_count ? 20 : 16;
    res->index_type = short_index ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

    res->vertex_buffer = new_buffer(
        self->instance,
        vstride * vertex_count,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        memory
    );

    res->instance_buffer = new_buffer(
        self->instance,
        istride * instance_count,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        memory
    );

    res->index_buffer = new_buffer(
        self->instance,
        index_count * index_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        memory
    );

    res->indirect_buffer = new_buffer(
        self->instance,
        indirect_count * indirect_size,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        memory
    );

    allocate_memory(memory);

    if (memory->offset > memory->size) {
        return NULL;
    }

    bind_buffer(res->vertex_buffer);
    bind_buffer(res->instance_buffer);
    bind_buffer(res->index_buffer);
    bind_buffer(res->indirect_buffer);

    res->sampler_count = (uint32_t)PyList_Size(samplers);
    res->sampler_array = (VkSampler *)PyMem_Malloc(res->sampler_count * sizeof(VkSampler));
    res->sampler_binding_array = (VkDescriptorImageInfo *)PyMem_Malloc(res->sampler_count * sizeof(VkDescriptorImageInfo));

    res->image_list = PyList_New(res->sampler_count);

    for (uint32_t i = 0; i < res->sampler_count; ++i) {
        Sampler * obj = (Sampler *)PyList_GetItem(samplers, i);

        Py_INCREF(obj->image);
        PyList_SetItem(res->image_list, i, (PyObject *)obj->image);

        VkImageViewCreateInfo image_view_create_info = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            obj->image->image,
            VK_IMAGE_VIEW_TYPE_2D,
            obj->image->format,
            obj->swizzle,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };

        VkImageView image_view = NULL;
        vkCreateImageView(self->instance->device, &image_view_create_info, NULL, &image_view);

        VkSampler sampler = NULL;
        vkCreateSampler(self->instance->device, &obj->info, NULL, &sampler);

        res->sampler_array[i] = sampler;

        res->sampler_binding_array[i] = {
            NULL,
            image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding_array[] = {
        {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        },
        {
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            res->sampler_count,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            res->sampler_array,
        },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        NULL,
        0,
        (uint32_t)(res->sampler_count ? 2 : 1),
        descriptor_set_layout_binding_array,
    };

    vkCreateDescriptorSetLayout(self->instance->device, &descriptor_set_layout_create_info, NULL, &res->descriptor_set_layout);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        NULL,
        0,
        1,
        &res->descriptor_set_layout,
        0,
        NULL,
    };

    vkCreatePipelineLayout(self->instance->device, &pipeline_layout_create_info, NULL, &res->pipeline_layout);

    VkDescriptorPoolSize descriptor_pool_size_array[] = {
        {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
        },
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            res->sampler_count,
        },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        NULL,
        0,
        1,
        (uint32_t)(res->sampler_count ? 2 : 1),
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

    VkDescriptorBufferInfo descriptor_buffer_info = {
        self->uniform_buffer->buffer,
        0,
        VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet write_buffer_descriptor_set = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        NULL,
        res->descriptor_set,
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        NULL,
        &descriptor_buffer_info,
        NULL,
    };

    vkUpdateDescriptorSets(self->instance->device, 1, &write_buffer_descriptor_set, 0, NULL);

    if (res->sampler_count) {
        VkWriteDescriptorSet write_sampler_descriptor_set = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            res->descriptor_set,
            1,
            0,
            res->sampler_count,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            res->sampler_binding_array,
            NULL,
            NULL,
        };

        vkUpdateDescriptorSets(self->instance->device, 1, &write_sampler_descriptor_set, 0, NULL);
    }

    VkShaderModuleCreateInfo vertex_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(vertex_shader),
        (uint32_t *)PyBytes_AsString(vertex_shader),
    };

    vkCreateShaderModule(self->instance->device, &vertex_shader_module_create_info, NULL, &res->vertex_shader_module);

    VkShaderModuleCreateInfo fragment_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(fragment_shader),
        (uint32_t *)PyBytes_AsString(fragment_shader),
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
        bindings,
        binding_array,
        bindings,
        attribute_array,
    };

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        NULL,
        0,
        topology,
        false,
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
        VK_CULL_MODE_BACK_BIT,
        VK_FRONT_FACE_CLOCKWISE,
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
        self->samples,
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
        depth,
        depth,
        VK_COMPARE_OP_LESS,
        false,
        false,
        {},
        {},
        0.0f,
        0.0f,
    };

    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_array[64];

    for (int i = 0; i < 64; ++i) {
        pipeline_color_blend_attachment_array[i] = {
            blend,
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
        self->color_attachment_count,
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

    res->vertex_count = vertex_count;
    res->instance_count = instance_count;
    res->index_count = index_count;
    res->indirect_count = indirect_count;

    res->vertex_buffer_count = bindings;
    res->vertex_buffer_array = (VkBuffer *)PyMem_Malloc(bindings * sizeof(VkBuffer));
    res->vertex_offset_array = (VkDeviceSize *)PyMem_Malloc(bindings * sizeof(VkDeviceSize));

    for (uint32_t i = 0; i < first_intance_binding; ++i) {
        res->vertex_buffer_array[i] = res->vertex_buffer->buffer;
        res->vertex_offset_array[i] = 0;
    }

    for (uint32_t i = first_intance_binding; i < bindings; ++i) {
        res->vertex_buffer_array[i] = res->instance_buffer->buffer;
        res->vertex_offset_array[i] = 0;
    }

    PyList_Append(self->renderer_list, (PyObject *)res);
    res->enabled = true;
    return res;
}

PyObject * Context_meth_update(Context * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"uniform_buffer", "clear_value_buffer", NULL};

    PyObject * uniform_buffer = Py_None;
    PyObject * clear_value_buffer = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$OO", keywords, &uniform_buffer, &clear_value_buffer)) {
        return NULL;
    }

    if (!self->instance) {
        return NULL;
    }

    update_buffer(self->uniform_buffer, uniform_buffer);

    if (clear_value_buffer != Py_None) {
        Py_buffer view = {};
        PyObject_GetBuffer(clear_value_buffer, &view, PyBUF_STRIDED_RO);
        PyBuffer_ToContiguous(self->clear_value_array, &view, view.len, 'C');
        PyBuffer_Release(&view);
    }
    Py_RETURN_NONE;
}

PyObject * Context_meth_render(Context * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", NULL};

    uint32_t width = self->width;
    uint32_t height = self->height;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|(II)", keywords, &width, &height)) {
        return NULL;
    }

    if (!self->instance) {
        return NULL;
    }

    VkCommandBuffer cmd = begin_commands(self->instance);

    VkRenderPassBeginInfo render_pass_begin_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        NULL,
        self->render_pass,
        self->framebuffer,
        {{0, 0}, {width, height}},
        self->attachment_count,
        self->clear_value_array,
    };

    vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {{0, 0}, {width, height}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (int i = 0; i < PyList_Size(self->renderer_list); ++i) {
        Renderer * renderer = (Renderer *)PyList_GetItem(self->renderer_list, i);
        if (!renderer->enabled) {
            continue;
        }

        if (renderer->vertex_buffer_count) {
            vkCmdBindVertexBuffers(
                cmd,
                0,
                renderer->vertex_buffer_count,
                renderer->vertex_buffer_array,
                renderer->vertex_offset_array
            );
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->pipeline_layout,
            0,
            1,
            &renderer->descriptor_set,
            0,
            NULL
        );

        if (renderer->index_count) {
            vkCmdBindIndexBuffer(cmd, renderer->index_buffer->buffer, 0, renderer->index_type);
        }

        if (renderer->indirect_count && renderer->index_count) {
            vkCmdDrawIndexedIndirect(cmd, renderer->indirect_buffer->buffer, 0, renderer->indirect_count, 20);
        } else if (renderer->indirect_count) {
            vkCmdDrawIndirect(cmd, renderer->indirect_buffer->buffer, 0, renderer->indirect_count, 16);
        } else if (renderer->index_count) {
            vkCmdDrawIndexed(cmd, renderer->index_count, renderer->instance_count, 0, 0, 0);
        } else {
            vkCmdDraw(cmd, renderer->vertex_count, renderer->instance_count, 0, 0);
        }
    }

    vkCmdEndRenderPass(cmd);

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        0,
        NULL
    );

    if (!self->render_to_image) {
        VkDeviceSize offset = 0;
        for (uint32_t i = 0; i < self->color_attachment_count; ++i) {
            Image * image = self->color_attachment_array[i].resolve;

            VkBufferImageCopy copy = {
                offset,
                width,
                height,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                {0, 0, 0},
                {width, height, 1},
            };

            offset += image->size;

            vkCmdCopyImageToBuffer(
                cmd,
                image->image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                self->instance->host_buffer->buffer,
                1,
                &copy
            );
        }
    }

    end_commands(self->instance);

    if (!self->render_to_image && self->color_attachment_count == 1) {
        VkDeviceSize size = width * height * self->color_attachment_array[0].size;
        return PyBytes_FromStringAndSize((char *)self->instance->host_memory->ptr, size);
    }

    if (!self->render_to_image) {
        PyObject * res = PyTuple_New(self->color_attachment_count);
        char * ptr = (char *)self->instance->host_memory->ptr;

        for (uint32_t i = 0; i < self->color_attachment_count; ++i) {
            VkDeviceSize size = width * height * self->color_attachment_array[i].size;
            PyObject * obj = PyBytes_FromStringAndSize(ptr, size);
            PyTuple_SetItem(res, i, obj);
            ptr += size;
        }
        return res;
    }

    Py_RETURN_NONE;
}

PyObject * Renderer_meth_update(Renderer * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_buffer",
        "instance_buffer",
        "index_buffer",
        "indirect_buffer",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        "enabled",
        NULL,
    };

    PyObject * vertex_buffer = Py_None;
    PyObject * instance_buffer = Py_None;
    PyObject * index_buffer = Py_None;
    PyObject * indirect_buffer = Py_None;

    uint32_t vertex_count = self->vertex_count;
    uint32_t instance_count = self->instance_count;
    uint32_t index_count = self->index_count;
    uint32_t indirect_count = self->indirect_count;
    VkBool32 enabled = self->enabled;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|$OOOOIIIIp",
        keywords,
        &vertex_buffer,
        &instance_buffer,
        &index_buffer,
        &indirect_buffer,
        &vertex_count,
        &instance_count,
        &index_count,
        &indirect_count,
        &enabled
    );

    if (!args_ok) {
        return NULL;
    }

    if (!self->ctx->instance) {
        return NULL;
    }

    update_buffer(self->vertex_buffer, vertex_buffer);
    update_buffer(self->instance_buffer, instance_buffer);
    update_buffer(self->index_buffer, index_buffer);
    update_buffer(self->indirect_buffer, indirect_buffer);

    self->vertex_count = vertex_count;
    self->instance_count = instance_count;
    self->index_count = index_count;
    self->indirect_count = indirect_count;
    self->enabled = enabled;

    Py_RETURN_NONE;
}

PyObject * Image_meth_write(Image * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"data", NULL};

    Py_buffer data = {};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*", keywords, &data)) {
        return NULL;
    }

    if (!self->instance) {
        return NULL;
    }

    resize_buffer(self->instance->host_buffer, (VkDeviceSize)data.len);

    PyBuffer_ToContiguous(self->instance->host_memory->ptr, &data, data.len, 'C');
    PyBuffer_Release(&data);

    VkCommandBuffer cmd = begin_commands(self->instance);

    VkImageMemoryBarrier image_barrier_transfer = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        0,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        self->image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
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
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
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

    VkImageMemoryBarrier image_barrier_general = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        0,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        self->image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
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

    end_commands(self->instance);
    Py_RETURN_NONE;
}

PyObject * Instance_meth_info(Instance * self) {
    if (!self->instance) {
        return NULL;
    }

    PyObject * context_info = PyList_New(PyList_Size(self->context_list));
    for (uint32_t i = 0; i < PyList_Size(self->context_list); ++i) {
        Context * context = (Context *)PyList_GetItem(self->context_list, i);

        PyObject * renderer_info = PyList_New(PyList_Size(context->renderer_list));
        for (uint32_t j = 0; j < PyList_Size(context->renderer_list); ++j) {
            Renderer * renderer = (Renderer *)PyList_GetItem(context->renderer_list, j);

            int vertex_buffer_index = list_index(self->buffer_list, renderer->vertex_buffer);
            int instance_buffer_index = list_index(self->buffer_list, renderer->instance_buffer);
            int index_buffer_index = list_index(self->buffer_list, renderer->index_buffer);
            int indirect_buffer_index = list_index(self->buffer_list, renderer->indirect_buffer);

            PyObject * image_info = PyList_New(PyList_Size(renderer->image_list));
            for (uint32_t k = 0; k < PyList_Size(renderer->image_list); ++k) {
                Image * image = (Image *)PyList_GetItem(renderer->image_list, k);
                PyList_SetItem(image_info, k, PyLong_FromLong(list_index(self->image_list, image)));
            }

            PyList_SetItem(renderer_info, j, Py_BuildValue(
                "{sOsIsIsIsIsisisisisN}",
                "enabled",
                renderer->enabled ? Py_True : Py_False,
                "vertex_count",
                renderer->vertex_count,
                "instance_count",
                renderer->instance_count,
                "index_count",
                renderer->index_count,
                "indirect_count",
                renderer->indirect_count,
                "vertex_buffer",
                vertex_buffer_index,
                "instance_buffer",
                instance_buffer_index,
                "index_buffer",
                index_buffer_index,
                "indirect_buffer",
                indirect_buffer_index,
                "images",
                image_info
            ));
        }

        PyObject * color_attachment_info = PyList_New(context->color_attachment_count);
        for (uint32_t j = 0; j < context->color_attachment_count; ++j) {
            int image_index = list_index(self->image_list, context->color_attachment_array[j].image);
            int resolve_index = list_index(self->image_list, context->color_attachment_array[j].resolve);
            PyList_SetItem(color_attachment_info, j, Py_BuildValue(
                "{sisi}",
                "image",
                image_index,
                "resolve",
                resolve_index
            ));
        }

        int depth_image_index = list_index(self->image_list, context->depth_image);

        PyList_SetItem(context_info, i, Py_BuildValue(
            "{s{s(II)sIsisN}sN}",
            "framebuffer",
            "size",
            context->width,
            context->height,
            "samples",
            context->samples,
            "depth_image",
            depth_image_index,
            "images",
            color_attachment_info,
            "renderers",
            renderer_info
        ));
    }

    PyObject * memory_info = PyList_New(PyList_Size(self->memory_list));
    for (int i = 0; i < PyList_Size(self->memory_list); ++i) {
        Memory * memory = (Memory *)PyList_GetItem(self->memory_list, i);
        PyList_SetItem(memory_info, i, Py_BuildValue(
            "{sksk}",
            "size",
            memory->size,
            "offset",
            memory->offset
        ));
    }

    PyObject * buffer_info = PyList_New(PyList_Size(self->buffer_list));
    for (int i = 0; i < PyList_Size(self->buffer_list); ++i) {
        Buffer * buffer = (Buffer *)PyList_GetItem(self->buffer_list, i);
        int memory_index = list_index(self->memory_list, buffer->memory);
        PyList_SetItem(buffer_info, i, Py_BuildValue(
            "{sIsksksi}",
            "id",
            buffer->buffer,
            "size",
            buffer->size,
            "offset",
            buffer->offset,
            "memory",
            memory_index
        ));
    }

    PyObject * image_info = PyList_New(PyList_Size(self->image_list));
    for (int i = 0; i < PyList_Size(self->image_list); ++i) {
        Image * image = (Image *)PyList_GetItem(self->image_list, i);
        int memory_index = list_index(self->memory_list, image->memory);
        PyList_SetItem(image_info, i, Py_BuildValue(
            "{s(II)sksi}",
            "size",
            image->extent.width,
            image->extent.height,
            "offset",
            image->offset,
            "memory",
            memory_index
        ));
    }

    int host_buffer_index = list_index(self->buffer_list, self->host_buffer);

    return Py_BuildValue(
        "{sNsNsNsNsi}",
        "contexts",
        context_info,
        "memories",
        memory_info,
        "buffers",
        buffer_info,
        "images",
        image_info,
        "host_buffer",
        host_buffer_index
    );

    return NULL;
    Py_RETURN_NONE;
}

PyObject * Instance_meth_release(Instance * self) {
    if (!self->instance) {
        return NULL;
    }

    for (int i = 0; i < PyList_Size(self->context_list); ++i) {
        Context * context = (Context *)PyList_GetItem(self->context_list, i);

        vkDestroyFramebuffer(self->device, context->framebuffer, NULL);
        vkDestroyRenderPass(self->device, context->render_pass, NULL);

        for (uint32_t i = 0; i < context->attachment_count; ++i) {
            vkDestroyImageView(self->device, context->attachment_image_view_array[i], NULL);
        }

        for (int j = 0; j < PyList_Size(context->renderer_list); ++j) {
            Renderer * renderer = (Renderer *)PyList_GetItem(context->renderer_list, j);

            vkDestroyPipeline(self->device, renderer->pipeline, NULL);
            vkDestroyPipelineLayout(self->device, renderer->pipeline_layout, NULL);
            vkDestroyShaderModule(self->device, renderer->vertex_shader_module, NULL);
            vkDestroyShaderModule(self->device, renderer->fragment_shader_module, NULL);

            vkDestroyDescriptorPool(self->device, renderer->descriptor_pool, NULL);
            vkDestroyDescriptorSetLayout(self->device, renderer->descriptor_set_layout, NULL);

            for (uint32_t k = 0; k < renderer->sampler_count; ++k) {
                vkDestroySampler(self->device, renderer->sampler_array[k], NULL);
                vkDestroyImageView(self->device, renderer->sampler_binding_array[k].imageView, NULL);
            }
        }
    }

    for (int i = 0; i < PyList_Size(self->buffer_list); ++i) {
        Buffer * buffer = (Buffer *)PyList_GetItem(self->buffer_list, i);
        if (buffer->buffer) {
            vkDestroyBuffer(self->device, buffer->buffer, NULL);
        }
    }

    for (int i = 0; i < PyList_Size(self->image_list); ++i) {
        Image * image = (Image *)PyList_GetItem(self->image_list, i);
        vkDestroyImage(self->device, image->image, NULL);
    }

    vkDestroyCommandPool(self->device, self->command_pool, NULL);
    vkDestroyFence(self->device, self->fence, NULL);

    for (int i = 0; i < PyList_Size(self->memory_list); ++i) {
        Memory * memory = (Memory *)PyList_GetItem(self->memory_list, i);
        vkFreeMemory(self->device, memory->memory, NULL);
    }

    vkDestroyDevice(self->device, NULL);
    vkDestroyInstance(self->instance, NULL);
    self->instance = NULL;
    Py_RETURN_NONE;
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
        if (!vformat) {
            return NULL;
        }
        for (int k = 0; k < PyList_Size(vformat); ++k) {
            PyObject * format = PyDict_GetItem(format_dict, PyList_GetItem(vformat, k));
            PackType pack_type = (PackType)PyLong_AsUnsignedLong(PyTuple_GetItem(format, 2));
            int pack_items = PyLong_AsLong(PyTuple_GetItem(format, 3));
            row_size += PyLong_AsLong(PyTuple_GetItem(format, 1));
            while (pack_items--) {
                format_array[format_count++] = pack_type;
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

void default_dealloc(PyObject * self) {
    Py_TYPE(self)->tp_free(self);
}

PyMemberDef Instance_members[] = {
    {"device_memory", T_OBJECT, offsetof(Instance, device_memory), READONLY, NULL},
    {},
};

PyMethodDef Instance_methods[] = {
    {"context", (PyCFunction)Instance_meth_context, METH_VARARGS | METH_KEYWORDS, NULL},
    {"memory", (PyCFunction)Instance_meth_memory, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image", (PyCFunction)Instance_meth_image, METH_VARARGS | METH_KEYWORDS, NULL},
    {"sampler", (PyCFunction)Instance_meth_sampler, METH_VARARGS | METH_KEYWORDS, NULL},
    {"release", (PyCFunction)Instance_meth_release, METH_NOARGS, NULL},
    {"info", (PyCFunction)Instance_meth_info, METH_NOARGS, NULL},
    {},
};

PyMemberDef Context_members[] = {
    {"output", T_OBJECT, offsetof(Context, output), READONLY, NULL},
    {},
};

PyMethodDef Context_methods[] = {
    {"renderer", (PyCFunction)Context_meth_renderer, METH_VARARGS | METH_KEYWORDS, NULL},
    {"update", (PyCFunction)Context_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {"render", (PyCFunction)Context_meth_render, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Renderer_methods[] = {
    {"update", (PyCFunction)Renderer_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Image_methods[] = {
    {"write", (PyCFunction)Image_meth_write, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyType_Slot Instance_slots[] = {
    {Py_tp_methods, Instance_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Context_slots[] = {
    {Py_tp_members, Context_members},
    {Py_tp_methods, Context_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Memory_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Renderer_slots[] = {
    {Py_tp_methods, Renderer_methods},
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
PyType_Spec Context_spec = {"glnext.Context", sizeof(Context), 0, Py_TPFLAGS_DEFAULT, Context_slots};
PyType_Spec Memory_spec = {"glnext.Memory", sizeof(Memory), 0, Py_TPFLAGS_DEFAULT, Memory_slots};
PyType_Spec Renderer_spec = {"glnext.Renderer", sizeof(Renderer), 0, Py_TPFLAGS_DEFAULT, Renderer_slots};
PyType_Spec Buffer_spec = {"glnext.Buffer", sizeof(Buffer), 0, Py_TPFLAGS_DEFAULT, Buffer_slots};
PyType_Spec Image_spec = {"glnext.Image", sizeof(Image), 0, Py_TPFLAGS_DEFAULT, Image_slots};
PyType_Spec Sampler_spec = {"glnext.Sampler", sizeof(Sampler), 0, Py_TPFLAGS_DEFAULT, Sampler_slots};

PyMethodDef module_methods[] = {
    {"instance", (PyCFunction)glnext_meth_instance, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pack", (PyCFunction)glnext_meth_pack, METH_FASTCALL, NULL},
    {"camera", (PyCFunction)glnext_meth_camera, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "glnext", NULL, -1, module_methods};

extern "C" PyObject * PyInit_glnext() {
    PyObject * module = PyModule_Create(&module_def);

    Instance_type = (PyTypeObject *)PyType_FromSpec(&Instance_spec);
    Context_type = (PyTypeObject *)PyType_FromSpec(&Context_spec);
    Memory_type = (PyTypeObject *)PyType_FromSpec(&Memory_spec);
    Renderer_type = (PyTypeObject *)PyType_FromSpec(&Renderer_spec);
    Buffer_type = (PyTypeObject *)PyType_FromSpec(&Buffer_spec);
    Image_type = (PyTypeObject *)PyType_FromSpec(&Image_spec);
    Sampler_type = (PyTypeObject *)PyType_FromSpec(&Sampler_spec);

    default_framebuffer_format = PyUnicode_FromString("4b");
    default_border = PyUnicode_FromString("transparent");

    empty_list = PyList_New(0);
    empty_str = PyUnicode_FromString("");

    sampler_address_mode_dict = Py_BuildValue(
        "{sIsIsIsIsI}",
        "repeat", VK_SAMPLER_ADDRESS_MODE_REPEAT,
        "mirrored_repeat", VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        "clamp_to_edge", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        "clamp_to_border", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        "mirror_clamp_to_edge", VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
    );

    sampler_filter_dict = Py_BuildValue(
        "{sIsI}",
        "nearest", VK_FILTER_NEAREST,
        "linear", VK_FILTER_LINEAR
    );

    mipmap_filter_dict = Py_BuildValue(
        "{sIsI}",
        "nearest", VK_SAMPLER_MIPMAP_MODE_NEAREST,
        "linear", VK_SAMPLER_MIPMAP_MODE_LINEAR
    );

    float_border_dict = Py_BuildValue(
        "{sIsIsI}",
        "transparent", VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        "black", VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        "white", VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
    );

    int_border_dict = Py_BuildValue(
        "{sIsIsI}",
        "transparent", VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        "black", VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        "white", VK_BORDER_COLOR_INT_OPAQUE_WHITE
    );

    topology_dict = Py_BuildValue(
        "{sIsIsIsIsIsI}",
        "points", VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        "lines", VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        "line_strip", VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        "triangles", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        "triangle_strip", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        "triangle_fan", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
    );

    format_dict = PyDict_New();

    register_format("1f", VK_FORMAT_R32_SFLOAT, 4, PACK_FLOAT, 1);
    register_format("2f", VK_FORMAT_R32G32_SFLOAT, 8, PACK_FLOAT, 2);
    register_format("3f", VK_FORMAT_R32G32B32_SFLOAT, 12, PACK_FLOAT, 3);
    register_format("4f", VK_FORMAT_R32G32B32A32_SFLOAT, 16, PACK_FLOAT, 4);
    register_format("1i", VK_FORMAT_R32_SINT, 4, PACK_INT, 1);
    register_format("2i", VK_FORMAT_R32G32_SINT, 8, PACK_INT, 2);
    register_format("3i", VK_FORMAT_R32G32B32_SINT, 12, PACK_INT, 3);
    register_format("4i", VK_FORMAT_R32G32B32A32_SINT, 16, PACK_INT, 4);
    register_format("1b", VK_FORMAT_R8_UNORM, 1, PACK_BYTE, 1);
    register_format("2b", VK_FORMAT_R8G8_UNORM, 2, PACK_BYTE, 2);
    register_format("3b", VK_FORMAT_R8G8B8_UNORM, 3, PACK_BYTE, 3);
    register_format("4b", VK_FORMAT_B8G8R8A8_UNORM, 4, PACK_BYTE, 4);
    register_format("1x", VK_FORMAT_UNDEFINED, 1, PACK_PAD, 1);
    register_format("2x", VK_FORMAT_UNDEFINED, 2, PACK_PAD, 2);
    register_format("3x", VK_FORMAT_UNDEFINED, 3, PACK_PAD, 3);
    register_format("4x", VK_FORMAT_UNDEFINED, 4, PACK_PAD, 4);

    return module;
}
