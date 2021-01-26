#include "glnext.hpp"

Instance * glnext_meth_instance(PyObject * self, PyObject * vargs, PyObject * kwargs) {
    ModuleState * state = (ModuleState *)PyModule_GetState(self);

    static char * keywords[] = {
        "physical_device",
        "application_name",
        "application_version",
        "engine_name",
        "engine_version",
        "layers",
        "debug",
        NULL,
    };

    struct {
        uint32_t physical_device = 0;
        const char * application_name = "application";
        uint32_t application_version = 0;
        const char * engine_name = "engine";
        uint32_t engine_version = 0;
        PyObject * layers = Py_None;
        VkBool32 debug = false;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$IsIsIOp",
        keywords,
        &args.physical_device,
        &args.application_name,
        &args.application_version,
        &args.engine_name,
        &args.engine_version,
        &args.layers,
        &args.debug
    );

    if (!args_ok) {
        return NULL;
    }

    if (args.layers == Py_None) {
        args.layers = state->empty_list;
    }

    Instance * res = PyObject_New(Instance, state->Instance_type);

    res->state = state;
    res->instance = NULL;
    res->physical_device = NULL;
    res->device = NULL;
    res->queue = NULL;
    res->command_pool = NULL;
    res->command_buffer = NULL;
    res->fence = NULL;

    res->presenter = {};

    res->task_list = PyList_New(0);
    res->memory_list = PyList_New(0);
    res->buffer_list = PyList_New(0);
    res->image_list = PyList_New(0);
    res->log_list = PyList_New(0);

    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        NULL,
        args.application_name,
        args.application_version,
        args.engine_name,
        args.engine_version,
        VK_API_VERSION_1_0,
    };

    uint32_t instance_layer_count = 0;
    uint32_t instance_extension_count = 0;
    uint32_t device_extension_count = 0;

    const char * instance_layer_array[64];
    const char * instance_extension_array[8];
    const char * device_extension_array[8];

    for (uint32_t i = 0; i < PyList_Size(args.layers); ++i) {
        instance_layer_array[instance_layer_count++] = PyUnicode_AsUTF8(PyList_GetItem(args.layers, i));
    }

    // instance_extension_array[instance_extension_count++] = SURFACE_EXTENSION;
    // instance_extension_array[instance_extension_count++] = "VK_KHR_surface";
    // device_extension_array[device_extension_count++] = "VK_KHR_swapchain";

    if (args.debug) {
        instance_layer_array[instance_layer_count++] = "VK_LAYER_KHRONOS_validation";
        instance_extension_array[instance_extension_count++] = "VK_EXT_debug_utils";
    }

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        NULL,
        0,
        &application_info,
        instance_layer_count,
        instance_layer_array,
        instance_extension_count,
        instance_extension_array,
    };

    res->instance = NULL;
    vkCreateInstance(&instance_create_info, NULL, &res->instance);

    if (!res->instance) {
        PyErr_Format(PyExc_RuntimeError, "cannot create instance");
        return NULL;
    }

    if (args.debug) {
        install_debug_messenger(res);
    }

    uint32_t physical_device_count = 0;
    VkPhysicalDevice physical_device_array[64] = {};
    vkEnumeratePhysicalDevices(res->instance, &physical_device_count, NULL);
    vkEnumeratePhysicalDevices(res->instance, &physical_device_count, physical_device_array);
    res->physical_device = physical_device_array[args.physical_device];

    if (!res->physical_device) {
        PyErr_Format(PyExc_RuntimeError, "physical device not found");
        return NULL;
    }

    VkPhysicalDeviceMemoryProperties device_memory_properties = {};
    vkGetPhysicalDeviceMemoryProperties(res->physical_device, &device_memory_properties);

    VkPhysicalDeviceFeatures supported_features = {};
    vkGetPhysicalDeviceFeatures(res->physical_device, &supported_features);

    res->host_memory_type_index = 0;
    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->host_memory_type_index = i;
            break;
        }
    }

    res->device_memory_type_index = 0;
    for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if ((device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            res->device_memory_type_index = i;
            break;
        }
    }

    VkFormat depth_formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    for (uint32_t i = 0; i < 3; ++i) {
        VkFormatProperties format_properties = {};
        vkGetPhysicalDeviceFormatProperties(res->physical_device, depth_formats[i], &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            res->depth_format = depth_formats[i];
        }
    }

    uint32_t queue_family_properties_count = 0;
    VkQueueFamilyProperties queue_family_properties_array[64];
    vkGetPhysicalDeviceQueueFamilyProperties(res->physical_device, &queue_family_properties_count, NULL);
    vkGetPhysicalDeviceQueueFamilyProperties(res->physical_device, &queue_family_properties_count, queue_family_properties_array);

    res->queue_family_index = 0;
    for (uint32_t i = 0; i < queue_family_properties_count; ++i) {
        if (queue_family_properties_array[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            res->queue_family_index = i;
            break;
        }
    }

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
    physical_device_features.samplerAnisotropy = supported_features.samplerAnisotropy;

    VkDeviceCreateInfo device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        NULL,
        0,
        1,
        &device_queue_create_info,
        0,
        NULL,
        device_extension_count,
        device_extension_array,
        &physical_device_features,
    };

    res->device = NULL;
    vkCreateDevice(res->physical_device, &device_create_info, NULL, &res->device);

    if (!res->device) {
        PyErr_Format(PyExc_RuntimeError, "cannot create device");
        return NULL;
    }

    vkGetDeviceQueue(res->device, res->queue_family_index, 0, &res->queue);

    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0};
    vkCreateFence(res->device, &fence_create_info, NULL, &res->fence);

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

    return res;
}

PyObject * Instance_meth_run(Instance * self) {
    VkCommandBuffer command_buffer = begin_commands(self);

    for (uint32_t i = 0; i < PyList_Size(self->task_list); ++i) {
        PyObject * task = PyList_GetItem(self->task_list, i);
        if (Py_TYPE(task) == self->state->Framebuffer_type) {
            Framebuffer * framebuffer = (Framebuffer *)PyList_GetItem(self->task_list, i);
            execute_framebuffer(command_buffer, framebuffer);
        }
    }

    end_commands(self);
    Py_RETURN_NONE;
}