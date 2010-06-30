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
#include "lib.h"
#include <stdio.h>
#include <string.h>
static s64_t read_blocks(struct block_entry* ents, u64_t block_count,
		char* buf, char* auth_code);
static s64_t write_blocks(struct block_entry* ents, u64_t block_count,
		char* buf, char* auth_code);
static s64_t read_block(struct block_entry* ents, int offset_in_ent_dnode,
		char* buf, char* auth_code);
static s64_t write_block(struct block_entry* ents, int offset_in_ent_dnode,
		char* buf, char* auth_code);
static s64_t sbp_read_or_write(s64_t fd, void* buf, u64_t len, char* method);

s64_t sbp_read(s64_t fd, void* buf, u64_t len) {
	if (fd < 0)
		return -1;
	if (fds[fd]->type == T_FILE)
		return sbp_read_or_write(fd, buf, len, "READ");
	else {
		seterr(REQ_ERR,FILE_TYPE);
		return -1;
	}
}

s64_t sbp_write(s64_t fd, void* buf, u64_t len) {
	if (fd < 0) {
		//printf("fd < 0 : fd:%lld\n", fd);
		return -1;
	}
	if (fds[fd] == NULL) {
		//printf("fds[fd] fd : %lld is NULL\n", fd);
		return -1;
	}
	if (fds[fd]->type == T_FILE)
		return sbp_read_or_write(fd, buf, len, "WRITE");
	else {
		seterr(REQ_ERR,FILE_TYPE);
		return -1;
	}
}

static s64_t sbp_read_or_write(s64_t fd, void* buf, u64_t len, char* method) {
	if (fds[fd] == NULL)
		return -1;
	char tran_fd[32];
	char tran_offset[32];
	char tran_len[32];
	s64_t ret = -1;
	SBP_PREPARE_REQUEST
	sprintf(tran_fd, "%lld", fds[fd]->server_fd);
	sprintf(tran_offset, "%lld", fds[fd]->offset);
	sprintf(tran_len, "%lld", len);
	mkent(head,METHOD,method);
	mkent(head,ARGC,"3");
	mkent(head,"Arg0",tran_fd);
	mkent(head,"Arg1",tran_offset);
	mkent(head,"Arg2",tran_len);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY

	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {
		u64_t content_l = -1;
		char* value = get_head_entry_value(&head, CONTENT_LEN);
		if (value == NULL) {
			seterr(HEAD_ERR,NO_CONTENT_LEN);
			goto err_exit3;
		}
		content_l = atoll(value);
		if (content_l % sizeof(struct block_entry)) {
			printf("content l :%lld :%ld\n", content_l,
					sizeof(struct block_entry));
			seterr(DATA_ERR,DATA_LEN);
			goto err_exit3;
		}
		u64_t block_ent_num = content_l / sizeof(struct block_entry);
		struct block_entry* ents = (struct block_entry*) (rec_data
				+ head.head_len);
		if (strcmp(method, "WRITE") == 0) {
			ret = write_blocks(ents, block_ent_num, buf, fds[fd]->auth_code);
		} else if (strcmp(method, "READ") == 0) {
			ret = read_blocks(ents, block_ent_num, buf, fds[fd]->auth_code);
		}
		if (ret >= 0){

			fds[fd]->offset += ret;
			//printf("fds offset :%lld\n",fds[fd]->offset);
		}
		goto ok_exit;
	} else if (strncmp(head.title, REQUEST_ERR, strlen(REQUEST_ERR)) == 0) {
		sbp_update_err(&head);
		goto err_exit3;
	} else {
		seterr(HEAD_ERR, UNKNOWN_HEAD);
	}

	err_exit3: free_head(&head);
	err_exit2: free(rec_data);
	err_exit1: free(data);
	err_exit: free(usr);
	free(pass);

	return ret;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);

	return ret;

}

