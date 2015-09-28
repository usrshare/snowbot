#ifndef HTTP_H
#define HTTP_H

extern bool curl_initialized;

char* make_http_request(const char* restrict url, const char* restrict postfields);
#endif
