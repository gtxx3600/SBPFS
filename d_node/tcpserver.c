/*
 * tcpserver.c
 *
 *  Created on: 2010-6-27
 *      Author: cxy
 */

#define BUF_SIZE  				8192
#define HEADER_FLAG				"\r\n\r\n"
#define CONTENT_LEN 			"Content-Length"

typedef unsigned long long u64_t;
typedef long long s64_t;
typedef unsigned int u32_t;
typedef int s32_t;
typedef unsigned short u16_t;
typedef short s16_t;
typedef unsigned char u8_t;
typedef char s8_t;

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8848
#define BACKLOG 5
#define MAXDATASIZE 1000

void process_cli(int connectfd, struct sockaddr_in client);
void* recinfo(void* arg);
s64_t ConnectServer(char* ip, int port);
s32_t sendinfo(s64_t sockfd, char* data, u64_t len);
char* rec_buf;
u64_t rec_len;
s64_t get_missing_len(char* buf);

int main(void) {
	int listenfd, connectfd, newfd;
	pthread_t thread; //id of thread
	struct sockaddr_in server; //server's address info
	struct sockaddr_in client; //client's
	int sin_size;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("creating socket failed.");
		exit(1);
	}

	int opt = SO_REUSEADDR;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *) &server, sizeof(struct sockaddr))
			== -1) {
		perror("bind error.");
		exit(1);
	}
	if (listen(listenfd, BACKLOG) == -1) {
		perror("listen() error ");
		exit(1);
	}

	sin_size = sizeof(struct sockaddr_in);
	while (1) {
		if ((connectfd = accept(listenfd, (struct sockaddr *) &client,
				(socklen_t*) &sin_size)) == -1) {
			printf("accept() error ");
		}
		newfd = connectfd;

		if (pthread_create(&thread, NULL, recinfo, (void*) newfd) != 0) {
			printf("error: can't create more thread!\n");
		}
	}
	return 0;
}

void* recinfo(void* newfd) {
	int tfd;
	tfd = *(int*) newfd;
	u64_t missing_bytes, numbytes;
	char buf[BUF_SIZE + 1];
	buf[BUF_SIZE] = 0;
	if ((numbytes = recv(tfd, buf, BUF_SIZE, 0)) < 0) {
		perror("error in receive");
	}
	if (numbytes < BUF_SIZE) {
		if ((rec_buf = (char*) malloc(numbytes + 1)) == NULL) {
			exit(-1);
		}
		if (memcpy(rec_buf, buf, numbytes) != rec_buf) {
			exit(-1);
		}
		rec_buf[numbytes] = 0;
		rec_len = numbytes + 1;

	} else {
		if ((missing_bytes = get_missing_len(buf)) < 0) {
		}
		if ((rec_buf = (char*) malloc(missing_bytes + numbytes + 1)) == NULL) {
			exit(-1);
		}
		if (memcpy(rec_buf, buf, numbytes) != rec_buf) {
			exit(-1);
		}
		if ((numbytes = recv(tfd, &(rec_buf)[numbytes], missing_bytes, 0))
				!= missing_bytes) {
			exit(-1);
		}
		rec_buf[missing_bytes + BUF_SIZE] = 0;
		rec_len = missing_bytes + BUF_SIZE + 1;
		goto ok_exit;

	}

	ok_exit: close(tfd);
	return 0;
}

s64_t ConnectServer(char* ip, int port) {
	struct sockaddr_in target_addr;
	s64_t sockfd;
	bzero(&(target_addr.sin_zero), 8);
	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = inet_addr(ip);
	target_addr.sin_port = htons(port);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("creating socket failed.");
		exit(1);
	}
	if (connect(sockfd, (struct sockaddr *) &target_addr,
			sizeof(struct sockaddr))) {
		perror("connect error!\n");
		exit(1);
	}
	return sockfd;
}

s32_t sendinfo(s64_t sockfd, char* data, u64_t len) {
	u64_t numbytes;
	numbytes = send(sockfd, data, len, 0);
	if (numbytes != len) {
		perror("error\n");
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
		perror("error in head\n");
		return -1;
	}

	header_length = header_end - buf + strlen(HEADER_FLAG);
	//printf("header_start: %p ;header_end: %p ;header_len %lld ; strlen(header_flag) : %ld\n",buf,header_end,header_length,strlen(HEADER_FLAG));
	if ((content_len_start = strstr(buf, CONTENT_LEN)) == NULL) {
		goto error_exit;
	}
	if ((content_len_end = strstr(content_len_start, "\r\n")) == NULL) {
		perror("line seperate");
		goto error_exit;
	}
	content_len_start += strlen(CONTENT_LEN);
	if (memcpy(content_len, content_len_start, content_len_end
			- content_len_start) != content_len) {
		perror("memory copy");
		goto error_exit;
	}
	content_len[content_len_end - content_len_start] = 0;
	content_length = atoll(content_len);
	//printf("missing length : %lld\n",(content_length + header_length - BUF_SIZE));
	return content_length + header_length - BUF_SIZE;

	error_exit: return -1;
}

