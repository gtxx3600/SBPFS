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
#include "d_node.h"
#include "lib.h"


void send_err(int skt,char* err_type,char* err_detail);
void free_ent(struct list_entry* ent);

void process_req(struct list_entry * ent);
void* serve(void* arg);
struct list_entry list_head;
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t 	list_cond = PTHREAD_COND_INITIALIZER;
FILE *fp;
int main(){

	bzero(&list_head,sizeof(struct list_entry));
	pthread_t pid;

	int socket_descriptor;
	if((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0))==-1)
	{
		perror("socket create error");
		exit(1);
	}
	int on;
	on =1;
	setsockopt( socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
	int port = 9010;
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	bzero(&(sin.sin_zero),8);
	if(bind(socket_descriptor,(struct sockaddr *)&sin, sizeof(sin) ) == -1)
	{
		perror("bind error");
		exit(1);
	}

	if(listen(socket_descriptor, 10) == -1)
	{
		perror("listen error");
		exit(1);
	}

	struct sockaddr_in pin;
	struct list_entry* ent;

	int temp_socket_descriptor;

	printf("DISK DRIVER START...\n");
	while(1) {
		unsigned int sin_size=sizeof(struct sockaddr_in);
	  	temp_socket_descriptor = accept(socket_descriptor,(struct sockaddr *)&pin,  &sin_size);

		if(temp_socket_descriptor == -1)
		{
			perror("Accept Failed");
			continue;
		}
		//printf("Accept Success\n");
		if((ent = (struct list_entry*)malloc(sizeof(struct list_entry)))== NULL)
		{
			perror("Malloc Failed");
			close(temp_socket_descriptor);
			continue;
		}
		bzero(ent, sizeof(struct list_entry));
		ent->skt = temp_socket_descriptor;
		if(pthread_create(&pid,NULL,serve,ent))
		{
			perror("Create Thread Failed");
			close(temp_socket_descriptor);
			free(ent);
			continue;
		}
		pthread_detach(pid);
	 	//close(temp_socket_descriptor);
	}

}

void* serve(void* arg)
{
	struct list_entry* ent = (struct list_entry*)arg;
	struct sbpfs_head head;
	char* rec_buf;
	u64_t rec_len;
	if(sbp_recv(ent->skt,&rec_buf, & rec_len) == -1)
	{
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
	if((method = get_head_entry_value(&head,METHOD))== NULL)
	{
		printf("get head entry failed\n");
		goto err_exit2;
	}
	if(strcmp(method,"READ") == 0)
	{
		ent->req = 1;
	}else if(strcmp(method,"WRITE") == 0)
	{
		ent->req = 2;
	}else
	{
		printf("invalid request\n");
		goto err_exit2;
	}
	char* block_num_s;
	char* offset_s;
	char* length_s;
	char* content_len_s;

	if((block_num_s = get_head_entry_value(&head,"Arg0"))== NULL)
	{
		printf("get head entry Arg0 failed\n");
		goto err_exit2;
	}
	if((offset_s = get_head_entry_value(&head,"Arg1"))== NULL)
	{
		printf("get head entry Arg1 failed\n");
		goto err_exit2;
	}
	if((length_s = get_head_entry_value(&head,"Arg2"))== NULL)
	{
		printf("get head entry Arg2 failed\n");
		goto err_exit2;
	}
	if((content_len_s = get_head_entry_value(&head,"Content-Length"))== NULL)
	{
		printf("get head entry Content-Length failed\n");
		goto err_exit2;
	}

	ent->block_num = atoll(block_num_s);
	ent->offset = atoi(offset_s);
	ent->length = atoi(length_s);
	if(ent->req == 2)
	{
		ent->data_length = atoll(content_len_s);
		ent->data = rec_buf + head.head_len;
	}
	send_err(ent->skt,"a","b");
	//process_req(ent);

err_exit2:
	free_head(&head);
err_exit:
	free(rec_buf);


	close(ent->skt);
	free(ent);
	return NULL;

}
void send_err(int skt,char* err_type,char* err_detail)
{
	struct sbpfs_head head;
	char* data;
	int data_len =0;
	head.data = NULL;
	head.title = REQUEST_ERR;
	head.entry_num = 0;
	mkent(head,ERR_TYPE,err_type);
	mkent(head,ERR_DETAIL,err_detail);
	mkent(head,CONTENT_LEN,"0");
	if (make_head(&data, &data_len, &head) == -1) {
		return;
	}
	if(sbp_send(skt, data, data_len) < 0){
		printf("send err failed\n");
		sbp_perror("a");
		perror("a");
		goto err_exit;
	}
	free(data);
	return;

err_exit:
	free(data);
	return;
}
void process_req(struct list_entry * ent)
{


}

