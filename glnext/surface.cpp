#include "glnext.hpp"

const uint32_t compatible_format_count = 8;
const SurfaceCompatibleFormat compatible_format_array[] = {
    {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM},
    {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB},
    {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_A2B10G10R10_UNORM_PACK32},
    {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_A2B10G10R10_UNORM_PACK32},
    {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_B8G8R8A8_SRGB},
    {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_B8G8R8A8_SRGB},
    {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_B8G8R8A8_UNORM},
    {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_B8G8R8A8_UNORM},
};

SurfaceCompatibleFormat * get_compatible_format(VkFormat src, uint32_t surface_format_count, VkSurfaceFormatKHR * surface_format_array) {
    for (uint32_t i = 0; i < compatible_format_count; ++i) {
        for (uint32_t j = 0; j < surface_format_count; ++j) {
            if (surface_format_array[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                if (src == compatible_format_array[i].src && surface_format_array[j].format == compatible_format_array[i].dst) {
                    return (SurfaceCompatibleFormat *)&compatible_format_array[i];
                }
            }
        }
    }
    return NULL;
}

Surface * Instance_meth_surface(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "window",
        "image",
        NULL
    };

    struct {
        PyObject * window;
        Image * image;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "OO!",
        keywords,
        &args.window,
        self->state->Image_type,
        &args.image
    );

    if (!args_ok) {
        return NULL;
    }

    // if (!self->presenter.supported) {
    //     PyErr_Format(PyExc_ValueError, "surface not enabled");
    //     return NULL;
    // }

    // for (uint32_t i = 0; i < PyList_Size(self->surface_list); ++i) {
    //     Surface * surface = PyList_GET_ITEM(self->surface_list, i);
    //     if (PyObject_RichCompare(surface->window, args.window, Py_EQ)) {
    //         surface->image = args.image;
    //         Py_INCREF(surface);
    //         return surface;
    //     }
    // }

    Surface * res = PyObject_New(Surface, self->state->Surface_type);
    res->instance = self;
    res->image = args.image;

    Py_INCREF(args.window);
    res->window = args.window;

    res->surface = NULL;
    res->swapchain = NULL;
    res->semaphore = NULL;
    res->images = {};

    #ifdef BUILD_WINDOWS
    if (self->vkCreateWin32SurfaceKHR) {
        HINSTANCE hinstance = (HINSTANCE)PyLong_AsVoidPtr(PyTuple_GetItem(args.window, 0));
        HWND hwnd = (HWND)PyLong_AsVoidPtr(PyTuple_GetItem(args.window, 1));

        VkWin32SurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            NULL,
            0,
            hinstance,
            hwnd,
        };

        self->vkCreateWin32SurfaceKHR(self->instance, &surface_create_info, NULL, &res->surface);
    }
    #endif

    #ifdef BUILD_LINUX
    if (self->vkCreateXlibSurfaceKHR) {
        Display * display = (Display *)PyLong_AsVoidPtr(PyTuple_GetItem(args.window, 0));
        Window window = (Window)PyLong_AsUnsignedLong(PyTuple_GetItem(args.window, 1));

        VkXlibSurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            NULL,
            0,
            display,
            window,
        };

        self->vkCreateXlibSurfaceKHR(self->instance, &surface_create_info, NULL, &res->surface);
    }
    #endif

    #ifdef BUILD_DARWIN
    if (self->vkCreateMetalSurfaceEXT) {
        CAMetalLayer * layer = (CAMetalLayer *)PyLong_AsVoidPtr(args.window);

        VkMetalSurfaceCreateInfoEXT surface_create_info = {
            VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            NULL,
            0,
            layer,
        };

        self->vkCreateMetalSurfaceEXT(self->instance, &surface_create_info, NULL, &res->surface);
    }
    #endif

    if (!res->surface) {
        PyErr_Format(PyExc_RuntimeError, "cannot create surface");
        return NULL;
    }

    VkBool32 present_support = false;
    self->vkGetPhysicalDeviceSurfaceSupportKHR(self->physical_device, self->queue_family_index, res->surface, &present_support);

    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    self->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self->physical_device, res->surface, &surface_capabilities);

    uint32_t surface_format_count = 0;
    self->vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, res->surface, &surface_format_count, NULL);
    VkSurfaceFormatKHR * surface_format_array = (VkSurfaceFormatKHR *)PyMem_Malloc(sizeof(VkSurfaceFormatKHR) * surface_format_count);
    self->vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, res->surface, &surface_format_count, surface_format_array);
    SurfaceCompatibleFormat * format = get_compatible_format(args.image->format, surface_format_count, surface_format_array);
    PyMem_Free(surface_format_array);

    if (!format) {
        PyErr_Format(PyExc_RuntimeError, "incompatible image");
        return NULL;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        NULL,
        0,
        res->surface,
        surface_capabilities.minImageCount,
        format->dst,
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        surface_capabilities.currentExtent,
        1,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        NULL,
        surface_capabilities.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_PRESENT_MODE_FIFO_KHR,
        true,
        NULL,
    };

    self->vkCreateSwapchainKHR(self->device, &swapchain_create_info, NULL, &res->swapchain);

    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0};
    self->vkCreateSemaphore(self->device, &semaphore_create_info, NULL, &res->semaphore);

    self->vkGetSwapchainImagesKHR(self->device, res->swapchain, &res->images.image_count, NULL);
    self->vkGetSwapchainImagesKHR(self->device, res->swapchain, &res->images.image_count, res->images.image_array);

    PyList_Append(self->surface_list, (PyObject *)res);
    return res;
}

Image * Surface_get_image(Surface * self) {
    Py_INCREF(self->image);
    return self->image;
}

int Surface_set_image(Surface * self, Image * value) {
    if (Py_TYPE(value) != self->instance->state->Image_type) {
        PyErr_Format(PyExc_TypeError, "image");
        return -1;
    }

    if (self->image->format != value->format) {
        PyErr_Format(PyExc_ValueError, "image format");
        return -1;
    }

    if (self->image->extent.width != value->extent.width || self->image->extent.height != value->extent.height) {
        PyErr_Format(PyExc_ValueError, "image size");
        return -1;
    }

    self->image = value;
    return 0;
}
