// vim: cin:sts=4:sw=4 
#ifndef IRC_WATCH_H
#define IRC_WATCH_H

#define WATCHLEN 1024
#include <time.h>

int watch_addmsg(const char* restrict nickname, const char* restrict channel, const char* restrict msg);
unsigned int watch_getlength(const char* restrict nickname, const char* restrict channel, time_t time_min, time_t time_max);
unsigned int watch_countmsg();

#endif
