#include <Python.h>

#include <stdlib.h>
#include <alps/apInfo.h>

struct module_state {
    PyObject *alps_info;
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

#if PY_MAJOR_VERSION >= 3
#define MAKE_STRING(value) PyUnicode_FromString(value)
#else
#define MAKE_STRING(value) PyString_FromString(value)
#endif

static uint64_t get_this_apid() {
    const char *s = getenv("ALPS_APP_ID");
    if (s == NULL)
        return 0;
    uint64_t apid = strtoull(s,NULL,10);
    if (errno == ERANGE)
        return 0;
    return apid;
}

// Cray messed up the libalps.h header with an incorrect include, so just copy
// the necessary prototype here.
extern int         alps_get_appinfo_ver3_err(uint64_t apid, appInfo_t *appinfo,
                       cmdDetail_t **cmdDetail, placeNodeList_ver3_t **places,
                       char **errMsg, int *err);

static PyObject *
alpsinfo_info(PyObject *self, PyObject *noargs) {
    appInfo_t appinfo;
    cmdDetail_t *cmd_details;
    placeNodeList_ver3_t *placement_details;
    char *err_msg;
    int err;

    uint64_t our_apid = get_this_apid();
    
    if (our_apid == 0)
        Py_RETURN_NONE;

    PyObject *d = PyDict_New();

    PyObject *apid_key = MAKE_STRING("apid");
    PyObject *apid = PyLong_FromUnsignedLongLong(our_apid);
    PyDict_SetItem(d, apid_key, apid);
    Py_DECREF(apid_key);
    Py_DECREF(apid);

    int ret = alps_get_appinfo_ver3_err(our_apid, &appinfo, &cmd_details, &placement_details, &err_msg, &err);

    PyObject *cmds_key = MAKE_STRING("cmds");
    PyObject *width_key = MAKE_STRING("width");
    PyObject *depth_key = MAKE_STRING("depth");
    PyObject *fixedPerNode_key = MAKE_STRING("fixedPerNode");
    PyObject *nodeCnt_key = MAKE_STRING("nodeCnt");
    PyObject *cpusPerCU_key = MAKE_STRING("cpusPerCU");
    PyObject *pesPerSeg_key = MAKE_STRING("pesPerSeg");
    PyObject *nodeSegCnt_key = MAKE_STRING("nodeSegCnt");
    PyObject *segBits_key = MAKE_STRING("segBits");
    PyObject *accel_key = MAKE_STRING("accel");
    PyObject *accel_none = MAKE_STRING("None");
    PyObject *accel_gpu = MAKE_STRING("GPU");
    PyObject *accel_knc = MAKE_STRING("KNC");


    PyObject *cmds = PyList_New(0);
    for (int i = 0; i < appinfo.numCmds; ++i) {
        PyObject *cmd = PyDict_New();

        PyObject *width = PyLong_FromLong(cmd_details[i].width); // -n value
        PyObject *depth = PyLong_FromLong(cmd_details[i].depth); // -d value
        PyObject *fixedPerNode = PyLong_FromLong(cmd_details[i].fixedPerNode); // -N value
        PyObject *nodeCnt = PyLong_FromLong(cmd_details[i].nodeCnt);
        PyObject *cpusPerCU = PyLong_FromLong(cmd_details[i].cpusPerCU); // -j value
        PyObject *pesPerSeg = PyLong_FromLong(cmd_details[i].pesPerSeg); // -S value
        PyObject *nodeSegCnt = PyLong_FromLong(cmd_details[i].nodeSegCnt); // -sn value
        PyObject *segBits = PyLong_FromLong(cmd_details[i].segBits); // -sl value
        PyObject *accel;
        switch (cmd_details[i].accelType) {
            case accel_arch_gpu:
                accel = accel_gpu;
                break;
            case accel_arch_knc:
                accel = accel_knc;
                break;
            case accel_arch_none:
            default:
                accel = accel_none;
                break;
        }

	PyDict_SetItem(cmd, width_key, width);
	PyDict_SetItem(cmd, depth_key, depth);
	PyDict_SetItem(cmd, fixedPerNode_key, fixedPerNode);
	PyDict_SetItem(cmd, nodeCnt_key, nodeCnt);
	PyDict_SetItem(cmd, cpusPerCU_key, cpusPerCU);
	PyDict_SetItem(cmd, pesPerSeg_key, pesPerSeg);
	PyDict_SetItem(cmd, nodeSegCnt_key, nodeSegCnt);
	PyDict_SetItem(cmd, segBits_key, segBits);
	PyDict_SetItem(cmd, accel_key, accel);

        Py_DECREF(width);
        Py_DECREF(depth);
        Py_DECREF(fixedPerNode);
        Py_DECREF(nodeCnt);
        Py_DECREF(cpusPerCU);
        Py_DECREF(pesPerSeg);
        Py_DECREF(nodeSegCnt);
        Py_DECREF(segBits);

	PyList_Append(cmds, cmd);
        Py_DECREF(cmd);
    }

    Py_DECREF(width_key);
    Py_DECREF(depth_key);
    Py_DECREF(fixedPerNode_key);
    Py_DECREF(nodeCnt_key);
    Py_DECREF(cpusPerCU_key);
    Py_DECREF(pesPerSeg_key);
    Py_DECREF(nodeSegCnt_key);
    Py_DECREF(segBits_key);
    Py_DECREF(accel_key);
    Py_DECREF(accel_none);
    Py_DECREF(accel_gpu);
    Py_DECREF(accel_knc);

    PyDict_SetItem(d, cmds_key, cmds);
    Py_DECREF(cmds_key);
    Py_DECREF(cmds);

    return d;
}


static PyMethodDef
alpsinfo_functions[] = {
    { "info", alpsinfo_info, METH_NOARGS, "ALPS info method" },
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3

static int alpsinfo_traverse(PyObject *m, visitproc visit, void *arg) {
    struct module_state* st = GETSTATE(m);
    Py_VISIT(st->alps_info);
    Py_VISIT(st->error);
    return 0;
}

static int alpsinfo_clear(PyObject *m) {
    struct module_state* st = GETSTATE(m);
    Py_CLEAR(st->alps_info);
    Py_CLEAR(st->error);
    return 0;
}

static struct PyModuleDef alpsinfo_module = {
    PyModuleDef_HEAD_INIT,
    "alpsinfo",   /* name of module */
    NULL, /* module documentation, may be NULL */
    sizeof(struct module_state),       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    alpsinfo_functions,
    NULL,
    alpsinfo_traverse,
    alpsinfo_clear,
    NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit_alpsinfo(void) {
#else
#define INITERROR return

void
initalpsinfo(void) {
#endif
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&alpsinfo_module);
#else
    PyObject *module = Py_InitModule("alpsinfo", alpsinfo_functions);
#endif

    if (module == NULL)
        INITERROR;

    struct module_state *st = GETSTATE(module);
    st->error = PyErr_NewException("alpsinfo.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
    
}
