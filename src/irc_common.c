// vim: cin:sts=4:sw=4 
#include "irc_common.h"

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>

char* strrecat(char* orig, const char* append) {
    
    char* new = NULL;
    if (orig) {
    new = realloc(orig,strlen(orig) + strlen(append) + 1);
    new = strcat(new,append);
    } else {
    new = strdup(append);
    }
    
    return new;
}

int irccharcasecmp(const char c1, const char c2) {

    //as defined by RFC 1459 and 2812,
    //the characters {}|~ (91~94) and []\^ (123~126)
    //are case-equivalent.

    if ((c1 < 'A') || (c1 > '~')) return c1-c2;
    if ((c2 < 'A') || (c2 > '~')) return c1-c2;
    if ((c1 == '_') || (c1 == '`')) return c1-c2;
    if ((c2 == '_') || (c2 == '`')) return c1-c2;

    return (c1 & 0x5F) - (c2 & 0x5F);

}

int cnt_tokens (const char* restrict string, const char* delim) {

    int tokens = 1;

    bool emptytkn = false;

    for (unsigned int i=0; i < strlen(string); i++)
	for (unsigned int j=0; j < strlen(delim); j++)
	    if (string[i] == delim[j]) { if (!emptytkn) tokens++; emptytkn = true;} else {emptytkn = false;}

    return tokens;
}

int ircstrcmp(const char* s1, const char* s2) {

    size_t n=0;

    if (s1 == s2) return 0;
    do {
	int r = irccharcasecmp(s1[n],s2[n]);
	if (r) return r;
	n++;
    } while ((s1[n]) || (s2[n]));
    return 0;
}

int ircstrncmp(const char* s1, const char* s2, size_t N) {
    
    size_t n=0;

    if (s1 == s2) return 0;
    do {
	int r = irccharcasecmp(s1[n],s2[n]);
	if (r) return r;
	n++;
    } while ( ((s1[n]) || (s2[n])) && (n < N) );
    return 0;

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

int decode_ctcp(const char* restrict msg, char* o_msg) {

    if (msg[0] != 1) return 1;
    if (msg[strlen(msg)-1] != 1) return 1;
	
    const char* curchar = msg+1;
    char* outchar = o_msg;

    while ( (*curchar > 1) && (curchar < (msg+strlen(msg))) ) {
    
	if (*curchar == 16) {
    
	switch(*(curchar+1)) {
	    case '0': *outchar = 0; break;
	    case 'n': *outchar = '\n'; break;
	    case 'r': *outchar = '\r'; break;
	    case 16: *outchar = 16; break;
	}
	curchar += 2;
	} else {

	*outchar = *curchar;	
	curchar++;
	}
	outchar++;
    }
    *outchar =0;
    return 0;
}
int encode_ctcp(const char* restrict msg, char* o_msg) {

    const char* curchar = msg;

    o_msg[0] = 1;
    char* outchar = o_msg+1;

    while ( (*curchar > 0) && (curchar < (msg+strlen(msg))) ) {
    
	switch(*curchar+1) {
	    case 0:    *outchar = 16; *(outchar+1) = 0;    outchar++; break;
	    case '\n': *outchar = 16; *(outchar+1) = '\n'; outchar++; break;
	    case '\r': *outchar = 16; *(outchar+1) = '\r'; outchar++; break;
	    case 16 :  *outchar = 16; *(outchar+1) = 16;   outchar++; break;
	    default: *outchar = *curchar; break;	
	}
	outchar++;
	curchar++;
    }
    *outchar =1;
    *(outchar+1) =0;
    return 0;
}
