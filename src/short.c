// vim: cin:sts=4:sw=4 
#include "short.h"
#include "http.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#define URL_CIRCBUF_COUNT 20
#define URL_LENGTH 512 //maximum IRC msg size anyway

unsigned int urlbuf_cursor = 0;
int urlbuf_last = -1;
char urlbuf[URL_CIRCBUF_COUNT][URL_LENGTH];

int add_url_to_buf(const char* url) {
    strncpy(urlbuf[urlbuf_cursor],url,URL_LENGTH);
    urlbuf[urlbuf_cursor][URL_LENGTH-1] = 0; //forcibly terminate
    urlbuf_last = urlbuf_cursor;
    urlbuf_cursor = (urlbuf_cursor + 1) % URL_CIRCBUF_COUNT;
    return 0;
}

int search_url(const char* restrict pattern, char* o_url) {

    int n=0, r=-1;
	
    if (urlbuf_last < 0) return 1;

    if (pattern == NULL) {
	n = 1; r = urlbuf_last;
    } else {

	for (int i=0; i < URL_CIRCBUF_COUNT; i++) {

	    if (strstr(urlbuf[i],pattern)) {n++; r = i;}

	}
    }
    if (n == 0) return 1;
    if (n > 1) return 2;

    strncpy(o_url,urlbuf[r],URL_LENGTH);
    o_url[URL_LENGTH-1] = 0;
    return 0;
}

struct title_cb {
    int n;
    char* url;
    char* title;
    url_title_cb callback;
    void* param;
};

void irc_shorten_and_title_cb(const char* data, void* param) {

    if (!data) return;

    struct title_cb* tcb = param;

    char* title1 = strstr(data, "<title>");
    if (!title1) title1 = strstr(data, "<TITLE>");
    char* title2 = strstr(data, "</title>");
    if (!title2) title2 = strstr(data, "</TITLE>");

    if ((!title1) || (!title2) || (title2 < title1)) {

	printf("Unable to extract title from page.\n");
	if (tcb->callback) tcb->callback(tcb->n, tcb->url, NULL, tcb->param);
	return;
    }

    char* tbegin = title1 + strlen("<title>");
    size_t tlen = title2 - tbegin;

    char* title = strndup(tbegin,tlen);


    if (tcb->callback) tcb->callback(tcb->n, tcb->url, title, tcb->param);
    free(title);
    return;
}

void irc_shorten_and_title(const char* url, url_title_cb callback, void* param) {

    char* urlcpy = strdup(url);

    char* hash = strchr(url,'#');
    if (hash) *hash = 0;

    struct title_cb* tcb = malloc(sizeof(struct title_cb));

    tcb->url = strdup(url);
    tcb->callback = callback;
    tcb->param = param;

    make_http_request_cb(urlcpy,NULL,128*1024,irc_shorten_and_title_cb,tcb);	

    free(urlcpy);
}

char* irc_shorten(const char* url) {

    char* escurl = http_escape_url(url,strlen(url));

    char vgdurl[strlen(escurl) + 128];

    sprintf(vgdurl,"http://v.gd/create.php?format=simple&url=%s",escurl);

    free(escurl);

    return make_http_request(vgdurl,0,0);
}
