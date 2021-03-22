#include "glnext.hpp"

#include "binding.cpp"
#include "buffer.cpp"
#include "compute_pipeline.cpp"
#include "debug.cpp"
#include "extension.cpp"
#include "framebuffer.cpp"
#include "group.cpp"
#include "image.cpp"
#include "info.cpp"
#include "instance.cpp"
#include "loader.cpp"
#include "render_pipeline.cpp"
#include "surface.cpp"
#include "task.cpp"
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
    {"buffer", (PyCFunction)Instance_meth_buffer, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image", (PyCFunction)Instance_meth_image, METH_VARARGS | METH_KEYWORDS, NULL},
    {"surface", (PyCFunction)Instance_meth_surface, METH_VARARGS | METH_KEYWORDS, NULL},
    {"task", (PyCFunction)Instance_meth_task, METH_NOARGS, NULL},
    {"cache", (PyCFunction)Instance_meth_cache, METH_NOARGS, NULL},
    {"present", (PyCFunction)Instance_meth_present, METH_NOARGS, NULL},
    {"group", (PyCFunction)Instance_meth_group, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Task_methods[] = {
    {"framebuffer", (PyCFunction)Task_meth_framebuffer, METH_VARARGS | METH_KEYWORDS, NULL},
    {"compute", (PyCFunction)Task_meth_compute, METH_VARARGS | METH_KEYWORDS, NULL},
    {"run", (PyCFunction)Task_meth_run, METH_NOARGS, NULL},
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

PyMethodDef Group_methods[] = {
    {"__enter__", (PyCFunction)Group_meth_enter, METH_NOARGS, NULL},
    {"__exit__", (PyCFunction)Group_meth_exit, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyGetSetDef Surface_getset[] = {
    {"image", (getter)Surface_get_image, (setter)Surface_set_image, NULL, NULL},
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

PyMemberDef Group_members[] = {
    {"output", T_OBJECT_EX, offsetof(Group, output), READONLY, NULL},
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

PyType_Slot Surface_slots[] = {
    {Py_tp_getset, Surface_getset},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Task_slots[] = {
    {Py_tp_methods, Task_methods},
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

PyType_Slot Group_slots[] = {
    {Py_tp_methods, Group_methods},
    {Py_tp_members, Group_members},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Spec Instance_spec = {"glnext.Instance", sizeof(Instance), 0, Py_TPFLAGS_DEFAULT, Instance_slots};
PyType_Spec Surface_spec = {"glnext.Surface", sizeof(Surface), 0, Py_TPFLAGS_DEFAULT, Surface_slots};
PyType_Spec Task_spec = {"glnext.Task", sizeof(Task), 0, Py_TPFLAGS_DEFAULT, Task_slots};
PyType_Spec Framebuffer_spec = {"glnext.Framebuffer", sizeof(Framebuffer), 0, Py_TPFLAGS_DEFAULT, Framebuffer_slots};
PyType_Spec RenderPipeline_spec = {"glnext.RenderPipeline", sizeof(RenderPipeline), 0, Py_TPFLAGS_DEFAULT, RenderPipeline_slots};
PyType_Spec ComputePipeline_spec = {"glnext.ComputePipeline", sizeof(ComputePipeline), 0, Py_TPFLAGS_DEFAULT, ComputePipeline_slots};
PyType_Spec Memory_spec = {"glnext.Memory", sizeof(Memory), 0, Py_TPFLAGS_DEFAULT, Memory_slots};
PyType_Spec Buffer_spec = {"glnext.Buffer", sizeof(Buffer), 0, Py_TPFLAGS_DEFAULT, Buffer_slots};
PyType_Spec Image_spec = {"glnext.Image", sizeof(Image), 0, Py_TPFLAGS_DEFAULT, Image_slots};
PyType_Spec Group_spec = {"glnext.Group", sizeof(Group), 0, Py_TPFLAGS_DEFAULT, Group_slots};

int module_exec(PyObject * self) {
    ModuleState * state = (ModuleState *)PyModule_GetState(self);

    state->Instance_type = (PyTypeObject *)PyType_FromSpec(&Instance_spec);
    state->Surface_type = (PyTypeObject *)PyType_FromSpec(&Surface_spec);
    state->Task_type = (PyTypeObject *)PyType_FromSpec(&Task_spec);
    state->Framebuffer_type = (PyTypeObject *)PyType_FromSpec(&Framebuffer_spec);
    state->RenderPipeline_type = (PyTypeObject *)PyType_FromSpec(&RenderPipeline_spec);
    state->ComputePipeline_type = (PyTypeObject *)PyType_FromSpec(&ComputePipeline_spec);
    state->Memory_type = (PyTypeObject *)PyType_FromSpec(&Memory_spec);
    state->Buffer_type = (PyTypeObject *)PyType_FromSpec(&Buffer_spec);
    state->Image_type = (PyTypeObject *)PyType_FromSpec(&Image_spec);
    state->Group_type = (PyTypeObject *)PyType_FromSpec(&Group_spec);

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
