// vim: cin:sts=4:sw=4 
#ifndef IRC_BOOKMARKS_H
#define IRC_BOOKMARKS_H

#include <stdbool.h>

void save_bm();
void load_bm();
int add_bm(const char* restrict nickname, const char* restrict text, bool bm_private);
int find_bm(int id, const char* restrict nickname, int start_id);
const char* get_bm(int id, const char* restrict nickname);
int del_bm(int id, const char* restrict nickname);

#endif
