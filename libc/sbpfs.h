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
s64_t sbp_open(char* filename, u64_t oflag, u64_t mode);
s64_t sbp_read(s64_t fd, void* buf, u64_t len);
s64_t sbp_write(s64_t fd,void* buf, u64_t len);
s64_t sbp_remove(char* filename);
s64_t sbp_move(char* dst, char* src);
s64_t sbp_mkdir(char* dirname);
s64_t sbp_opendir(char* dirname);
s64_t sbp_chmod(char* filename, u64_t mode);
s64_t sbp_chown(char* filename, char* newowner, char* group);
s64_t sbp_login(char* username, char* password);
s64_t sbp_test(char* buf, u64_t len, char* target ,u64_t port);
/*Default*/


#endif
