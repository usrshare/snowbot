#ifndef IRC_COMMON_H
#define IRC_COMMON_H
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

char* strrecat(char* orig, const char* append);

// RFC-accurate string comparison functions.
int ircstrcmp(const char* s1, const char* s2);
int ircstrncmp(const char* s1, const char* s2, size_t N);

int respond(irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict msg);
int ircprintf (irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict format, ...);

#endif
