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

#ifndef __SBP_LIBC_GLOBAL_H_
#define __SBP_LIBC_GLOBAL_H_
#include "const.h"

extern char sbp_user[MAX_USERNAME_LEN + 1];
extern char sbp_pass[MAX_PASSWORD_LEN + 1];
extern char sbp_host[MAX_HOSTNAME_LEN + 1];

extern struct sbp_err * err_trace;


extern struct 	sbp_filedesc* fds[MAX_FILE_OPEN];
extern char 	sbp_PWD[MAX_PATH_LEN];
extern char		sbp_ROOT[MAX_PATH_LEN];

#endif /* GLOBAL_H_ */
