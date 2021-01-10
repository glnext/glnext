#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <vulkan/vulkan_core.h>

struct Context;
struct Memory;
struct Renderer;
struct Buffer;
struct Image;

enum PackType {
    PACK_FLOAT,
    PACK_INT,
    PACK_BYTE,
    PACK_PAD,
};

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
    VkSampleCountFlagBits samples;

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
    VkDeviceSize offset;
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
PyObject * format_dict;

void register_format(const char * name, VkFormat format, int size, PackType pack_type, int pack_count) {
    PyObject * item = Py_BuildValue("IIII", format, size, pack_type, pack_count);
    PyDict_SetItemString(format_dict, name, item);
    Py_DECREF(item);
}

VkCommandBuffer begin_commands(Instance * self) {
    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        NULL,
    };

    vkBeginCommandBuffer(self->command_buffer, &command_buffer_begin_info);
    return self->command_buffer;
}

void end_commands(Instance * self) {
    vkEndCommandBuffer(self->command_buffer);

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        0,
        NULL,
        NULL,
        1,
        &self->command_buffer,
        0,
        NULL,
    };

    vkQueueSubmit(self->queue, 1, &submit_info, self->fence);
    vkWaitForFences(self->device, 1, &self->fence, true, UINT64_MAX);
    vkResetFences(self->device, 1, &self->fence);
}

Memory * new_memory(Instance * self, bool host) {
    Memory * res = PyObject_New(Memory, Memory_type);
    res->instance = self;
    res->host = host;
    res->memory = NULL;
    res->ptr = NULL;
    res->offset = 0;
    res->size = 0;
    PyList_Append(self->memory_list, (PyObject *)res);
    return res;
}

VkDeviceSize take_memory(Memory * self, VkMemoryRequirements requirements) {
    if (VkDeviceSize padding = self->offset % requirements.alignment) {
        self->offset += requirements.alignment - padding;
    }
    VkDeviceSize res = self->offset;
    self->offset += requirements.size;
    return res;
}

Buffer * new_buffer(Instance * self, VkDeviceSize size, VkBufferUsageFlags usage, Memory * memory) {
    Buffer * res = PyObject_New(Buffer, Buffer_type);
    res->instance = self;
    res->memory = memory;
    res->usage = usage;
    res->buffer = NULL;
    res->size = size;

    if (!size) {
        return res;
    }

    VkBufferCreateInfo buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0,
        size,
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
    };

    vkCreateBuffer(self->device, &buffer_create_info, NULL, &res->buffer);

    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(self->device, res->buffer, &requirements);
    res->offset = take_memory(memory, requirements);

    PyList_Append(self->buffer_list, (PyObject *)res);
    return res;
}

Image * new_image(Instance * self, VkDeviceSize size, VkExtent3D extent, VkFormat format, VkFlags usage, VkSampleCountFlagBits samples, Memory * memory) {
    Image * res = PyObject_New(Image, Image_type);
    res->instance = self;
    res->memory = memory;
    res->image = NULL;
    res->size = size;
    res->extent = extent;
    res->samples = samples;
    res->format = format;

    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        0,
        VK_IMAGE_TYPE_2D,
        format,
        extent,
        1,
        1,
        samples,
        VK_IMAGE_TILING_OPTIMAL,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    vkCreateImage(self->device, &image_create_info, NULL, &res->image);

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(self->device, res->image, &requirements);
    res->offset = take_memory(memory, requirements);

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

    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        NULL,
        "application",
        0,
        "engine",
        0,
        VK_API_VERSION_1_0,
    };

    const char * instance_layer_array[] = {"VK_LAYER_KHRONOS_validation"};
    const char * instance_extension_array[] = {"VK_EXT_debug_utils"};

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        NULL,
        0,
        &application_info,
        1,
        instance_layer_array,
        1,
        instance_extension_array,
    };

    vkCreateInstance(&instance_create_info, NULL, &res->instance);

    uint32_t num_physical_devices = 1;
    vkEnumeratePhysicalDevices(res->instance, &num_physical_devices, &res->physical_device);

    VkPhysicalDeviceMemoryProperties device_memory_properties = {};
    vkGetPhysicalDeviceMemoryProperties(res->physical_device, &device_memory_properties);

    VkPhysicalDeviceFeatures supported_features = {};
    vkGetPhysicalDeviceFeatures(res->physical_device, &supported_features);

    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->host_memory_type_index = i;
            break;
        }
    }

    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->device_memory_type_index = i;
            break;
        }
    }

    res->queue_family_index = 0;
    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo device_queue_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        res->queue_family_index,
        1,
        &queue_priority,
    };

    VkPhysicalDeviceFeatures physical_device_features = {};
    physical_device_features.multiDrawIndirect = supported_features.multiDrawIndirect;

    VkDeviceCreateInfo device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        NULL,
        0,
        1,
        &device_queue_create_info,
        0,
        NULL,
        0,
        NULL,
        &physical_device_features,
    };

    vkCreateDevice(res->physical_device, &device_create_info, NULL, &res->device);
    vkGetDeviceQueue(res->device, res->queue_family_index, 0, &res->queue);

    VkCommandPoolCreateInfo command_pool_create_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        res->queue_family_index,
    };

    vkCreateCommandPool(res->device, &command_pool_create_info, NULL, &res->command_pool);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        res->command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1,
    };

    vkAllocateCommandBuffers(res->device, &command_buffer_allocate_info, &res->command_buffer);

    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0};
    vkCreateFence(res->device, &fence_create_info, NULL, &res->fence);

    res->host_memory = new_memory(res, true);
    res->device_memory = new_memory(res, false);

    res->host_buffer = new_buffer(res, host_memory_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, res->host_memory);

    return res;
}

