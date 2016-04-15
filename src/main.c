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

    char server_addr[64]; // IP address or hostname of the IRC server
    unsigned int server_port; // port of the IRC server
    char server_tag[10]; // tag, used locally to identify networks
    char server_password[64]; // IP address or hostname of the IRC server
    char bot_channels[256]; // list of channels to join upon connecting
    char bot_nickname[10]; // nickname
}; 

int parse_arguments(int argc, char** argv, struct irc_conn_params* o_params) {

    // "nickname@irc.server.org/#channels,#channels"
    // "nickname:password@irc.server.org:port/#channels,#channels"
    // "tag=nickname@irc.server.org:port/#channels,#channels"

    for (int i=0; i < argc; i++) {
    
    char* at = strchr(argv[i],'@');
    char* slash = strchr(argv[i],'/');

    if ((!at) || (!slash) || (slash < at) || !strlen(at) || !strlen(slash)) {
	fprintf(stderr,"Argument %d is incorrect.\n",i); return -1;
    }
    
    // first, we get the channel list part from the end.

    strncpy(o_params[i].bot_channels,slash+1,255);
    *slash = 0; //now the string ends on the server/port;


    // is there a port specified?
    char* colon = strchr(at+1, ':');
    o_params[i].server_port = ( colon ? atoi(colon+1) : 6667); 
    if (colon) *colon = 0;
    
    // now the string ends at the server name.
    strncpy(o_params[i].server_addr,at+1, 63);
    *at = 0;

    // now it only has the tag/nick/pwd part.
    char* pwdcolon = strchr(argv[i], ':');
    if (pwdcolon) { strncpy(o_params[i].server_password, pwdcolon+1, 63);
    *pwdcolon = 0; }

    char* equals = strchr(argv[i], '=');

    if (equals) {
	int tagl = (equals - argv[i]);
	strncpy(o_params[i].server_tag,argv[i], tagl < 9 ? tagl : 9);
	strncpy(o_params[i].bot_nickname,equals+1, 9);
    } else {
	strncpy(o_params[i].bot_nickname,argv[i], 9);
    }

    printf("%s # %s : %s @ %s : %u / %s\n",o_params[i].server_tag,o_params[i].bot_nickname,o_params[i].server_password,o_params[i].server_addr,o_params[i].server_port,o_params[i].bot_channels);

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

	int r = parse_arguments(argc-1, argv+1, cp);
	if (r < 0) return 1;
	
	/*
	char* server_addr = NULL;
	int server_port = 6667;
	bool use_ssl = 0;
	char* bot_nickname = "snowbot";
	char* bot_channel = NULL;
	char* srv_password = NULL;

	int opt;

	while ((opt = getopt(argc,argv,"P:p:sn:c:")) != -1) {
	    switch(opt) {
		case 'P':
		    srv_password = optarg;
		    break;
		case 'p':
		    server_port = atoi(optarg);
		    break;
		case 's':
		    fprintf(stderr,"SSL connections not supported.\n");
		    exit(1);
		    break;
		case 'n':
		    bot_nickname = optarg;
		    break;
		case 'c':
		    bot_channel = optarg;
		    break;
		default:
		    print_usage(); exit(1);
		    break;
	    }
	}

	server_addr = argv[optind];

	if ((!server_addr) || (!bot_channel)) {
	    fprintf(stderr,"Server address or channel not specified.\n");
	    print_usage();
	    exit(1);
	}
	*/

	http_initialize();

	printf("Connecting to %s, port %d, channel %s as %s...\n",cp[0].server_addr,cp[0].server_port,cp[0].bot_channels,cp[0].bot_nickname);

	void* bothnd = create_bot(cp[0].bot_channels);
	if (!bothnd) { fprintf(stderr,"Unable to create a bot.\n"); return 1;}
	
    signal(SIGINT,inthandler);

    while (true) {

    connect_bot(bothnd,cp[0].server_addr,cp[0].server_port,0,cp[0].bot_nickname,cp[0].server_password);

    loop_bot(bothnd);
    
    disconnect_bot(bothnd);
    }

    return 0;
}
