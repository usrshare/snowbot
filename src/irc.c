// vim: cin:sts=4:sw=4 
//#include <libircclient.h>
//#include <libirc_rfcnumeric.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <search.h>

#include <pthread.h>
#include <math.h>
#include <stddef.h>
#include <time.h>

#include "config.h"

#include "boyer-moore.h"
#include "irc_proto.h"

#include "derail.h"
#include "short.h"
#include "irc_watch.h"
#include "irc_commands.h"
#include "irc_common.h"
#include "irc_user.h"

#include "entities.h"
#include "paste.h"
#include "weather.h"

#define ACTIVITY_CIRCBUF_SIZE 20

#define NOTIFY_USERS_MAX 16

#define WALL_BEGINS 200
#define WALL_ENDS 100

int save_initialized = 0;
int save_atexited = 0;

#if ENABLE_URL_NOTIFY 

char* notify_url_list = NULL;
size_t notify_url_len = 0;

#endif

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

bool validchar(char cchar) {

    if (!cchar) return false;
    if ((cchar >= '0') && (cchar <= '9')) return true;
    if ((cchar >= 'a') && (cchar <= 'z')) return true;
    if ((cchar >= 'A') && (cchar <= 'Z')) return true;
    if (strchr("+-&@#/%?=~_()|!:,.;",cchar)) return true;
    if (cchar < 0) return true; //unicode shmunicode
    return false;
}

char* find_url(const char* restrict msg, const char** msgend) {

    char* proto = NULL;

    /*       */ proto = strstr(msg,"https:");
    if (!proto) proto = strstr(msg,"http:");
    if (!proto) proto = strstr(msg,"ftp:");
    if (!proto) proto = strstr(msg,"sftp:");
    if (!proto) { if (msgend) *msgend = NULL; return NULL; }

    char* cchar = proto; 

    if (strncmp(cchar,"//",2) == 0) cchar+=2;

    size_t dnsize=0;

    while (validchar(*cchar)) { cchar++; dnsize++; }

    if ((proto > msg) && (*(proto-1) == '(') && (*(cchar-1) == ')')) cchar--;

    if (dnsize == 0) {return NULL;}
    if (msgend) *msgend = cchar;
    return proto; 
}


#ifdef __DATE__
#define COMPILEDATE __DATE__
#else
#define COMPILEDATE "unknown"
#endif

int handle_ctcp(irc_session_t* session, const char* restrict nick, const char* restrict msg) {

    printf("[CTCP][%10s]:%s\n",nick,msg);

    if (strcmp(msg,"VERSION") == 0) {

	//version

	char ctcpresp[512];
	encode_ctcp("VERSION snowbot:" COMPILEDATE ":unknown",ctcpresp);
	irc_cmd_notice(session,nick,ctcpresp);
    }

    return 0;
}


int handle_msg(irc_session_t* session, const char* restrict origin, const char* restrict nick, const char* restrict channel, const char* restrict msg) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    if (strlen(msg) == 0) return 1;

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    int r = handle_commands(session,origin,nick,channel,msg); 
    if (r == 0) return 0;

    if (msg[0] == 1) {

	char ctcp[512];
	decode_ctcp(msg,ctcp);

	return handle_ctcp(session,nick,ctcp);
    }

    switch(up->mode) {
	case BM_NONE:

	    switch(msg[0]) {
		case 'b': {
			      if (strcmp(msg, "botsnack") == 0) respond(session,nick,channel,"nomnomnom");
			      break;}
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

    //    char nick[10];
    //    irc_target_get_nick(origin,nick,10);
    //
    //    printf("User %s quit the server.\n",nick);
    //
    //    struct irc_bot_params* ibp = irc_get_ctx(session);
    //    struct irc_user_params* up = get_user_params(nick, EB_NULL);
    //
    //    if (up) {
    //	save_user_params(nick,up);
    //	del_user_params(nick,up);
    //    }

}

void part_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    //    char nick[10];
    //    irc_target_get_nick(origin,nick,10);
    //
    //    printf("User %s left channel %s.\n",nick,params[0]);
    //	
    //    struct irc_bot_params* ibp = irc_get_ctx(session);
    //    if (ircstrcmp(nick, ibp->msg_current_nickname) != 0) {
    //    
    //    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    //    //up->channel_count--;
    //    
    //    }

}

