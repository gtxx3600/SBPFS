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

PyObject *wrap_c_chown(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u32_t uid;
	u32_t newuid;
	int ret;
	if (!PyArg_ParseTuple(args, "ksII", &dt, &tmp, &newuid, &uid)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("i", -1);
	}
	strcpy(path, tmp);
	ret = c_chown((dtree_t *)dt, path, newuid, uid);
	free(path);
	return Py_BuildValue("i", ret);
}

PyObject *wrap_c_chmod(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u32_t uid;
	u8_t mode;
	int ret;
	if (!PyArg_ParseTuple(args, "ksbI", &dt, &tmp, &mode, &uid)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("i", -1);
	}
	strcpy(path, tmp);
	ret = c_chmod((dtree_t *)dt, path, mode, uid);
	free(path);
	return Py_BuildValue("i", ret);
}

PyObject *wrap_c_rmdir(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u32_t uid;
	int ret;
	if (!PyArg_ParseTuple(args, "ksI", &dt, &tmp, &uid)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("i", -1);
	}
	strcpy(path, tmp);
	ret = c_rmdir((dtree_t *)dt, path, uid);
	free(path);
	return Py_BuildValue("i", ret);
}

PyObject *wrap_c_mkdir(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u32_t uid;
	int ret;
	if (!PyArg_ParseTuple(args, "ksI", &dt, &tmp, &uid)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("i", -1);
	}
	strcpy(path, tmp);
	ret = c_mkdir((dtree_t *)dt, path, uid);
	free(path);
	return Py_BuildValue("i", ret);
}

PyObject *wrap_open_disk(PyObject *self, PyObject *args)
{
	char *dt_name, *in_name, *dt_tmp, *in_tmp;
	unsigned long ret;
	if (!PyArg_ParseTuple(args, "ss", &dt_tmp, &in_tmp)) {
		return NULL;
	}
	dt_name = malloc(strlen(dt_tmp)+1);
	if (!dt_name) {
		return Py_BuildValue("i", -1);
	}
	strcpy(dt_name, dt_tmp);
	in_name = malloc(strlen(in_tmp)+1);
	if (!in_name) {
		return Py_BuildValue("i", -1);
	}
	strcpy(in_name, in_tmp);
	ret = (unsigned long)open_disk(dt_name, in_name);
	free(dt_name);
	free(in_name);
	return Py_BuildValue("k", ret);
}

PyObject *wrap_save_disk(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *dt_name, *in_name, *dt_tmp, *in_tmp;
	int ret;
	if (!PyArg_ParseTuple(args, "kss", &dt, &dt_tmp, &in_tmp)) {
		return NULL;
	}
	dt_name = malloc(strlen(dt_tmp)+1);
	if (!dt_name) {
		return Py_BuildValue("i", -1);
	}
	strcpy(dt_name, dt_tmp);
	in_name = malloc(strlen(in_tmp)+1);
	if (!in_name) {
		return Py_BuildValue("i", -1);
	}
	strcpy(in_name, in_tmp);
	ret = save_disk((dtree_t *)dt, dt_name, in_name);
	free(dt_name);
	free(in_name);
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
		{"c_chown", wrap_c_chown, METH_VARARGS, ""},
		{"c_chmod", wrap_c_chmod, METH_VARARGS, ""},
		{"c_rmdir", wrap_c_rmdir, METH_VARARGS, ""},
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
