#include "glnext.hpp"

Instance * glnext_meth_instance(PyObject * self, PyObject * vargs, PyObject * kwargs) {
    ModuleState * state = (ModuleState *)PyModule_GetState(self);

    static char * keywords[] = {
        "physical_device",
        "application_name",
        "application_version",
        "engine_name",
        "engine_version",
        "backend",
        "surface",
        "layers",
        "cache",
        "debug",
        NULL,
    };

    struct {
        uint32_t physical_device = 0;
        const char * application_name = NULL;
        uint32_t application_version = 0;
        const char * engine_name = NULL;
        uint32_t engine_version = 0;
        const char * backend = NULL;
        PyObject * surface = Py_False;
        PyObject * layers = Py_None;
        PyObject * cache = Py_None;
        VkBool32 debug = false;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "|$IzIzIzOOOp",
        keywords,
        &args.physical_device,
        &args.application_name,
        &args.application_version,
        &args.engine_name,
        &args.engine_version,
        &args.backend,
        &args.surface,
        &args.layers,
        &args.cache,
        &args.debug
    );

    if (!args_ok) {
        return NULL;
    }

    if (!PyBool_Check(args.surface) && !PyUnicode_CheckExact(args.surface)) {
        PyErr_Format(PyExc_ValueError, "surface");
        return NULL;
    }

    if (args.layers == Py_None) {
        args.layers = state->empty_list;
    }

    const char * surface = NULL;

    if (args.surface == Py_True) {
        surface = getenv("GLNEXT_SURFACE");
        surface = surface ? surface : DEFAULT_SURFACE;
    }

    if (PyUnicode_CheckExact(args.surface)) {
        surface = PyUnicode_AsUTF8(args.surface);
    }

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = get_instance_proc_addr(args.backend);

    if (!vkGetInstanceProcAddr) {
        PyErr_Format(PyExc_RuntimeError, "cannot load backend");
        return NULL;
    }

    Instance * res = PyObject_New(Instance, state->Instance_type);

    res->state = state;
    res->instance = NULL;
    res->physical_device = NULL;
    res->device = NULL;
    res->queue = NULL;
    res->fence = NULL;
    res->command_pool = NULL;
    res->command_buffer = NULL;
    res->pipeline_cache = NULL;
    res->debug_messenger = NULL;

    res->extension = {};
    res->group = NULL;

    res->surface_list = PyList_New(0);
    res->task_list = PyList_New(0);
    res->memory_list = PyList_New(0);
    res->buffer_list = PyList_New(0);
    res->image_list = PyList_New(0);
    res->log_list = PyList_New(0);

    res->vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    load_library_methods(res);

    res->debug = args.debug;
    res->api_version = VK_API_VERSION_1_0;

    if (res->vkEnumerateInstanceVersion) {
        res->vkEnumerateInstanceVersion(&res->api_version);
    }

    const char * layer_array[64];
    uint32_t layer_count = load_instance_layers(res, layer_array, args.layers);

    const char * extension_array[64];
    uint32_t extension_count = load_instance_extensions(res, extension_array, surface);

    VkApplicationInfo application_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        NULL,
        args.application_name,
        args.application_version,
        args.engine_name,
        args.engine_version,
        res->api_version,
    };

    VkInstanceCreateInfo instance_create_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        NULL,
        0,
        &application_info,
        layer_count,
        layer_array,
        extension_count,
        extension_array,
    };

    res->vkCreateInstance(&instance_create_info, NULL, &res->instance);

    if (!res->instance) {
        PyErr_Format(PyExc_RuntimeError, "cannot create instance");
        return NULL;
    }

    load_instance_methods(res);

    if (args.debug) {
        install_debug_messenger(res);
    }

    uint32_t physical_device_count = 0;
    VkPhysicalDevice physical_device_array[64] = {};
    res->vkEnumeratePhysicalDevices(res->instance, &physical_device_count, NULL);
    res->vkEnumeratePhysicalDevices(res->instance, &physical_device_count, physical_device_array);
    res->physical_device = physical_device_array[args.physical_device];

    if (!res->physical_device) {
        PyErr_Format(PyExc_RuntimeError, "physical device not found");
        return NULL;
    }

    VkPhysicalDeviceMemoryProperties device_memory_properties = {};
    res->vkGetPhysicalDeviceMemoryProperties(res->physical_device, &device_memory_properties);

    VkPhysicalDeviceFeatures supported_features = {};
    res->vkGetPhysicalDeviceFeatures(res->physical_device, &supported_features);

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
        res->vkGetPhysicalDeviceFormatProperties(res->physical_device, depth_formats[i], &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            res->depth_format = depth_formats[i];
        }
    }

    uint32_t queue_family_properties_count = 0;
    VkQueueFamilyProperties queue_family_properties_array[64];
    res->vkGetPhysicalDeviceQueueFamilyProperties(res->physical_device, &queue_family_properties_count, NULL);
    res->vkGetPhysicalDeviceQueueFamilyProperties(res->physical_device, &queue_family_properties_count, queue_family_properties_array);

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

    const char * device_extension_array[64];
    uint32_t device_extension_count = load_device_extensions(res, device_extension_array, surface);

    VkPhysicalDeviceMeshShaderFeaturesNV mesh_shader_features = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV,
        NULL,
    };

    if (res->extension.mesh_shader) {
        VkPhysicalDeviceFeatures2 physical_device_features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            &mesh_shader_features,
        };
        res->vkGetPhysicalDeviceFeatures2(res->physical_device, &physical_device_features);
    }

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

    if (res->extension.mesh_shader) {
        device_create_info.pNext = &mesh_shader_features;
    }

    res->vkCreateDevice(res->physical_device, &device_create_info, NULL, &res->device);

    if (!res->device) {
        PyErr_Format(PyExc_RuntimeError, "cannot create device");
        return NULL;
    }

    load_device_methods(res);

    res->vkGetDeviceQueue(res->device, res->queue_family_index, 0, &res->queue);

    VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, 0};
    res->vkCreateFence(res->device, &fence_create_info, NULL, &res->fence);

    VkCommandPoolCreateInfo command_pool_create_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        res->queue_family_index,
    };

    res->vkCreateCommandPool(res->device, &command_pool_create_info, NULL, &res->command_pool);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        res->command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1,
    };

    res->vkAllocateCommandBuffers(res->device, &command_buffer_allocate_info, &res->command_buffer);

    if (PyBytes_CheckExact(args.cache)) {
        VkPipelineCacheCreateInfo pipeline_cache_create_info = {
            VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            NULL,
            0,
            (size_t)PyBytes_Size(args.cache),
            PyBytes_AsString(args.cache),
        };
        res->vkCreatePipelineCache(res->device, &pipeline_cache_create_info, NULL, &res->pipeline_cache);
    }

    return res;
}

