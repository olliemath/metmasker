#include <Python.h>
#include "structmember.h"
#include "numpy/arrayobject.h"
#include "loader.h"
#include "algorithms.h"


/* ====== FUNCTION FOR ERROR HANDLING ===== */
static void masker_translate_error_codes(int code, const char* file_name) {
  PyObject* exception;
  char *message;
  switch (code) {
    case MASKER_IO_ERROR:
      exception = PyExc_IOError;
      message = "Error reading %s";
      break;
    case MASKER_MEMORY_ERROR:
      exception = PyExc_MemoryError;
      message = "Memory error while processing %s";
      break;
    case MASKER_INIT_IO_ERROR:
      exception = PyExc_IOError;
      message = "Error during png_init_io reading %s";
      break;
    case MASKER_COLOR_TYPE_ERROR:
      exception = PyExc_AttributeError;
      message = "Image has unrecognized color type: %s";
      break;
    case MASKER_READ_ERROR:
      exception = PyExc_IOError;
      message = "Error reading %s";
      break;
    case MASKER_WRITE_ERROR:
      exception = PyExc_IOError;
      message = "Error writing to %s";
      break;
    case MASKER_IMAGE_SIZE_DEPTH_ERROR:
      exception = PyExc_AttributeError;
      message = "Image has incorrect size/bit-depth: %s";
      break;
    case MASKER_MET_COLOR_ERROR:
      exception = PyExc_AttributeError;
      message = "Image has unrecognized met-office color: %s";
      break;
    case MASKER_NOT_PNG_ERROR:
      exception = PyExc_AttributeError;
      message = "File at %s was not of type PNG.";
      break;
    default:
      exception = PyExc_RuntimeError;
      message = "MetMasker encountered a runtime error on %s.";
      break;
  }

  PyErr_Format(exception, message, file_name);
}

/* ====== MASK TYPE ====== */
typedef struct {
    PyObject_HEAD
    masker_mask_t mask;
} masker_MaskObject;

static void masker_MaskObject_dealloc(masker_MaskObject* self)
{
  free_mask_memory(&(self->mask));
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject* masker_MaskObject_new(
  PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  masker_MaskObject *self;
  self = (masker_MaskObject*)type->tp_alloc(type, 0);
  if (self != NULL) {
    masker_mask_t mask = {.image = NULL, .is_freed=1};
    self->mask = mask;
  }
  return (PyObject*)self;
}

static int masker_MaskObject_init(
  masker_MaskObject *self, PyObject *args, PyObject *kwds)
{
  char *file_name;
  static char *kwlist[] = {"image_path", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &file_name))
    return -1;

  int error_bit = read_mask_file(&(self->mask), file_name);
  if (error_bit != MASKER_SUCCESS) {
    masker_translate_error_codes(error_bit, file_name);
    return -1;
  }

  return 0;
}

static PyObject* masker_MaskObject_mask_total_met(
  masker_MaskObject *self, PyObject *args, PyObject *kwargs)
{
  const char *file_name;
  static char *kwlist[] = {"file_name", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file_name))
    return NULL;

  float res;
  int error_bit = mask_total_met_image(&res, self->mask, file_name);

  if (error_bit != MASKER_SUCCESS) {
    masker_translate_error_codes(error_bit, file_name);
    return NULL;
  }

  return Py_BuildValue("f", res);
}

static PyObject* masker_MaskObject_mask_total_gray(
  masker_MaskObject *self, PyObject *args, PyObject *kwargs)
{
  const char *file_name;
  static char *kwlist[] = {"file_name", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file_name))
    return NULL;

  float res;
  int error_bit = mask_total_gray_image(&res, self->mask, file_name);
  if (error_bit != MASKER_SUCCESS) {
    masker_translate_error_codes(error_bit, file_name);
    return NULL;
  }

  return Py_BuildValue("f", res);
}

static PyObject* masker_MaskObject_load_mask_gray(
  masker_MaskObject *self, PyObject *args, PyObject *kwargs)
{
  const char *file_name;
  static char *kwlist[] = {"in_file", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file_name))
    return NULL;

  static npy_intp dims[2] = {HEIGHT, WIDTH};
  PyArrayObject *array = (PyArrayObject*)PyArray_SimpleNew(2, dims, NPY_FLOAT);
  if (array == NULL) {
    return NULL;
  }

  float* data_ptr = (float*)array->data;
  int error_bit = mask_gray_image(data_ptr, self->mask, file_name);
  if (error_bit != MASKER_SUCCESS) {
    Py_DECREF(array);
    masker_translate_error_codes(error_bit, file_name);
    return NULL;
  }

  return PyArray_Return(array);
}


