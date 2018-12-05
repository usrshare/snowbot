// vim: cin:sts=4:sw=4 
#include "derail.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "irc_common.h"
#include "savefile.h"

char d_nickname[IRC_MAX_NICK_LEN+1]; //a separate watch for messages
unsigned int dc_count; //number of consecutive messages
unsigned int dc_length; //length of consecutive messages

int data_loaded = false;
int atexit_set = false;

#define COUNT_THRESHOLD 5
#define LENGTH_THRESHOLD 300

//this is different from cons_count and cons_length in irc.c.
//whenever this bot will post a suggestion, the counts and lengths
//are to be set back to zero.


struct derail_sug {
    char nickname[IRC_MAX_NICK_LEN+1]; //user who made the suggestion
    char text[140];
};

#define SUG_COUNT 32

struct derail_sug derail[SUG_COUNT];

void derail_save(void) {
    
    FILE* df = sfopen("derail.sug","wb");
    if (!df) return;

    for (int i=0; i < SUG_COUNT; i++) {
	fwrite(derail[i].nickname,IRC_MAX_NICK_LEN+1,1,df);
	fwrite(derail[i].text,140,1,df);
    }

    printf("Derail suggestions saved.\n");
    fclose(df);
}

void derail_load(void) {
    
    FILE* df = sfopen("derail.sug","rb");
    if (!df) return;

    for (int i=0; i < SUG_COUNT; i++) {
	fread(derail[i].nickname,IRC_MAX_NICK_LEN+1,1,df);
	fread(derail[i].text,140,1,df);
    }

    if (!atexit_set) { atexit(derail_save); atexit_set = 1;}
    printf("Derail suggestions loaded.\n");
    fclose(df);
}

int random_suggestion() {

    // returns the index of a random suggestion or -1 if none exist.

    int sugc = 0;
    int sugn[SUG_COUNT];

    for (int i=0; i < SUG_COUNT; i++)
	if (derail[i].nickname[0]) {sugn[sugc] = i; sugc++;}

    //we determine whether a suggestion exists based on whether
    //the nickname has a non-zero length.

    if (sugc == 0) return -1;

    int r = rand() % sugc;
    return sugn[r];
}

int empty_suggestion() {

    // returns the index of an empty suggestion or -1 if none exist.

    for (int i=0; i < SUG_COUNT; i++)
	if (derail[i].nickname[0] == 0) return i;

    return -1;
}

int insert_sug(const char* restrict nickname, const char* restrict text) {
    
    if (!data_loaded) {
	derail_load();
	data_loaded = true;
    }

    int r = empty_suggestion();
    if (r == -1) return 1;

    strncpy(derail[r].nickname,nickname,IRC_MAX_NICK_LEN+1);
    strncpy(derail[r].text,text,140);

    return 0;
}

int derail_addmsg(irc_session_t* session,const char* restrict nickname, const char* restrict channel, const char* restrict msg) {

    if (!channel) return 1; //ignore PMs.

    if (!data_loaded) {
	derail_load();
	data_loaded = true;
    }

    if (strncmp(nickname,d_nickname,IRC_MAX_NICK_LEN+1) != 0) {
	dc_count = 0;
	dc_length = 0;
	strncpy(d_nickname,nickname,IRC_MAX_NICK_LEN+1);
    }

    dc_count++;
    dc_length += strlen(msg);

    if ((dc_count >= COUNT_THRESHOLD) && (dc_length >= LENGTH_THRESHOLD)) {

	//find a random suggestion, send it and delete it.
	int sugid = random_suggestion();

	if (sugid != -1) {
	    ircprintf(session,NULL,channel,"<%.*s> %.140s",IRC_MAX_NICK_LEN+1,derail[sugid].nickname,derail[sugid].text);
	}
	derail[sugid].nickname[0] = 0;
	dc_count = 0;
	dc_length = 0;
    }
    return 0;
}