struct irc_url_params {
    irc_session_t* session;
    char* nick;
    char* channel;
};

void irc_url_title_cb(int n, const char* url, const char* title, void* param) {

    struct irc_url_params* ctx = param;


    if ((title) && (strlen(title) > 0)) {

	size_t strl = strlen(title) + 1;
	char title_unesc[strl];

	html_unescape(title,title_unesc);	
	ircprintf(ctx->session,NULL,ctx->channel,"Title: \00310%s\017",title_unesc);
    }

    if (ctx->nick) free(ctx->nick);
    if (ctx->channel) free (ctx->channel);
    if (ctx) free (ctx); 
}

#if ENABLE_URL_NOTIFY

char* find_domain_name(const char* restrict url, const char** urlend) {

    if (strchr(url,':') == NULL) return NULL;

    char* dstart = strchr(url,':')+1;

    if (strncmp(dstart,"//",2) == 0) dstart +=2; //skip the double slash

    if (strchr(dstart,'@')) dstart = strchr(dstart,'@') + 1; //skip user/pwd

    char* dend = dstart + strlen(dstart);

    if (strchr(dstart,'/')) dend = strchr(dstart,'/');

    if ( strchr(dstart,':') && (strchr(dstart,':') < dend) ) dend = strchr(dstart,':');

    *urlend = dend;
    return dstart;
}

bool check_if_in_notify(const char* url) {

    if (!notify_url_list) return false;

    int pos = boyer_moore(notify_url_list, notify_url_len, url, strlen(url)); 

    if (pos) return true;
    return false;
}
#endif

bool url_titlable(const char* url) {

    if (strstr(url,"youtube.com/")) return true;
    if (strstr(url,"youtu.be/")) return true;
    if (strstr(url,"vimeo.com/")) return true;
    //if (strstr(url,"reddit.com/")) return true;
    if (strstr(url,"themoscowtimes.com/")) return true;
    if (strstr(url,"redd.it/")) return true;
    //if (strstr(url,"i.imgur.com/")) return false; //prevent i.imgur.com
    //if (strstr(url,"imgur.com/")) return true;
    if (strstr(url,"twitter.com/")) return true;
    return false;
}
void find_urls(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    const char* end = params[1] + strlen(params[1]);
    char* url1 = find_url(params[1],&end);

    int i=1;

    while (end) {

	size_t ulen = end - url1 + 1;

	char url[ulen];
	strncpy(url,url1,ulen);
	url[ulen-1]=0;
	printf("Found URL: %s\n",url);
	add_url_to_buf(url);

#if ENABLE_URL_NOTIFY
	bool n = check_if_in_notify(url);

	if (n) { ircprintf(session,NULL, params[0], "\00304The URL posted above is reported to be blocked in the Russian Federation.\017"); } else {

	    const char* d_end = url + strlen(url);
	    char* domain1 = find_domain_name(url,&d_end);

	    if (domain1) {	
		size_t dlen = d_end - domain1 + 1;

		char domain[dlen];

		strncpy(domain,domain1,dlen);
		domain[dlen-1]=0;
		printf("Found domain name: %s\n",domain); 

		bool n = check_if_in_notify(domain);

		if (n) { ircprintf(session,NULL, params[0], "\00304The domain name in the URL posted above is reported to be blocked in the Russian Federation.\017");} else {

		    char ipaddr[64]; //enough for v4 and v6

		    int r = resolve_to_ip(domain,&ipaddr);	

		    printf("Resolved to IP: %s\n",ipaddr);

		    bool n = check_if_in_notify(ipaddr);

		    ircprintf(session,NULL, params[0], "\00308The IP address for the website linked above is reported to be blocked in the Russian Federation.\017");

		}
	    }
	}
#endif

	/*
	 * functionality disabled at request of Rein@##chat

	 if (ulen > 60) {

	 char* shurl = irc_shorten(url);
	 ircprintf(session,NULL,params[0],"Short URL (#%d): \00312%s\017" ,i,shurl);
	 if (shurl) free(shurl);
	 }
	 */

	if (strstr(url,"http://i.imgur.com/") == url) {

	    struct irc_url_params* iup = malloc(sizeof(struct irc_url_params));
	    memset(iup, 0, sizeof(struct irc_url_params));

	    iup->session = session;
	    iup->nick = NULL;
	    iup->channel = strdup(params[0]);

	    irc_imgur_title(url,irc_url_title_cb,iup); //currently only test
	}

	if ( (strcmp(nick,"Tubbee")) && (url_titlable(url)) ) {

	    struct irc_url_params* iup = malloc(sizeof(struct irc_url_params));
	    memset(iup, 0, sizeof(struct irc_url_params));

	    iup->session = session;
	    iup->nick = NULL;
	    iup->channel = strdup(params[0]);

	    irc_shorten_and_title(url,irc_url_title_cb,iup); //currently only test
	}

	i++;
	url1 = find_url(end,&end);
    }
}

