#include "glnext.hpp"

int parse_compute_count(PyObject * arg, ComputeCount * compute_count) {
    if (PyLong_Check(arg)) {
        compute_count->x = PyLong_AsUnsignedLong(arg);
        compute_count->y = 1;
        compute_count->z = 1;
    } else if (PyTuple_Check(arg) && PyTuple_Size(arg) == 2) {
        compute_count->x = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 0));
        compute_count->y = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 1));
        compute_count->z = 1;
    } else if (PyTuple_Check(arg) && PyTuple_Size(arg) == 3) {
        compute_count->x = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 0));
        compute_count->y = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 1));
        compute_count->z = PyLong_AsUnsignedLong(PyTuple_GetItem(arg, 2));
    } else {
        PyErr_Format(PyExc_TypeError, "compute_count");
        return 0;
    }
    if (PyErr_Occurred()) {
        return 0;
    }
    return 1;
}

ComputePipeline * new_compute_pipeline(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "compute_shader",
        "compute_count",
        "buffers",
        "images",
        "memory",
        NULL,
    };

    struct {
        PyObject * compute_shader = Py_None;
        ComputeCount compute_count = {};
        PyObject * buffers;
        PyObject * images;
        PyObject * memory = Py_None;
    } args;

    args.buffers = self->state->empty_list;
    args.images = self->state->empty_list;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$O!O&OOO",
        keywords,
        &PyBytes_Type,
        &args.compute_shader,
        parse_compute_count,
        &args.compute_count,
        &args.buffers,
        &args.images,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.compute_shader == Py_None) {
        return NULL;
    }

    if (!args.compute_count.x || !args.compute_count.y || !args.compute_count.z) {
        return NULL;
    }

    Memory * memory = get_memory(self, args.memory);

    ComputePipeline * res = PyObject_New(ComputePipeline, self->state->ComputePipeline_type);

    res->instance = self;
    res->members = PyDict_New();
    res->compute_count = args.compute_count;

    res->buffer_count = (uint32_t)PyList_Size(args.buffers);
    res->image_count = (uint32_t)PyList_Size(args.images);

    res->buffer_array = (BufferBinding *)PyMem_Malloc(sizeof(BufferBinding) * PyList_Size(args.buffers));
    res->image_array = (ImageBinding *)PyMem_Malloc(sizeof(ImageBinding) * PyList_Size(args.images));

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        BufferBinding buffer_binding = parse_buffer_binding(PyList_GetItem(args.buffers, i));
        if (!buffer_binding.buffer) {
            buffer_binding.buffer = new_buffer({
                self,
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

    allocate_memory(memory);

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        bind_buffer(res->buffer_array[i].buffer);
    }

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        res->buffer_array[i].descriptor_buffer_info.buffer = res->buffer_array[i].buffer->buffer;
    }

    for (uint32_t i = 0; i < res->image_count; ++i) {
        ImageBinding & image_binding = res->image_array[i];
        for (uint32_t j = 0; j < image_binding.image_count; ++j) {
            VkImageView image_view = NULL;
            self->vkCreateImageView(
                self->device,
                &image_binding.image_view_create_info_array[j],
                NULL,
                &image_view
            );
            VkSampler sampler = NULL;
            if (image_binding.sampled) {
                self->vkCreateSampler(
                    self->device,
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

    uint32_t descriptor_binding_count = res->buffer_count + res->image_count;
    res->descriptor_binding_array = (VkDescriptorSetLayoutBinding *)PyMem_Malloc(sizeof(VkDescriptorSetLayoutBinding) * descriptor_binding_count);
    res->descriptor_pool_size_array = (VkDescriptorPoolSize *)PyMem_Malloc(sizeof(VkDescriptorPoolSize) * descriptor_binding_count);
    res->write_descriptor_set_array = (VkWriteDescriptorSet *)PyMem_Malloc(sizeof(VkWriteDescriptorSet) * descriptor_binding_count);

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        res->descriptor_binding_array[i] = {
            res->buffer_array[i].binding,
            res->buffer_array[i].descriptor_type,
            1,
            VK_SHADER_STAGE_COMPUTE_BIT,
            NULL,
        };
        res->descriptor_pool_size_array[i] = {
            res->buffer_array[i].descriptor_type,
            1,
        };
        res->write_descriptor_set_array[i] = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            NULL,
            res->buffer_array[i].binding,
            0,
            1,
            res->buffer_array[i].descriptor_type,
            NULL,
            &res->buffer_array[i].descriptor_buffer_info,
            NULL,
        };
    }

    for (uint32_t i = 0; i < res->image_count; ++i) {
        res->descriptor_binding_array[res->buffer_count + i] = {
            res->image_array[i].binding,
            res->image_array[i].descriptor_type,
            1,
            VK_SHADER_STAGE_COMPUTE_BIT,
            NULL,
        };
        res->descriptor_pool_size_array[res->buffer_count + i] = {
            res->image_array[i].descriptor_type,
            1,
        };
        res->write_descriptor_set_array[res->buffer_count + i] = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            NULL,
            res->image_array[i].binding,
            0,
            1,
            res->image_array[i].descriptor_type,
            res->image_array[i].descriptor_image_info_array,
            NULL,
            NULL,
        };
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

    if (descriptor_binding_count) {
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            NULL,
            0,
            descriptor_binding_count,
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
            descriptor_binding_count,
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

        for (uint32_t i = 0; i < descriptor_binding_count; ++i) {
            res->write_descriptor_set_array[i].dstSet = res->descriptor_set;
        }

        self->vkUpdateDescriptorSets(
            self->device,
            descriptor_binding_count,
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

    self->vkCreateShaderModule(self->device, &compute_shader_module_create_info, NULL, &res->compute_shader_module);

    VkComputePipelineCreateInfo compute_pipeline_create_info = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        NULL,
        0,
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            NULL,
            0,
            VK_SHADER_STAGE_COMPUTE_BIT,
            res->compute_shader_module,
            "main",
            NULL,
        },
        res->pipeline_layout,
        NULL,
        0,
    };

    self->vkCreateComputePipelines(self->device, NULL, 1, &compute_pipeline_create_info, NULL, &res->pipeline);

    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        if (res->buffer_array[i].name != Py_None) {
            PyDict_SetItem(res->members, res->buffer_array[i].name, (PyObject *)res->buffer_array[i].buffer);
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
            if (!parse_compute_count(value, &self->compute_count)) {
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

void execute_compute_pipeline(ComputePipeline * self) {
    self->instance->vkCmdBindPipeline(self->instance->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, self->pipeline);

    self->instance->vkCmdBindDescriptorSets(
        self->instance->command_buffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        self->pipeline_layout,
        0,
        1,
        &self->descriptor_set,
        0,
        NULL
    );

    self->instance->vkCmdDispatch(self->instance->command_buffer, self->compute_count.x, self->compute_count.x, self->compute_count.z);
}

PyObject * ComputePipeline_subscript(ComputePipeline * self, PyObject * key) {
    return PyObject_GetItem(self->members, key);
}
