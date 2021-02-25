#include "glnext.hpp"

#include "batch.cpp"
#include "binding.cpp"
#include "buffer.cpp"
#include "compute_pipeline.cpp"
#include "debug.cpp"
#include "extension.cpp"
#include "framebuffer.cpp"
#include "image.cpp"
#include "info.cpp"
#include "instance.cpp"
#include "loader.cpp"
#include "render_pipeline.cpp"
#include "staging_buffer.cpp"
#include "surface.cpp"
#include "tools.cpp"
#include "utils.cpp"

PyMethodDef module_methods[] = {
    {"info", (PyCFunction)glnext_meth_info, METH_VARARGS | METH_KEYWORDS, NULL},
    {"instance", (PyCFunction)glnext_meth_instance, METH_VARARGS | METH_KEYWORDS, NULL},
    {"camera", (PyCFunction)glnext_meth_camera, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rgba", (PyCFunction)glnext_meth_rgba, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pack", (PyCFunction)glnext_meth_pack, METH_FASTCALL, NULL},
    {},
};

PyMethodDef Instance_methods[] = {
    {"batch", (PyCFunction)Instance_meth_batch, METH_VARARGS | METH_KEYWORDS, NULL},
    {"framebuffer", (PyCFunction)Instance_meth_framebuffer, METH_VARARGS | METH_KEYWORDS, NULL},
    {"compute", (PyCFunction)Instance_meth_compute, METH_VARARGS | METH_KEYWORDS, NULL},
    {"buffer", (PyCFunction)Instance_meth_buffer, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image", (PyCFunction)Instance_meth_image, METH_VARARGS | METH_KEYWORDS, NULL},
    {"staging", (PyCFunction)Instance_meth_staging, METH_VARARGS | METH_KEYWORDS, NULL},
    {"surface", (PyCFunction)Instance_meth_surface, METH_VARARGS | METH_KEYWORDS, NULL},
    {"cache", (PyCFunction)Instance_meth_cache, METH_NOARGS, NULL},
    {"run", (PyCFunction)Instance_meth_run, METH_NOARGS, NULL},
    {},
};

PyMethodDef Batch_methods[] = {
    {"run", (PyCFunction)Batch_meth_run, METH_NOARGS, NULL},
    {},
};

PyMethodDef Framebuffer_methods[] = {
    {"compute", (PyCFunction)Framebuffer_meth_compute, METH_VARARGS | METH_KEYWORDS, NULL},
    {"render", (PyCFunction)Framebuffer_meth_render, METH_VARARGS | METH_KEYWORDS, NULL},
    {"update", (PyCFunction)Framebuffer_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef RenderPipeline_methods[] = {
    {"update", (PyCFunction)RenderPipeline_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef ComputePipeline_methods[] = {
    {"update", (PyCFunction)ComputePipeline_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Buffer_methods[] = {
    {"read", (PyCFunction)Buffer_meth_read, METH_NOARGS, NULL},
    {"write", (PyCFunction)Buffer_meth_write, METH_O, NULL},
    {},
};

PyMethodDef Image_methods[] = {
    {"read", (PyCFunction)Image_meth_read, METH_NOARGS, NULL},
    {"write", (PyCFunction)Image_meth_write, METH_O, NULL},
    {},
};

PyMethodDef StagingBuffer_methods[] = {
    {"read", (PyCFunction)StagingBuffer_meth_read, METH_NOARGS, NULL},
    {"write", (PyCFunction)StagingBuffer_meth_write, METH_O, NULL},
    {},
};

PyMemberDef Instance_members[] = {
    {"log", T_OBJECT_EX, offsetof(Instance, log_list), READONLY, NULL},
    {},
};

PyMemberDef Framebuffer_members[] = {
    {"output", T_OBJECT_EX, offsetof(Framebuffer, output), READONLY, NULL},
    {},
};

PyMemberDef RenderPipeline_members[] = {
    {"vertex_buffer", T_OBJECT_EX, offsetof(RenderPipeline, vertex_buffer), READONLY, NULL},
    {},
};

PyMemberDef StagingBuffer_members[] = {
    {"mem", T_OBJECT_EX, offsetof(StagingBuffer, mem), READONLY, NULL},
    {"ptr", T_ULONGLONG, offsetof(StagingBuffer, ptr), READONLY, NULL},
    {},
};

void default_dealloc(PyObject * self) {
    Py_TYPE(self)->tp_free(self);
}

PyType_Slot Instance_slots[] = {
    {Py_tp_methods, Instance_methods},
    {Py_tp_members, Instance_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Batch_slots[] = {
    {Py_tp_methods, Batch_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Framebuffer_slots[] = {
    {Py_tp_methods, Framebuffer_methods},
    {Py_tp_members, Framebuffer_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot RenderPipeline_slots[] = {
    {Py_tp_methods, RenderPipeline_methods},
    {Py_tp_members, RenderPipeline_members},
    {Py_mp_subscript, RenderPipeline_subscript},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot ComputePipeline_slots[] = {
    {Py_tp_methods, ComputePipeline_methods},
    {Py_mp_subscript, ComputePipeline_subscript},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Memory_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Buffer_slots[] = {
    {Py_tp_methods, Buffer_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Image_slots[] = {
    {Py_tp_methods, Image_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot StagingBuffer_slots[] = {
    {Py_tp_methods, StagingBuffer_methods},
    {Py_tp_members, StagingBuffer_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Spec Instance_spec = {"glnext.Instance", sizeof(Instance), 0, Py_TPFLAGS_DEFAULT, Instance_slots};
PyType_Spec Batch_spec = {"glnext.Batch", sizeof(Batch), 0, Py_TPFLAGS_DEFAULT, Batch_slots};
PyType_Spec Framebuffer_spec = {"glnext.Framebuffer", sizeof(Framebuffer), 0, Py_TPFLAGS_DEFAULT, Framebuffer_slots};
PyType_Spec RenderPipeline_spec = {"glnext.RenderPipeline", sizeof(RenderPipeline), 0, Py_TPFLAGS_DEFAULT, RenderPipeline_slots};
PyType_Spec ComputePipeline_spec = {"glnext.ComputePipeline", sizeof(ComputePipeline), 0, Py_TPFLAGS_DEFAULT, ComputePipeline_slots};
PyType_Spec Memory_spec = {"glnext.Memory", sizeof(Memory), 0, Py_TPFLAGS_DEFAULT, Memory_slots};
PyType_Spec Buffer_spec = {"glnext.Buffer", sizeof(Buffer), 0, Py_TPFLAGS_DEFAULT, Buffer_slots};
PyType_Spec Image_spec = {"glnext.Image", sizeof(Image), 0, Py_TPFLAGS_DEFAULT, Image_slots};
PyType_Spec StagingBuffer_spec = {"glnext.StagingBuffer", sizeof(StagingBuffer), 0, Py_TPFLAGS_DEFAULT, StagingBuffer_slots};

int module_exec(PyObject * self) {
    ModuleState * state = (ModuleState *)PyModule_GetState(self);

    state->Instance_type = (PyTypeObject *)PyType_FromSpec(&Instance_spec);
    state->Batch_type = (PyTypeObject *)PyType_FromSpec(&Batch_spec);
    state->Framebuffer_type = (PyTypeObject *)PyType_FromSpec(&Framebuffer_spec);
    state->RenderPipeline_type = (PyTypeObject *)PyType_FromSpec(&RenderPipeline_spec);
    state->ComputePipeline_type = (PyTypeObject *)PyType_FromSpec(&ComputePipeline_spec);
    state->Memory_type = (PyTypeObject *)PyType_FromSpec(&Memory_spec);
    state->StagingBuffer_type = (PyTypeObject *)PyType_FromSpec(&StagingBuffer_spec);
    state->Buffer_type = (PyTypeObject *)PyType_FromSpec(&Buffer_spec);
    state->Image_type = (PyTypeObject *)PyType_FromSpec(&Image_spec);

    state->empty_str = PyUnicode_FromString("");
    state->empty_list = PyList_New(0);

    state->default_topology = PyUnicode_FromString("triangles");
    state->default_front_face = PyUnicode_FromString("counter_clockwise");
    state->default_format = PyUnicode_FromString("4p");
    state->one_float_str = PyUnicode_FromString("1f");
    state->one_int_str = PyUnicode_FromString("1i");
    state->texture_str = PyUnicode_FromString("texture");
    state->output_str = PyUnicode_FromString("output");

    return 0;
}

PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {},
};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "glnext", NULL, sizeof(ModuleState), module_methods, module_slots};

extern "C" PyObject * PyInit_glnext() {
    return PyModuleDef_Init(&module_def);
}
