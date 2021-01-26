#include "glnext.hpp"

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

Memory * new_memory(Instance * instance, VkBool32 host) {
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

    if (!self->offset) {
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

Image * new_image(ImageCreateInfo info) {
    Image * res = PyObject_New(Image, info.instance->state->Image_type);
    res->instance = info.instance;
    res->memory = info.memory;
    res->offset = 0;
    res->size = info.size;
    res->aspect = info.aspect;
    res->extent = info.extent;
    res->samples = info.samples;
    res->levels = info.levels;
    res->layers = info.layers;
    res->mode = info.mode;
    res->format = info.format;
    res->image = NULL;
    // res->staging_buffer = NULL;
    // res->staging_offset = 0;

    VkImageCreateFlags flags = 0;
    if (info.mode == IMG_STORAGE) {
        flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    VkImageCreateInfo image_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        flags,
        VK_IMAGE_TYPE_2D,
        info.format,
        info.extent,
        info.levels,
        info.layers,
        (VkSampleCountFlagBits)info.samples,
        VK_IMAGE_TILING_OPTIMAL,
        info.usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    vkCreateImage(info.instance->device, &image_info, NULL, &res->image);

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(info.instance->device, res->image, &requirements);
    res->offset = take_memory(info.memory, &requirements);

    PyList_Append(info.instance->image_list, (PyObject *)res);
    return res;
}

Buffer * new_buffer(BufferCreateInfo info) {
    Buffer * res = PyObject_New(Buffer, info.instance->state->Buffer_type);
    res->instance = info.instance;
    res->memory = info.memory;
    res->offset = 0;
    res->size = info.size;
    res->usage = info.usage;
    res->buffer = NULL;
    // res->staging_buffer = NULL;
    // res->staging_offset = 0;

    VkBufferCreateInfo buffer_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        info.size,
        info.usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    vkCreateBuffer(info.instance->device, &buffer_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(info.instance->device, res->buffer, &requirements);
    res->offset = take_memory(info.memory, &requirements);

    PyList_Append(info.instance->buffer_list, (PyObject *)res);
    return res;
}

void bind_image(Image * image) {
    vkBindImageMemory(image->instance->device, image->image, image->memory->memory, image->offset);
}

void bind_buffer(Buffer * buffer) {
    vkBindBufferMemory(buffer->instance->device, buffer->buffer, buffer->memory->memory, buffer->offset);
}

BufferBinding parse_buffer_binding(PyObject * obj) {
    BufferBinding res = {};

    PyObject * type = PyDict_GetItemString(obj, "type");
    PyObject * buffer = PyDict_GetItemString(obj, "buffer");

    if (buffer && buffer != Py_None) {
        res.buffer = (Buffer *)buffer;
    } else {
        res.size = PyLong_AsUnsignedLongLong(PyDict_GetItemString(obj, "size"));
    }

    res.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (!PyUnicode_CompareWithASCIIString(type, "storage_buffer")) {
        res.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        res.descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "uniform_buffer")) {
        res.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        res.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    return res;
}

ImageBinding parse_image_binding(PyObject * obj) {
    ImageBinding res = {};

    PyObject * type = PyDict_GetItemString(obj, "type");

    res.sampled = false;
    res.image_count = 1;
    res.image_array = new Image * [1];
    res.sampler_array = new VkSampler[1];
    res.image_view_array = new VkImageView[1];
    res.sampler_create_info_array = new VkSamplerCreateInfo[1];
    res.descriptor_image_info_array = new VkDescriptorImageInfo[1];
    res.image_view_create_info_array = new VkImageViewCreateInfo[1];

    Image * image = (Image *)PyDict_GetItemString(obj, "image");

    if (!PyUnicode_CompareWithASCIIString(type, "storage_image")) {
        res.descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        res.layout = VK_IMAGE_LAYOUT_GENERAL;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "sampled_image")) {
        res.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        res.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    res.image_view_array[0] = NULL;
    res.sampler_array[0] = NULL;
    res.image_array[0] = image;

    res.sampler_create_info_array[0] = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        NULL,
        0,
        VK_FILTER_NEAREST,
        VK_FILTER_NEAREST,
        VK_SAMPLER_MIPMAP_MODE_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0.0f,
        false,
        0.0f,
        false,
        VK_COMPARE_OP_NEVER,
        0.0f,
        1000.0f,
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        false,
    };

    res.image_view_create_info_array[0] = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        NULL,
        0,
        res.image_array[0]->image,
        VK_IMAGE_VIEW_TYPE_2D,
        res.image_array[0]->format,
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, res.image_array[0]->layers},
    };

    return res;
}

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
