#include "irc_watch.h"
// vim: cin:sts=4:sw=4 
#include <time.h>
#include <string.h>

struct watch_msgs {
	char nickname[10];
	unsigned int length;
	time_t time;
};

#define WATCHLEN 100
struct watch_msgs watch[WATCHLEN];
unsigned int newmsg_i = 0;

int watch_addmsg(const char* restrict nickname, const char* restrict msg) {

	strncpy(watch[newmsg_i].nickname, nickname,10);
	
	watch[newmsg_i].length = strlen(msg); //replace with UTF-8 based length?
	newmsg_i = ((newmsg_i+1) % WATCHLEN);
	watch[newmsg_i].time = time(NULL);
	return 0;
}

unsigned int watch_countmsg() {

    unsigned int res = 0;
    for (int i=0; i < WATCHLEN; i++)
	if (watch[i].time != 0) res++;

    return res;
}

unsigned int watch_getlength(const char* restrict nickname, unsigned int seconds) {
	
	unsigned int res = 0;
	time_t curtime = time(NULL);

	for (int i=0; i < WATCHLEN; i++) {
		if ((seconds) && ( (curtime - watch[i].time) > seconds) ) continue;
		if (strncmp(nickname,watch[i].nickname,9) == 0) res += watch[i].length;
	}

	return res;
}
