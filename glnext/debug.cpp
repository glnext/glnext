#include "glnext.hpp"

typedef VkDebugUtilsMessageTypeFlagsEXT LogType;
typedef VkDebugUtilsMessengerCallbackDataEXT LogData;
typedef VkDebugUtilsMessageSeverityFlagBitsEXT LogSeverity;

VkBool32 debug_callback(LogSeverity severity, LogType types, const LogData * data, void * list) {
    PyObject * message = PyUnicode_FromString(data->pMessage);
    PyList_Append((PyObject *)list, message);
    Py_DECREF(message);
    return false;
}

void install_debug_messenger(Instance * instance) {
    VkFlags severity = 0;
    // severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    // severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    VkFlags message_type = 0;
    message_type |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    message_type |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    message_type |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT create_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        NULL,
        0,
        severity,
        message_type,
        debug_callback,
        instance->log_list,
    };

    PFN_vkVoidFunction proc = instance->vkGetInstanceProcAddr(instance->instance, "vkCreateDebugUtilsMessengerEXT");
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)proc;
    vkCreateDebugUtilsMessengerEXT(instance->instance, &create_info, NULL, &instance->debug_messenger);
}
