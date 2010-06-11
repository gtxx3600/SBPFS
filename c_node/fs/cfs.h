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

#define MAX_FSNAME 16

#define S_DIR 0x10
#define S_IRD 0x8
#define S_IWR 0x4
#define S_ORD 0x2
#define S_OWR 0x1

#define S_IRW S_IRD|S_IWR
#define S_ORW S_ORD|S_IWR

typedef struct sb {
	char fstype[MAX_FSNAME];
	u32_t bytsPerSec;
	u32_t totSecNum;
	u32_t fatSecNum;
} sb_t;

typedef struct file_entry {
	u64_t name;
	u32_t uid;
	u16_t flags;
	u32_t fb;
	u64_t flen;
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

dtree_t *open_disk(char *dt_name, char *in_name);
int save_disk(dtree_t *dt, char *dt_name, char *in_name);
void close_disk(dtree_t *p);
void mkcfs(void *buf, unsigned int size, unsigned int bytsPerSec);

#endif
