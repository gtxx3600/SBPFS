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

#include "sbpfs.h"
#include "lib.h"
#include <stdio.h>
#include <string.h>
s32_t sbp_open(char* filename, u32_t oflag, u16_t mode)
{

	return 0;
}


s32_t sbp_test(char* buf, u64_t len, char* target ,u32_t port){
	char* rec_buf;
	u64_t rec_len;
	int ret = 0;
	if((ret = sendrec_hostname(target, port, buf, len, &rec_buf, &rec_len)) < 0)
	{
		//printf("TEST Failed : %d\n ", ret);
		perror("TEST Failed");
		return ret;
	}
	printf("Received : %ld\n%s",strlen(rec_buf),rec_buf);
	return 0;


}
