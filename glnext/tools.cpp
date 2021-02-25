#include "glnext.hpp"

struct vec3 {
    double x, y, z;
};

vec3 operator - (const vec3 & a, const vec3 & b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3 normalize(const vec3 & a) {
    const double l = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    return {a.x / l, a.y / l, a.z / l};
}

vec3 cross(const vec3 & a, const vec3 & b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

double dot(const vec3 & a, const vec3 & b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

PyObject * glnext_meth_camera(PyObject * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"eye", "target", "up", "fov", "aspect", "near", "far", "size", NULL};

    vec3 eye;
    vec3 target;
    vec3 up = {0.0, 0.0, 1.0};
    double fov = 60.0;
    double aspect = 1.0;
    double znear = 0.1;
    double zfar = 1000.0;
    double size = 1.0;

    int args_ok = PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "(ddd)(ddd)|(ddd)ddddd",
        keywords,
        &eye.x,
        &eye.y,
        &eye.z,
        &target.x,
        &target.y,
        &target.z,
        &up.x,
        &up.y,
        &up.z,
        &fov,
        &aspect,
        &znear,
        &zfar,
        &size
    );

    if (!args_ok) {
        return NULL;
    }

    const vec3 f = normalize(target - eye);
    const vec3 s = normalize(cross(f, up));
    const vec3 u = cross(s, f);
    const vec3 t = {-dot(s, eye), -dot(u, eye), -dot(f, eye)};

    if (!fov) {
        const double r1 = size;
        const double r2 = r1 * aspect;
        const double r3 = 1.0 / (zfar - znear);
        const double r4 = znear / (zfar - znear);

        float res[] = {
            (float)(s.x / r2), (float)(u.x / r1), (float)(r3 * f.x), 0.0f,
            (float)(s.y / r2), (float)(u.y / r1), (float)(r3 * f.y), 0.0f,
            (float)(s.z / r2), (float)(u.z / r1), (float)(r3 * f.z), 0.0f,
            0.0f, 0.0f, (float)(r3 * t.z - r4), 1.0f,
        };

        return PyBytes_FromStringAndSize((char *)res, 64);
    }

    const double r1 = tan(fov * 0.01745329251994329576923690768489 / 2.0);
    const double r2 = r1 * aspect;
    const double r3 = zfar / (zfar - znear);
    const double r4 = (zfar * znear) / (zfar - znear);

    float res[] = {
        (float)(s.x / r2), (float)(u.x / r1), (float)(r3 * f.x), (float)f.x,
        (float)(s.y / r2), (float)(u.y / r1), (float)(r3 * f.y), (float)f.y,
        (float)(s.z / r2), (float)(u.z / r1), (float)(r3 * f.z), (float)f.z,
        (float)(t.x / r2), (float)(t.y / r1), (float)(r3 * t.z - r4), (float)t.z,
    };

    return PyBytes_FromStringAndSize((char *)res, 64);
}

PyObject * glnext_meth_rgba(PyObject * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"data", "format", NULL};

    struct {
        PyObject * data;
        PyObject * format;
    } args;

    int args_ok = PyArg_ParseTupleAndKeywords(
        vargs,
        kwargs,
        "OO!",
        keywords,
        &args.data,
        &PyUnicode_Type,
        &args.format
    );

    if (!args_ok) {
        return NULL;
    }

    Py_buffer view = {};
    if (PyObject_GetBuffer(args.data, &view, PyBUF_SIMPLE)) {
        return NULL;
    }

    PyObject * res = NULL;

    if (!PyUnicode_CompareWithASCIIString(args.format, "rgba")) {
        res = PyBytes_FromStringAndSize((char *)view.buf, view.len);
    }

    if (!PyUnicode_CompareWithASCIIString(args.format, "bgr")) {
        uint32_t pixel_count = (uint32_t)view.len / 3;
        res = PyBytes_FromStringAndSize(NULL, pixel_count * 4);
        uint8_t * data = (uint8_t *)PyBytes_AsString(res);
        uint8_t * src = (uint8_t *)view.buf;
        while (pixel_count--) {
            data[0] = src[2];
            data[1] = src[1];
            data[2] = src[0];
            data[3] = 255;
            data += 4;
            src += 3;
        }
    }

    if (!PyUnicode_CompareWithASCIIString(args.format, "rgb")) {
        uint32_t pixel_count = (uint32_t)view.len / 3;
        res = PyBytes_FromStringAndSize(NULL, pixel_count * 4);
        uint8_t * data = (uint8_t *)PyBytes_AsString(res);
        uint8_t * src = (uint8_t *)view.buf;
        while (pixel_count--) {
            data[0] = src[0];
            data[1] = src[1];
            data[2] = src[2];
            data[3] = 255;
            data += 4;
            src += 3;
        }
    }

    if (!PyUnicode_CompareWithASCIIString(args.format, "bgra")) {
        uint32_t pixel_count = (uint32_t)view.len / 4;
        res = PyBytes_FromStringAndSize(NULL, view.len);
        uint8_t * data = (uint8_t *)PyBytes_AsString(res);
        uint8_t * src = (uint8_t *)view.buf;
        while (pixel_count--) {
            data[0] = src[2];
            data[1] = src[1];
            data[2] = src[0];
            data[3] = src[3];
            data += 4;
            src += 4;
        }
    }

    if (!PyUnicode_CompareWithASCIIString(args.format, "lum")) {
        uint32_t pixel_count = (uint32_t)view.len;
        res = PyBytes_FromStringAndSize(NULL, pixel_count * 4);
        uint8_t * data = (uint8_t *)PyBytes_AsString(res);
        uint8_t * src = (uint8_t *)view.buf;
        while (pixel_count--) {
            data[0] = src[0];
            data[1] = src[0];
            data[2] = src[0];
            data[3] = 255;
            data += 4;
            src += 1;
        }
    }

    if (!res) {
        PyBuffer_Release(&view);
        PyErr_Format(PyExc_ValueError, "invalid format");
        return NULL;
    }

    PyBuffer_Release(&view);
    return res;
}

