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

    for (uint32_t i = 0; i < res->image_count; ++i) {
        ImageBinding & image_binding = res->image_array[i];
        for (uint32_t j = 0; j < image_binding.image_count; ++j) {
            VkImageView image_view = NULL;
            vkCreateImageView(
                self->device,
                &image_binding.image_view_create_info_array[j],
                NULL,
                &image_view
            );
            VkSampler sampler = NULL;
            if (image_binding.sampled) {
                vkCreateSampler(
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
            VK_SHADER_STAGE_COMPUTE_BIT,
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

    vkCreateDescriptorSetLayout(self->device, &descriptor_set_layout_create_info, NULL, &res->descriptor_set_layout);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        NULL,
        0,
        1,
        &res->descriptor_set_layout,
        0,
        NULL,
    };

    vkCreatePipelineLayout(self->device, &pipeline_layout_create_info, NULL, &res->pipeline_layout);

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        NULL,
        0,
        1,
        descriptor_binding_count,
        descriptor_pool_size_array,
    };

    vkCreateDescriptorPool(self->device, &descriptor_pool_create_info, NULL, &res->descriptor_pool);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        NULL,
        res->descriptor_pool,
        1,
        &res->descriptor_set_layout,
    };

    vkAllocateDescriptorSets(self->device, &descriptor_set_allocate_info, &res->descriptor_set);

    for (uint32_t i = 0; i < descriptor_binding_count; ++i) {
        write_descriptor_set_array[i].dstSet = res->descriptor_set;
    }

    vkUpdateDescriptorSets(
        self->device,
        descriptor_binding_count,
        write_descriptor_set_array,
        NULL,
        NULL
    );

    VkShaderModuleCreateInfo compute_shader_module_create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        NULL,
        0,
        (VkDeviceSize)PyBytes_Size(args.compute_shader),
        (uint32_t *)PyBytes_AsString(args.compute_shader),
    };

    vkCreateShaderModule(self->device, &compute_shader_module_create_info, NULL, &res->compute_shader_module);

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

    vkCreateComputePipelines(self->device, NULL, 1, &compute_pipeline_create_info, NULL, &res->pipeline);

    res->buffer = PyTuple_New(res->buffer_count);
    for (uint32_t i = 0; i < res->buffer_count; ++i) {
        PyTuple_SetItem(res->buffer, i, (PyObject *)res->buffer_array[i].buffer);
        Py_INCREF(res->buffer_array[i].buffer);
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

void execute_compute_pipeline(VkCommandBuffer cmd, ComputePipeline * pipeline) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipeline->pipeline_layout,
        0,
        1,
        &pipeline->descriptor_set,
        0,
        NULL
    );

    vkCmdDispatch(cmd, pipeline->compute_count.x, pipeline->compute_count.x, pipeline->compute_count.z);
}
