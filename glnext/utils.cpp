#include "glnext.hpp"

uint32_t take_uint(PyObject * dict, const char * key) {
    PyObject * value = PyDict_GetItemString(dict, key);
    if (!value) {
        PyErr_Format(PyExc_KeyError, key);
        return 0;
    }
    return PyLong_AsUnsignedLong(value);
}

VkCommandBuffer begin_commands(Instance * self) {
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        NULL,
    };

    self->vkBeginCommandBuffer(self->command_buffer, &command_buffer_begin_info);
    return self->command_buffer;
}

void end_commands(Instance * self) {
    self->vkEndCommandBuffer(self->command_buffer);

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

    self->vkQueueSubmit(self->queue, 1, &submit_info, self->fence);
    self->vkWaitForFences(self->device, 1, &self->fence, true, UINT64_MAX);
    self->vkResetFences(self->device, 1, &self->fence);
}

void end_commands_with_present(Instance * self) {
    for (uint32_t i = 0; i < self->presenter.surface_count; ++i) {
        self->vkAcquireNextImageKHR(
            self->device,
            self->presenter.swapchain_array[i],
            UINT64_MAX,
            self->presenter.semaphore_array[i],
            NULL,
            &self->presenter.index_array[i]
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
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                self->presenter.image_array[i][self->presenter.index_array[i]],
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
        }

        self->vkCmdPipelineBarrier(
            self->command_buffer,
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
        self->vkCmdBlitImage(
            self->command_buffer,
            self->presenter.image_source_array[i],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            self->presenter.image_array[i][self->presenter.index_array[i]],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &self->presenter.image_blit_array[i],
            VK_FILTER_NEAREST
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

        self->vkCmdPipelineBarrier(
            self->command_buffer,
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

    self->vkEndCommandBuffer(self->command_buffer);

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

    self->vkQueueSubmit(self->queue, 1, &submit_info, self->fence);
    self->vkWaitForFences(self->device, 1, &self->fence, true, UINT64_MAX);
    self->vkResetFences(self->device, 1, &self->fence);

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

        self->vkQueuePresentKHR(self->queue, &present_info);

        uint32_t idx = 0;
        while (idx < self->presenter.surface_count) {
            if (self->presenter.result_array[idx] == VK_ERROR_OUT_OF_DATE_KHR) {
                self->vkDestroySemaphore(self->device, self->presenter.semaphore_array[idx], NULL);
                self->vkDestroySwapchainKHR(self->device, self->presenter.swapchain_array[idx], NULL);
                self->vkDestroySurfaceKHR(self->instance, self->presenter.surface_array[idx], NULL);
                presenter_remove(&self->presenter, idx);
                continue;
            }
            idx += 1;
        }
    }
}

Memory * new_memory(Instance * self, VkBool32 host) {
    Memory * res = PyObject_New(Memory, self->state->Memory_type);
    res->instance = self;
    res->memory = NULL;
    res->offset = 0;
    res->size = 0;
    res->host = host;
    res->ptr = NULL;
    PyList_Append(self->memory_list, (PyObject *)res);
    return res;
}

Memory * get_memory(Instance * self, PyObject * memory) {
    if (memory == Py_None) {
        return new_memory(self);
    }
    if (PyObject_Type(memory) == (PyObject *)self->state->Memory_type) {
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
    self->instance->vkAllocateMemory(self->instance->device, &memory_allocate_info, NULL, &self->memory);

    if (self->host) {
        self->instance->vkMapMemory(self->instance->device, self->memory, 0, VK_WHOLE_SIZE, 0, &self->ptr);
    }
}

void free_memory(Memory * self) {
    if (self->host) {
        self->instance->vkUnmapMemory(self->instance->device, self->memory);
    }

    self->instance->vkFreeMemory(self->instance->device, self->memory, NULL);

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
    res->staging_buffer = NULL;
    res->staging_offset = 0;
    res->user_created = info.user_created;

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

    info.instance->vkCreateImage(info.instance->device, &image_info, NULL, &res->image);

    VkMemoryRequirements requirements = {};
    info.instance->vkGetImageMemoryRequirements(info.instance->device, res->image, &requirements);
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
    res->staging_buffer = NULL;
    res->staging_offset = 0;

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

    info.instance->vkCreateBuffer(info.instance->device, &buffer_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    info.instance->vkGetBufferMemoryRequirements(info.instance->device, res->buffer, &requirements);
    res->offset = take_memory(info.memory, &requirements);

    PyList_Append(info.instance->buffer_list, (PyObject *)res);
    return res;
}

void bind_image(Image * self) {
    self->instance->vkBindImageMemory(self->instance->device, self->image, self->memory->memory, self->offset);
}

void bind_buffer(Buffer * self) {
    self->instance->vkBindBufferMemory(self->instance->device, self->buffer, self->memory->memory, self->offset);
}

BufferBinding parse_buffer_binding(Instance * instance, PyObject * obj) {
    BufferBinding res = {};

    res.name = PyDict_GetItemString(obj, "name");

    if (!res.name) {
        res.name = Py_None;
    }

    Py_INCREF(res.name);

    res.binding = take_uint(obj, "binding");

    if (PyErr_Occurred()) {
        return res;
    }

    PyObject * type = PyDict_GetItemString(obj, "type");

    if (!type) {
        PyErr_Format(PyExc_KeyError, "type");
        return res;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "uniform_buffer")) {
        res.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        res.descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        res.mode = BUF_UNIFORM;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "storage_buffer")) {
        res.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        res.descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        res.mode = BUF_STORAGE;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "input_buffer")) {
        res.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        res.descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        res.mode = BUF_INPUT;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "output_buffer")) {
        res.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        res.descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        res.mode = BUF_OUTPUT;
    }

    if (!res.usage) {
        PyErr_Format(PyExc_TypeError, "invalid type");
        return res;
    }

    PyObject * buffer = PyDict_GetItemString(obj, "buffer");

    if (buffer && Py_TYPE(buffer) != instance->state->Buffer_type) {
        PyErr_Format(PyExc_TypeError, "invalid buffer");
        return res;
    }

    if (buffer && buffer != Py_None) {
        res.buffer = (Buffer *)buffer;
    }

    if (!res.buffer) {
        PyObject * size = PyDict_GetItemString(obj, "size");

        if (!size) {
            PyErr_Format(PyExc_KeyError, "type");
            return res;
        }

        res.size = PyLong_AsUnsignedLongLong(size);

        if (PyErr_Occurred()) {
            return res;
        }
    }

    res.descriptor_buffer_info = {
        NULL,
        0,
        VK_WHOLE_SIZE,
    };

    return res;
}

ImageBinding parse_image_binding(Instance * instance, PyObject * obj) {
    ImageBinding res = {};

    res.name = PyDict_GetItemString(obj, "name");

    if (!res.name) {
        res.name = Py_None;
    }

    Py_INCREF(res.name);

    res.binding = take_uint(obj, "binding");

    if (PyErr_Occurred()) {
        return res;
    }

    PyObject * type = PyDict_GetItemString(obj, "type");

    if (!type) {
        PyErr_Format(PyExc_KeyError, "type");
        return res;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "storage_image")) {
        res.descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        res.layout = VK_IMAGE_LAYOUT_GENERAL;
        res.sampled = false;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "sampled_image")) {
        res.descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        res.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        res.sampled = true;
    }

    if (res.layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        PyErr_Format(PyExc_TypeError, "invalid type");
        return res;
    }

    PyObject * images = PyDict_GetItemString(obj, "images");

    if (!images) {
        PyErr_Format(PyExc_KeyError, "images");
        return res;
    }

    if (!PyList_CheckExact(images)) {
        PyErr_Format(PyExc_ValueError, "images");
        return res;
    }

    res.image_count = (uint32_t)PyList_Size(images);

    res.image_array = allocate<Image *>(res.image_count);
    res.sampler_array = allocate<VkSampler>(res.image_count);
    res.image_view_array = allocate<VkImageView>(res.image_count);
    res.sampler_create_info_array = allocate<VkSamplerCreateInfo>(res.image_count);
    res.descriptor_image_info_array = allocate<VkDescriptorImageInfo>(res.image_count);
    res.image_view_create_info_array = allocate<VkImageViewCreateInfo>(res.image_count);

    for (uint32_t i = 0; i < res.image_count; ++i) {
        PyObject * item = PyList_GetItem(images, i);

        if (!PyDict_CheckExact(item)) {
            PyErr_Format(PyExc_ValueError, "images");
            return res;
        }
        Image * image = (Image *)PyDict_GetItemString(item, "image");

        if (!image) {
            PyErr_Format(PyExc_KeyError, "image");
            return res;
        }

        if (Py_TYPE(image) != instance->state->Image_type) {
            PyErr_Format(PyExc_ValueError, "invalid image");
            return res;
        }

        res.image_view_array[i] = NULL;
        res.sampler_array[i] = NULL;
        res.image_array[i] = image;

        res.sampler_create_info_array[i] = {
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

        res.image_view_create_info_array[i] = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            res.image_array[i]->image,
            VK_IMAGE_VIEW_TYPE_2D,
            res.image_array[i]->format,
            {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, res.image_array[i]->layers},
        };
    }

    return res;
}

