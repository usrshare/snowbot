// vim: cin:sts=4:sw=4 
#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <pthread.h>

#include "http.h"
/* based on http://curl.haxx.se/libcurl/c/getinmemory.html, licensed as: */
/*
   COPYRIGHT AND PERMISSION NOTICE

   Copyright (c) 1996 - 2015, Daniel Stenberg, daniel@haxx.se.

   All rights reserved.

   Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder. */

bool curl_initialized = false;

//CURLM* cmhnd = NULL; //multi handle


struct MemoryStruct {
    char *memory;
    size_t size;
    size_t maxsize;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {

    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    if ((!mem->maxsize) || (mem->size < mem->maxsize)) {

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
	/* out of memory! */ 
	printf("not enough memory (realloc returned NULL)\n");
	return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    }

    return realsize;
}

void http_initialize() {

    curl_global_init(CURL_GLOBAL_ALL);
    //if (!cmhnd) cmhnd = curl_multi_init();
    curl_initialized = true;
}

char* http_escape_url(const char* url, int length) {

    CURL* httpreq = curl_easy_init();
    if (!httpreq) return NULL;

    char* res = curl_easy_escape(httpreq,url,length);

    curl_easy_cleanup(httpreq);

    return res;
}

struct async_request_params {
    char* restrict url;
    char* restrict postfields;
    size_t maxdl;
    http_recv_cb callback;
    void* cbparam;
};

char* make_http_request(const char* restrict url, const char* restrict postfields, size_t maxdl) {

    CURL* httpreq = curl_easy_init();
    if (!httpreq) return NULL;

    curl_easy_setopt(httpreq, CURLOPT_URL, url);
    if (postfields) curl_easy_setopt(httpreq, CURLOPT_POSTFIELDS, postfields);

    struct MemoryStruct chunk;
    memset(&chunk,0,sizeof chunk);
    
    if (maxdl) chunk.maxsize = maxdl;

    curl_easy_setopt(httpreq, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(httpreq, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(httpreq, CURLOPT_MAXREDIRS, 4);
    curl_easy_setopt(httpreq, CURLOPT_MAXFILESIZE, 1024*128);
    curl_easy_setopt(httpreq, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(httpreq, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    CURLcode res = curl_easy_perform(httpreq);

    char* restext = NULL;

    if (res != CURLE_OK)
	restext = strdup(curl_easy_strerror(res)); else
	    restext = strndup(chunk.memory,chunk.size);

    curl_easy_cleanup(httpreq);

    return restext;
}

static void* async_request_thread (void* param) {

    struct async_request_params* ctx = param;
    
    char* restext = make_http_request(ctx->url,ctx->postfields,ctx->maxdl);

    ctx->callback(restext,ctx->cbparam);

    if (ctx->url) free(ctx->url);
    if (ctx->postfields) free(ctx->postfields);
    free (ctx);

    return NULL;
}

void make_http_request_cb(const char* restrict url, const char* restrict postfields, size_t maxdl, http_recv_cb callback, void* cbparam) {

    struct async_request_params* ap = malloc(sizeof(struct async_request_params));

    ap->url = (url ? strdup(url) : NULL);
    ap->postfields = (postfields ? strdup(postfields) : NULL);
    ap->maxdl = maxdl;
    ap->callback = callback;
    ap->cbparam = cbparam;

    pthread_t httpthread;
    pthread_create (&httpthread,NULL,async_request_thread,ap);

    return;
}

