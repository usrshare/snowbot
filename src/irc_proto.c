// vim: cin:sts=4:sw=4 
#include "irc_proto.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#define CRLF "\x0D\x0A"

struct irc_session {
    int sockfd;
    int connected;
    char msgbuf[512];
    size_t msglen;
    void* ctx;
    irc_callbacks_t cb;
};

int irc_set_ctx(irc_session_t* session, void* ctx) {
    session->ctx = ctx;
    return 0;
}

void* irc_get_ctx(irc_session_t* session) {
    return session->ctx;
}

irc_session_t* irc_create_session(irc_callbacks_t* callbacks) {

    irc_session_t* newsess = malloc(sizeof(irc_session_t));
    memset(newsess,0,sizeof(irc_session_t));
    newsess->cb = *callbacks; //copy

    return newsess;
}

int irc_raw_sendf(irc_session_t* session, const char* fmt, ...) {

    va_list vl;

    char sendbuf[512];

    va_start(vl, fmt);
    int n=vsnprintf(sendbuf, 510, fmt, vl);
    va_end(vl);

    if (n > 510) return 1;
    
    //printf("> %.512s\n",sendbuf);

    char* outp = sendbuf+n;

    *outp++ = '\x0D';
    *outp++ = '\x0A';

    ssize_t r = send(session->sockfd,sendbuf,n+2,0);
    if (r == n+2) return 0; else return -1;
}

int irc_connect(irc_session_t* session, const char* restrict address, int port, const char* password, const char* nickname, const char* username, const char* realname) {

    struct sockaddr_in sin;
    struct addrinfo *ai, hai = { 0 };

    hai.ai_family = AF_INET;
    hai.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(address, 0, &hai, &ai))
    { perror("Cannot resolve host."); exit(1); }
    memcpy(&sin, ai->ai_addr, sizeof sin);
    sin.sin_port = htons(port);
    freeaddrinfo(ai);
    session->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (session->sockfd<0)
    { perror("Cannot create socket."); exit(1); }
    if (connect(session->sockfd, (struct sockaddr *)&sin, sizeof sin)<0)
    { perror("Cannot connect to host."); exit(1); }

    if (password) irc_raw_sendf(session,"PASS %s",nickname);
    irc_raw_sendf(session,"USER %s 0 0 :%s",username ? username : nickname,realname);
    irc_raw_sendf(session,"NICK %s",nickname);
    //irc_raw_sendf(session,"MODE %s +i",nickname);
    return 0;
}

int irc_cmd_msg(irc_session_t* session, const char* restrict target, const char* restrict msg) {

    return irc_raw_sendf(session,"PRIVMSG %s :%s",target,msg);
}

int irc_cmd_notice(irc_session_t* session, const char* restrict target, const char* restrict msg) {

    return irc_raw_sendf(session,"NOTICE %s :%s",target,msg);
}

int irc_disconnect(irc_session_t* session) {
    shutdown(session->sockfd,SHUT_RDWR);
    return 0;
}

int cnt_irc_tokens (const char* restrict string) {

    int tokens = 1;

    bool emptytkn = false;

    for (unsigned int i=0; i < strlen(string); i++) {
	 if (string[i] == ' ') { if (!emptytkn) tokens++; emptytkn = true; if (string[i+1] == ':') return tokens; } else {emptytkn = false;}
	 //if ((string[i] == ':') && (i != 0)) return tokens;
    }

    return tokens;
}

int get_irc_tokens (char* string, int argc, const char** argv) {
    
    int argi = 0;

    char* saveptr = NULL;
    char* curtok = strtok_r(string," ",&saveptr);

    while ((curtok) && (argi < argc)) {

	argv[argi] = curtok;
	argi++;

	curtok=strtok_r(NULL," ",&saveptr);
    }

    return argi;
}

bool is_channel(const char* origin) {
    if (strchr("#&!+",origin[0])) return true;
    return false;
}

