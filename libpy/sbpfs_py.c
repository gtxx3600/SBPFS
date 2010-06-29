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
#include <sbpfs.h>

PyObject *wrap_sbp_login(PyObject *self, PyObject *args)
{
	char *username,*password;
	int result;
	if (!PyArg_ParseTuple(args, "ss", &username,&password))
		return NULL;

	result = sbp_login(username,password);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_sethost(PyObject *self, PyObject *args)
{
	char *hostname;
	int result;
	if (!PyArg_ParseTuple(args, "s", &hostname))
		return NULL;

	result = sbp_sethost(hostname);
	return Py_BuildValue("i", result);
}

PyObject *wrap_sbp_getUandP(PyObject *self, PyObject *args)
{
	char *username = NULL,*password = NULL;

	PyObject* pDict = PyDict_New();

	sbp_getUandP(&username,&password);
	PyDict_SetItemString(pDict, "username",
	                     Py_BuildValue("s", username));
	PyDict_SetItemString(pDict, "password",
	                     Py_BuildValue("s", password));
	return pDict;
}

PyObject *wrap_sbp_chmod(PyObject *self, PyObject *args)
{
	char *filename;
	unsigned short mode;
	int result;
	if (!PyArg_ParseTuple(args, "sH", &filename, &mode))
		return NULL;
	result = sbp_chmod(filename,mode);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_chown(PyObject *self, PyObject *args)
{
	char *filename,*username;
	int result;
	if (!PyArg_ParseTuple(args, "ss", &filename, &username))
		return NULL;
	result = sbp_chown(filename,username);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_open(PyObject *self, PyObject *args)
{
	char *filename;
	unsigned int oflag;
	unsigned char mode;
	int result;
	if (!PyArg_ParseTuple(args, "sIb", &filename, &oflag,&mode))
		return NULL;
	result = sbp_open(filename,oflag,mode);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_readdir(PyObject *self, PyObject *args)
{
	int fd;

	struct sbp_dirent* result;
	if (!PyArg_ParseTuple(args, "i", &fd))
		return NULL;

	result = sbp_readdir(fd);
	if(result == NULL)
	{
		return Py_BuildValue("");
	}
	PyObject* pTuple = PyTuple_New(3);
	assert(PyTuple_Check(pTuple));
	assert(PyTuple_Size(pTuple) == 3);
	PyTuple_SetItem(pTuple, 0, Py_BuildValue("K", result->offset));
	PyTuple_SetItem(pTuple, 1, Py_BuildValue("s", result->d_type == T_DIR ? "directory" : "file"));
	if(strlen(result->d_name) <= 256){
		PyTuple_SetItem(pTuple, 2, Py_BuildValue("s", result->d_name));
	}
	else{
		PyTuple_SetItem(pTuple, 2, Py_BuildValue("s#", result->d_name,256));
	}
	return pTuple;
}
PyObject *wrap_sbp_read(PyObject *self, PyObject *args)
{
	int fd;
	unsigned long long length;
	char *rec_buf;
	int result;
	if (!PyArg_ParseTuple(args, "iK", &fd, &length))
		return NULL;
	if((rec_buf = (char*)malloc(length))== NULL)
	{
		return Py_BuildValue("");
	}

	result = sbp_read(fd,rec_buf,length);
	PyObject * temp = Py_BuildValue("s#", rec_buf, length);
	free(rec_buf);
	return temp;
}
PyObject *wrap_sbp_write(PyObject *self, PyObject *args)
{
	int fd;
	unsigned long long length;
	char *send_buf;
	unsigned long long result;
	if (!PyArg_ParseTuple(args, "isK", &fd, &send_buf, &length))
		return NULL;

	result = sbp_write(fd,send_buf,length);
	if(result == length)
	{
		return Py_BuildValue("");
	}

	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_opendir(PyObject *self, PyObject *args)
{
	char *filename;
	int result;
	if (!PyArg_ParseTuple(args, "s", &filename))
		return NULL;
	result = sbp_opendir(filename);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_perror(PyObject *self, PyObject *args)
{
	char *str;
	if (!PyArg_ParseTuple(args, "s", &str))
			return NULL;
	sbp_perror(str);
	return Py_BuildValue("");
}
PyObject *wrap_sbp_remove(PyObject *self, PyObject *args)
{
	char *filename;
	int result;
	if (!PyArg_ParseTuple(args, "s", &filename))
			return NULL;
	result = sbp_remove(filename);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_mkdir(PyObject *self, PyObject *args)
{
	char *filename;
	int result;
	if (!PyArg_ParseTuple(args, "s", &filename))
			return NULL;
	result = sbp_mkdir(filename);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_rmdir(PyObject *self, PyObject *args)
{
	char *filename;
	int result;
	if (!PyArg_ParseTuple(args, "s", &filename))
			return NULL;
	result = sbp_rmdir(filename);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_move(PyObject *self, PyObject *args)
{
	char *dst,*src;
	int result;
	if (!PyArg_ParseTuple(args, "ss", &dst, &src))
		return NULL;
	result = sbp_move(dst,src);
	return Py_BuildValue("i", result);
}
PyObject *wrap_sbp_close(PyObject *self, PyObject *args)
{
	int fd;
	if (!PyArg_ParseTuple(args, "i",&fd))
		return NULL;
	sbp_close(fd);
	return Py_BuildValue("");
}
PyObject *wrap_sbp_closedir(PyObject *self, PyObject *args)
{
	int dirfd;
	if (!PyArg_ParseTuple(args, "i",&dirfd))
		return NULL;
	sbp_close(dirfd);
	return Py_BuildValue("");
}
static PyMethodDef cpymodMethods[] =
{
	{"login", wrap_sbp_login, METH_VARARGS, "Login(user,pass)"},
	{"sethost", wrap_sbp_sethost, METH_VARARGS, "sethost(hostname)"},
	{"getUserAndPassword", wrap_sbp_getUandP, METH_VARARGS, "getUandP() return {'username':'xxx','password':'xxx'}"},
	{"chmod", wrap_sbp_chmod, METH_VARARGS, "chmod(filename,mode)"},
	{"chown", wrap_sbp_chown, METH_VARARGS, "chown(filename,newuser)"},
	{"perror", wrap_sbp_perror, METH_VARARGS, "perror() print error detail"},
	{"remove", wrap_sbp_remove, METH_VARARGS, "remove(filename)"},
	{"move", wrap_sbp_move, METH_VARARGS, "move(dst,src)"},
	{"closedir",wrap_sbp_closedir, METH_VARARGS, "closedir(dirfd)"},
	{"close",wrap_sbp_close, METH_VARARGS, "close(fd)"},
	{"mkdir",wrap_sbp_mkdir, METH_VARARGS, "mkdir(dirname)"},
	{"rmdir",wrap_sbp_rmdir, METH_VARARGS, "rmdir(dirname)"},
	{"read",wrap_sbp_read, METH_VARARGS, "read(fd,length) return string"},
	{"readdir",wrap_sbp_readdir, METH_VARARGS, "readdir() return (offset_in_dir(number), file_type(string), filename(string))"},
	{"write",wrap_sbp_write, METH_VARARGS, "write(fd,data,length) on error, return length of sent data"},
	{"open",wrap_sbp_open, METH_VARARGS, "open(filename,oflag,mode) return fd"},
	{"opendir",wrap_sbp_opendir, METH_VARARGS, "opendir(dirname) return fd"},
	{NULL, NULL}
};

void initsbpfs()
{
	PyObject *m;
	m = Py_InitModule("sbpfs", cpymodMethods);
}