void channel_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    struct irc_bot_params* ibp = irc_get_ctx(session);

    bool handle_this_message = false;
    const char* handle_ptr = params[1];

    find_urls(session,event,origin,params,count);

    if (strncmp(params[1],".",1) == 0) handle_this_message = true;

    if (strncmp(params[1],ibp->irc_nickname,strlen(ibp->irc_nickname)) == 0) {
	handle_this_message = true;
	handle_ptr = params[1] + strlen(ibp->irc_nickname);
	while ((strlen(handle_ptr) > 0) && (strchr(",: ",handle_ptr[0])) ) handle_ptr++; //skip delimiters.
    }

    if (handle_this_message) {
	handle_msg(session, origin, nick, params[0], handle_ptr);
    } else {

	if (params[0] != NULL) count_msg(session,nick,params[0],params[1]);
    }
}

void privmsg_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];

    irc_target_get_nick(origin,nick,10);
    //printf("[   !!PRIVATE!!] [%10s]:%s\n",nick,params[1]);

    handle_msg(session,origin,nick,NULL,params[1]); // handle private message

}

void join_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    if (ircstrcmp(nick,ibp->irc_nickname) == 0) {

	//I joined the channel.

	printf("User %s joined channel %s.\n",nick,params[0]);
	ibp->channel_joined = 1;
    } else {

	//Someone else did.

	struct irc_user_params* up = get_user_params(nick, EB_LOAD);

	//fullnick stores the complete nickname of the user.
	//that includes the hostname.
	if (strncmp(origin,up->fullnick,129)) up->logged_in = 0;
	strncpy(up->fullnick,origin,129);
	//up->channel_count++;
    }

}

void connect_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    printf("Successfully connected to the network.\n");

    watch_load();
    if (!save_atexited) { atexit(watch_save); save_atexited = 1;}

    struct irc_bot_params* ibp = irc_get_ctx(session);

    char* saveptr = NULL;
    char* chantok = strtok_r(ibp->irc_channel,",",&saveptr);

    int r = 0;

    while (chantok) {

	char* saveptr2 = NULL;
	char* channame = strtok_r(chantok,":",&saveptr2);
	char* chankey = strtok_r(NULL,":",&saveptr2);
	r = irc_cmd_join( session, channame, chankey);
	if (r != 0) printf("Can't join %s: %d\n",channame,r);
	chantok = strtok_r(NULL,",",&saveptr);
    }

}

int ison_count = 0;
char* ison_nicknames = NULL;
bool* ison_statuses = NULL;
pthread_cond_t ison_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t ison_condm = PTHREAD_MUTEX_INITIALIZER;

