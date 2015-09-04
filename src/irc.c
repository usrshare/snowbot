// vim: cin:sts=4:sw=4 
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <string.h>

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

    int msg_cursor;
    char msg_nickname[ACTIVITY_CIRCBUF_SIZE][10]; //9 bytes for std irc
    unsigned int msg_length[ACTIVITY_CIRCBUF_SIZE]; //msg length
    time_t msg_time[ACTIVITY_CIRCBUF_SIZE]; //msg time	


    char notify_users[NOTIFY_USERS_MAX][160]; //this is a huge list of users to notify, separated by commas. enough for 16 users
    char notify_targets[NOTIFY_USERS_MAX][10]; //9 bytes for std irc
    unsigned int notify_lengths[NOTIFY_USERS_MAX]; //user activity in bytes
    bool notify_overactive[NOTIFY_USERS_MAX]; //is the user overly active?
    time_t last_activity[NOTIFY_USERS_MAX]; //last activity for the user
};

int add_watch(struct irc_bot_params* ibp, int i, const char* nick) {

    char nick_cut[10];
    strncpy(nick_cut,nick,9);
    if ((strlen(nick_cut) + strlen(ibp->notify_users[i]) + 1) > 160) return 1; //list overflow

    strncat(nick_cut,",",1); //delimiter

    if (strstr(ibp->notify_users[i],nick_cut)) return 2; //nickname already in list 

    strncat(ibp->notify_users[i],nick_cut,10); //added.

    printf("New notify list for %s: %s\n",ibp->notify_targets[i],ibp->notify_users[i]);
    return 0;

}

int remove_watch(struct irc_bot_params* ibp, int i, const char* nick) {

    char nick_cut[10];
    strncpy(nick_cut,nick,9);

    strncat(nick_cut,",",1); //delimiter

    char* fragment = strstr(ibp->notify_users[i],nick_cut); //find where exactly the nickname is used.

    if (!fragment) return 1; //nickname not in list 

    char new_users[160];

    strncpy(new_users,ibp->notify_users[i], (fragment - ibp->notify_users[i]));
    strcat(new_users,fragment + strlen(nick_cut));

    strncpy(ibp->notify_users[i],new_users,160);
    printf("New notify list for %s: %s\n",ibp->notify_targets[i],ibp->notify_users[i]);
    return 0;
}

unsigned int count_activity (struct irc_bot_params* ibp, char* nick) {

    unsigned int bytes = 0;

    for (int i=0; i < ACTIVITY_CIRCBUF_SIZE; i++)
	if (strcmp(nick,ibp->msg_nickname[i]) == 0)
	    bytes += ibp->msg_length[i];

    return bytes;
}

int get_target_index(struct irc_bot_params* ibp, const char* nick) {

    for (int i=0; i < NOTIFY_USERS_MAX; i++)
	if (strncmp(nick,ibp->notify_targets[i],9) == 0)
	    return i;

    return -1;
}

