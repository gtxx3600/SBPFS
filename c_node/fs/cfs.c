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

#define INIT_MAX (32*1024)

#define do_writedir(dt, fb, data, size) do_writefile(dt, fb, data, size)
#define do_readdir(dt, fb, ep) do_readfile(dt, fb, ep)

static void reset_fat(u32_t fb, fat_t *fat);
static int do_remove_entry(void *pdata, file_entry_t *entry);
static void do_writefile(dtree_t *dt, u32_t fb, void *data, u64_t size);
static void do_readfile(dtree_t *dt, u32_t fb, void *ep);
static file_entry_t *find_entry_by_name(void *pdata, char *name);
static int c_find_entry(dtree_t *dt, file_entry_t* base,
		char *path, u32_t uid, file_entry_t *ret);
static int c_lookup(dtree_t *dt, char *path, u32_t uid, file_entry_t *ret);
static int extend(dtree_t *dt, u64_t newsize);
static void do_mkcfs(dtree_t *dt, unsigned int size, unsigned int bytsPerSec);
static int initdir(dtree_t *dt, u32_t uid,
		file_entry_t *file_entry, file_entry_t *parent);
static u32_t next_free_fat(u32_t base, fat_t *fat, u32_t fat_num);
static void *load_file(char *filename, unsigned int *size, unsigned *max);
static int save_file(void *buf, unsigned int size, char *filename);

#define R 0x2
#define W 0x1
static int permission_check(u32_t uid, file_meta_t *meta, u8_t op)
{
	u32_t oid = meta->uid;
	u8_t fmode = meta->flags & S_ALL;
//	printf("fmode=%x\n", fmode);
	if (!uid) uid = oid;
	if (uid == oid && op == R) {
		return fmode & S_IRD;
	} else if (uid == oid && op == W) {
		return fmode & S_IWR;
	} else if (uid != oid && op == R) {
		return fmode & S_ORD;
	} else if (uid != oid && op == W) {
		return fmode & S_OWR;
	} else {
		return 0;
	}
}

static int own_check(u32_t uid, u32_t oid)
{
	if (!uid) return 1;
	return uid == oid;
}

