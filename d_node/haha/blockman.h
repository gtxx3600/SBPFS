/*
 * blockman.h
 *
 *  Created on: 2010-6-28
 *      Author: cxy
 */

#ifndef BLOCKMAN_H_
#define BLOCKMAN_H_

#define MAX_BLOCKSIZE 6400
#define BLOCKNAME 64
#define FILEMAXSIZE 1024*1024*16
#include "const.h"

char blocklist[BLOCKNAME][MAX_BLOCKSIZE];
u64_t blolist[MAX_BLOCKSIZE];
char path[1000];

u64_t bnum;

u64_t getlist();
int readblock(u64_t blocknum, u32_t offset, u32_t length,char* data);
int writeblock(u64_t blocknum, u32_t offset, u32_t length, char* data);
int createblock(u64_t blocknum);
int deleteblock(u64_t blocknum);
void getpath(u64_t blocknum);
int deletelist(u64_t blocknum);
int addlist(u64_t blocknum);
void initlist();
int blockdir(u64_t blocknum) ;
#endif /* BLOCKMAN_H_ */
