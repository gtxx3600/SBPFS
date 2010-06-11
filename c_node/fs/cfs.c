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
#include <unistd.h>

#include "cfs.h"
#include "type.h"

#define CFSNAME "cfs"
#define INIT_MAX 65536
#define BYTSPERSEC 512

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
	}

	dt->in_buf = load_file(in_name, &in_size, &in_max);
	if (!dt->in_buf) {
		dt->in_buf = malloc(INIT_MAX);
		if (!dt->in_buf)
			goto out_error;
		in_size = in_max = INIT_MAX;
	}

	mkcfs(dt->dt_buf, dt_max, BYTSPERSEC);

	dt->dt_size = dt_size;
	dt->in_size = in_size;
	dt->dt_max = dt_max;
	dt->in_max = in_max;

	dt->sb = dt->dt_buf;
	bytsPerSec = dt->sb->bytsPerSec;
	dt->fat_p = (void *)dt->sb + bytsPerSec;
	dt->data = (void *)dt->fat_p + bytsPerSec * dt->sb->fatSecNum;

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

void mkcfs(void *buf, unsigned int size, unsigned int bytsPerSec)
{
	file_entry_t *root_entry;
	sb_t *sb = buf;
	u32_t totSecNum = size / bytsPerSec;
	u32_t fatSize = totSecNum * sizeof(fat_t);
	u32_t fatSecNum = (fatSize + bytsPerSec -1) / bytsPerSec;
	memset(buf, 0, sizeof(sb_t));
	strcpy(sb->fstype, CFSNAME);
	sb->bytsPerSec = bytsPerSec;
	sb->totSecNum = totSecNum;
	sb->fatSecNum = fatSecNum;
	memset(buf+bytsPerSec, 0, fatSecNum*bytsPerSec);
	root_entry = buf + bytsPerSec + fatSecNum*bytsPerSec;
	root_entry->name = 0;
	root_entry->uid = 0;
	root_entry->flags = S_DIR|S_IRW|S_ORW;
	root_entry->fb = 0;
	root_entry->flen = 0;
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
