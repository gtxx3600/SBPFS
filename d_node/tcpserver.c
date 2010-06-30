/*
 * tcpserver.c
 *
 *  Created on: 2010-6-27
 *      Author: cxy
 */

#define BUF_SIZE  				8192
#define HEADER_FLAG				"\r\n\r\n"
#define CONTENT_LEN 			"Content-Length"

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
#include "blockman.h"
#define HEADER_FLAG				"\r\n\r\n"
#define PORT 8848
#define BACKLOG 5
#define MAXDATASIZE 1000
#define MAX_ENTRY_IN_HEAD 32
struct head_entry {
	char* name;
	char* value;
};
extern char* listpath;
struct sbpfs_head {
	u64_t head_len;
	char* title;
	int entry_num;
	struct head_entry entrys[MAX_ENTRY_IN_HEAD];
	char* data;
};

char* rec_buf;
u64_t rec_len;
u64_t req_len;
struct sbpfs_head sbphead;
char* send_buf;
s64_t get_missing_len(char* buf);
int decode_head(char* data, struct sbpfs_head* head);
int ana_head(struct sbpfs_head* head, char* data);
void* sbp_recv(void* newfd);
void process_cli(int connectfd, struct sockaddr_in client);
void* recinfo(int tfd);
s64_t ConnectServer(char* ip, int port);
s32_t sendinfo(s64_t sockfd, char* data, u64_t len);
struct sbpfs_head read_head(char* data, u64_t data_len, int status);
int build_headr(char** data, struct sbpfs_head* head);
int build_headw(char** data, struct sbpfs_head* head);
struct sbpfs_head write_head(char* data, u64_t data_len, int status);
int reportlist(char* ip, int port);
int reportwrite(char*ip, u64_t blocknum, int port, int status);

int main(void) {
	int listenfd, connectfd, newfd;
	pthread_t thread; //id of thread
	struct sockaddr_in server; //server's address info
	struct sockaddr_in client; //client's
	int sin_size;

	writeblock(3263, 3, 3, "abc");
		writeblock(3363, 3, 4, "abcd");
		writeblock(3463, 3, 3, "abc");
		readblock(3263, 3, 3);
		readblock(3363, 3, 4);


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
		perror("bind error.\n");
		exit(1);
	}
	if (listen(listenfd, BACKLOG) == -1) {
		perror("listen() error \n");
		exit(1);
	}
	char* ip = "127.0.0.1";
	reportlist(ip, PORT);
	sin_size = sizeof(struct sockaddr_in);
	while (1) {
		if ((connectfd = accept(listenfd, (struct sockaddr *) &client,
				(socklen_t*) &sin_size)) == -1) {
			printf("accept() error \n");
		}
		newfd = connectfd;

		if (pthread_create(&thread, NULL, sbp_recv, &newfd) != 0) {
			printf("error: can't create more thread!\n");
		}
	}
	return 0;
}

void* sbp_recv(void* newfd) {
	//char* ip;
	//int port;
	int tfd = *(int*) newfd;
	recinfo(tfd);
	decode_head(rec_buf, &sbphead);
	ana_head(&sbphead, rec_buf);

	//s64_t sockfd = ConnectServer(ip, port);
	sendinfo(tfd, send_buf, sizeof(send_buf));
	return 0;
}

void* recinfo(int tfd) {
	u64_t missing_bytes, numbytes;
	char buf[BUF_SIZE + 1];
	buf[BUF_SIZE] = 0;
	if ((numbytes = recv(tfd, buf, BUF_SIZE, 0)) < 0) {
		perror("error in receive\n");
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
		perror("creating socket failed\n");
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
		perror("line seperate\n");
		goto error_exit;
	}
	content_len_start += strlen(CONTENT_LEN);
	if (memcpy(content_len, content_len_start, content_len_end
			- content_len_start) != content_len) {
		perror("memory copy\n");
		goto error_exit;
	}
	content_len[content_len_end - content_len_start] = 0;
	content_length = atoll(content_len);
	//printf("missing length : %lld\n",(content_length + header_length - BUF_SIZE));
	return content_length + header_length - BUF_SIZE;
	error_exit: return -1;
}

