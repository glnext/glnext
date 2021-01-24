#include "glnext.hpp"

Memory * Instance_meth_memory(Instance * self, PyObject * vargs, PyObject * kwargs) {
    static char * keywords[] = {"size", NULL};

    struct {
        VkDeviceSize size;
    } args;

    if (!PyArg_ParseTupleAndKeywords(vargs, kwargs, "k", keywords, &args.size)) {
        return NULL;
    }

    Memory * res = new_memory(self, false);
    res->offset = args.size;
    allocate_memory(res);
    res->offset = 0;
    return res;
}
