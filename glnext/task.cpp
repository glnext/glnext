#include "glnext.hpp"

Task * Instance_meth_task(Instance * self) {
    Task * res = PyObject_New(Task, self->state->Task_type);
    res->instance = self;
    res->task_list = PyList_New(0);
    res->finished = false;
    PyList_Append(self->task_list, (PyObject *)res);
    return res;
}

PyObject * Task_meth_run(Task * self) {
    if (!self->instance->group) {
        begin_commands(self->instance);
    }

    for (uint32_t i = 0; i < PyList_Size(self->task_list); ++i) {
        PyObject * obj = PyList_GetItem(self->task_list, i);
        if (Py_TYPE(obj) == self->instance->state->Framebuffer_type) {
            execute_framebuffer((Framebuffer *)obj, self->instance->command_buffer);
        }
        if (Py_TYPE(obj) == self->instance->state->ComputePipeline_type) {
            execute_compute_pipeline((ComputePipeline *)obj, self->instance->command_buffer);
        }
    }

    if (!self->instance->group) {
        end_commands(self->instance);
    }

    Py_RETURN_NONE;

    // if (!self->finished) {
    //     VkCommandBufferAllocateInfo command_buffer_allocate_info = {
    //         VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    //         NULL,
    //         self->instance->task_command_pool,
    //         VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    //         1,
    //     };

    //     self->instance->vkAllocateCommandBuffers(self->instance->device, &command_buffer_allocate_info, &self->command_buffer);

    //     VkCommandBufferBeginInfo command_buffer_begin_info = {
    //         VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    //         NULL,
    //         0,
    //         NULL,
    //     };

    //     self->instance->vkBeginCommandBuffer(self->command_buffer, &command_buffer_begin_info);

    //     for (uint32_t i = 0; i < PyList_Size(self->task_list); ++i) {
    //         PyObject * obj = PyList_GetItem(self->task_list, i);
    //         if (Py_TYPE(obj) == self->instance->state->Framebuffer_type) {
    //             execute_framebuffer((Framebuffer *)obj, self->command_buffer);
    //         }
    //         if (Py_TYPE(obj) == self->instance->state->ComputePipeline_type) {
    //             execute_compute_pipeline((ComputePipeline *)obj, self->command_buffer);
    //         }
    //     }

    //     self->instance->vkEndCommandBuffer(self->command_buffer);
    //     self->finished = true;
    // }

    // VkSubmitInfo submit_info = {
    //     VK_STRUCTURE_TYPE_SUBMIT_INFO,
    //     NULL,
    //     0,
    //     NULL,
    //     NULL,
    //     1,
    //     &self->command_buffer,
    //     0,
    //     NULL,
    // };

    // self->instance->vkQueueSubmit(self->instance->queue, 1, &submit_info, self->instance->fence);
    // self->instance->vkWaitForFences(self->instance->device, 1, &self->instance->fence, true, UINT64_MAX);
    // self->instance->vkResetFences(self->instance->device, 1, &self->instance->fence);
    // Py_RETURN_NONE;
}