static s64_t read_block(struct block_entry* ents, int offset_in_ent_dnode,
		char* buf, char* auth_code) {
	s64_t ret = 0;

	struct sbpfs_head head;
	char* usr;
	char* pass;
	char* data;
	char* rec_data;
	char tran_usr[TRAN_USERNAME_LEN];

	char tran_blocknum[32];
	char tran_offset[32];
	char tran_len[32];
	if (sbp_getUandP(&usr, &pass) == -1) {
		printf("Can not get username and password\n");
		return -1;
	}
	sprintf(tran_usr, "Client_%s", usr);

	u64_t rec_len = 0;
	int data_len = 0;
	head.data = NULL;
	head.title = PROTOCOL;
	head.entry_num = 0;
	mkent(head,USER,tran_usr);
	mkent(head,PASS,pass);
	mkent(head,AUTH_CODE,auth_code);
	mkent(head,METHOD,"READ");
	mkent(head,ARGC,"3");
	bzero(tran_blocknum, 32);
	bzero(tran_offset, 32);
	bzero(tran_len, 32);
	sprintf(tran_blocknum, "%lld", ents->block_id);
	sprintf(tran_offset, "%d", ents->offset_in_block);
	sprintf(tran_len, "%d", ents->length_in_block);
	mkent(head,"Arg0",tran_blocknum);
	mkent(head,"Arg1",tran_offset);
	mkent(head,"Arg2",tran_len);
	mkent(head,CONTENT_LEN,"0");

	if (make_head(&data, &data_len, &head) == -1) {
		seterr(HEAD_ERR,MAKE_HEAD);
		goto err_exit;
	}/*data need free*/
	if (sendrec_hostname(
			get_dnode_hostname(ents->d_nodes[offset_in_ent_dnode]),
			DNODE_SERVICE_PORT, data, data_len, &rec_data, &rec_len) != 0) {
		goto err_exit1;
	}/*rec_data need free*/
	if (strlen(rec_data) == 0) {
		seterr(SOCKET_ERR,RECV);
		goto err_exit2;
	}
	if (decode_head(rec_data, rec_len, &head) == -1) {
		seterr("Data Error", "Can not decode SBPFS_HEAD");
		goto err_exit2;
	}/*head need free*/
	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {
		u64_t content_l = -1;
		char* value = get_head_entry_value(&head, CONTENT_LEN);
		if (value == NULL) {
			seterr(HEAD_ERR,NO_CONTENT_LEN);
			goto err_exit3;
		}
		content_l = atoll(value);
		if (content_l != ents->length_in_block) {
			seterr(DATA_ERR,DATA_LEN);
			goto err_exit3;
		}
		memcpy(buf, rec_data + head.head_len, content_l);
		ret = content_l;

	} else if (strncmp(head.title, REQUEST_ERR, strlen(REQUEST_ERR)) == 0) {
		sbp_update_err(&head);
		goto err_exit3;
	} else {
		seterr(HEAD_ERR, UNKNOWN_HEAD);
	}

	goto ok_exit;

	err_exit3:

	free_head(&head);
	err_exit2:

	free(rec_data);
	err_exit1:

	free(data);
	err_exit:

	free(usr);
	free(pass);

	return -1;
	ok_exit: free(usr);
	free(pass);
	free(data);
	free(rec_data);
	free_head(&head);
	return ret;
}
static s64_t read_blocks(struct block_entry* ents, u64_t block_count,
		char* buf, char* auth_code) {
	int i = 0;
	s64_t buf_ptr = 0;
	s64_t rec_len = 0;
	for (; i < block_count; i++) {
		int j = 0;
		int k = 0;
		for (; j <= REDUNDANCY; j++) {
			if (ents[i].d_nodes[j]) {
				rec_len = read_block(&ents[i], j, buf + buf_ptr, auth_code);
				if (rec_len >= ents[i].length_in_block) {
					k = 1;
					break;
				}
			}
		}
		if (k == 1) {
			buf_ptr += ents[i].length_in_block;
		} else {
			return -1;
		}
	}
	return buf_ptr;
}
static s64_t write_block(struct block_entry* ents, int offset_in_ent_dnode,
		char* buf, char* auth_code) {
	s64_t ret = 0;

	struct sbpfs_head head;
	char* usr;
	char* pass;
	char* data;
	char* rec_data;
	char tran_usr[TRAN_USERNAME_LEN];

	char tran_blocknum[32];
	char tran_offset[32];
	char tran_len[32];
	if (sbp_getUandP(&usr, &pass) == -1) {
		printf("Can not get username and password\n");
		return -1;
	}
	sprintf(tran_usr, "Client_%s", usr);

	s64_t sockfd;

	u64_t rec_len = 0;
	int data_len = 0;
	head.data = NULL;
	head.title = PROTOCOL;
	head.entry_num = 0;
	mkent(head,USER,tran_usr);
	mkent(head,PASS,pass);
	mkent(head,AUTH_CODE,auth_code);
	mkent(head,METHOD,"WRITE");
	mkent(head,ARGC,"3");
	bzero(tran_blocknum, 32);
	bzero(tran_offset, 32);
	bzero(tran_len, 32);
	sprintf(tran_blocknum, "%lld", ents->block_id);
	sprintf(tran_offset, "%d", ents->offset_in_block);
	sprintf(tran_len, "%d", ents->length_in_block);
	mkent(head,"Arg0",tran_blocknum);
	mkent(head,"Arg1",tran_offset);
	mkent(head,"Arg2",tran_len);
	mkent(head,CONTENT_LEN,tran_len);

	if (make_head(&data, &data_len, &head) == -1) {
		seterr(HEAD_ERR,MAKE_HEAD);
		goto err_exit;
	}/*data need free*/

	if ((sockfd = sbp_connect(get_dnode_hostname(
			ents->d_nodes[offset_in_ent_dnode]), DNODE_SERVICE_PORT)) < 0) {
		printf("connect ip: %s err\n", get_dnode_hostname(
				ents->d_nodes[offset_in_ent_dnode]));
		seterr(SOCKET_ERR, EST_SOCK);
		goto err_exit1;
	}

	if (sbp_send(sockfd, data, data_len) < 0) {
		printf("send head to ip: %s err\n", get_dnode_hostname(
				ents->d_nodes[offset_in_ent_dnode]));
		seterr(SOCKET_ERR, SEND);
		goto err_exit1;
	}
	if (sbp_send(sockfd, buf, ents->length_in_block) < 0) {
		printf("send data to ip: %s err\n", get_dnode_hostname(
				ents->d_nodes[offset_in_ent_dnode]));
		seterr(SOCKET_ERR, SEND);
		goto err_exit1;
	}
	if ((sbp_recv(sockfd, &rec_data, &rec_len)) < 0) {
		printf("receive from ip: %s err\n", get_dnode_hostname(
				ents->d_nodes[offset_in_ent_dnode]));
		seterr(SOCKET_ERR, RECV);
		goto err_exit1;
	}

	if (strlen(rec_data) == 0) {
		seterr(SOCKET_ERR,RECV);
		goto err_exit2;
	}
	if (decode_head(rec_data, rec_len, &head) == -1) {
		seterr("Data Error", "Can not decode SBPFS_HEAD");
		goto err_exit2;
	}/*head need free*/
	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {
		ret = ents->length_in_block;

		free(data);
		free(rec_data);
		free_head(&head);

//		printf("succeeded write to %s block: %lld length:%d \n",
//				get_dnode_hostname(ents->d_nodes[offset_in_ent_dnode]),
//				ents->block_id, ents->length_in_block);

	} else if (strncmp(head.title, REQUEST_ERR, strlen(REQUEST_ERR)) == 0) {
		sbp_update_err(&head);
		goto err_exit3;
	} else {
		seterr(HEAD_ERR, UNKNOWN_HEAD);
	}

	goto ok_exit;

	err_exit3:

	free_head(&head);
	err_exit2:

	free(rec_data);
	err_exit1:

	free(data);
	err_exit:

	free(usr);
	free(pass);

	return -1;
	ok_exit: free(usr);
	free(pass);
	return ret;
}
static s64_t write_blocks(struct block_entry* ents, u64_t block_count,
		char* buf, char* auth_code) {
	int i = 0;
	s64_t buf_ptr = 0;
	s64_t rec_len = 0;
	for (; i < block_count; i++) {
		int j = 0;
		int k = 0;
		int l = 0;
		for (; j <= REDUNDANCY; j++) {
			//printf("write redundancy %d\n",j);
			if (ents[i].d_nodes[j]) {
				l++;
				//printf("write block %lld offset: %d length: %d\n",ents[i].block_id,ents[i].offset_in_block,ents[i].length_in_block);
				rec_len = write_block(&ents[i], j, buf + buf_ptr, auth_code);
				if (rec_len >= ents[i].length_in_block) {
					k ++;
				}else
				{
				//	printf("write to %s length = %lld\n not eq to lengthin block: %d\n",get_dnode_hostname(ents[i].d_nodes[j]),rec_len,ents[i].length_in_block);
				}
			}else
			{
				//printf("redundancy :%d is empty\n",j);
			}
		}
		if (k == l) {
			buf_ptr += ents[i].length_in_block;
		} else {
			return -1;
		}
	}
	//printf("write blocks return :%lld\n",buf_ptr);
	return buf_ptr;
}
s64_t sbp_seek(s64_t fd, s64_t offset) {
	if (fd < 0) {
		printf("fd < 0 : fd:%lld\n", fd);
		return -1;
	}
	if (fds[fd] == NULL) {
		printf("fds[fd] fd : %lld is NULL\n", fd);
		return -1;
	}
	if (((s64_t) (fds[fd]->offset + offset)) < 0) {
		return -1;
	}
	fds[fd]->offset += offset;
	return 0;
}