PyObject * Instance_meth_cache(Instance * self) {
    if (!self->pipeline_cache) {
        PyErr_Format(PyExc_ValueError, "cache not enabled");
        return NULL;
    }
    size_t size = 0;
    self->vkGetPipelineCacheData(self->device, self->pipeline_cache, &size, NULL);
    PyObject * res = PyBytes_FromStringAndSize(NULL, size);
    self->vkGetPipelineCacheData(self->device, self->pipeline_cache, &size, PyBytes_AsString(res));
    return res;
}

PyObject * Instance_meth_present(Instance * self) {
    uint32_t surface_count = (uint32_t)PyList_Size(self->surface_list);

    if (!surface_count) {
        Py_RETURN_NONE;
    }

    VkSwapchainKHR swapchain_array[64];
    VkPipelineStageFlags wait_stage_array[64];
    VkImageMemoryBarrier image_barrier_array[64];
    VkSemaphore semaphore_array[64];
    VkResult result_array[64];
    uint32_t index_array[64];

    begin_commands(self);

    for (uint32_t i = 0; i < surface_count; ++i) {
        Surface * surface = (Surface *)PyList_GET_ITEM(self->surface_list, i);

        swapchain_array[i] = surface->swapchain;
        semaphore_array[i] = surface->semaphore;
        wait_stage_array[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        self->vkAcquireNextImageKHR(
            self->device,
            surface->swapchain,
            UINT64_MAX,
            surface->semaphore,
            NULL,
            &index_array[i]
        );
    }

    for (uint32_t i = 0; i < surface_count; ++i) {
        Surface * surface = (Surface *)PyList_GET_ITEM(self->surface_list, i);

        image_barrier_array[i] = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            NULL,
            0,
            0,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            surface->images.image_array[index_array[i]],
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
    }

    self->vkCmdPipelineBarrier(
        self->command_buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        surface_count,
        image_barrier_array
    );

    for (uint32_t i = 0; i < surface_count; ++i) {
        Surface * surface = (Surface *)PyList_GET_ITEM(self->surface_list, i);

        VkImageBlit image_blit = {
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            {{0, 0, 0}, {(int)surface->image->extent.width, (int)surface->image->extent.height, 1}},
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
            {{0, 0, 0}, {(int)surface->image->extent.width, (int)surface->image->extent.height, 1}},
        };

        self->vkCmdBlitImage(
            self->command_buffer,
            surface->image->image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            surface->images.image_array[index_array[i]],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &image_blit,
            VK_FILTER_NEAREST
        );
    }

    for (uint32_t i = 0; i < surface_count; ++i) {
        Surface * surface = (Surface *)PyList_GET_ITEM(self->surface_list, i);

        image_barrier_array[i] = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            NULL,
            0,
            0,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            surface->images.image_array[index_array[i]],
            {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
    }

    self->vkCmdPipelineBarrier(
        self->command_buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        surface_count,
        image_barrier_array
    );

    self->vkEndCommandBuffer(self->command_buffer);

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        surface_count,
        semaphore_array,
        wait_stage_array,
        1,
        &self->command_buffer,
        0,
        NULL,
    };

    self->vkQueueSubmit(self->queue, 1, &submit_info, self->fence);
    self->vkWaitForFences(self->device, 1, &self->fence, true, UINT64_MAX);
    self->vkResetFences(self->device, 1, &self->fence);

    VkPresentInfoKHR present_info = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        NULL,
        0,
        NULL,
        surface_count,
        swapchain_array,
        index_array,
        result_array,
    };

    self->vkQueuePresentKHR(self->queue, &present_info);

    uint32_t idx = 0;
    while (idx < surface_count) {
        if (result_array[idx] == VK_ERROR_OUT_OF_DATE_KHR) {
            Surface * surface = (Surface *)PyList_GET_ITEM(self->surface_list, idx);
            self->vkDestroySemaphore(self->device, surface->semaphore, NULL);
            self->vkDestroySwapchainKHR(self->device, surface->swapchain, NULL);
            self->vkDestroySurfaceKHR(self->instance, surface->surface, NULL);
            continue;
        }
        idx += 1;
    }

    Py_RETURN_NONE;
}
