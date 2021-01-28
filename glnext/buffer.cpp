#include "glnext.hpp"

PyObject * Buffer_meth_read(Buffer * self) {
    HostBuffer temp = {};
    new_temp_buffer(self->instance, &temp, self->size);

    VkCommandBuffer command_buffer = begin_commands(self->instance);

    VkBufferCopy copy = {0, 0, self->size};
    self->instance->vkCmdCopyBuffer(
        command_buffer,
        self->buffer,
        temp.buffer,
        1,
        &copy
    );

    end_commands(self->instance);

    PyObject * res = PyBytes_FromStringAndSize((char *)temp.ptr, self->size);
    free_temp_buffer(self->instance, &temp);
    return res;
}

PyObject * Buffer_meth_write(Buffer * self, PyObject * arg) {
    if (self->staging_buffer) {
        PyErr_Format(PyExc_ValueError, "staged");
        return NULL;
    }

    Py_buffer view = {};
    if (PyObject_GetBuffer(arg, &view, PyBUF_STRIDED_RO)) {
        return NULL;
    }

    if ((VkDeviceSize)view.len > self->size) {
        PyErr_Format(PyExc_ValueError, "wrong size");
        return NULL;
    }

    HostBuffer temp = {};
    new_temp_buffer(self->instance, &temp, self->size);

    PyBuffer_ToContiguous(temp.ptr, &view, view.len, 'C');
    PyBuffer_Release(&view);

    VkCommandBuffer command_buffer = begin_commands(self->instance);

    VkBufferCopy copy = {0, 0, self->size};
    self->instance->vkCmdCopyBuffer(
        command_buffer,
        temp.buffer,
        self->buffer,
        1,
        &copy
    );

    end_commands(self->instance);
    free_temp_buffer(self->instance, &temp);
    Py_RETURN_NONE;
}
