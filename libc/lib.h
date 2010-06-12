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

#ifndef __SBP_LIBC_LIB_H_
#define __SBP_LIBC_LIB_H_
#include "const.h"
#include <sys/types.h>
#include <stdlib.h>
struct sbpfs_head;


s32_t sendrec_hostname(char* host_name, u64_t port,char* data, u64_t len, char** rec_buf, u64_t * rec_len);
s64_t sbp_connect(char* host_name, u64_t port);
s32_t decode_head(char* data, u64_t len, struct sbpfs_head* head);
s32_t make_head(char** data, int* len, struct sbpfs_head* head);
void  free_head(struct sbpfs_head* head);
void  init_head(struct sbpfs_head* head);
s32_t get_slot();
char* get_head_entry_value(struct sbpfs_head* head,char* entry);
struct sbp_filedesc
{
	u64_t server_fd;
	u32_t oflag;
	u64_t offset;
	char* filename;
	char  auth_code[AUTH_CODE_LEN+1];

};
void free_sbpfd(struct sbp_filedesc* fd);
struct sbp_dirent{
	u64_t fd;
	u64_t d_off;
	u8_t d_type;
	char d_name[MAX_FILENAME_LEN];
};
struct sbp_stat {
	u64_t	fd;    			/* FD number */
	u64_t   nlink;   		/* number of hard links */
	u64_t   size;   	 	/* total size, in bytes */
	u64_t 	atime;   		/* time of last access */
	u64_t   mtime;   		/* time of last modification */
	u64_t   ctime;   		/* time of last status change */
	u8_t    mode;    		/* file type & protection */
	u8_t	owner[MAX_USERNAME_LEN];     		/* user ID of owner */
	u8_t 	preserved[15];
};

/* Size of click is 16MB*/
struct click_data{
	u64_t id;
	u32_t offset;
	u32_t len;
};
struct head_entry{
	char* name;
	char* value;
};
#define mkent(head,n,val)  head.entrys[head.entry_num].name = n;\
	head.entrys[head.entry_num++].value = val;
#define mkusr(usr)	"Client_"##usr
struct sbpfs_head{
	u64_t head_len;
	char* title;
	int entry_num;
	struct head_entry entrys[MAX_ENTRY_IN_HEAD];
	char* data;
};


#endif
