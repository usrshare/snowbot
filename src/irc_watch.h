// vim: cin:sts=4:sw=4 
#ifndef IRC_WATCH_H
#define IRC_WATCH_H

#define WATCHLEN 1024
#include <time.h>

int watch_addmsg(const char* restrict nickname, const char* restrict channel, const char* restrict msg);
unsigned int count_by_period(const char* restrict nickname, time_t start, time_t interval_s, size_t bcount_c, int* bcount_v);
unsigned int watch_getlength(const char* restrict nickname, const char* restrict channel, time_t time_min, time_t time_max, unsigned int* wc, unsigned int* bs);
unsigned int watch_countmsg();

void watch_save(void);
void watch_load(void);
#endif
