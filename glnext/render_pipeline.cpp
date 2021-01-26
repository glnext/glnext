#include "glnext.hpp"

RenderPipeline * Framebuffer_meth_render(Framebuffer * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_shader",
        "fragment_shader",
        "vertex_format",
        "instance_format",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        "topology",
        "buffers",
        "images",
        "memory",
        NULL,
    };

    struct {
        PyObject * vertex_shader = Py_None;
        PyObject * fragment_shader = Py_None;
        PyObject * vertex_format;
        PyObject * instance_format;
        uint32_t vertex_count = 0;
        uint32_t instance_count = 1;
        uint32_t index_count = 0;
        uint32_t indirect_count = 0;
        PyObject * topology;
        PyObject * buffers;
        PyObject * images;
        PyObject * memory = Py_None;
    } args;

    args.vertex_format = self->instance->state->empty_str;
    args.instance_format = self->instance->state->empty_str;
    args.topology = self->instance->state->default_topology;
    args.buffers = self->instance->state->empty_list;
    args.images = self->instance->state->empty_list;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$O!O!OOIIIIOOOO",
        keywords,
        &PyBytes_Type,
        &args.vertex_shader,
        &PyBytes_Type,
        &args.fragment_shader,
        &args.vertex_format,
        &args.instance_format,
        &args.vertex_count,
        &args.instance_count,
        &args.index_count,
        &args.indirect_count,
        &args.topology,
        &args.buffers,
        &args.images,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    Memory * memory = get_memory(self->instance, args.memory);

    RenderPipeline * res = PyObject_New(RenderPipeline, self->instance->state->RenderPipeline_type);

    res->instance = self->instance;
    res->vertex_count = args.vertex_count;
    res->instance_count = args.instance_count;
    res->index_count = args.index_count;
    res->indirect_count = args.indirect_count;

    PyObject * vertex_format = PyUnicode_Split(args.vertex_format, NULL, -1);
    PyObject * instance_format = PyUnicode_Split(args.instance_format, NULL, -1);

    uint32_t attribute_count = 0;
    VkVertexInputAttributeDescription attribute_array[64];
    VkVertexInputBindingDescription binding_array[64];

    uint32_t vstride = 0;
    uint32_t istride = 0;

    for (uint32_t i = 0; i < PyList_Size(vertex_format); ++i) {
        Format format = get_format(PyList_GetItem(vertex_format, i));
        if (format.format) {
            uint32_t location = attribute_count++;
            attribute_array[location] = {location, location, format.format, vstride};
        }
        vstride += format.size;
    }

    uint32_t vertex_attribute_count = attribute_count;

    for (uint32_t i = 0; i < PyList_Size(instance_format); ++i) {
        Format format = get_format(PyList_GetItem(instance_format, i));
        if (format.format) {
            uint32_t location = attribute_count++;
            attribute_array[location] = {location, location, format.format, istride};
        }
        istride += format.size;
    }

    for (uint32_t i = 0; i < vertex_attribute_count; ++i) {
        binding_array[i] = {i, vstride, VK_VERTEX_INPUT_RATE_VERTEX};
    }

    for (uint32_t i = vertex_attribute_count; i < attribute_count; ++i) {
        binding_array[i] = {i, istride, VK_VERTEX_INPUT_RATE_INSTANCE};
    }

    VkBool32 short_index = false;
    uint32_t index_size = short_index ? 2 : 4;
    uint32_t indirect_size = args.index_count ? 20 : 16;

    res->buffer_count = (uint32_t)PyList_Size(args.buffers);
    res->image_count = (uint32_t)PyList_Size(args.images);

    res->buffer_array = (BufferBinding *)PyMem_Malloc(sizeof(BufferBinding) * PyList_Size(args.buffers));
    res->image_array = (ImageBinding *)PyMem_Malloc(sizeof(ImageBinding) * PyList_Size(args.images));

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        BufferBinding buffer_binding = parse_buffer_binding(PyList_GetItem(args.buffers, i));
        if (!buffer_binding.buffer) {
            buffer_binding.buffer = new_buffer({
                self->instance,
                memory,
                buffer_binding.size,
                buffer_binding.usage,
            });
        }
        res->buffer_array[i] = buffer_binding;
    }

    for (uint32_t i = 0; i < res->image_count; ++i) {
        res->image_array[i] = parse_image_binding(PyList_GetItem(args.images, i));
    }

    res->vertex_buffer = NULL;
    res->instance_buffer = NULL;
    res->index_buffer = NULL;
    res->indirect_buffer = NULL;

    if (vstride * args.vertex_count) {
        res->vertex_buffer = new_buffer({
            self->instance,
            memory,
            vstride * args.vertex_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        });
    }

    if (istride * args.instance_count) {
        res->instance_buffer = new_buffer({
            self->instance,
            memory,
            istride * args.instance_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        });
    }

    if (args.index_count) {
        res->index_buffer = new_buffer({
            self->instance,
            memory,
            args.index_count * index_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        });
    }

    if (args.indirect_count) {
        res->indirect_buffer = new_buffer({
            self->instance,
            memory,
            args.indirect_count * indirect_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        });
    }

    allocate_memory(memory);

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        bind_buffer(res->buffer_array[i].buffer);
    }

    if (res->vertex_buffer) {
        bind_buffer(res->vertex_buffer);
    }

    if (res->instance_buffer) {
        bind_buffer(res->instance_buffer);
    }

    if (res->index_buffer) {
        bind_buffer(res->index_buffer);
    }

    if (res->indirect_buffer) {
        bind_buffer(res->indirect_buffer);
    }

    for (uint32_t i = 0; i < res->image_count; ++i) {
        ImageBinding & image_binding = res->image_array[i];
        for (uint32_t j = 0; j < image_binding.image_count; ++j) {
            VkImageView image_view = NULL;
            vkCreateImageView(
                self->instance->device,
                &image_binding.image_view_create_info_array[j],
                NULL,
                &image_view
            );
            VkSampler sampler = NULL;
            if (image_binding.sampled) {
                vkCreateSampler(
                    self->instance->device,
                    &image_binding.sampler_create_info_array[j],
                    NULL,
                    &sampler
                );
            }
            image_binding.sampler_array[j] = sampler;
            image_binding.image_view_array[j] = image_view;
            image_binding.descriptor_image_info_array[j] = {
                sampler,
                image_view,
                image_binding.layout,
            };
        }
    }

    res->vertex_attribute_count = vertex_attribute_count;
    res->vertex_attribute_buffer_array = (VkBuffer *)PyMem_Malloc(sizeof(VkBuffer) * vertex_attribute_count);
    res->vertex_attribute_offset_array = (VkDeviceSize *)PyMem_Malloc(sizeof(VkDeviceSize) * vertex_attribute_count);

    for (uint32_t i = 0; i < vertex_attribute_count; ++i) {
        res->vertex_attribute_buffer_array[i] = res->vertex_buffer->buffer;
        res->vertex_attribute_offset_array[i] = 0;
    }

    for (uint32_t i = vertex_attribute_count; i < attribute_count; ++i) {
        res->vertex_attribute_buffer_array[i] = res->instance_buffer->buffer;
        res->vertex_attribute_offset_array[i] = 0;
    }

    uint32_t descriptor_binding_count = 0;
    VkDescriptorSetLayoutBinding descriptor_binding_array[64];
    VkDescriptorPoolSize descriptor_pool_size_array[64];
    VkDescriptorBufferInfo descriptor_buffer_info_array[64];
    VkWriteDescriptorSet write_descriptor_set_array[64];

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        uint32_t binding = descriptor_binding_count++;
        descriptor_binding_array[binding] = {
            binding,
            res->buffer_array[i].descriptor_type,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        };
        descriptor_pool_size_array[binding] = {
            res->buffer_array[i].descriptor_type,
            1,
        };
        descriptor_buffer_info_array[binding] = {
            res->buffer_array[i].buffer->buffer,
            0,
            VK_WHOLE_SIZE,
        };
        write_descriptor_set_array[binding] = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            NULL,
            binding,
            0,
            1,
            res->buffer_array[i].descriptor_type,
            NULL,
            &descriptor_buffer_info_array[binding],
            NULL,
        };
    }

    for (uint32_t i = 0; i < res->image_count; ++i) {
        uint32_t binding = descriptor_binding_count++;
        descriptor_binding_array[binding] = {
            binding,
            res->image_array[i].descriptor_type,
            1,
            VK_SHADER_STAGE_COMPUTE_BIT,
            NULL,
        };
        descriptor_pool_size_array[binding] = {
            res->image_array[i].descriptor_type,
            1,
        };
        write_descriptor_set_array[binding] = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            NULL,
            binding,
            0,
            1,
            res->image_array[i].descriptor_type,
            res->image_array[i].descriptor_image_info_array,
            NULL,
            NULL,
        };
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        NULL,
        0,
        descriptor_binding_count,
        descriptor_binding_array,
    };

    vkCreateDescriptorSetLayout(self->instance->device, &descriptor_set_layout_create_info, NULL, &res->descriptor_set_layout);

    VkPushConstantRange push_constant_range = {
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        4,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        NULL,
        0,
        1,
        &res->descriptor_set_layout,
        1,
        &push_constant_range,
    };

    vkCreatePipelineLayout(self->instance->device, &pipeline_layout_create_info, NULL, &res->pipeline_layout);

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        NULL,
        0,
        1,
        descriptor_binding_count,
        descriptor_pool_size_array,
    };

    vkCreateDescriptorPool(self->instance->device, &descriptor_pool_create_info, NULL, &res->descriptor_pool);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        NULL,
        res->descriptor_pool,
        1,
        &res->descriptor_set_layout,
    };

    vkAllocateDescriptorSets(self->instance->device, &descriptor_set_allocate_info, &res->descriptor_set);

    for (uint32_t i = 0; i < descriptor_binding_count; ++i) {
        write_descriptor_set_array[i].dstSet = res->descriptor_set;
    }

    vkUpdateDescriptorSets(
        self->instance->device,
        descriptor_binding_count,
        write_descriptor_set_array,
        NULL,
        NULL
    );

    VkShaderModuleCreateInfo vertex_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.vertex_shader),
        (uint32_t *)PyBytes_AsString(args.vertex_shader),
    };

    vkCreateShaderModule(self->instance->device, &vertex_shader_module_create_info, NULL, &res->vertex_shader_module);

    VkShaderModuleCreateInfo fragment_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.fragment_shader),
        (uint32_t *)PyBytes_AsString(args.fragment_shader),
    };

    vkCreateShaderModule(self->instance->device, &fragment_shader_module_create_info, NULL, &res->fragment_shader_module);

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_array[] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            NULL,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            res->vertex_shader_module,
            "main",
            NULL,
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            NULL,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            res->fragment_shader_module,
            "main",
            NULL,
        },
    };

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        NULL,
        0,
        attribute_count,
        binding_array,
        attribute_count,
        attribute_array,
    };

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        NULL,
        0,
        get_topology(args.topology),
        false,
    };

    VkPipelineViewportStateCreateInfo pipeline_viewport_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        NULL,
        0,
        1,
        NULL,
        1,
        NULL,
    };

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        NULL,
        0,
        false,
        false,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        false,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        NULL,
        0,
        (VkSampleCountFlagBits)self->samples,
        false,
        0.0f,
        NULL,
        false,
        false,
    };

    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        NULL,
        0,
        true,
        true,
        VK_COMPARE_OP_LESS,
        false,
        false,
        {},
        {},
        0.0f,
        0.0f,
    };

    uint32_t color_attachment_count = (uint32_t)PyTuple_Size(self->output);
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_array[64];

    for (uint32_t i = 0; i < color_attachment_count; ++i) {
        pipeline_color_blend_attachment_array[i] = {
            false,
            VK_BLEND_FACTOR_SRC_ALPHA,
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_FACTOR_ZERO,
            VK_BLEND_OP_ADD,
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
    }

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        NULL,
        0,
        false,
        VK_LOGIC_OP_CLEAR,
        color_attachment_count,
        pipeline_color_blend_attachment_array,
        {0.0f, 0.0f, 0.0f, 0.0f},
    };

    VkDynamicState dynamic_state_array[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        NULL,
        0,
        2,
        dynamic_state_array,
    };

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        NULL,
        0,
        2,
        pipeline_shader_stage_array,
        &pipeline_vertex_input_state,
        &pipeline_input_assembly_state,
        NULL,
        &pipeline_viewport_state,
        &pipeline_rasterization_state,
        &pipeline_multisample_state,
        &pipeline_depth_stencil_state,
        &pipeline_color_blend_state,
        &pipeline_dynamic_state,
        res->pipeline_layout,
        self->render_pass,
        0,
        NULL,
        0,
    };

    vkCreateGraphicsPipelines(self->instance->device, NULL, 1, &graphics_pipeline_create_info, NULL, &res->pipeline);

    res->buffer = PyTuple_New(res->buffer_count);
    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        PyTuple_SetItem(res->buffer, i, (PyObject *)res->buffer_array[i].buffer);
        Py_INCREF(res->buffer_array[i].buffer);
    }

    PyList_Append(self->render_pipeline_list, (PyObject *)res);
    return res;
}

void execute_render_pipeline(VkCommandBuffer command_buffer, RenderPipeline * pipeline) {
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    if (pipeline->vertex_attribute_count) {
        vkCmdBindVertexBuffers(
            command_buffer,
            0,
            pipeline->vertex_attribute_count,
            pipeline->vertex_attribute_buffer_array,
            pipeline->vertex_attribute_offset_array
        );
    }

    if (pipeline->descriptor_set) {
        vkCmdBindDescriptorSets(
            command_buffer,
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
        vkCmdBindIndexBuffer(command_buffer, pipeline->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    if (pipeline->indirect_count && pipeline->index_count) {
        vkCmdDrawIndexedIndirect(command_buffer, pipeline->indirect_buffer->buffer, 0, pipeline->indirect_count, 20);
    } else if (pipeline->indirect_count) {
        vkCmdDrawIndirect(command_buffer, pipeline->indirect_buffer->buffer, 0, pipeline->indirect_count, 16);
    } else if (pipeline->index_count) {
        vkCmdDrawIndexed(command_buffer, pipeline->index_count, pipeline->instance_count, 0, 0, 0);
    } else {
        vkCmdDraw(command_buffer, pipeline->vertex_count, pipeline->instance_count, 0, 0);
    }
}