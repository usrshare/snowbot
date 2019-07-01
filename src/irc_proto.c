// vim: cin:sts=4:sw=4 
#include "irc_proto.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#define CRLF "\x0D\x0A"

struct irc_session {
    int sockfd;
    int connected;
    char* tag; //optional for printfs
    char msgbuf[512];
    char* msgp;
    size_t msglen;
    void* ctx;
    irc_callbacks_t cb;

    //connection params, for reconnect.
    
    const char* addresses;
    int addr_count;
    int addr_current_index;

    const char* password;
    const char* nickname;
    const char* username;
    const char* realname;
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
    newsess->msgp = newsess->msgbuf;
    memcpy(&(newsess->cb),callbacks, sizeof (irc_callbacks_t));

    return newsess;
}

int irc_raw_sendf(irc_session_t* session, const char* fmt, ...) {

    va_list vl;

    char sendbuf[512];

    va_start(vl, fmt);
    int n=vsnprintf(sendbuf, 510, fmt, vl);
    va_end(vl);

    if (n > 510) { fprintf(stderr,"Message wasn't sent due to overly long contents.\n"); return 1; }

    //printf("> %.512s\n",sendbuf);

    char* outp = sendbuf+n;

    *outp++ = '\x0D';
    *outp++ = '\x0A';

    ssize_t r = send(session->sockfd,sendbuf,n+2,0);
    if (r == n+2) return 0; else return -1;
}

int irc_set_addresses(irc_session_t* session, const char* restrict addresses) {
    
    if (strlen(addresses) == 0) { fprintf(stderr,"Empty IRC server address list specified.\n"); return 1; }

    session->addresses = addresses;

    char* addrcopy = strdup(addresses);

    char* saveptr;
    char* addr_i = strtok_r(addrcopy, ",", &saveptr);

    session->addr_count = 0;

    while (addr_i) {

	session->addr_count++;
	addr_i = strtok_r(NULL, ",", &saveptr);
    }

    free(addrcopy);

    return 0;
}

int irc_get_address(irc_session_t* session, int index, char* o_address, size_t o_addr_size, uint16_t* o_port) {

    char* addrcopy = strdup(session->addresses);

    char* saveptr;
    char* addr_i = strtok_r(addrcopy, ",", &saveptr);

    int cur_index = 0;

    while (addr_i) {

	if (index == cur_index) {

	    char* saveptr2;
	    char* addr1 = strtok_r(addr_i, ":", &saveptr2);
	    char* portstr = strtok_r(NULL, ":", &saveptr2);

	    strncpy(o_address,addr1,o_addr_size);
	    
	    if (portstr) { 
		uint16_t port = (uint16_t)atoi(portstr);
		*o_port = port;
	    }

	    free(addrcopy);
	    return 0;
	}

	cur_index++;
	addr_i = strtok_r(NULL, ",", &saveptr);
    }

    free(addrcopy);

    return 1;

}

