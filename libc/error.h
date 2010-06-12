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

#ifndef __SBP_LIBC_ERROR_H_
#define __SBP_LIBC_ERROR_H_
#include "lib.h"
#define SOCKET_ERR 		"Socket Error"
#define MEM_ERR			"Memory Error"
#define SBPFS_HEAD_ERR	"SBPFS HEAD Error"

#define EST_SOCK		"Could not establish a socket connection"
#define INIT_SOCK		"Could not init a socket"
#define GET_HOSTNAME	"Could not get hostname"
#define TOO_MANY_ENTRY	"There is too many entrys"
#define HEAD_FLAG		"Could not find the end flag of HEAD"
#define LINE_SEPARATOR	"Could not find line separator"
#define COLON			"Could not find colon"
#define CONNECT			"Connect failed"
#define DATA_LEN		"Length of data not match"
#define MALLOC		"Malloc failed"
#define MEMCPY		"Memory copy failed"
#define MISSING_LEN		"Get missing length error"
#define SEND			"Could not send a specified length"
#define RECV			"Could not receive a specified length"
#define FIND_CONTENT_LEN	"Could not find 'Content-length'"
#define seterr(type,detail) sbp_seterr(type,detail,__FILE__,__LINE__)
void sbp_seterr(char* type, char* detail, char* file, int line);
void sbp_update_err(struct sbpfs_head* head);

struct sbp_err{
	char* 	type;
	char*  	detail;
	char*	file;
	int 	line;
	struct sbp_err * next;
};

#endif /* ERROR_H_ */
