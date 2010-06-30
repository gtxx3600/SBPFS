/*
 * d_node.c
 *
 *  Created on: 2010-6-29
 *      Author: hhf
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "d_node.h"
#include "lib.h"
#include "error.h"
#include "blockman.h"
char dname[32];
void send_err(int skt, char* err_type, char* err_detail);
void free_ent(struct list_entry* ent);

void process_req(struct list_entry * ent);
void* serve(void* arg);
void send_list(char* ip);
int send_copy(char*ip, u64_t blocknum);
int get_reply(int skt);
struct list_entry list_head;
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t list_cond = PTHREAD_COND_INITIALIZER;
FILE *fp;
int main() {

	//getlist();

	initlist();
	//writeblock(3, 3, 3, "abc");
	//getlist();
	char*ip = "192.168.1.107";
	strcpy(dname,"haha");
	send_list(ip);
	//

	bzero(&list_head, sizeof(struct list_entry));
	pthread_t pid;

	int socket_descriptor;
	if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket create error");
		exit(1);
	}
	int on;
	on = 1;
	setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	int port = 9010;
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	bzero(&(sin.sin_zero), 8);
	if (bind(socket_descriptor, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		perror("bind error");
		exit(1);
	}

	if (listen(socket_descriptor, 10) == -1) {
		perror("listen error");
		exit(1);
	}

	struct sockaddr_in pin;
	struct list_entry* ent;

	int temp_socket_descriptor;

	printf("DISK DRIVER START...\n");
	//if (fork() != 0) {
	//send_copy(ip,1234);
	//} else {
	while (1) {
		unsigned int sin_size = sizeof(struct sockaddr_in);
		temp_socket_descriptor = accept(socket_descriptor,
				(struct sockaddr *) &pin, &sin_size);

		if (temp_socket_descriptor == -1) {
			perror("Accept Failed");
			continue;
		}
		//printf("Accept Success\n");
		if ((ent = (struct list_entry*) malloc(sizeof(struct list_entry)))
				== NULL) {
			perror("Malloc Failed");
			close(temp_socket_descriptor);
			continue;
		}
		bzero(ent, sizeof(struct list_entry));
		ent->skt = temp_socket_descriptor;
		if (pthread_create(&pid, NULL, serve, ent)) {
			perror("Create Thread Failed");
			close(temp_socket_descriptor);
			free(ent);
			continue;
		}
		pthread_detach(pid);
		//close(temp_socket_descriptor);
	}
	//}
	return 0;
}

void* serve(void* arg) {
	struct list_entry* ent = (struct list_entry*) arg;
	struct sbpfs_head head;
	char* rec_buf;
	u64_t rec_len;
	//printf("start receive\n");

	if (sbp_recv(ent->skt, &rec_buf, &rec_len) == -1) {
		printf("failed to receive data\n");
		return NULL;
	}
	if (strlen(rec_buf) == 0) {
		goto err_exit;
	}
	if (decode_head(rec_buf, rec_len, &head) == -1) {
		printf("decode head failed\n");
		goto err_exit;
	}/*head need free*/
	char* method;
	if ((method = get_head_entry_value(&head, METHOD)) == NULL) {
		printf("get head entry failed\n");
		goto err_exit2;
	}
	if (strcmp(method, "READ") == 0) {
		ent->req = 1;
	} else if (strcmp(method, "WRITE") == 0) {
		ent->req = 2;
	} else if (strcmp(method, "Copy") == 0) {
		ent->req = 3;
	} else {
		printf("invalid request\n");
		goto err_exit2;
	}
	char* block_num_s;
	char* offset_s;
	char* length_s;
	char* content_len_s;

	if ((block_num_s = get_head_entry_value(&head, "Arg0")) == NULL) {
		printf("get head entry Arg0 failed\n");
		goto err_exit2;
	}
	if ((offset_s = get_head_entry_value(&head, "Arg1")) == NULL) {
		printf("get head entry Arg1 failed\n");
		goto err_exit2;
	}
	if ((length_s = get_head_entry_value(&head, "Arg2")) == NULL) {
		printf("get head entry Arg2 failed\n");
		goto err_exit2;
	}
	if ((content_len_s = get_head_entry_value(&head, "Content-Length")) == NULL) {
		printf("get head entry Content-Length failed\n");
		goto err_exit2;
	}

	ent->block_num = atoll(block_num_s);
	ent->offset = atoi(offset_s);
	ent->length = atoi(length_s);
	if (ent->req == 2 || ent->req == 3) {
		ent->data_length = atoll(content_len_s);
		ent->data = rec_buf + head.head_len;
	}
	//send_err(ent->skt, "a", "b");
	process_req(ent);

	err_exit2: free_head(&head);
	err_exit: free(rec_buf);

	close(ent->skt);
	free(ent);
	return NULL;

}
void send_err(int skt, char* err_type, char* err_detail) {
	struct sbpfs_head head;
	char* data;
	int data_len = 0;
	head.data = NULL;
	head.title = REQUEST_ERR;
	head.entry_num = 0;
	mkent(head,ERR_TYPE,err_type);
	mkent(head,ERR_DETAIL,err_detail);
	mkent(head,CONTENT_LEN,"0");
	if (make_head(&data, &data_len, &head) == -1) {
		return;
	}
	if (sbp_send(skt, data, data_len) < 0) {
		printf("send err failed\n");
		sbp_perror("a");
		perror("a");
		goto err_exit;
	}
	free(data);
	return;

	err_exit: free(data);
	return;
}
void process_req(struct list_entry * ent) {
	char* head_data;
	int head_data_len;
	struct sbpfs_head head;
	if (ent->req == 1) {
		char* data = (char*) malloc(ent->length);
		if (data == NULL) {
			printf("malloc data failed\n");
			return;
		}
		int succeed = readblock(ent->block_num, ent->offset, ent->length, data);
		printf("succeed: %d\n", succeed);
		if (succeed < 0)
			send_err(ent->skt, "read_block", "cannot read block");
		else {
			head.data = NULL;
			head.title = REQUEST_OK;
			head.entry_num = 0;
			char data_length_s[32];
			sprintf(data_length_s, "%d", ent->length);
			mkent(head,CONTENT_LEN,data_length_s);
			if (make_head(&head_data, &head_data_len, &head) == -1) {
				printf("mkhead failed\n");
				free(head_data);
				free(data);
				return;
			}
			if (sbp_send(ent->skt, head_data, head_data_len) < 0) {
				printf("send head err failed\n");
				sbp_perror("a");
				perror("a");
				free(data);
				free(head_data);
				return;
			}
			if (sbp_send(ent->skt, data, ent->length) < 0) {
				printf("send data err failed\n");
				sbp_perror("a");
				perror("a");
				free(data);
				free(head_data);

				return;
			}
			free(data);
			free(head_data);
			return;
		}
	}
	if (ent->req == 2) {
		int succeed = writeblock(ent->block_num, ent->offset, ent->length,
				ent->data);
		printf("succeed in write : %d\n", succeed);
		if (succeed < 0) {
			send_err(ent->skt, "write_block", "cannot write block");
		} else {
			head.data = NULL;
			head.title = REQUEST_OK;
			head.entry_num = 0;
			mkent(head,CONTENT_LEN,"0");
		}
		if (make_head(&head_data, &head_data_len, &head) == -1) {
			printf("mkhead failed\n");
			free(head_data);
			return;
		}
		if (sbp_send(ent->skt, head_data, head_data_len) < 0) {
			printf("send head err failed\n");
			sbp_perror("a");
			perror("a");
			free(head_data);
			return;
		}
	}
	if (ent->req == 3) {
		int succeed = writeblock(ent->block_num, ent->offset, ent->length,
				ent->data);
		printf("succeed in copy : %d\n", succeed);

	}
}

