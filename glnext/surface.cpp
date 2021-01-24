#include "glnext.hpp"

PyObject * Instance_meth_surface(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {
        "window",
        "image",
        "format",
        NULL
    };

    struct {
        PyObject * window;
        Image * image;
        PyObject * format = Py_None;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "OO!|O",
        keywords,
        &args.window,
        self->state->Image_type,
        &args.image,
        &args.format
    );

    if (!args_ok) {
        return NULL;
    }

    VkFormat image_format = args.image->format;
    if (args.format != Py_None) {
        image_format = get_format(args.format).format;
    }

    #ifdef BUILD_WINDOWS
    HWND hwnd = (HWND)PyLong_AsVoidPtr(args.window);

    VkWin32SurfaceCreateInfoKHR surface_create_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        NULL,
        0,
        NULL,
        hwnd,
    };

    VkSurfaceKHR surface = NULL;
    vkCreateWin32SurfaceKHR(self->instance, &surface_create_info, NULL, &surface);
    #endif

    #ifdef BUILD_LINUX
    Window wnd = (Window)PyLong_AsUnsignedLong(args.window);

    VkXlibSurfaceCreateInfoKHR surface_create_info = {
        VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        NULL,
        0,
        NULL,
        wnd,
    };

    VkSurfaceKHR surface = NULL;
    vkCreateXlibSurfaceKHR(self->instance, &surface_create_info, NULL, &surface);
    #endif

    #ifdef BUILD_DARWIN
    CAMetalLayer * layer = (CAMetalLayer *)PyLong_AsVoidPtr(args.window);

    VkMetalSurfaceCreateInfoEXT surface_create_info = {
        VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        NULL,
        0,
        layer,
    };

    VkSurfaceKHR surface = NULL;
    vkCreateMetalSurfaceEXT(self->instance, &surface_create_info, NULL, &surface);
    #endif

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(self->physical_device, self->queue_family_index, surface, &present_support);

    uint32_t num_formats = 1;
    VkSurfaceFormatKHR surface_format = {};
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device, surface, &num_formats, &surface_format);
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self->physical_device, surface, &surface_capabilities);

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        NULL,
        0,
        surface,
        surface_capabilities.minImageCount,
        image_format,
        surface_format.colorSpace,
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
    vkCreateSwapchainKHR(self->device, &swapchain_create_info, NULL, &swapchain);

    VkSemaphore semaphore = NULL;
    VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0};
    vkCreateSemaphore(self->device, &semaphore_create_info, NULL, &semaphore);

    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(self->device, swapchain, &swapchain_image_count, NULL);
    VkImage * swapchain_image_array = (VkImage *)PyMem_Malloc(sizeof(VkImage) * swapchain_image_count);
    vkGetSwapchainImagesKHR(self->device, swapchain, &swapchain_image_count, swapchain_image_array);

    uint32_t idx = self->presenter.surface_count++;

    presenter_resize(&self->presenter);

    self->presenter.surface_array[idx] = surface;
    self->presenter.swapchain_array[idx] = swapchain;
    self->presenter.semaphore_array[idx] = semaphore;
    self->presenter.image_source_array[idx] = args.image->image;
    self->presenter.image_count_array[idx] = swapchain_image_count;
    self->presenter.image_array[idx] = swapchain_image_array;
    self->presenter.wait_stage_array[idx] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    self->presenter.result_array[idx] = VK_SUCCESS;
    self->presenter.index_array[idx] = 0;

    self->presenter.image_copy_array[idx] = {
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        args.image->extent,
    };

    Py_RETURN_NONE;
}
