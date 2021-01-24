#include "glnext.hpp"

ComputeSet * Instance_meth_compute_set(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "size",
        "format",
        "levels",
        "layers",
        "mode",
        "compute_shader",
        "compute_groups",
        "uniform_buffer",
        "storage_buffer",
        "samplers",
        "memory",
        NULL,
    };

    struct {
        uint32_t width = 0;
        uint32_t height = 0;
        PyObject * format;
        uint32_t levels = 1;
        uint32_t layers = 1;
        PyObject * mode;
        PyObject * compute_shader = NULL;
        uint32_t compute_groups_x = 0;
        uint32_t compute_groups_y = 0;
        uint32_t compute_groups_z = 0;
        VkDeviceSize uniform_buffer_size = 0;
        VkDeviceSize storage_buffer_size = 0;
        PyObject * samplers = Py_None;
        PyObject * memory = Py_None;
    } args;

    args.format = self->state->default_format;
    args.mode = self->state->output_str;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$(II)O!IIO!O!(III)kkOO",
        keywords,
        &args.width,
        &args.height,
        &PyUnicode_Type,
        &args.format,
        &args.levels,
        &args.layers,
        &PyUnicode_Type,
        &args.mode,
        &PyBytes_Type,
        &args.compute_shader,
        &args.compute_groups_x,
        &args.compute_groups_y,
        &args.compute_groups_z,
        &args.uniform_buffer_size,
        &args.storage_buffer_size,
        &args.samplers,
        &args.memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.compute_groups_x == 0 && args.compute_groups_y == 0 && args.compute_groups_z == 0) {
        uint32_t local_size[] = {1, 1, 1};
        get_compute_local_size(args.compute_shader, local_size);
        args.compute_groups_x = args.width / local_size[0];
        args.compute_groups_y = args.height / local_size[1];
        args.compute_groups_z = 1;
    }

    if (args.samplers == Py_None) {
        args.samplers = self->state->empty_list;
    }

    Memory * memory = get_memory(self, args.memory);

    PyObject * format_list = PyUnicode_Split(args.format, NULL, -1);
    uint32_t output_count = (uint32_t)PyList_Size(format_list);
    ImageMode image_mode = get_image_mode(args.mode);

    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (image_mode == IMG_TEXTURE) {
        image_usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    ComputeSet * res = PyObject_New(ComputeSet, self->state->ComputeSet_type);

    res->instance = self;
    res->width = args.width;
    res->height = args.height;
    res->levels = args.levels;
    res->layers = args.layers;

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

    Image * result_image_array[64];

    for (uint32_t i = 0; i < output_count; ++i) {
        Format format = get_format(PyList_GetItem(format_list, i));
        ImageCreateInfo image_create_info = {
            self,
            memory,
            args.width * args.height * args.layers * format.size,
            image_usage | VK_IMAGE_USAGE_STORAGE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            {args.width, args.height, 1},
            VK_SAMPLE_COUNT_1_BIT,
            args.levels,
            args.layers,
            image_mode,
            format.format,
        };
        result_image_array[i] = new_image(&image_create_info);
    }

    allocate_memory(memory);

    if (res->uniform_buffer) {
        bind_buffer(res->uniform_buffer);
    }

    if (res->storage_buffer) {
        bind_buffer(res->storage_buffer);
    }

    for (uint32_t i = 0; i < output_count; ++i) {
        bind_image(result_image_array[i]);
    }

    VkImageView image_view_array[64];
    VkDescriptorImageInfo result_binding_array[64];

    for (uint32_t i = 0; i < output_count; ++i) {
        VkImageViewCreateInfo image_view_create_info = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            result_image_array[i]->image,
            VK_IMAGE_VIEW_TYPE_2D,
            result_image_array[i]->format,
            {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, args.layers},
        };

        VkImageView image_view = NULL;
        vkCreateImageView(self->device, &image_view_create_info, NULL, &image_view);

        image_view_array[i] = image_view;
        result_binding_array[i] = {
            NULL,
            image_view,
            VK_IMAGE_LAYOUT_GENERAL,
        };
    }

    uint32_t uniform_buffer_binding = 0;
    uint32_t storage_buffer_binding = 0;
    uint32_t output_image_binding = 0;

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

    output_image_binding = descriptor_binding_count++;
    descriptor_binding_array[output_image_binding] = {
        output_image_binding,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        output_count,
        VK_SHADER_STAGE_COMPUTE_BIT,
        NULL,
    };
    descriptor_pool_size_array[output_image_binding] = {
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        output_count,
    };

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

    VkWriteDescriptorSet write_result_descriptor_set = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        NULL,
        res->descriptor_set,
        output_image_binding,
        0,
        output_count,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        result_binding_array,
        NULL,
        NULL,
    };

    vkUpdateDescriptorSets(self->device, 1, &write_result_descriptor_set, 0, NULL);

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

    res->output = PyTuple_New(output_count);
    for (uint32_t i = 0; i < output_count; ++i) {
        PyTuple_SetItem(res->output, i, (PyObject *)result_image_array[i]);
        Py_INCREF(result_image_array[i]);
    }

    res->result_images = preserve_array(output_count, result_image_array);

    PyList_Append(self->task_list, (PyObject *)res);
    return res;
}

PyObject * ComputeSet_meth_update(ComputeSet * self, PyObject * vargs, PyObject * kwargs) {
    Py_RETURN_NONE;
}

void execute_compute_set(VkCommandBuffer cmd, ComputeSet * compute_set) {
    Image ** result_image_array = NULL;
    uint32_t result_image_count = retreive_array(compute_set->result_images, &result_image_array);

    {
        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[64];

        for (uint32_t i = 0; i < result_image_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                result_image_array[i]->image,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, compute_set->layers},
            };
        }

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            image_barrier_count,
            image_barrier_array
        );
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_set->pipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        compute_set->pipeline_layout,
        0,
        1,
        &compute_set->descriptor_set,
        0,
        NULL
    );

    vkCmdDispatch(cmd, compute_set->width, compute_set->height, 1);

    {
        uint32_t image_barrier_count = 0;
        VkImageMemoryBarrier image_barrier_array[64];

        for (uint32_t i = 0; i < result_image_count; ++i) {
            image_barrier_array[image_barrier_count++] = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                NULL,
                0,
                0,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                result_image_array[i]->image,
                {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, compute_set->layers},
            };
        }

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            NULL,
            0,
            NULL,
            image_barrier_count,
            image_barrier_array
        );
    }

    if (compute_set->levels > 1) {
        build_mipmaps({
            cmd,
            compute_set->width,
            compute_set->height,
            compute_set->levels,
            compute_set->layers,
            result_image_count,
            result_image_array,
        });
    }
}
