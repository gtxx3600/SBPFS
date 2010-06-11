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

void sbp_perror(char* s){
	printf("%s",s);
	if(!(sbp_err_type == NULL)){
		printf("Error Type  : %s\n",sbp_err_type);
	}
	if(!(sbp_err_detail == NULL)){
		printf("Error Detail: %s\n",sbp_err_detail);
	}
}
void sbp_seterr(char* type, char* detail){
	free_err();
	int typelen = strlen(type);
	int detaillen = strlen(detail);
	if(((sbp_err_type = (char*)malloc(typelen+1)) == NULL)||((sbp_err_detail = (char*)malloc(typelen+1)) == NULL))
	{
		printf("Fatel Error ! sbp_seterr malloc failed\n");
		free_err();
	}
	memcpy(sbp_err_type, type, typelen);
	memcpy(sbp_err_detail, detail, detaillen);
	sbp_err_type[typelen] = 0;
	sbp_err_detail[detaillen] = 0;


}
void sbp_update_err(struct sbpfs_head* head){
	char* type = NULL;
	char* detail = NULL;
	int i;
	for(i = 0; i < head->entry_num ; i++)
	{
		if(strcmp(head->entrys[i].name,ERR_TYPE) == 0){
			type = head->entrys[i].value;
		}else if(strcmp(head->entrys[i].name,ERR_DETAIL) == 0){
			detail = head->entrys[i].value;
		}
		if(type&&detail)
		{
			sbp_seterr(type,detail);
			return;
		}
	}
}
static void free_err()
{
	if(!(sbp_err_type == NULL))
	{
		free(sbp_err_type);
	}
	if(!(sbp_err_detail == NULL))
	{
		free(sbp_err_detail);
	}
}
