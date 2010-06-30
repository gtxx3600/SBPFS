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
static void dump_recbuf(char* rec_buf,u64_t rec_len);
char dnode_hostname[32];
s64_t get_missing_len(char* buf,u64_t received_bytes);
char* get_dnode_hostname(u32_t dnode)
{
	unsigned char i1,i2,i3,i4;
	i1 = dnode / (256*256*256);
	i2 = (dnode / (256*256))%256;
	i3 = (dnode /256)%256;
	i4 = dnode % 256;
	sprintf(dnode_hostname,"%u.%u.%u.%u",i1,i2,i3,i4);
	return dnode_hostname;
}
s32_t sbp_send(s64_t sockfd, char* data, u64_t len)
{
	//dump_recbuf(data,512);
	s64_t numbytes;
	s64_t sent_len = 0;
	while(1){
		numbytes = send(sockfd, data+sent_len, len-sent_len, 0);
		//printf("numbytes: %lld\n",numbytes);
		if(numbytes <= 0)
		{
			seterr(SOCKET_ERR, "Connection Closed");
			close(sockfd);
			return -1;
		}

		sent_len += numbytes;
		if(sent_len >= len)
		{
			return 0;
		}
	}

	return 0;
}
static void dump_recbuf(char* rec_buf,u64_t rec_len)
{
	u64_t i =0;
	for(;i<rec_len;i++)
	{
		if((rec_buf[i]>= 32)&& (rec_buf[i] <= 126))
		{
			printf("%c",rec_buf[i]);
		}else{
			printf("\\%.2x",rec_buf[i]);
		}
	}
	printf("\n");
}
s32_t sbp_recv(s64_t sockfd, char** rec_buf, u64_t* rec_len)
{
	s64_t missing_bytes,numbytes,received_len = 0;
	char buf[BUF_SIZE + 1];
	buf[BUF_SIZE] = 0;
	while(1){
		//printf("main loop\n");
		numbytes = recv(sockfd, buf + received_len, BUF_SIZE - received_len, 0);
		//printf("main loop2\n");
		if(numbytes == 0)
		{//printf("main loop3\n");
			seterr(SOCKET_ERR,RECV);
			goto error_exit;
		}
		received_len += numbytes;
		//printf("main start get missing len\n");
		if((missing_bytes = get_missing_len(buf,received_len)) < 0)
		{
			if(received_len >= BUF_SIZE)
			{
				seterr(SOCKET_ERR,RECV);
				goto error_exit;
			}
			//printf("main loop continue numbytes :%lld\n",numbytes);
			continue;
		}else
		{
			//printf("recev %lld length\n",numbytes);
			if ((*rec_buf = (char*) malloc(missing_bytes + received_len + 1)) == NULL) {
				char tmp[128];
				sprintf(tmp,"missing bytes:%lld received_bytes: %lld",missing_bytes,numbytes);

				seterr(MEM_ERR, tmp);//MALLOC tmp);
				goto error_exit;
			}
			if (memcpy(*rec_buf, buf, received_len) != *rec_buf) {
				seterr(MEM_ERR, MEMCPY);
				goto error_exit2;
			}
			u64_t received_len2 = 0;

			while(missing_bytes){
				//printf("sub loop\n");
				if ((numbytes = recv(sockfd, (*rec_buf) + received_len + received_len2, missing_bytes - received_len2, 0))
						< 0) {
					seterr(SOCKET_ERR,RECV);
					goto error_exit2;
				}
				//printf("advanced recev %lld length\n",numbytes);
				received_len2 += numbytes;
				if(received_len2 >= missing_bytes)
				{
					break;
				}
				if(numbytes == 0)
				{
						//printf("received_len1 :%lld received_len2 :%lld missing: %lld numbytes:%lld",received_len,received_len2,missing_bytes,numbytes);
						seterr(SOCKET_ERR,RECV);
						goto error_exit2;
				}
			}
			(*rec_buf)[missing_bytes + received_len] = 0;
			*rec_len = missing_bytes + received_len + 1;
			goto ok_exit;
		}
	}

	error_exit2:free(*rec_buf);
	error_exit: close(sockfd);
	//printf("sbp receive return\n");
	return -1;

	ok_exit: close(sockfd);

	//printf("received :%lld ok and return\n",*rec_len);
	//dump_recbuf(*rec_buf, *rec_len);
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
	//printf("connect done\n");
	if(sbp_send(sockfd, data, len) < 0){
		seterr(SOCKET_ERR, SEND);
		return -1;
	}
	//printf("send done\n");
	if((sbp_recv(sockfd, rec_buf, rec_len))< 0){
		seterr(SOCKET_ERR, RECV);
		return -1;
	}
	//printf("recv done\n");
	return 0;
}
s64_t get_missing_len(char* buf,u64_t received_bytes) {
	//printf("start get_missing_len\n");
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
	//printf("start get_missing_len 2\n");
	content_len_start += strlen(CONTENT_LEN) + 2;
	if (memcpy(content_len, content_len_start, content_len_end
			- content_len_start) != content_len) {
		seterr(MEM_ERR, MEMCPY);
		goto error_exit;
	}
	content_len[content_len_end - content_len_start] = 0;
	content_length = atoll(content_len);
	//printf("start get_missing_len 3\n");
	//printf("missing length : %lld content_length:%lld header_len: %lld received_len: %lld\n ",(content_length + header_length - received_bytes),content_length,header_length,received_bytes);
	return content_length + header_length - received_bytes;

	error_exit: return -1;
}

s32_t decode_head(char* data, u64_t len, struct sbpfs_head* head) {
	if(head == NULL) return -1;
	char* header_end;
	char* prev_ptr, *next_ptr;
	//char* next_ptr;
	u64_t header_len;
	int i = 0;
	init_head(head);
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
		i++;
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
		i++;
	}
	return NULL;
}

