// vim: cin:sts=4:sw=4 
#include "paste.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
//#include <curl/curl.h>

/* based on http://curl.haxx.se/libcurl/c/getinmemory.html, licensed as: */
/*
   COPYRIGHT AND PERMISSION NOTICE

   Copyright (c) 1996 - 2015, Daniel Stenberg, daniel@haxx.se.

   All rights reserved.

   Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder. */

#define APIKEY "9efa8584c169af9fc3764af880009687"
const char api_url[] = "http://pastebin.com/api/api_post.php";
const char res_url[] = "http://pastebin.com";

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

char* upload_to_pastebin(const char* restrict nickname, const char* restrict pastename, const char* restrict text) {

    /*
    if (!curl_initialized) { curl_global_init(CURL_GLOBAL_ALL); curl_initialized = true; }

    CURL* paste = curl_easy_init();
    if (!paste) return NULL;

    char staticpost[8192];
    char* post = staticpost;    

    char* name_escaped = curl_easy_escape(paste, pastename, 0);
    char* content_escaped = curl_easy_escape(paste, text, 0);

    int r = snprintf(staticpost,8192,"api_dev_key=" APIKEY "&api_option=paste&api_paste_code=%s&api_paste_name=%s",content_escaped,name_escaped);

    if (r > 8192) { post = malloc(r+1); snprintf(post,r+1,"api_dev_key=" APIKEY "&api_option=paste&api_paste_code=%s&api_paste_name=%s",content_escaped,name_escaped); };

    curl_easy_setopt(paste, CURLOPT_URL, api_url);
    curl_easy_setopt(paste, CURLOPT_POSTFIELDS, post);

    struct MemoryStruct chunk;

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    CURLcode res = curl_easy_perform(paste);

    char* restext = NULL;

    if (res != CURLE_OK)
	restext = strdup(curl_easy_strerror(res)); else
	    restext = strndup(chunk.memory,chunk.size);

    if (post != staticpost) free(post);
    curl_free(content_escaped);

    return restext;
    */
    return NULL;
}
