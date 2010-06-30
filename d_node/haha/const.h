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

#ifndef __SBP_LIBC_CONST_H_
#define __SBP_LIBC_CONST_H_

#define REDUNDANCY				2

#define CNODE_SERVICE_PORT  	9000
#define DNODE_SERVICE_PORT  	9010
#define BUF_SIZE  				4096
#define CONTENT_LEN 			"Content-Length"
#define ARGC 					"Argc"
#define METHOD					"Method"
#define ARG(x)              	"Arg"##x
#define USER					"User"
#define PASS					"Password"
#define PROTOCOL				"SBPFS/1.0"
#define REQUEST_OK				"SBPFS/1.0 OK"
#define REQUEST_ERR				"SBPFS/1.0 ERROR"
#define ERR_TYPE				"Error-Type"
#define ERR_DETAIL				"Error-Detail"
#define AUTH_CODE				"Auth-code"
#define FILE_DESC				"File-desc"


#define ROOTPATH 				"/home/cxy/"
#define BLOCKLISTPATH 			"/home/cxy/blocklist"

#define HEADER_FLAG				"\r\n\r\n"
#define MAX_USERNAME_LEN		16
#define TRAN_USERNAME_LEN		16 + 7
#define MAX_PASSWORD_LEN		32
#define MAX_HOSTNAME_LEN		32
#define AUTH_CODE_LEN		32
#define MAX_ENTRY_IN_HEAD		32
#define MAX_FILE_OPEN			4096
#define MAX_PATH_LEN			4096
#define MAX_FILENAME_LEN		256

/* CLICK SIZE is 16MB (16*1024*1024)*/
#define CLICK_SIZE				16777216

typedef unsigned long long u64_t;
typedef long long s64_t;
typedef unsigned int u32_t;
typedef int s32_t;
typedef unsigned short u16_t;
typedef short s16_t;
typedef unsigned char u8_t;
typedef char s8_t;

#endif
