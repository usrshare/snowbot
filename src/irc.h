// vim: cin:sts=4:sw=4 
#ifndef IRC_H
#define IRC_H

void* create_bot(char* irc_channel);
int connect_bot(void* session, char* address, int port, bool use_ssl, char* nickname);
int disconnect_bot(void* session);
int loop_bot(void* session);

#endif
