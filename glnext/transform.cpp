#include "glnext.hpp"

Transform * Instance_meth_transform(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "compute_shader",
        "compute_groups",
        "uniform_buffer",
        "storage_buffer",
        "output_buffer",
        "memory",
        NULL,
    };

    struct {
        PyObject * compute_shader = NULL;
        uint32_t compute_groups_x = 1;
        uint32_t compute_groups_y = 1;
        uint32_t compute_groups_z = 1;
        VkDeviceSize uniform_buffer_size = 0;
        VkDeviceSize storage_buffer_size = 0;
        VkDeviceSize output_buffer_size = 0;
        PyObject * memory = Py_None;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$O!(III)kkkO",
        keywords,
        &PyBytes_Type,
        &args.compute_shader,
        &args.compute_groups_x,
        &args.compute_groups_y,
        &args.compute_groups_z,
        &args.uniform_buffer_size,
        &args.storage_buffer_size,
        &args.output_buffer_size,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (!args.compute_shader) {
        return NULL;
    }

    Memory * memory = get_memory(self, args.memory);

    Transform * res = PyObject_New(Transform, self->state->Transform_type);

    res->instance = self;
    res->compute_groups_x = args.compute_groups_x;
    res->compute_groups_y = args.compute_groups_y;
    res->compute_groups_z = args.compute_groups_z;

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

    res->output_buffer = NULL;
    if (args.output_buffer_size) {
        BufferCreateInfo buffer_create_info = {
            self,
            memory,
            args.output_buffer_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        };
        res->output_buffer = new_buffer(&buffer_create_info);
    }

    allocate_memory(memory);

    if (res->uniform_buffer) {
        bind_buffer(res->uniform_buffer);
    }

    if (res->storage_buffer) {
        bind_buffer(res->storage_buffer);
    }

    if (res->output_buffer) {
        bind_buffer(res->output_buffer);
    }

    uint32_t uniform_buffer_binding = 0;
    uint32_t storage_buffer_binding = 0;
    uint32_t output_buffer_binding = 0;

    uint32_t descriptor_binding_count = 0;
    VkDescriptorSetLayoutBinding descriptor_binding_array[8];
    VkDescriptorPoolSize descriptor_pool_size_array[8];

    if (res->uniform_buffer) {
        uniform_buffer_binding = descriptor_binding_count++;
        descriptor_binding_array[uniform_buffer_binding] = {
            uniform_buffer_binding,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_COMPUTE_BIT,
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
            VK_SHADER_STAGE_COMPUTE_BIT,
            NULL,
        };
        descriptor_pool_size_array[storage_buffer_binding] = {
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
        };
    }

    if (res->output_buffer) {
        output_buffer_binding = descriptor_binding_count++;
        descriptor_binding_array[output_buffer_binding] = {
            output_buffer_binding,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            VK_SHADER_STAGE_COMPUTE_BIT,
            NULL,
        };
        descriptor_pool_size_array[output_buffer_binding] = {
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

    if (res->output_buffer) {
        buffer_descriptor_update({
            res->output_buffer,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            res->descriptor_set,
            output_buffer_binding,
        });
    }

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

    PyList_Append(self->transform_list, (PyObject *)res);
    return res;
}

PyObject * Transform_meth_update(Transform * self, PyObject * vargs, PyObject * kwargs) {
    Py_RETURN_NONE;
}

void execute_transform(VkCommandBuffer command_buffer, Transform * transform) {
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, transform->pipeline);

    vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        transform->pipeline_layout,
        0,
        1,
        &transform->descriptor_set,
        0,
        NULL
    );

    vkCmdDispatch(command_buffer, transform->compute_groups_x, transform->compute_groups_y, transform->compute_groups_z);
}
