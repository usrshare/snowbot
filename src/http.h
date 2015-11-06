#ifndef HTTP_H
#define HTTP_H
#include <stdbool.h>
#include <stdlib.h>

extern bool curl_initialized;

typedef void (*http_recv_cb) (const char* data, void* param);

char* http_escape_url(const char* url, int length);
void make_http_request_cb(const char* restrict url, const char* restrict postfields, size_t maxdl, http_recv_cb callback, void* cbparam);
char* make_http_request(const char* restrict url, const char* restrict postfields);
#endif
