#include "irc_user.h"
#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "config.h"

struct hashtable* userht = NULL;

struct saveparam irc_save_params[] = {
    {"wmode",ST_UINT32,0,offsetof(struct irc_user_params,wmode),SV_VISIBLE,false},
    {"cityid",ST_UINT32,0,offsetof(struct irc_user_params,cityid),SV_VISIBLE,false},
    {"usegraphics",ST_UINT8,0,offsetof(struct irc_user_params,usegraphics),SV_VISIBLE,false},
    {"color_scheme",ST_UINT8,0,offsetof(struct irc_user_params,color_scheme),SV_VISIBLE,false},
    {"use_dark_colors",ST_UINT8,0,offsetof(struct irc_user_params,color_scheme),SV_HIDDEN,false}, //old name for same param
    {"pwdhash",ST_STRING,129,offsetof(struct irc_user_params,pwdhash),SV_HIDDEN,false}
};

const size_t paramcnt = sizeof(irc_save_params) / sizeof(*irc_save_params);

int save_user_params(const char* restrict nick, struct irc_user_params* up) {

    if (!(up->modified)) return 1; //no need to save

    char filename[16];
    snprintf(filename,16,"%.9s.dat",nick);
    irctolower(filename,16);

    return savedata(filename,up,irc_save_params,(sizeof(irc_save_params) / sizeof(*irc_save_params)) ); 
}

int load_user_params(const char* restrict nick, struct irc_user_params* up) {

    char filename[16];
    snprintf(filename,16,"%.9s.dat",nick);
    irctolower(filename,16);

    int r = loaddata(filename,up,irc_save_params,(sizeof(irc_save_params) / sizeof(*irc_save_params)) ); 
    up->modified = false;
    return r;
}


struct irc_user_params* get_user_params(const char* restrict nick, enum empty_beh add_if_empty) {

    if (!userht) { userht = ht_create(128);
	atexit(saveall);
    }

    char ircnick[IRC_MAX_NICK_LEN+1];
    strncpy(ircnick,nick,IRC_MAX_NICK_LEN+1);
    irctolower(ircnick,IRC_MAX_NICK_LEN+1);

    void* p = ht_search(userht,ircnick);
    if (p) return p;
    if (!add_if_empty) return NULL;

    struct irc_user_params* newparams = malloc(sizeof(struct irc_user_params));
    memset(newparams,0,sizeof(struct irc_user_params));

    if (add_if_empty == EB_LOAD) load_user_params(nick,newparams);

    int r = ht_insert(userht,ircnick,newparams);

    return newparams;
}


int del_user_params(const char* restrict nick, struct irc_user_params* value) {

    if (!userht) return 1;
    
    char ircnick[IRC_MAX_NICK_LEN+1];
    strncpy(ircnick,nick,IRC_MAX_NICK_LEN+1);
    irctolower(ircnick,IRC_MAX_NICK_LEN+1);

    void* p = ht_search(userht,ircnick);
    if (!p) return 1;

    free(p);
    ht_delete(userht,ircnick);

    return 0;
}

int saveall_htcb(const char* restrict key, void* value, void* ctx) {
	save_user_params(key,(struct irc_user_params*) value);
	return 0;
}

void saveall() {

	ht_all(userht,saveall_htcb,NULL);
	return;
}
