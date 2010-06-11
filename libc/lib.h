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
struct sbpfs_head;


s32_t sendrec_ip(char* host_name, u64_t port,char* data, u64_t len, char** rec_buf, u64_t * rec_len);
s32_t decode_head(char* data, u64_t len, struct sbpfs_head* head);
s32_t make_head(char** data, int* len, struct sbpfs_head* head);
void  free_head(struct sbpfs_head* head);
void  init_head(struct sbpfs_head* head);
struct file_desc
{
	u16_t mode;
	u64_t offset;
	u64_t total_length;
	char* filename;
	char  identify[IDENTIFY_CODE_LEN];
	char  owner[MAX_USERNAME_LEN+1];


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
	char* title;
	int entry_num;
	struct head_entry entrys[MAX_ENTRY_IN_HEAD];
	char* data;
};

#endif
