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
#include "error.h"
#include "global.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
char dnode_hostname[32];
s64_t get_missing_len(char* buf);
char* get_dnode_hostname(u32_t dnode)
{
	char i1,i2,i3,i4;
	i1 = (char)(dnode>>24);
	i2 = (char)((dnode>>16)&(255));
	i3 = (char)((dnode>>8)&(255));
	i4 = (char)dnode;
	sprintf(dnode_hostname,"%d.%d.%d.%d",i1,i2,i3,i4);
	return dnode_hostname;
}
s32_t sbp_send(s64_t sockfd, char* data, u64_t len)
{
	u64_t numbytes;
	numbytes = send(sockfd, data, len, 0);
	if (numbytes != len) {
		seterr(SOCKET_ERR, DATA_LEN);
		return -1;
	}
	return 0;
}
s32_t sbp_recv(s64_t sockfd, char** rec_buf, u64_t* rec_len)
{
	u64_t missing_bytes,numbytes;
	char buf[BUF_SIZE + 1];
	buf[BUF_SIZE] = 0;
	numbytes = recv(sockfd, buf, BUF_SIZE, 0);
	if (numbytes < BUF_SIZE) {
		//printf("numbytes < BUF_SIZE\n");
		if ((*rec_buf = (char*) malloc(numbytes + 1)) == NULL) {
			seterr(MEM_ERR, MALLOC);
			goto error_exit;
		}
		if (memcpy(*rec_buf, buf, numbytes) != *rec_buf) {
			seterr(MEM_ERR, MEMCPY);
			goto error_exit2;
		}

		(*rec_buf)[numbytes] = 0;
		*rec_len = numbytes + 1;
		goto ok_exit;

	} else {
		//printf("numbytes >= BUF_SIZE\n");
		if ((missing_bytes = get_missing_len(buf)) < 0) {
			seterr(SOCKET_ERR,MISSING_LEN);
			goto error_exit;
		}
		if ((*rec_buf = (char*) malloc(missing_bytes + numbytes + 1)) == NULL) {
			seterr(MEM_ERR, MALLOC);
			goto error_exit;
		}
		if (memcpy(*rec_buf, buf, numbytes) != *rec_buf) {
			seterr(MEM_ERR, MEMCPY);
			goto error_exit2;
		}
		if ((numbytes = recv(sockfd, &(*rec_buf)[numbytes], missing_bytes, 0))
				!= missing_bytes) {
			seterr(SOCKET_ERR,RECV);
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
s64_t sbp_connect(char* host_name, u64_t port)
{
	s64_t sockfd;

	struct hostent *target;
	struct sockaddr_in target_addr;

	if((target = gethostbyname(host_name)) == NULL){
		seterr(MEM_ERR,GET_HOSTNAME);
		goto error_exit;
	}
	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = ((struct in_addr *) (target->h_addr))->s_addr;
	target_addr.sin_port = htons(port);
	bzero(&(target_addr.sin_zero), 8);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		seterr(SOCKET_ERR, INIT_SOCK);
		goto error_exit;
	}
	if (connect(sockfd, (struct sockaddr *) &target_addr,
			sizeof(struct sockaddr))) {
		seterr(SOCKET_ERR, CONNECT);
		goto error_exit;
	}
	return sockfd;
error_exit:
	close(sockfd);
	return -1;

}
s32_t sendrec_hostname(char* host_name, u64_t port, char* data, u64_t len,
		char** rec_buf, u64_t * rec_len) {

	s64_t sockfd;
	if((sockfd = sbp_connect(host_name, port)) < 0){
		seterr(SOCKET_ERR, EST_SOCK);
		return -1;
	}

	if(sbp_send(sockfd, data, len) < 0){
		seterr(SOCKET_ERR, SEND);
		return -1;
	}

	if((sbp_recv(sockfd, rec_buf, rec_len))< 0){
		seterr(SOCKET_ERR, RECV);
		return -1;
	}
	return 0;
}
s64_t get_missing_len(char* buf) {
	u64_t content_length = 0, header_length = 0;
	char content_len[64];
	char* content_len_start = NULL;
	char* content_len_end = NULL;
	char* header_end;
	if ((header_end = strstr(buf, HEADER_FLAG)) == NULL) {
		seterr(HEAD_ERR, HEAD_FLAG);
		goto error_exit;
	}

	header_length = header_end - buf + strlen(HEADER_FLAG);
	//printf("header_start: %p ;header_end: %p ;header_len %lld ; strlen(header_flag) : %ld\n",buf,header_end,header_length,strlen(HEADER_FLAG));
	if ((content_len_start = strstr(buf, CONTENT_LEN)) == NULL) {
		seterr(HEAD_ERR, CONTENT_LEN);
		goto error_exit;
	}
	if ((content_len_end = strstr(content_len_start, "\r\n")) == NULL) {
		seterr(HEAD_ERR, LINE_SEPARATOR);
		goto error_exit;
	}
	content_len_start += strlen(CONTENT_LEN);
	if (memcpy(content_len, content_len_start, content_len_end
			- content_len_start) != content_len) {
		seterr(MEM_ERR, MEMCPY);
		goto error_exit;
	}
	content_len[content_len_end - content_len_start] = 0;
	content_length = atoll(content_len);
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
		seterr(HEAD_ERR, HEAD_FLAG);
		goto error_exit;
	}
	header_len = header_end - data + strlen(HEADER_FLAG);
	head->head_len = header_len;
	if ((head->data = malloc(header_len + 1)) == NULL) {
		seterr(MEM_ERR, MALLOC);
		goto error_exit;
	}
	head->data[header_len] = '\0';
	memcpy(head->data, data, header_len);
	head->title = head->data;
	if ((prev_ptr = strstr(head->data, "\r\n")) == NULL) {
		seterr(HEAD_ERR, LINE_SEPARATOR);
		goto error_exit2;
	}
	*prev_ptr = '\0';
	prev_ptr += 2;
	while (1) {
		if ((next_ptr = strstr(prev_ptr, "\r\n")) == NULL) {
			seterr(HEAD_ERR, LINE_SEPARATOR);
			goto error_exit2;
		}
		if (next_ptr == prev_ptr) {
			break;
		}
		head->entrys[i].name = prev_ptr;

		*next_ptr = '\0';
		next_ptr += 2;
		if ((prev_ptr = strstr(prev_ptr, ": ")) == NULL) {
			seterr(HEAD_ERR, COLON);
			goto error_exit2;
		}
		*prev_ptr = '\0';
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
		seterr(HEAD_ERR,TOO_MANY_ENTRY);
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
		seterr(MEM_ERR, MALLOC);
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
s32_t get_slot()
{
	s32_t i =0;
	while(i<MAX_FILE_OPEN)
	{
		if(fds[i] == NULL)
		{
			return i;
		}
		i++;
	}
	seterr(MEM_ERR, SLOT_FULL);
	return -1;
}
void free_sbpfd(struct sbp_filedesc* fd)
{
	if(fd == NULL)
		return;
	if(fd->filename!= NULL)
	{
		free(fd->filename);
	}
	free(fd);
	return;
}
char* get_head_entry_value(struct sbpfs_head* head,char* entry)
{
	if (entry == NULL) return NULL;
	int i = 0;
	while(i< head->entry_num)
	{
		if(strcmp(head->entrys[i].name,entry)== 0)
		{
			return head->entrys[i].value;
		}
	}
	return NULL;
}

