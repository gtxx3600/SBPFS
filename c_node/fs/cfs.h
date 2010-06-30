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

#ifndef C_NODE_FS_CFS_H
#define C_NODE_FS_CFS_H

#include "type.h"

#define MAX_FILENAME 256
#define MAX_FSNAME 16
#define NR_COPY 3
#define BLOCKSIZE (16*1024*1024)

/*
 * FILE TYPE
 */
#define S_NFD ~0
#define S_DIR 0x1
#define S_FIL 0x2

/*
 * FILE FLAGS
 */
#define S_IRD 0x8
#define S_IWR 0x4
#define S_ORD 0x2
#define S_OWR 0x1

#define S_IRW (S_IRD|S_IWR)
#define S_ORW (S_ORD|S_OWR)
#define S_ALL (S_IRW|S_ORW)

#define INITMODE S_IRW|S_ORD

/*
 * OPEN FLAGS
 */
#define SBP_O_RDONLY 0x01
#define SBP_O_WRONLY 0x02
#define SBP_O_RDWR 0x04
#define SBP_O_CREAT 0x08
#define SBP_O_APPEND 0x10

/*
 * FS
 */
#define CFSNAME "cfs"
#define BYTSPERSEC 512

/*
 * FAT
 */
#define FREESEC 0
#define ENDSEC ~0

typedef struct sb {
	char fstype[MAX_FSNAME];
	u32_t bytsPerSec;
	u32_t totSecNum;
	u32_t fatSecNum;
	u32_t fatNum;
	u64_t maxBlockNum;
} sb_t;

typedef struct file_entry {
	u64_t name;
	u32_t fb;
	u8_t type;
	u8_t reserve[3];
} file_entry_t;

typedef u32_t  fat_t;

typedef struct dtree {
	sb_t *sb;
	fat_t *fat_p;
	unsigned int dt_size;
	unsigned int in_size;
	unsigned int dt_max;
	unsigned int in_max;
	void *data;
	void *dt_buf;
	void *in_buf;
} dtree_t;

typedef struct file_meta {
	u32_t uid;
	u32_t count;
	u64_t flen;
	u64_t c_time;
	u64_t r_time;
	u64_t w_time;
	u64_t strbase;
	u64_t strlen;
	u32_t blocks;
	u8_t flags;
	u8_t reserve[3];
} file_meta_t;

#define EXIST 0x1

typedef struct file_block {
	u64_t block_num;
	u32_t offset;
	u32_t length;
	u32_t ip[NR_COPY];
	u8_t flags;
	u8_t reserve[3];
} file_block_t;

typedef struct sendblock {
	u64_t block_num;
	u32_t offset;
	u32_t length;
	u32_t ip[NR_COPY];
	u8_t reserver[4];
} sendblock_t;

typedef struct c_stat {
	u64_t size;
	u64_t atime;
	u64_t mtime;
	u64_t ctime;
	u32_t owner;
	u8_t mode;
	u8_t reserver[27];
} stat_t;

int c_getblocknum(dtree_t *dt, char *path);
int c_getblocks(dtree_t *dt, char *path, u64_t *l);

int c_stat(dtree_t *dt, char *path, u32_t uid, stat_t *st);
int c_open(dtree_t *dt, char *path, u8_t oflags, u8_t mode, u32_t uid);
int c_read(dtree_t *dt, u32_t fb, u64_t offset, u64_t length, u32_t uid,
		sendblock_t *blocks);
int c_write(dtree_t *dt, u32_t fb, u64_t offset, u64_t length, u32_t uid,
		u32_t *ips, int iplen, sendblock_t *blocks);
int c_opendir(dtree_t *dt, char *path, u32_t uid, u64_t *entnum);
int c_readdir(dtree_t *dt, u32_t fb, u64_t entnum, u32_t uid,
		char *name, u8_t *type);
int c_move(dtree_t *dt, char *srcpath, char *dstpath, u32_t uid);
int c_remove(dtree_t *dt, char *path, u32_t uid);
int c_chown(dtree_t *dt, char *path, u32_t newuid, u32_t uid);
int c_chmod(dtree_t *dt, char *path, u8_t mode, u32_t uid);
int c_mkdir(dtree_t *dt, char *path, u32_t uid);
int c_rmdir(dtree_t *dt, char *path, u32_t uid);
dtree_t *open_disk(char *dt_name, char *in_name);
int save_disk(dtree_t *dt, char *dt_name, char *in_name);
void close_disk(dtree_t *p);

#endif
