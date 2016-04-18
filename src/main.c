// vim: cin:sts=4:sw=4 
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "irc.h"
#include "http.h"

void print_usage(void) {

    fprintf(stderr,"Available parameters: [-p port] [-n nickname] [-c channel] address\n");
}

struct irc_conn_params {

    char* server_addr; // IP address or hostname of the IRC server
    unsigned int server_port; // port of the IRC server
    char* server_tag; // tag, used locally to identify networks
    char* server_password; // IP address or hostname of the IRC server
    char* bot_channels; // list of channels to join upon connecting
    char* bot_nickname; // nickname
}; 

int parse_arguments(int argc, char** argv, struct irc_conn_params* o_params) {

    // WARNING: this function modifies argv!

    // "nickname@irc.server.org/#channels,#channels"
    // "nickname:password@irc.server.org:port/#channels,#channels"
    // "tag=nickname@irc.server.org:port/#channels,#channels"

    memset(o_params, 0, sizeof(struct irc_conn_params) * argc);

    for (int i=0; i < argc; i++) {

	char* at = strchr(argv[i],'@');
	char* slash = strchr(argv[i],'/');

	if ((!at) || (!slash) || (slash < at) || !strlen(at) || !strlen(slash)) {
	    fprintf(stderr,"Argument %d is incorrect.\n",i); return -1;
	}

	// first, we get the channel list part from the end.

	o_params[i].bot_channels = slash+1;
	*slash = 0; //now the string ends on the server/port;

	// is there a port specified?
	char* colon = strchr(at+1, ':');
	o_params[i].server_port = ( colon ? atoi(colon+1) : 6667); 
	if (colon) *colon = 0;

	// now the string ends at the server name.
	o_params[i].server_addr = at+1;
	*at = 0;

	// now it only has the tag/nick/pwd part.
	char* pwdcolon = strchr(argv[i], ':');
	if (pwdcolon) { o_params[i].server_password = pwdcolon+1; *pwdcolon = 0; }

	char* equals = strchr(argv[i], '=');

	if (equals) {
	    int tagl = (equals - argv[i]);
	    o_params[i].server_tag = argv[i];
	    *equals = 0;
	    o_params[i].bot_nickname = equals+1;
	} else {
	    o_params[i].bot_nickname = argv[i];
	}

	printf("%s # %s @ %s : %u / %s\n",o_params[i].server_tag,o_params[i].bot_nickname,o_params[i].server_addr,o_params[i].server_port,o_params[i].bot_channels);

    }

    return 0; 
}

void inthandler(int sig) {
    exit(0);
}

int main(int argc, char** argv) {

    if (argc <= 1) {

	fprintf(stderr, "Usage: %s [tag=]nickname[:password]@server[:port]/#channel[,#channel2,...]\n", argv[0]);

	return 1; }

    struct irc_conn_params cp[argc];
    memset(cp, 0, sizeof cp);

    int sessions_c = argc-1;
    
    int r = parse_arguments(sessions_c, argv+1, cp);
    if (r < 0) return 1;

    http_initialize();

    signal(SIGINT,inthandler);

    void* bothnd[sessions_c];

    for (int i=0; i < (sessions_c); i++) {
    
	bothnd[i] = create_bot(cp[i].bot_channels);
	if (!bothnd[i]) { fprintf(stderr,"Unable to create a bot.\n"); return 1;}
	printf("Connecting to %s, port %d, channel %s as %s...\n",cp[i].server_addr,cp[i].server_port,cp[i].bot_channels,cp[i].bot_nickname);
	connect_bot(bothnd[i],cp[i].server_addr,cp[i].server_port,0,cp[i].bot_nickname,cp[i].server_password);
    }


    while (true) {

	loop_bot2(sessions_c,bothnd);
	for (int i=0; i < (sessions_c); i++) disconnect_bot(bothnd[i]);
    }

    return 0;
}
