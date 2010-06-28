/*
 * blockman.c
 *
 *  Created on: 2010-6-27
 *      Author: cxy
 */

typedef unsigned long long u64_t;
typedef long long s64_t;
typedef unsigned int u32_t;
typedef int s32_t;
typedef unsigned short u16_t;
typedef short s16_t;
typedef unsigned char u8_t;
typedef char s8_t;

#define MAX_BLOCKSIZE 6400
#define BLOCKNAME 64
#define FILEMAXSIZE 1024*1024*16
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include<stdio.h>
char blocklist[BLOCKNAME][MAX_BLOCKSIZE];
char path[1000];
char* listpath = "/home/cxy/blocklist";
char databuf[4096];
int bnum;
int getlist();
int readblock(u64_t blocknum, int offset, int length);
int writeblock(u64_t blocknum, int offset, int length, char* data);
int createblock(u64_t blocknum);
int deleteblock(u64_t blocknum);
void getpath(u64_t blocknum);
int deletelist(u64_t blocknum);
int addlist(u64_t blocknum);
void initlist();
/*int main() {
	initlist();
	getlist();
	writeblock(3263, 3, 3, "abc");
	writeblock(3363, 3, 4, "abcd");
	writeblock(3463, 3, 3, "abc");
	readblock(3263, 3, 3);
	readblock(3363, 3, 4);
	getlist();
	deleteblock(3363);
	getlist();
	deleteblock(3263);
	getlist();
	deleteblock(3463);
	getlist();
	return 0;
}*/

int getlist() {
	FILE *blist;
	blist = fopen(listpath, "rb");
	if (blist < 0) {
		perror("error: blocklist open!");
		return -1;
	}
	char numc[4];
	fread(numc, 4, 1, blist);
	bnum = atoi(numc);
	printf("get blocknum: %d \n", bnum);
	int i;
	char temp[64];
	for (i = 0; i < bnum; i++) {

		if (fseek(blist, (i+1) * BLOCKNAME, SEEK_SET) < 0) {
			perror("error: lseek");
			return -1;
		}
		fread(temp, BLOCKNAME, 1, blist);
			//perror("error size");
		//printf("temp :%s\n",temp);
		strcpy(*(blocklist+i),temp);
	}
	for (i = 0; i < bnum; i++) {
		printf("block %d : %s\n", i, blocklist[i]);
	}
	fclose(blist);
	return 0;
}

void initlist() {
	int i;
	char buf[64];

	//memset(buf,0, 64);
	strcpy(buf, "0000");
	FILE *fd = fopen("/home/cxy/blocklist", "rb+");
	for (i = 0; i < 6401; i++) {
		fwrite(buf, 64, 1, fd);
	}
	fclose(fd);
}

int readblock(u64_t blocknum, int offset, int length) {
	getpath(blocknum);
	FILE *block;
	if ((block = fopen(path, "rb")) == NULL) {
		printf("error: blocklist open!");
		return -1;
	}
	if (fseek(block, offset, SEEK_SET) < 0) {
		perror("error: fseek");
		return -1;
	}
	if (fread(databuf, length, 1, block) != BLOCKNAME) {
		//perror("error size");
	}
	printf("databuf: %s\n", databuf);
	fclose(block);
	return 0;
}

int writeblock(u64_t blocknum, int offset, int length, char* data) {
	getpath(blocknum);
	FILE *block;
	if ((block = fopen(path, "rb+")) == NULL) {
		printf("block creating!\n");

		createblock(blocknum);
	}
	if ((block = fopen(path, "rb+")) == NULL) {
		perror("error in creating block.");
	}

	if (fseek(block, offset, SEEK_SET) < 0) {
		perror("error: fseek");
		return -1;
	}
	fwrite(data, length, 1, block);
	//perror("error size");
	fclose(block);
	return 0;
}

int createblock(u64_t blocknum) {
	getpath(blocknum);
	int i;
	FILE *block1;
	block1 = fopen(path, "wb");
	char buf[1024];
	//memset(buf, "0", 1024);
	strcpy(buf, "567890");
	for (i = 0; i < 1024 * 16; i++) {
		fwrite(buf, 1024, 1, block1);
	}
	fclose(block1);
	addlist(blocknum);
	return 0;
}

void getpath(u64_t blocknum) {

	int flagnum = blocknum % 100;

	strcpy(path, "/home/cxy/");
	char p[2];
	sprintf(p, "%d", flagnum);
	//printf("%s",p);
	strcat(path, p);
	strcat(path, "/");
	sprintf(p, "%lld", blocknum);
	strcat(path, p);
	//printf("%s", path);
}

int deleteblock(u64_t blocknum) {
	getpath(blocknum);
	FILE *block;
	if ((block = fopen(path, "rb")) == NULL) {
		printf("block not exist!");
	} else {
		fclose(block);
		remove(path);
	}
	deletelist(blocknum);
	return 0;
}

int addlist(u64_t blocknum) {

	char p[2];
	sprintf(p, "%lld", blocknum);
	strcpy(*(blocklist+bnum),p);
	bnum++;
	printf("list add: %d :%s\n", bnum - 1, blocklist[bnum - 1]);
	FILE *blist;
	blist = fopen(listpath, "rb+");
	if (blist < 0) {
		perror("error: blocklist open!");
		return -1;
	}
	char numc[4];
	sprintf(numc, "%d", bnum);
	fwrite(numc, 4, 1, blist);
	if (fseek(blist, bnum * BLOCKNAME, SEEK_SET) < 0) {
		perror("error: lseek");
		return -1;
	}
	if (fwrite(p, BLOCKNAME, 1, blist) != BLOCKNAME) {
		//perror("error size");
	}
	fclose(blist);
	return 0;
}

int deletelist(u64_t blocknum) {
	int i;
	int delnum;
	char p[2];
	sprintf(p, "%lld", blocknum);
	for (i = 0; i < bnum; i++) {
		if (strcmp(p, blocklist[i]) == 0) {
			strcpy(*(blocklist+i),blocklist[bnum-1]);
			bnum--;
			delnum=i;
		}
	}
	FILE *blist;
		blist = fopen(listpath, "rb+");
		if (blist < 0) {
			perror("error: blocklist open!");
			return -1;
		}
		char numc[4];
		sprintf(numc, "%d", bnum);
		fwrite(numc, 4, 1, blist);
		if (fseek(blist, (delnum+1) * BLOCKNAME, SEEK_SET) < 0) {
			perror("error: lseek");
			return -1;
		}
		if (fwrite(blocklist[delnum], BLOCKNAME, 1, blist) != BLOCKNAME) {
			//perror("error size");
		}
		fclose(blist);
		return 0;

}

