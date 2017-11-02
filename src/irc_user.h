#ifndef IRC_USER_H
#define IRC_USER_H
#include <stdbool.h>
#include <stddef.h>
#include "savefile.h"

enum bot_modes {
    BM_NONE = 0,
    BM_PASTE = 1,
};

struct hashtable;

extern struct hashtable* userht;

enum weather_modes {
    WM_CELSIUS = 0,
    WM_FAHRENHEIT = 1,
    WM_KELVIN = 2,
};

struct irc_user_params{
    //for hash tables. key is nickname.
    enum bot_modes mode;
    bool modified; // were the params modified / should they be saved?

    enum weather_modes wmode;
    unsigned int cityid;
    uint8_t usegraphics;
    uint8_t color_scheme;

    unsigned int channel_count;
    uint8_t logged_in;
    char pwdhash[129];
    char fullnick[129]; // including the ident

    char* paste_title;
    char* paste_text;
    size_t paste_size;
};

extern struct saveparam irc_save_params[];
extern const size_t paramcnt;

enum empty_beh {
    EB_NULL = 0,
    EB_EMPTY = 1,
    EB_LOAD = 2,
};

int save_user_params(const char* restrict nick, struct irc_user_params* up);
int load_user_params(const char* restrict nick, struct irc_user_params* up);
struct irc_user_params* get_user_params(const char* restrict nick, enum empty_beh add_if_empty);
int del_user_params(const char* restrict nick, struct irc_user_params* value);

void saveall();

#endif