int irc_parse(irc_session_t* session, const char* prefix, int argc, const char** argv) {

    /*
    if(prefix) printf("[%s]",prefix);
    for (int i=0; i < argc; i++) 
	printf("(%s) ",argv[i]);
    printf("\n");
    */

    if ((argc >= 2) && (strcmp(argv[0],"PING") == 0)) {
	irc_raw_sendf(session,"PONG :%s",argv[1]);
    }

    if ((argc >= 1) && (strlen(argv[0]) == 3)) {

	int numcode = atoi(argv[0]);

	if ((numcode == 376) && (session->cb.event_connect)) //end of motd
	    session->cb.event_connect(session,"connect",prefix,argv+1,argc-1);

	if (session->cb.event_numeric)
	    session->cb.event_numeric(session,numcode,prefix,argv+1,argc-1);
    }
    
    if ((argc >= 1) && (strcmp(argv[0],"JOIN") == 0)) {
	    if (session->cb.event_join) session->cb.event_join(session,"join",prefix,argv+1,argc-1);
    }
    
    if ((argc >= 1) && (strcmp(argv[0],"QUIT") == 0)) {
	    if (session->cb.event_quit) session->cb.event_quit(session,"quit",prefix,argv+1,argc-1);
    }
    
    if ((argc >= 3) && (strcmp(argv[0],"PRIVMSG") == 0)) {

	if (is_channel(argv[1])) {
	    if (session->cb.event_channel) session->cb.event_channel(session,"privmsg",prefix,argv+1,argc-1); }
	else {
	    if (session->cb.event_privmsg) session->cb.event_privmsg(session,"privmsg",prefix,argv+1,argc-1); }
    }

    return 0;
}

int irc_run(irc_session_t* session) {

    int r = -1;

    char l[512]; char* p = l;

    while (r != 0) {

	if (p-l >= 512) {
	    printf("WTF?\n"); p=l;
	}

	//printf("sockfd is %d\n",session->sockfd);
	r = recv(session->sockfd,p, 512 - (p-l), 0);

	if (r < 0) {
	    if (errno == EINTR) return 1;
	    perror("Error while reading"); exit(1);
	}

	p += r;

	char* crlf = NULL;

	while ( (crlf = memchr(l,'\x0A',p-l)) ) {

	if ((crlf > l) && (crlf[-1] == '\x0D'))
	    crlf[-1] = 0; else continue;

	*crlf++ = 0; //terminate with zero.

	//printf("< %.512s\n",l);

	char* prefix = NULL;
	if (l[0] == ':') prefix = l+1; else prefix = NULL;

	char* command = (prefix ? memchr(l,' ',512)+1 : l);
	if (prefix) command[-1] = 0;

	char* trailing = strstr(command," :");
	if (trailing) { trailing[0] = 0; trailing+=2; }

	int argc = cnt_irc_tokens(command);
	if (trailing) argc++;

	const char* argv[argc];

	int argn = get_irc_tokens(command,argc,argv);
	if (trailing) argv[argn] = trailing;

	irc_parse(session,prefix,argc,argv);

	//TODO parse

	memmove(l,crlf,p - crlf);
	p -= (crlf-l);
	
	}

    };
    return 0;
}

int irc_cmd_join(irc_session_t* session, const char* restrict channel, const char* restrict key) {

    if (key)
	return irc_raw_sendf(session,"JOIN %s %s",channel,key);
    else
	return irc_raw_sendf(session,"JOIN %s",channel);

}

int irc_target_get_nick(const char* restrict origin, char* o_nick, size_t o_nick_sz) {

    size_t nicklen = strlen(origin);

    char* orig_end = strchr(origin,'!');
    if (orig_end) nicklen = orig_end - origin;
    if (nicklen > (o_nick_sz - 1)) nicklen = (o_nick_sz-1);

    strncpy(o_nick,origin,nicklen);
    o_nick[nicklen] = 0;    
    return 0;
}
