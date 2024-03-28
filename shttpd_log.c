#include "shttpd_log.h"

void Log(const char *identity,int priority,const char *format,...)
{
    openlog(identity,LOG_CONS|LOG_PID,LOG_USER);
    va_list args;
    va_start(args,format);
    vsyslog(priority,format,args);
    va_end(args);
    closelog();
}

