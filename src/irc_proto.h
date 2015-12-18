// vim: cin:sts=4:sw=4 
#ifndef IRC_PROTO_H
#define IRC_PROTO_H

#include <stdlib.h>
struct irc_session_t;
typedef struct irc_session irc_session_t;

typedef void (*irc_event_cb) (irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count);
typedef void (*irc_numeric_cb)(irc_session_t* session, unsigned int event, const char* origin, const char** params, unsigned int count);

struct irc_callbacks {
	irc_event_cb event_connect;
	irc_numeric_cb event_numeric;
	irc_event_cb event_join;
	irc_event_cb event_quit;
	irc_event_cb event_part;
	irc_event_cb event_channel;
	irc_event_cb event_privmsg;
};

typedef struct irc_callbacks irc_callbacks_t;

irc_session_t* irc_create_session(irc_callbacks_t* callbacks);

int irc_connect(irc_session_t* session, const char* restrict address, int port, const char* password, const char* nickname, const char* username, const char* realname);

int irc_run(irc_session_t* session);
int irc_disconnect(irc_session_t* session);

int irc_raw_sendf(irc_session_t* session, const char* fmt, ...);

int irc_cmd_msg(irc_session_t* session, const char* restrict target, const char* restrict msg);
int irc_cmd_notice(irc_session_t* session, const char* restrict target, const char* restrict msg);

int irc_target_get_nick(const char* restrict origin, char* o_nick, size_t o_nick_sz);

int irc_cmd_join(irc_session_t* session, const char* restrict channel, const char* restrict key); 

int irc_set_ctx(irc_session_t* session, void* ctx);
void* irc_get_ctx(irc_session_t* session);
#endif

