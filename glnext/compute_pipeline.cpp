#include "glnext.hpp"

int parse_compute_count(PyObject * arg, uint32_t * compute_count) {
    if (PyLong_Check(arg)) {
        compute_count[0] = PyLong_AsUnsignedLong(arg);
        compute_count[1] = 1;
        compute_count[2] = 1;
    } else if (PyTuple_Check(arg) && PyTuple_Size(arg) == 2) {
        compute_count[0] = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 0));
        compute_count[1] = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 1));
        compute_count[2] = 1;
    } else if (PyTuple_Check(arg) && PyTuple_Size(arg) == 3) {
        compute_count[0] = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 0));
        compute_count[1] = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 1));
        compute_count[2] = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 2));
    } else {
        PyErr_Format(PyExc_TypeError, "compute_count");
        return 0;
    }
    if (PyErr_Occurred()) {
        PyErr_Format(PyExc_TypeError, "compute_count");
        return 0;
    }
    return 1;
}

ComputePipeline * new_compute_pipeline(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "compute_shader",
        "compute_count",
        "bindings",
        "memory",
        NULL,
    };

    struct {
        PyObject * compute_shader = Py_None;
        uint32_t compute_count[3] = {};
        PyObject * bindings;
        PyObject * memory = Py_None;
    } args;

    args.bindings = self->state->empty_list;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$O!O&OO",
        keywords,
        &PyBytes_Type,
        &args.compute_shader,
        parse_compute_count,
        args.compute_count,
        &args.bindings,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.compute_shader == Py_None) {
        return NULL;
    }

    if (!args.compute_count[0] || !args.compute_count[1] || !args.compute_count[2]) {
        return NULL;
    }

    Memory * memory = get_memory(self, args.memory);

    ComputePipeline * res = PyObject_New(ComputePipeline, self->state->ComputePipeline_type);

    res->instance = self;
    res->members = PyDict_New();

    res->parameters = {
        true,
        args.compute_count[0],
        args.compute_count[1],
        args.compute_count[2],
    };

    res->binding_count = (uint32_t)PyList_Size(args.bindings);
    res->binding_array = (DescriptorBinding *)PyMem_Malloc(sizeof(DescriptorBinding) * PyList_Size(args.bindings));

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        if (parse_descriptor_binding(self, &res->binding_array[i], PyList_GetItem(args.bindings, i))) {
            return NULL;
        }
    }

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        create_descriptor_binding_objects(self, &res->binding_array[i], memory);
    }

    allocate_memory(memory);

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        bind_descriptor_binding_objects(self, &res->binding_array[i]);
    }

    res->descriptor_binding_array = (VkDescriptorSetLayoutBinding *)PyMem_Malloc(sizeof(VkDescriptorSetLayoutBinding) * res->binding_count);
    res->descriptor_pool_size_array = (VkDescriptorPoolSize *)PyMem_Malloc(sizeof(VkDescriptorPoolSize) * res->binding_count);
    res->write_descriptor_set_array = (VkWriteDescriptorSet *)PyMem_Malloc(sizeof(VkWriteDescriptorSet) * res->binding_count);

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        res->descriptor_binding_array[i] = res->binding_array[i].descriptor_binding;
        res->descriptor_pool_size_array[i] = res->binding_array[i].descriptor_pool_size;
        res->write_descriptor_set_array[i] = res->binding_array[i].write_descriptor_set;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        NULL,
        0,
        0,
        NULL,
        0,
        NULL,
    };

    if (res->binding_count) {
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            NULL,
            0,
            res->binding_count,
            res->descriptor_binding_array,
        };

        self->vkCreateDescriptorSetLayout(self->device, &descriptor_set_layout_create_info, NULL, &res->descriptor_set_layout);

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

        self->vkCreateDescriptorPool(self->device, &descriptor_pool_create_info, NULL, &res->descriptor_pool);

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            NULL,
            res->descriptor_pool,
            1,
            &res->descriptor_set_layout,
        };

        self->vkAllocateDescriptorSets(self->device, &descriptor_set_allocate_info, &res->descriptor_set);

        for (uint32_t i = 0; i < res->binding_count; ++i) {
            res->write_descriptor_set_array[i].dstSet = res->descriptor_set;
        }

        self->vkUpdateDescriptorSets(
            self->device,
            res->binding_count,
            res->write_descriptor_set_array,
            NULL,
            NULL
        );
    }

    self->vkCreatePipelineLayout(self->device, &pipeline_layout_create_info, NULL, &res->pipeline_layout);

    VkShaderModuleCreateInfo compute_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.compute_shader),
        (uint32_t *)PyBytes_AsString(args.compute_shader),
    };

    VkShaderModule compute_shader_module = NULL;
    self->vkCreateShaderModule(self->device, &compute_shader_module_create_info, NULL, &compute_shader_module);

    VkComputePipelineCreateInfo pipeline_create_info = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        NULL,
        0,
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            NULL,
            0,
            VK_SHADER_STAGE_COMPUTE_BIT,
            compute_shader_module,
            "main",
            NULL,
        },
        res->pipeline_layout,
        NULL,
        0,
    };

    self->vkCreateComputePipelines(self->device, self->pipeline_cache, 1, &pipeline_create_info, NULL, &res->pipeline);

    self->vkDestroyShaderModule(self->device, compute_shader_module, NULL);

    for (uint32_t i = 0; i < res->binding_count; ++i) {
        if (res->binding_array[i].name) {
            if (res->binding_array[i].is_buffer) {
                PyDict_SetItem(res->members, res->binding_array[i].name, (PyObject *)res->binding_array[i].buffer.buffer);
            }
        }
    }

    return res;
}

ComputePipeline * Framebuffer_meth_compute(Framebuffer * self, PyObject * vargs, PyObject * kwargs) {
    if (!self->compute) {
        return NULL;
    }
    ComputePipeline * res = new_compute_pipeline(self->instance, vargs, kwargs);
    if (!res) {
        return NULL;
    }
    PyList_Append(self->compute_pipeline_list, (PyObject *)res);
    return res;
}

ComputePipeline * Instance_meth_compute(Instance * self, PyObject * vargs, PyObject * kwargs) {
    ComputePipeline * res = new_compute_pipeline(self, vargs, kwargs);
    if (!res) {
        return NULL;
    }
    PyList_Append(self->task_list, (PyObject *)res);
    return res;
}

PyObject * ComputePipeline_meth_update(ComputePipeline * self, PyObject * vargs, PyObject * kwargs) {
    if (PyTuple_Size(vargs) || !kwargs) {
        PyErr_Format(PyExc_TypeError, "invalid arguments");
    }

    Py_ssize_t pos = 0;
    PyObject * key = NULL;
    PyObject * value = NULL;

    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        if (!PyUnicode_CompareWithASCIIString(key, "compute_count")) {
            uint32_t compute_count[3];
            if (!parse_compute_count(value, compute_count)) {
                return NULL;
            }
            self->parameters.x = compute_count[0];
            self->parameters.y = compute_count[1];
            self->parameters.z = compute_count[2];
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

void execute_compute_pipeline(ComputePipeline * self, VkCommandBuffer command_buffer) {
    if (!self->parameters.enabled) {
        return;
    }

    self->instance->vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, self->pipeline);

    self->instance->vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        self->pipeline_layout,
        0,
        1,
        &self->descriptor_set,
        0,
        NULL
    );

    self->instance->vkCmdDispatch(command_buffer, self->parameters.x, self->parameters.x, self->parameters.z);
}

PyObject * ComputePipeline_subscript(ComputePipeline * self, PyObject * key) {
    return PyObject_GetItem(self->members, key);
}
