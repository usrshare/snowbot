// vim: cin:sts=4:sw=4 
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <search.h>

#include <math.h>
#include <stddef.h>
#include <time.h>

#include "derail.h"
#include "irc_watch.h"
#include "irc_commands.h"
#include "irc_common.h"
#include "irc_user.h"

#include "paste.h"
#include "weather.h"

irc_callbacks_t callbacks;

#define ACTIVITY_CIRCBUF_SIZE 20

#define NOTIFY_USERS_MAX 16

#define WALL_BEGINS 200
#define WALL_ENDS 100


struct irc_bot_params{

    char* irc_channel;
    char* irc_nickname;
    bool channel_joined;

    char msg_current_nickname[10]; //nickname of user who posted last message
    unsigned int cons_count; //number of consecutive messages from that user.
    unsigned int cons_length; //length of consecutive messages from that user.

};

int add_paste_line(irc_session_t* session, struct irc_user_params* up, const char* restrict string) {

    size_t cursize = (up->paste_text) ? strlen(up->paste_text) : 0;
    up->paste_size = cursize + 2 + strlen(string);
    up->paste_text = realloc(up->paste_text,up->paste_size);

    if (cursize) {
	strcpy(up->paste_text + cursize,"\n");
	strcat(up->paste_text,string); }
    else strcpy(up->paste_text,string);

    return 0;
}


int handle_msg(irc_session_t* session, const char* restrict nick, const char* restrict channel, const char* restrict msg) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    if (strlen(msg) == 0) return 1;

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    int r = handle_commands(session,nick,channel,msg); 
    if (r == 0) return 0;

    switch(up->mode) {
	case BM_NONE:

	    switch(msg[0]) {
		case 'k': {
			      if (strncmp(msg, "kill all humans",15) == 0) respond(session,nick,channel,"kill all humans!!");
			      break;}
		case 'h': {
			      if (strcmp(msg, "hello") == 0) respond(session,nick,channel,"hi");
			      break;}
		case 'p': {
			      if (strcmp(msg, "ping") == 0) respond(session,nick,channel,"pong");
			      break;}
		default: {
			     break; }
	    }
	    break;
	case BM_PASTE:
	    switch(msg[0]) {
		case '.': {
			      if (strlen(msg) == 1) {
				  char* resurl = upload_to_pastebin(nick,up->paste_title ? up->paste_title : "snowbot paste",up->paste_text);	
				  free(up->paste_text);
				  up->paste_text = NULL;
				  up->paste_size = 0;
				  respond(session,nick,channel,resurl);
				  up->mode = BM_NONE;
				  free(resurl);
			      } else {
				  if (msg[1] == '.') add_paste_line(session,up,msg+1);
			      }

			      break; }
		default: {
			     add_paste_line(session,up,msg);
			     break; }
	    }
	    break;
    }

    return 0;
}

void count_msg(irc_session_t* session, const char* restrict nick, const char* restrict channel, const char* restrict msg) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    watch_addmsg(nick,channel,msg);
    derail_addmsg(session,nick,channel,msg);
   
    if (ircstrcmp(nick, ibp->msg_current_nickname) == 0) {

	ibp->cons_count++;
	ibp->cons_length += strlen(msg); 
    } else {
	ibp->cons_count = 0;
	ibp->cons_length = strlen(msg);
    }

}

void quit_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    printf("User %s quit the server.\n",nick);

    struct irc_user_params* up = get_user_params(nick, EB_NULL);

    if (up) {
	save_user_params(nick,up);
	del_user_params(nick,up);
    }

}

void channel_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    struct irc_bot_params* ibp = irc_get_ctx(session);

    bool handle_this_message = false;
    const char* handle_ptr = params[1];

    if (strncmp(params[1],".",1) == 0) handle_this_message = true;

    if (strncmp(params[1],ibp->irc_nickname,strlen(ibp->irc_nickname)) == 0) {
	handle_this_message = true;
	handle_ptr = params[1] + strlen(ibp->irc_nickname);
	while ((strlen(handle_ptr) > 0) && (strchr(",: ",handle_ptr[0])) ) handle_ptr++; //skip delimiters.
    }

    if (handle_this_message) {
	handle_msg(session, nick, params[0], handle_ptr);
    } else {

	if (params[0] != NULL) count_msg(session,nick,params[0],params[1]);
    }
}

