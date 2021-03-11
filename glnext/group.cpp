#include "glnext.hpp"

Group * Instance_meth_group(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"buffer", NULL};

    VkDeviceSize buffer = 0;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwargs, "$K", keywords, &buffer)) {
        return NULL;
    }

    Group * res = PyObject_New(Group, self->state->Group_type);

    res->instance = self;
    res->output = PyList_New(0);
    res->offset = 0;

    res->temp = {};
    new_temp_buffer(self, &res->temp, buffer);
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
