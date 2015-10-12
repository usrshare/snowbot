// vim: cin:sts=4:sw=4 
#include "irc_bookmarks.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irc_common.h"

struct irc_bm {
    char nickname[10];
    char text[256];
    bool bm_private;
};

int bm_c = 0;
struct irc_bm* bm_v = NULL;

int realloc_bm(int new_c) {

    int old_c = bm_c;
    bm_v = realloc(bm_v, new_c * sizeof(struct irc_bm));
    bm_c = new_c;

    if (new_c > old_c) memset(bm_v + old_c,0,sizeof(struct irc_bm) * (new_c - old_c));
    return 0;
}

void save_bm() {

    if (!bm_v) return;
    FILE* sf = fopen("bookmarks.bm","wb");
    if (!sf) { perror("Can't open BM save"); return; }

    fwrite(&bm_c,1,sizeof(int),sf);

    for (int i=0; i < bm_c; i++) {

	if (bm_v[i].nickname[0] == 0) continue;

	fwrite(&i,1,sizeof(int),sf);
	fwrite(bm_v[i].nickname,1,10,sf);
	fwrite(bm_v[i].text,1,256,sf);
	fwrite(&bm_v[i].bm_private,1,sizeof(bool),sf);

    }

    fclose(sf);
    return;
}

void load_bm() {
    
    FILE* sf = fopen("bookmarks.bm","rb");
    if (!sf) { perror("Can't open BM load"); return; }

    int new_c;
    fread(&new_c,1,sizeof(int),sf);

    realloc_bm(new_c);

    int i = 0;

    while (!feof(sf)) {
	fread(&i,1,sizeof(int),sf);
	fread(bm_v[i].nickname,1,10,sf);
	fread(bm_v[i].text,1,256,sf);
	fread(&bm_v[i].bm_private,1,sizeof(bool),sf);
    }

    fclose(sf);
    return;


}

int find_empty_bm() {

    if (!bm_v) return 0;
    
    for (int i=0; i < bm_c; i++)
	if (bm_v[i].nickname[0] == 0) return i;

    return bm_c;
}


int add_bm(const char* restrict nickname, const char* restrict text, bool bm_private) {

    if (!bm_v) {
	realloc_bm(1);
    }

    int n = find_empty_bm(); 

    if (n >= bm_c)
	realloc_bm(n+1);

    strncpy(bm_v[n].nickname,nickname,10);
    strncpy(bm_v[n].text,text,256);
    bm_v[n].bm_private = bm_private;

    return 0;
}

int find_bm(int id, const char* restrict nickname, int start_id) {

    if (!bm_v) return -1;
    if (bm_c >= start_id) return -1;

    for (int i=start_id; i < bm_c; i++)
	if (ircstrncmp(bm_v[id].nickname,nickname,10) == 0) return i;

    return -1;
}

const char* get_bm(int id, const char* restrict nickname) {

    if (!bm_v) return NULL;
    if (bm_c >= id) return NULL;

    if ((bm_v[id].bm_private) && (ircstrncmp(bm_v[id].nickname,nickname,10) != 0)) return NULL;

    return (bm_v[id].text);
}

int del_bm(int id, const char* restrict nickname) {
    
    if (!bm_v) return 1;
    if (bm_c >= id) return 1;

    if ((ircstrncmp(bm_v[id].nickname,nickname,10) != 0)) return 2;
    bm_v[id].nickname[0] = 0;
    return 0;
}
