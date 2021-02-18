#include "glnext.hpp"

Buffer * get_buffer(Instance * instance, PyObject * obj) {
    if (obj == Py_None) {
        return NULL;
    }
    if (Py_TYPE(obj) == instance->state->Buffer_type) {
        return (Buffer *)obj;
    }
    PyErr_Format(PyExc_ValueError, "invalid buffer");
    return NULL;
}

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
        "vertex_buffer",
        "instance_buffer",
        "index_buffer",
        "indirect_buffer",
        "topology",
        "bindings",
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
        PyObject * vertex_buffer = Py_None;
        PyObject * instance_buffer = Py_None;
        PyObject * index_buffer = Py_None;
        PyObject * indirect_buffer = Py_None;
        PyObject * topology;
        PyObject * bindings;
        PyObject * memory = Py_None;
    } args;

    args.vertex_format = self->instance->state->empty_str;
    args.instance_format = self->instance->state->empty_str;
    args.topology = self->instance->state->default_topology;
    args.bindings = self->instance->state->empty_list;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$O!O!OOIIIIOOOOOOO",
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
        &args.vertex_buffer,
        &args.instance_buffer,
        &args.index_buffer,
        &args.indirect_buffer,
        &args.topology,
        &args.bindings,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    Memory * memory = get_memory(self->instance, args.memory);

    RenderPipeline * res = PyObject_New(RenderPipeline, self->instance->state->RenderPipeline_type);

    res->instance = self->instance;
    res->members = PyDict_New();

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

    res->parameters = {
        true,
        args.vertex_count,
        args.instance_count,
        args.index_count,
        args.indirect_count,
    };

    VkBool32 short_index = false;
    uint32_t index_size = short_index ? 2 : 4;
    uint32_t indirect_size = args.index_count ? 20 : 16;

    res->binding_count = (uint32_t)PyList_Size(args.bindings);
    res->binding_array = (DescriptorBinding *)PyMem_Malloc(sizeof(DescriptorBinding) * PyList_Size(args.bindings));

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        if (parse_descriptor_binding(self->instance, &res->binding_array[i], PyList_GetItem(args.bindings, i))) {
            return NULL;
        }
    }

    res->vertex_buffer = get_buffer(self->instance, args.vertex_buffer);
    res->instance_buffer = get_buffer(self->instance, args.instance_buffer);
    res->index_buffer = get_buffer(self->instance, args.index_buffer);
    res->indirect_buffer = get_buffer(self->instance, args.indirect_buffer);

    if (PyErr_Occurred()) {
        return NULL;
    }

    if (vstride && !res->vertex_buffer) {
        res->vertex_buffer = new_buffer({
            self->instance,
            memory,
            vstride * args.vertex_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        });
    }

    if (istride && !res->instance_buffer) {
        res->instance_buffer = new_buffer({
            self->instance,
            memory,
            istride * args.instance_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        });
    }

    if (args.index_count && !res->index_buffer) {
        res->index_buffer = new_buffer({
            self->instance,
            memory,
            args.index_count * index_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        });
    }

    if (args.indirect_count && !res->indirect_buffer) {
        res->indirect_buffer = new_buffer({
            self->instance,
            memory,
            args.indirect_count * indirect_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        });
    }

    if (res->vertex_buffer) {
        PyDict_SetItemString(res->members, "vertex_buffer", (PyObject *)res->vertex_buffer);
    }

    if (res->instance_buffer) {
        PyDict_SetItemString(res->members, "instance_buffer", (PyObject *)res->instance_buffer);
    }

    if (res->index_buffer) {
        PyDict_SetItemString(res->members, "index_buffer", (PyObject *)res->index_buffer);
    }

    if (res->indirect_buffer) {
        PyDict_SetItemString(res->members, "indirect_buffer", (PyObject *)res->indirect_buffer);
    }

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        create_descriptor_binding_objects(self->instance, &res->binding_array[i], memory);
    }

    allocate_memory(memory);

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

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        bind_descriptor_binding_objects(self->instance, &res->binding_array[i]);
    }

    res->attribute_count = attribute_count;
    res->attribute_buffer_array = (VkBuffer *)PyMem_Malloc(sizeof(VkBuffer) * attribute_count);
    res->attribute_offset_array = (VkDeviceSize *)PyMem_Malloc(sizeof(VkDeviceSize) * attribute_count);

    for (uint32_t i = 0; i < vertex_attribute_count; ++i) {
        res->attribute_buffer_array[i] = res->vertex_buffer->buffer;
        res->attribute_offset_array[i] = 0;
    }

    for (uint32_t i = vertex_attribute_count; i < attribute_count; ++i) {
        res->attribute_buffer_array[i] = res->instance_buffer->buffer;
        res->attribute_offset_array[i] = 0;
    }

    res->descriptor_binding_array = (VkDescriptorSetLayoutBinding *)PyMem_Malloc(sizeof(VkDescriptorSetLayoutBinding) * res->binding_count);
    res->descriptor_pool_size_array = (VkDescriptorPoolSize *)PyMem_Malloc(sizeof(VkDescriptorPoolSize) * res->binding_count);
    res->write_descriptor_set_array = (VkWriteDescriptorSet *)PyMem_Malloc(sizeof(VkWriteDescriptorSet) * res->binding_count);

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        res->descriptor_binding_array[i] = res->binding_array[i].descriptor_binding;
        res->descriptor_pool_size_array[i] = res->binding_array[i].descriptor_pool_size;
        res->write_descriptor_set_array[i] = res->binding_array[i].write_descriptor_set;
    }

    VkPushConstantRange push_constant_range = {
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        4,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        NULL,
        0,
        0,
        NULL,
        1,
        &push_constant_range,
    };

    res->descriptor_set_layout = NULL;
    res->descriptor_pool = NULL;
    res->descriptor_set = NULL;

    if (res->binding_count) {
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            NULL,
            0,
            res->binding_count,
            res->descriptor_binding_array,
        };

        self->instance->vkCreateDescriptorSetLayout(self->instance->device, &descriptor_set_layout_create_info, NULL, &res->descriptor_set_layout);

        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &res->descriptor_set_layout;

        VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            NULL,
            0,
            1,
            res->binding_count,
            res->descriptor_pool_size_array,
        };

        self->instance->vkCreateDescriptorPool(self->instance->device, &descriptor_pool_create_info, NULL, &res->descriptor_pool);

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            NULL,
            res->descriptor_pool,
            1,
            &res->descriptor_set_layout,
        };

        self->instance->vkAllocateDescriptorSets(self->instance->device, &descriptor_set_allocate_info, &res->descriptor_set);

        for (uint32_t i = 0; i < res->binding_count; ++i) {
            res->write_descriptor_set_array[i].dstSet = res->descriptor_set;
        }

        self->instance->vkUpdateDescriptorSets(
            self->instance->device,
            res->binding_count,
            res->write_descriptor_set_array,
            NULL,
            NULL
        );
    }

    self->instance->vkCreatePipelineLayout(self->instance->device, &pipeline_layout_create_info, NULL, &res->pipeline_layout);

    VkShaderModuleCreateInfo vertex_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.vertex_shader),
        (uint32_t *)PyBytes_AsString(args.vertex_shader),
    };

    self->instance->vkCreateShaderModule(self->instance->device, &vertex_shader_module_create_info, NULL, &res->vertex_shader_module);

    VkShaderModuleCreateInfo fragment_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.fragment_shader),
        (uint32_t *)PyBytes_AsString(args.fragment_shader),
    };

    self->instance->vkCreateShaderModule(self->instance->device, &fragment_shader_module_create_info, NULL, &res->fragment_shader_module);

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

    self->instance->vkCreateGraphicsPipelines(self->instance->device, NULL, 1, &graphics_pipeline_create_info, NULL, &res->pipeline);

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        if (res->binding_array[i].name) {
            if (res->binding_array[i].is_buffer) {
                PyDict_SetItem(res->members, res->binding_array[i].name, (PyObject *)res->binding_array[i].buffer.buffer);
            }
        }
    }

    PyList_Append(self->render_pipeline_list, (PyObject *)res);
    return res;
}

