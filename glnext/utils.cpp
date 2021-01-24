#include "glnext.hpp"

#define SpvMagicNumber 0x07230203
#define SpvOpExecutionMode 16
#define SpvExecutionModeLocalSize 17

void get_compute_local_size(PyObject * shader, uint32_t * size) {
    uint32_t * data = (uint32_t *)PyBytes_AsString(shader);
    uint32_t count = (uint32_t)PyBytes_Size(shader) / 4;
    if (data[0] != SpvMagicNumber) {
        return;
    }
    uint32_t idx = 5;
    while (idx < count) {
        uint32_t opcode = data[idx] & 0xffff;
        uint32_t word_count = data[idx] >> 16;
        if (opcode == SpvOpExecutionMode) {
            if (data[idx + 2] == SpvExecutionModeLocalSize) {
                size[0] = data[idx + 3];
                size[1] = data[idx + 4];
                size[2] = data[idx + 5];
            }
        }
        idx += word_count;
    }
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

Memory * new_memory(Instance * instance, VkBool32 host = false) {
    Memory * res = PyObject_New(Memory, instance->state->Memory_type);
    res->instance = instance;
    res->memory = NULL;
    res->offset = 0;
    res->size = 0;
    res->host = host;
    res->ptr = NULL;
    PyList_Append(instance->memory_list, (PyObject *)res);
    return res;
}

Memory * get_memory(Instance * instance, PyObject * memory) {
    if (memory == Py_None) {
        return new_memory(instance);
    }
    if (PyObject_Type(memory) == (PyObject *)instance->state->Memory_type) {
        return (Memory *)memory;
    }
    PyErr_Format(PyExc_TypeError, "memory");
    return NULL;
}

VkDeviceSize take_memory(Memory * self, VkMemoryRequirements * requirements) {
    if (VkDeviceSize padding = self->offset % requirements->alignment) {
        self->offset += requirements->alignment - padding;
    }
    VkDeviceSize res = self->offset;
    self->offset += requirements->size;
    return res;
}

void allocate_memory(Memory * self) {
    if (self->size) {
        return;
    }

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        NULL,
        self->offset,
        self->host ? self->instance->host_memory_type_index : self->instance->device_memory_type_index,
    };

    self->size = self->offset;
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

Image * new_image(ImageCreateInfo * create_info) {
    Image * res = PyObject_New(Image, create_info->instance->state->Image_type);
    res->instance = create_info->instance;
    res->memory = create_info->memory;
    res->offset = 0;
    res->size = create_info->size;
    res->aspect = create_info->aspect;
    res->extent = create_info->extent;
    res->samples = create_info->samples;
    res->levels = create_info->levels;
    res->layers = create_info->layers;
    res->mode = create_info->mode;
    res->format = create_info->format;
    res->image = NULL;
    res->staging_buffer = NULL;
    res->staging_offset = 0;

    VkImageCreateFlags flags = 0;
    if (create_info->mode == IMG_STORAGE) {
        flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        flags,
        VK_IMAGE_TYPE_2D,
        create_info->format,
        create_info->extent,
        create_info->levels,
        create_info->layers,
        (VkSampleCountFlagBits)create_info->samples,
        VK_IMAGE_TILING_OPTIMAL,
        create_info->usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    vkCreateImage(create_info->instance->device, &image_create_info, NULL, &res->image);

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(create_info->instance->device, res->image, &requirements);
    res->offset = take_memory(create_info->memory, &requirements);

    PyList_Append(create_info->instance->image_list, (PyObject *)res);
    return res;
}

Buffer * new_buffer(BufferCreateInfo * create_info) {
    Buffer * res = PyObject_New(Buffer, create_info->instance->state->Buffer_type);
    res->instance = create_info->instance;
    res->memory = create_info->memory;
    res->offset = 0;
    res->size = create_info->size;
    res->usage = create_info->usage;
    res->buffer = NULL;
    res->staging_buffer = NULL;
    res->staging_offset = 0;

    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        create_info->size,
        create_info->usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    vkCreateBuffer(create_info->instance->device, &buffer_create_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(create_info->instance->device, res->buffer, &requirements);
    res->offset = take_memory(create_info->memory, &requirements);

    PyList_Append(create_info->instance->buffer_list, (PyObject *)res);
    return res;
}

void bind_image(Image * image) {
    vkBindImageMemory(image->instance->device, image->image, image->memory->memory, image->offset);
}

void bind_buffer(Buffer * buffer) {
    vkBindBufferMemory(buffer->instance->device, buffer->buffer, buffer->memory->memory, buffer->offset);
}


uint16_t half_float(double value) {
    union {
        float f;
        uint32_t x;
    };
    f = (float)value;
    return (x >> 16 & 0x8000) | (((x >> 23 & 0xff) - 127 + 15) << 10 & 0x1f) | (x >> 12 & 0x3ff);
}

void pack_float_1(char * ptr, PyObject ** obj) {
    ((float *)ptr)[0] = (float)PyFloat_AsDouble(obj[0]);
}

void pack_float_2(char * ptr, PyObject ** obj) {
    ((float *)ptr)[0] = (float)PyFloat_AsDouble(obj[0]);
    ((float *)ptr)[1] = (float)PyFloat_AsDouble(obj[1]);
}

void pack_float_3(char * ptr, PyObject ** obj) {
    ((float *)ptr)[0] = (float)PyFloat_AsDouble(obj[0]);
    ((float *)ptr)[1] = (float)PyFloat_AsDouble(obj[1]);
    ((float *)ptr)[2] = (float)PyFloat_AsDouble(obj[2]);
}

void pack_float_4(char * ptr, PyObject ** obj) {
    ((float *)ptr)[0] = (float)PyFloat_AsDouble(obj[0]);
    ((float *)ptr)[1] = (float)PyFloat_AsDouble(obj[1]);
    ((float *)ptr)[2] = (float)PyFloat_AsDouble(obj[2]);
    ((float *)ptr)[3] = (float)PyFloat_AsDouble(obj[3]);
}

void pack_hfloat_1(char * ptr, PyObject ** obj) {
    ((uint16_t *)ptr)[0] = half_float(PyFloat_AsDouble(obj[0]));
}

void pack_hfloat_2(char * ptr, PyObject ** obj) {
    ((uint16_t *)ptr)[0] = half_float(PyFloat_AsDouble(obj[0]));
    ((uint16_t *)ptr)[1] = half_float(PyFloat_AsDouble(obj[1]));
}

void pack_hfloat_3(char * ptr, PyObject ** obj) {
    ((uint16_t *)ptr)[0] = half_float(PyFloat_AsDouble(obj[0]));
    ((uint16_t *)ptr)[1] = half_float(PyFloat_AsDouble(obj[1]));
    ((uint16_t *)ptr)[2] = half_float(PyFloat_AsDouble(obj[2]));
}

void pack_hfloat_4(char * ptr, PyObject ** obj) {
    ((uint16_t *)ptr)[0] = half_float(PyFloat_AsDouble(obj[0]));
    ((uint16_t *)ptr)[1] = half_float(PyFloat_AsDouble(obj[1]));
    ((uint16_t *)ptr)[2] = half_float(PyFloat_AsDouble(obj[2]));
    ((uint16_t *)ptr)[3] = half_float(PyFloat_AsDouble(obj[3]));
}

void pack_int_1(char * ptr, PyObject ** obj) {
    ((int32_t *)ptr)[0] = PyLong_AsLong(obj[0]);
}

void pack_int_2(char * ptr, PyObject ** obj) {
    ((int32_t *)ptr)[0] = PyLong_AsLong(obj[0]);
    ((int32_t *)ptr)[1] = PyLong_AsLong(obj[1]);
}

void pack_int_3(char * ptr, PyObject ** obj) {
    ((int32_t *)ptr)[0] = PyLong_AsLong(obj[0]);
    ((int32_t *)ptr)[1] = PyLong_AsLong(obj[1]);
    ((int32_t *)ptr)[2] = PyLong_AsLong(obj[2]);
}

void pack_int_4(char * ptr, PyObject ** obj) {
    ((int32_t *)ptr)[0] = PyLong_AsLong(obj[0]);
    ((int32_t *)ptr)[1] = PyLong_AsLong(obj[1]);
    ((int32_t *)ptr)[2] = PyLong_AsLong(obj[2]);
    ((int32_t *)ptr)[3] = PyLong_AsLong(obj[3]);
}

void pack_uint_1(char * ptr, PyObject ** obj) {
    ((uint32_t *)ptr)[0] = PyLong_AsUnsignedLong(obj[0]);
}

void pack_uint_2(char * ptr, PyObject ** obj) {
    ((uint32_t *)ptr)[0] = PyLong_AsUnsignedLong(obj[0]);
    ((uint32_t *)ptr)[1] = PyLong_AsUnsignedLong(obj[1]);
}

void pack_uint_3(char * ptr, PyObject ** obj) {
    ((uint32_t *)ptr)[0] = PyLong_AsUnsignedLong(obj[0]);
    ((uint32_t *)ptr)[1] = PyLong_AsUnsignedLong(obj[1]);
    ((uint32_t *)ptr)[2] = PyLong_AsUnsignedLong(obj[2]);
}

void pack_uint_4(char * ptr, PyObject ** obj) {
    ((uint32_t *)ptr)[0] = PyLong_AsUnsignedLong(obj[0]);
    ((uint32_t *)ptr)[1] = PyLong_AsUnsignedLong(obj[1]);
    ((uint32_t *)ptr)[2] = PyLong_AsUnsignedLong(obj[2]);
    ((uint32_t *)ptr)[3] = PyLong_AsUnsignedLong(obj[3]);
}

void pack_byte_1(char * ptr, PyObject ** obj) {
    ((uint8_t *)ptr)[0] = (uint8_t)PyLong_AsUnsignedLong(obj[0]);
}

void pack_byte_2(char * ptr, PyObject ** obj) {
    ((uint8_t *)ptr)[0] = (uint8_t)PyLong_AsUnsignedLong(obj[0]);
    ((uint8_t *)ptr)[1] = (uint8_t)PyLong_AsUnsignedLong(obj[1]);
}

void pack_byte_3(char * ptr, PyObject ** obj) {
    ((uint8_t *)ptr)[0] = (uint8_t)PyLong_AsUnsignedLong(obj[0]);
    ((uint8_t *)ptr)[1] = (uint8_t)PyLong_AsUnsignedLong(obj[1]);
    ((uint8_t *)ptr)[2] = (uint8_t)PyLong_AsUnsignedLong(obj[2]);
}

void pack_byte_4(char * ptr, PyObject ** obj) {
    ((uint8_t *)ptr)[0] = (uint8_t)PyLong_AsUnsignedLong(obj[0]);
    ((uint8_t *)ptr)[1] = (uint8_t)PyLong_AsUnsignedLong(obj[1]);
    ((uint8_t *)ptr)[2] = (uint8_t)PyLong_AsUnsignedLong(obj[2]);
    ((uint8_t *)ptr)[3] = (uint8_t)PyLong_AsUnsignedLong(obj[3]);
}

void pack_pad(char * ptr, PyObject ** obj) {
}

Format get_format(PyObject * name) {
    const char * s = PyUnicode_AsUTF8(name);
    if (!strcmp(s, "1f")) return {VK_FORMAT_R32_SFLOAT, 4, pack_float_1, 1};
    if (!strcmp(s, "2f")) return {VK_FORMAT_R32G32_SFLOAT, 8, pack_float_2, 2};
    if (!strcmp(s, "3f")) return {VK_FORMAT_R32G32B32_SFLOAT, 12, pack_float_3, 3};
    if (!strcmp(s, "4f")) return {VK_FORMAT_R32G32B32A32_SFLOAT, 16, pack_float_4, 4};
    if (!strcmp(s, "1h")) return {VK_FORMAT_R16_SFLOAT, 2, pack_hfloat_1, 1};
    if (!strcmp(s, "2h")) return {VK_FORMAT_R16G16_SFLOAT, 4, pack_hfloat_2, 2};
    if (!strcmp(s, "3h")) return {VK_FORMAT_R16G16B16_SFLOAT, 6, pack_hfloat_3, 3};
    if (!strcmp(s, "4h")) return {VK_FORMAT_R16G16B16A16_SFLOAT, 8, pack_hfloat_4, 4};
    if (!strcmp(s, "1i")) return {VK_FORMAT_R32_SINT, 4, pack_int_1, 1};
    if (!strcmp(s, "2i")) return {VK_FORMAT_R32G32_SINT, 8, pack_int_2, 2};
    if (!strcmp(s, "3i")) return {VK_FORMAT_R32G32B32_SINT, 12, pack_int_3, 3};
    if (!strcmp(s, "4i")) return {VK_FORMAT_R32G32B32A32_SINT, 16, pack_int_4, 4};
    if (!strcmp(s, "1u")) return {VK_FORMAT_R32_UINT, 4, pack_uint_1, 1};
    if (!strcmp(s, "2u")) return {VK_FORMAT_R32G32_UINT, 8, pack_uint_2, 2};
    if (!strcmp(s, "3u")) return {VK_FORMAT_R32G32B32_UINT, 12, pack_uint_3, 3};
    if (!strcmp(s, "4u")) return {VK_FORMAT_R32G32B32A32_UINT, 16, pack_uint_4, 4};
    if (!strcmp(s, "1b")) return {VK_FORMAT_R8_UINT, 1, pack_byte_1, 1};
    if (!strcmp(s, "2b")) return {VK_FORMAT_R8G8_UINT, 2, pack_byte_2, 2};
    if (!strcmp(s, "3b")) return {VK_FORMAT_R8G8B8_UINT, 3, pack_byte_3, 3};
    if (!strcmp(s, "4b")) return {VK_FORMAT_R8G8B8A8_UINT, 4, pack_byte_4, 4};
    if (!strcmp(s, "1p")) return {VK_FORMAT_R8_UNORM, 1, pack_byte_1, 1};
    if (!strcmp(s, "2p")) return {VK_FORMAT_R8G8_UNORM, 2, pack_byte_2, 2};
    if (!strcmp(s, "3p")) return {VK_FORMAT_R8G8B8_UNORM, 3, pack_byte_3, 3};
    if (!strcmp(s, "4p")) return {VK_FORMAT_R8G8B8A8_UNORM, 4, pack_byte_4, 4};
    if (!strcmp(s, "1x")) return {VK_FORMAT_UNDEFINED, 1, pack_pad, 0};
    if (!strcmp(s, "2x")) return {VK_FORMAT_UNDEFINED, 2, pack_pad, 0};
    if (!strcmp(s, "3x")) return {VK_FORMAT_UNDEFINED, 3, pack_pad, 0};
    if (!strcmp(s, "4x")) return {VK_FORMAT_UNDEFINED, 4, pack_pad, 0};
    if (!strcmp(s, "8x")) return {VK_FORMAT_UNDEFINED, 8, pack_pad, 0};
    if (!strcmp(s, "12x")) return {VK_FORMAT_UNDEFINED, 12, pack_pad, 0};
    if (!strcmp(s, "16x")) return {VK_FORMAT_UNDEFINED, 16, pack_pad, 0};
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
    if (!PyUnicode_CompareWithASCIIString(name, "storage")) {
        return IMG_STORAGE;
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

VkSampler sampler_from_json(Instance * instance, PyObject * obj) {
    VkSamplerCreateInfo sampler_create_info = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        NULL,
        0,
    };

    VkSampler sampler = NULL;
    vkCreateSampler(instance->device, &sampler_create_info, NULL, &sampler);
    return sampler;
};

void new_temp_buffer(Instance * instance, HostBuffer * temp, VkDeviceSize size) {
    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    vkCreateBuffer(instance->device, &buffer_create_info, NULL, &temp->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(instance->device, temp->buffer, &requirements);

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        NULL,
        requirements.size,
        instance->host_memory_type_index,
    };

    vkAllocateMemory(instance->device, &memory_allocate_info, NULL, &temp->memory);
    vkMapMemory(instance->device, temp->memory, 0, size, 0, &temp->ptr);
    vkBindBufferMemory(instance->device, temp->buffer, temp->memory, 0);
}

void free_temp_buffer(Instance * instance, HostBuffer * temp) {
    vkUnmapMemory(instance->device, temp->memory);
    vkFreeMemory(instance->device, temp->memory, NULL);
    vkDestroyBuffer(instance->device, temp->buffer, NULL);
}

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
                args.image_array[i]->image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                args.image_array[i]->image,
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
                args.image_array[i]->image,
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
                args.image_array[i]->image,
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
                args.image_array[i]->image,
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

void end_commands_with_present(Instance * instance) {
    if (instance->presenter.surface_count) {
        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[64];

        for (uint32_t i = 0; i < instance->presenter.surface_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                instance->presenter.image_array[i][instance->presenter.index_array[i]],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
        }

        vkCmdPipelineBarrier(
            instance->command_buffer,
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

    for (uint32_t i = 0; i < instance->presenter.surface_count; ++i) {
        vkCmdCopyImage(
            instance->command_buffer,
            instance->presenter.image_source_array[i],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            instance->presenter.image_array[i][instance->presenter.index_array[i]],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &instance->presenter.image_copy_array[i]
        );
    }

    if (instance->presenter.surface_count) {
        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[64];

        for (uint32_t i = 0; i < instance->presenter.surface_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                instance->presenter.image_array[i][instance->presenter.index_array[i]],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
        }

        vkCmdPipelineBarrier(
            instance->command_buffer,
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

    vkEndCommandBuffer(instance->command_buffer);

    for (uint32_t i = 0; i < instance->presenter.surface_count; ++i) {
        vkAcquireNextImageKHR(
            instance->device,
            instance->presenter.swapchain_array[i],
            UINT64_MAX,
            instance->presenter.semaphore_array[i],
            NULL,
            &instance->presenter.index_array[i]
        );
    }

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        instance->presenter.surface_count,
        instance->presenter.semaphore_array,
        instance->presenter.wait_stage_array,
        1,
        &instance->command_buffer,
        0,
        NULL,
    };

    vkQueueSubmit(instance->queue, 1, &submit_info, instance->fence);
    vkWaitForFences(instance->device, 1, &instance->fence, true, UINT64_MAX);
    vkResetFences(instance->device, 1, &instance->fence);

    if (instance->presenter.surface_count) {
        VkPresentInfoKHR present_info = {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            NULL,
            0,
            NULL,
            instance->presenter.surface_count,
            instance->presenter.swapchain_array,
            instance->presenter.index_array,
            instance->presenter.result_array,
        };

        vkQueuePresentKHR(instance->queue, &present_info);

        uint32_t idx = 0;
        while (idx < instance->presenter.surface_count) {
            if (instance->presenter.result_array[idx] == VK_ERROR_OUT_OF_DATE_KHR) {
                vkDestroySemaphore(instance->device, instance->presenter.semaphore_array[idx], NULL);
                vkDestroySwapchainKHR(instance->device, instance->presenter.swapchain_array[idx], NULL);
                vkDestroySurfaceKHR(instance->instance, instance->presenter.surface_array[idx], NULL);
                presenter_remove(&instance->presenter, idx);
                continue;
            }
            idx += 1;
        }
    }
}
