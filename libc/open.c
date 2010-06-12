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
s32_t sbp_open(char* filename, u32_t oflag, u16_t mode) {
	s32_t fd = get_slot();
	char tran_oflag[32];
	char tran_mode[16];

	SBP_PREPARE_REQUEST

	sprintf(tran_oflag, "%08x", oflag);
	sprintf(tran_mode, "%04x", mode);

	mkent(head,METHOD,"OPEN");
	mkent(head,ARGC,"3");
	mkent(head,"Arg0",filename);
	mkent(head,"Arg1",tran_oflag);
	mkent(head,"Arg2",tran_mode);
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
		fds[fd]->oflag = oflag;
		fds[fd]->offset = 0;
		fds[fd]->type = T_FILE;
		fds[fd]->server_fd = atoll(get_head_entry_value(&head, FILE_DESC));
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

	err_exit4: sbp_close(fd);
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
s32_t sbp_close(u32_t fd) {
	if (fds[fd] == NULL)
		return -1;
	char tran_fd[32];

	SBP_PREPARE_REQUEST

	sprintf(tran_fd, "%lld", fds[fd]->server_fd);
	mkent(head,METHOD,"CLOSE");
	mkent(head,ARGC,"2");
	mkent(head,"Arg0",tran_fd);
	mkent(head,"Arg1",fds[fd]->auth_code);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY
	SBP_PROCESS_RESULT

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
	free_sbpfd(fds[fd]);
	fds[fd] = NULL;
	return 0;
}

s32_t sbp_test(char* buf, u64_t len, char* target, u32_t port) {
	char* rec_buf;
	u64_t rec_len;
	int ret = 0;
	if ((ret = sendrec_hostname(target, port, buf, len, &rec_buf, &rec_len))
			< 0) {
		//printf("TEST Failed : %d\n ", ret);
		perror("TEST Failed");
		return ret;
	}
	printf("Received : %ld\n%s", strlen(rec_buf), rec_buf);
	return 0;
}
