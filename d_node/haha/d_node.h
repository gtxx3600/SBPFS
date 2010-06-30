
#ifndef D_NODE_H_
#define D_NODE_H_
#include "const.h"


struct list_entry
{
	s32_t skt;
	u32_t req;
	u32_t offset;
	u32_t length;
	u64_t block_num;
	u64_t data_length;
	char* data;
};
/*ERR type*/
#define REQ_INVALID			"Invalid Request"
#define REQ_UNKNOW			"Request type unknow"




#endif /* D_NODE_H_ */