Context * Instance_meth_context(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", "format", "samples", NULL};

    uint32_t width;
    uint32_t height;
    const char * format;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_4_BIT;
    Memory * memory = NULL;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "(II)s|IO!",
        keywords,
        &width,
        &height,
        &format,
        &samples,
        Memory_type,
        &memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (!memory) {
        memory = new_memory(self, false);
    }

    Context * res = PyObject_New(Context, Context_type);
    res->instance = self;

    res->renderer_list = PyList_New(0);

    res->uniform_buffer = new_buffer(self, 64 * 1024, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memory);

    res->depth_image = new_image(
        self,
        width * height * samples * 4,
        {width, height, 1},
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        samples,
        memory
    );

    res->width = width;
    res->height = height;
    res->samples = samples;

    PyList_Append(self->context_list, (PyObject *)res);
    return res;
}

Memory * Instance_meth_memory(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", NULL};

    VkDeviceSize size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "k", keywords, &size)) {
        return NULL;
    }

    Memory * res = new_memory(self, false);
    return res;
}

Image * Instance_meth_image(Instance * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"size", "format", "memory", NULL};

    uint32_t width;
    uint32_t height;
    const char * format_str;
    Memory * memory = NULL;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "(II)s|O!",
        keywords,
        &width,
        &height,
        &format_str,
        Memory_type,
        &memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (!memory) {
        memory = new_memory(self, false);
    }

    Image * res = new_image(
        self,
        width * height * 4,
        {width, height, 1},
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_SAMPLE_COUNT_1_BIT,
        memory
    );

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
    Memory * memory = NULL;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|$O!O!OOIIIIsO!",
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
        &topology_str,
        Memory_type,
        &memory
    );

    if (!args_ok) {
        return NULL;
    }

    if (!memory) {
        memory = new_memory(self->instance, false);
    }

    Renderer * res = PyObject_New(Renderer, Renderer_type);
    res->ctx = self;
    res->memory = memory;
    res->enabled = true;

    res->vertex_buffer = new_buffer(
        self->instance,
        vertex_count * 12, // TODO: from format
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        memory
    );

    res->instance_buffer = new_buffer(
        self->instance,
        instance_count * 12, // TODO: from format
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        memory
    );

    res->index_buffer = new_buffer(
        self->instance,
        index_count * 4,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        memory
    );

    res->indirect_buffer = new_buffer(
        self->instance,
        indirect_count * 20, // TODO: indexed ? 20 : 16
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        memory
    );

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

    VkCommandBuffer cmd = begin_commands(self->instance);

    // TODO: draw

    end_commands(self->instance);

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

PyObject * Image_meth_write(Image * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"data", NULL};

    PyObject * data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", keywords, &data)) {
        return NULL;
    }

    VkCommandBuffer cmd = begin_commands(self->instance);

    // TODO: layout transition and copy buffer to image

    end_commands(self->instance);
    Py_RETURN_NONE;
}

PyObject * Instance_meth_info(Instance * self) {
    Py_RETURN_NONE;
}

PyObject * Instance_meth_release(Instance * self) {
    Py_RETURN_NONE;
}