PyObject * RenderPipeline_meth_update(RenderPipeline * self, PyObject * vargs, PyObject * kwargs) {
    if (PyTuple_Size(vargs) || !kwargs) {
        PyErr_Format(PyExc_TypeError, "invalid arguments");
    }

    Py_ssize_t pos = 0;
    PyObject * key = NULL;
    PyObject * value = NULL;

    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        if (!PyUnicode_CompareWithASCIIString(key, "vertex_count")) {
            self->parameters.vertex_count = PyLong_AsUnsignedLong(value);
            if (PyErr_Occurred()) {
                return NULL;
            }
            continue;
        }
        if (!PyUnicode_CompareWithASCIIString(key, "instance_count")) {
            self->parameters.instance_count = PyLong_AsUnsignedLong(value);
            if (PyErr_Occurred()) {
                return NULL;
            }
            continue;
        }
        if (!PyUnicode_CompareWithASCIIString(key, "index_count")) {
            self->parameters.index_count = PyLong_AsUnsignedLong(value);
            if (PyErr_Occurred()) {
                return NULL;
            }
            continue;
        }
        if (!PyUnicode_CompareWithASCIIString(key, "indirect_count")) {
            self->parameters.indirect_count = PyLong_AsUnsignedLong(value);
            if (PyErr_Occurred()) {
                return NULL;
            }
            continue;
        }
        PyObject * member = PyDict_GetItem(self->members, key);
        if (!member) {
            return NULL;
        }
        PyObject * res = PyObject_CallMethod(member, "write", "O", value);
        if (!res) {
            return NULL;
        }
        Py_DECREF(res);
    }

    Py_RETURN_NONE;
}

