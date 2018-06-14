#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/ioctl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<net/if.h>

int f()
{
    char ip[16] = { 0 };
    struct ifreq temp;
    struct sockaddr_in *addr;
    int fd = 0;
    int ret = -1;
    strcpy(temp.ifr_name, "eth0");
    if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
    {
      return -1;
    }
    ret = ioctl(fd, SIOCGIFADDR, &temp);
    close(fd);
    if(ret < 0)
       return -1;
    addr = (struct sockaddr_in *)&(temp.ifr_addr);
    strcpy(ip, inet_ntoa(addr->sin_addr));
    printf("%s\n", ip);

    return 0;
}

int main(int argc, char **argv)
{
    f();

    return 0;
}
