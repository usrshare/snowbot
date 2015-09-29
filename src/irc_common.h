#ifndef IRC_COMMON_H
#define IRC_COMMON_H
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

char* strrecat(char* orig, const char* append);

int respond(irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict msg);
int ircprintf (irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict format, ...);

#endif
