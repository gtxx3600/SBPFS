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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cfs.h"
#include "type.h"

#define INIT_MAX (1024*1024)

static void do_mkcfs(void *buf, unsigned int size, unsigned int bytsPerSec);
static int initdir(u32_t uid,
		file_entry_t *file_entry, file_entry_t *parent,
		sb_t *sb, fat_t *fat, void *data);
static u32_t next_free_fat(u32_t base, fat_t *fat, u32_t fat_num);
static void *load_file(char *filename, unsigned int *size, unsigned *max);
static int save_file(void *buf, unsigned int size, char *filename);

dtree_t *open_disk(char *dt_name, char *in_name)
{
	dtree_t *dt;
	unsigned int dt_size, in_size, dt_max, in_max;
	int bytsPerSec;

	dt = (dtree_t *)malloc(sizeof(dtree_t));
	memset(dt, 0, sizeof(dtree_t));

	dt->dt_buf = load_file(dt_name, &dt_size, &dt_max);
	if (!dt->dt_buf) {
		dt->dt_buf = malloc(INIT_MAX);
		if (!dt->dt_buf)
			goto out_error;
		dt_size = dt_max = INIT_MAX;
		do_mkcfs(dt->dt_buf, dt_max, BYTSPERSEC);
	}

	dt->in_buf = load_file(in_name, &in_size, &in_max);
	if (!dt->in_buf) {
		dt->in_buf = malloc(INIT_MAX);
		if (!dt->in_buf)
			goto out_error;
		in_size = in_max = INIT_MAX;
	}

	dt->dt_size = dt_size;
	dt->in_size = in_size;
	dt->dt_max = dt_max;
	dt->in_max = in_max;

	dt->sb = dt->dt_buf;
	bytsPerSec = dt->sb->bytsPerSec;
	dt->fat_p = (void *)dt->sb + bytsPerSec;
	dt->data = (void *)dt->fat_p + bytsPerSec * dt->sb->fatSecNum;

//	save_disk(dt, dt_name, in_name);

	return dt;

out_error:
	close_disk(dt);
	return NULL;
}

int save_disk(dtree_t *dt, char *dt_name, char *in_name)
{
	int ret;

	if (dt) {
		ret = save_file(dt->dt_buf, dt->dt_size, dt_name);
		if (ret < 0)
			return -1;
		ret = save_file(dt->in_buf, dt->in_size, in_name);
		if (ret < 0)
			return -1;
		return 0;
	}

	return -1;
}

void close_disk(dtree_t *p)
{
	if (p) {
		if (p->dt_buf)
			free(p->dt_buf);
		if (p->in_buf)
			free(p->in_buf);
		free(p);
	}
}

static void do_mkcfs(void *buf, unsigned int size, unsigned int bytsPerSec)
{
	file_entry_t *root_entry;
	sb_t *sb = buf;
	fat_t *fat;
	u32_t totSecNum = size / bytsPerSec;
	u32_t fatSize = totSecNum * sizeof(fat_t);
	u32_t fatSecNum = (fatSize + bytsPerSec -1) / bytsPerSec;

	memset(buf, 0, sizeof(sb_t));
	strcpy(sb->fstype, CFSNAME);
	sb->bytsPerSec = bytsPerSec;
	sb->totSecNum = totSecNum;
	sb->fatSecNum = fatSecNum;
	sb->fatNum = (size-fatSize-bytsPerSec) / bytsPerSec;

	fat = buf+bytsPerSec;
	memset(fat, 0, fatSecNum*bytsPerSec);
	fat[0] = ENDSEC;
	root_entry = buf + bytsPerSec + fatSecNum*bytsPerSec;
	root_entry->name = 0;
	root_entry->type = S_DIR;
	initdir(0, root_entry, root_entry, sb, fat, (void *)root_entry);
}

