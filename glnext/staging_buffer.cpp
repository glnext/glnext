#include "glnext.hpp"

StagingBuffer * Instance_meth_staging_buffer(Instance * self, PyObject * arg) {
    StagingBuffer * res = PyObject_New(StagingBuffer, self->state->StagingBuffer_type);

    res->instance = self;

    PyObject * seq = PySequence_Fast(arg, "not iterable");
    if (!seq) {
        return NULL;
    }

    VkDeviceSize max_size = 0;

    for (uint32_t k = 0; k < PyList_Size(arg); ++k) {
        PyObject * lst = PyList_GetItem(arg, k);
        VkDeviceSize size = 0;

        for (uint32_t i = 0; i < PyList_Size(lst); ++i) {
            PyObject * obj = PyList_GetItem(lst, i);

            if (Py_TYPE(obj) == self->state->Image_type) {
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
    Py_DECREF(seq);

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

    vkCreateBuffer(self->device, &buffer_create_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(self->device, res->buffer, &requirements);

    VkMemoryAllocateInfo memory_allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        NULL,
        requirements.size,
        self->host_memory_type_index,
    };

    vkAllocateMemory(self->device, &memory_allocate_info, NULL, &res->memory);
    vkMapMemory(self->device, res->memory, 0, res->size, 0, &res->ptr);
    vkBindBufferMemory(self->device, res->buffer, res->memory, 0);

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

void staging_input_buffer(VkCommandBuffer command_buffer, Buffer * buffer) {
    if (buffer && buffer->staging_buffer) {
        VkBufferCopy copy = {
            buffer->staging_offset,
            0,
            buffer->size,
        };
        vkCmdCopyBuffer(
            command_buffer,
            buffer->staging_buffer->buffer,
            buffer->buffer,
            1,
            &copy
        );
    }
}

void staging_output_buffer(VkCommandBuffer command_buffer, Buffer * buffer) {
    if (buffer && buffer->staging_buffer) {
        VkBufferCopy copy = {
            0,
            buffer->staging_offset,
            buffer->size,
        };
        vkCmdCopyBuffer(
            command_buffer,
            buffer->buffer,
            buffer->staging_buffer->buffer,
            1,
            &copy
        );
    }
}

void staging_input_image(VkCommandBuffer command_buffer, Image * image) {
    if (image && image->staging_buffer) {
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

        vkCmdPipelineBarrier(
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
            image->staging_offset,
            image->extent.width,
            image->extent.height,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, image->layers},
            {0, 0, 0},
            image->extent,
        };

        vkCmdCopyBufferToImage(
            command_buffer,
            image->staging_buffer->buffer,
            image->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copy
        );

        if (image->samples == 1) {
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

            vkCmdPipelineBarrier(
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
            build_mipmaps({
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
}
