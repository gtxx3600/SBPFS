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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
s32_t sbp_remove(char* filename) {
	SBP_PREPARE_REQUEST

	mkent(head,METHOD,"REMOVE");
	mkent(head,ARGC,"1");
	mkent(head,"Arg0",filename);
	mkent(head,CONTENT_LEN,"0");

	SBP_SEND_AND_PROCESS_REPLY
	SBP_PROCESS_RESULT
	SBP_PROCESS_ERR
}
