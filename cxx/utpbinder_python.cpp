/*
 * Copyright (c) 2015, Shinya Yagyu
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its 
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include <list>  

#include "Storjutp.hpp"
#include <Python.h>
#include "bytesobject.h"

#ifndef LOG
void *logging0(const char *file, int line, const char *function, 
               const char * format, ...)
{
  char buffer[256];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer, 256, format, args);
  fprintf(stderr,"%s:%d %s() %s\n", file, line, function, buffer);
  fflush(stderr);
  va_end (args);
  return NULL;

}
#define LOG(fmt, ...)  logging0(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#endif

void EvaluatePyObject(PyObject *obj, unsigned char hash[32], 
                        const char *errorMessage){
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    if(!PyCallable_Check(obj)){
        LOG("not callable object");
    }
#if PY_MAJOR_VERSION >= 3
    PyObject *arglist = Py_BuildValue("(y#z)", hash, 32, errorMessage);
#else
    PyObject *arglist = Py_BuildValue("(z#z)", hash, 32, errorMessage);
#endif
    if(!arglist){
        LOG("failed to build arg list");
    }
    PyObject *ret = PyObject_CallObject(obj, arglist);
    if(!ret){
        PyErr_PrintEx(1);
    }
    Py_XDECREF(ret);
    Py_XDECREF(arglist);
    PyGILState_Release(gstate);
}
    
class HandlerImpl : public Handler{
private:
    PyObject *pyHandler;
    
public:
	HandlerImpl(PyObject *pyHandler){
        Py_XINCREF(pyHandler);
        this->pyHandler=pyHandler;
	}
    
    bool hasPyHandler(PyObject *p){
        return pyHandler == p;
    }

	virtual int on_finish(unsigned char hash[32], 
                           const char *errMessage){
        EvaluatePyObject(pyHandler, hash, errMessage);
        return 1;
	}
    
    ~HandlerImpl(){
        Py_XDECREF(pyHandler);
    }
};

static PyObject *utpbinder_init(PyObject *self, PyObject *args){
    int port=0;
    
    if (! PyEval_ThreadsInitialized()) {
        PyEval_InitThreads();
    }
    if (!PyArg_ParseTuple(args, "i", &port)){
        return NULL;
    }

    Storjutp *m=new Storjutp(port);
    PyObject *po=PyCapsule_New(m, NULL,NULL);
    return po;
}

static PyObject *utpbinder_registHash(PyObject *self, 
                                                PyObject *args){
    PyObject *cobj=NULL;
    PyByteArrayObject *hash_=NULL;
    PyObject *handler=NULL;
    char *dir=NULL;
    if (!PyArg_ParseTuple(args, "OOOs",&cobj,&hash_,&handler,&dir)){
        return NULL;
    }
    if (!PyCallable_Check(handler)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        return NULL;
    } 
    unsigned char* hash = (unsigned char *)
                                PyByteArray_AsString((PyObject*) hash_);
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    HandlerImpl *h=new HandlerImpl(handler);
    int r = m->registHash(hash,h,dir);
    return Py_BuildValue("i", r);
}

static PyObject *utpbinder_getServerPort(PyObject *self, 
                                                PyObject *args){
    PyObject *cobj=NULL;
    if (!PyArg_ParseTuple(args, "O",&cobj)){
        return NULL;
    }
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    return Py_BuildValue("i", m->server_port);
}

static PyObject *utpbinder_stopHash(PyObject *self, 
                                                   PyObject *args){
    PyObject *cobj=NULL;
    PyByteArrayObject *hash_=NULL;
    if (!PyArg_ParseTuple(args, "OO",&cobj,&hash_)){
        return NULL;
    }
    unsigned char* hash = (unsigned char *)
                                PyByteArray_AsString((PyObject*) hash_);
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    m->stopHash(hash);
    Py_RETURN_NONE;
}

static PyObject *utpbinder_sendFile(PyObject *self, 
                                                   PyObject *args){
    PyObject *cobj=NULL;
    char *dest=NULL;
    int port = 0;
    char *fname=NULL;
    PyByteArrayObject *hash_=NULL;
    PyObject *handler=NULL;
    if (!PyArg_ParseTuple(args, "OsisOO",&cobj,&dest,&port,&fname,
                                        &hash_,&handler)){
        return NULL;
    }
    if (!PyCallable_Check(handler)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        return NULL;
    } 
    if (!PyByteArray_Check(hash_)) {
        PyErr_SetString(PyExc_TypeError, "parameter must be bytearray");
        return NULL;
    }
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    HandlerImpl *h = new HandlerImpl(handler);
    unsigned char *hash = (unsigned char*)PyByteArray_AsString((PyObject*) hash_);
    int r = m->sendFile(dest,port,fname,hash,h);
    return Py_BuildValue("i", r);
}


static PyObject *utpbinder_start(PyObject *self, PyObject *args){
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject *cobj=NULL;
    if (!PyArg_ParseTuple(args, "O",&cobj)){
        return NULL;
    }
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    Py_BEGIN_ALLOW_THREADS
        m->start();
    Py_END_ALLOW_THREADS
    PyGILState_Release(gstate);
    Py_RETURN_NONE;
}

static PyObject *utpbinder_setStopFlag(PyObject *self, PyObject *args){
    PyObject *cobj=NULL;
    int flag=0;
    if (!PyArg_ParseTuple(args, "Oi",&cobj,&flag)){
        return NULL;
    }
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    m->setStopFlag(flag);
    Py_RETURN_NONE;
}

static PyObject *utpbinder_getProgress(PyObject *self, 
                                             PyObject *args){
    PyObject *cobj=NULL;
    PyByteArrayObject *hash_=NULL;
    if (!PyArg_ParseTuple(args, "OO",&cobj,&hash_)){
        return NULL;
    }
    unsigned char* hash = (unsigned char *)
                             PyByteArray_AsString((PyObject*) hash_);
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    size_t s = m->getProgress(hash);
    return Py_BuildValue("k", s);
}

static PyObject *utpbinder_finalize(PyObject *self, 
                                             PyObject *args){
    PyObject *cobj=NULL;
    if (!PyArg_ParseTuple(args, "O",&cobj)){
        return NULL;
    }
    Storjutp *m=(Storjutp *)PyCapsule_GetPointer(cobj,NULL);
    delete m;
    Py_RETURN_NONE;
}

static PyMethodDef methods[] = {
    {"init", utpbinder_init, METH_VARARGS, 
        "initialize"},
    {"regist_hash", utpbinder_registHash, METH_VARARGS,
         "regist a acceptable hash"},
    {"stop_hash", utpbinder_stopHash, METH_VARARGS,
         "unregist a acceptable hash"},
    {"send_file", utpbinder_sendFile, METH_VARARGS,
         "start sending a file"},
    {"start", utpbinder_start, METH_VARARGS,
         "start receiving/downloading loop"},
    {"set_stopflag", utpbinder_setStopFlag, METH_VARARGS,
         "stop receiving loop"},
    {"get_serverport", utpbinder_getServerPort, METH_VARARGS
        , "get listening port"},
    {"get_progress", utpbinder_getProgress, METH_VARARGS
        , "get downloaded/uploaded size"},
    {"finalize", utpbinder_finalize, METH_VARARGS
        , "finalize C object."},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef modules = {
   PyModuleDef_HEAD_INIT,
   "utpbinder",   /* name of module */
   "File Transfer library by libutp", /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   methods
   };

extern "C" PyMODINIT_FUNC PyInit_utpbinder(void){
     return PyModule_Create(&modules);
}

#else
extern "C" PyMODINIT_FUNC
initutpbinder(void) {
    (void) Py_InitModule("utpbinder", methods);
}

#endif