void execute_render_pipeline(RenderPipeline * self) {
    if (!self->parameters.enabled) {
        return;
    }

    self->instance->vkCmdBindPipeline(self->instance->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->pipeline);

    if (self->attribute_count) {
        self->instance->vkCmdBindVertexBuffers(
            self->instance->command_buffer,
            0,
            self->attribute_count,
            self->attribute_buffer_array,
            self->attribute_offset_array
        );
    }

    if (self->descriptor_set) {
        self->instance->vkCmdBindDescriptorSets(
            self->instance->command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            self->pipeline_layout,
            0,
            1,
            &self->descriptor_set,
            0,
            NULL
        );
    }

    if (self->index_buffer) {
        self->instance->vkCmdBindIndexBuffer(
            self->instance->command_buffer,
            self->index_buffer->buffer,
            0,
            VK_INDEX_TYPE_UINT32
        );
    }

    if (self->indirect_buffer && self->index_buffer) {
        self->instance->vkCmdDrawIndexedIndirect(self->instance->command_buffer, self->indirect_buffer->buffer, 0, self->parameters.indirect_count, 20);
    } else if (self->indirect_buffer) {
        self->instance->vkCmdDrawIndirect(self->instance->command_buffer, self->indirect_buffer->buffer, 0, self->parameters.indirect_count, 16);
    } else if (self->index_buffer) {
        self->instance->vkCmdDrawIndexed(self->instance->command_buffer, self->parameters.index_count, self->parameters.instance_count, 0, 0, 0);
    } else {
        self->instance->vkCmdDraw(self->instance->command_buffer, self->parameters.vertex_count, self->parameters.instance_count, 0, 0);
    }
}

PyObject * RenderPipeline_subscript(RenderPipeline * self, PyObject * key) {
    return PyObject_GetItem(self->members, key);
}
