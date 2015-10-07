// vim: cin:sts=4:sw=4 
#ifndef IRC_WATCH_H
#define IRC_WATCH_H

#define WATCHLEN 1024

int watch_addmsg(const char* restrict nickname, const char* restrict msg);
unsigned int watch_getlength(const char* restrict nickname, unsigned int seconds);

#endif
