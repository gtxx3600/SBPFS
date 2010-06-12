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

#define DEBUG
#ifdef DEBUG
#include <stdio.h>
void dump_meta(file_meta_t *fm)
{
	printf("uid=%u\n", fm->uid);
	printf("count=%u\n", fm->count);
	printf("flen=%llx\n", fm->flen);
	printf("strbase=%llx\n", fm->strbase);
	printf("strlen=%llx\n", fm->strlen);
	printf("blocks=%x\n", fm->blocks);
}
#endif

#define INIT_MAX (16*1024)

static void reset_fat(u32_t fb, fat_t *fat);
static void do_writedir(dtree_t *dt, u32_t fb, void *data, u64_t size);
static void do_readdir(dtree_t *dt, u32_t fb, void *ep);
static int c_find_entry(dtree_t *dt, file_entry_t* base,
		char *path, u32_t uid, file_entry_t *ret);
static int c_lookup(dtree_t *dt, char *path, u32_t uid, file_entry_t *ret);
static int extend(dtree_t *dt, u64_t newsize);
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

int c_mkdir(dtree_t *dt, char *path, u32_t uid)
{
	char *dirname;
	int len = strlen(path), i, last, ret;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	file_entry_t parent, *newdir;
	file_meta_t pmeta, *meta;
	u64_t newsize;
	void *pdata, *buf;

	for (i = 0; i < len; i++) {
		if (path[i] == '/') {
			if (i != len-1)
				last = i;
			else
				path[i] = 0;
		}
	}
	path[last] = 0;
	dirname = &path[last+1];

//	printf("name=%s\n", dirname);
	ret = c_lookup(dt, path, uid, &parent);
//	printf("here: %x\n", parent.name);
	if (ret)
		return ret;

	memcpy(&pmeta, dt->data + parent.fb*bytsPerSec,
			sizeof(file_meta_t));
//	dump_meta(&pmeta);
	pdata = malloc(pmeta.blocks * bytsPerSec);
	if (!pdata)
		return -1;

	newsize = pmeta.flen + strlen(dirname) + 1 + sizeof(file_entry_t);
	buf = malloc(newsize);
	if (!buf) {
		free(pdata);
		return -1;
	}

	do_readdir(dt, parent.fb, pdata);
	memcpy(buf, pdata, sizeof(file_meta_t)+pmeta.strbase);
	meta = buf;
//	dump_meta(meta);
	newdir = (file_entry_t *)(buf + sizeof(file_meta_t) + pmeta.strbase);
	newdir->name = pmeta.strlen;
	newdir->type = S_DIR;
	initdir(uid, newdir, &parent, dt->sb, dt->fat_p, dt->data);
	meta->strbase += sizeof(file_entry_t);
	memcpy(buf+sizeof(file_meta_t)+meta->strbase,
			pdata+sizeof(file_meta_t)+pmeta.strbase, pmeta.strlen);
	memcpy(buf+sizeof(file_meta_t)+meta->strbase+pmeta.strlen,
			dirname, strlen(dirname)+1);
	meta->strlen += strlen(dirname)+1;
	meta->flen = newsize;
	meta->blocks = (newsize + bytsPerSec - 1) / bytsPerSec;
//	dump_meta(meta);
	do_writedir(dt, parent.fb, buf, newsize);

	free(pdata);
	free(buf);

	return 0;
}

static void reset_fat(u32_t fb, fat_t *fat)
{
	u32_t fat_num = fb, next;
	while (fat_num != ENDSEC) {
		next = fat[fat_num];
		fat[fat_num] = FREESEC;
		fat_num = next;
	}
}

static void do_writedir(dtree_t *dt, u32_t fb, void *data, u64_t size)
{
	file_meta_t *meta = data;
	u32_t i, bytsPerSec = dt->sb->bytsPerSec, fat_index = fb;
	reset_fat(fb, dt->fat_p);

	for (i = 0; i < meta->blocks;) {
		int len = (size < bytsPerSec) ? size : bytsPerSec;
		u32_t next;
		memcpy(dt->data+fat_index*bytsPerSec, data+i*bytsPerSec, len);
		i++;
		if (i >= meta->blocks) break;
		next = next_free_fat(fat_index, dt->fat_p, dt->sb->fatNum);
		dt->fat_p[fat_index] = next;
		fat_index = next;
		size -= bytsPerSec;
	}
	dt->fat_p[fat_index] = ENDSEC;
}

