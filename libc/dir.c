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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
struct sbp_dirent dir_ent;
s32_t sbp_opendir(char* filename) {
	s32_t fd = get_slot();
	SBP_PREPARE_REQUEST

	mkent(head,METHOD,"OPENDIR");
	mkent(head,ARGC,"1");
	mkent(head,"Arg0",filename);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY

	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {
		if ((fds[fd] = (struct sbp_filedesc *) malloc(
				sizeof(struct sbp_filedesc))) == NULL) {
			seterr(MEM_ERR,MALLOC);
			goto err_exit3;
		}// need sbp_close(fd)
		bzero(fds[fd], sizeof(struct sbp_filedesc));
		if ((fds[fd]->filename = (char *) malloc(sizeof(strlen(filename) + 1)))
				== NULL) {
			seterr(MEM_ERR,MALLOC);
			goto err_exit4;
		}
		memcpy(fds[fd]->filename, filename, strlen(filename));
		fds[fd]->filename[strlen(filename)] = 0;
		fds[fd]->offset = 0;
		fds[fd]->type = T_DIR;
		char* fd_s = get_head_entry_value(&head, DIR_FD);
		if(fd_s == NULL)
		{
			seterr(DATA_ERR,"Could not get FD");
			goto err_exit4;
		}
		fds[fd]->server_fd = atoll(fd_s);
//		char* auth_code = get_head_entry_value(&head, AUTH_CODE);
//		if (auth_code == NULL) {
//			seterr(DATA_ERR,AUTH_LEN);
//			goto err_exit4;
//		}
		char* dir_ent_num = get_head_entry_value(&head, DIR_ENT_NUM);
		if (dir_ent_num == NULL)
		{
			seterr(DATA_ERR,DIR_ENT_LEN);
			goto err_exit4;
		}
		fds[fd]->length = atoll(dir_ent_num);
//		int auth_len = strlen(auth_code);
//		if (auth_len > AUTH_CODE_LEN) {
//			seterr(DATA_ERR,AUTH_LEN);
//			goto err_exit4;
//		}
//		memcpy(fds[fd]->auth_code, auth_code, auth_len);

		goto ok_exit;
	} else if (strncmp(head.title, REQUEST_ERR, strlen(REQUEST_ERR)) == 0) {
		sbp_update_err(&head);
		goto err_exit3;
	} else {
		seterr(HEAD_ERR, UNKNOWN_HEAD);
	}

	err_exit4: sbp_closedir(fd);
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
	return fd;

}
s32_t sbp_closedir(s32_t dirfd){
	if(dirfd < 0)return -1;
	if (fds[dirfd] == NULL)
		return -1;
	if (fds[dirfd]->type != T_DIR)
		return -1;
	char tran_fd[32];

	SBP_PREPARE_REQUEST

	sprintf(tran_fd, "%lld", fds[dirfd]->server_fd);
	mkent(head,METHOD,"CLOSEDIR");
	mkent(head,ARGC,"1");
	mkent(head,"Arg0",tran_fd);
	//mkent(head,"Arg1",fds[fd]->auth_code);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY
	SBP_PROCESS_RESULT

	err_exit3: free_head(&head);
	err_exit2: free(rec_data);
	err_exit1: free(data);
	err_exit: free(usr);
	free(pass);
	free_sbpfd(fds[dirfd]);
	fds[dirfd] = NULL;
	return -1;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);
	free_sbpfd(fds[dirfd]);
	fds[dirfd] = NULL;
	return 0;
}
struct sbp_dirent* sbp_readdir(s32_t dirfd) {

	if(dirfd<0)return NULL;
	if (fds[dirfd] == NULL) {
		return NULL;
	}
	struct sbpfs_head head;
	char* usr;
	char* pass;
	char* data;
	char* rec_data;
	char tran_usr[16 + 7];
	u64_t rec_len = 0;
	int data_len = 0;
	head.data = ((void *)0);
	head.title = "SBPFS/1.0";
	head.entry_num = 0;
	if (sbp_getUandP(&usr, &pass) == -1) {
		printf("Can not get username and password\n");
		return NULL;
	}
	sprintf(tran_usr, "Client_%s", usr);
	head.entrys[head.entry_num].name = "User";
	head.entrys[head.entry_num++].value = tran_usr;;
	head.entrys[head.entry_num].name = "Password";
	head.entrys[head.entry_num++].value = pass;;
	char tran_fd[32];
	char tran_offset[32];
	sprintf(tran_fd,"%lld",fds[dirfd]->server_fd);
	sprintf(tran_offset,"%lld",fds[dirfd]->offset);
	mkent(head,METHOD,"READDIR");
	mkent(head,ARGC,"2");
	mkent(head,"Arg0",tran_fd);
	mkent(head,"Arg1",tran_offset);
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
		if(content_l != sizeof(struct sbp_dirent_for_trans))
		{
			goto err_exit3;
		}

		memcpy(&dir_ent,rec_data + head.head_len,content_l);
		dir_ent.offset = fds[dirfd]->offset++;
		goto ok_exit;
	} else if (strncmp(head.title, REQUEST_ERR, strlen(REQUEST_ERR)) == 0) {
		sbp_update_err(&head);
		goto err_exit3;
	} else {
		seterr(HEAD_ERR,UNKNOWN_HEAD);
	}
	err_exit3: free_head(&head);
	err_exit2: free(rec_data);
	err_exit1: free(data);
	err_exit: free(usr);
	free(pass);
	return NULL;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);
	return &dir_ent;
}

s32_t sbp_mkdir(char* filename) {

	SBP_PREPARE_REQUEST

	mkent(head,METHOD,"MKDIR");
	mkent(head,ARGC,"1");
	mkent(head,"Arg0",filename);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY
	SBP_PROCESS_RESULT
	SBP_PROCESS_ERR
}
s32_t sbp_rmdir(char* filename){
	SBP_PREPARE_REQUEST

	mkent(head,METHOD,"RMDIR");
	mkent(head,ARGC,"1");
	mkent(head,"Arg0",filename);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY
	SBP_PROCESS_RESULT
	SBP_PROCESS_ERR
}
