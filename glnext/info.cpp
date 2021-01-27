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

    load(vkEnumerateInstanceLayerProperties);
    load(vkEnumerateInstanceExtensionProperties);
    load(vkCreateInstance);

    VkInstanceCreateInfo instance_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    vkCreateInstance(&instance_create_info, NULL, &instance);

    load(vkDestroyInstance);
    load(vkEnumeratePhysicalDevices);
    load(vkGetPhysicalDeviceProperties);

    #undef load

    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    VkLayerProperties * layer_array = allocate<VkLayerProperties>(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layer_array);

    PyObject * layers = PyList_New(layer_count);
    for (uint32_t i = 0; i < layer_count; ++i) {
        PyList_SetItem(layers, i, PyUnicode_FromString(layer_array[i].layerName));
    }

    PyMem_Free(layer_array);

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    VkExtensionProperties * extension_array = allocate<VkExtensionProperties>(extension_count);
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extension_array);

    PyObject * extensions = PyList_New(extension_count);
    for (uint32_t i = 0; i < extension_count; ++i) {
        PyList_SetItem(extensions, i, PyUnicode_FromString(extension_array[i].extensionName));
    }

    PyMem_Free(extension_array);

    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);

    VkPhysicalDevice * physical_device_array = (VkPhysicalDevice *)PyMem_Malloc(sizeof(VkPhysicalDevice) * physical_device_count);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_device_array);

    PyObject * phyiscal_devices = PyList_New(physical_device_count);

    for (uint32_t i = 0; i < physical_device_count; ++i) {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physical_device_array[i], &properties);
        PyList_SetItem(phyiscal_devices, i, Py_BuildValue("{ss}", "name", properties.deviceName));
    }

    vkDestroyInstance(instance, NULL);
    return Py_BuildValue("{sNsNsN}", "layers", layers, "extensions", extensions, "phyiscal_devices", phyiscal_devices);
    return NULL;
}
