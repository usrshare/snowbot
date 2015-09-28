#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <curl/curl.h>

/* based on http://curl.haxx.se/libcurl/c/getinmemory.html, licensed as: */
/*
   COPYRIGHT AND PERMISSION NOTICE

   Copyright (c) 1996 - 2015, Daniel Stenberg, daniel@haxx.se.

   All rights reserved.

   Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder. */

bool curl_initialized = false;

struct MemoryStruct {
    char *memory;
    size_t size;
};

    static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
	/* out of memory! */ 
	printf("not enough memory (realloc returned NULL)\n");
	return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char* make_http_request(char* restrict url, char* restrict postfields) {

    if (!curl_initialized) { curl_global_init(CURL_GLOBAL_ALL); curl_initialized = true; }

    CURL* httpreq = curl_easy_init();
    if (!httpreq) return NULL;

    curl_easy_setopt(httpreq, CURLOPT_URL, url);
    if (postfields) curl_easy_setopt(httpreq, CURLOPT_POSTFIELDS, postfields);

    struct MemoryStruct chunk;
    memset(&chunk,0,sizeof chunk);

    curl_easy_setopt(httpreq, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(httpreq, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    CURLcode res = curl_easy_perform(httpreq);

    char* restext = NULL;

    if (res != CURLE_OK)
	restext = strdup(curl_easy_strerror(res)); else
	    restext = strndup(chunk.memory,chunk.size);

    return restext;
}
