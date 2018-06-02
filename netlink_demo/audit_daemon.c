#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <fcntl.h>
#include <asm/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pwd.h>

#define TM_FMT "%Y-%m-%d %H:%M:%S"
#define NETLINK_TEST 29
#define MAX_PAYLOAD 1024

int sock_fd;
struct msghdr msg;
struct nlmsghdr *nlh = NULL;
struct sockaddr_nl src_addr, dest_addr;
struct iovec iov;
FILE *logfile;

void write_log(char *command_name, int uid, int pid, char *filename, int flags, int ret)
{
	char logtime[64];
	char username[32];
	struct passwd *pwinfo;
	char openresult[10];
	char opentype[16];

	if (ret > 0)
		strcpy(openresult, "success");
	else
		strcpy(openresult, "failure");

	if (flags & O_RDONLY)
		strcpy(opentype, "READ");
	else if (flags & O_WRONLY)
		strcpy(opentype, "WRITE");
	else if (flags & O_RDWR)
		strcpy(opentype, "READ&WRITE");
	else
		strcpy(opentype, "other");

	time_t t = time(NULL);

	if (NULL == logfile)
		return;

	pwinfo = getpwuid(uid);
	strcpy(username, pwinfo->pw_name);
	strftime(logtime, sizeof(logtime), TM_FMT, localtime(&t));
	fprintf(logfile, "[%s]%s(%d) %s(%d) \"%s\" %s %s\n", logtime, username, uid, command_name, pid, filename, opentype, openresult);
	printf("[%s]%s(%d) %s(%d) \"%s\" %s %s\n", logtime, username, uid, command_name, pid, filename, opentype, openresult);

	return;
}

void send_pid(unsigned int pid)
{
	memset(&msg, 0, sizeof(msg));
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = pid;
	src_addr.nl_groups = 0;
	if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0)
	{
		perror("bind");
		exit(0);
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;
	dest_addr.nl_groups = 0;

	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = pid;
	nlh->nlmsg_flags = 0;

	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void *) &dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(sock_fd, &msg, 0);

	return;
}

void term_handler()
{
	printf("The process is being killed\n");
	if (logfile)
		fclose(logfile);
	if (nlh)
		free(nlh);

	exit(0);
}

int main(int argc, char ** argv)
{
	char buff[128];
	void term_handler();
	char logpath[256];
	pid_t self;

	unsigned int uid, pid, flags, ret;
	char *filename;
	char *command_name;
	void *data;

	if (argc == 1)
		strcpy(logpath, "./log");
	else if (argc == 2)
		strncpy(logpath, argv[1], 256);
	else
	{
		printf("Usage: %s [log path name]\n", argv[0]);
		exit(1);
	}

	if (SIG_ERR == signal(SIGTERM, term_handler))
	{
		perror("signal");
		exit(1);
	}

	if ((sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST)) < 0)
	{
		perror("socket");
		exit(1);
	}

	if (!(nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD))))
	{
		printf("failed to malloc\n");
		exit(1);
	}
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	self = getpid();
	printf("This Process's pid: %u\n", self);
	send_pid(self);

	logfile = fopen(logpath, "w+");
	if (!logfile)
	{
		printf("cannot create log file\n");
		exit(1);
	}

	while (1)
	{
		if (recvmsg(sock_fd, &msg, 0) == 0)
		{
			printf("receive a msg from netlink\n");
			data = (void *)NLMSG_DATA(nlh);
			uid = *((unsigned int *)data);
			pid = *((int *)data + 1);
			flags = *((int *)data + 2);
			ret = *((int *)data + 3);
			command_name = (char *)((int *)data + 4);
			filename = (char *)((int *)data + 4 + 16 / 4);
			printf("try to write a log into log file %s\n", logfile);
			write_log(command_name, uid, pid, filename, flags, ret);
		}
		else
		{
			perror("recvmsg");
		}
	}
	close(sock_fd);
	free(nlh);
	fclose(logfile);

	return 0;
}
