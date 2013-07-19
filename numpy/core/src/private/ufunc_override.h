#ifndef __UFUNC_OVERRIDE_H
#define __UFUNC_OVERRIDE_H
#include <npy_config.h>
#include "numpy/arrayobject.h"
#include "multiarray/common.h"


/* Normalizes the args passed to ufunc. Returns a tuple of the data
 * args and dict of the kwargs.
 */
NPY_NO_EXPORT PyObject *
PyUFunc_CheckOverride(PyUFuncObject *ufunc, PyObject *ufunc_method,
                    PyObject *args, PyObject *kwds)
{
    int i;
    int nargs = PyTuple_GET_SIZE(args);

    PyObject *obj;
    PyObject *override_args;

    PyObject *override = NULL;
    PyObject *res = NULL;

    for (i = 0; i < nargs; i++) {
        obj = PyTuple_GET_ITEM(args, i);
        if (PyArray_CheckExact(obj) || PyArray_IsAnyScalar(obj)) {
            continue;
        }

        if (PyObject_HasAttrString(obj, "__numpy_ufunc__")) {
            override = PyObject_GetAttrString(obj, "__numpy_ufunc__");
            override_args = Py_BuildValue("OOiO", ufunc, ufunc_method, i, args);
            res = PyObject_Call(override, override_args, kwds);

            Py_DECREF(override);
            Py_DECREF(override_args);
            if (res == Py_NotImplemented) {
                continue;
            }
            else {
                return res;
            }
        }
    }
    if (res) {
        PyErr_SetString(PyExc_TypeError, 
                "Not implemented for this type.");
        return NULL;
    }
    else {
        return NULL;
    }
}

/* Check for __numpy_ufunc__ and call it appropriately. */
NPY_NO_EXPORT PyObject *
PyUFunc_GetOverride(PyObject *ufunc, PyObject *args, PyObject *kwds)
{
    int i;
    int nargs = PyTuple_GET_SIZE(args);
    int noa = 0;
    PyObject *obj;
    PyObject *with_override[NPY_MAXARGS], *overrides[NPY_MAXARGS];
    PyObject *override = NULL, *override_dict = NULL;

    for (i = 0; i < nargs; i++) {
        obj = PyTuple_GET_ITEM(args, i);
        if (PyArray_CheckExact(obj) || PyArray_IsAnyScalar(obj)) {
            continue;
        }
        override_dict = PyObject_GetAttrString(obj, "__ufunc_override__");
        if (override_dict) {
            if (PyDict_CheckExact(override_dict)) {
                override = PyDict_GetItem(override_dict, ufunc);
                if (PyCallable_Check(override)) {
                    with_override[noa] = obj;
                    overrides[noa] = override;
                    ++noa;
                } 
                override = NULL;
            }
            Py_DECREF(override_dict);
            override_dict = NULL;
        }
        else {
            PyErr_Clear();
        }
    }
    if (noa > 0) {
        /* If we have some overrides, find the one of the highest priority. */
        override = overrides[0];
        if (noa > 1) {
            double maxpriority = PyArray_GetPriority(with_override[0], 
                    NPY_PRIORITY);
            for (i = 1; i < noa; i++) {
                double priority = PyArray_GetPriority(with_override[i],
                        NPY_PRIORITY);
                if (priority > maxpriority) {
                    maxpriority = priority;
                    override = overrides[i];
                }
            }
        }
    }
    return override;
}

NPY_NO_EXPORT int *
PyUFunc_HasOverride(PyObject *args)
{
    int i;
    int nargs = PyTuple_GET_SIZE(args);
    int noa = 0;
    PyObject *obj;
    PyObject *with_override[NPY_MAXARGS], *overrides[NPY_MAXARGS];
    PyObject *override = NULL, *override_dict = NULL;

    for (i = 0; i < nargs; i++) {
        obj = PyTuple_GET_ITEM(args, i);
        if (PyArray_CheckExact(obj) || PyArray_IsAnyScalar(obj)) {
            continue;
        }
        override_dict = PyObject_GetAttrString(obj, "__ufunc_override__");
        if (override_dict) {
            if (PyDict_CheckExact(override_dict)) {
                Py_DECREF(override_dict);
                return 1;
            }
        }
        else {
            Py_DECREF(override_dict);
            PyErr_Clear();
        }
    }
    return 0;
}
#endif
