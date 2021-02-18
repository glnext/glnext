#include "glnext.hpp"

StagingBuffer * Instance_meth_staging(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"bindings", "size", NULL};

    struct {
        PyObject * bindings;
        VkDeviceSize size = 0;
    } args;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwargs, "O!|K", keywords, &PyList_Type, &args.bindings, &args.size)) {
        return NULL;
    }

    StagingBuffer * res = PyObject_New(StagingBuffer, self->state->StagingBuffer_type);

    res->instance = self;
    res->binding_count = (uint32_t)PyList_Size(args.bindings);
    res->binding_array = (StagingBufferBinding *)PyMem_Malloc(sizeof(StagingBufferBinding) * res->binding_count);

    VkDeviceSize max_size = args.size;

    for (uint32_t k = 0; k < res->binding_count; ++k) {
        PyObject * obj = PyList_GetItem(args.bindings, k);

        if (!PyDict_CheckExact(obj)) {
            PyErr_Format(PyExc_ValueError, "bindings");
            return NULL;
        }

        PyObject * offset_int = PyDict_GetItemString(obj, "offset");

        if (!offset_int || !PyLong_CheckExact(offset_int)) {
            PyErr_Format(PyExc_ValueError, "offset");
            return NULL;
        }

        VkDeviceSize offset = PyLong_AsUnsignedLongLong(offset_int);

        if (PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "offset");
            return NULL;
        }

        res->binding_array[k].offset = offset;

        PyObject * type = PyDict_GetItemString(obj, "type");

        if (!type || !PyUnicode_CheckExact(type)) {
            PyErr_Format(PyExc_ValueError, "type");
            return NULL;
        }

        bool is_image = false;
        bool is_buffer = false;
        bool is_input = false;
        bool is_output = false;

        if (!PyUnicode_CompareWithASCIIString(type, "input_image")) {
            is_image = true;
            is_input = true;
        }

        if (!PyUnicode_CompareWithASCIIString(type, "input_buffer")) {
            is_buffer = true;
            is_input = true;
        }

        if (!PyUnicode_CompareWithASCIIString(type, "output_image")) {
            is_image = true;
            is_output = true;
        }

        if (!PyUnicode_CompareWithASCIIString(type, "output_buffer")) {
            is_buffer = true;
            is_output = true;
        }

        if (!PyUnicode_CompareWithASCIIString(type, "input_output_buffer")) {
            is_buffer = true;
            is_input = true;
            is_output = true;
        }

        if (!is_input && !is_output) {
            PyErr_Format(PyExc_ValueError, "type");
            return NULL;
        }

        res->binding_array[k].is_input = is_input;
        res->binding_array[k].is_output = is_output;

        if (is_buffer) {
            Buffer * buffer = (Buffer *)PyDict_GetItemString(obj, "buffer");

            if (!buffer || Py_TYPE(buffer) != self->state->Buffer_type) {
                PyErr_Format(PyExc_ValueError, "buffer");
                return NULL;
            }

            res->binding_array[k].buffer = buffer;

            if (max_size < offset + buffer->size) {
                max_size = offset + buffer->size;
            }
        }

        if (is_image) {
            Image * image = (Image *)PyDict_GetItemString(obj, "image");

            if (!image || Py_TYPE(image) != self->state->Image_type) {
                PyErr_Format(PyExc_ValueError, "image");
                return NULL;
            }

            res->binding_array[k].image = image;

            if (max_size < offset + image->size) {
                max_size = offset + image->size;
            }
        }
    }

    res->size = max_size;

    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        res->size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    self->vkCreateBuffer(self->device, &buffer_create_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    self->vkGetBufferMemoryRequirements(self->device, res->buffer, &requirements);

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        NULL,
        requirements.size,
        self->host_memory_type_index,
    };

    self->vkAllocateMemory(self->device, &memory_allocate_info, NULL, &res->memory);
    self->vkMapMemory(self->device, res->memory, 0, res->size, 0, (void **)&res->ptr);
    self->vkBindBufferMemory(self->device, res->buffer, res->memory, 0);

    memset(res->ptr, 0, res->size);

    res->mem = PyMemoryView_FromMemory(res->ptr, res->size, PyBUF_WRITE);
    PyList_Append(self->staging_list, (PyObject *)res);
    return res;
}

PyObject * StagingBuffer_meth_read(StagingBuffer * self) {
    return PyBytes_FromStringAndSize(self->ptr, self->size);
}

