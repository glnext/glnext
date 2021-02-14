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

    VkDeviceSize max_size = args.size;

    for (uint32_t k = 0; k < PyList_Size(args.bindings); ++k) {
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

        PyObject * type = PyDict_GetItemString(obj, "type");

        if (!type || !PyUnicode_CheckExact(type)) {
            PyErr_Format(PyExc_ValueError, "type");
            return NULL;
        }

        bool is_image = false;
        bool is_buffer = false;
        bool is_render_pipeline = false;
        bool is_compute_pipeline = false;
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

        if (!PyUnicode_CompareWithASCIIString(type, "render_parameters")) {
            is_render_pipeline = true;
            is_input = true;
        }

        if (!PyUnicode_CompareWithASCIIString(type, "compute_parameters")) {
            is_compute_pipeline = true;
            is_input = true;
        }

        if (!is_input && !is_output) {
            PyErr_Format(PyExc_ValueError, "type");
            return NULL;
        }

        if (is_buffer) {
            Buffer * buffer = (Buffer *)PyDict_GetItemString(obj, "buffer");

            if (!buffer || Py_TYPE(buffer) != self->state->Buffer_type) {
                PyErr_Format(PyExc_ValueError, "buffer");
                return NULL;
            }

            if (buffer->staging_buffer) {
                PyErr_Format(PyExc_ValueError, "buffer already marked for staging");
                return NULL;
            }

            if (is_input) {
                PyList_Append(self->staged_inputs, (PyObject *)buffer);
            }

            if (is_output) {
                PyList_Append(self->staged_outputs, (PyObject *)buffer);
            }

            buffer->staging_buffer = res;
            buffer->staging_offset = offset;

            if (max_size < offset + buffer->size + 8) {
                max_size = offset + buffer->size + 8;
            }
        }

        if (is_image) {
            Image * image = (Image *)PyDict_GetItemString(obj, "image");

            if (!image || Py_TYPE(image) != self->state->Image_type) {
                PyErr_Format(PyExc_ValueError, "image");
                return NULL;
            }

            if (image->staging_buffer) {
                PyErr_Format(PyExc_ValueError, "image already marked for staging");
                return NULL;
            }

            if (is_input) {
                PyList_Append(self->staged_inputs, (PyObject *)image);
            }

            if (is_output) {
                PyList_Append(self->staged_outputs, (PyObject *)image);
            }

            image->staging_buffer = res;
            image->staging_offset = offset;

            if (max_size < offset + image->size + 4) {
                max_size = offset + image->size + 4;
            }
        }

        if (is_render_pipeline) {
            RenderPipeline * pipeline = (RenderPipeline *)PyDict_GetItemString(obj, "pipeline");

            if (!pipeline || Py_TYPE(pipeline) != self->state->RenderPipeline_type) {
                PyErr_Format(PyExc_ValueError, "pipeline");
                return NULL;
            }

            if (pipeline->staging_buffer) {
                PyErr_Format(PyExc_ValueError, "pipeline already marked for staging");
                return NULL;
            }

            pipeline->staging_buffer = res;
            pipeline->staging_offset = offset;

            if (max_size < offset + sizeof(RenderParameters)) {
                max_size = offset + sizeof(RenderParameters);
            }

            PyList_Append(self->staged_inputs, (PyObject *)pipeline);
        }

        if (is_compute_pipeline) {
            ComputePipeline * pipeline = (ComputePipeline *)PyDict_GetItemString(obj, "pipeline");

            if (!pipeline || Py_TYPE(pipeline) != self->state->ComputePipeline_type) {
                PyErr_Format(PyExc_ValueError, "pipeline");
                return NULL;
            }

            if (pipeline->staging_buffer) {
                PyErr_Format(PyExc_ValueError, "pipeline already marked for staging");
                return NULL;
            }

            pipeline->staging_buffer = res;
            pipeline->staging_offset = offset;

            if (max_size < offset + sizeof(ComputeParameters)) {
                max_size = offset + sizeof(ComputeParameters);
            }

            PyList_Append(self->staged_inputs, (PyObject *)pipeline);
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
    self->vkMapMemory(self->device, res->memory, 0, res->size, 0, &res->ptr);
    self->vkBindBufferMemory(self->device, res->buffer, res->memory, 0);

    memset(res->ptr, 0, res->size);

    res->mem = PyMemoryView_FromMemory((char *)res->ptr, res->size, PyBUF_WRITE);
    Py_INCREF(res);
    return res;
}

PyObject * StagingBuffer_meth_read(StagingBuffer * self) {
    return PyBytes_FromStringAndSize((char *)self->ptr, self->size);
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

void staging_input_buffer(Buffer * self) {
    VkDeviceSize size = *(VkDeviceSize *)((char *)self->staging_buffer->ptr + self->staging_offset);

    if (!size) {
        return;
    }

    VkBufferCopy copy = {
        self->staging_offset + 8,
        0,
        size,
    };

    self->instance->vkCmdCopyBuffer(
        self->instance->command_buffer,
        self->staging_buffer->buffer,
        self->buffer,
        1,
        &copy
    );
}

void staging_output_buffer(Buffer * self) {
    VkDeviceSize size = *(VkDeviceSize *)((char *)self->staging_buffer->ptr + self->staging_offset);

    if (!size) {
        return;
    }

    VkBufferCopy copy = {
        0,
        self->staging_offset + 8,
        size,
    };

    self->instance->vkCmdCopyBuffer(
        self->instance->command_buffer,
        self->buffer,
        self->staging_buffer->buffer,
        1,
        &copy
    );
}

void staging_input_image(Image * self) {
    VkBool32 enabled = *(VkBool32 *)((char *)self->staging_buffer->ptr + self->staging_offset);

    if (!enabled) {
        return;
    }

    VkImageLayout image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if (self->mode == IMG_TEXTURE) {
        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (self->mode == IMG_STORAGE) {
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
        self->image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, self->layers},
    };

    self->instance->vkCmdPipelineBarrier(
        self->instance->command_buffer,
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
        self->staging_offset + 4,
        self->extent.width,
        self->extent.height,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, self->layers},
        {0, 0, 0},
        self->extent,
    };

    self->instance->vkCmdCopyBufferToImage(
        self->instance->command_buffer,
        self->staging_buffer->buffer,
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

        self->instance->vkCmdPipelineBarrier(
            self->instance->command_buffer,
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
        // build_mipmaps({
        //     command_buffer,
        //     image->extent.width,
        //     image->extent.height,
        //     image->levels,
        //     image->layers,
        //     1,
        //     &image,
        // });
    }
}

void staging_output_image(Image * self) {
    VkBool32 enabled = *(VkBool32 *)((char *)self->staging_buffer->ptr + self->staging_offset);

    if (!enabled) {
        return;
    }

    VkBufferImageCopy copy = {
        self->staging_offset + 4,
        self->extent.width,
        self->extent.height,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, self->layers},
        {0, 0, 0},
        self->extent,
    };

    self->instance->vkCmdCopyImageToBuffer(
        self->instance->command_buffer,
        self->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        self->staging_buffer->buffer,
        1,
        &copy
    );
}

void staging_render_parameters(RenderPipeline * self) {
    self->parameters = *(RenderParameters *)((char *)self->staging_buffer->ptr + self->staging_offset);
}

void staging_compute_parameters(ComputePipeline * self) {
    self->parameters = *(ComputeParameters *)((char *)self->staging_buffer->ptr + self->staging_offset);
}