int decode_head(char* data, struct sbpfs_head* head) {
	char* header_end;
	u64_t header_len;
	char* prev_ptr, *next_ptr;
	int i;
	if ((header_end = strstr(data, HEADER_FLAG)) == NULL) {
		perror("error in get headflag\n");
	}
	header_len = header_end - data + strlen(HEADER_FLAG);
	head->head_len = header_len;
	if ((head->data = malloc(header_len + 1)) == NULL) {
		perror("error in malloc\n");
	}
	head->data[header_len] = 0;
	memcpy(head->data, data, header_len);
	head->title = head->data;
	if ((prev_ptr = strstr(head->data, "\r\n")) == NULL) {
		perror("not get line separator\n");
		return -1;
	}
	while (1) {
		if ((next_ptr = strstr(prev_ptr, "\r\n")) == NULL) {
			perror("not get line separator\n");
			return -1;
		}
		if (next_ptr == prev_ptr) {
			break;
		}
		head->entrys[i].name = prev_ptr;
		*next_ptr = 0;
		next_ptr += 2;
		if ((prev_ptr = strstr(prev_ptr, ": ")) == NULL) {
			perror("not get value");
			return -1;
		}
		*prev_ptr = 0;
		prev_ptr += 2;
		head->entrys[i].value = prev_ptr;
		head->entry_num += 1;
		prev_ptr = next_ptr;
		i++;
	}
	char* databuffer = malloc(sizeof(data) - header_len + 1);
	databuffer[sizeof(data) - header_len] = 0;
	memcpy(databuffer, header_end, sizeof(data) - header_len);
	return 0;
}
char* get_head_entry_value(struct sbpfs_head* head, char* entry) {
	if (entry == NULL)
		return NULL;
	int i = 0;
	while (i < head->entry_num) {
		if (strcmp(head->entrys[i].name, entry) == 0) {
			return head->entrys[i].value;
		}
	}
	return NULL;
}

int ana_head(struct sbpfs_head* head, char* data) {
	char* ip;
	int port;
	char* headmethod;
	char* blocknumc;
	char* offsetc;
	char* lengthc;
	blocknumc = get_head_entry_value(head, "Arg0");
	offsetc = get_head_entry_value(head, "Arg1");
	lengthc = get_head_entry_value(head, "Arg2");
	u64_t blocknum = atoll(blocknumc);
	req_len = atoll(lengthc);
	u64_t req_offset = atoll(offsetc);
	if ((headmethod = get_head_entry_value(head, "method")) == NULL) {
		perror("fail to ananlyse! \n");
		return -1;
	} else if (strcmp(headmethod, "READ") == 0) {
		int succeed=readblock(blocknum, req_offset, req_len);
		sbphead = read_head(databuf, req_len,succeed);
		build_headr(&send_buf, &sbphead);
	} else if (strcmp(headmethod, "WRITE") == 0) {
		int succeed = writeblock(blocknum, req_offset, req_len, data);
		sbphead = write_head(databuf, req_len,succeed);
		build_headw(&send_buf, &sbphead);
		reportwrite(ip, blocknum, port, succeed);
	}
	return 0;
}

int build_headr(char** data, struct sbpfs_head* head) {
	int i = 0;
	u64_t total_len = 0;
	char* cp_ptr;
	if (head->entry_num > MAX_ENTRY_IN_HEAD) {
		perror("too much entry \n");
		return -1;
	}
	total_len += strlen(head->title) + 2;
	total_len += strlen(head->entrys[0].name) + strlen(head->entrys[0].value)
			+ 4;
	total_len += 2;
	total_len += strlen(databuf);
	if ((*data = malloc(total_len)) == NULL) {
		perror("fail in malloc");
		return -1;
	}
	cp_ptr = *data;
	strcpy(cp_ptr, head->title);
	cp_ptr += strlen(head->title);
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	strcpy(cp_ptr, head->entrys[i].name);
	cp_ptr += strlen(head->entrys[i].name);
	*cp_ptr++ = ':';
	*cp_ptr++ = ' ';
	strcpy(cp_ptr, head->entrys[i].value);
	cp_ptr += strlen(head->entrys[i].value);
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	strcpy(cp_ptr, databuf);
	return 0;
}

struct sbpfs_head read_head(char* data, u64_t data_len, int status) {
	struct sbpfs_head head;
		if (status < 0)
			head.title = "SBPFS ERR";
		else
			head.title = "SBPFS OK";
	head.entry_num = 1;
	head.entrys[0].name = "Content-Length";
	char tran_len[32];
	sprintf(tran_len, "%lld", data_len);
	head.entrys[0].value = tran_len;
	head.data = data;
	return head;
}

struct sbpfs_head write_head(char* data, u64_t data_len, int status) {
	struct sbpfs_head head;
	if (status < 0)
		head.title = "SBPFS ERR";
	else
		head.title = "SBPFS OK";
	head.entry_num = 1;
	head.entrys[0].name = "Content-Length";
	char tran_len[32];
	sprintf(tran_len, "%lld", data_len);
	head.entrys[0].value = tran_len;
	head.data = data;
	return head;
}

