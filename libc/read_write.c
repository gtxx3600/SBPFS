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
static s64_t read_blocks(struct block_entry* ents, u64_t block_count, void* buf);
s64_t sbp_read(s64_t fd, void* buf, u64_t len){
	if (fds[fd] == NULL)
		return -1;
	char tran_fd[32];
	char tran_offset[32];
	char tran_len[32];
	s64_t ret = -1;
	SBP_PREPARE_REQUEST
	sprintf(tran_fd, "%lld", fds[fd]->server_fd);
	sprintf(tran_offset, "%lld",fds[fd]->offset);
	sprintf(tran_len,"%lld", len);
	mkent(head,METHOD,"READ");
	mkent(head,ARGC,"3");
	mkent(head,"Arg0",tran_fd);
	mkent(head,"Arg1",tran_offset);
	mkent(head,"Arg2",tran_len);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY

	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {
		u64_t content_l = -1;
		char* value = get_head_entry_value(&head,CONTENT_LEN);
		if(value == NULL)
		{
			seterr(HEAD_ERR,NO_CONTENT_LEN);
			goto err_exit3;
		}
		content_l = atoll(value);
		if(content_l%sizeof(struct block_entry))
		{
			seterr(DATA_ERR,DATA_LEN);
			goto err_exit3;
		}
		u64_t block_ent_num = content_l / sizeof(struct block_entry);
		struct block_entry* ents = (struct block_entry*)head.data;
		ret = read_blocks(ents,block_ent_num,buf);

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
	free_sbpfd(fds[fd]);
	fds[fd] = NULL;
	return ret;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);
	free_sbpfd(fds[fd]);
	fds[fd] = NULL;
	return ret;

}

s64_t sbp_write(s64_t fd,void* buf, u64_t len){
	return 0;
}
static s64_t read_blocks(struct block_entry* ents, u64_t block_count, void* buf)
{
	s64_t ret = -1;
	s64_t buf_ptr;
	SBP_PREPARE_REQUEST
	if (make_head(&data, &data_len, &head) == -1) {
		seterr(HEAD_ERR,MAKE_HEAD);
		goto err_exit;
	}/*data need free*/
	if (sendrec_hostname(sbp_host, DNODE_SERVICE_PORT, data, data_len,&rec_data, &rec_len) != 0) {
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
	return -1;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);
return ret;
}
