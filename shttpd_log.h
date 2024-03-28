#ifndef __SHTTPD_LOG_H
#define __SHTTPD_LOG_H

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>

void Log(const char *identity,int priority,const char *format,...);

#endif