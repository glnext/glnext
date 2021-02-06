#include "glnext.hpp"

VkComponentSwizzle get_swizzle(char c) {
    switch (c) {
        case '0': return VK_COMPONENT_SWIZZLE_ZERO;
        case '1': return VK_COMPONENT_SWIZZLE_ONE;
        case 'r': return VK_COMPONENT_SWIZZLE_R;
        case 'g': return VK_COMPONENT_SWIZZLE_G;
        case 'b': return VK_COMPONENT_SWIZZLE_B;
        case 'a': return VK_COMPONENT_SWIZZLE_A;
    }
    return VK_COMPONENT_SWIZZLE_IDENTITY;
}

int parse_buffer_binding(Instance * instance, BufferBinding * binding, PyObject * obj) {
    memset(binding, 0, sizeof(BufferBinding));

    binding->name = PyDict_GetItemString(obj, "name");
    Py_XINCREF(binding->name);

    if (binding->name && !PyUnicode_CheckExact(binding->name)) {
        PyErr_Format(PyExc_ValueError, "name");
        return -1;
    }

    PyObject * binding_int = PyDict_GetItemString(obj, "binding");

    if (!binding_int) {
        PyErr_Format(PyExc_ValueError, "binding");
        return -1;
    }

    binding->binding = PyLong_AsUnsignedLong(binding_int);

    if (PyErr_Occurred()) {
        PyErr_Format(PyExc_ValueError, "binding");
        return -1;
    }

    PyObject * type = PyDict_GetItemString(obj, "type");

    if (!type || !PyUnicode_CheckExact(type)) {
        PyErr_Format(PyExc_ValueError, "type");
        return -1;
    }

    bool valid_type = false;

    if (!PyUnicode_CompareWithASCIIString(type, "uniform_buffer")) {
        binding->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        binding->descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding->mode = BUF_UNIFORM;
        valid_type = true;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "storage_buffer")) {
        binding->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        binding->descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding->mode = BUF_STORAGE;
        valid_type = true;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "input_buffer")) {
        binding->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        binding->descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding->mode = BUF_INPUT;
        valid_type = true;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "output_buffer")) {
        binding->usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        binding->descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding->mode = BUF_OUTPUT;
        valid_type = true;
    }

    if (!valid_type) {
        PyErr_Format(PyExc_ValueError, "type");
        return -1;
    }

    if (Buffer * buffer = (Buffer *)PyDict_GetItemString(obj, "buffer")) {
        if (Py_TYPE(buffer) != instance->state->Buffer_type) {
            PyErr_Format(PyExc_ValueError, "buffer");
            return -1;
        }

        binding->buffer = buffer;
        binding->size = buffer->size;
    }

    if (PyObject * size = PyDict_GetItemString(obj, "size")) {
        if (!PyLong_CheckExact(size)) {
            PyErr_Format(PyExc_ValueError, "size");
            return -1;
        }

        binding->size = PyLong_AsUnsignedLongLong(size);

        if (PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError, "size");
            return -1;
        }
    }

    if (!binding->buffer && !binding->size) {
        PyErr_Format(PyExc_ValueError, "buffer");
        return -1;
    }

    binding->descriptor_buffer_info = {
        NULL,
        0,
        VK_WHOLE_SIZE,
    };

    return 0;
}

