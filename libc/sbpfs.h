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


#include "global.h"
#include "error.h"
/* MODE */

#define S_ORD		0x02
#define S_OWR		0x01
#define S_IRD 		0x08
#define S_IWR		0x04

/* OFLAGS */

#define O_RDONLY	0x01
#define O_WRONLY	0x02
#define O_RDWR		0x04
#define O_CREAT		0x08
#define O_APPEND	0x10

/* TYPE */
#define T_DIR		0x01
#define T_FILE		0x02
/* APIs */

/* open.c */
s32_t sbp_open(char* filename, u32_t oflag, u16_t mode);
s32_t sbp_close(u32_t fd);
/* dir.c */
s32_t sbp_opendir(char* dirname);//TODO
s32_t sbp_mkdir(char* dirname);//TODO
s32_t sbp_readdir(u32_t dirfd);//TODO
s32_t sbp_closedir(u32_t dirfd);//TODO

/*read_write.c*/
s64_t sbp_read(s64_t fd, void* buf, u64_t len);//TODO
s64_t sbp_write(s64_t fd,void* buf, u64_t len);//TODO

/*move.c*/
s32_t sbp_move(char* dst, char* src);

/*remove.c*/
s32_t sbp_remove(char* filename);

/*chmod.c*/
s32_t sbp_chmod(char* filename, u16_t mode);

/*chown.c*/
s32_t sbp_chown(char* filename, char* newowner);

/*login.c*/
s32_t sbp_login(char* username, char* password);
s32_t sbp_sethost(char* hostname);
s32_t sbp_test(char* buf, u64_t len, char* target ,u32_t port);


/*TOOLS*/
void sbp_perror(char* s);

/*login.c*/
s32_t sbp_getusername(char** username);
s32_t sbp_getUandP(char** username,char** password);

#endif
