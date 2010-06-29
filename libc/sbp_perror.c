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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void free_err();
static struct sbp_err * init_err();
static void free_err_entry(struct sbp_err * err_ent);
static void print_errent(struct sbp_err *errent);
static void clear_err();
void sbp_perror(char* s){
	if(s != NULL && strlen(s))
		printf("%s\n",s);
	while(err_trace != NULL)
	{
		print_errent(err_trace);
		err_trace = err_trace -> next;
	}
	clear_err();

}
static void print_errent(struct sbp_err *errent)
{
	if(!(errent->type == NULL)){
		printf("Error Type  : %s\n",errent->type);
	}
	if(!(errent->detail == NULL)){
		printf("Error Detail: %s\n",errent->detail);
	}
	if(!(errent->file == NULL)&&!(errent->func == NULL)){
		printf("Error Position: FILE: %s LINE: %d FUNC: %s\n",errent->file,errent->line,errent->func);
	}
}
void sbp_seterr(char* type, char* detail,const char* file, int line,const char* func){
	struct sbp_err* new_err = init_err();
	if(new_err == NULL) return;
	new_err -> next = err_trace;
	err_trace = new_err;
	int typelen = strlen(type);
	int detaillen = strlen(detail);
	int filelen = strlen(file);
	int funclen = strlen(func);
	if(((new_err->type = (char*)malloc(typelen+1)) == NULL)||((new_err->detail = (char*)malloc(detaillen+1)) == NULL)||((new_err->file = (char*)malloc(filelen+1)) == NULL)||((new_err->func = (char*)malloc(funclen+1)) == NULL))
	{
		printf("Fatel Error ! sbp_seterr malloc failed\n");
		free_err();
		return;
	}
	new_err -> line = line;
	memcpy(new_err -> type, type, typelen);
	memcpy(new_err -> detail, detail, detaillen);
	memcpy(new_err -> file, file, filelen);
	memcpy(new_err -> func, func, funclen);
	new_err -> type[typelen] = 0;
	new_err -> detail[detaillen] = 0;
	new_err -> file[filelen] = 0;
	new_err -> func[funclen] = 0;
}
void sbp_update_err(struct sbpfs_head* head){
	char* type = NULL;
	char* detail = NULL;
	int i;
	//printf("update_err called \n");
	//printf("head entry_num : %d \n",head->entry_num);
	for(i = 0; i < head->entry_num ; i++)
	{
		if(strcmp(head->entrys[i].name,ERR_TYPE) == 0){
			type = head->entrys[i].value;
		}else if(strcmp(head->entrys[i].name,ERR_DETAIL) == 0){
			detail = head->entrys[i].value;
		}
		if((type!=NULL)&&(detail))
		{
			//printf("update_err found entry\n");
			seterr(type,detail);
			//printf("update_err done\n");
			return;
		}
	}
	//printf("update_err do nothing\n");
}
static struct sbp_err * init_err()
{
	struct sbp_err* ret = NULL;
	if((ret = (struct sbp_err*)malloc(sizeof(struct sbp_err))) == NULL)
	{
		printf("Fatal error : init err can not malloc\n");
		return ret;
	}
	bzero(ret,sizeof(struct sbp_err));
	return ret;
}
static void free_err()
{
	struct sbp_err* tmp;
	if(!(err_trace == NULL))
	{
		tmp = err_trace;
		err_trace = tmp -> next;
		free_err_entry(tmp);
	}
	return;
}
static void clear_err()
{
	while(err_trace!=NULL){
		free_err();
	}
}
static void free_err_entry(struct sbp_err * err_ent)
{
	if(!(err_ent ==NULL)){

		if(!(err_ent->detail == NULL))
		{
			free(err_ent->detail);
		}
		if(!(err_ent->file == NULL))
		{
			free(err_ent->file);
		}
		if(!(err_ent->type == NULL))
		{
			free(err_ent->type);
		}
		if(!(err_ent->func == NULL))
		{
			free(err_ent->func);
		}
		free(err_ent);

	}
	return;

}