void send_list(char* ip) {
	int skt = sbp_connect(ip, CNODE_SERVICE_PORT);
	u64_t blnum = getlist();
	struct sbpfs_head head;
	char* head_data;
	int head_data_len;
	head.data = NULL;
	head.entry_num = 0;
	head.title = PROTOCOL;
	char block_num_s[32];
	sprintf(block_num_s, "%lld", blnum);
	char c_len[32];
	sprintf(c_len, "%lld", blnum * 8);
	char dnodename[38];
	sprintf(dnodename, "%s%s", "DNode_", dname);
	mkent(head,CONTENT_LEN,c_len);
	mkent(head,USER,dnodename);
	mkent(head,"BlockNum",block_num_s);
	if (make_head(&head_data, &head_data_len, &head) == -1) {
		printf("mkhead failed\n");
		free(head_data);
		return;
	}
	if (sbp_send(skt, head_data, head_data_len) < 0) {
		printf("send head err failed\n");
		sbp_perror("a");
		perror("a");
		free(head_data);
		return;
	}
	if (sbp_send(skt, (char*) blolist, bnum * 8) < 0) {
		printf("send head err failed\n");
		sbp_perror("a");
		perror("a");
		free(head_data);
		return;
	}
	printf("sendlist succeed\n");
	if (get_reply(skt) < 0) {
		printf("error sendlist \n");
		return;
	}
	printf("reply succeed\n");

}

