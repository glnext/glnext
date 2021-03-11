#include "glnext.hpp"

Task * Instance_meth_task(Instance * self) {
    Task * res = PyObject_New(Task, self->state->Task_type);
    res->instance = self;
    res->task_list = PyList_New(0);
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
}
