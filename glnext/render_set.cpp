#include "glnext.hpp"

RenderSet * Instance_meth_render_set(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "size",
        "format",
        "samples",
        "levels",
        "layers",
        "depth",
        "uniform_buffer",
        "storage_buffer",
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
        VkDeviceSize uniform_buffer_size = 0;
        VkDeviceSize storage_buffer_size = 0;
        PyObject * mode;
        PyObject * memory = Py_None;
    } args;

    args.format = self->state->default_format;
    args.mode = self->state->output_str;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "(II)|O!$IIIpkkOO",
        keywords,
        &args.width,
        &args.height,
        &PyUnicode_Type,
        &args.format,
        &args.samples,
        &args.levels,
        &args.layers,
        &args.depth,
        &args.uniform_buffer_size,
        &args.storage_buffer_size,
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

    if (args.depth) {
        attachment_count += 1;
    }

    if (args.samples) {
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

    RenderSet * res = PyObject_New(RenderSet, self->state->RenderSet_type);

    res->instance = self;
    res->width = args.width;
    res->height = args.height;
    res->samples = args.samples;
    res->levels = args.levels;
    res->layers = args.layers;
    res->depth = args.depth;
    res->attachment_count = attachment_count;
    res->output_count = output_count;

    res->image_array = (Image **)PyMem_Malloc(sizeof(Image *) * attachment_count);
    res->image_view_array = (VkImageView *)PyMem_Malloc(sizeof(VkImageView) * attachment_count * args.layers);
    res->framebuffer_array = (VkFramebuffer *)PyMem_Malloc(sizeof(VkFramebuffer) * args.layers);
    res->description_array = (VkAttachmentDescription *)PyMem_Malloc(sizeof(VkAttachmentDescription) * attachment_count);
    res->reference_array = (VkAttachmentReference *)PyMem_Malloc(sizeof(VkAttachmentReference) * attachment_count);
    res->clear_value_array = (VkClearValue *)PyMem_Malloc(sizeof(VkClearValue) * attachment_count);

    res->pipeline_list = PyList_New(0);

    res->uniform_buffer = NULL;
    if (args.uniform_buffer_size) {
        BufferCreateInfo buffer_create_info = {
            self,
            memory,
            args.uniform_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };
        res->uniform_buffer = new_buffer(&buffer_create_info);
    }

    res->storage_buffer = NULL;
    if (args.storage_buffer_size) {
        BufferCreateInfo buffer_create_info = {
            self,
            memory,
            args.storage_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        };
        res->storage_buffer = new_buffer(&buffer_create_info);
    }

    for (uint32_t i = 0; i < output_count; ++i) {
        Format format = get_format(PyList_GetItem(format_list, i));
        ImageCreateInfo image_create_info = {
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
        };
        res->image_array[i] = new_image(&image_create_info);
    }

    if (args.depth) {
        ImageCreateInfo image_create_info = {
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
        };
        res->image_array[output_count] = new_image(&image_create_info);
    }

    if (args.samples) {
        for (uint32_t i = 0; i < output_count; ++i) {
            Format format = get_format(PyList_GetItem(format_list, i));
            ImageCreateInfo image_create_info = {
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
            };
            res->image_array[attachment_count - output_count + i] = new_image(&image_create_info);
        }
    }

    allocate_memory(memory);

    if (res->uniform_buffer) {
        bind_buffer(res->uniform_buffer);
    }

    if (res->storage_buffer) {
        bind_buffer(res->storage_buffer);
    }

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
        res->description_array[i].finalLayout = final_image_layout;
    }

    if (args.depth) {
        res->reference_array[output_count].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        res->description_array[output_count].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        res->description_array[output_count].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        subpass_description.pDepthStencilAttachment = res->reference_array + output_count;
    }

    if (args.samples) {
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

    vkCreateRenderPass(res->instance->device, &render_pass_create_info, NULL, &res->render_pass);

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
            vkCreateImageView(res->instance->device, &image_view_create_info, NULL, &image_view);
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

        vkCreateFramebuffer(res->instance->device, &framebuffer_create_info, NULL, &res->framebuffer_array[layer]);
    }

    memset(res->clear_value_array, 0, sizeof(VkClearValue) * attachment_count);

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

