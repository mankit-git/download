#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void http_request(char *buf, int size, char *file, int start, char *host)
{
	assert(buf);
	
	bzero(buf, size);
	snprintf(buf, size, "GET /%s HTTP/1.1\r\n"
			 "Range: bytes=%d-\r\n"
			 "Host: %s\r\n\r\n", file, start, host);

}

int main(int argc, char **argv)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	char *host = "www.boa.org";
	char *file = "boa-0.94.13.tar.gz";

	FILE *fp;
	int filesize;
	if(access(file, F_OK) == -1)
	{
		fp = fopen(file, "w");
	}
	else 
	{
		struct stat info;
		stat(file, &info);

		filesize = info.st_size;	
		fp = fopen(file, "a");
	}
	if(fp == NULL)
	{
		perror("fopen() failed");
		exit(0);
	}

	struct hostent *he = gethostbyname(host);
	if(he == NULL)
	{
		perror("gethostbyname() failed");
		exit(0);
	}

	struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;

	struct sockaddr_in svaddr;
	socklen_t len = sizeof(svaddr);
	bzero(&svaddr, len);
	svaddr.sin_family = AF_INET;
	svaddr.sin_port = htons(80);
	svaddr.sin_addr = **addr_list;

	if(connect(fd, (struct sockaddr *)&svaddr, len) == -1)
	{
		perror("connect() failed");
		exit(0);
	}

	char buf[1024];
	http_request(buf, 1024, file, filesize, host);

	int w = write(fd, buf, 1024);
	if(w == -1)
	{
		perror("write() failed");
		exit(0);
	}

	char msg[1024];
	int n = read(fd, msg, 1024);
	printf("n: %d\n", n);

	char *head = strstr(msg, "\r\n\r\n");
	*(head+3) = '\0';
	printf("msg: %s\n", msg);
	//printf("%s\n", head);
	//printf("%s\n", head+4);
	printf("msg: %d\n", strlen(msg));
	fwrite(head+4, n-strlen(msg)-1, 1, fp);
	//获取要下载的文件大小：length
	char *length = strstr(msg, "Content-Length:") + strlen("Content-Length: ");
	if(atoi(length) == 0)
	{
		printf("%s is already download\n", file);
		exit(0);
	}
	
	//获取该文件总大小：size
	printf("download %s ...\n", file);
	char *begin = strstr(msg, "Content-Range:");
	char *end = strstr(msg, "Content-Type:");
	char range[end-begin];
	memcpy(range, begin, end-begin);

	char *size = NULL;
	size = strtok(range, "/");
	size = strtok(NULL, "/");

	//下载
	char *recvbuf = calloc(1, 1024);
	while(1)
	{
		int m = recv(fd, recvbuf, 1024, 0);

		if(m == 0)
			break;

		if(m == -1)
		{
			perror("recv() failed");
			exit(0);
		}

		fwrite(recvbuf, m, 1, fp);
		filesize += m;

		if(filesize == atoi(size))
			break;
	}
	return 0;
}
