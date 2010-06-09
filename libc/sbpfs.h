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

#ifndef __SBP_LIBC_SBPFS_H_
#define __SBP_LIBC_SBPFS_H_
#include "lib.h"
#include "global.h"
/* APIs */
ssize_t sbp_open(char* filename, size_t oflag, size_t mode);
ssize_t sbp_read(ssize_t fd, void* buf, size_t len);
ssize_t sbp_write(ssize_t fd,void* buf, size_t len);
ssize_t sbp_remove(char* filename);
ssize_t sbp_move(char* dst, char* src);
ssize_t sbp_mkdir(char* dirname);
ssize_t sbp_opendir(char* dirname);
ssize_t sbp_chmod(char* filename, size_t mode);
ssize_t sbp_chown(char* filename, char* newowner, char* group);
ssize_t	sbp_login(char* username, char* password);

/*Default*/


#endif