#include "glnext.hpp"

PyObject * glnext_meth_info(PyObject * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"backend", NULL};

    struct {
        const char * backend = NULL;
    } args;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwargs, "|$z", keywords, &args.backend)) {
        return NULL;
    }

    VkInstance instance = NULL;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = get_instance_proc_addr(args.backend);

    if (!vkGetInstanceProcAddr) {
        return NULL;
    }

    #define load(name) PFN_ ## name name = (PFN_ ## name)vkGetInstanceProcAddr(instance, #name)

    load(vkEnumerateInstanceVersion);
    load(vkEnumerateInstanceLayerProperties);
    load(vkEnumerateInstanceExtensionProperties);
    load(vkCreateInstance);

    VkInstanceCreateInfo instance_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    vkCreateInstance(&instance_create_info, NULL, &instance);

    load(vkDestroyInstance);
    load(vkEnumeratePhysicalDevices);
    load(vkEnumerateDeviceExtensionProperties);
    load(vkGetPhysicalDeviceProperties);

    #undef load

    uint32_t api_version = VK_API_VERSION_1_0;

    if (vkEnumerateInstanceVersion) {
        vkEnumerateInstanceVersion(&api_version);
    }

    PyObject * version = PyUnicode_FromFormat("%d.%d.%d", VK_VERSION_MAJOR(api_version), VK_VERSION_MINOR(api_version), VK_VERSION_PATCH(api_version));

    PyObject * layers_map = get_instance_layers(vkEnumerateInstanceLayerProperties);
    PyObject * layers = PySequence_List(layers_map);
    Py_DECREF(layers_map);

    PyObject * extensions_map = get_instance_extensions(vkEnumerateInstanceExtensionProperties);
    PyObject * extensions = PySequence_List(extensions_map);
    Py_DECREF(extensions_map);

    uint32_t physical_device_count = 0;
    VkPhysicalDevice physical_device_array[64];
    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_device_array);

    PyObject * phyiscal_devices = PyList_New(physical_device_count);

    for (uint32_t i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physical_device_array[i], &properties);

        PyObject * device_extensions_map = get_device_extensions(physical_device_array[i], vkEnumerateDeviceExtensionProperties);
        PyObject * device_extensions = PySequence_List(device_extensions_map);
        Py_DECREF(device_extensions_map);

        PyList_SetItem(phyiscal_devices, i, Py_BuildValue("{sssN}", "name", properties.deviceName, "extensions", device_extensions));
    }

    vkDestroyInstance(instance, NULL);

    return Py_BuildValue("{sNsNsNsN}", "version", version, "layers", layers, "extensions", extensions, "phyiscal_devices", phyiscal_devices);
}
