#include <sys/types.h> 
#include <signal.h> 
 
void f()
{
	while(1)
		kill(0, SIGTERM);
}

int main(void)
{
    f();

    return 0;
}