static int initdir(u32_t uid,
		file_entry_t *file_entry, file_entry_t *parent,
		sb_t *sb, fat_t *fat, void *data)
{
	file_meta_t *meta;
	file_entry_t *entries;
	char *spool;
	u32_t count, fat_index, i;
	u64_t size = sizeof(file_meta_t) + 2*sizeof(file_entry_t) + 5;
	time_t tm = time(NULL);

	meta = malloc(size);
	if (!meta)
		return -1;

	fat_index = file_entry->fb = next_free_fat(0, fat, sb->fatNum);

	entries = (void *)meta + sizeof(file_meta_t);

	meta->flags = INITMODE;
	meta->c_time = tm;
	meta->r_time = tm;
	meta->w_time = tm;
	meta->uid = uid;
	meta->count = 1;
	meta->flen = size;
	meta->strlen = 5;

	spool = (char *)&entries[2];
	memcpy(spool, ".\0..\0", 5);
//	printf("spool=%p, entries=%p, meta=%p\n", spool, entries, meta);
//	printf("meta: %x, entry: %x\n", sizeof(file_meta_t), sizeof(file_entry_t));
//	printf("size=%x\n", size);
//	printf("0:%x  ", spool[0]);
//	printf("1:%x  ", spool[1]);
//	printf("2:%x  ", spool[2]);
//	printf("3:%x  ", spool[3]);
//	printf("4:%x  ", spool[4]);
//	printf("\n");

	entries[0].name = (u64_t)(unsigned long)spool - (u64_t)(unsigned long)entries;
	entries[0].type = S_DIR;
	entries[0].fb = file_entry->fb;
	meta->count++;

	entries[1].name = entries[0].name + 2;
	entries[1].type = S_DIR;
	entries[1].fb = parent->fb;
	meta->count++;

	count = (size + sb->bytsPerSec - 1) / sb->bytsPerSec;
//printf("here1: count=%d, fat_index=%d\n", count, fat_index);
	for (i = 0; i < count;) {
		int len = (size < sb->bytsPerSec) ? size : sb->bytsPerSec;
		u32_t next;
		memcpy(data+fat_index*sb->bytsPerSec,
				(void *)meta+i*sb->bytsPerSec, len);
		i += 1;
//printf("here2: i=%d, count=%d\n",i,count);
		if (i >= count) break;
		next = next_free_fat(fat_index, fat, sb->fatNum);
		fat[fat_index] = next;
		fat_index = next;
		size -= sb->bytsPerSec;
	}
	fat[fat_index] = ENDSEC;
//printf("free meta: fat_index=%d\n", fat_index);

	free(meta);
//printf("vvvv\n");
	return 0;
}

static u32_t next_free_fat(u32_t base, fat_t *fat, u32_t fat_num)
{
	u32_t i;
	for (i = base; i < fat_num; i++) {
		if (fat[i] == FREESEC)
			return i;
	}
	return -1;
}

static void *load_file(char *filename, unsigned int *size, unsigned int *max)
{
	void *buf;
	int fd, ret;

	fd = open(filename, O_RDONLY, 0);
	if (fd < 0) {
		return NULL;
	}

	*max = INIT_MAX;
	buf = malloc(*max);
	if (!buf)
		goto out_error;

	*size = 0;
	while ((ret = read(fd, buf+*size, *max-*size)) > 0) {
		*size += ret;
		if (*size == *max) {
			void *p;
			p = realloc(buf, *max *= 2);
			if (!p)
				goto out_error;
			buf = p;
		}
	}
	if (ret < 0)
		goto out_error;

	close(fd);
	return buf;

out_error:
	free(buf);
	close(fd);
	return NULL;
}

static int save_file(void *buf, unsigned int size, char *filename)
{
	int fd;
	int ret;

	fd = open(filename, O_WRONLY|O_CREAT, 0644);
	if (!fd)
		return -1;
	ret = write(fd, buf, size);
	if (ret != size)
		goto out_error;

	close(fd);
	return 0;

out_error:
	close(fd);
	return -1;
}
