// vim: cin:sts=4:sw=4 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "irc.h"

void print_usage(void) {

	fprintf(stderr,"Available parameters: [-p port] [-s] [-n nickname] [-c channel] address\n");
}

int main(int argc, char** argv) {

	char* server_addr = NULL;
	int server_port = 6667;
	bool use_ssl = 0;
	char* bot_nickname = "snowbot";
	char* bot_channel = NULL;

	int opt;

	while ((opt = getopt(argc,argv,"p:sn:c:")) != -1) {
		switch(opt) {
			case 'p':
				server_port = atoi(optarg);
				break;
			case 's':
				use_ssl = 1;
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


	printf("Connecting to %s, port %d, channel %s as %s...\n",server_addr,server_port,bot_channel,bot_nickname);

	void* bothnd = create_bot(bot_channel);
	if (!bothnd) { fprintf(stderr,"Unable to create a bot.\n"); return 1;}

	connect_bot(bothnd,server_addr,server_port,use_ssl,bot_nickname);

	loop_bot(bothnd);

	return 0;
}
