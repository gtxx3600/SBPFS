// SBPFS -  DFS
//
// Copyright (C) 2009-2010, SBPFS Developers Team
//
// SBPFS is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// SBPFS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SBPFS; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

#include <Python.h>

#include "cfs.h"

PyObject *wrap_c_mkdir(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *path;
	unsigned int uid;
	int ret;
	if (!PyArg_ParseTuple(args, "ksI", &dt, &path, &uid)) {
		return NULL;
	}
	ret = c_mkdir((dtree_t *)dt, path, uid);
	return Py_BuildValue("i", ret);
}

PyObject *wrap_open_disk(PyObject *self, PyObject *args)
{
	char *dt_name, *in_name;
	unsigned long ret;
	if (!PyArg_ParseTuple(args, "ss", &dt_name, &in_name)) {
		return NULL;
	}
	ret = (unsigned long)open_disk(dt_name, in_name);
	return Py_BuildValue("k", ret);
}

PyObject *wrap_save_disk(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *dt_name, *in_name;
	int ret;
	if (!PyArg_ParseTuple(args, "kss", &dt, &dt_name, &in_name)) {
		return NULL;
	}
	ret = save_disk((dtree_t *)dt, dt_name, in_name);
	return Py_BuildValue("i", ret);
}

PyObject *wrap_close_disk(PyObject *self, PyObject *args)
{
	unsigned long dt;
	if (!PyArg_ParseTuple(args, "k", &dt)) {
		return NULL;
	}
	close_disk((dtree_t *)dt);
	return Py_BuildValue("");
}

static PyMethodDef libcfsMethods[] = {
		{"c_mkdir", wrap_c_mkdir, METH_VARARGS, ""},
		{"open_disk", wrap_open_disk, METH_VARARGS, ""},
		{"save_disk", wrap_save_disk, METH_VARARGS, ""},
		{"close_disk", wrap_close_disk, METH_VARARGS, ""},
		{NULL, NULL},
};

void initlibcfs()
{
	PyObject *m;
	m = Py_InitModule("libcfs", libcfsMethods);
}
