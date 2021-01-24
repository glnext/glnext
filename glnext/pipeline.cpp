#include "glnext.hpp"

Pipeline * RenderSet_meth_pipeline(RenderSet * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_shader",
        "fragment_shader",
        "vertex_format",
        "instance_format",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        "front_face",
        "cull_mode",
        "depth_test",
        "depth_write",
        "blending",
        "primitive_restart",
        "uniform_buffer",
        "storage_buffer",
        "topology",
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
        PyObject * front_face;
        PyObject * cull_mode = Py_None;
        VkBool32 depth_test = true;
        VkBool32 depth_write = true;
        VkBool32 blending = false;
        VkBool32 primitive_restart = false;
        VkDeviceSize uniform_buffer_size = 0;
        VkDeviceSize storage_buffer_size = 0;
        PyObject * topology;
        PyObject * memory = Py_None;
    } args;

    args.vertex_format = self->instance->state->empty_str;
    args.instance_format = self->instance->state->empty_str;
    args.front_face = self->instance->state->default_front_face;
    args.topology = self->instance->state->default_topology;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$O!O!OOIIIIOOppppkkOO",
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
        &args.front_face,
        &args.cull_mode,
        &args.depth_test,
        &args.depth_write,
        &args.blending,
        &args.primitive_restart,
        &args.uniform_buffer_size,
        &args.storage_buffer_size,
        &args.topology,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    Memory * memory = get_memory(self->instance, args.memory);

    Pipeline * res = PyObject_New(Pipeline, self->instance->state->Pipeline_type);
    res->instance = self->instance;

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

    res->vertex_buffer = NULL;
    res->instance_buffer = NULL;
    res->index_buffer = NULL;
    res->indirect_buffer = NULL;

    if (vstride * args.vertex_count) {
        BufferCreateInfo buffer_create_info = {
            self->instance,
            memory,
            vstride * args.vertex_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        };
        res->vertex_buffer = new_buffer(&buffer_create_info);
    }

    if (istride * args.instance_count) {
        BufferCreateInfo buffer_create_info = {
            self->instance,
            memory,
            istride * args.instance_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        };
        res->instance_buffer = new_buffer(&buffer_create_info);
    }

    if (args.index_count) {
        BufferCreateInfo buffer_create_info = {
            self->instance,
            memory,
            args.index_count * index_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        };
        res->index_buffer = new_buffer(&buffer_create_info);
    }

    if (args.indirect_count) {
        BufferCreateInfo buffer_create_info = {
            self->instance,
            memory,
            args.indirect_count * indirect_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        };
        res->indirect_buffer = new_buffer(&buffer_create_info);
    }

    res->uniform_buffer = NULL;
    if (args.uniform_buffer_size) {
        BufferCreateInfo buffer_create_info = {
            self->instance,
            memory,
            args.uniform_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };
        res->uniform_buffer = new_buffer(&buffer_create_info);
    }

    res->storage_buffer = NULL;
    if (args.storage_buffer_size) {
        BufferCreateInfo buffer_create_info = {
            self->instance,
            memory,
            args.storage_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        };
        res->storage_buffer = new_buffer(&buffer_create_info);
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

    if (res->uniform_buffer) {
        bind_buffer(res->uniform_buffer);
    }

    if (res->storage_buffer) {
        bind_buffer(res->storage_buffer);
    }

    VkBuffer vertex_buffer_array[64];

    for (uint32_t i = 0; i < vertex_attribute_count; ++i) {
        vertex_buffer_array[i] = res->vertex_buffer->buffer;
    }

    for (uint32_t i = vertex_attribute_count; i < attribute_count; ++i) {
        vertex_buffer_array[i] = res->instance_buffer->buffer;
    }

    res->vertex_buffers = preserve_array(attribute_count, vertex_buffer_array);

    uint32_t parent_uniform_buffer_binding = 0;
    uint32_t parent_storage_buffer_binding = 0;
    uint32_t uniform_buffer_binding = 0;
    uint32_t storage_buffer_binding = 0;

    uint32_t descriptor_binding_count = 0;
    VkDescriptorSetLayoutBinding descriptor_binding_array[8];
    VkDescriptorPoolSize descriptor_pool_size_array[8];

    if (self->uniform_buffer) {
        parent_uniform_buffer_binding = descriptor_binding_count++;
        descriptor_binding_array[parent_uniform_buffer_binding] = {
            parent_uniform_buffer_binding,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        };
        descriptor_pool_size_array[parent_uniform_buffer_binding] = {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
        };
    }

    if (self->storage_buffer) {
        parent_storage_buffer_binding = descriptor_binding_count++;
        descriptor_binding_array[parent_storage_buffer_binding] = {
            parent_storage_buffer_binding,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        };
        descriptor_pool_size_array[parent_storage_buffer_binding] = {
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
        };
    }

    if (res->uniform_buffer) {
        uniform_buffer_binding = descriptor_binding_count++;
        descriptor_binding_array[uniform_buffer_binding] = {
            uniform_buffer_binding,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        };
        descriptor_pool_size_array[uniform_buffer_binding] = {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
        };
    }

    if (res->storage_buffer) {
        storage_buffer_binding = descriptor_binding_count++;
        descriptor_binding_array[storage_buffer_binding] = {
            storage_buffer_binding,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        };
        descriptor_pool_size_array[storage_buffer_binding] = {
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
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

    res->descriptor_pool = NULL;
    res->descriptor_set = NULL;

    if (descriptor_binding_count) {
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
    }

    if (self->uniform_buffer) {
        buffer_descriptor_update({
            self->uniform_buffer,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            res->descriptor_set,
            parent_uniform_buffer_binding,
        });
    }

    if (self->storage_buffer) {
        buffer_descriptor_update({
            self->storage_buffer,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            res->descriptor_set,
            parent_storage_buffer_binding,
        });
    }

    if (res->uniform_buffer) {
        buffer_descriptor_update({
            res->uniform_buffer,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            res->descriptor_set,
            uniform_buffer_binding,
        });
    }

    if (res->storage_buffer) {
        buffer_descriptor_update({
            res->storage_buffer,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            res->descriptor_set,
            storage_buffer_binding,
        });
    }

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
        args.primitive_restart,
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
        get_cull_mode(args.cull_mode),
        get_front_face(args.front_face),
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
        args.depth_test,
        args.depth_write,
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
            args.blending,
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

    PyList_Append(self->pipeline_list, (PyObject *)res);
    return res;
}

PyObject * Pipeline_meth_update(Pipeline * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_buffer",
        "instance_buffer",
        "index_buffer",
        "indirect_buffer",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        NULL,
    };

    struct {
        PyObject * vertex_buffer = Py_None;
        PyObject * instance_buffer = Py_None;
        PyObject * index_buffer = Py_None;
        PyObject * indirect_buffer = Py_None;
        uint32_t vertex_count;
        uint32_t instance_count;
        uint32_t index_count;
        uint32_t indirect_count;
    } args;

    args.vertex_count = self->vertex_count;
    args.instance_count = self->instance_count;
    args.index_count = self->index_count;
    args.indirect_count = self->indirect_count;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$OOOOIIII",
        keywords,
        &args.vertex_buffer,
        &args.instance_buffer,
        &args.index_buffer,
        &args.indirect_buffer,
        &args.vertex_count,
        &args.instance_count,
        &args.index_count,
        &args.indirect_count
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.vertex_buffer != Py_None) {
        Py_XDECREF(PyObject_CallMethod((PyObject *)self->vertex_buffer, "write", "O", args.vertex_buffer));
    }
    if (args.instance_buffer != Py_None) {
        Py_XDECREF(PyObject_CallMethod((PyObject *)self->instance_buffer, "write", "O", args.instance_buffer));
    }
    if (args.index_buffer != Py_None) {
        Py_XDECREF(PyObject_CallMethod((PyObject *)self->index_buffer, "write", "O", args.index_buffer));
    }
    if (args.indirect_buffer != Py_None) {
        Py_XDECREF(PyObject_CallMethod((PyObject *)self->indirect_buffer, "write", "O", args.indirect_buffer));
    }

    self->vertex_count = args.vertex_count;
    self->instance_count = args.instance_count;
    self->index_count = args.index_count;
    self->indirect_count = args.indirect_count;

    Py_RETURN_NONE;
}
