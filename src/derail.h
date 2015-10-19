// vim: cin:sts=4:sw=4 
#ifndef DERAIL_H
#define DERAIL_H
#include "irc_common.h"

int insert_sug(const char* restrict nickname, const char* restrict text);
int derail_addmsg(irc_session_t* session,const char* restrict nickname, const char* restrict channel, const char* restrict msg);

#endif
