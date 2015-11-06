// vim: cin:sts=4:sw=4 
#include "irc_short.h"
#include "http.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>

void irc_shorten_and_title_cb(const char* data, void* param) {

    if (!data) return;

    char* title1 = strstr(data, "<title>");
    char* title2 = strstr(data, "</title>");

    if ((!title1) || (!title2) || (title2 < title1)) {

	printf("Unable to extract title from page.\n");
	return;
    }

    char* tbegin = title1 + strlen("<title>");
    size_t tlen = title2 - tbegin;

    char* title = strndup(tbegin,tlen);
    
    printf("Page title: %s\n",title);

    free(title);
    return;
}

void irc_shorten_and_title(const char* url) {

    make_http_request_cb(url,NULL,8192,irc_shorten_and_title_cb,NULL);	
}

const char* irc_shorten(const char* url) {

    char* escurl = http_escape_url(url,strlen(url));

    char vgdurl[strlen(escurl) + 128];

    sprintf(vgdurl,"http://v.gd/create.php?format=simple&url=%s",escurl);

    free(escurl);

    return make_http_request(vgdurl,0);
}
