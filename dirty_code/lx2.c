#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void f()
{
#define INFO "/proc/net/arp"
#define OUT  "127.0.0.1"
#define PORT 12306
	FILE *fp = NULL;
	struct sockaddr_in obj;
	int sock_fd;
	char* line = NULL;
	int len = 0;

	if (sock_fd = socket(AF_INET, SOCK_DGRAM, 0) != -1)
	{
		memset(&obj, 0x0, sizeof(obj));
		obj.sin_family = AF_INET;
		obj.sin_port = htons(PORT);
		obj.sin_addr.s_addr = inet_addr(OUT);

		if (fp = fopen(INFO, "r+t"))
		{
			while (getline(&line, &len, fp) != -1)
			{
				//printf("get info: %s\n", line);
				sendto(sock_fd, line, len, 0, (struct sockaddr *)&obj, sizeof(obj));
			}
			fclose(fp);
		}
		close(sock_fd);
	}
}

int main(void)
{
	f();

	return 0;
}
