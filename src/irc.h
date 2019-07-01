// vim: cin:sts=4:sw=4 
#ifndef IRC_H
#define IRC_H
#include <stdbool.h>

void* create_bot(char* irc_channel);
int connect_bot(void* session, char* addresses, bool use_ssl, char* nickname, char* password);
int disconnect_bot(void* session);
int destroy_bot(void* session);
int loop_bot(void* session);
int loop_bot2(int session_c, void** session_v);

#endif
