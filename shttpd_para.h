#ifndef __SHTTPD_PARA_H
#define __SHTTPD_PARA_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

void display_usage(void);
void display_para(void);
void Para_CmdParse(int argc, char *argv[]);
void Para_Init(int argc, char *argv[]);

#endif