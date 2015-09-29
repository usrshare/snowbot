#include "irc_common.h"
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

char* strrecat(char* orig, const char* append) {
    char* new = realloc(orig,strlen(orig) + strlen(append) + 1);
    new = strcat(new,append);
    return new;
}

int respond(irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict msg) {

    if (!channel) {
	return irc_cmd_msg(session,target,msg);
    } else { if (target) {
	int ml = strlen(msg) + 9 + 2 + 1;
	char newmsg[ml];
	snprintf(newmsg,ml,"%s: %s",target,msg);
	return irc_cmd_msg(session,channel,newmsg);
    } else return irc_cmd_msg(session,channel,msg); }
}
int ircvprintf (irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict format, va_list vargs) {

    char res[1024]; 

    int r = vsnprintf(res,1024,format,vargs);

    if (r > 1024) {

	char* outtext = malloc(r + 64);
	r = vsnprintf(res,r+64,format,vargs);
	r = respond(session,target,channel,outtext);
	free(outtext);
    } else {
	r = respond(session,target,channel,res);
    }

    return r; 
}
int ircprintf (irc_session_t* session, const char* restrict target, const char* restrict channel, const char* restrict format, ...) {


    va_list vargs;
    va_start (vargs, format);

    int r = ircvprintf(session,target,channel,format,vargs);

    va_end(vargs);
    return r;
}