int irc_connect(irc_session_t* session, const char* password, const char* nickname, const char* username, const char* realname) {

    struct sockaddr_in sin;
    struct addrinfo *ai, hai = { 0 };

    //get an address:port from the session list

    char address[256]; uint16_t port = 0;
    irc_get_address(session,session->addr_current_index,address,256,&port);
    if (port == 0) port = 6667; //default IRC port
	
    printf("Connecting to %s:%" PRIu16 " as %s...\n", address, port, nickname);

    hai.ai_family = AF_INET;
    hai.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(address, 0, &hai, &ai))
    { perror("Cannot resolve host."); return 1; }
    memcpy(&sin, ai->ai_addr, sizeof sin);
    sin.sin_port = htons(port);
    freeaddrinfo(ai);
    session->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (session->sockfd<0)
    { perror("Cannot create socket."); return 1; }
    if (connect(session->sockfd, (struct sockaddr *)&sin, sizeof sin)<0)
    { perror("Cannot connect to host."); return 1; }

    if (password) irc_raw_sendf(session,"PASS %s",password);
    irc_raw_sendf(session,"NICK %s",nickname);
    irc_raw_sendf(session,"USER %s 0 0 :%s",username ? username : nickname,realname);
    //irc_raw_sendf(session,"MODE %s +i",nickname);
    
    /*session->address = address; session->port = port;*/ session->password = password; session->nickname = nickname; session->username = username; session->realname = realname;
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

    if ((argc >= 1) && (strcmp(argv[0],"PART") == 0)) {
	if (session->cb.event_quit) session->cb.event_part(session,"part",prefix,argv+1,argc-1);
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

	if (p-l >= 512) { printf("This should never happen.\n"); p=l; } // to my knowledge, it never does.

	r = recv(session->sockfd,p, 512 - (p-l), 0);

	if (r < 0) {
	    if (errno == EINTR) return 1;
	    perror("Error while reading"); return 1;
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

int irc_recv(irc_session_t* session) {

    if ((session->msgp) - (session->msgbuf) >= 512) { printf("This should never happen.\n"); session->msgp=session->msgbuf; } // to my knowledge, it never does.

    int r = recv(session->sockfd,session->msgp, 512 - ((session->msgp) - (session->msgbuf)), 0);

    if (r < 0) {
	if (errno == EINTR) return 1;
	perror("Error while reading"); return 1;
    }

    if (r == 0) {

	printf("Server disconnected.\n"); return 1;
    }

    (session->msgp) += r;

    char* crlf = NULL;

    while ( (crlf = memchr(session->msgbuf,'\x0A',(session->msgp) - (session->msgbuf))) ) {

	if ((crlf > session->msgbuf) && (crlf[-1] == '\x0D'))
	    crlf[-1] = 0; else continue;

	*crlf++ = 0; //terminate with zero.

	//printf("< %.512s\n",l);

	char* prefix = NULL;
	if (session->msgbuf[0] == ':') prefix = (session->msgbuf)+1; else prefix = NULL;

	char* command = (prefix ? memchr((session->msgbuf),' ',512)+1 : (session->msgbuf));
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

	memmove(session->msgbuf,crlf,session->msgp - crlf);
	session->msgp -= (crlf-(session->msgbuf));
    }
    return 0;
}

int irc_run2(int session_c, irc_session_t** session_v) {

    int r = -1;

    if (session_c == 0) return 1; //no sessions, no irc_run

    int fd_max = 0;

    fd_set rfds,xfds;

    //struct timeval tv = {.tv_sec=10, .tv_usec = 0};

    do {
    
	FD_ZERO(&rfds); FD_ZERO(&xfds);

	for (int i=0; i < session_c; i++) {

	    if (session_v[i]->sockfd > fd_max) fd_max = session_v[i]->sockfd;
	    FD_SET(session_v[i]->sockfd,&rfds);
	    FD_SET(session_v[i]->sockfd,&xfds);
	}

	r = select(fd_max + 1, &rfds, NULL, &xfds, NULL);

	if (r == -1) {
	    perror("select"); exit(1);
	}

	for (int i=0; i < session_c; i++) 
	    if (FD_ISSET(session_v[i]->sockfd,&rfds)) {
		if ( irc_recv(session_v[i]) != 0 ) { 
		    printf("Connection lost. Connecting to the next server...\n");
		    session_v[i]->addr_current_index = ((session_v[i]->addr_current_index + 1) % (session_v[i]->addr_count));
		    if (irc_connect(session_v[i],session_v[i]->password, session_v[i]->nickname, session_v[i]->username, session_v[i]->realname) != 0) exit(0); }
	    }

	for (int i=0; i < session_c; i++) 
	    if (FD_ISSET(session_v[i]->sockfd,&xfds)) fprintf(stderr,"Exception %d on session %d, sockfd %d", errno, i, session_v[i]->sockfd);


    }  while (r > 0);

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
