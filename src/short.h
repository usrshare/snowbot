// vim: cin:sts=4:sw=4 
#ifndef SHORT_H
#define SHORT_H

int add_url_to_buf(const char* url);
int search_url(const char* restrict pattern, char* o_url);

typedef void (*url_title_cb) (int n, const char* url, const char* title, void* param);

char* irc_shorten(const char* url); //don't forget to free();
void irc_shorten_and_title(const char* url, url_title_cb callback, void* param);
void irc_imgur_title(const char* url, url_title_cb callback, void* param);

#endif