void new_temp_buffer(Instance * self, HostBuffer * temp, VkDeviceSize size) {
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

    self->vkCreateBuffer(self->device, &buffer_create_info, NULL, &temp->buffer);

    VkMemoryRequirements requirements = {};
    self->vkGetBufferMemoryRequirements(self->device, temp->buffer, &requirements);

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        NULL,
        requirements.size,
        self->host_memory_type_index,
    };

    self->vkAllocateMemory(self->device, &memory_allocate_info, NULL, &temp->memory);
    self->vkMapMemory(self->device, temp->memory, 0, size, 0, &temp->ptr);
    self->vkBindBufferMemory(self->device, temp->buffer, temp->memory, 0);
}

void free_temp_buffer(Instance * self, HostBuffer * temp) {
    self->vkUnmapMemory(self->device, temp->memory);
    self->vkFreeMemory(self->device, temp->memory, NULL);
    self->vkDestroyBuffer(self->device, temp->buffer, NULL);
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
    return (x >> 16 & 0xc000) | (x >> 13 & 0x3cf0) | (x >> 13 & 0x03ff);
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
    if (!strcmp(s, "1s")) return {VK_FORMAT_R8_SRGB, 1, pack_byte_1, 1};
    if (!strcmp(s, "2s")) return {VK_FORMAT_R8G8_SRGB, 2, pack_byte_2, 2};
    if (!strcmp(s, "3s")) return {VK_FORMAT_R8G8B8_SRGB, 3, pack_byte_3, 3};
    if (!strcmp(s, "4s")) return {VK_FORMAT_R8G8B8A8_SRGB, 4, pack_byte_4, 4};
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

template <typename T>
void realloc_array(T ** array, uint32_t size) {
    *array = (T *)PyMem_Realloc(*array, sizeof(T) * size);
}

void presenter_resize(Presenter * self) {
    realloc_array(&self->surface_array, self->surface_count);
    realloc_array(&self->swapchain_array, self->surface_count);
    realloc_array(&self->wait_stage_array, self->surface_count);
    realloc_array(&self->semaphore_array, self->surface_count);
    realloc_array(&self->image_source_array, self->surface_count);
    realloc_array(&self->image_blit_array, self->surface_count);
    realloc_array(&self->image_count_array, self->surface_count);
    realloc_array(&self->image_array, self->surface_count);
    realloc_array(&self->result_array, self->surface_count);
    realloc_array(&self->index_array, self->surface_count);
}

void presenter_remove(Presenter * self, uint32_t index) {
    PyMem_Free(self->image_array[index]);
    self->surface_count -= 1;
    for (uint32_t i = index; i < self->surface_count; ++i) {
        self->surface_array[i] = self->surface_array[i + 1];
        self->swapchain_array[i] = self->swapchain_array[i + 1];
        self->wait_stage_array[i] = self->wait_stage_array[i + 1];
        self->semaphore_array[i] = self->semaphore_array[i + 1];
        self->image_source_array[i] = self->image_source_array[i + 1];
        self->image_blit_array[i] = self->image_blit_array[i + 1];
        self->image_count_array[i] = self->image_count_array[i + 1];
        self->image_array[i] = self->image_array[i + 1];
        self->result_array[i] = self->result_array[i + 1];
        self->index_array[i] = self->index_array[i + 1];
    }
    presenter_resize(self);
}