PyObject * StagingBuffer_meth_write(StagingBuffer * self, PyObject * arg) {
    Py_buffer view = {};
    if (PyObject_GetBuffer(arg, &view, PyBUF_STRIDED_RO)) {
        return NULL;
    }

    PyBuffer_ToContiguous(self->ptr, &view, view.len, 'C');
    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

void execute_staging_buffer_input(StagingBuffer * self, VkCommandBuffer command_buffer) {
    for (uint32_t k = 0; k < self->binding_count; ++k) {
        if (!self->binding_array[k].is_input) {
            continue;
        }

        if (Py_TYPE(self->binding_array[k].obj) == self->instance->state->Buffer_type) {
            VkBufferCopy copy = {
                self->binding_array[k].offset,
                0,
                self->binding_array[k].buffer->size,
            };

            self->instance->vkCmdCopyBuffer(
                command_buffer,
                self->buffer,
                self->binding_array[k].buffer->buffer,
                1,
                &copy
            );
        }

        if (Py_TYPE(self->binding_array[k].obj) == self->instance->state->Image_type) {
            Image * image = self->binding_array[k].image;

            VkImageLayout image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            if (image->mode == IMG_TEXTURE) {
                image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            if (image->mode == IMG_STORAGE) {
                image_layout = VK_IMAGE_LAYOUT_GENERAL;
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
                image->image,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, image->layers},
            };

            self->instance->vkCmdPipelineBarrier(
                command_buffer,
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
                self->binding_array[k].offset,
                image->extent.width,
                image->extent.height,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, image->layers},
                {0, 0, 0},
                image->extent,
            };

            self->instance->vkCmdCopyBufferToImage(
                command_buffer,
                self->buffer,
                image->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &copy
            );

            if (image->levels == 1) {
                VkImageMemoryBarrier image_barrier_general = {
                    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    NULL,
                    0,
                    0,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    image_layout,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image->image,
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, image->layers},
                };

                self->instance->vkCmdPipelineBarrier(
                    command_buffer,
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
                VkImageMemoryBarrier image_barriers[] = {
                    {
                        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        NULL,
                        0,
                        0,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED,
                        image->image,
                        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, image->layers},
                    },
                    {
                        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        NULL,
                        0,
                        0,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED,
                        image->image,
                        {VK_IMAGE_ASPECT_COLOR_BIT, 1, image->levels - 1, 0, image->layers},
                    },
                };

                self->instance->vkCmdPipelineBarrier(
                    command_buffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0,
                    0,
                    NULL,
                    0,
                    NULL,
                    2,
                    image_barriers
                );

                build_mipmaps({
                    self->instance,
                    command_buffer,
                    image->extent.width,
                    image->extent.height,
                    image->levels,
                    image->layers,
                    1,
                    &image,
                });
            }
        }

        if (Py_TYPE(self->binding_array[k].obj) == self->instance->state->RenderPipeline_type) {
            self->binding_array[k].render_pipeline->parameters = *(RenderParameters *)(self->ptr + self->binding_array[k].offset);
        }

        if (Py_TYPE(self->binding_array[k].obj) == self->instance->state->ComputePipeline_type) {
            self->binding_array[k].compute_pipeline->parameters = *(ComputeParameters *)(self->ptr + self->binding_array[k].offset);
        }
    }
}

void execute_staging_buffer_output(StagingBuffer * self, VkCommandBuffer command_buffer) {
    for (uint32_t k = 0; k < self->binding_count; ++k) {
        if (!self->binding_array[k].is_output) {
            continue;
        }

        if (Py_TYPE(self->binding_array[k].obj) == self->instance->state->Buffer_type) {
            VkBufferCopy copy = {
                0,
                self->binding_array[k].offset,
                self->binding_array[k].buffer->size,
            };

            self->instance->vkCmdCopyBuffer(
                command_buffer,
                self->binding_array[k].buffer->buffer,
                self->buffer,
                1,
                &copy
            );
        }

        if (Py_TYPE(self->binding_array[k].obj) == self->instance->state->Image_type) {
            Image * image = self->binding_array[k].image;

            VkBufferImageCopy copy = {
                self->binding_array[k].offset,
                image->extent.width,
                image->extent.height,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, image->layers},
                {0, 0, 0},
                image->extent,
            };

            self->instance->vkCmdCopyImageToBuffer(
                command_buffer,
                image->image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                self->buffer,
                1,
                &copy
            );
        }
    }
}