static PyObject* masker_MaskObject_mask_split_gray(
  masker_MaskObject *self, PyObject *args, PyObject *kwargs)
{
  const char *file_name;
  static char *kwlist[] = {"file_name", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file_name))
    return NULL;

  static npy_intp dims[3] = {8, HEIGHT, WIDTH};
  PyArrayObject *array = (PyArrayObject*)PyArray_SimpleNew(3, dims, NPY_FLOAT);
  if (array == NULL) {
    return NULL;
  }

  float *data_ptr = (float*)array->data;
  int error_bit = mask_split_gray_image(data_ptr, self->mask, file_name);
  if (error_bit != MASKER_SUCCESS) {
    masker_translate_error_codes(error_bit, file_name);
    Py_DECREF(array);
    return NULL;
  }

  return PyArray_Return(array);
}

static PyMethodDef masker_MaskObject_methods[] = {
  {"total_met", (PyCFunction)masker_MaskObject_mask_total_met,
   METH_VARARGS | METH_KEYWORDS, "Mask met image and sum rain values."},
  {"total_gray", (PyCFunction)masker_MaskObject_mask_total_gray,
   METH_VARARGS | METH_KEYWORDS,
   "Mask converted grayscale image and sum rain values."},
  {"load_gray", (PyCFunction)masker_MaskObject_load_mask_gray,
   METH_VARARGS | METH_KEYWORDS,
  "Load grayscale image to numpy array."},
  {"load_channels", (PyCFunction)masker_MaskObject_mask_split_gray,
   METH_VARARGS | METH_KEYWORDS,
  "Load grayscale image to numpy arrays with channels for rain types."},
  {NULL}
};

static PyTypeObject masker_MaskType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "masker.Mask",             /*tp_name*/
    sizeof(masker_MaskObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)masker_MaskObject_dealloc,                  /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    "Mask object",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    masker_MaskObject_methods,             /* tp_methods */
    0, // masker_MaskObject_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)masker_MaskObject_init,      /* tp_init */
    0,                         /* tp_alloc */
    masker_MaskObject_new,                 /* tp_new */
};

static PyObject* masker_save_met_to_gray(
  PyObject *self, PyObject *args, PyObject *kwargs)
{
  const char *in_file;
  const char *out_file;
  static char *kwlist[] = {"in_file", "out_file", NULL};
  if (!PyArg_ParseTupleAndKeywords(
    args, kwargs, "ss", kwlist, &in_file, &out_file)) return NULL;

  masker_image_t res;
  int error_bit = met_image_to_gray(&res, in_file);
  if (error_bit != MASKER_SUCCESS) {
    masker_translate_error_codes(error_bit, in_file);
    return NULL;
  }

  error_bit = write_png_file(res, out_file);
  if (error_bit != MASKER_SUCCESS) {
    masker_translate_error_codes(error_bit, out_file);
    free_image_memory(&res);
    return NULL;
  }

  free_image_memory(&res);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* masker_load_gray(
  masker_MaskObject *self, PyObject *args, PyObject *kwargs)
{
  const char *file_name;
  static char *kwlist[] = {"in_file", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &file_name))
    return NULL;

  npy_intp dims[2] = {HEIGHT, WIDTH};
  PyArrayObject *array = (PyArrayObject*)PyArray_SimpleNew(2, dims, NPY_FLOAT);
  if (array == NULL) {
    return NULL;
  }

  float* data_ptr = (float*)array->data;
  int error_bit = load_gray_to_array(data_ptr, file_name);
  if (error_bit != MASKER_SUCCESS) {
    Py_DECREF(array);
    masker_translate_error_codes(error_bit, file_name);
    return NULL;
  }

  return PyArray_Return(array);
}


static PyMethodDef masker_methods[] = {
  {"met_to_gray", (PyCFunction)masker_save_met_to_gray,
   METH_VARARGS | METH_KEYWORDS,
   "Convert met image to grayscale.\nUsage: met_to_gray(in_file, out_file)."},
  {"load_gray", (PyCFunction)masker_load_gray,
   METH_VARARGS | METH_KEYWORDS, "Load grayscale image to numpy array."},
  {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC initmasker(void)
{
  PyObject* m;

  if (PyType_Ready(&masker_MaskType) < 0)
      return;

  m = Py_InitModule3("masker", masker_methods,
  "Utilities for masking/converting met office images.");

  if (m == NULL) return;

  import_array();
  Py_INCREF(&masker_MaskType);
  PyModule_AddObject(m, "Mask", (PyObject *)&masker_MaskType);
}
