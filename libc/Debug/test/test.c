#include <stdio.h>
#include <sbpfs.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
int main(int argc,char * argv[])
{
	printf("long :%d\n",sizeof(long));
	printf("int: %d\n",sizeof(int));
	printf("size_t:%d\n",sizeof(size_t));
	printf("ssize_t:%d\n",sizeof(ssize_t));
	printf("long int:%d\n",sizeof(long int));
	printf("long long:%d\n",sizeof(long long));
	int i =-1;
	char* x = "hahagaga";
	char* y = strstr(x,"g");
	long long l = y -x;
	

	printf("long long:%lld\n",l);
    char* host = "192.168.1.105";
    int port = 9000;
    char* buf = "SBPFS/1.0\r\nUser: Client_haha\r\nMethod: OPEN\r\n\r\n";
    char* mkdir = "SBPFS/1.0\r\nUser: Client_haha\r\nMethod: MKDIR\r\nArgc: 1\r\nArg0: haha\r\n\r\n";
    int len = strlen(buf);

	/*if(sendrec() == -1)
	{
		printf("sendrec failed\n");
	}*/
    long int rec_len;
    sbp_test(mkdir,strlen(mkdir),host,port);
    sbp_test(buf,strlen(buf),host,port);
    return 0;
    

}

int sendrec()
{
	ssize_t sockfd,numbytes,missing_bytes;
	char buf[BUF_SIZE+1];

	struct hostent *target;
	struct sockaddr_in target_addr;

	char* host_name = "www.google.com";
	size_t port = 80;
	buf[BUF_SIZE] = 0;
	target = gethostbyname(host_name);
	target_addr.sin_family = AF_INET;
//	target_addr.sin_addr.s_addr = htol(INADDR_ANY);
	target_addr.sin_addr.s_addr = ((struct in_addr *)(target->h_addr))->s_addr;
	printf("target_addr:%d\n",target_addr.sin_addr.s_addr);
	target_addr.sin_port = htons(port);
	printf("target_port:%d\n",target_addr.sin_port);
	bzero(&(target_addr.sin_zero), 8);


	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Socket Error, %d\n", errno);
		goto error_exit;
	}
	if(connect(sockfd, (struct sockaddr *)&target_addr, sizeof(struct sockaddr)))
	{
		printf("Connect Error, %d\n", errno);
		goto error_exit;
	}else
	{
		printf("Connect success\n");
	}
	return 0;
error_exit:
	close(sockfd);
	return -1;

}
