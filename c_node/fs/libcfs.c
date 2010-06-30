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

static u32_t iptoi(char *s) {
	char tmp[4];
	int i, k = 0;
	u32_t n = 0;
	for (i = 0;; i++) {
		if (k >= 4) {
			return 0;
		}
		if (s[i] == '.' || !s[i] || s[i] == '\n') {
			tmp[k] = 0;
//			printf("n=%d\n", n);
			n = n * 256 + atoi(tmp);
			if (!s[i] || s[i] == '\n') break;
			k = 0;
		} else {
			tmp[k] = s[i];
//			printf("asdf=%d\n", s[i]);
			k++;
		}
	}
	return n;
}

PyObject *wrap_c_getblocks(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u64_t *l = NULL;
	int ret;
	PyObject *reto;

	if (!PyArg_ParseTuple(args, "ks", &dt, &tmp)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("");
	}
	strcpy(path, tmp);
	ret = c_getblocknum((dtree_t *)dt, path);
	if (ret < 0) {
		free(path);
		return Py_BuildValue("");
	}
	if (ret == 0) {
		free(path);
		return Py_BuildValue("(si)", NULL, ret);
	}
	l = malloc(ret*sizeof(u64_t));
	strcpy(path, tmp);
	ret = c_getblocks((dtree_t *)dt, path, l);
	printf("ret=%d\n", ret);
	if (ret <= 0) {
		free(path);
		free(l);
		return Py_BuildValue("");
	}
	free(path);
	reto = Py_BuildValue("(s#i)", (char *)l, ret*sizeof(u64_t), ret);
	free(l);
	return reto;
}

PyObject *wrap_c_open(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u32_t uid;
	u8_t oflags, mode;
	int ret;
	if (!PyArg_ParseTuple(args, "ksbbI", &dt, &tmp,
			&oflags, &mode, &uid)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("i", -1);
	}
	strcpy(path, tmp);
	ret = c_open((dtree_t *)dt, path, oflags, mode, uid);
	free(path);
	return Py_BuildValue("i", ret);
}


PyObject *wrap_c_stat(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u32_t uid;
	stat_t st;
	int ret;
	if (!PyArg_ParseTuple(args, "ksI", &dt, &tmp, &uid)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("");
	}
	strcpy(path, tmp);
	ret = c_stat((dtree_t *)dt, path, uid, &st);
	printf("ret=%d\n", ret);
	free(path);
	if (ret) {
		return Py_BuildValue("");
	}
	return Py_BuildValue("s#", (char *)(&st), sizeof(stat_t));
}

PyObject *wrap_c_read(PyObject *self, PyObject *args)
{
	unsigned long dt;
	u32_t fb, uid;
	u64_t offset, length;
	sendblock_t *blocks;
	PyObject *rets;
	int ret;

	if (!PyArg_ParseTuple(args, "kIKKI",
			&dt, &fb, &offset, &length, &uid)) {
		return NULL;
	}

	blocks = malloc((length/BLOCKSIZE+2) * sizeof(sendblock_t));
	ret = c_read((dtree_t *)dt, fb, offset, length, uid,
			blocks);
	if (ret <= 0) {
		return Py_BuildValue("");
	}
	rets = Py_BuildValue("s#", (char *)blocks, ret*sizeof(sendblock_t));
	free(blocks);
	return rets;
}

PyObject *wrap_c_write(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *buf, *p;
	u32_t fb, uid, *ips;
	u64_t offset, length;
	sendblock_t *blocks;
	PyObject *rets;
	int i = 0, ret, max;
//printf("wrap_c_write\n");
	if (!PyArg_ParseTuple(args, "kIKKsI",
			&dt, &fb, &offset, &length, &tmp, &uid)) {
		return NULL;
	}
	p = buf = malloc(strlen(tmp)+1);
	if (!buf) {
		return Py_BuildValue("");
	}
	strcpy(buf, tmp);
	while (*p) {
		if (*p == ',') {
			i++;
			*p = 0;
		}
		p++;
	}

	p = buf;
	max = i + 1;
	ips = malloc(max*sizeof(u32_t));
	if (!ips) {
		return Py_BuildValue("");
	}
	for (; i >= 0; i--) {
//		printf("%s\n", p);
		ips[i] = iptoi(p);
//		printf("ip=%d\n", ips[i]);
		p += (strlen(p)+1);
	}

	blocks = malloc((length/BLOCKSIZE+2) * sizeof(sendblock_t));
	free(buf);
	ret = c_write((dtree_t *)dt, fb, offset, length, uid,
			ips, max, blocks);
	free(ips);
	if (ret <= 0) {
		return Py_BuildValue("");
	}
	rets = Py_BuildValue("s#", (char *)blocks, ret*sizeof(sendblock_t));
	free(blocks);
	return rets;
}

PyObject *wrap_c_opendir(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp, *path;
	u32_t uid;
	u64_t entnum;
	int ret;
	if (!PyArg_ParseTuple(args, "ksI", &dt, &tmp, &uid)) {
		return NULL;
	}
	path = malloc(strlen(tmp)+1);
	if (!path) {
		return Py_BuildValue("");
	}
	strcpy(path, tmp);
	ret = c_opendir((dtree_t *)dt, path, uid, &entnum);
	free(path);
	return Py_BuildValue("(il)", ret, entnum);
}

PyObject *wrap_c_readdir(PyObject *self, PyObject *args)
{
	unsigned long dt;
	u32_t fb, uid;
	u8_t type;
	u64_t entnum;
	int ret;
	char filename[MAX_FILENAME];
	if (!PyArg_ParseTuple(args, "kIKI", &dt, &fb, &entnum, &uid)) {
		return NULL;
	}
	ret = c_readdir((dtree_t *)dt, fb, entnum, uid, filename, &type);
	if (ret) {
		return Py_BuildValue("");
	}
	return Py_BuildValue("(sb)", filename, type);
}

PyObject *wrap_c_move(PyObject *self, PyObject *args)
{
	unsigned long dt;
	char *tmp1, *tmp2, *path1, *path2;
	u32_t uid;
	int ret;
	if (!PyArg_ParseTuple(args, "kssI", &dt, &tmp1, &tmp2, &uid)) {
		return NULL;
	}
	path1 = malloc(strlen(tmp1)+1);
	path2 = malloc(strlen(tmp2)+1);
	strcpy(path1, tmp1);
	strcpy(path2, tmp2);
	ret = c_move((dtree_t *)dt, path1, path2, uid);
	free(path1);
	free(path2);
	return Py_BuildValue("i", ret);
}

PyObject *wrap_c_remove(PyObject *self, PyObject *args)
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
	ret = c_remove((dtree_t *)dt, path, uid);
	free(path);
	return Py_BuildValue("i", ret);
}

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
		{"c_getblocks", wrap_c_getblocks, METH_VARARGS, ""},
		{"c_open", wrap_c_open, METH_VARARGS, ""},
		{"c_read", wrap_c_read, METH_VARARGS, ""},
		{"c_stat", wrap_c_stat, METH_VARARGS, ""},
		{"c_write", wrap_c_write, METH_VARARGS, ""},
		{"c_opendir", wrap_c_opendir, METH_VARARGS, ""},
		{"c_readdir", wrap_c_readdir, METH_VARARGS, ""},
		{"c_chown", wrap_c_chown, METH_VARARGS, ""},
		{"c_remove", wrap_c_remove, METH_VARARGS, ""},
		{"c_move", wrap_c_move, METH_VARARGS, ""},
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
