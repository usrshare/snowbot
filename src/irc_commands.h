#ifndef IRC_COMMANDS_H
#define IRC_COMMANDS_H
#include <libircclient.h>

int handle_commands(irc_session_t* session, const char* restrict nick,const char* restrict channel, const char* cmd);

#endif
