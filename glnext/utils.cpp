#include "glnext.hpp"

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
    copy_present_images(self);

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

    present_images(self);
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

void allocate_memory(Memory * self, VkMemoryDedicatedAllocateInfo * dedicated) {
    if (self->size) {
        return;
    }

    if (!self->offset) {
        return;
    }

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        dedicated,
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
    res->bound = false;

    VkImageCreateFlags flags = 0;
    if (info.mode == IMG_STORAGE) {
        flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    if (info.layers % 6 == 0) {
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
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

    if (info.instance->extension.dedicated_allocation && info.mode == IMG_PROTECTED) {
        VkMemoryDedicatedAllocateInfo dedicated_info = {
            VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
            NULL,
            res->image,
            NULL,
        };
        res->memory = new_memory(info.instance);
        res->offset = take_memory(res->memory, &requirements);
        allocate_memory(res->memory, &dedicated_info);
    } else {
        res->offset = take_memory(res->memory, &requirements);
    }

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
    res->bound = false;

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
    if (!self->bound) {
        self->instance->vkBindImageMemory(self->instance->device, self->image, self->memory->memory, self->offset);
        self->bound = true;
    }
}

void bind_buffer(Buffer * self) {
    if (!self->bound) {
        self->instance->vkBindBufferMemory(self->instance->device, self->buffer, self->memory->memory, self->offset);
        self->bound = true;
    }
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

void build_mipmaps(BuildMipmapsInfo args) {
    for (uint32_t level = 1; level < args.levels; ++level) {
        uint32_t parent = level - 1;
        VkImageBlit image_blit = {
            {VK_IMAGE_ASPECT_COLOR_BIT, parent, 0, args.layers},
            {{0, 0, 0}, {(int32_t)args.width >> parent, (int32_t)args.height >> parent, 1}},
            {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, args.layers},
            {{0, 0, 0}, {(int32_t)args.width >> level, (int32_t)args.height >> level, 1}},
        };

        for (uint32_t i = 0; i < args.image_count; ++i) {
            args.instance->vkCmdBlitImage(
                args.command_buffer,
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

        args.instance->vkCmdPipelineBarrier(
            args.command_buffer,
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
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, args.levels, 0, args.layers},
            };
        }

        args.instance->vkCmdPipelineBarrier(
            args.command_buffer,
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

void copy_present_images(Instance * self) {
    if (!self->presenter.surface_count) {
        return;
    }

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

    for (uint32_t i = 0; i < self->presenter.surface_count; ++i) {
        VkImage image = self->presenter.image_array[i].image_array[self->presenter.index_array[i]];
        self->presenter.copy_image_barrier_array[i].image = image;
        self->presenter.present_image_barrier_array[i].image = image;
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
        self->presenter.surface_count,
        self->presenter.copy_image_barrier_array
    );

    for (uint32_t i = 0; i < self->presenter.surface_count; ++i) {
        self->vkCmdBlitImage(
            self->command_buffer,
            self->presenter.image_source_array[i],
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            self->presenter.image_array[i].image_array[self->presenter.index_array[i]],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &self->presenter.image_blit_array[i],
            VK_FILTER_NEAREST
        );
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
        self->presenter.surface_count,
        self->presenter.present_image_barrier_array
    );
}

void present_images(Instance * self) {
    if (!self->presenter.surface_count) {
        return;
    }

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

void presenter_remove(Presenter * self, uint32_t index) {
    self->surface_count -= 1;
    for (uint32_t i = index; i < self->surface_count; ++i) {
        self->surface_array[i] = self->surface_array[i + 1];
        self->swapchain_array[i] = self->swapchain_array[i + 1];
        self->wait_stage_array[i] = self->wait_stage_array[i + 1];
        self->copy_image_barrier_array[i] = self->copy_image_barrier_array[i + 1];
        self->present_image_barrier_array[i] = self->present_image_barrier_array[i + 1];
        self->semaphore_array[i] = self->semaphore_array[i + 1];
        self->image_source_array[i] = self->image_source_array[i + 1];
        self->image_blit_array[i] = self->image_blit_array[i + 1];
        self->image_array[i] = self->image_array[i + 1];
        self->result_array[i] = self->result_array[i + 1];
        self->index_array[i] = self->index_array[i + 1];
    }
}
