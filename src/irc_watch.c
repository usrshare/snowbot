// vim: cin:sts=4:sw=4 
#include "irc_watch.h"
#include "irc_common.h"

#include <time.h>
#include <string.h>
#include <strings.h>

const char* bs[] = {
    "breitbart",
    "dailymail",
    "wnd",
    "infowars",
    "rt",
    "sputniknews",
    "sputnikne", //.ws
};

struct watch_msgs {
	char nickname[10];
	char channel[10]; //for purposes of this thing.
	unsigned int length;
	unsigned int wordcount;
	unsigned int bscount;
	time_t time;
};

struct watch_msgs watch[WATCHLEN];
unsigned int newmsg_i = 0;

#define WORDDELIM " ,.:;/\\?!\"[]{}()_-+=@#$%^&*~`|<>" // single apostrophe not included, because of contractions.

int watch_addmsg(const char* restrict nickname, const char* restrict channel, const char* restrict msg) {

	strncpy(watch[newmsg_i].nickname, nickname,10);
	strncpy(watch[newmsg_i].channel, channel,10);
	
	watch[newmsg_i].length = strlen(msg); //replace with UTF-8 based length?
	newmsg_i = ((newmsg_i+1) % WATCHLEN);
	watch[newmsg_i].time = time(NULL);

	char* msgcpy = strdup(msg);

	char* saveptr = NULL;

	char* msgword = strtok_r(msgcpy,WORDDELIM,&saveptr);

	while (msgword) {

	    watch[newmsg_i].wordcount++;

	    for (unsigned int i=0; i < (sizeof(bs) / sizeof(*bs)); i++)
		    if (strcasecmp(msgword,bs[i]) == 0) watch[newmsg_i].bscount++;
	    
	    msgword = strtok_r(NULL,WORDDELIM,&saveptr);
	}

	free(msgcpy);
	return 0;
}

unsigned int watch_countmsg() {

    unsigned int res = 0;
    for (int i=0; i < WATCHLEN; i++)
	if (watch[i].time != 0) res++;

    return res;
}

unsigned int count_by_period(const char* restrict nickname, time_t start, time_t interval_s, size_t bcount_c, int* bcount_v) {
	
	for (int i=0; i < WATCHLEN; i++) {
		if (watch[i].time < start) continue;
		if (watch[i].time > (start + (time_t)(interval_s*bcount_c)) ) continue;
		if (ircstrncmp(nickname,watch[i].nickname,9) == 0) {
		
		 int x = (watch[i].time - start) / interval_s;
		 if ((unsigned int)x < bcount_c) bcount_v[x] += watch[i].length; //if it's <0, it'll be too large.
		}
	}

	return 0;
}

unsigned int watch_getlength(const char* restrict nickname, const char* restrict channel, time_t time_min, time_t time_max) {
	
	unsigned int res = 0;

	for (int i=0; i < WATCHLEN; i++) {
		if ((time_min) && (watch[i].time < time_min)) continue;
		if ((time_max) && (watch[i].time > time_max)) continue;
		if ((channel) && (strncmp(watch[i].channel,channel,10) != 0)) continue;
		if ((!nickname) || (ircstrncmp(nickname,watch[i].nickname,9) == 0)) res += watch[i].length;
	}

	return res;
}
