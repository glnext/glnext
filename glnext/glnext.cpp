#include "glnext.hpp"

#include "buffer.cpp"
#include "compute_set.cpp"
#include "encoder.cpp"
#include "image.cpp"
#include "instance.cpp"
#include "memory.cpp"
#include "pipeline.cpp"
#include "render_set.cpp"
#include "staging_buffer.cpp"
#include "surface.cpp"
#include "tools.cpp"
#include "transform.cpp"
#include "utils.cpp"

PyMethodDef module_methods[] = {
    {"instance", (PyCFunction)glnext_meth_instance, METH_VARARGS | METH_KEYWORDS, NULL},
    {"camera", (PyCFunction)glnext_meth_camera, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rgba", (PyCFunction)glnext_meth_rgba, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pack", (PyCFunction)glnext_meth_pack, METH_FASTCALL, NULL},
    {},
};

PyMethodDef Instance_methods[] = {
    {"compute_set", (PyCFunction)Instance_meth_compute_set, METH_VARARGS | METH_KEYWORDS, NULL},
    {"render_set", (PyCFunction)Instance_meth_render_set, METH_VARARGS | METH_KEYWORDS, NULL},
    {"transform", (PyCFunction)Instance_meth_transform, METH_VARARGS | METH_KEYWORDS, NULL},
    {"encoder", (PyCFunction)Instance_meth_encoder, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image", (PyCFunction)Instance_meth_image, METH_VARARGS | METH_KEYWORDS, NULL},
    {"memory", (PyCFunction)Instance_meth_memory, METH_VARARGS | METH_KEYWORDS, NULL},
    {"staging_buffer", (PyCFunction)Instance_meth_staging_buffer, METH_O, NULL},
    {"surface", (PyCFunction)Instance_meth_surface, METH_VARARGS | METH_KEYWORDS, NULL},
    {"render", (PyCFunction)Instance_meth_render, METH_NOARGS, NULL},
    {},
};

PyMethodDef ComputeSet_methods[] = {
    {"update", (PyCFunction)ComputeSet_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef RenderSet_methods[] = {
    {"pipeline", (PyCFunction)RenderSet_meth_pipeline, METH_VARARGS | METH_KEYWORDS, NULL},
    {"update", (PyCFunction)RenderSet_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Pipeline_methods[] = {
    {"update", (PyCFunction)Pipeline_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Transform_methods[] = {
    {"update", (PyCFunction)Transform_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Encoder_methods[] = {
    {"update", (PyCFunction)Encoder_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Image_methods[] = {
    {"read", (PyCFunction)Image_meth_read, METH_NOARGS, NULL},
    {"write", (PyCFunction)Image_meth_write, METH_O, NULL},
    {},
};

PyMethodDef Buffer_methods[] = {
    {"read", (PyCFunction)Buffer_meth_read, METH_NOARGS, NULL},
    {"write", (PyCFunction)Buffer_meth_write, METH_O, NULL},
    {},
};

PyMethodDef StagingBuffer_methods[] = {
    {"read", (PyCFunction)StagingBuffer_meth_read, METH_NOARGS, NULL},
    {"write", (PyCFunction)StagingBuffer_meth_write, METH_O, NULL},
    {},
};

PyMemberDef ComputeSet_members[] = {
    {"uniform_buffer", T_OBJECT_EX, offsetof(ComputeSet, uniform_buffer), READONLY, NULL},
    {"storage_buffer", T_OBJECT_EX, offsetof(ComputeSet, storage_buffer), READONLY, NULL},
    {"output", T_OBJECT_EX, offsetof(ComputeSet, output), READONLY, NULL},
    {},
};

PyMemberDef RenderSet_members[] = {
    {"uniform_buffer", T_OBJECT_EX, offsetof(RenderSet, uniform_buffer), READONLY, NULL},
    {"storage_buffer", T_OBJECT_EX, offsetof(RenderSet, storage_buffer), READONLY, NULL},
    {"output", T_OBJECT_EX, offsetof(RenderSet, output), READONLY, NULL},
    {},
};

PyMemberDef Pipeline_members[] = {
    {"uniform_buffer", T_OBJECT_EX, offsetof(Pipeline, uniform_buffer), READONLY, NULL},
    {"storage_buffer", T_OBJECT_EX, offsetof(Pipeline, storage_buffer), READONLY, NULL},
    {"vertex_buffer", T_OBJECT_EX, offsetof(Pipeline, vertex_buffer), READONLY, NULL},
    {"instance_buffer", T_OBJECT_EX, offsetof(Pipeline, instance_buffer), READONLY, NULL},
    {"index_buffer", T_OBJECT_EX, offsetof(Pipeline, index_buffer), READONLY, NULL},
    {"indirect_buffer", T_OBJECT_EX, offsetof(Pipeline, indirect_buffer), READONLY, NULL},
    {},
};

PyMemberDef Transform_members[] = {
    {"uniform_buffer", T_OBJECT_EX, offsetof(Transform, uniform_buffer), READONLY, NULL},
    {"storage_buffer", T_OBJECT_EX, offsetof(Transform, storage_buffer), READONLY, NULL},
    {"output", T_OBJECT_EX, offsetof(Transform, output_buffer), READONLY, NULL},
    {},
};

PyMemberDef Encoder_members[] = {
    {"uniform_buffer", T_OBJECT_EX, offsetof(Encoder, uniform_buffer), READONLY, NULL},
    {"storage_buffer", T_OBJECT_EX, offsetof(Encoder, storage_buffer), READONLY, NULL},
    {"output", T_OBJECT_EX, offsetof(Encoder, output_buffer), READONLY, NULL},
    {},
};

PyMemberDef StagingBuffer_members[] = {
    {"mem", T_OBJECT_EX, offsetof(StagingBuffer, mem), READONLY, NULL},
    {},
};

void default_dealloc(PyObject * self) {
    Py_TYPE(self)->tp_free(self);
}

PyType_Slot Instance_slots[] = {
    {Py_tp_methods, Instance_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot ComputeSet_slots[] = {
    {Py_tp_methods, ComputeSet_methods},
    {Py_tp_members, ComputeSet_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot RenderSet_slots[] = {
    {Py_tp_methods, RenderSet_methods},
    {Py_tp_members, RenderSet_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Pipeline_slots[] = {
    {Py_tp_methods, Pipeline_methods},
    {Py_tp_members, Pipeline_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Transform_slots[] = {
    {Py_tp_methods, Transform_methods},
    {Py_tp_members, Transform_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Encoder_slots[] = {
    {Py_tp_methods, Encoder_methods},
    {Py_tp_members, Encoder_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Image_slots[] = {
    {Py_tp_methods, Image_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Buffer_slots[] = {
    {Py_tp_methods, Buffer_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Memory_slots[] = {
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
PyType_Spec ComputeSet_spec = {"glnext.ComputeSet", sizeof(ComputeSet), 0, Py_TPFLAGS_DEFAULT, ComputeSet_slots};
PyType_Spec RenderSet_spec = {"glnext.RenderSet", sizeof(RenderSet), 0, Py_TPFLAGS_DEFAULT, RenderSet_slots};
PyType_Spec Pipeline_spec = {"glnext.Pipeline", sizeof(Pipeline), 0, Py_TPFLAGS_DEFAULT, Pipeline_slots};
PyType_Spec Transform_spec = {"glnext.Transform", sizeof(Transform), 0, Py_TPFLAGS_DEFAULT, Transform_slots};
PyType_Spec Encoder_spec = {"glnext.Encoder", sizeof(Encoder), 0, Py_TPFLAGS_DEFAULT, Encoder_slots};
PyType_Spec Image_spec = {"glnext.Image", sizeof(Image), 0, Py_TPFLAGS_DEFAULT, Image_slots};
PyType_Spec Buffer_spec = {"glnext.Buffer", sizeof(Buffer), 0, Py_TPFLAGS_DEFAULT, Buffer_slots};
PyType_Spec Memory_spec = {"glnext.Memory", sizeof(Memory), 0, Py_TPFLAGS_DEFAULT, Memory_slots};
PyType_Spec StagingBuffer_spec = {"glnext.StagingBuffer", sizeof(StagingBuffer), 0, Py_TPFLAGS_DEFAULT, StagingBuffer_slots};

int module_exec(PyObject * self) {
    ModuleState * state = (ModuleState *)PyModule_GetState(self);

    state->Instance_type = (PyTypeObject *)PyType_FromSpec(&Instance_spec);
    state->ComputeSet_type = (PyTypeObject *)PyType_FromSpec(&ComputeSet_spec);
    state->RenderSet_type = (PyTypeObject *)PyType_FromSpec(&RenderSet_spec);
    state->Pipeline_type = (PyTypeObject *)PyType_FromSpec(&Pipeline_spec);
    state->Transform_type = (PyTypeObject *)PyType_FromSpec(&Transform_spec);
    state->Encoder_type = (PyTypeObject *)PyType_FromSpec(&Encoder_spec);
    state->Image_type = (PyTypeObject *)PyType_FromSpec(&Image_spec);
    state->Buffer_type = (PyTypeObject *)PyType_FromSpec(&Buffer_spec);
    state->Memory_type = (PyTypeObject *)PyType_FromSpec(&Memory_spec);
    state->StagingBuffer_type = (PyTypeObject *)PyType_FromSpec(&StagingBuffer_spec);

    state->empty_str = PyUnicode_FromString("");
    state->empty_list = PyList_New(0);

    state->default_topology = PyUnicode_FromString("triangles");
    state->default_front_face = PyUnicode_FromString("counter_clockwise");
    state->default_border_color = PyUnicode_FromString("transparent");
    state->default_format = PyUnicode_FromString("4p");
    state->default_filter = PyUnicode_FromString("nearest");
    state->default_address_mode = PyUnicode_FromString("repeat");
    state->default_swizzle = PyUnicode_FromString("rgba");
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