static void do_readdir(dtree_t *dt, u32_t fb, void *data)
{
	u32_t fat_num = fb;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	u32_t i = 0;
	fat_t *fat = dt->fat_p;
	while (fat_num != ENDSEC) {
		memcpy(data+i*bytsPerSec, dt->data+fat_num*bytsPerSec, bytsPerSec);
		fat_num = fat[fat_num];
		i++;
	}
}

static int c_find_entry(dtree_t *dt, file_entry_t* base,
		char *path, u32_t uid, file_entry_t *ret)
{
	file_meta_t meta;
	file_entry_t *ep, *p;
	void *buf;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	u32_t fat_num = base->fb;
	char *next_base = path, *cp;

	memcpy(&meta, dt->data + fat_num*bytsPerSec,
			sizeof(file_meta_t));
	buf = malloc(meta.blocks * bytsPerSec);
	if (!buf)
		return -1;

	ep = buf + sizeof(file_meta_t);

	do_readdir(dt, base->fb, buf);

	for (cp = path; *cp != 0; cp++) {
		if (*cp == '/') {
			*cp = 0;
			path = cp + 1;
			break;
		}
	}

//	printf("next_base: %s\n", next_base);
	for (p = ep; (u64_t)(long)p != (u64_t)(long)ep + meta.strbase; p++) {
		char *s = (char *)((char *)ep + meta.strbase + p->name);
//		printf("filename: %s\n", s);
		if (!strcmp(s, next_base))
			break;
	}
	if ((u64_t)(long)p == (u64_t)(long)ep + meta.strbase) {
//		printf("fail\n");
		return 1;
	}
//	printf("success\n");

	if (next_base == path) {
		memcpy(ret, p, sizeof(file_entry_t));
	} else {
		c_find_entry(dt, p, path, uid, ret);
	}

	free(buf);

	return 0;
}

static int c_lookup(dtree_t *dt, char *path, u32_t uid, file_entry_t *ret)
{
	file_entry_t root_entry;
	if (path[0] == '/') {
		path++;
	}
	if (!path[0]) {
		memcpy(ret, dt->data, sizeof(file_entry_t));
		return 0;
	}
	memcpy(&root_entry, dt->data, sizeof(file_entry_t));
	return c_find_entry(dt, &root_entry, path, uid, ret);
}

static int extend(dtree_t *dt, u64_t newsize)
{
	void *buf;
	sb_t *sb;
	fat_t *fat;
	void *data;
	u64_t olddata_len;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	u32_t totSecNum = newsize / bytsPerSec;
	u32_t fatSize = totSecNum * sizeof(fat_t);
	u32_t fatSecNum = (fatSize + bytsPerSec -1) / bytsPerSec;

	if (dt->dt_size >= newsize) {
		return 1;
	}

	buf = malloc(newsize);
	if (!buf)
		return -1;

	sb = buf;
	memcpy(sb, dt->sb, sizeof(sb_t));
	sb->totSecNum = totSecNum;
	sb->fatSecNum = fatSecNum;
	sb->fatNum = (newsize-fatSize-bytsPerSec) / bytsPerSec;

	fat = buf + bytsPerSec;
	memset(fat, 0, fatSecNum * bytsPerSec);
	memcpy(fat, dt->fat_p, dt->sb->fatSecNum*bytsPerSec);

	data = (void *)fat + fatSecNum*bytsPerSec;
	olddata_len = dt->dt_size - bytsPerSec - dt->sb->fatSecNum*bytsPerSec;
	memcpy(data, dt->data, olddata_len);

//	printf("dt->data=%x\n", dt->data);
//	printf("buf=%x\n", dt->dt_buf);
//	printf("data=%x\n", data-buf);
//	printf("fatSecNum=%x\n", fatSecNum);

	free(dt->dt_buf);
	dt->dt_buf = buf;
	dt->sb = sb;
	dt->fat_p = fat;
	dt->data = data;
	dt->dt_size = newsize;
	dt->dt_max = newsize;

	return 0;
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
	spool = (char *)&entries[2];

	count = (size + sb->bytsPerSec - 1) / sb->bytsPerSec;

	meta->flags = INITMODE;
	meta->c_time = tm;
	meta->r_time = tm;
	meta->w_time = tm;
	meta->uid = uid;
	meta->count = 1;
	meta->flen = size;
	meta->strbase = (u64_t)((unsigned long)spool - (unsigned long)entries);
	meta->strlen = 5;
	meta->blocks = count;

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

	entries[0].name = 0;
	entries[0].type = S_DIR;
	entries[0].fb = file_entry->fb;
	meta->count++;

	entries[1].name = 2;
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