void ison_response_cb(irc_session_t* session, unsigned int event, const char* origin, const char** params, unsigned int count) {

    if ((!ison_count) || (!ison_nicknames) || (!ison_statuses)) return;

    char* saveptr = NULL;

    char* curtok = strtok_r(ison_nicknames," ",&saveptr);
    int i=0;

    while ((curtok) && (i < ison_count)) {

	if (strstr(params[1],curtok))
	    ison_statuses[i] = true;

	curtok = strtok_r(NULL," ",&saveptr);
	i++;
    }

    pthread_mutex_lock(&ison_condm);
    ison_count = 0;
    pthread_cond_signal(&ison_cond);
    pthread_mutex_unlock(&ison_condm);

    return;
}

int ison_request(irc_session_t* session, int count, const char** nicknames, bool* o_statuses) {

    if ((!count) || (!nicknames) || (!o_statuses)) return 1;;

    if ((ison_count) || (ison_nicknames) || (ison_statuses)) return 2;

    char tempnick[506]; tempnick[0] = 0; int left=506;

    for (int i=0; i < count; i++) {
	strncat(tempnick,nicknames[i],left);
	left -= strlen(nicknames[i]);
	strncat(tempnick," ",left);
	left --;
    }

    if (left < 0) return 3;

    pthread_mutex_lock(&ison_condm);

    ison_count = count;
    ison_statuses = o_statuses;
    ison_nicknames = strdup(tempnick);

    irc_raw_sendf(session, "ISON %s", tempnick);

    while (ison_count != 0) pthread_cond_wait(&ison_cond,&ison_condm);
    pthread_mutex_unlock(&ison_condm);

    free(ison_nicknames);
    ison_nicknames = NULL;
    ison_statuses = NULL;
    return 0;
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

	case 303: // ISON response

	    printf("Received ISON response.\n");
	    ison_response_cb(session,event,origin,params,count);
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

    if (!save_initialized) {
	findsavedir();
#if ENABLE_URL_NOTIFY
	notify_url_list = map_notifylist(&notify_url_len);
#endif
	save_initialized = 1;}

    struct irc_bot_params* new_params = malloc(sizeof(struct irc_bot_params));
    if (!new_params) {
	perror("Unable to allocate memory for bot structure");
	return NULL;
    }

    memset(new_params,0,sizeof(struct irc_bot_params));

    new_params->irc_channel = irc_channel;

    irc_callbacks_t callbacks = {
	.event_connect=connect_cb, .event_numeric = numeric_cb,
	.event_join = join_cb, .event_quit = quit_cb, .event_part = part_cb,
	.event_channel = channel_cb, .event_privmsg = privmsg_cb }; 

    irc_session_t* session = irc_create_session (&callbacks);
    irc_set_ctx(session,new_params);

    return session;
}

int connect_bot(void* session, char* address, int port, bool use_ssl, char* nickname, char* password) {

    if (use_ssl) { fprintf(stderr,"Sorry, SSL not supported.\n"); return 1;}

    struct irc_bot_params* ibp = irc_get_ctx(session);
    ibp->irc_nickname = nickname;

    int r = irc_connect(session,address,port,password,nickname,NULL,"snowbot");
    if (r != 0) fprintf(stderr,"IRC connection error %d.\n",r); else printf("Connected to %s.\n",address);
    return r;
}

int destroy_bot(void* session) {

    free(irc_get_ctx(session));
    return 0;
}

int disconnect_bot(void* session) {

    irc_disconnect(session);
    return 0;
}

int reconnect_bot(void* session) {

    //reconnects to the same server with the same parameters
    //without damaging the session context.
    //usable for connection losses.

    void* ctx = irc_get_ctx(session);


    return 0;
}

int loop_bot2(int session_c, void** session_v) {

    int r = irc_run2(session_c, (irc_session_t**)session_v);
    if (r != 0) fprintf(stderr,"irc_run2 error %d.\n",r);
    printf("Terminating.\n");
    return 0;
}

int loop_bot(void* session) {

    int r = irc_run(session);
    if (r != 0) fprintf(stderr,"irc_run error %d.\n",r);
    printf("Terminating.\n");
    return 0;
}


