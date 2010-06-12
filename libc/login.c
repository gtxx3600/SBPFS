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
#include "lib.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

s32_t sbp_login(char* username, char* password)
{
	if( strlen(username) > MAX_USERNAME_LEN || strlen(password) > MAX_PASSWORD_LEN )
	{
		return -1;
	}
	strcpy(sbp_user, username);
	strcpy(sbp_pass, password);
	return 0;
}

s32_t sbp_getusername(char** username)
{
	if((*username = (char*)malloc(MAX_USERNAME_LEN + 1)) == NULL)
	{
		perror("sbp_getusername malloc error");
		return -1;
	}
	strcpy(*username, sbp_user);
	return 0;
}
s32_t sbp_getpassword(char** password)
{
	if((*password = (char*)malloc(MAX_PASSWORD_LEN + 1)) == NULL)
	{
		perror("sbp_getpasswrod malloc error");
		return -1;
	}
	strcpy(*password, sbp_pass);
	return 0;
}
s32_t sbp_getUandP(char** username,char** password)
{
	if(sbp_getusername(username) == -1)
	{
		return -1;
	}
	if(sbp_getpassword(password) == -1)
	{
		free(*username);
		return -1;
	}
	return 0;
}
s32_t sbp_sethost(char* hostname)
{
	int len = strlen(hostname);
	if((len == 0)||(len > MAX_HOSTNAME_LEN))
	{
		printf("Invalid hostname\n");
		return -1;
	}
	strcpy(sbp_host, hostname);
	return 0;
}
