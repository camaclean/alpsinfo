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


static appInfo_t appinfo;
static cmdDetail_t *cmd_details;
static placeNodeList_ver3_t *placement_details;
static uint64_t our_apid = 0;
static char *err_msg;
static int err;

static PyObject *
alpsinfo_apid(PyObject *self, PyObject *noargs) {
    if (our_apid)
        return PyLong_FromUnsignedLongLong(our_apid);
    else
        Py_RETURN_NONE;
}

static PyObject *
alpsinfo_accel(PyObject *self, PyObject *noargs) {
    if (our_apid) {
        switch (cmd_details[0].accelType) {
            case accel_arch_gpu:
                return MAKE_STRING("GPU");
            case accel_arch_knc:
		return MAKE_STRING("KNC");
            case accel_arch_none:
            default:
		return MAKE_STRING("None");
        }
    } else {
        Py_RETURN_NONE;
    }
}

#define MAKE_FN_NAME(x) alpsinfo_ ## x
#define ALPS_PROPERTY(prop) \
static PyObject * \
MAKE_FN_NAME(prop) (PyObject *self, PyObject *noargs) { \
    if (our_apid) \
        return PyLong_FromLong(cmd_details[0].prop); \
    else \
        return PyLong_FromLong(0); \
} 

ALPS_PROPERTY(width)
ALPS_PROPERTY(depth)
ALPS_PROPERTY(fixedPerNode)
ALPS_PROPERTY(nodeCnt)
ALPS_PROPERTY(cpusPerCU)
ALPS_PROPERTY(pesPerSeg)
ALPS_PROPERTY(nodeSegCnt)
ALPS_PROPERTY(segBits)

static PyObject *
alpsinfo_createinfo(void) {
    if (our_apid == 0)
        Py_RETURN_NONE;

    PyObject *alps_info = PyDict_New();

    PyObject *apid_key = MAKE_STRING("apid");
    PyObject *apid = PyLong_FromUnsignedLongLong(our_apid);
    PyDict_SetItem(alps_info, apid_key, apid);
    Py_DECREF(apid_key);
    Py_DECREF(apid);


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

    PyDict_SetItem(alps_info, cmds_key, cmds);
    Py_DECREF(cmds_key);
    Py_DECREF(cmds);

    Py_INCREF(alps_info);
    return alps_info;
}

static PyObject *
alpsinfo_info(PyObject *self, PyObject *noargs) {
    struct module_state *st = GETSTATE(self);
    if (st->alps_info == NULL)
        st->alps_info = alpsinfo_createinfo();
    else
        Py_INCREF(st->alps_info);
    return st->alps_info;
}
//ALPS_PROPERTY(nodeCnt)
//ALPS_PROPERTY(cpusPerCU)
//ALPS_PROPERTY(pesPerSeg)
//ALPS_PROPERTY(nodeSegCnt)
//ALPS_PROPERTY(segBits)

static PyMethodDef
alpsinfo_functions[] = {
    { "info", alpsinfo_info, METH_NOARGS, "ALPS info method" },
    { "apid", alpsinfo_apid, METH_NOARGS, "ALPS apid" },
    { "width", alpsinfo_width, METH_NOARGS, "ALPS width (-n) for the first command" },
    { "depth", alpsinfo_depth, METH_NOARGS, "ALPS depth (-d) for the first command" },
    { "fixedPerNode", alpsinfo_fixedPerNode, METH_NOARGS, "ALPS pes_per_node (-N) for the first command"},
    { "cpusPerCU", alpsinfo_cpusPerCU, METH_NOARGS, "ALPS cpus_per_cu (-j) for the first command" },
    { "pesPerSeg", alpsinfo_pesPerSeg, METH_NOARGS, "ALPS pes_per_numa_node (-S) for the first command" },
    { "nodeSegCnt", alpsinfo_nodeSegCnt, METH_NOARGS, "ALPS numa_nodes_per_node (-sn) for the first command" },
    { "segBits", alpsinfo_segBits, METH_NOARGS, "ALPS list_of_numa_node bits (-sl) for the first command" },
    { "nodeCnt", alpsinfo_nodeCnt, METH_NOARGS, "The number of nodes used by the first command" },
    { "accel", alpsinfo_accel, METH_NOARGS, "The requested accelerator type" },
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
    st->alps_info = NULL;
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

    our_apid = get_this_apid();
    if (our_apid) {
        printf("Our Apid: %d\n", our_apid);
        alps_get_appinfo_ver3_err(our_apid, &appinfo, &cmd_details, &placement_details, &err_msg, &err);
    }

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
    
}
