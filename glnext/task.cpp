#include "glnext.hpp"

Task * Instance_meth_task(Instance * self, PyObject * vargs, PyObject * kwargs) {
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

    Task * res = PyObject_New(Task, self->state->Task_type);

    res->instance = self;
    res->present = args.present;

    res->task_count = task_count;
    res->task_array = (PyObject **)PyMem_Malloc(sizeof(PyObject *) * task_count);
    res->task_callback_array = (TaskCallback *)PyMem_Malloc(sizeof(TaskCallback) * task_count);

    for (uint32_t i = 0; i < task_count; ++i) {
        PyObject * obj = PyList_GetItem(args.tasks, i);
        res->task_array[i] = obj;
        if (Py_TYPE(obj) == self->state->Framebuffer_type) {
            res->task_callback_array[i] = (TaskCallback)execute_framebuffer;
        }
        if (Py_TYPE(obj) == self->state->ComputePipeline_type) {
            res->task_callback_array[i] = (TaskCallback)execute_compute_pipeline;
        }
    }

    res->staging_buffer_count = staging_buffer_count;
    res->staging_buffer_array = (StagingBuffer **)PyMem_Malloc(sizeof(StagingBuffer *) * staging_buffer_count);

    for (uint32_t i = 0; i < staging_buffer_count; ++i) {
        res->staging_buffer_array[i] = (StagingBuffer *)PyList_GetItem(args.staging_buffers, i);
    }

    return res;
}

void execute_task(Task * self) {
    begin_commands(self->instance);

    for (uint32_t i = 0; i < self->staging_buffer_count; ++i) {
        execute_staging_buffer_input(self->staging_buffer_array[i]);
    }

    for (uint32_t i = 0; i < self->task_count; ++i) {
        self->task_callback_array[i](self->task_array[i]);
    }

    for (uint32_t i = 0; i < self->staging_buffer_count; ++i) {
        execute_staging_buffer_output(self->staging_buffer_array[i]);
    }

    if (self->present && self->instance->presenter.surface_count) {
        end_commands_with_present(self->instance);
    } else {
        end_commands(self->instance);
    }
}

PyObject * Task_meth_run(Task * self) {
    execute_task(self);
    Py_RETURN_NONE;
}