int build_headw(char** data, struct sbpfs_head* head) {
	int i = 0;
	u64_t total_len = 0;
	char* cp_ptr;
	if (head->entry_num > MAX_ENTRY_IN_HEAD) {
		perror("too much entry \n");
		return -1;
	}
	total_len += strlen(head->title) + 2;
	total_len += strlen(head->entrys[0].name) + strlen(head->entrys[0].value)
			+ 4;
	total_len += 2;

	if ((*data = malloc(total_len)) == NULL) {
		perror("fail in malloc");
		return -1;
	}
	cp_ptr = *data;
	strcpy(cp_ptr, head->title);
	cp_ptr += strlen(head->title);
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	strcpy(cp_ptr, head->entrys[i].name);
	cp_ptr += strlen(head->entrys[i].name);
	*cp_ptr++ = ':';
	*cp_ptr++ = ' ';
	strcpy(cp_ptr, head->entrys[i].value);
	cp_ptr += strlen(head->entrys[i].value);
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	return 0;
}
struct sbpfs_head report_head(char* data, int blocknum) {
	struct sbpfs_head head;
	head.title = "SBPFS OK";
	head.entry_num = 3;
	head.entrys[0].name = "USER";
	head.entrys[0].value = "";
	head.entrys[1].name = "USER";
	head.entrys[1].value = "";
	head.entrys[2].name = "Block_num";
	char tran_num[32];
	sprintf(tran_num, "%d", blocknum);
	head.entrys[2].value = tran_num;
	head.data = data;
	return head;
}

int build_headp(char** data, struct sbpfs_head* head) {
	int i = 0;
	u64_t total_len = 0;
	char* cp_ptr;
	if (head->entry_num > MAX_ENTRY_IN_HEAD) {
		perror("too much entry \n");
		return -1;
	}
	total_len += strlen(head->title) + 2;
	while (i < head->entry_num) {
		total_len += strlen(head->entrys[i].name) + strlen(
				head->entrys[i].value) + 4;
		i++;
	}
	total_len += 2;
	total_len += 64 * bnum;
	if ((*data = malloc(total_len)) == NULL) {
		perror("malloc error \n");
		return -1;
	}
	cp_ptr = *data;
	strcpy(cp_ptr, head->title);
	cp_ptr += strlen(head->title);
	*cp_ptr++ = '\r';
	*cp_ptr++ = '\n';
	i = 0;
	while (i < head->entry_num) {
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
	memcpy(cp_ptr, *blocklist, 64 * bnum);
	return 0;
}

int reportlist(char* ip, int port) {
	if (getlist() < 0) {
		perror("fail to get list \n");
		return -1;
	}
	s64_t sockfd = ConnectServer(ip, port);
	sbphead = report_head(databuf, req_len);
	build_headp(&send_buf, &sbphead);
	sendinfo(sockfd, send_buf, sizeof(send_buf));
	recinfo(sockfd);
	if (strcmp(rec_buf, "SBPFS_OK") == 0) {
		printf("succeed in report");
		return 0;
	} else if (strcmp(rec_buf, "SBPFS_ERR") == 0) {
		printf("fail in report");
		return -1;
	} else {
		perror("fail to receive feedback \n");
		return -1;
	}
}

int reportwrite(char*ip, u64_t blocknum, int port, int status) {
	s64_t sockfd = ConnectServer(ip, port);
	u64_t total_len = 0;
	char* result_report;
	char* title = "WRITE_OK";
	char* name = "Blocknum";
	char blnum[64];
	sprintf(blnum, "%lld", blocknum);
	total_len += strlen(title) + 2;
	total_len += strlen(name) + strlen(blnum) + 4;
	total_len += 2;
	if ((result_report = malloc(total_len)) == NULL) {
		perror("fail to malloc\n");
		return -1;
	}
	strcpy(result_report, title);
	result_report += strlen(title);
	*result_report++ = '\r';
	*result_report++ = '\n';
	strcpy(result_report, name);
	result_report += strlen(name);
	*result_report++ = ':';
	*result_report++ = ' ';
	strcpy(result_report, blnum);
	result_report += strlen(blnum);
	*result_report++ = '\r';
	*result_report++ = '\n';
	*result_report++ = '\r';
	*result_report++ = '\n';
	sendinfo(sockfd, result_report, total_len);
	return 0;
}