void privmsg_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];

    irc_target_get_nick(origin,nick,10);
    //printf("[   !!PRIVATE!!] [%10s]:%s\n",nick,params[1]);

    handle_msg(session,nick,NULL,params[1]); // handle private message


}

void join_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    if (ircstrcmp(nick,ibp->irc_nickname) == 0) {
	printf("Joined channel %s.\n",params[0]);
	ibp->channel_joined = 1;
    }


}

void connect_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    printf("Successfully connected to the network.\n");
	
    watch_load();
    atexit(watch_save);

    struct irc_bot_params* ibp = irc_get_ctx(session);

    char* saveptr = NULL;
    char* chantok = strtok_r(ibp->irc_channel,",",&saveptr);

    int r = 0;

    while (chantok) {
	r = irc_cmd_join( session, chantok, 0);
	if (r != 0) printf("Can't join %s: %s\n",chantok,irc_strerror(irc_errno(session)));
	chantok = strtok_r(NULL,",",&saveptr);
    }


    r = hcreate(512);
    if (r == 0) perror("Can't create user hashtable");
}

void numeric_cb(irc_session_t* session, unsigned int event, const char* origin, const char** params, unsigned int count) {

    switch(event) {

	case 1: //welcome
	case 2: //your host
	case 3: //created
	case 4: //information

	case 5: //bounce?

	case 251: // user, service and server count.
	case 252: // operator count
	case 253: // unknown connection count
	case 254: // channel count
	case 255: // client and server count

	case 265: // + local user count 
	case 266: // + global user count
	case 250: // + connection count

	case 332: // topic
	case 333: // + who set the topic and when
	case 353: // names list
	case 366: // names list end

	case 375: // MOTD start
	case 372: // MOTD
	case 376: // MOTD end
	    break;
	default:
	    printf("Received event %u with %u parameters:\n",event,count);
	    for (unsigned int i = 0; i < count; i++)
		printf("Param %3u: %s\n",i,params[i]);
	    break;
	    break;
    }

}

void* create_bot(char* irc_channel) {
    memset( &callbacks, 0, sizeof(callbacks));

    struct irc_bot_params* new_params = malloc(sizeof(struct irc_bot_params));
    if (!new_params) {
	perror("Unable to allocate memory for bot structure");
	return NULL;
    }

    memset(new_params,0,sizeof(struct irc_bot_params));

    new_params->irc_channel = irc_channel;

    callbacks.event_connect = connect_cb;
    callbacks.event_numeric = numeric_cb;
    callbacks.event_join = join_cb;
    callbacks.event_quit = quit_cb;
    callbacks.event_channel = channel_cb;
    callbacks.event_privmsg = privmsg_cb;

    irc_session_t* session = irc_create_session (&callbacks);
    irc_set_ctx(session,new_params);

    return session;
}

int connect_bot(void* session, char* address, int port, bool use_ssl, char* nickname, char* password) {

    struct irc_bot_params* ibp = irc_get_ctx(session);
    ibp->irc_nickname = nickname;

    char address_copy[128];

    if (use_ssl) {
	strcpy(address_copy,"#");
	strncat(address_copy,address,126);
    } else strncpy(address_copy,address,127);

    int r = irc_connect(session,address_copy,port,password,nickname,NULL,"snowbot");
    if (r != 0) fprintf(stderr,"IRC connection error: %s.\n",irc_strerror(irc_errno(session)));
    return r;
}

int loop_bot(void* session) {

    int r = irc_run(session);
    if (r != 0) fprintf(stderr,"irc_run error: %s.\n",irc_strerror(irc_errno(session)));
    printf("Terminating.\n");
    return 0;
}


int disconnect_bot(void* session) {

    free(irc_get_ctx(session));
    irc_disconnect(session);
    hdestroy();
    return 0;
}
