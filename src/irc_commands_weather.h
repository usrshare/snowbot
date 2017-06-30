#ifndef IRC_COMMANDS_WEATHER_H
#define IRC_COMMANDS_WEATHER_H
#include "irc_common.h"

int weather_celsius_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);
int weather_kelvin_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);
int weather_fahrenheit_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);
int weather_current_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);
int weather_forecast_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);
int weather_longforecast_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);

#endif //IRC_COMMANDS_WEATHER_H
