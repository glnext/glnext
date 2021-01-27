#include "glnext.hpp"

PFN_vkGetInstanceProcAddr get_instance_proc_addr(const char * backend) {
    #ifdef BUILD_WINDOWS
    HMODULE library = LoadLibrary(backend ? backend : "vulkan-1.dll");

    if (!library) {
        return NULL;
    }

    return (PFN_vkGetInstanceProcAddr)GetProcAddress(library, "vkGetInstanceProcAddr");
    #endif

    #ifdef BUILD_LINUX
    void * library = dlopen(backend ? backend : "libvulkan.so", RTLD_LAZY);

    if (!library) {
        return NULL;
    }

    return (PFN_vkGetInstanceProcAddr)dlsym(library, "vkGetInstanceProcAddr");
    #endif
}

void load_library_methods(Instance * self) {
    #define load(name) self->name = (PFN_ ## name)self->vkGetInstanceProcAddr(NULL, #name);

    load(vkEnumerateInstanceLayerProperties);
    load(vkCreateInstance);

    #undef load
}

void load_instance_methods(Instance * self) {
    #define load(name) self->name = (PFN_ ## name)self->vkGetInstanceProcAddr(self->instance, #name);

    load(vkDestroyInstance);
    load(vkEnumeratePhysicalDevices);
    load(vkGetPhysicalDeviceProperties);
    load(vkGetPhysicalDeviceMemoryProperties);
    load(vkGetPhysicalDeviceFeatures);
    load(vkGetPhysicalDeviceFormatProperties);
    load(vkGetPhysicalDeviceQueueFamilyProperties);
    load(vkGetDeviceProcAddr);
    load(vkCreateDevice);

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
    load(vkCmdCopyBufferToImage);
    load(vkCmdDrawIndirect);
    load(vkCreateImageView);
    load(vkCmdDispatch);
    load(vkCmdBindIndexBuffer);
    load(vkBindImageMemory);
    load(vkFreeMemory);
    load(vkCmdSetViewport);
    load(vkCmdBindDescriptorSets);
    load(vkCmdCopyBuffer);
    load(vkCmdDrawIndexedIndirect);
    load(vkCmdPushConstants);
    load(vkUnmapMemory);
    load(vkEndCommandBuffer);
    load(vkCreateBuffer);
    load(vkDestroyBuffer);
    load(vkUpdateDescriptorSets);
    load(vkCreateRenderPass);
    load(vkCmdDraw);
    load(vkCmdBindVertexBuffers);
    load(vkAllocateMemory);
    load(vkBindBufferMemory);
    load(vkCreatePipelineLayout);
    load(vkCmdSetScissor);
    load(vkCmdBindPipeline);
    load(vkCreateGraphicsPipelines);
    load(vkCreateDescriptorSetLayout);
    load(vkCmdDrawIndexed);
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

    #undef load
}
