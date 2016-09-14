// vim: cin:sts=4:sw=4 
#include "resolve.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int resolve_to_ip(const char* domain, char* o_ipstr) {

    struct addrinfo hints, *res;
    
    memset(&hints,0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int r = getaddrinfo(domain,NULL,&hints,&res);

    if (r != 0) {
	fprintf(stderr,"gai error %s\n", gai_strerror(r));
	return 2; }

    void* addr = NULL;

    if (res->ai_family == AF_INET)
	addr = &( ( (struct sockaddr_in *)(res->ai_addr) ) ->sin_addr);
    else if (res->ai_family == AF_INET6)
	addr = &( ( (struct sockaddr_in6 *)(res->ai_addr) ) ->sin6_addr);

    inet_ntop(res->ai_family, addr, o_ipstr, 64);

    freeaddrinfo(res);
    return 0;

}
