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


#include "lib.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

s64_t get_missing_len(char* buf);

s32_t sendrec_ip(char* host_name, u64_t port, char* data, u64_t len,
		char** rec_buf, u64_t * rec_len) {
	s64_t sockfd, numbytes, missing_bytes;
	char buf[BUF_SIZE + 1];

	struct hostent *target;
	struct sockaddr_in target_addr;

	buf[BUF_SIZE] = 0;
	target = gethostbyname(host_name);
	target_addr.sin_family = AF_INET;
	//	target_addr.sin_addr.s_addr = htol(INADDR_ANY);
	target_addr.sin_addr.s_addr = ((struct in_addr *) (target->h_addr))->s_addr;
	target_addr.sin_port = htons(port);
	bzero(&(target_addr.sin_zero), 8);
	//printf("target_addr:%d\n",target_addr.sin_addr.s_addr);
	//printf("target_port:%d\n",target_addr.sin_port);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket Error, %d\n", errno);
		goto error_exit;
	}
	if (connect(sockfd, (struct sockaddr *) &target_addr,
			sizeof(struct sockaddr))) {
		printf("Connect Error, %d\n", errno);
		goto error_exit;
	}

	numbytes = send(sockfd, data, len, 0);
	if (numbytes != len) {
		printf("Socket Send Length Error!\n");
		goto error_exit;
	}

	numbytes = recv(sockfd, buf, BUF_SIZE, 0);
	if (numbytes < BUF_SIZE) {
		//printf("numbytes < BUF_SIZE\n");
		if ((*rec_buf = (char*) malloc(numbytes + 1)) == NULL) {
			printf("Socket Send Malloc Error!\n");
			goto error_exit;
		}
		if (memcpy(*rec_buf, buf, numbytes) != *rec_buf) {
			printf("Socket Send Memcpy Error!\n");
			goto error_exit2;
		}

		(*rec_buf)[numbytes] = 0;
		*rec_len = numbytes + 1;
		goto ok_exit;

	} else {
		//printf("numbytes >= BUF_SIZE\n");
		if ((missing_bytes = get_missing_len(buf)) < 0) {
			printf("Socket Send get_missing_len Error! missing_len:%lld\n",
					missing_bytes);
			goto error_exit;
		}
		if ((*rec_buf = (char*) malloc(missing_bytes + numbytes + 1)) == NULL) {
			printf("Socket Send Malloc Error!\n");
			goto error_exit;
		}
		if (memcpy(*rec_buf, buf, numbytes) != *rec_buf) {
			printf("Socket Send Memcpy Error!\n");
			goto error_exit2;
		}
		if ((numbytes = recv(sockfd, &(*rec_buf)[numbytes], missing_bytes, 0))
				!= missing_bytes) {
			printf(
					"Socket Send last_recv Error! Received bytes: %lld ; missing_bytes : %lld\n",
					numbytes, missing_bytes);
			goto error_exit2;
		}
		(*rec_buf)[missing_bytes + BUF_SIZE] = 0;
		*rec_len = missing_bytes + BUF_SIZE + 1;
		goto ok_exit;

	}
	error_exit2:free(*rec_buf);
	error_exit: close(sockfd);
	return -1;

	ok_exit: close(sockfd);
	return 0;
}
s64_t get_missing_len(char* buf) {
	u64_t content_length = 0, header_length = 0;
	char content_len[64];
	char* content_len_start = NULL;
	char* content_len_end = NULL;
	char* header_end;
	if ((header_end = strstr(buf, HEADER_FLAG)) == NULL) {
		printf("Socket Send find header_flag Error!\n");
		goto error_exit;
	}

	header_length = header_end - buf + strlen(HEADER_FLAG);
	//printf("header_start: %p ;header_end: %p ;header_len %lld ; strlen(header_flag) : %ld\n",buf,header_end,header_length,strlen(HEADER_FLAG));
	if ((content_len_start = strstr(buf, CONTENT_LEN)) == NULL) {
		printf("Socket get_missing_len Strstr Error!\n");
		goto error_exit;
	}
	if ((content_len_end = strstr(content_len_start, "\r\n")) == NULL) {
		printf("Socket get_missing_len Strstr2 Error!\n");
		goto error_exit;
	}
	content_len_start += strlen(CONTENT_LEN);
	if (memcpy(content_len, content_len_start, content_len_end
			- content_len_start) != content_len) {
		printf("Socket Send Memcpy2 Error!\n");
		goto error_exit;
	}
	content_len[content_len_end - content_len_start] = 0;
	content_length = atoi(content_len);
	//printf("missing length : %lld\n",(content_length + header_length - BUF_SIZE));
	return content_length + header_length - BUF_SIZE;

	error_exit: return -1;
}

