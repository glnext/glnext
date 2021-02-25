#include "glnext.hpp"

PFN_vkGetInstanceProcAddr get_instance_proc_addr(const char * backend) {
    if (!backend) {
        backend = DEFAULT_BACKEND;
    }

    #ifdef BUILD_WINDOWS
    HMODULE library = LoadLibrary(backend);
    #endif

    #ifdef BUILD_LINUX
    void * library = dlopen(backend, RTLD_LAZY);
    #endif

    if (!library) {
        PyErr_Format(PyExc_RuntimeError, "cannot load %s", backend);
        return NULL;
    }

    #ifdef BUILD_WINDOWS
    return (PFN_vkGetInstanceProcAddr)GetProcAddress(library, "vkGetInstanceProcAddr");
    #endif

    #ifdef BUILD_LINUX
    return (PFN_vkGetInstanceProcAddr)dlsym(library, "vkGetInstanceProcAddr");
    #endif
}

void load_library_methods(Instance * self) {
    #define load(name) self->name = (PFN_ ## name)self->vkGetInstanceProcAddr(NULL, #name);

    load(vkEnumerateInstanceVersion);
    load(vkEnumerateInstanceLayerProperties);
    load(vkEnumerateInstanceExtensionProperties);
    load(vkCreateInstance);

    #undef load
}

void load_instance_methods(Instance * self) {
    #define load(name) self->name = (PFN_ ## name)self->vkGetInstanceProcAddr(self->instance, #name);

    load(vkDestroyInstance);
    load(vkEnumeratePhysicalDevices);
    load(vkEnumerateDeviceExtensionProperties);
    load(vkGetPhysicalDeviceProperties);
    load(vkGetPhysicalDeviceFeatures2);
    load(vkGetPhysicalDeviceMemoryProperties);
    load(vkGetPhysicalDeviceFeatures);
    load(vkGetPhysicalDeviceFormatProperties);
    load(vkGetPhysicalDeviceQueueFamilyProperties);
    load(vkGetDeviceProcAddr);
    load(vkCreateDevice);

    load(vkGetPhysicalDeviceSurfaceSupportKHR);
    load(vkGetPhysicalDeviceSurfaceFormatsKHR);
    load(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    load(vkCreateWin32SurfaceKHR);
    load(vkCreateXlibSurfaceKHR);
    load(vkCreateXcbSurfaceKHR);
    load(vkCreateWaylandSurfaceKHR);
    load(vkCreateMetalSurfaceEXT);
    load(vkDestroySurfaceKHR);

    #undef load
}

void load_device_methods(Instance * self) {
    #define load(name) self->name = (PFN_ ## name)self->vkGetDeviceProcAddr(self->device, #name);

    load(vkGetDeviceQueue);
    load(vkCreateFence);
    load(vkCreateCommandPool);
    load(vkAllocateCommandBuffers);
    load(vkGetImageMemoryRequirements);
    load(vkCmdCopyImageToBuffer);
    load(vkCreateShaderModule);
    load(vkDestroyShaderModule);
    load(vkCmdCopyBufferToImage);
    load(vkCreateImageView);
    load(vkCmdDispatch);
    load(vkCmdBindIndexBuffer);
    load(vkBindImageMemory);
    load(vkFreeMemory);
    load(vkCmdSetViewport);
    load(vkCmdBindDescriptorSets);
    load(vkCmdCopyBuffer);
    load(vkCmdPushConstants);
    load(vkUnmapMemory);
    load(vkEndCommandBuffer);
    load(vkCreateBuffer);
    load(vkDestroyBuffer);
    load(vkUpdateDescriptorSets);
    load(vkCreateRenderPass);
    load(vkCmdBindVertexBuffers);
    load(vkAllocateMemory);
    load(vkBindBufferMemory);
    load(vkCreatePipelineLayout);
    load(vkCreatePipelineCache);
    load(vkGetPipelineCacheData);
    load(vkCmdSetScissor);
    load(vkCmdBindPipeline);
    load(vkCreateGraphicsPipelines);
    load(vkCreateDescriptorSetLayout);
    load(vkCmdEndRenderPass);
    load(vkCmdPipelineBarrier);
    load(vkCreateDescriptorPool);
    load(vkCreateImage);
    load(vkCmdBeginRenderPass);
    load(vkCreateComputePipelines);
    load(vkBeginCommandBuffer);
    load(vkCreateFramebuffer);
    load(vkMapMemory);
    load(vkQueueSubmit);
    load(vkAllocateDescriptorSets);
    load(vkCreateSampler);
    load(vkGetBufferMemoryRequirements);
    load(vkWaitForFences);
    load(vkResetFences);
    load(vkCreateSemaphore);
    load(vkDestroySemaphore);
    load(vkCmdCopyImage);
    load(vkCmdBlitImage);

    load(vkCmdDraw);
    load(vkCmdDrawIndexed);
    load(vkCmdDrawIndirect);
    load(vkCmdDrawIndexedIndirect);
    load(vkCmdDrawIndirectCount);
    load(vkCmdDrawIndexedIndirectCount);
    load(vkCmdDrawMeshTasksNV);
    load(vkCmdDrawMeshTasksIndirectNV);
    load(vkCmdDrawMeshTasksIndirectCountNV);

    load(vkAcquireNextImageKHR);
    load(vkQueuePresentKHR);
    load(vkCreateSwapchainKHR);
    load(vkGetSwapchainImagesKHR);
    load(vkDestroySwapchainKHR);

    #undef load
}
