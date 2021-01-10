#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <vulkan/vulkan_core.h>

struct Context;
struct Memory;
struct Renderer;
struct Buffer;
struct Image;

struct Instance {
    PyObject_HEAD

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkFence fence;

    uint32_t queue_family_index;
    uint32_t host_memory_type_index;
    uint32_t device_memory_type_index;

    Memory * device_memory;
    Memory * host_memory;
    Buffer * host_buffer;

    PyObject * context_list;
    PyObject * memory_list;
    PyObject * buffer_list;
    PyObject * image_list;
};

struct Context {
    PyObject_HEAD

    Instance * instance;

    uint32_t width;
    uint32_t height;
    uint32_t samples;

    VkRenderPass render_pass;
    VkFramebuffer framebuffer;

    Image * depth_image;
    Buffer * uniform_buffer;

    PyObject * renderer_list;
};

struct Memory {
    PyObject_HEAD

    Instance * instance;

    VkBool32 host;
    VkDeviceSize size;
    VkDeviceMemory memory;
    void * ptr;
};

struct Renderer {
    PyObject_HEAD

    Context * ctx;
    Memory * memory;

    Buffer * vertex_buffer;
    Buffer * instance_buffer;
    Buffer * index_buffer;
    Buffer * indirect_buffer;

    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t index_count;
    uint32_t indirect_count;
    VkBool32 enabled;

    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    VkPipeline pipeline;
};

struct Buffer {
    PyObject_HEAD

    Instance * instance;
    Memory * memory;

    VkBufferUsageFlags usage;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkBuffer buffer;
};

struct Image {
    PyObject_HEAD

    Instance * instance;
    Memory * memory;

    VkDeviceSize offset;
    VkDeviceSize size;
    VkExtent3D extent;
    VkSampleCountFlagBits samples;
    VkFormat format;
    VkImage image;
};

PyTypeObject * Instance_type;
PyTypeObject * Context_type;
PyTypeObject * Memory_type;
PyTypeObject * Renderer_type;
PyTypeObject * Buffer_type;
PyTypeObject * Image_type;

PyObject * empty_str;

Memory * new_memory(Instance * self) {
    Memory * res = PyObject_New(Memory, Memory_type);
    res->instance = self;
    PyList_Append(self->memory_list, (PyObject *)res);
    return res;
}

Buffer * new_buffer(Instance * self) {
    Buffer * res = PyObject_New(Buffer, Buffer_type);
    res->instance = self;
    PyList_Append(self->buffer_list, (PyObject *)res);
    return res;
}

Image * new_image(Instance * self) {
    Image * res = PyObject_New(Image, Image_type);
    res->instance = self;
    PyList_Append(self->image_list, (PyObject *)res);
    return res;
}

Instance * glnext_meth_instance(PyObject * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"host_memory", "device_memory", NULL};

    VkDeviceSize host_memory_size = 0;
    VkDeviceSize device_memory_size = 0;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|$kk",
        keywords,
        &host_memory_size,
        &device_memory_size
    );

    if (!args_ok) {
        return NULL;
    }

    if (host_memory_size < 64 * 1024) {
        host_memory_size = 64 * 1024;
    }

    Instance * res = PyObject_New(Instance, Instance_type);

    res->context_list = PyList_New(0);
    res->memory_list = PyList_New(0);
    res->buffer_list = PyList_New(0);
    res->image_list = PyList_New(0);

    res->host_memory = new_memory(res);
    res->device_memory = new_memory(res);

    res->host_buffer = new_buffer(res);

    return res;
}

Context * Instance_meth_context(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", "format", "samples", NULL};

    uint32_t width;
    uint32_t height;
    const char * format;
    uint32_t samples;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "(II)sI", keywords, &width, &height, &format, &samples)) {
        return NULL;
    }

    Context * res = PyObject_New(Context, Context_type);
    res->instance = self;

    res->uniform_buffer = new_buffer(self);
    res->depth_image = new_image(self);

    res->width = width;
    res->height = height;
    res->samples = samples;

    PyList_Append(self->context_list, (PyObject *)res);
    return res;
}

Memory * Instance_meth_memory(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", NULL};

    uint64_t size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k", keywords, &size)) {
        return NULL;
    }

    Memory * res = new_memory(self);
    return res;
}

Image * Instance_meth_image(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", "format", NULL};

    uint32_t width;
    uint32_t height;
    const char * format;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "(II)s", keywords, &width, &height, &format)) {
        return NULL;
    }

    Image * res = new_image(self);
    return res;
}

Renderer * Context_meth_renderer(Context * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_shader",
        "fragment_shader",
        "vertex_format",
        "instance_format",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        "topology",
        NULL,
    };

    PyObject * vertex_shader = Py_None;
    PyObject * fragment_shader = Py_None;
    PyObject * vertex_format = empty_str;
    PyObject * instance_format = empty_str;
    uint32_t vertex_count = 0;
    uint32_t instance_count = 1;
    uint32_t index_count = 0;
    uint32_t indirect_count = 0;
    const char * topology_str = "triangles";

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|$O!O!OOIIIIs",
        keywords,
        &PyBytes_Type,
        &vertex_shader,
        &PyBytes_Type,
        &fragment_shader,
        &vertex_format,
        &instance_format,
        &vertex_count,
        &instance_count,
        &index_count,
        &indirect_count,
        &topology_str
    );

    if (!args_ok) {
        return NULL;
    }

    Renderer * res = PyObject_New(Renderer, Renderer_type);
    res->ctx = self;

    res->vertex_buffer = new_buffer(self->instance);
    res->instance_buffer = new_buffer(self->instance);
    res->index_buffer = new_buffer(self->instance);
    res->indirect_buffer = new_buffer(self->instance);

    PyList_Append(self->renderer_list, (PyObject *)res);
    return res;
}