void send_success(int skt, u64_t blocknum) {
	struct sbpfs_head head;
	char* head_data;
	int head_data_len = 0;
	head.data = NULL;
	head.title = PROTOCOL;
	head.entry_num = 0;
	char blocknum_s[32];
	sprintf(blocknum_s, "%lld", blocknum);
	mkent(head,"Arg0",blocknum_s);
	mkent(head,CONTENT_LEN,"0");
	if (make_head(&head_data, &head_data_len, &head) == -1) {
		printf("mkhead failed\n");
		free(head_data);
		return;
	}
	if (sbp_send(skt, head_data, head_data_len) < 0) {
		printf("send head err failed\n");
		sbp_perror("a");
		perror("a");
		free(head_data);
		return;
	}
}

int get_reply(int skt) {
	struct sbpfs_head head;
	char* rec_buf;
	u64_t rec_len;
	if (sbp_recv(skt, &rec_buf, &rec_len) == -1) {
		printf("failed to receive data\n");
		return -1;
	}
	if (strlen(rec_buf) == 0) {
		return -1;
	}
	if (decode_head(rec_buf, rec_len, &head) == -1) {
		printf("decode head failed\n");
		return -1;
	}/*head need free*/
	if (strcmp(head.title, REQUEST_OK) == 0) {
		free_head(&head);
		return 0;
	} else {
		free_head(&head);
		return -1;
	}
}

int send_copy(char*ip, u64_t blocknum) {
	struct sbpfs_head head;
	char* head_data;
	int head_data_len;
	char* data = (char*) malloc(1024 * 1024 * 16);
	if (data == NULL) {
		printf("malloc data failed\n");
		return -1;
	}
	int succeed = readblock(blocknum, 0, 1024 * 1024 * 16, data);
	printf("succeed: %d\n", succeed);
	int skt = sbp_connect(ip, DNODE_SERVICE_PORT);
	printf("%d", skt);
	if (succeed < 0)
		send_err(skt, "copy_block", "cannot read block");
	else {
		printf("send start\n");
		head.data = NULL;
		head.entry_num = 0;
		head.title = REQUEST_OK;
		mkent(head,METHOD,"Copy");
		mkent(head,ARGC,"3");
		char blocknum_s[32];
		sprintf(blocknum_s, "%lld", blocknum);
		mkent(head,"Arg0",blocknum_s);
		mkent(head,"Arg1","0");
		char data_len_s[32];
		sprintf(data_len_s, "%d", 1024 * 1024 * 16);
		mkent(head,"Arg2",data_len_s);
		char data_length_s[32];
		sprintf(data_length_s, "%d", 1024 * 1024 * 16);
		mkent(head,CONTENT_LEN,data_length_s);

		if (make_head(&head_data, &head_data_len, &head) == -1) {
			printf("mkhead failed\n");
			free(data);
			free(head_data);
			return -1;
		}
		if (sbp_send(skt, head_data, head_data_len) < 0) {
			printf("send head err failed\n");
			sbp_perror("a");
			perror("a");
			free(data);
			free(head_data);
			return -1;
		}

		if (sbp_send(skt, data, 1024 * 1024 * 16) < 0) {
			printf("send data err failed\n");
			sbp_perror("a");
			perror("a");
			free(data);
			free(head_data);
			return -1;
		}
		printf("send succeed\n");
		free(data);
		free(head_data);
		return 0;
	}
	return 0;
}