s32_t decode_head(char* data, u64_t len, struct sbpfs_head* head) {
	char* header_end;
	char* prev_ptr, *next_ptr;
	//char* next_ptr;
	u64_t header_len;
	int i = 0;
	if ((header_end = strstr(data, HEADER_FLAG)) == NULL) {
		printf("decode_head find header_flag Error!\n");
		goto error_exit;
	}
	header_len = header_end - data + strlen(HEADER_FLAG);
	if ((head->data = malloc(header_len + 1)) == NULL) {
		perror("decode_head malloc error");
		goto error_exit;
	}
	head->data[header_len] = 0;
	memcpy(head->data, data, header_len);
	head->title = head->data;
	if ((prev_ptr = strstr(head->data, "\r\n")) == NULL) {
		printf("decode_head find \\r\\n Error!\n");
		goto error_exit2;
	}
	*prev_ptr = 0;
	prev_ptr += 2;
	while (1) {
		if ((next_ptr = strstr(prev_ptr, "\r\n")) == NULL) {
			printf("decode_head find '\\r\\n' Error!\n");
			goto error_exit2;
		}
		if (next_ptr == prev_ptr) {
			break;
		}
		head->entrys[i].name = prev_ptr;

		*next_ptr = 0;
		next_ptr += 2;
		if ((prev_ptr = strstr(prev_ptr, ": ")) == NULL) {
			printf("decode_head find ': ' Error!\n");
			goto error_exit2;
		}
		*prev_ptr = 0;
		prev_ptr += 2;
		head->entrys[i].value = prev_ptr;
		head->entry_num += 1;
		prev_ptr = next_ptr;
	}

	return 0;
	error_exit2: free(head->data);
	error_exit: return -1;
}
s32_t make_head(char** data, int* len, struct sbpfs_head* head) {
	int i = 0;
	u64_t total_len = 0;
	char* cp_ptr;
	if (head->entry_num > MAX_ENTRY_IN_HEAD){
		printf("entry_num too much\n");
		return -1;
	}
	total_len += strlen(head->title) + 2;
	while(i < head->entry_num)
	{
		total_len += strlen(head->entrys[i].name) + strlen(head->entrys[i].value) + 4;
		i++;
	}
	total_len += 2;
	*len = total_len;
	if((*data = malloc(total_len)) == NULL)
	{
		perror("make_head malloc error");
		return -1;
	}
	cp_ptr = *data;
	strcpy(cp_ptr,head->title);
	cp_ptr += strlen(head->title);
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	i = 0;
	while(i < head->entry_num)
	{
		strcpy(cp_ptr, head->entrys[i].name);
		cp_ptr += strlen(head->entrys[i].name);
		*cp_ptr++ = ':';
		*cp_ptr++ = ' ';
		strcpy(cp_ptr, head->entrys[i].value);
		cp_ptr += strlen(head->entrys[i].value);
		*cp_ptr++ = '\r';
		*cp_ptr++ = '\n';
		i++;
	}
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';

	return 0;
}
void free_head(struct sbpfs_head* head){
	if(head->data != NULL)
	{
		free(head->data);
	}
	bzero(head, sizeof(struct sbpfs_head));
}
void init_head(struct sbpfs_head* head){
	bzero(head, sizeof(struct sbpfs_head));
}
void test() {
	printf("haha");
}

