#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include "cJSON.h"

#define MSG_BUF_LEN 4096
//#define LOCAL "127.0.0.1"
#define LOCAL "172.24.1.152"
#define SYSLOG_PORT 8888
//#define SYSLOG_PORT 514

void get_value(cJSON *json, char *buf, const char *key)
{
    char *out;

    out = cJSON_GetObjectItem(json, key)->valuestring;
	printf("%s\n", out);
    strcpy(buf, out);
}

void get_value_with_type(cJSON *json, char *msg, const char *key)
{
    char buf[100];
    char *out;

    snprintf(buf, 100, "%s: ", key);
    strcat(msg, buf);
    out = cJSON_GetObjectItem(json, key)->valuestring;
	printf("%s\n", out);
    strcat(msg, out);
    strcat(msg, ";\n");
}

/* Read a file, parse, render back, etc. */
void doit(char *text, char *msg, char *program)
{
	char *out;
    cJSON *json;

    if (NULL == text || NULL == msg || NULL == program)
    {
        printf("input error in doit\n");
        exit(1);
    }
	
	json = cJSON_Parse(text);
	if (!json) 
    {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
    }
	else
	{
		out = cJSON_Print(json);
		printf("%s\n", out);
        get_value(json, program, "log_type");
        get_value_with_type(json, msg, "log_type");
        get_value_with_type(json, msg, "time");
        get_value_with_type(json, msg, "dev_ip");
        get_value_with_type(json, msg, "log_level");
        get_value_with_type(json, msg, "content");
        get_value_with_type(json, msg, "EXTEND_A");
        get_value_with_type(json, msg, "EXTEND_B");
        get_value_with_type(json, msg, "EXTEND_C");
		cJSON_Delete(json);
		free(out);
	}
}

/* Read a file, parse, render back, etc. */
void dofile(char *filename, char *msg, char *program)
{
	FILE *fp;
    long len;
    char *data;
	
    if (NULL == filename || NULL == msg || NULL == program)
    {
        printf("input error in doit\n");
        exit(1);
    }
	
	fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
	data = (char *)malloc(len + 1);
    fread(data, 1, len, fp);
    fclose(fp);
	doit(data, msg, program);
	free(data);
}

int connect_server_sendmsg(char *ip, unsigned short port, char *msg, int len)
{
    struct sockaddr_in server_addr;
    int client_sock_fd;
    int snd_ret;

    if (NULL == ip || port <= 0 || NULL == msg || len <= 0)
    {
        printf("input error in connect_server\n");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    if (0 == inet_aton(ip, &server_addr.sin_addr))
    {
        printf("bad address\n");
        exit(1);
    }
    server_addr.sin_port = htons(port);

    client_sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (client_sock_fd < 0)
    {
        perror("Create socket failed:");
        exit(1);
    }

    if ((snd_ret = sendto(client_sock_fd, msg, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr))) <= 0)
    {
        perror("Send message failed:");
        exit(1);
    }
    printf("send length: %d\n", snd_ret);
    close(client_sock_fd);

    return 0;
}

int main (int argc, char * argv[])
{
    char msg[MSG_BUF_LEN];
    char program[256];
    int ret = -1;
    int len;
    
    if (argc != 2)
    {
        printf("Usage: %s filename(in json type)\n", argv[0]);
        exit(0);
    }
    else if (0 != access(argv[0], R_OK))
    {
        printf("%s is not a file\n\n", argv[1]);
        printf("Usage: %s filename(in json type)\n", argv[0]);
        exit(1);
    }

    /* Parse standard testfiles: */
    dofile(argv[1], msg, program);
    len = strlen(msg);
#ifdef UDP
    printf("length: %d\ncontent: %s\n", len, msg);
    ret = connect_server_sendmsg(LOCAL, SYSLOG_PORT, msg, len);
    printf("ret = %d(0 send succ)\n", ret);
#endif
#ifdef SYSLOG
    openlog(program, LOG_CONS | LOG_PID, 0);
    syslog(LOG_USER | LOG_EMERG, msg);
    closelog();
#endif
	
    return 0;
}