PyObject * RenderSet_meth_update(RenderSet * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"size", "uniform_buffer", "storage_buffer", "clear_color", "clear_depth", NULL};

    struct {
        uint32_t width;
        uint32_t height;
        PyObject * uniform_buffer = Py_None;
        PyObject * storage_buffer = Py_None;
        PyObject * clear_color = Py_None;
        PyObject * clear_depth = Py_None;
    } args;

    args.width = self->width;
    args.height = self->height;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$(II)OOOO",
        keywords,
        &args.width,
        &args.height,
        &args.uniform_buffer,
        &args.storage_buffer,
        &args.clear_color,
        &args.clear_depth
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.uniform_buffer != Py_None) {
        PyObject * res = Buffer_meth_write(self->uniform_buffer, args.uniform_buffer);
        Py_XDECREF(res);
        if (!res) {
            return NULL;
        }
    }

    if (args.storage_buffer != Py_None) {
        PyObject * res = Buffer_meth_write(self->storage_buffer, args.storage_buffer);
        Py_XDECREF(res);
        if (!res) {
            return NULL;
        }
    }

    if (args.clear_depth != Py_None && self->depth) {
        if (!PyFloat_Check(args.clear_depth)) {
            PyErr_Format(PyExc_TypeError, "expected float");
            return NULL;
        }
        self->clear_value_array[self->output_count].depthStencil.depth = (float)PyFloat_AsDouble(args.clear_depth);
    }

    if (args.clear_color != Py_None) {
        Py_buffer view = {};
        if (PyObject_GetBuffer(args.clear_color, &view, PyBUF_STRIDED_RO)) {
            return NULL;
        }

        if (view.len != sizeof(VkClearValue) * self->output_count) {
            PyBuffer_Release(&view);
            PyErr_Format(PyExc_ValueError, "wrong size");
            return NULL;
        }

        void * ptr = self->clear_value_array;
        if (self->samples) {
            ptr = self->clear_value_array + self->attachment_count - self->output_count;
        }

        PyBuffer_ToContiguous(ptr, &view, view.len, 'C');
        PyBuffer_Release(&view);
    }

    Py_RETURN_NONE;
}

void execute_render_set(VkCommandBuffer cmd, RenderSet * render_set) {
    for (uint32_t layer = 0; layer < render_set->layers; ++layer) {
        VkRenderPassBeginInfo render_pass_begin_info = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            NULL,
            render_set->render_pass,
            render_set->framebuffer_array[layer],
            {{0, 0}, {render_set->width, render_set->height}},
            render_set->attachment_count,
            render_set->clear_value_array,
        };

        vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {0.0f, 0.0f, (float)render_set->width, (float)render_set->height, 0.0f, 1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {{0, 0}, {render_set->width, render_set->height}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        for (uint32_t i = 0; i < PyList_Size(render_set->pipeline_list); ++i) {
            Pipeline * pipeline = (Pipeline *)PyList_GetItem(render_set->pipeline_list, i);

            VkBuffer * vertex_buffer_array = NULL;
            uint32_t vertex_buffer_count = retreive_array(pipeline->vertex_buffers, &vertex_buffer_array);
            const VkDeviceSize vertex_buffer_offset_array[64] = {};

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

            if (vertex_buffer_count) {
                vkCmdBindVertexBuffers(
                    cmd,
                    0,
                    vertex_buffer_count,
                    vertex_buffer_array,
                    vertex_buffer_offset_array
                );
            }

            if (pipeline->descriptor_set) {
                vkCmdBindDescriptorSets(
                    cmd,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->pipeline_layout,
                    0,
                    1,
                    &pipeline->descriptor_set,
                    0,
                    NULL
                );
            }

            if (pipeline->index_buffer) {
                vkCmdBindIndexBuffer(cmd, pipeline->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            }

            vkCmdPushConstants(
                cmd,
                pipeline->pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                4,
                &layer
            );

            if (pipeline->indirect_count && pipeline->index_count) {
                vkCmdDrawIndexedIndirect(cmd, pipeline->indirect_buffer->buffer, 0, pipeline->indirect_count, 20);
            } else if (pipeline->indirect_count) {
                vkCmdDrawIndirect(cmd, pipeline->indirect_buffer->buffer, 0, pipeline->indirect_count, 16);
            } else if (pipeline->index_count) {
                vkCmdDrawIndexed(cmd, pipeline->index_count, pipeline->instance_count, 0, 0, 0);
            } else {
                vkCmdDraw(cmd, pipeline->vertex_count, pipeline->instance_count, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd);
    }

    if (render_set->levels > 1) {
        build_mipmaps({
            cmd,
            render_set->width,
            render_set->height,
            render_set->levels,
            render_set->layers,
            render_set->output_count,
            render_set->image_array,
        });
    }
}