PyObject * Context_meth_update(Context * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"uniform_buffer", "clear_value_buffer", NULL};

    PyObject * uniform_buffer = Py_None;
    PyObject * clear_value_buffer = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$OO", keywords, &uniform_buffer, &clear_value_buffer)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyObject * Context_meth_render(Context * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", NULL};

    uint32_t width = self->width;
    uint32_t height = self->height;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|(II)", keywords, &width, &height)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyObject * Renderer_meth_update(Renderer * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {
        "vertex_buffer",
        "instance_buffer",
        "index_buffer",
        "indirect_buffer",
        "vertex_count",
        "instance_count",
        "index_count",
        "indirect_count",
        "enabled",
        NULL,
    };

    PyObject * vertex_buffer = Py_None;
    PyObject * instance_buffer = Py_None;
    PyObject * index_buffer = Py_None;
    PyObject * indirect_buffer = Py_None;

    uint32_t vertex_count = self->vertex_count;
    uint32_t instance_count = self->instance_count;
    uint32_t index_count = self->index_count;
    uint32_t indirect_count = self->indirect_count;
    VkBool32 enabled = self->enabled;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|$OOOOIIIIp",
        keywords,
        &vertex_buffer,
        &instance_buffer,
        &index_buffer,
        &indirect_buffer,
        &vertex_count,
        &instance_count,
        &index_count,
        &indirect_count,
        &enabled
    );

    if (!args_ok) {
        return NULL;
    }

    self->vertex_count = vertex_count;
    self->instance_count = instance_count;
    self->index_count = index_count;
    self->indirect_count = indirect_count;
    self->enabled = enabled;

    Py_RETURN_NONE;
}

void default_dealloc(PyObject * self) {
    Py_TYPE(self)->tp_free(self);
}

PyMethodDef Instance_methods[] = {
    {"context", (PyCFunction)Instance_meth_context, METH_VARARGS | METH_KEYWORDS, NULL},
    {"memory", (PyCFunction)Instance_meth_memory, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image", (PyCFunction)Instance_meth_image, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Context_methods[] = {
    {"renderer", (PyCFunction)Context_meth_renderer, METH_VARARGS | METH_KEYWORDS, NULL},
    {"update", (PyCFunction)Context_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {"render", (PyCFunction)Context_meth_render, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyMethodDef Renderer_methods[] = {
    {"update", (PyCFunction)Renderer_meth_update, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyType_Slot Instance_slots[] = {
    {Py_tp_methods, Instance_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Context_slots[] = {
    {Py_tp_methods, Context_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Memory_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Renderer_slots[] = {
    {Py_tp_methods, Renderer_methods},
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Buffer_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Slot Image_slots[] = {
    {Py_tp_dealloc, default_dealloc},
    {},
};

PyType_Spec Instance_spec = {"glnext.Instance", sizeof(Instance), 0, Py_TPFLAGS_DEFAULT, Instance_slots};
PyType_Spec Context_spec = {"glnext.Context", sizeof(Context), 0, Py_TPFLAGS_DEFAULT, Context_slots};
PyType_Spec Memory_spec = {"glnext.Memory", sizeof(Memory), 0, Py_TPFLAGS_DEFAULT, Memory_slots};
PyType_Spec Renderer_spec = {"glnext.Renderer", sizeof(Renderer), 0, Py_TPFLAGS_DEFAULT, Renderer_slots};
PyType_Spec Buffer_spec = {"glnext.Buffer", sizeof(Buffer), 0, Py_TPFLAGS_DEFAULT, Buffer_slots};
PyType_Spec Image_spec = {"glnext.Image", sizeof(Image), 0, Py_TPFLAGS_DEFAULT, Image_slots};

PyMethodDef module_methods[] = {
    {"instance", (PyCFunction)glnext_meth_instance, METH_VARARGS | METH_KEYWORDS, NULL},
    {},
};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "glnext", NULL, -1, module_methods};

extern "C" PyObject * PyInit_glnext() {
    PyObject * module = PyModule_Create(&module_def);

    Instance_type = (PyTypeObject *)PyType_FromSpec(&Instance_spec);
    Context_type = (PyTypeObject *)PyType_FromSpec(&Context_spec);
    Memory_type = (PyTypeObject *)PyType_FromSpec(&Memory_spec);
    Renderer_type = (PyTypeObject *)PyType_FromSpec(&Renderer_spec);
    Buffer_type = (PyTypeObject *)PyType_FromSpec(&Buffer_spec);
    Image_type = (PyTypeObject *)PyType_FromSpec(&Image_spec);

    return module;
}
