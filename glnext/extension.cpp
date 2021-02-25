#include "glnext.hpp"

PyObject * get_instance_layers(PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties) {
    PyObject * res = PyDict_New();
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    VkLayerProperties * layer_array = allocate<VkLayerProperties>(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layer_array);
    for (uint32_t i = 0; i < layer_count; ++i) {
        PyDict_SetItemString(res, layer_array[i].layerName, Py_True);
    }
    free(layer_array);
    return res;
}

PyObject * get_instance_extensions(PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties) {
    PyObject * res = PyDict_New();
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    VkExtensionProperties * extension_array = allocate<VkExtensionProperties>(extension_count);
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_array);
    for (uint32_t i = 0; i < extension_count; ++i) {
        PyDict_SetItemString(res, extension_array[i].extensionName, Py_True);
    }
    free(extension_array);
    return res;
}

PyObject * get_device_extensions(VkPhysicalDevice physical_device, PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties) {
    PyObject * res = PyDict_New();
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL);
    VkExtensionProperties * extension_array = allocate<VkExtensionProperties>(extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, extension_array);
    for (uint32_t i = 0; i < extension_count; ++i) {
        PyDict_SetItemString(res, extension_array[i].extensionName, Py_True);
    }
    free(extension_array);
    return res;
}

uint32_t load_device_extensions(Instance * instance, const char ** array) {
    uint32_t count = 0;

    if (instance->presenter.supported) {
        array[count++] = "VK_KHR_swapchain";
    }

    PyObject * device_extensions = get_device_extensions(instance->physical_device, instance->vkEnumerateDeviceExtensionProperties);

    if (instance->api_version < VK_API_VERSION_1_1 && has_key(device_extensions, "VK_KHR_get_memory_requirements2")) {
        array[count++] = "VK_KHR_get_memory_requirements2";
    }

    if (instance->api_version < VK_API_VERSION_1_1 && has_key(device_extensions, "VK_KHR_dedicated_allocation")) {
        array[count++] = "VK_KHR_dedicated_allocation";
        instance->extension.dedicated_allocation = true;
    }

    if (instance->api_version < VK_API_VERSION_1_2 && has_key(device_extensions, "VK_KHR_spirv_1_4")) {
        array[count++] = "VK_KHR_spirv_1_4";
    }

    if (instance->api_version < VK_API_VERSION_1_2 && has_key(device_extensions, "VK_KHR_draw_indirect_count")) {
        array[count++] = "VK_KHR_draw_indirect_count";
        instance->extension.draw_indirect_count = true;
    }

    if (has_key(device_extensions, "VK_KHR_deferred_host_operations")) {
        array[count++] = "VK_KHR_deferred_host_operations";
        instance->extension.deferred_host_operations = true;
    }

    if (has_key(device_extensions, "VK_KHR_acceleration_structure")) {
        array[count++] = "VK_KHR_acceleration_structure";
        instance->extension.acceleration_structure = true;
    }

    if (has_key(device_extensions, "VK_KHR_ray_query")) {
        array[count++] = "VK_KHR_ray_query";
        instance->extension.ray_query = true;
    }

    if (has_key(device_extensions, "VK_KHR_pipeline_library")) {
        array[count++] = "VK_KHR_pipeline_library";
        instance->extension.pipeline_library = true;
    }

    if (has_key(device_extensions, "VK_KHR_ray_tracing_pipeline")) {
        array[count++] = "VK_KHR_ray_tracing_pipeline";
        instance->extension.ray_tracing_pipeline = true;
    }

    if (has_key(device_extensions, "VK_NV_mesh_shader")) {
        array[count++] = "VK_NV_mesh_shader";
        instance->extension.mesh_shader = true;
    }

    return count;
}