int get_empty_index(struct irc_bot_params* ibp) {

    for (int i=0; i < NOTIFY_USERS_MAX; i++)
	if (strlen(ibp->notify_targets[i]) == 0)
	    return i;

    return -1;
}

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

	case '+': {
		      const char* target = msg+1;
		      printf("User %s asked to add %s to watchlist.\n",nick,target);
		      int target_i = get_target_index(ibp,target);
		      if (target_i != -1) {

			  int r = add_watch(ibp,target_i,nick);

			  switch (r) {

			      case 0: ircprintf(session,nick,channel,"You are now following %s's activity.",target); break;
			      case 1: ircprintf(session,nick,channel,"The follower list of %s is too long. Blame usr_share.",target); break;
			      case 2: ircprintf(session,nick,channel,"You are already following %s.",target); break;
			  }
			  return 1;
		      }

		      int empty_i = get_empty_index(ibp);
		      if (empty_i == -1) {

			  ircprintf(session,nick,channel,"Global watch list too long. Unable to add %s. Blame usr_share.",target);
			  return 1;
		      }

		      add_watch(ibp,empty_i,nick);
		      strncpy(ibp->notify_targets[empty_i],target,9);

		      ircprintf(session,nick,channel,"You are now following %s's activity.",target);
		      return 0;

		      break; }
	case '-': {
		      const char* target = msg+1;
		      printf("User %s asked to remove %s from watchlist.\n",nick,target);

		      int target_i = get_target_index(ibp,target);

		      if (target_i != -1) {
			  ircprintf(session,nick,channel,"You aren't following %s's activity.");
			  return 1;
		      }

		      int r = remove_watch(ibp,target_i,nick);

		      switch (r) {

			  case 0: ircprintf(session,nick,channel,"You aren't following %s's activity.",target); break;
			  case 1: ircprintf(session,nick,channel,"You weren't following %s's activity in the first place.",target); break;
		      }

		      if (strlen(ibp->notify_users[target_i]) == 0)
			  ibp->notify_targets[target_i][0] = 0; //if nobody is watching this user, remove them.

		      break; }
	case '?': {
		      /*                           123456789012345678901234567890123456789012345678901234567890*/
		      ircprintf(session,nick,NULL,"The + and - commands allow you to be notified of whenever ");
		      ircprintf(session,nick,NULL,"other users have stopped being excessively active on this  ");
		      ircprintf(session,nick,NULL,"channel. If a user starts posting huge walls of text or ");
		      ircprintf(session,nick,NULL,"creates messages too quickly, you're free to ignore the ");
		      ircprintf(session,nick,NULL,"channel. When the user stops, this bot will write a private ");
		      ircprintf(session,nick,NULL,"message informing that you're free to talk on the channel ");
		      ircprintf(session,nick,NULL,"again. ");
		      ircprintf(session,nick,NULL,"To follow a user's activity, send this bot a private message ");
		      ircprintf(session,nick,NULL,"consisting of a + (plus) sign, followed by their nickname, ");
		      ircprintf(session,nick,NULL,"(e.g. +usr_share).");
		      ircprintf(session,nick,NULL,"To stop following a user's activity, send a private message ");
		      ircprintf(session,nick,NULL,"consisting of a - (minus) sign, followed by their nickname, ");
		      ircprintf(session,nick,NULL,"(e.g. -usr_share).");
		      ircprintf(session,nick,NULL,"To find out whose activity you're currently following, ");
		      ircprintf(session,nick,NULL,"send a private message consisting of an * (asterisk) symbol. ");

		      break; }
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
    int cur = ibp->msg_cursor;

    // if the buffer if filled, whose message are we overwriting now?

    int rewr_i = get_target_index(ibp,ibp->msg_nickname[cur]);
    int target_i = get_target_index(ibp,nick);

    if (rewr_i != -1) {
	ibp->notify_lengths[rewr_i] -= ibp->msg_length[cur];
    }

    if (target_i != -1) {
	ibp->notify_lengths[target_i] += strlen(msg);
	ibp->last_activity[target_i] = time(NULL);
    }

    if (rewr_i != -1) {
	if ((ibp->notify_overactive[rewr_i]) && (ibp->notify_lengths[rewr_i] < WALL_ENDS))
	{ ibp->notify_overactive[rewr_i] = 0;
	    printf("Wall of text over for %s.\n",ibp->notify_targets[rewr_i]);
	}
    }

    if (target_i != -1) {
	if ((!ibp->notify_overactive[target_i]) && (ibp->notify_lengths[target_i] >= WALL_BEGINS))
	{ ibp->notify_overactive[target_i] = 1;
	    printf("Wall of text starts for %s.\n",ibp->notify_targets[target_i]);
	}
    }

    strncpy(ibp->msg_nickname[cur],nick,10);
    ibp->msg_length[cur] = strlen(msg);
    ibp->msg_time[cur] = time(NULL);

    ibp->msg_cursor = (cur+1) % ACTIVITY_CIRCBUF_SIZE; //cycle
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
    printf("[   !!PRIVATE!!] [%10s]:%s\n",nick,params[1]);

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
    return 0;
}
