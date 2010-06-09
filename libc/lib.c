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
#define PORT 1500 /* the port client will be connecting to */

int get_missing_len(char* buf);

int sendrec_ip(char* host_name,int port,char* data,int len,char** rec_buf,int * rec_len)
{
	int sockfd,numbytes,missing_bytes;
	char buf[BUF_SIZE+1];

	struct hostent *target;
	struct sockaddr_in target_addr;



	buf[BUF_SIZE] = 0;
	target = gethostbyname(host_name);
	target_addr.sin_family = AF_INET;
//	target_addr.sin_addr.s_addr = htol(INADDR_ANY);
	target_addr.sin_addr.s_addr = ((struct in_addr *)(target->h_addr))->s_addr;
	target_addr.sin_port = htons(port);
	bzero(&(target_addr.sin_zero), 8);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Socket Error, %d\n", errno);
		goto error_exit;
	}
	if(connect(sockfd, (struct sockaddr *)&target_addr, sizeof(struct sockaddr)))
	{
		printf("Connect Error, %d\n", errno);
		goto error_exit;
	}

	numbytes = send(sockfd, data, len, 0);
	if(numbytes != len){
		printf("Socket Send Length Error!\n");
		goto error_exit;
	}

	numbytes = recv(sockfd, buf, BUF_SIZE, 0);
	if(numbytes < BUF_SIZE){
		if((*rec_buf = (char*)malloc(numbytes + 1)) == NULL){
			printf("Socket Send Malloc Error!\n");
			goto error_exit;
		}
		if(memcpy(*rec_buf,buf,numbytes) != rec_buf){
			printf("Socket Send Memcpy Error!\n");
			goto error_exit;
		}
		(*rec_buf)[numbytes] = 0;
		*rec_len = numbytes;
		return 0;
	}else{
		if((missing_bytes = get_missing_len(buf)) < 0){
			printf("Socket Send get_missing_len Error!\n");
			goto error_exit;
		}
		if((*rec_buf = (char*)malloc(missing_bytes + numbytes + 1)) == NULL){
			printf("Socket Send Malloc Error!\n");
			goto error_exit;
		}
		if(memcpy(*rec_buf,buf,numbytes) != rec_buf){
			printf("Socket Send Memcpy Error!\n");
			goto error_exit;
		}
		if((numbytes = recv(sockfd, &(*rec_buf)[numbytes], missing_bytes, 0)) != missing_bytes){
			printf("Socket Send last_recv Error!\n");
			goto error_exit;
		}
		(*rec_buf)[missing_bytes + numbytes + 1] = 0;
		*rec_len = missing_bytes + numbytes + 1;
		return 0;

	}


error_exit:
	close(sockfd);
	return -1;
}
int get_missing_len(char* buf)
{
	int content_length = 0,header_length=0;
	char content_len[64];
	char* content_len_start = NULL;
	char* content_len_end = NULL;
	char* header_end;
	if((header_end = strstr(buf,HEADER_FLAG)) == NULL){
		printf("Socket Send find header_flag Error!\n");
		goto error_exit;
	}
	header_length = header_end - buf + sizeof(HEADER_FLAG);
	if((content_len_start = strstr(buf,CONTENT_LEN)) == NULL){
		printf("Socket Send Strstr Error!\n");
		goto error_exit;
	}
	if((content_len_end = strstr(content_len_start, "\r\n")) == NULL)
	{
		printf("Socket Send Strstr2 Error!\n");
		goto error_exit;
	}
	if(memcpy(content_len,content_len_start + sizeof(CONTENT_LEN),content_len_end - content_len_start) != content_len){
		printf("Socket Send Memcpy2 Error!\n");
		goto error_exit;
	}
	content_len[content_len_end - content_len_start] = 0;
	content_length = atoi(content_len);
	return content_length + header_length - BUF_SIZE;

error_exit:
	return -1;
}
void test()
{
	printf("haha");
}





