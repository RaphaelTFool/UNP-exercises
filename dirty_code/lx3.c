#include <syslog.h> 
 
void f()  
{  
    openlog("ERROR", LOG_CONS | LOG_PID, 0);  
    syslog(LOG_USER | LOG_INFO, "Your System have big program by problem syslog\n");
    closelog();   
}

int main(void)
{
    f();

    return 0;
}
