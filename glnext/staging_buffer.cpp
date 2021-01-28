#include "glnext.hpp"

StagingBuffer * Instance_meth_staging_buffer(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"bindings", "size", NULL};

    struct {
        PyObject * bindings;
        VkDeviceSize size = 0;
    } args;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwargs, "O!|k", keywords, &PyList_Type, &args.bindings, &args.size)) {
        return NULL;
    }

    StagingBuffer * res = PyObject_New(StagingBuffer, self->state->StagingBuffer_type);

    res->instance = self;

    VkDeviceSize max_size = args.size;

    for (uint32_t k = 0; k < PyList_Size(args.bindings); ++k) {
        PyObject * lst = PyList_GetItem(args.bindings, k);
        VkDeviceSize size = 0;

        for (uint32_t i = 0; i < PyList_Size(lst); ++i) {
            PyObject * obj = PyList_GetItem(lst, i);

            if (Py_TYPE(obj) == self->state->Image_type) {
                PyList_Append(self->staged_input_image_list, obj);
                Image * image = (Image *)obj;
                image->staging_buffer = res;
                image->staging_offset = size;
                size += image->size;
            }
            if (Py_TYPE(obj) == self->state->Buffer_type) {
                Buffer * buffer = (Buffer *)obj;
                buffer->staging_buffer = res;
                buffer->staging_offset = size;
                size += buffer->size;
            }
        }

        if (max_size < size) {
            max_size = size;
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

    res->mem = PyMemoryView_FromMemory((char *)res->ptr, res->size, PyBUF_WRITE);
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
    if (self && self->staging_buffer) {
        VkBufferCopy copy = {
            self->staging_offset,
            0,
            self->size,
        };
        self->instance->vkCmdCopyBuffer(
            self->instance->command_buffer,
            self->staging_buffer->buffer,
            self->buffer,
            1,
            &copy
        );
    }
}

void staging_output_buffer(Buffer * self) {
    if (self && self->staging_buffer) {
        VkBufferCopy copy = {
            0,
            self->staging_offset,
            self->size,
        };
        self->instance->vkCmdCopyBuffer(
            self->instance->command_buffer,
            self->buffer,
            self->staging_buffer->buffer,
            1,
            &copy
        );
    }
}

void staging_input_image(Image * self) {
    if (self && self->staging_buffer) {
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
            self->staging_offset,
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
}
