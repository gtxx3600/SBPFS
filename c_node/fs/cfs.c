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
static int initfile(dtree_t *dt, u32_t uid,
		file_entry_t *file_entry, file_entry_t *parent, u8_t mode);
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

static int gen_ip_index(int max) {
	static int i = 0;
	return (i++) % max;
}

static u64_t up_div(u64_t a, u64_t b)
{
	return (a+b-1) / b;
}

static void alloc_block(dtree_t *dt, u32_t *ips, int max,
		file_block_t *block) {
	u32_t ip;
	int i;
	memset(block, 0, sizeof(file_block_t));
	for (i = 0; i < NR_COPY; i++) {
		ip = ips[gen_ip_index(max)];
		if (i != 0 && ip == block->ip[0]) {
			break;
		}
		block->ip[i] = ip;
	}
	block->block_num = dt->sb->maxBlockNum++;
	block->offset = 0;
	block->flags |= EXIST;
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
	if (!dt)
		return NULL;
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
//printf("last=%d, %c\n", last, path[last]);
	path[last] = 0;
	return &path[last+1];
}

int c_getblocknum(dtree_t *dt, char *path)
{
	char *filename;
	int ret;
	file_entry_t parent, *ofile;
	file_meta_t pmeta, meta;
	void *pdata;

//printf("open=%s\n", path);
	filename = get_filename_from_path(path);
	if (!filename) {
		return -1;
	}
//printf("filename=%s\n", filename);
	ret = c_lookup(dt, path, 0, &parent);
	if (ret) {
		return -1;
	}
	if (parent.type != S_DIR) {
		return -1;
	}

	memcpy(&pmeta, dt->data + parent.fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	pdata = malloc(pmeta.blocks * dt->sb->bytsPerSec);
	if (!pdata)
		return -9;

	do_readdir(dt, parent.fb, pdata);
	ofile = find_entry_by_name(pdata, filename);
	if (!ofile) {
		return -1;
	}
	memcpy(&meta, dt->data + ofile->fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	ret = meta.strbase / sizeof(file_block_t);
	free(pdata);
	return ret;
}

int c_getblocks(dtree_t *dt, char *path, u64_t *l)
{
	char *filename;
	int ret, i;
	file_entry_t parent, *ofile;
	file_meta_t pmeta, meta;
	file_block_t *ptr;
	void *pdata, *buf;

//printf("open=%s\n", path);
	filename = get_filename_from_path(path);
	if (!filename) {
		return -1;
	}
//printf("filename=%s\n", filename);
	ret = c_lookup(dt, path, 0, &parent);
	if (ret)
		return -1;
	if (parent.type != S_DIR)
		return -1;

	memcpy(&pmeta, dt->data + parent.fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	pdata = malloc(pmeta.blocks * dt->sb->bytsPerSec);
	if (!pdata)
		return -9;

	do_readdir(dt, parent.fb, pdata);
	ofile = find_entry_by_name(pdata, filename);
	if (!ofile) {
		return -1;
	}
	memcpy(&meta, dt->data + ofile->fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	buf = malloc(meta.blocks*dt->sb->bytsPerSec);
	if (!buf)
		return -9;
	do_readfile(dt, ofile->fb, buf);
	ptr = buf + sizeof(file_meta_t);
	for (i =  0;ptr != buf+sizeof(file_meta_t)+meta.strbase;
			ptr++, i++) {
//		printf("here=%d\n", i);
		l[i] = ptr->block_num;
	}
	free(buf);
	free(pdata);
	return i;
}

int c_stat(dtree_t *dt, char *path, u32_t uid, stat_t *st)
{
	char *filename;
	int ret, check;
	file_entry_t parent, *ofile;
	file_meta_t pmeta, meta;
	void *pdata;

//printf("open=%s\n", path);
	filename = get_filename_from_path(path);
	if (!filename) {
		return -1;
	}
//printf("filename=%s\n", filename);
	ret = c_lookup(dt, path, uid, &parent);
	if (ret)
		return -1;
	if (parent.type != S_DIR)
		return -1;

	memcpy(&pmeta, dt->data + parent.fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	pdata = malloc(pmeta.blocks * dt->sb->bytsPerSec);
	if (!pdata)
		return -9;

	do_readdir(dt, parent.fb, pdata);
	ofile = find_entry_by_name(pdata, filename);
	if (!ofile) {
		return -1;
	}
	memcpy(&meta, dt->data + ofile->fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	check = permission_check(uid, &meta, R);
	if (!check) {
		return -4;
	}

	st->atime = meta.r_time;
	st->ctime = meta.c_time;
	st->mtime = meta.w_time;
	st->owner = meta.uid;
	st->size = meta.flen;
	st->mode = meta.flags;
	free(pdata);
	return 0;
}

int c_move(dtree_t *dt, char *srcpath, char *dstpath, u32_t uid)
{
	char *srcname, *dstname;
	int ret;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	file_entry_t sparent, dparent, *srcent, *dstent;
	file_meta_t meta, spmeta, dpmeta;
	void *spdata, *dpdata;

	srcname = get_filename_from_path(srcpath);
	if (!srcname) {
		return 1;
	}
	dstname = get_filename_from_path(dstpath);
	if (!dstname) {
		return 1;
	}

	ret = c_lookup(dt, srcpath, uid, &sparent);
	if (ret)
		return ret;
	if (sparent.type != S_DIR)
		return 1;
	ret = c_lookup(dt, dstpath, uid, &dparent);
	if (ret)
		return ret;
	if (dparent.type != S_DIR)
		return 1;

	memcpy(&spmeta, dt->data + sparent.fb*bytsPerSec,
			sizeof(file_meta_t));
	memcpy(&dpmeta, dt->data + dparent.fb*bytsPerSec,
			sizeof(file_meta_t));
	if (!permission_check(uid, &spmeta, W) ||
			!permission_check(uid, &dpmeta, W)) {
		return 4;
	}

	spdata = malloc((spmeta.blocks+1) * bytsPerSec);
	if (!spdata)
		return -9;
	do_readdir(dt, sparent.fb, spdata);
	srcent = find_entry_by_name(spdata, srcname);
	if (!srcent) {
		free(spdata);
		return 1; // file not found
	}
	dpdata = malloc((dpmeta.blocks+1) * bytsPerSec);
	if (!dpdata)
		return -9;
	do_readdir(dt, dparent.fb, dpdata);
	dstent = find_entry_by_name(dpdata, dstname);
	if (dstent) {
		free(spdata);
		free(dpdata);
		return 2;
	}

	memcpy(&meta, dt->data + srcent->fb*bytsPerSec,
			sizeof(file_meta_t));
	if (sparent.fb == dparent.fb) {
		file_meta_t *pmeta = spdata;
		pmeta->w_time = time(NULL);
		strcpy(spdata+pmeta->strlen, dstname);
		srcent->name = pmeta->strlen;
		pmeta->strlen += strlen(dstname) + 1;
		pmeta->flen += strlen(dstname) + 1;
		do_writedir(dt, sparent.fb, spdata, pmeta->flen);
		free(spdata);
		free(dpdata);
		return 0;
	} else {
		file_meta_t *spmeta = spdata;
		file_meta_t *dpmeta = dpdata;
		void *tmp = malloc(dpmeta->strlen);

		memcpy(tmp, dpdata+sizeof(file_meta_t)+dpmeta->strbase,
				dpmeta->strlen);
		memcpy(dpdata+sizeof(file_meta_t)+dpmeta->strbase, srcent,
				sizeof(file_entry_t));
		dpmeta->strbase += sizeof(file_entry_t);
		memcpy(dpdata+sizeof(file_meta_t)+dpmeta->strbase, tmp,
				dpmeta->strlen);
		memcpy(dpdata+sizeof(file_meta_t)+dpmeta->strbase+dpmeta->strlen,
				dstname, strlen(dstname)+1);
		dpmeta->strlen += strlen(dstname)+1;
		free(tmp);

		do_remove_entry(spdata, srcent);
		if (srcent->type == S_DIR) {
			dpmeta->count++;
			spmeta->count--;
		}
		do_writedir(dt, sparent.fb, spdata, spmeta->flen);
		do_writedir(dt, dparent.fb, dpdata, dpmeta->flen);
		free(spdata);
		free(dpdata);
		return 0;
	}
}

int c_remove(dtree_t *dt, char *path, u32_t uid)
{
	char *filename;
	int ret;
	u32_t bytsPerSec = dt->sb->bytsPerSec;
	file_entry_t parent, *entry;
	file_meta_t pmeta, meta, *real_pmeta;
	void *pdata;

	filename = get_filename_from_path(path);
	if (!filename) {
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
		return -9;

	do_readdir(dt, parent.fb, pdata);
	entry = find_entry_by_name(pdata, filename);
	if (!entry) {
		free(pdata);
		return 1; // file not found
	}
	if (entry->type == S_DIR) {
		free(pdata);
		return 2; // is a directory
	}

	memcpy(&meta, dt->data + entry->fb*bytsPerSec,
			sizeof(file_meta_t));

	real_pmeta = pdata;
	real_pmeta->count--;
	reset_fat(entry->fb, dt->fat_p);
	ret = do_remove_entry(pdata, entry);
	if (ret) {
		free(pdata);
		return ret;
	}
	do_writedir(dt, parent.fb, pdata, real_pmeta->flen);

	free(pdata);
	return 0;
}

int c_open(dtree_t *dt, char *path, u8_t oflags, u8_t mode, u32_t uid)
{
	char *filename;
	int ret, wcheck, rcheck, check;
	u32_t bytsPerSec = dt->sb->bytsPerSec, fb;
	file_entry_t parent, *ofile;
	file_meta_t pmeta;
	void *pdata;

//printf("open=%s\n", path);
	filename = get_filename_from_path(path);
	if (!filename) {
		return -1;
	}
//printf("filename=%s\n", filename);
	ret = c_lookup(dt, path, uid, &parent);
	if (ret)
		return -1;
	if (parent.type != S_DIR)
		return -1;

	memcpy(&pmeta, dt->data + parent.fb*bytsPerSec,
			sizeof(file_meta_t));
	wcheck = permission_check(uid, &pmeta, W);
	rcheck = permission_check(uid, &pmeta, R);
	check = (wcheck && (oflags|SBP_O_WRONLY)) ||
			(rcheck && (oflags|SBP_O_RDONLY)) ||
			(wcheck && rcheck && (oflags|SBP_O_RDWR));
	if (!check) {
		return -4;
	}
	pdata = malloc(pmeta.blocks * bytsPerSec);
	if (!pdata)
		return -9;

	do_readdir(dt, parent.fb, pdata);
	ofile = find_entry_by_name(pdata, filename);
	if (!ofile) {
		if (oflags|SBP_O_CREAT) {
			file_meta_t *meta;
			void *buf;
			u64_t newsize = pmeta.flen + sizeof(file_entry_t) +
					strlen(filename) + 1;
			buf = malloc(newsize);
			if (!buf) {
				free(pdata);
				return -9;
			}
			meta = buf;
			memcpy(buf, pdata, sizeof(file_meta_t)+pmeta.strbase);
			ofile = buf+sizeof(file_meta_t)+pmeta.strbase;
			ofile->name = pmeta.strlen;
			ofile->type = S_FIL;
			initfile(dt, uid, ofile, &parent, mode);
			meta->strbase += sizeof(file_entry_t);
			memcpy(buf+sizeof(file_meta_t)+meta->strbase,
					pdata+sizeof(file_meta_t)+pmeta.strbase, pmeta.strlen);
			memcpy(buf+sizeof(file_meta_t)+meta->strbase+pmeta.strlen,
					filename, strlen(filename)+1);
			meta->strlen += strlen(filename)+1;
			meta->flen = newsize;
			meta->blocks = (newsize + bytsPerSec - 1) / bytsPerSec;
		//	dump_meta(meta);
			do_writedir(dt, parent.fb, buf, newsize);
			fb = ofile->fb;
			free(buf);
		} else {
			free(pdata);
			return -1; // file not found
		}
	} else if (ofile->type == S_DIR) {
		free(pdata);
		return -2; // is a directory
	} else {

		fb = ofile->fb;
	}

	free(pdata);
	return fb;
}

int c_read(dtree_t *dt, u32_t fb, u64_t offset, u64_t length, u32_t uid,
		sendblock_t *blocks)
{
	file_meta_t meta, *real_meta;
	int blocknum;
	sendblock_t *sblock_ptr = blocks;
	u64_t offset_1, length_1, newsize;
	u64_t bufsize;
	void *buf;
	int i;
	file_block_t *fb_p;

//	printf("initsize: %llu, %llu, %llu\n", newsize,  length, offset);
	memcpy(&meta, dt->data+fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	if (meta.flen < offset+length) {
//		printf("olllllllllllllllllllllllllllllllll%llu\n", offset+length);
		newsize = offset+length;
	} else {
//		printf("flfffffffffffffffffffffffffff%llu\n", meta.flen);
		newsize = meta.flen;
	}
//	printf("flen: %llu\n", meta.flen);
	if (meta.flen < offset) {
		return -1;
	}
	length = (length < meta.flen-offset) ?
			length : meta.flen-offset;
	bufsize = meta.blocks * dt->sb->bytsPerSec;
	real_meta = buf = malloc(bufsize);
	if (!buf) {
		return -9;
	}
	do_readfile(dt, fb, buf);
	offset_1 = up_div(offset, BLOCKSIZE) * BLOCKSIZE;
	length_1 = ((length + offset) / BLOCKSIZE) * BLOCKSIZE;
	blocknum = (length_1 - offset_1) / BLOCKSIZE;
//printf("num=%d, %llu, %llu\n", blocknum, length, offset);
//printf("num=%d, %llu, %llu\n", blocknum, length_1, offset_1);
	if (offset != offset_1) blocknum++;
	if (offset+length != length_1) blocknum++;
	fb_p = buf+sizeof(file_meta_t) +
				(offset/BLOCKSIZE) * sizeof(file_block_t);
	if (blocknum == 1) {
		sblock_ptr->block_num = fb_p->block_num;
		sblock_ptr->offset = offset % BLOCKSIZE;
		sblock_ptr->length = length;
//printf("sbp: %d\n", sblock_ptr->length);
		memcpy(sblock_ptr->ip, fb_p->ip, NR_COPY*sizeof(u32_t));
	} else {
		for (i = 0; i < blocknum; i++) {
			sblock_ptr->block_num = fb_p->block_num;
			if (i == 0) {
				sblock_ptr->offset = offset%BLOCKSIZE;
				sblock_ptr->length = BLOCKSIZE - offset;
			} else if (i == blocknum-1) {
				sblock_ptr->offset = 0;
				sblock_ptr->length = ((offset+length)%BLOCKSIZE) ?
						(offset+length)%BLOCKSIZE : BLOCKSIZE;
			} else {
				sblock_ptr->offset = 0;
				sblock_ptr->length = BLOCKSIZE;
			}
			memcpy(sblock_ptr->ip, fb_p->ip, NR_COPY*sizeof(u32_t));
			sblock_ptr++;
			fb_p++;
		}
	}
	free(buf);
	return blocknum;
//
//	file_meta_t meta;
//	u64_t blocknum;
//	sendblock_t *sblock_ptr = blocks;
//	u64_t offset_1, length_1, real_length;
//	u64_t bufsize;
//	void *buf;
//	int i;
//
//	memcpy(&meta, dt->data+fb*dt->sb->bytsPerSec,
//			sizeof(file_meta_t));
//	if (meta.flen < offset) {
//		return -1;
//	}
//	real_length = (length < meta.flen-offset) ?
//			length : meta.flen-offset;
//	bufsize = meta.blocks * dt->sb->bytsPerSec;
//	buf = malloc(bufsize);
//	if (!buf) {
//		return -9;
//	}
//	do_readfile(dt, fb, buf);
//	offset_1 = up_div(offset, BLOCKSIZE) * BLOCKSIZE;
//	length_1 = ((real_length + offset) / BLOCKSIZE) * BLOCKSIZE;
//	blocknum = (length_1 - offset_1) / BLOCKSIZE;
//	if (offset != offset_1) blocknum++;
//	if (real_length != length_1) blocknum++;
//	file_block_t *fb_p = buf+sizeof(file_meta_t) +
//			(offset/BLOCKSIZE) * sizeof(file_block_t);
//
//	if (blocknum == 1) {
//		sblock_ptr->block_num = fb_p->block_num;
//		sblock_ptr->offset = offset % BLOCKSIZE;
//		sblock_ptr->length = real_length - offset;
////printf("sbp: %d\n", sblock_ptr->length);
//		memcpy(sblock_ptr->ip, fb_p->ip, NR_COPY*sizeof(u32_t));
//	} else {
//		for (i = 0; i < blocknum; i++) {
//			sblock_ptr->block_num = fb_p->block_num;
//			if (i == 0) {
//				sblock_ptr->offset = offset%BLOCKSIZE;
//				sblock_ptr->length = BLOCKSIZE - offset;
//			} else if (i == blocknum-1) {
//				sblock_ptr->offset = 0;
//				sblock_ptr->length = (real_length%BLOCKSIZE) ?
//						real_length%BLOCKSIZE : BLOCKSIZE;
//			} else {
//				sblock_ptr->offset = 0;
//				sblock_ptr->length = BLOCKSIZE;
//			}
//			memcpy(sblock_ptr->ip, fb_p->ip, NR_COPY*sizeof(u32_t));
//			sblock_ptr++;
//			fb_p++;
//		}
//	}
//	meta.r_time = time(NULL);
//	memcpy(dt->data+fb*dt->sb->bytsPerSec, &meta,
//			sizeof(file_meta_t));
//
//	free(buf);
//	return blocknum;
}

int c_write(dtree_t *dt, u32_t fb, u64_t offset, u64_t length, u32_t uid,
		u32_t *ips, int iplen, sendblock_t *blocks)
{
	file_meta_t meta, *real_meta;
	int newblocknum, blocknum;
	sendblock_t *sblock_ptr = blocks;
	u64_t offset_1, length_1, newsize;
	u64_t bufsize, bufsize_1, bufsize_2;
	void *buf;
	int i;

//	printf("initsize: %llu, %llu, %llu\n", newsize,  length, offset);
	memcpy(&meta, dt->data+fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	if (meta.flen < offset+length) {
//		printf("olllllllllllllllllllllllllllllllll%llu\n", offset+length);
		newsize = offset+length;
	} else {
//		printf("flfffffffffffffffffffffffffff%llu\n", meta.flen);
		newsize = meta.flen;
	}
//	printf("flen: %llu\n", meta.flen);
	if (meta.flen < offset) {
		return -1;
	}
	bufsize_1 = sizeof(file_meta_t) +
			up_div(offset+length, BLOCKSIZE)*sizeof(file_block_t);
	bufsize_2 = meta.blocks * dt->sb->bytsPerSec;
	bufsize = (bufsize_1 > bufsize_2) ? bufsize_1 : bufsize_2;
	real_meta = buf = malloc(bufsize);
	if (!buf) {
		return -9;
	}
	do_readfile(dt, fb, buf);
	offset_1 = up_div(offset, BLOCKSIZE) * BLOCKSIZE;
	length_1 = ((length + offset) / BLOCKSIZE) * BLOCKSIZE;
	blocknum = (length_1 - offset_1) / BLOCKSIZE;
//printf("num=%d, %llu, %llu\n", blocknum, length, offset);
//printf("num=%d, %llu, %llu\n", blocknum, length_1, offset_1);
	if (offset != offset_1) blocknum++;
	if (offset+length != length_1) blocknum++;
	newblocknum = up_div(offset+length, BLOCKSIZE) -
			up_div(meta.flen, BLOCKSIZE);
//	printf("asdf: %llu\n", meta.flen);
//	printf("new=%d, %llu, %llu\n", newblocknum, up_div(offset+length, BLOCKSIZE), up_div(meta.flen, BLOCKSIZE));
	if (newblocknum <= 0) {
		file_block_t *fb_p = buf+sizeof(file_meta_t) +
				(offset/BLOCKSIZE) * sizeof(file_block_t);
//printf("asdfasdf=%d, %llu, %llu\n", blocknum, length_1, offset_1);
		if (blocknum == 1) {
			sblock_ptr->block_num = fb_p->block_num;
			sblock_ptr->offset = offset % BLOCKSIZE;
			sblock_ptr->length = length;
			fb_p->length = length + offset;
//printf("sbp: %d\n", sblock_ptr->length);
			memcpy(sblock_ptr->ip, fb_p->ip, NR_COPY*sizeof(u32_t));
		} else {
			for (i = 0; i < blocknum; i++) {
				sblock_ptr->block_num = fb_p->block_num;
				if (i == 0) {
					sblock_ptr->offset = offset%BLOCKSIZE;
					sblock_ptr->length = BLOCKSIZE - offset;
				} else if (i == blocknum-1) {
					sblock_ptr->offset = 0;
					sblock_ptr->length = ((offset+length)%BLOCKSIZE) ?
							(offset+length)%BLOCKSIZE : BLOCKSIZE;
					fb_p->length = (sblock_ptr->length < fb_p->length) ?
							fb_p->length : sblock_ptr->length;
				} else {
					sblock_ptr->offset = 0;
					sblock_ptr->length = BLOCKSIZE;
				}
				memcpy(sblock_ptr->ip, fb_p->ip, NR_COPY*sizeof(u32_t));
				sblock_ptr++;
				fb_p++;
			}
		}
	} else {
		u64_t oldblocknum = blocknum - newblocknum;
		int i;
		file_block_t *fb_p = buf+sizeof(file_meta_t) +
				(offset/BLOCKSIZE) * sizeof(file_block_t);
		for (i = 0; i < oldblocknum; i++) {
			sblock_ptr->block_num = fb_p->block_num;
			if (i == 0) {
				sblock_ptr->offset = offset % BLOCKSIZE;
				sblock_ptr->length = BLOCKSIZE - offset;
			} else {
				sblock_ptr->offset = 0;
				sblock_ptr->length = BLOCKSIZE;
			}
			memcpy(sblock_ptr->ip, fb_p->ip, NR_COPY*sizeof(u32_t));
			sblock_ptr++;
			fb_p++;
		}
		for (i = 0; i < newblocknum; i++) {
			file_block_t *newblock = buf + sizeof(file_meta_t) +
					real_meta->strbase;
			alloc_block(dt, ips, iplen, newblock);
			sblock_ptr->block_num = newblock->block_num;
			sblock_ptr->offset = 0;
			if (i == newblocknum-1) {
				sblock_ptr->length = (length%BLOCKSIZE) ?
						length%BLOCKSIZE : BLOCKSIZE;
			} else {
				sblock_ptr->length = BLOCKSIZE;
			}
			newblock->length = sblock_ptr->length;
			memcpy(sblock_ptr->ip, newblock->ip, NR_COPY*sizeof(u32_t));
			sblock_ptr++;
			real_meta->strbase += sizeof(file_block_t);
		}
	}
	real_meta->flen = newsize;
//	printf("newsize: %llu\n", newsize);
	real_meta->w_time = time(NULL);
	real_meta->blocks = up_div(sizeof(file_meta_t)+real_meta->strbase,
			dt->sb->bytsPerSec);
	do_writefile(dt, fb, buf,
			sizeof(file_meta_t)+real_meta->strbase);
	free(buf);
	return blocknum;
}

#if 0
int c_write(dtree_t *dt, u32_t fb, u64_t offset, u64_t length, u32_t uid,
		u32_t *ips, int iplen, sendblock_t *blocks)
{
	file_block_t *endbp;
	file_block_t *bp=NULL;
	file_meta_t meta, *rmeta;
	u64_t block_num, i, newsize, lastlen, newblocknum = 0;
	void *data;
printf("write %d, %d", offset, length);
	memcpy(&meta, dt->data+fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	if (meta.flen < offset) {
		return -1;
	}
	newsize = (meta.flen > (offset+length)) ? meta.flen : offset+length;
printf("newsize=%d\n", newsize);
	data = malloc(sizeof(file_meta_t)+(newsize+BLOCKSIZE-1)/BLOCKSIZE);
	if (!data) {
		printf("hre\n");
		return -9;
	}
	do_readfile(dt, fb, data);
	rmeta = data;

	endbp = data + sizeof(file_meta_t) + meta.strbase;
	block_num = (offset % BLOCKSIZE + length + BLOCKSIZE -1) / BLOCKSIZE;
	lastlen = length / BLOCKSIZE;
	if (lastlen == 0) lastlen = BLOCKSIZE;
	bp = data + sizeof(file_meta_t) +
			(offset%BLOCKSIZE) * sizeof(file_block_t);
printf("block_num=%llu, bp=%p, endbp=%p\n", block_num, bp, endbp);
	file_block_t *tmp=bp;
printf("block_num=%llu, bp=%p, endbp=%p\n", block_num, bp, endbp);
//printf("i=%d, bp=%p, endbp=%p, tmp=%p\n", i, bp, endbp, tmp);
	for (i = 0; i < block_num; i++) {
//printf("i=%d, bp=%p, endbp=%p\n", i, bp, endbp);
printf("i=%d, bp=%p, endbp=%p\n", 90909, bp, endbp);
		if ((unsigned long)bp < (unsigned long)endbp) {
			printf("i=%d, bp=%p, endbp=%p\n", i, bp, endbp);
printf("if\n");
			blocks[i].block_num = bp->block_num;
			blocks[i].offset = (i == 0) ?
					(offset % BLOCKSIZE) : 0;
			blocks[i].length = (i == block_num-1) ?
					lastlen : BLOCKSIZE;
			memcpy(blocks[i].ip, bp->ip, NR_COPY*sizeof(u32_t));
			bp++;
		} else {
			file_block_t newblock;
printf("alloc_block");
			alloc_block(dt, ips, iplen, &newblock);
printf("end alloc_block");
			newblocknum++;
			memcpy(data+sizeof(file_meta_t)+rmeta->strbase, &newblock,
					sizeof(file_block_t));
			rmeta->strbase += sizeof(file_block_t);
			blocks[i].block_num = newblock.block_num;
			blocks[i].offset = (i == 0) ?
					(offset % BLOCKSIZE) : 0;
			blocks[i].length = (i == block_num-1) ?
					lastlen : BLOCKSIZE;
		}
	}
printf("end for\n");
	do_writefile(dt, fb, data,
			meta.flen+newblocknum*sizeof(file_block_t));
	free(data);

	return block_num;
}
#endif

int c_opendir(dtree_t *dt, char *path, u32_t uid, u64_t *entnum)
{
	file_entry_t entry;
	file_meta_t meta;
	int fb, ret;

	ret = c_lookup(dt, path, uid, &entry);
	if (ret)
		return -1;
	fb = entry.fb;

	memcpy(&meta, dt->data+entry.fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));

	if (!permission_check(uid, &meta, R)) {
		return -4;
	}

	*entnum = meta.strbase / sizeof(file_entry_t);

	return fb;
}

int c_readdir(dtree_t *dt, u32_t fb, u64_t entnum, u32_t uid,
		char *name, u8_t *type)
{
	file_meta_t meta;
	file_entry_t *ptr;
	void *buf;

	memcpy(&meta, dt->data+fb*dt->sb->bytsPerSec,
			sizeof(file_meta_t));
	if (entnum*sizeof(file_entry_t) >= meta.strbase) {
		return -1;
	}
	buf = malloc(meta.blocks * dt->sb->bytsPerSec);
	if (!buf) {
		return -9;
	}
	do_readdir(dt, fb, buf);
	ptr = buf + sizeof(file_meta_t) +
			entnum*sizeof(file_entry_t);
	*type = ptr->type;
	strncpy(name, buf+sizeof(file_meta_t)+meta.strbase+ptr->name,
			MAX_FILENAME);
	name[MAX_FILENAME-1] = 0;
	free(buf);
	return 0;
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
		return -9;
	do_readdir(dt, parent.fb, pdata);
	if (find_entry_by_name(pdata, dirname)) {
		return 3;
	}

	newsize = pmeta.flen + strlen(dirname) + 1 + sizeof(file_entry_t);
	buf = malloc(newsize);
	if (!buf) {
		free(pdata);
		return -9;
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
		return -9;

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
		return -9;
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
		return -9;
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
		return -9;

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
		return -9;
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
	sb->maxBlockNum = 0;

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

static int initfile(dtree_t *dt, u32_t uid,
		file_entry_t *file_entry, file_entry_t *parent, u8_t mode)
{
	file_meta_t *meta;
	u32_t count, fat_index, i;
	u64_t size = sizeof(file_meta_t);
	time_t tm = time(NULL);

	meta = malloc(size);
	if (!meta)
		return -9;
	memset(meta, 0, size);

	do {
		fat_index = next_free_fat(0, dt->fat_p, dt->sb->fatNum);
		if (fat_index != -1) break;
		extend(dt, dt->dt_size * 2);
	} while (1);

	file_entry->fb = fat_index;

	count = (size + dt->sb->bytsPerSec - 1) / dt->sb->bytsPerSec;

	meta->flags = mode;
	meta->c_time = tm;
	meta->r_time = tm;
	meta->w_time = tm;
	meta->uid = uid;
	meta->count = 1;
	meta->flen = 0;
	meta->strbase = 0;
	meta->strlen = 0;
	meta->blocks = count;

	for (i = 0; i < count;) {
		int len = (size < dt->sb->bytsPerSec) ? size : dt->sb->bytsPerSec;
		u32_t next;
		memcpy(dt->data+fat_index*dt->sb->bytsPerSec,
				(void *)meta+i*dt->sb->bytsPerSec, len);
		i += 1;
		dt->fat_p[fat_index] = ENDSEC;
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

	free(meta);

	return 0;
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
		return -9;
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
		dt->fat_p[fat_index] = ENDSEC;
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
//printf("out: %s\n", filename);

	fd = open(filename, O_WRONLY|O_CREAT, 0644);
	if (fd < 0)
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