PyObject * glnext_meth_pack(PyObject * self, PyObject ** args, Py_ssize_t nargs) {
    if (nargs != 1 && nargs != 2) {
        return NULL;
    }

    PyObject * seq = PySequence_Fast(args[nargs - 1], "");
    PyObject ** array = PySequence_Fast_ITEMS(seq);

    int row_size = 0;
    int format_count = 0;
    PackType format_array[256];

    if (nargs == 1) {
        format_array[format_count++] = PyLong_CheckExact(array[0]) ? PACK_INT : PACK_FLOAT;
        row_size = 4;
    } else {
        PyObject * vformat = PyUnicode_Split(args[0], NULL, -1);
        if (!vformat) {
            return NULL;
        }
        for (int k = 0; k < PyList_Size(vformat); ++k) {
            PyObject * format = PyDict_GetItem(format_dict, PyList_GetItem(vformat, k));
            PackType pack_type = (PackType)PyLong_AsUnsignedLong(PyTuple_GetItem(format, 2));
            int pack_items = PyLong_AsLong(PyTuple_GetItem(format, 3));
            row_size += PyLong_AsLong(PyTuple_GetItem(format, 1));
            while (pack_items--) {
                format_array[format_count++] = pack_type;
            }
        }
        Py_DECREF(vformat);
    }

    int rows = (int)PySequence_Fast_GET_SIZE(seq) / format_count;

    PyObject * res = PyBytes_FromStringAndSize(NULL, rows * row_size);
    char * data = PyBytes_AsString(res);

    for (int i = 0; i < rows; ++i) {
        for (int k = 0; k < format_count; ++k) {
            switch (format_array[k]) {
                case PACK_FLOAT:
                    *(*(float **)&data)++ = (float)PyFloat_AsDouble(*array++);
                    break;
                case PACK_INT:
                    *(*(int **)&data)++ = PyLong_AsLong(*array++);
                    break;
                case PACK_BYTE:
                    *(*(uint8_t **)&data)++ = (uint8_t)PyLong_AsUnsignedLong(*array++);
                    break;
                case PACK_PAD:
                    *data++ = 0;
                    break;
            }
        }
    }

    return res;
}

void default_dealloc(PyObject * self) {
    Py_TYPE(self)->tp_free(self);
}

PyMemberDef Instance_members[] = {
    {"device_memory", T_OBJECT, offsetof(Instance, device_memory), READONLY, NULL},
    {},
};

PyMethodDef Instance_methods[] = {
    {"context", (PyCFunction)Instance_meth_context, METH_VARARGS | METH_KEYWORDS, NULL},
    {"memory", (PyCFunction)Instance_meth_memory, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image", (PyCFunction)Instance_meth_image, METH_VARARGS | METH_KEYWORDS, NULL},
    {"release", (PyCFunction)Instance_meth_release, METH_NOARGS, NULL},
    {"info", (PyCFunction)Instance_meth_info, METH_NOARGS, NULL},
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

PyMethodDef Image_methods[] = {
    {"write", (PyCFunction)Image_meth_write, METH_VARARGS | METH_KEYWORDS, NULL},
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
    {Py_tp_methods, Image_methods},
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
    {"pack", (PyCFunction)glnext_meth_pack, METH_FASTCALL, NULL},
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

    empty_str = PyUnicode_FromString("");

    format_dict = PyDict_New();

    register_format("1f", VK_FORMAT_R32_SFLOAT, 4, PACK_FLOAT, 1);
    register_format("2f", VK_FORMAT_R32G32_SFLOAT, 8, PACK_FLOAT, 2);
    register_format("3f", VK_FORMAT_R32G32B32_SFLOAT, 12, PACK_FLOAT, 3);
    register_format("4f", VK_FORMAT_R32G32B32A32_SFLOAT, 16, PACK_FLOAT, 4);
    register_format("1i", VK_FORMAT_R32_SINT, 4, PACK_INT, 1);
    register_format("2i", VK_FORMAT_R32G32_SINT, 8, PACK_INT, 2);
    register_format("3i", VK_FORMAT_R32G32B32_SINT, 12, PACK_INT, 3);
    register_format("4i", VK_FORMAT_R32G32B32A32_SINT, 16, PACK_INT, 4);
    register_format("1b", VK_FORMAT_R8_UNORM, 1, PACK_BYTE, 1);
    register_format("2b", VK_FORMAT_R8G8_UNORM, 2, PACK_BYTE, 2);
    register_format("3b", VK_FORMAT_R8G8B8_UNORM, 3, PACK_BYTE, 3);
    register_format("4b", VK_FORMAT_B8G8R8A8_UNORM, 4, PACK_BYTE, 4);
    register_format("1x", VK_FORMAT_UNDEFINED, 1, PACK_PAD, 1);
    register_format("2x", VK_FORMAT_UNDEFINED, 2, PACK_PAD, 2);
    register_format("3x", VK_FORMAT_UNDEFINED, 3, PACK_PAD, 3);
    register_format("4x", VK_FORMAT_UNDEFINED, 4, PACK_PAD, 4);

    return module;
}
