#include "glnext.hpp"

struct CompatibleFormat {
    VkFormat src;
    VkFormat dst;
};

const uint32_t compatible_format_count = 8;
const CompatibleFormat compatible_format_array[] = {
    {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM},
    {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB},
    {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_A2B10G10R10_UNORM_PACK32},
    {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_A2B10G10R10_UNORM_PACK32},
    {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_B8G8R8A8_SRGB},
    {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_B8G8R8A8_SRGB},
    {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_B8G8R8A8_UNORM},
    {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_B8G8R8A8_UNORM},
};

CompatibleFormat * get_compatible_format(VkFormat src, uint32_t surface_format_count, VkSurfaceFormatKHR * surface_format_array) {
    for (uint32_t i = 0; i < compatible_format_count; ++i) {
        for (uint32_t j = 0; j < surface_format_count; ++j) {
            if (surface_format_array[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                if (src == compatible_format_array[i].src && surface_format_array[j].format == compatible_format_array[i].dst) {
                    return (CompatibleFormat *)&compatible_format_array[i];
                }
            }
        }
    }
    return NULL;
}

PyObject * Instance_meth_surface(Instance * self, PyObject * vargs, PyObject * kwargs) {
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

    if (!self->presenter.supported) {
        PyErr_Format(PyExc_ValueError, "surface not enabled");
        return NULL;
    }

    VkSurfaceKHR surface = NULL;

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

        self->vkCreateWin32SurfaceKHR(self->instance, &surface_create_info, NULL, &surface);
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

        self->vkCreateXlibSurfaceKHR(self->instance, &surface_create_info, NULL, &surface);
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

        self->vkCreateMetalSurfaceEXT(self->instance, &surface_create_info, NULL, &surface);
    }
    #endif

    if (!surface) {
        PyErr_Format(PyExc_RuntimeError, "cannot create surface");
        return NULL;
    }

    VkBool32 present_support = false;
    self->vkGetPhysicalDeviceSurfaceSupportKHR(self->physical_device, self->queue_family_index, surface, &present_support);

    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    self->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self->physical_device, surface, &surface_capabilities);

    uint32_t surface_format_count = 0;
    self->vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, surface, &surface_format_count, NULL);
    VkSurfaceFormatKHR * surface_format_array = (VkSurfaceFormatKHR *)PyMem_Malloc(sizeof(VkSurfaceFormatKHR) * surface_format_count);
    self->vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, surface, &surface_format_count, surface_format_array);
    CompatibleFormat * format = get_compatible_format(args.image->format, surface_format_count, surface_format_array);
    PyMem_Free(surface_format_array);

    if (!format) {
        PyErr_Format(PyExc_RuntimeError, "incompatible image");
        return NULL;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        NULL,
        0,
        surface,
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

    VkSwapchainKHR swapchain = NULL;
    self->vkCreateSwapchainKHR(self->device, &swapchain_create_info, NULL, &swapchain);

    VkSemaphore semaphore = NULL;
    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0};
    self->vkCreateSemaphore(self->device, &semaphore_create_info, NULL, &semaphore);

    SwapChainImages images = {};
    self->vkGetSwapchainImagesKHR(self->device, swapchain, &images.image_count, NULL);
    self->vkGetSwapchainImagesKHR(self->device, swapchain, &images.image_count, images.image_array);

    uint32_t idx = self->presenter.surface_count++;

    self->presenter.surface_array[idx] = surface;
    self->presenter.swapchain_array[idx] = swapchain;
    self->presenter.semaphore_array[idx] = semaphore;
    self->presenter.image_source_array[idx] = args.image->image;
    self->presenter.image_array[idx] = images;
    self->presenter.wait_stage_array[idx] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    self->presenter.result_array[idx] = VK_SUCCESS;
    self->presenter.index_array[idx] = 0;

    self->presenter.copy_image_barrier_array[idx] = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        0,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        NULL,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };

    self->presenter.present_image_barrier_array[idx] = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        0,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        NULL,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };

    self->presenter.image_blit_array[idx] = {
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {{0, 0, 0}, {(int)args.image->extent.width, (int)args.image->extent.height, 1}},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {{0, 0, 0}, {(int)args.image->extent.width, (int)args.image->extent.height, 1}},
    };

    Py_RETURN_NONE;
}
