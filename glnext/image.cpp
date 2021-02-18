#include "glnext.hpp"

Image * Instance_meth_image(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "size",
        "format",
        "levels",
        "layers",
        "mode",
        "memory",
        NULL,
    };

    struct {
        uint32_t width;
        uint32_t height;
        PyObject * format;
        uint32_t levels = 1;
        uint32_t layers = 1;
        PyObject * mode;
        PyObject * memory = Py_None;
    } args;

    args.format = self->state->default_format;
    args.mode = self->state->texture_str;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "(II)|O$IIOO",
        keywords,
        &args.width,
        &args.height,
        &args.format,
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

    if (args.levels > 1 && image_mode == IMG_OUTPUT) {
        PyErr_Format(PyExc_ValueError, "invalid mode");
        return NULL;
    }

    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VkImageLayout image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if (image_mode == IMG_TEXTURE) {
        image_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (image_mode == IMG_STORAGE) {
        image_usage = VK_IMAGE_USAGE_STORAGE_BIT;
        image_layout = VK_IMAGE_LAYOUT_GENERAL;
    }

    if (args.levels > 1) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    Image * res = new_image({
        self,
        memory,
        args.width * args.height * args.layers * format.size,
        image_usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        {args.width, args.height, 1},
        1,
        args.levels,
        args.layers,
        image_mode,
        format.format,
    });

    allocate_memory(memory);
    bind_image(res);

    VkCommandBuffer command_buffer = begin_commands(self);

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
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, args.levels, 0, args.layers},
    };

    self->vkCmdPipelineBarrier(
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

    end_commands(self);

    return res;
}

PyObject * Image_meth_read(Image * self) {
    if (self->mode != IMG_OUTPUT) {
        PyErr_Format(PyExc_ValueError, "not an output image");
        return NULL;
    }

    HostBuffer temp = {};
    new_temp_buffer(self->instance, &temp, self->size);

    VkCommandBuffer command_buffer = begin_commands(self->instance);

    VkBufferImageCopy copy = {
        0,
        self->extent.width,
        self->extent.height,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, self->layers},
        {0, 0, 0},
        self->extent,
    };

    self->instance->vkCmdCopyImageToBuffer(
        command_buffer,
        self->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        temp.buffer,
        1,
        &copy
    );

    end_commands(self->instance);
    PyObject * res = PyBytes_FromStringAndSize((char *)temp.ptr, self->size);
    free_temp_buffer(self->instance, &temp);
    return res;
}

PyObject * Image_meth_write(Image * self, PyObject * arg) {
    Py_buffer view = {};
    if (PyObject_GetBuffer(arg, &view, PyBUF_STRIDED_RO)) {
        return NULL;
    }

    if (view.len != self->size) {
        PyErr_Format(PyExc_ValueError, "wrong size");
        return NULL;
    }

    HostBuffer temp = {};
    new_temp_buffer(self->instance, &temp, self->size);

    PyBuffer_ToContiguous(temp.ptr, &view, view.len, 'C');
    PyBuffer_Release(&view);

    VkCommandBuffer command_buffer = begin_commands(self->instance);

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
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, self->levels, 0, self->layers},
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
        0,
        self->extent.width,
        self->extent.height,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, self->layers},
        {0, 0, 0},
        self->extent,
    };

    self->instance->vkCmdCopyBufferToImage(
        command_buffer,
        temp.buffer,
        self->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy
    );

    if (self->levels == 1) {
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
        VkImageMemoryBarrier image_barrier = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            NULL,
            0,
            0,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            self->image,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, self->layers},
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
            &image_barrier
        );

        build_mipmaps({
            self->instance,
            command_buffer,
            self->extent.width,
            self->extent.height,
            self->levels,
            self->layers,
            1,
            &self,
        });
    }

    end_commands(self->instance);
    free_temp_buffer(self->instance, &temp);
    Py_RETURN_NONE;
}
