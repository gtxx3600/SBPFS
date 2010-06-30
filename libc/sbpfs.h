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

#define SBP_S_ORD		0x02
#define SBP_S_OWR		0x01
#define SBP_S_IRD 		0x08
#define SBP_S_IWR		0x04

/* OFLAGS */
#define SBP_O_RDONLY	0x01
#define SBP_O_WRONLY	0x02
#define SBP_O_RDWR		0x04
#define SBP_O_CREAT		0x08
#define SBP_O_APPEND	0x10
/* TYPE */
#define T_DIR		0x01
#define T_FILE		0x02

struct sbp_filestat;

/* APIs */
/* open.c */
s32_t sbp_open(char* filename, u32_t oflag, u16_t mode);
s32_t sbp_close(s32_t fd);
/* dir.c */
s32_t sbp_opendir(char* dirname);
s32_t sbp_mkdir(char* dirname);
s32_t sbp_rmdir(char* dirname);
struct sbp_dirent* sbp_readdir(s32_t dirfd);
struct sbp_filestat* sbp_stat(char* filename);
s32_t sbp_closedir(s32_t dirfd);
/*read_write.c*/
s64_t sbp_read(s64_t fd, void* buf, u64_t len);
s64_t sbp_write(s64_t fd,void* buf, u64_t len);
s64_t sbp_seek(s64_t fd,s64_t offset);
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
s32_t sbp_getusername(char** username);
s32_t sbp_getUandP(char** username,char** password);

/*TOOLS*/
void sbp_perror(char* s);
/* structures */
struct sbp_dirent{
	u8_t d_type;								/* dir entry type file or directory */
	char d_name[MAX_FILENAME_LEN];				/* dir entry name */
	u64_t offset;								/* offset in parent directory */
};
struct sbp_filestat {
	u64_t   size;   	 						/* total size, in bytes */
	u64_t 	atime;   							/* time of last access */
	u64_t   mtime;   							/* time of last modification */
	u64_t   ctime;   							/* time of last status change */
	u32_t	owner;								/* user id */
	u8_t    mode;    							/* file type & protection */
	u8_t 	preserved[27];
};

#endif
