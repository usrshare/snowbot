// vim: cin:sts=4:sw=4 
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <search.h>

#include <time.h>

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

struct irc_user_params{
//for hash tables. key is nickname.
    int bot_mode;
    char* current_wall_of_text;
};

int respond(irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict msg) {

    if (!channel) {
	return irc_cmd_msg(session,target,msg);
    } else { if (target) {
	int ml = strlen(msg) + 9 + 2 + 1;
	char newmsg[ml];
	snprintf(newmsg,ml,"%s: %s",target,msg);
	return irc_cmd_msg(session,channel,newmsg);
    } else return irc_cmd_msg(session,channel,msg); }
}

int ircvprintf (irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict format, va_list vargs) {

    char res[1024]; 

    int r = vsnprintf(res,1024,format,vargs);

    if (r > 1024) {

	char* outtext = malloc(r + 64);
	r = vsnprintf(res,r+64,format,vargs);
	r = respond(session,target,channel,outtext);
	free(outtext);
    } else {
	r = respond(session,target,channel,res);
    }

    return r; 
}

int ircprintf (irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict format, ...) {


    va_list vargs;
    va_start (vargs, format);

    int r = ircvprintf(session,target,channel,format,vargs);

    va_end(vargs);
    return r;
}

int handle_msg(irc_session_t* session, const char* restrict nick, const char* restrict channel, const char* restrict msg) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    if (strlen(msg) == 0) return 1;

    switch(msg[0]) {

	case '.': {
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

    return 0;
}

void count_msg(irc_session_t* session, const char* restrict nick, const char* restrict msg) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    if (strcmp(nick, ibp->msg_current_nickname) == 0) {

	ibp->cons_count++;
	ibp->cons_length += strlen(msg); 
    } else {
	ibp->cons_count = 0;
	ibp->cons_length = strlen(msg);
    }

}

void channel_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    struct irc_bot_params* ibp = irc_get_ctx(session);

    if (strncmp(params[1],ibp->irc_nickname,strlen(ibp->irc_nickname)) == 0) {
	// we're being highlighted. find the text.
	const char* cmd = params[1] + strlen(ibp->irc_nickname);
	while ((strlen(cmd) > 0) && (strchr(",: ",cmd[0])) ) cmd++; //skip delimiters.
	handle_msg(session, nick, params[0], cmd);
    } else {

	count_msg(session,nick,params[1]);
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

    if (strcmp(nick,ibp->irc_nickname) == 0) {
	printf("Joined channel %s.\n",params[0]);
	ibp->channel_joined = 1;
    }


}

void connect_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    printf("Successfully connected to the network.\n");
    struct irc_bot_params* ibp = irc_get_ctx(session);

    int r = irc_cmd_join( session, ibp->irc_channel, 0);
    if (r != 0) printf("Can't join %s: %s\n",ibp->irc_channel,irc_strerror(irc_errno(session)));
    
    r = hcreate(256);
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
    callbacks.event_channel = channel_cb;
    callbacks.event_privmsg = privmsg_cb;

    irc_session_t* session = irc_create_session (&callbacks);
    irc_set_ctx(session,new_params);

    return session;
}

int connect_bot(void* session, char* address, int port, bool use_ssl, char* nickname) {

    struct irc_bot_params* ibp = irc_get_ctx(session);
    ibp->irc_nickname = nickname;

    char address_copy[128];

    if (use_ssl) {
	strcpy(address_copy,"#");
	strncat(address_copy,address,126);
    } else strncpy(address_copy,address,127);

    int r = irc_connect(session,address_copy,port,NULL,nickname,NULL,"snowbot");
    if (r != 0) fprintf(stderr,"IRC connection error: %s.\n",irc_strerror(irc_errno(session)));
    return r;
}

int loop_bot(void* session) {

    irc_run(session);
    return 0;
}


int disconnect_bot(void* session) {

    free(irc_get_ctx(session));
    irc_disconnect(session);
    hdestroy();
    return 0;
}