PyObject * glnext_meth_pack(PyObject * self, PyObject ** args, Py_ssize_t nargs) {
    ModuleState * state = (ModuleState *)PyModule_GetState(self);

    if (nargs != 1 && nargs != 2) {
        PyErr_Format(PyExc_TypeError, "wrong arguments");
        return NULL;
    }

    PyObject * seq = PySequence_Fast(args[nargs - 1], "not iterable");
    if (!seq) {
        return NULL;
    }

    PyObject ** array = PySequence_Fast_ITEMS(seq);

    uint32_t row_size = 0;
    uint32_t row_items = 0;
    uint32_t format_count = 0;
    Format format_array[256];

    if (nargs == 1) {
        PyObject * vformat = PyFloat_CheckExact(array[0]) ? state->one_float_str : state->one_int_str;
        format_array[0] = get_format(vformat);
        format_count = 1;
        row_items = 1;
        row_size = 4;
    } else {
        PyObject * vformat = PyUnicode_Split(args[0], NULL, -1);
        if (!vformat) {
            Py_DECREF(seq);
            return NULL;
        }
        format_count = (uint32_t)PyList_Size(vformat);
        for (uint32_t k = 0; k < format_count; ++k) {
            format_array[k] = get_format(PyList_GetItem(vformat, k));
            row_size += format_array[k].size;
            row_items += format_array[k].items;
        }
        Py_DECREF(vformat);
    }

    uint32_t rows = (uint32_t)PySequence_Fast_GET_SIZE(seq) / row_items;

    PyObject * res = PyBytes_FromStringAndSize(NULL, rows * row_size);
    char * data = PyBytes_AsString(res);
    memset(data, 0, rows * row_size);

    for (uint32_t i = 0; i < rows; ++i) {
        for (uint32_t k = 0; k < format_count; ++k) {
            format_array[k].packer(data, array);
            data += format_array[k].size;
            array += format_array[k].items;
        }
    }

    Py_DECREF(seq);

    if (PyErr_Occurred()) {
        Py_DECREF(res);
        return NULL;
    }

    return res;
}
