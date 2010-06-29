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
char* get_dnode_hostname(u32_t dnode);
s32_t sbp_send(s64_t sockfd, char* data, u64_t len);
s32_t sbp_recv(s64_t sockfd, char** rec_buf, u64_t* rec_len);
struct block_entry;
struct sbp_filedesc
{
	u64_t server_fd;
	u64_t offset;
	u32_t oflag;
	u64_t length;
	u8_t  type;

	char* filename;
	char  auth_code[AUTH_CODE_LEN+1];

};
void free_sbpfd(struct sbp_filedesc* fd);
struct sbp_dirent_for_trans{
	u8_t d_type;
	char d_name[MAX_FILENAME_LEN];
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
struct block_entry{
	u64_t block_id;
	u32_t offset_in_block;
	u32_t length_in_block;
	u32_t d_nodes[REDUNDANCY + 1];

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
#define SBP_PREPARE_REQUEST 	\
	struct sbpfs_head head;\
	char* usr;\
	char* pass;\
	char* data;\
	char* rec_data;\
	char tran_usr[TRAN_USERNAME_LEN];\
	u64_t rec_len = 0;\
	int data_len = 0;\
	head.data = NULL;\
	head.title = PROTOCOL;\
	head.entry_num = 0;\
	if (sbp_getUandP(&usr, &pass) == -1) {\
		printf("Can not get username and password\n");\
		return -1;\
	}\
	sprintf(tran_usr, "Client_%s", usr);\
	mkent(head,USER,tran_usr);\
	mkent(head,PASS,pass);

#define SBP_SEND_AND_PROCESS_REPLY	\
	if (make_head(&data, &data_len, &head) == -1) {\
		seterr(HEAD_ERR,MAKE_HEAD);\
		goto err_exit;\
	}/*data need free*/\
	if (sendrec_hostname(sbp_host, CNODE_SERVICE_PORT, data, data_len,&rec_data, &rec_len) != 0) {\
		goto err_exit1;\
	}/*rec_data need free*/\
	if (strlen(rec_data) == 0) {\
		seterr(SOCKET_ERR,RECV);\
		goto err_exit2;\
	}\
	if (decode_head(rec_data, rec_len, &head) == -1) {\
		seterr("Data Error", "Can not decode SBPFS_HEAD");\
		goto err_exit2;\
	}/*head need free*/
#define SBP_PROCESS_RESULT \
	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {\
		goto ok_exit;\
	} else if (strncmp(head.title, REQUEST_ERR, strlen(REQUEST_ERR)) == 0) {\
		sbp_update_err(&head);\
		goto err_exit3;\
	} else {\
		seterr(HEAD_ERR, UNKNOWN_HEAD);\
	}

#define SBP_PROCESS_ERR \
	err_exit3: free_head(&head);\
	err_exit2: free(rec_data);\
	err_exit1: free(data);\
	err_exit: free(usr);\
	free(pass);\
	return -1;\
	ok_exit: free(data);\
	free(rec_data);\
	free_head(&head);\
	free(usr);\
	free(pass);\
	return 0;



#endif
