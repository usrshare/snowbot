#ifndef IRC_COMMON_H
#define IRC_COMMON_H

#include <stdlib.h>
#include <stdbool.h>
#include "irc_proto.h"
//#include <libircclient.h>
//#include <libirc_rfcnumeric.h>

char* strrecat(char* orig, const char* append);

int cnt_tokens (const char* restrict string, const char* delim);
// RFC-accurate string comparison functions.
int ircstrcmp(const char* s1, const char* s2);
int ircstrncmp(const char* s1, const char* s2, size_t N);

int respond(irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict msg);

int idprintf (irc_session_t* session, const char* restrict format, ...);

int ircprintf (irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict format, ...);

int decode_ctcp(const char* restrict msg, char* o_msg);
int encode_ctcp(const char* restrict msg, char* o_msg);

int ison_request(irc_session_t* session, int count, const char** nicknames, bool* o_statuses);

#endif
