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

#include "sbpfs.h"
#include <stdlib.h>

char sbp_user[MAX_USERNAME_LEN + 1] = "anonymous";
char sbp_pass[MAX_PASSWORD_LEN + 1] = "";
char sbp_host[MAX_HOSTNAME_LEN + 1] = "127.0.0.1";

struct sbp_filedesc* fds[MAX_FILE_OPEN] = {};

char sbp_PWD[MAX_PATH_LEN] = "";

struct sbp_err *err_trace = NULL;