dtree_t *open_disk(char *dt_name, char *in_name)
{
	dtree_t *dt;
	unsigned int dt_size, in_size, dt_max, in_max;
	int bytsPerSec, needmkfs = 0;

	dt = (dtree_t *)malloc(sizeof(dtree_t));
	memset(dt, 0, sizeof(dtree_t));

	dt->dt_buf = load_file(dt_name, &dt_size, &dt_max);
	if (!dt->dt_buf) {
		dt->dt_buf = malloc(INIT_MAX);
		if (!dt->dt_buf)
			goto out_error;
		dt_size = dt_max = INIT_MAX;
		needmkfs = 1;
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
	bytsPerSec = dt->sb->bytsPerSec = BYTSPERSEC;
	dt->fat_p = (void *)dt->sb + bytsPerSec;
	dt->data = (void *)dt->fat_p + bytsPerSec * dt->sb->fatSecNum;

	if (needmkfs)
		do_mkcfs(dt, dt_max, BYTSPERSEC);

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

static char *get_filename_from_path(char *path)
{
	int i, last = -1, len = strlen(path);

	for (i = 0; i < len; i++) {
		if (path[i] == '/') {
			if (i != len-1)
				last = i;
			else
				path[i] = 0;
		}
	}

	if (last == -1)
		return NULL;

	path[last] = 0;
	return &path[last+1];
}

int c_chown(dtree_t *dt, char *path, u32_t newuid, u32_t uid)
{
	int ret;
	file_meta_t meta;
	file_entry_t entry;
	u32_t olduid;

	ret = c_lookup(dt, path, uid, &entry);
	if (ret)
		return ret;

	memcpy(&meta, dt->data + entry.fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	olduid = meta.uid;
	if (!own_check(uid, olduid)) {
		return 4;
	}
	if (olduid == newuid) {
		return 0;
	}
	if (uid) {
		return 4;
	}

	meta.uid = newuid;
	memcpy(dt->data + entry.fb*dt->sb->bytsPerSec, &meta,
			sizeof(file_meta_t));
	return 0;
}

int c_chmod(dtree_t *dt, char *path, u8_t mode, u32_t uid)
{
	int ret;
	file_meta_t meta;
	file_entry_t entry;

	ret = c_lookup(dt, path, uid, &entry);
	if (ret)
		return ret;

	memcpy(&meta, dt->data + entry.fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	if (!own_check(uid, meta.uid)) {
		return 4;
	}
	meta.flags &= ~S_ALL;
	meta.flags |= mode;
	memcpy(dt->data + entry.fb*dt->sb->bytsPerSec, &meta,
			sizeof(file_meta_t));
	return 0;
}

int c_mkdir(dtree_t *dt, char *path, u32_t uid)
{
	char *dirname;
	int ret;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	file_entry_t parent, *newdir;
	file_meta_t pmeta, *meta;
	u64_t newsize;
	void *pdata, *buf;

	dirname = get_filename_from_path(path);
	if (!dirname) {
		return 1;
	}

//	printf("name=%s\n", dirname);
	ret = c_lookup(dt, path, uid, &parent);
//	printf("here: %x\n", parent.name);
	if (ret)
		return ret;
	if (parent.type != S_DIR)
		return 2;

	memcpy(&pmeta, dt->data + parent.fb*bytsPerSec,
			sizeof(file_meta_t));
	if (!permission_check(uid, &pmeta, W)) {
		return 4;
	}
//	dump_meta(&pmeta);
	pdata = malloc(pmeta.blocks * bytsPerSec);
	if (!pdata)
		return -1;
	do_readdir(dt, parent.fb, pdata);
	if (find_entry_by_name(pdata, dirname)) {
		return 3;
	}

	newsize = pmeta.flen + strlen(dirname) + 1 + sizeof(file_entry_t);
	buf = malloc(newsize);
	if (!buf) {
		free(pdata);
		return -1;
	}
	memset(buf, 0, newsize);

	memcpy(buf, pdata, sizeof(file_meta_t)+pmeta.strbase);
	meta = buf;
//	dump_meta(meta);
	newdir = (file_entry_t *)(buf + sizeof(file_meta_t) + pmeta.strbase);
	newdir->name = pmeta.strlen;
	newdir->type = S_DIR;
	initdir(dt, uid, newdir, &parent);
	meta->strbase += sizeof(file_entry_t);
	memcpy(buf+sizeof(file_meta_t)+meta->strbase,
			pdata+sizeof(file_meta_t)+pmeta.strbase, pmeta.strlen);
	memcpy(buf+sizeof(file_meta_t)+meta->strbase+pmeta.strlen,
			dirname, strlen(dirname)+1);
	meta->strlen += strlen(dirname)+1;
	meta->flen = newsize;
	meta->blocks = (newsize + bytsPerSec - 1) / bytsPerSec;
	meta->count++;
//	dump_meta(meta);
	do_writedir(dt, parent.fb, buf, newsize);

	free(pdata);
	free(buf);

	return 0;
}

static int is_dir_empty(file_entry_t *entry, file_meta_t *meta)
{
	if (meta->strbase > 2 * sizeof(file_entry_t))
		return 0;
	return 1;
}

int c_rmdir(dtree_t *dt, char *path, u32_t uid)
{
	char *dirname;
	int ret;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	file_entry_t parent, *rmdirent;
	file_meta_t pmeta, meta, *real_pmeta;
	void *pdata, *buf;

	dirname = get_filename_from_path(path);
	if (!dirname) {
		return 1;
	}

	ret = c_lookup(dt, path, uid, &parent);
	if (ret)
		return ret;
	if (parent.type != S_DIR)
		return 1;

	memcpy(&pmeta, dt->data + parent.fb*bytsPerSec,
			sizeof(file_meta_t));
	if (!permission_check(uid, &pmeta, W)) {
		return 4;
	}
	pdata = malloc(pmeta.blocks * bytsPerSec);
	if (!pdata)
		return -1;

	do_readdir(dt, parent.fb, pdata);
	rmdirent = find_entry_by_name(pdata, dirname);
	if (!rmdirent) {
		free(pdata);
		return 1; // directory not found
	}
	if (rmdirent->type != S_DIR) {
		free(pdata);
		return 2; // not a directory
	}

	memcpy(&meta, dt->data + rmdirent->fb*bytsPerSec,
			sizeof(file_meta_t));
	buf = malloc(meta.blocks * bytsPerSec);
	if (!buf)
		return -1;
	do_readdir(dt, rmdirent->fb, buf);
	if (!is_dir_empty(rmdirent, &meta)) {
		free(pdata);
		return 3; // directory is not empty
	}

	real_pmeta = pdata;
	real_pmeta->count--;
	reset_fat(rmdirent->fb, dt->fat_p);
	ret = do_remove_entry(pdata, rmdirent);
	if (ret) {
		free(pdata);
		free(buf);
		return ret;
	}
	do_writedir(dt, parent.fb, pdata, real_pmeta->flen);

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

static int do_remove_entry(void *pdata, file_entry_t *entry)
{
	void *tmp, *p;
	file_meta_t *meta = (file_meta_t *)pdata;
	u64_t tmpsize, now = time(NULL);
	p = (void *)entry + sizeof(file_entry_t);
	tmpsize = meta->flen - (u64_t)((long)entry - (long)pdata);
	tmp = malloc(tmpsize);
	if (!tmp)
		return -1;
	memcpy(tmp, p, tmpsize);
	memcpy(entry, tmp, tmpsize);
	free(tmp);
	meta->strbase -= sizeof(file_entry_t);
	meta->w_time = now;
	meta->flen -= sizeof(file_entry_t);
	return 0;
}

static void do_writefile(dtree_t *dt, u32_t fb, void *data, u64_t size)
{
	file_meta_t *meta = data;
	u32_t i, bytsPerSec = dt->sb->bytsPerSec, fat_index = fb;
	reset_fat(fb, dt->fat_p);

	for (i = 0; i < meta->blocks;) {
		int len = (size < bytsPerSec) ? size : bytsPerSec;
		u32_t next;
		memcpy(dt->data+fat_index*bytsPerSec, data+i*bytsPerSec, len);
		dt->fat_p[fat_index] = ENDSEC;
		i++;
		if (i >= meta->blocks) break;
		do {
			next = next_free_fat(fat_index, dt->fat_p, dt->sb->fatNum);
			if (next != -1) break;
			extend(dt, dt->dt_size * 2);
		} while (1);
//		printf("    next=%d\n", next);
		dt->fat_p[fat_index] = next;
		fat_index = next;
		size -= bytsPerSec;
	}
}

static void do_readfile(dtree_t *dt, u32_t fb, void *data)
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

static file_entry_t *find_entry_by_name(void *pdata, char *name)
{
	file_entry_t *ep, *p;
	file_meta_t *meta = pdata;

	ep = pdata + sizeof(file_meta_t);
	for (p = ep; (u64_t)(long)p != (u64_t)(long)ep + meta->strbase; p++) {
		char *s = (char *)ep + meta->strbase + p->name;
		if (!strcmp(s, name))
			return p;
	}

	return NULL;
}

static int c_find_entry(dtree_t *dt, file_entry_t* base,
		char *path, u32_t uid, file_entry_t *ret)
{
	file_meta_t meta;
	file_entry_t *p;
	void *buf;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	u32_t fat_num = base->fb;
	char *next_base = path, *cp;
	int retval = 0;

	memcpy(&meta, dt->data + fat_num*bytsPerSec,
			sizeof(file_meta_t));
	buf = malloc(meta.blocks * bytsPerSec);
	if (!buf)
		return -1;

	do_readdir(dt, base->fb, buf);

	for (cp = path; *cp != 0; cp++) {
		if (*cp == '/') {
			*cp = 0;
			path = cp + 1;
			break;
		}
	}

	p = find_entry_by_name(buf, next_base);
	if (!p) {
		return 1;
	}

	if (next_base == path) {
		memcpy(ret, p, sizeof(file_entry_t));
	} else if (p->type == S_DIR) {
		retval = c_find_entry(dt, p, path, uid, ret);
	} else {
		retval = 1;
	}

	free(buf);

	return retval;
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
	memset(buf, 0, bytsPerSec);

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

static void do_mkcfs(dtree_t *dt, unsigned int size, unsigned int bytsPerSec)
{
	void *buf = dt->dt_buf;
	file_entry_t root_entry;
	sb_t *sb = buf;
	fat_t *fat;
	u32_t totSecNum = size / bytsPerSec;
	u32_t fatSize = totSecNum * sizeof(fat_t);
	u32_t fatSecNum = (fatSize + bytsPerSec -1) / bytsPerSec;

	dt->data = (void *)dt->fat_p + bytsPerSec * fatSecNum;

	memset(buf, 0, sizeof(sb_t));
	strcpy(sb->fstype, CFSNAME);
	sb->bytsPerSec = bytsPerSec;
	sb->totSecNum = totSecNum;
	sb->fatSecNum = fatSecNum;
	sb->fatNum = (size-fatSize-bytsPerSec) / bytsPerSec;

	fat = buf+bytsPerSec;
	memset(fat, 0, fatSecNum*bytsPerSec);
	fat[0] = ENDSEC;
	//buf + bytsPerSec + fatSecNum*bytsPerSec;
	memset(&root_entry, 0, sizeof(file_entry_t));
	root_entry.name = 0;
	root_entry.type = S_DIR;
	initdir(dt, 0, &root_entry, &root_entry);
	memcpy(dt->data, &root_entry,
			sizeof(file_entry_t));
}

static int initdir(dtree_t *dt, u32_t uid,
		file_entry_t *file_entry, file_entry_t *parent)
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
	memset(meta, 0, size);

	do {
		fat_index = next_free_fat(0, dt->fat_p, dt->sb->fatNum);
//printf("next=%d\n", fat_index);
		if (fat_index != -1) break;
//printf("extend\n");
		extend(dt, dt->dt_size * 2);
	} while (1);

	file_entry->fb = fat_index;
	entries = (void *)meta + sizeof(file_meta_t);
	spool = (char *)&entries[2];

	count = (size + dt->sb->bytsPerSec - 1) / dt->sb->bytsPerSec;

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

//printf("here1: count=%d, fat_index=%d, bps=%d\n", count, fat_index, dt->sb->bytsPerSec);
	for (i = 0; i < count;) {
		int len = (size < dt->sb->bytsPerSec) ? size : dt->sb->bytsPerSec;
		u32_t next;
		memcpy(dt->data+fat_index*dt->sb->bytsPerSec,
				(void *)meta+i*dt->sb->bytsPerSec, len);
		i += 1;
//printf("here2: i=%d, count=%d\n",i,count);
		if (i >= count) break;
		do {
			next = next_free_fat(fat_index, dt->fat_p, dt->sb->fatNum);
			if (next != -1) break;
			extend(dt, dt->dt_size * 2);
		} while (1);
		dt->fat_p[fat_index] = next;
		fat_index = next;
		size -= dt->sb->bytsPerSec;
	}
//printf("fat=%d\n", fat_index);
	dt->fat_p[fat_index] = ENDSEC;
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