int parse_image_binding(Instance * instance, ImageBinding * binding, PyObject * obj) {
    memset(binding, 0, sizeof(ImageBinding));

    binding->name = PyDict_GetItemString(obj, "name");
    Py_XINCREF(binding->name);

    if (binding->name && !PyUnicode_CheckExact(binding->name)) {
        PyErr_Format(PyExc_ValueError, "name");
        return -1;
    }

    PyObject * binding_int = PyDict_GetItemString(obj, "binding");

    if (!binding_int) {
        PyErr_Format(PyExc_ValueError, "binding");
        return -1;
    }

    binding->binding = PyLong_AsUnsignedLong(binding_int);

    if (PyErr_Occurred()) {
        PyErr_Format(PyExc_ValueError, "binding");
        return -1;
    }

    PyObject * type = PyDict_GetItemString(obj, "type");

    if (!type || !PyUnicode_CheckExact(type)) {
        PyErr_Format(PyExc_ValueError, "type");
        return -1;
    }

    bool valid_type = false;

    if (!PyUnicode_CompareWithASCIIString(type, "storage_image")) {
        binding->descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        binding->layout = VK_IMAGE_LAYOUT_GENERAL;
        binding->sampled = false;
        valid_type = true;
    }

    if (!PyUnicode_CompareWithASCIIString(type, "sampled_image")) {
        binding->descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        binding->sampled = true;
        valid_type = true;
    }

    if (!valid_type) {
        PyErr_Format(PyExc_ValueError, "type");
        return -1;
    }

    PyObject * images = PyDict_GetItemString(obj, "images");

    if (!images || !PyList_CheckExact(images)) {
        PyErr_Format(PyExc_ValueError, "images");
        return -1;
    }

    binding->image_count = (uint32_t)PyList_Size(images);
    binding->image_array = allocate<Image *>(binding->image_count);
    binding->sampler_array = allocate<VkSampler>(binding->image_count);
    binding->image_view_array = allocate<VkImageView>(binding->image_count);
    binding->sampler_create_info_array = allocate<VkSamplerCreateInfo>(binding->image_count);
    binding->descriptor_image_info_array = allocate<VkDescriptorImageInfo>(binding->image_count);
    binding->image_view_create_info_array = allocate<VkImageViewCreateInfo>(binding->image_count);

    for (uint32_t i = 0; i < binding->image_count; ++i) {
        PyObject * item = PyList_GetItem(images, i);

        if (!PyDict_CheckExact(item)) {
            PyErr_Format(PyExc_ValueError, "images");
            return -1;
        }

        Image * image = (Image *)PyDict_GetItemString(item, "image");

        if (!image || Py_TYPE(image) != instance->state->Image_type) {
            PyErr_Format(PyExc_ValueError, "image");
            return -1;
        }

        if (binding->sampled) {
            PyObject * sampler = PyDict_GetItemString(item, "sampler");

            if (!sampler || !PyDict_CheckExact(sampler)) {
                PyErr_Format(PyExc_ValueError, "sampler");
                return -1;
            }

            VkFilter min_filter = VK_FILTER_NEAREST;

            if (PyObject * temp = PyDict_GetItemString(sampler, "min_filter")) {
                if (!temp || !PyUnicode_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "min_filter");
                    return -1;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "nearest")) {
                    min_filter = VK_FILTER_NEAREST;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "linear")) {
                    min_filter = VK_FILTER_LINEAR;
                }
            }

            VkFilter mag_filter = VK_FILTER_NEAREST;

            if (PyObject * temp = PyDict_GetItemString(sampler, "mag_filter")) {
                if (!temp || !PyUnicode_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "mag_filter");
                    return -1;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "nearest")) {
                    mag_filter = VK_FILTER_NEAREST;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "linear")) {
                    mag_filter = VK_FILTER_LINEAR;
                }
            }

            VkSamplerMipmapMode mipmap_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;

            if (PyObject * temp = PyDict_GetItemString(sampler, "mipmap_filter")) {
                if (!temp || !PyUnicode_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "mipmap_filter");
                    return -1;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "nearest")) {
                    mipmap_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "linear")) {
                    mipmap_filter = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                }
            }

            VkSamplerAddressMode address_mode_x = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            if (PyObject * temp = PyDict_GetItemString(sampler, "address_mode_x")) {
                if (!temp || !PyUnicode_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "address_mode_x");
                    return -1;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "repeat")) {
                    address_mode_x = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "mirrored_repeat")) {
                    address_mode_x = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "clamp_to_edge")) {
                    address_mode_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "clamp_to_border")) {
                    address_mode_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "mirror_clamp_to_edge")) {
                    address_mode_x = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
                }
            }

            VkSamplerAddressMode address_mode_y = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            if (PyObject * temp = PyDict_GetItemString(sampler, "address_mode_y")) {
                if (!temp || !PyUnicode_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "address_mode_y");
                    return -1;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "repeat")) {
                    address_mode_y = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "mirrored_repeat")) {
                    address_mode_y = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "clamp_to_edge")) {
                    address_mode_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "clamp_to_border")) {
                    address_mode_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "mirror_clamp_to_edge")) {
                    address_mode_y = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
                }
            }

            VkSamplerAddressMode address_mode_z = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            if (PyObject * temp = PyDict_GetItemString(sampler, "address_mode_z")) {
                if (!temp || !PyUnicode_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "address_mode_z");
                    return -1;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "repeat")) {
                    address_mode_z = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "mirrored_repeat")) {
                    address_mode_z = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "clamp_to_edge")) {
                    address_mode_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "clamp_to_border")) {
                    address_mode_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "mirror_clamp_to_edge")) {
                    address_mode_z = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
                }
            }

            float lod_bias = 0.0f;

            if (PyObject * temp = PyDict_GetItemString(sampler, "lod_bias")) {
                if (!temp || !PyFloat_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "lod_bias");
                    return -1;
                }

                lod_bias = (float)PyFloat_AsDouble(temp);
            }

            float max_anisotropy = 0.0f;

            if (PyObject * temp = PyDict_GetItemString(sampler, "max_anisotropy")) {
                if (!temp || !PyFloat_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "max_anisotropy");
                    return -1;
                }

                max_anisotropy = (float)PyFloat_AsDouble(temp);
            }

            float min_lod = 0.0f;

            if (PyObject * temp = PyDict_GetItemString(sampler, "min_lod")) {
                if (!temp || !PyFloat_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "min_lod");
                    return -1;
                }

                min_lod = (float)PyFloat_AsDouble(temp);
            }

            float max_lod = 1000.0f;

            if (PyObject * temp = PyDict_GetItemString(sampler, "max_lod")) {
                if (!temp || !PyFloat_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "max_lod");
                    return -1;
                }

                max_lod = (float)PyFloat_AsDouble(temp);
            }

            bool is_float = true;

            VkBorderColor border_color = is_float ? VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK : VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;

            if (PyObject * temp = PyDict_GetItemString(sampler, "border_color")) {
                if (!temp || !PyUnicode_CheckExact(temp)) {
                    PyErr_Format(PyExc_ValueError, "border_color");
                    return -1;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "transparent_black")) {
                    border_color = is_float ? VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK : VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "opaque_black")) {
                    border_color = is_float ? VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK : VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                }

                if (!PyUnicode_CompareWithASCIIString(temp, "opaque_white")) {
                    border_color = is_float ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_INT_OPAQUE_WHITE;
                }
            }

            VkBool32 normalized_coordinates = true;

            if (PyObject * temp = PyDict_GetItemString(sampler, "normalized_coordinates")) {
                if (!PyObject_IsTrue(temp)) {
                    normalized_coordinates = false;
                }
            }

            binding->sampler_create_info_array[i] = {
                VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                NULL,
                0,
                min_filter,
                mag_filter,
                mipmap_filter,
                address_mode_x,
                address_mode_y,
                address_mode_z,
                lod_bias,
                !!max_anisotropy,
                max_anisotropy,
                false,
                VK_COMPARE_OP_NEVER,
                min_lod,
                max_lod,
                border_color,
                !normalized_coordinates,
            };
        }

        binding->image_view_array[i] = NULL;
        binding->sampler_array[i] = NULL;
        binding->image_array[i] = image;

        uint32_t level = 0;

        if (PyObject * temp = PyDict_GetItemString(item, "level")) {
            if (!temp || !PyLong_CheckExact(temp)) {
                PyErr_Format(PyExc_ValueError, "level");
                return -1;
            }

            level = PyLong_AsUnsignedLong(temp);

            if (PyErr_Occurred()) {
                PyErr_Format(PyExc_ValueError, "level");
                return -1;
            }
        }

        uint32_t levels = image->levels;

        if (PyObject * temp = PyDict_GetItemString(item, "levels")) {
            if (!temp || !PyLong_CheckExact(temp)) {
                PyErr_Format(PyExc_ValueError, "levels");
                return -1;
            }

            levels = PyLong_AsUnsignedLong(temp);

            if (PyErr_Occurred()) {
                PyErr_Format(PyExc_ValueError, "levels");
                return -1;
            }
        }

        uint32_t layer = 0;

        if (PyObject * temp = PyDict_GetItemString(item, "layer")) {
            if (!temp || !PyLong_CheckExact(temp)) {
                PyErr_Format(PyExc_ValueError, "layer");
                return -1;
            }

            layer = PyLong_AsUnsignedLong(temp);

            if (PyErr_Occurred()) {
                PyErr_Format(PyExc_ValueError, "layer");
                return -1;
            }
        }

        uint32_t layers = image->layers;

        if (PyObject * temp = PyDict_GetItemString(item, "layers")) {
            if (!temp || !PyLong_CheckExact(temp)) {
                PyErr_Format(PyExc_ValueError, "layers");
                return -1;
            }

            layers = PyLong_AsUnsignedLong(temp);

            if (PyErr_Occurred()) {
                PyErr_Format(PyExc_ValueError, "layers");
                return -1;
            }
        }

        VkComponentMapping swizzle = {};

        if (PyObject * temp = PyDict_GetItemString(item, "swizzle")) {
            if (!temp || !PyUnicode_CheckExact(temp) || PyObject_Size(temp) != 4) {
                PyErr_Format(PyExc_ValueError, "swizzle");
                return -1;
            }

            const char * str = PyUnicode_AsUTF8(temp);
            swizzle.r = get_swizzle(str[0]);
            swizzle.g = get_swizzle(str[1]);
            swizzle.b = get_swizzle(str[2]);
            swizzle.a = get_swizzle(str[3]);

            if (!swizzle.r && !swizzle.g && !swizzle.b && !swizzle.a) {
                PyErr_Format(PyExc_ValueError, "swizzle");
                return -1;
            }
        }

        binding->image_view_create_info_array[i] = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            image->image,
            VK_IMAGE_VIEW_TYPE_2D,
            image->format,
            swizzle,
            {VK_IMAGE_ASPECT_COLOR_BIT, level, levels, layer, layers},
        };
    }

    return 0;
}
