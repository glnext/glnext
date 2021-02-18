#include "glnext.hpp"

Framebuffer * Instance_meth_framebuffer(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "size",
        "format",
        "samples",
        "levels",
        "layers",
        "depth",
        "compute",
        "mode",
        "memory",
        NULL
    };

    struct {
        uint32_t width;
        uint32_t height;
        PyObject * format;
        uint32_t samples = 4;
        uint32_t levels = 1;
        uint32_t layers = 1;
        VkBool32 depth = true;
        VkBool32 compute = false;
        PyObject * mode;
        PyObject * memory = Py_None;
    } args;

    args.format = self->state->default_format;
    args.mode = self->state->output_str;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "(II)|O!$IIIppOO",
        keywords,
        &args.width,
        &args.height,
        &PyUnicode_Type,
        &args.format,
        &args.samples,
        &args.levels,
        &args.layers,
        &args.depth,
        &args.compute,
        &args.mode,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    Memory * memory = get_memory(self, args.memory);
    PyObject * format_list = PyUnicode_Split(args.format, NULL, -1);
    ImageMode image_mode = get_image_mode(args.mode);

    uint32_t output_count = (uint32_t)PyList_Size(format_list);
    uint32_t attachment_count = output_count;
    uint32_t image_barrier_count = 0;

    if (args.depth) {
        attachment_count += 1;
    }

    if (args.samples > 1) {
        attachment_count += output_count;
    }

    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VkImageLayout final_image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if (image_mode == IMG_TEXTURE) {
        image_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        if (args.levels == 1) {
            final_image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    if (image_mode == IMG_STORAGE) {
        image_usage = VK_IMAGE_USAGE_STORAGE_BIT;
        final_image_layout = VK_IMAGE_LAYOUT_GENERAL;
    }

    if (args.compute) {
        image_barrier_count = output_count;
        image_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (args.samples > 1) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    Framebuffer * res = PyObject_New(Framebuffer, self->state->Framebuffer_type);

    res->instance = self;
    res->width = args.width;
    res->height = args.height;
    res->samples = args.samples;
    res->levels = args.levels;
    res->layers = args.layers;
    res->depth = args.depth;
    res->compute = args.compute;
    res->mode = image_mode;
    res->image_barrier_count = image_barrier_count;
    res->attachment_count = attachment_count;
    res->output_count = output_count;

    res->image_array = (Image **)PyMem_Malloc(sizeof(Image *) * attachment_count);
    res->image_view_array = (VkImageView *)PyMem_Malloc(sizeof(VkImageView) * attachment_count * args.layers);
    res->framebuffer_array = (VkFramebuffer *)PyMem_Malloc(sizeof(VkFramebuffer) * args.layers);
    res->description_array = (VkAttachmentDescription *)PyMem_Malloc(sizeof(VkAttachmentDescription) * attachment_count);
    res->reference_array = (VkAttachmentReference *)PyMem_Malloc(sizeof(VkAttachmentReference) * attachment_count);
    res->clear_value_array = (VkClearValue *)PyMem_Malloc(sizeof(VkClearValue) * attachment_count);
    res->image_barrier_array = (VkImageMemoryBarrier *)PyMem_Malloc(sizeof(VkImageMemoryBarrier) * image_barrier_count);

    res->render_pipeline_list = PyList_New(0);
    res->compute_pipeline_list = PyList_New(0);

    for (uint32_t i = 0; i < output_count; ++i) {
        Format format = get_format(PyList_GetItem(format_list, i));
        res->image_array[i] = new_image({
            self,
            memory,
            args.width * args.height * args.layers * format.size,
            image_usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            {args.width, args.height, 1},
            VK_SAMPLE_COUNT_1_BIT,
            args.levels,
            args.layers,
            image_mode,
            format.format,
        });
    }

    if (args.depth) {
        res->image_array[output_count] = new_image({
            self,
            memory,
            0,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            {args.width, args.height, 1},
            args.samples,
            1,
            args.layers,
            IMG_PROTECTED,
            self->depth_format,
        });
    }

    if (args.samples > 1) {
        for (uint32_t i = 0; i < output_count; ++i) {
            Format format = get_format(PyList_GetItem(format_list, i));
            res->image_array[attachment_count - output_count + i] = new_image({
                self,
                memory,
                0,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                {args.width, args.height, 1},
                args.samples,
                1,
                args.layers,
                IMG_PROTECTED,
                format.format,
            });
        }
    }

    allocate_memory(memory);

    for (uint32_t i = 0; i < attachment_count; ++i) {
        bind_image(res->image_array[i]);
    }

    VkSubpassDescription subpass_description = {
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0,
        NULL,
        output_count,
        res->reference_array,
        NULL,
        NULL,
        0,
        NULL,
    };

    for (uint32_t i = 0; i < attachment_count; ++i) {
        res->description_array[i] = {
            0,
            res->image_array[i]->format,
            (VkSampleCountFlagBits)res->image_array[i]->samples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        res->reference_array[i] = {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    }

    for (uint32_t i = 0; i < output_count; ++i) {
        if (args.compute) {
            res->description_array[i].finalLayout = VK_IMAGE_LAYOUT_GENERAL;
        } else {
            res->description_array[i].finalLayout = final_image_layout;
        }
    }

    if (args.depth) {
        res->reference_array[output_count].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        res->description_array[output_count].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        res->description_array[output_count].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        subpass_description.pDepthStencilAttachment = res->reference_array + output_count;
    }

    if (args.samples > 1) {
        for (uint32_t i = 0; i < output_count; ++i) {
            res->description_array[attachment_count - output_count + i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            res->description_array[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        subpass_description.pColorAttachments = res->reference_array + attachment_count - output_count;
        subpass_description.pResolveAttachments = res->reference_array;
    }

    VkSubpassDependency subpass_dependency = {
        VK_SUBPASS_EXTERNAL,
        0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        0,
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        NULL,
        0,
        attachment_count,
        res->description_array,
        1,
        &subpass_description,
        1,
        &subpass_dependency,
    };

    self->vkCreateRenderPass(res->instance->device, &render_pass_create_info, NULL, &res->render_pass);

    for (uint32_t layer = 0; layer < args.layers; ++layer) {
        uint32_t offset = attachment_count * layer;

        for (uint32_t i = 0; i < attachment_count; ++i) {
            VkImageViewCreateInfo image_view_create_info = {
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                NULL,
                0,
                res->image_array[i]->image,
                VK_IMAGE_VIEW_TYPE_2D,
                res->image_array[i]->format,
                {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
                {res->image_array[i]->aspect, 0, 1, layer, 1},
            };

            VkImageView image_view = NULL;
            self->vkCreateImageView(res->instance->device, &image_view_create_info, NULL, &image_view);
            res->image_view_array[offset + i] = image_view;
        }

        VkFramebufferCreateInfo framebuffer_create_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            NULL,
            0,
            res->render_pass,
            attachment_count,
            res->image_view_array + offset,
            args.width,
            args.height,
            1,
        };

        self->vkCreateFramebuffer(res->instance->device, &framebuffer_create_info, NULL, &res->framebuffer_array[layer]);
    }

    memset(res->clear_value_array, 0, sizeof(VkClearValue) * attachment_count);

    for (uint32_t i = 0; i < image_barrier_count; ++i) {
        res->image_barrier_array[i] = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            NULL,
            0,
            0,
            VK_IMAGE_LAYOUT_GENERAL,
            final_image_layout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            res->image_array[i]->image,
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, args.layers},
        };
    }

    if (args.depth) {
        res->clear_value_array[output_count] = {1.0f, 0};
    }

    res->output = PyTuple_New(output_count);
    for (uint32_t i = 0; i < output_count; ++i) {
        PyTuple_SetItem(res->output, i, (PyObject *)res->image_array[i]);
        Py_INCREF(res->image_array[i]);
    }

    PyList_Append(self->task_list, (PyObject *)res);
    return res;
}

void execute_framebuffer(Framebuffer * self, VkCommandBuffer command_buffer) {
    for (uint32_t layer = 0; layer < self->layers; ++layer) {
        VkRenderPassBeginInfo render_pass_begin_info = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            NULL,
            self->render_pass,
            self->framebuffer_array[layer],
            {{0, 0}, {self->width, self->height}},
            self->attachment_count,
            self->clear_value_array,
        };

        self->instance->vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {0.0f, 0.0f, (float)self->width, (float)self->height, 0.0f, 1.0f};
        self->instance->vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {{0, 0}, {self->width, self->height}};
        self->instance->vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        for (uint32_t i = 0; i < PyList_GET_SIZE(self->render_pipeline_list); ++i) {
            RenderPipeline * pipeline = (RenderPipeline *)PyList_GET_ITEM(self->render_pipeline_list, i);

            self->instance->vkCmdPushConstants(
                command_buffer,
                pipeline->pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                4,
                &layer
            );

            execute_render_pipeline(pipeline, command_buffer);
        }

        self->instance->vkCmdEndRenderPass(command_buffer);

        for (uint32_t i = 0; i < PyList_GET_SIZE(self->compute_pipeline_list); ++i) {
            ComputePipeline * pipeline = (ComputePipeline *)PyList_GET_ITEM(self->compute_pipeline_list, i);
            execute_compute_pipeline(pipeline, command_buffer);
        }

        if (self->image_barrier_count) {
            self->instance->vkCmdPipelineBarrier(
                command_buffer,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,
                0,
                NULL,
                0,
                NULL,
                self->image_barrier_count,
                self->image_barrier_array
            );

        }
    }

    if (self->levels > 1) {
        for (uint32_t i = 0; i < self->output_count; ++i) {
            VkImageMemoryBarrier image_barrier = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                self->image_array[i]->image,
                {VK_IMAGE_ASPECT_COLOR_BIT, 1, self->levels - 1, 0, self->layers},
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
        }

        build_mipmaps({
            self->instance,
            command_buffer,
            self->width,
            self->height,
            self->levels,
            self->layers,
            self->output_count,
            self->image_array,
        });
    }
}

PyObject * Framebuffer_meth_update(Framebuffer * self, PyObject * vargs, PyObject * kwargs) {
    if (PyTuple_Size(vargs) || !kwargs) {
        PyErr_Format(PyExc_TypeError, "invalid arguments");
    }

    Py_ssize_t pos = 0;
    PyObject * key = NULL;
    PyObject * value = NULL;

    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        if (!PyUnicode_CompareWithASCIIString(key, "clear_values")) {
            Py_buffer view = {};
            if (PyObject_GetBuffer(value, &view, PyBUF_SIMPLE)) {
                return NULL;
            }
            Py_ssize_t max_size = self->output_count * sizeof(VkClearValue);
            if (view.len > max_size) {
                PyBuffer_Release(&view);
                return NULL;
            }
            VkClearValue * ptr = self->clear_value_array;
            if (self->samples > 1) {
                ptr = self->clear_value_array + self->attachment_count - self->output_count;
            }
            PyBuffer_ToContiguous(ptr, &view, view.len, 'C');
            PyBuffer_Release(&view);
            continue;
        }
        if (!PyUnicode_CompareWithASCIIString(key, "clear_depth")) {
            if (!self->depth) {
                PyErr_Format(PyExc_ValueError, "clear_depth");
                return NULL;
            }
            self->clear_value_array[self->output_count].depthStencil.depth = (float)PyFloat_AsDouble(value);
            if (PyErr_Occurred()) {
                return NULL;
            }
            continue;
        }
    }

    Py_RETURN_NONE;
}
