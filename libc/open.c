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
#include <stdlib.h>
#include <string.h>
struct sbp_filestat filestat;
char* sbp_test(char* buf, u64_t len, char* target ,u32_t port);
s32_t sbp_open(char* filename, u32_t oflag, u16_t mode) {
	s32_t fd = get_slot();
	char tran_oflag[32];
	char tran_mode[16];

	SBP_PREPARE_REQUEST

	sprintf(tran_oflag, "%08d", oflag);
	sprintf(tran_mode, "%04d", mode);

	mkent(head,METHOD,"OPEN");
	mkent(head,ARGC,"3");
	mkent(head,"Arg0",filename);
	mkent(head,"Arg1",tran_oflag);
	mkent(head,"Arg2",tran_mode);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY

	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {
		//printf("reply ok\n");
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
		fds[fd]->oflag = oflag;
		fds[fd]->offset = 0;
		fds[fd]->type = T_FILE;
		char* fd_s = get_head_entry_value(&head, FILE_DESC);
		if(fd_s == NULL)
		{
			goto err_exit4;
		}
		fds[fd]->server_fd = atoll(fd_s);
		char* auth_code = get_head_entry_value(&head, AUTH_CODE);
		if (auth_code == NULL) {
			seterr(DATA_ERR,AUTH_LEN);
			goto err_exit4;
		}
		int auth_len = strlen(auth_code);
		if (auth_len > AUTH_CODE_LEN) {
			seterr(DATA_ERR,AUTH_LEN);
			goto err_exit4;
		}
		memcpy(fds[fd]->auth_code, auth_code, auth_len);

		goto ok_exit;
	} else if (strncmp(head.title, REQUEST_ERR, strlen(REQUEST_ERR)) == 0) {
		sbp_update_err(&head);
		goto err_exit3;
	} else {
		seterr(HEAD_ERR, UNKNOWN_HEAD);
	}

	err_exit4:
	//printf("errexit4\n");
	sbp_close(fd);
	err_exit3:
	//printf("errexit3\n");
	free_head(&head);
	err_exit2:
	//printf("errexit2\n");
	free(rec_data);
	err_exit1:
	//printf("errexit1\n");
	free(data);
	err_exit:
	//printf("errexit\n");
	free(usr);
	free(pass);
	//printf("open return\n");
	return -1;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);
	return fd;

}
struct sbp_filestat* sbp_stat(char* filename)
{
	struct sbpfs_head head;
	char* usr;
	char* pass;
	char* data;
	char* rec_data;
	char tran_usr[TRAN_USERNAME_LEN];
	u64_t rec_len = 0;
	int data_len = 0;
	head.data = NULL;
	head.title = PROTOCOL;
	head.entry_num = 0;
	if (sbp_getUandP(&usr, &pass) == -1) {
		printf("Can not get username and password\n");
		return NULL;
	}
	sprintf(tran_usr, "Client_%s", usr);
	mkent(head,USER,tran_usr);
	mkent(head,PASS,pass);
	mkent(head,METHOD,"STAT");
	mkent(head,ARGC,"1");
	mkent(head,"Arg0",filename);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY

	if (strncmp(head.title, REQUEST_OK, strlen(REQUEST_OK)) == 0) {
		bzero(&filestat,sizeof(struct sbp_filestat));
		u64_t content_l = -1;
		char* value = get_head_entry_value(&head,CONTENT_LEN);
		if(value == NULL)
		{
			seterr(HEAD_ERR,NO_CONTENT_LEN);
			goto err_exit3;
		}
		content_l = atoll(value);
		//printf("stat size :%ld\n",sizeof(struct sbp_filestat));
		if(content_l != sizeof(struct sbp_filestat))
		{
			goto err_exit3;
		}

		memcpy(&filestat,rec_data + head.head_len,content_l);
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
	return NULL;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);
	return &filestat;
}
s32_t sbp_close(s32_t fd) {
	//printf("close enter\n");
	if (fd < 0) return -1;
	if (fds[fd] == NULL)
		return -1;
	char tran_fd[32];
	//printf("close start\n");
	SBP_PREPARE_REQUEST

	sprintf(tran_fd, "%lld", fds[fd]->server_fd);
	mkent(head,METHOD,"CLOSE");
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
	free_sbpfd(fds[fd]);
	fds[fd] = NULL;
	//printf("close err return\n");
	return -1;
	ok_exit: free(data);
	free(rec_data);
	free_head(&head);
	free(usr);
	free(pass);
	//free_sbpfd(fds[fd]);
	//fds[fd] = NULL;
	//printf("close done\n");
	return 0;
}

char* sbp_test(char* buf, u64_t len, char* target, u32_t port) {
	char* rec_buf;
	u64_t rec_len;
	int ret = 0;
	if ((ret = sendrec_hostname(target, port, buf, len, &rec_buf, &rec_len))
			< 0) {
		//printf("TEST Failed : %d\n ", ret);
		sbp_perror("TEST Failed");
		return NULL;
	}
	//printf("Received : %ld\ndata:\n%s", strlen(rec_buf), rec_buf);
	//free(rec_buf);

	return rec_buf;
}
