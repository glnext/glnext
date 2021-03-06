#include "glnext.hpp"

Group * Instance_meth_group(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"buffer", "present", NULL};

    struct {
        VkDeviceSize buffer = 0;
        VkBool32 present = false;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(vargs, kwargs, "|$Kp", keywords, &args.buffer, &args.present);

    if (!args_ok) {
        return NULL;
    }

    Group * res = PyObject_New(Group, self->state->Group_type);

    res->instance = self;
    res->present = args.present;
    res->output = PyList_New(0);
    res->offset = 0;

    res->temp = {};
    new_temp_buffer(self, &res->temp, args.buffer);
    return res;
}

PyObject * Group_meth_enter(Group * self) {
    begin_commands(self->instance);
    PySequence_DelSlice(self->output, 0, PyList_Size(self->output));
    self->instance->group = self;
    self->offset = 0;
    Py_INCREF(self);
    Py_RETURN_NONE;
}

PyObject * Group_meth_exit(Group * self) {
    end_commands(self->instance);
    self->instance->group = NULL;
    Py_DECREF(self);
    Py_RETURN_NONE;
}
