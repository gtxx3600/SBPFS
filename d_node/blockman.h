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

typedef unsigned long long u64_t;
typedef long long s64_t;
typedef unsigned int u32_t;
typedef int s32_t;
typedef unsigned short u16_t;
typedef short s16_t;
typedef unsigned char u8_t;
typedef char s8_t;

char blocklist[BLOCKNAME][MAX_BLOCKSIZE];
char path[1000];

char databuf[4096];
int bnum;
int getlist();
int readblock(u64_t blocknum, u64_t offset, u64_t length);
int writeblock(u64_t blocknum, u64_t offset, u64_t length, char* data);
int createblock(u64_t blocknum);
int deleteblock(u64_t blocknum);
void getpath(u64_t blocknum);
int deletelist(u64_t blocknum);
int addlist(u64_t blocknum);
void initlist();

#endif /* BLOCKMAN_H_ */
