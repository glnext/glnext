#include "glnext.hpp"

Batch * Instance_meth_task(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "tasks",
        "staging_buffers",
        "present",
        NULL,
    };

    struct {
        PyObject * tasks;
        PyObject * staging_buffers;
        VkBool32 present = false;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "O!O!|p",
        keywords,
        &PyList_Type,
        &args.tasks,
        &PyList_Type,
        &args.staging_buffers,
        &args.present
    );

    if (!args_ok) {
        return NULL;
    }

    uint32_t task_count = (uint32_t)PyList_Size(args.tasks);
    uint32_t staging_buffer_count = (uint32_t)PyList_Size(args.staging_buffers);

    Batch * res = PyObject_New(Batch, self->state->Batch_type);

    res->instance = self;
    res->present = args.present;

    VkCommandPoolCreateInfo command_pool_create_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        0,
        res->instance->queue_family_index,
    };

    res->instance->vkCreateCommandPool(res->instance->device, &command_pool_create_info, NULL, &res->command_pool);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        res->command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1,
    };

    res->instance->vkAllocateCommandBuffers(res->instance->device, &command_buffer_allocate_info, &res->command_buffer);

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        0,
        NULL,
    };

    res->instance->vkBeginCommandBuffer(res->command_buffer, &command_buffer_begin_info);

    for (uint32_t i = 0; i < staging_buffer_count; ++i) {
        PyObject * obj = PyList_GetItem(args.staging_buffers, i);
        execute_staging_buffer_input((StagingBuffer *)obj);
    }

    for (uint32_t i = 0; i < task_count; ++i) {
        PyObject * obj = PyList_GetItem(args.tasks, i);
        if (Py_TYPE(obj) == self->state->Framebuffer_type) {
            execute_framebuffer((Framebuffer *)obj);
        }
        if (Py_TYPE(obj) == self->state->ComputePipeline_type) {
            execute_compute_pipeline((ComputePipeline *)obj);
        }
    }

    for (uint32_t i = 0; i < staging_buffer_count; ++i) {
        PyObject * obj = PyList_GetItem(args.staging_buffers, i);
        execute_staging_buffer_output((StagingBuffer *)obj);
    }

    self->vkEndCommandBuffer(res->command_buffer);
    return res;
}

PyObject * Batch_meth_run(Batch * self) {
    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        0,
        NULL,
        NULL,
        1,
        &self->command_buffer,
        0,
        NULL,
    };

    self->instance->vkQueueSubmit(self->instance->queue, 1, &submit_info, self->instance->fence);
    self->instance->vkWaitForFences(self->instance->device, 1, &self->instance->fence, true, UINT64_MAX);
    self->instance->vkResetFences(self->instance->device, 1, &self->instance->fence);

    if (self->present && self->instance->presenter.surface_count) {
        begin_commands(self->instance);
        end_commands_with_present(self->instance);
    }

    Py_RETURN_NONE;
}
