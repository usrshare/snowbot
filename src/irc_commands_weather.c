// vim:cin:sts=4:sw=4
#include "irc_commands_weather.h"
#include "irc_commands.h"

#include <string.h>
#include <math.h>
#include <time.h>

#include "irc_user.h"
#include "weather.h"
const char* wmode_desc( int wmode) {

    switch(wmode) {
	case WM_CELSIUS: return "°C";
	case WM_FAHRENHEIT: return "°F";
	case WM_KELVIN: return "°K";
	default: return "°K";
    }
}
float convert_temp(float temp_k, int wmode) {

    switch(wmode) {
	case WM_CELSIUS: return (temp_k - 273.15f); 
	case WM_FAHRENHEIT: return (((temp_k - 273.15f)*1.8f)+32.0f);
	case WM_KELVIN: return temp_k; 
	default: return temp_k;
    }
}

void fill_format_temp(float temp_k, char** fmtst, char** fmted) {

    *fmted = "\017";
    if (temp_k >= (273.15f + 30.0f)) { *fmtst = "\00300,04"; return;}
    if (temp_k >= (273.15f + 25.0f)) { *fmtst = "\00304"; return;}
    if (temp_k >= (273.15f + 20.0f)) { *fmtst = "\00307"; return;}
    if (temp_k >= (273.15f + 15.0f)) { *fmtst = "\00308"; return;}
    if (temp_k >= (273.15f + 10.0f)) { *fmtst = "\00309"; return;}
    if (temp_k >= (273.15f + 5.0f))  { *fmtst = "\00311"; return;}
    if (temp_k >= (273.15f - 5.0f))  { *fmtst = "\00312"; return;}
    if (temp_k >= (273.15f - 15.0f)) { *fmtst = "\00302"; return;}
    if (temp_k < (273.15f - 15.0f))  { *fmtst = "\00300,02"; return;}
    return;
};
void fill_format_wind(float wspd_ms, char** fmtst, char** fmted) {

    if (wspd_ms >= 30.0f) *fmtst = "\00301,04"; else
	if (wspd_ms >= 25.0f) *fmtst = "\00304"; else
	    if (wspd_ms >= 20.0f) *fmtst = "\00307"; else
		if (wspd_ms >= 15.0f) *fmtst = "\00308"; else
		    if (wspd_ms >= 10.0f) *fmtst = "\00309"; else
			if (wspd_ms >= 5.0f)  *fmtst = "\00311"; else

			    *fmted = "\017";
    return;
};

int handle_weather_current(irc_session_t* session, const char* restrict nick, const char* restrict channel, struct weather_loc* wloc, struct weather_data* wdata) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (!wloc->city_id) {
	respond(session,nick,channel,"Sorry, but OWM returned no results for your location.");
	return 0;
    }

    char* weathermsg = malloc(128);

    char weathertmp[256];

    snprintf (weathermsg,1024,"weather in %s (#%d)",wloc->city_name,wloc->city_id);

    if (strlen(wloc->sys_country) == 2) {
	snprintf (weathertmp,256,",%2s",wloc->sys_country);
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    weathermsg = strrecat(weathermsg,": ");

    for (int i=0; i < (wdata->weather_c); i++) {
	const struct weather_id* wid = getwid(wdata->weather_id[i]);
	if (wid->format_on)
	    snprintf(weathertmp,256,"%s%s%s,",wid->format_on,wid->description,wid->format_off); else
		snprintf(weathertmp,256,"%s,",wid->description);
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    char* fmtst = ""; char* fmted = "";

    fill_format_temp(wdata->main_temp, &fmtst, &fmted);

    snprintf (weathertmp,255,"%s%+.1f%s%s",fmtst,convert_temp(wdata->main_temp,up->wmode),wmode_desc(up->wmode),fmted);

    weathermsg = strrecat(weathermsg,weathertmp);

    if (fabsf(wdata->main_temp_max - wdata->main_temp_min > 0.5f)) {

	snprintf(weathertmp,255," (%+.1f° .. %+.1f°)",convert_temp(wdata->main_temp_min,up->wmode),convert_temp(wdata->main_temp_max,up->wmode));
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    if (wdata->main_pressure >= 0.0) {
	snprintf(weathertmp,255,", pressure: %.1f hPa", wdata->main_pressure);
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    if (wdata->main_humidity >= 0) {
	snprintf(weathertmp,255,", %d%% humidity", wdata->main_humidity);
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    if (wdata->wind_speed >= 0.0) {

	int wspd = (int) round(wdata->wind_speed);

	char* fmtst = ""; char* fmted = "";

	fill_format_wind(wspd,&fmtst,&fmted);

	snprintf(weathertmp,255,", wind: %s%.1f%s m/s (%s%.1f%s mph)", fmtst,wdata->wind_speed,fmted, fmtst,wdata->wind_speed * 2.23694f,fmted);
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    if (wdata->wind_deg >= 0.0) {
	snprintf(weathertmp,255," %s", describe_wind_direction(wdata->wind_deg));
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    respond(session,nick,channel,weathermsg);
    free(weathermsg);
    return 0;
}
int handle_long_forecast(irc_session_t* session, const char* restrict nick, const char* restrict channel, struct weather_loc* wloc, struct forecast_data* wdata, int cnt) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (!(wloc[0].city_id)) {
	respond(session,nick,channel,"Sorry, but OWM returned no results for your location.");
	return 0;
    }

    while (wdata[cnt-1].temp_day < 1.0) cnt--; //avoid -273

    char* weathermsg = malloc(128);

    char weathertmp[512];

    snprintf (weathermsg,128,"d#:");

    char weathertmp2[32];
    memset(weathertmp,0,sizeof weathertmp);

    struct tm weathertime;
    memset(&weathertime,0,sizeof weathertime);

    for (int i=0; i<cnt; i++) {

	gmtime_r(&((wdata+i)->dt), &weathertime);

	char* daycolor = "";
	if (weathertime.tm_wday == 6) daycolor = "\00312";
	else if (weathertime.tm_wday == 0) daycolor = "\00304";
	snprintf(weathertmp2,16,"%s%3d \017",daycolor,weathertime.tm_mday);
	strcat(weathertmp,weathertmp2);
    }

    weathermsg = strrecat(weathermsg,weathertmp);

    respond(session,nick,channel,weathermsg);

    snprintf (weathermsg,128,"%s:",wmode_desc(up->wmode));

    memset(weathertmp,0,sizeof weathertmp);
    for (int i=0; i<cnt; i++) {
	snprintf(weathertmp2,16,"\00308%3d \017",(int)round(convert_temp((wdata+i)->temp_day,up->wmode)));
	strcat(weathertmp,weathertmp2);
    }
    weathermsg = strrecat(weathermsg,weathertmp);
    respond(session,nick,channel,weathermsg);

    snprintf (weathermsg,128,"%s:",wmode_desc(up->wmode));

    memset(weathertmp,0,sizeof weathertmp);
    for (int i=0; i<cnt; i++) {
	snprintf(weathertmp2,16,"\00312%3d \017",(int)round(convert_temp((wdata+i)->temp_night,up->wmode)));
	strcat(weathertmp,weathertmp2);
    }
    weathermsg = strrecat(weathermsg,weathertmp);
    respond(session,nick,channel,weathermsg);

    snprintf (weathermsg,128,"wd:");

    for (int i=0; i<cnt; i++) {

	int wspd = (int) round((wdata+i)->wind_speed);

	char* fmtst = ""; char* fmted = "";
	fill_format_wind(wspd,&fmtst,&fmted);

	snprintf(weathertmp,255,"%s%3.0f %s", fmtst,(wdata+i)->wind_speed,fmted);
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    respond(session,nick,channel,weathermsg);

    int wcnt = 0;
    for (int i=0; i<cnt; i++)
	if ((wdata+i)->weather_c > wcnt) wcnt = (wdata+i)->weather_c;

    for (int c=0; c < wcnt; c++) {

	snprintf (weathermsg,128,"sp:");

	memset(weathertmp,0,sizeof weathertmp);
	//char weathertmp3[16];
	for (int i=0; i<cnt; i++) {

	    const struct weather_id* wid = getwid((wdata+i)->weather_id[c]);

	    if (wid->format_on)
		snprintf(weathertmp2,16," %s%s%s ",wid->format_on,wid->symbol,wid->format_off); else
		    snprintf(weathertmp2,16," %s ",wid->symbol);
	    strcat(weathertmp,weathertmp2);
	}
	weathermsg = strrecat(weathermsg,weathertmp);
	respond(session,nick,channel,weathermsg);
    }
    free(weathermsg);
    return 0;
}
int handle_weather_forecast(irc_session_t* session, const char* restrict nick, const char* restrict channel, struct weather_loc* wloc, struct weather_data* wdata, int cnt) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (!(wloc[0].city_id)) {
	respond(session,nick,channel,"Sorry, but OWM returned no results for your location.");
	return 0;
    }

    while (wdata[cnt-1].main_temp < 1.0) cnt--; //avoid -273

    char* weathermsg = malloc(128);

    char weathertmp[512];

    snprintf (weathermsg,128,"hr:");

    char weathertmp2[32];
    memset(weathertmp,0,sizeof weathertmp);

    struct tm weathertime;
    memset(&weathertime,0,sizeof weathertime);

    for (int i=0; i<cnt; i++) {

	gmtime_r(&((wdata+i)->dt), &weathertime);
	snprintf(weathertmp2,16,"%3d",weathertime.tm_hour);
	strcat(weathertmp,weathertmp2);
    }

    weathermsg = strrecat(weathermsg,weathertmp);

    respond(session,nick,channel,weathermsg);

    snprintf (weathermsg,128,"%s:",wmode_desc(up->wmode));

    memset(weathertmp,0,sizeof weathertmp);
    char* fmtst = ""; char* fmted = "";

    for (int i=0; i<cnt; i++) {

	fill_format_temp(wdata[i].main_temp,&fmtst,&fmted);
	snprintf(weathertmp2,16,"%s%3d%s",fmtst,(int)round(convert_temp((wdata+i)->main_temp,up->wmode)),fmted);
	strcat(weathertmp,weathertmp2);
    }
    weathermsg = strrecat(weathermsg,weathertmp);
    respond(session,nick,channel,weathermsg);


    snprintf(weathermsg,128,"wd:");

    memset(weathertmp,0,sizeof weathertmp);
    for (int i=0; i<cnt; i++) {

	int wspd = (int) round((wdata+i)->wind_speed);

	char* fmtst = ""; char* fmted = "";

	fill_format_wind(wspd,&fmtst,&fmted);

	snprintf(weathertmp2,16,"%s%3d%s",fmtst,wspd,fmted);
	strcat(weathertmp,weathertmp2);
    }

    weathermsg = strrecat(weathermsg,weathertmp);
    respond(session,nick,channel,weathermsg);

    int wcnt = 0;
    for (int i=0; i<cnt; i++)
	if ((wdata+i)->weather_c > wcnt) wcnt = (wdata+i)->weather_c;

    for (int c=0; c < wcnt; c++) {

	snprintf (weathermsg,128,"sp:");

	memset(weathertmp,0,sizeof weathertmp);
	//char weathertmp3[16];
	for (int i=0; i<cnt; i++) {

	    const struct weather_id* wid = getwid((wdata+i)->weather_id[c]);

	    if (wid->format_on)
		snprintf(weathertmp2,16," %s%s%s",wid->format_on,wid->symbol,wid->format_off); else
		    snprintf(weathertmp2,16," %s",wid->symbol);

	    strcat(weathertmp,weathertmp2);
	}
	weathermsg = strrecat(weathermsg,weathertmp);
	respond(session,nick,channel,weathermsg);
    }
    return 0;
}
int handle_weather_search(irc_session_t* session, const char* restrict nick, const char* restrict channel, struct weather_loc* wloc, struct weather_data* wdata, int cnt) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (cnt == 0) {
	respond(session,nick,channel,"Sorry, but OWM returned no results for your location.");
	return 0;
    }

    while (wdata[cnt-1].main_temp < 1.0) cnt--; //avoid -273

    for (int i=0; i < cnt; i++) {
	
	char weathertmp[512];
    
	char* t_fmtst = ""; char* t_fmted = "";
	fill_format_temp(wdata[i].main_temp,&t_fmtst,&t_fmted);
	
	const struct weather_id* wid = getwid(wdata[i].weather_id[0]);

	snprintf(weathertmp,512,"%d. #%-7d: %s,%s [%.2f°%c, %.2f°%c] / %s%+.1f%s%s, %s%s%s", 
		i+1, wloc[i].city_id, wloc[i].city_name, wloc[i].sys_country, 
		fabs(wloc[i].coord_lat), (wloc[i].coord_lat >= 0 ? 'N' : 'S'), //latitude
		fabs(wloc[i].coord_lon), (wloc[i].coord_lon >= 0 ? 'E' : 'W'), //longitude
		t_fmtst, convert_temp(wdata[i].main_temp,up->wmode),wmode_desc(up->wmode), t_fmted,
		wid->format_on ? wid->format_on : "",
		wid->description,
		wid->format_off ? wid->format_off : "");
	
    
	respond(session,nick,NULL,weathertmp);
    }

    return 0;
}

int escape_location (char* city_name) {

    for (unsigned int i=0; i < strlen(city_name); i++)
	if (city_name[i] == ' ') city_name[i] = '+';

    return 0;
}

int load_location (int argc, const char** argv, struct irc_user_params* up, struct weather_loc* wloc) {

    //argv[0] refers to the actual first parameter, not name of command.

    if (argc == 0) return 1;

    if (argv[0][0] == '#') { //city id
	char* endid = NULL;
	wloc->city_id = strtol(argv[0]+1,&endid,10);
	if (endid != (argv[0]+1)) return 0; else return 1;
    }

    if (argv[0][0] == '@') { //city id
	strncpy(wloc->postcode,argv[0]+1,16);
	if (argc == 2) strncpy(wloc->sys_country,argv[1],2);
	return 0;
    }

    if (argc == 2) {

	float lat = -91.0f, lon = -181.0f;
	char* endlat = NULL, *endlon = NULL;

	lat = strtof(argv[0],&endlat);
	lon = strtof(argv[1],&endlon);

	if ((endlat != argv[0]) && (endlon != argv[1])) {
	    wloc->coord_lat = lat; wloc->coord_lon = lon;
	    return 0;
	}
    }

    strncpy(wloc->city_name,argv[0],64);
    escape_location(wloc->city_name);

    if (argc == 2) {
	strncpy(wloc->sys_country,argv[1],2);
    }

    return 0;
}

int weather_current_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    struct weather_loc wloc;
    memset(&wloc,0,sizeof wloc);
    struct weather_data wdata;
    memset(&wdata,0,sizeof wdata);

    if ((argc == 1) && (!up->cityid)) {
	respond(session,nick,channel,"Usage: .owm <location>");
	respond(session,nick,channel,"Location is one of:  #<OWM city ID>, @<zip code>[ <2char country code>], \"<city name>\"[ <2char country code>], <longitude>,<latitude>"); return 0; }

    else {
	int r = load_location(argc-1, argv+1,up,&wloc);

	if (r) r = ((wloc.city_id = up->cityid) == 0);
	if (r) {respond(session,nick,channel,"Can't understand the parameters. Sorry."); return 0;}
    }

    get_current_weather( &wloc, &wdata);

    handle_weather_current(session, nick, channel, &wloc, &wdata);
    return 0;
}	
int weather_forecast_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    struct weather_loc wloc;
    memset(&wloc,0,sizeof wloc);

    if ((argc == 1) && (!up->cityid)) {
	respond(session,nick,channel,"Usage: .owf <# of 1-day intervals> <location>");
	respond(session,nick,channel,"Location is one of:  #<OWM city ID>, @<zip code>[ <2char country code>], \"<city name>\"[ <2char country code>], <longitude>,<latitude>"); return 0; }

    int cnt = 0;

    char* endcnt = NULL;

    int r = 1;

    if (argc > 1) 
    { cnt = strtol(argv[1],&endcnt,10);

	if (cnt < 0) cnt = 1; if (cnt == 0) cnt = 16; if (cnt>40) cnt=40;
	if (endcnt == argv[1]) cnt = 16; } else cnt = 16;

    struct weather_data wdata[cnt];
    memset(&wdata,0,sizeof (struct weather_data) *cnt);

    if (argc > 1) r = load_location((endcnt != argv[1]) ? argc-2 : argc-1,(endcnt != argv[1]) ? argv+2: argv+1,up,&wloc);
    if (r) r = ((wloc.city_id = up->cityid) == 0);

    if (r) respond(session,nick,channel,"Can't understand the parameters. Sorry."); else {

	cnt = get_weather_forecast( &wloc, wdata, cnt);
	handle_weather_forecast( session, nick, channel, &wloc, wdata, cnt);

    }
    return 0;
}	
int weather_longforecast_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    struct weather_loc wloc;
    memset(&wloc,0,sizeof wloc);

    if ((argc == 1) && (!up->cityid)) {
	respond(session,nick,channel,"Usage: .owl <# of 1-day intervals> <location>");
	respond(session,nick,channel,"Location is one of:  #<OWM city ID>, @<zip code>[ <2char country code>], \"<city name>\"[ <2char country code>], <longitude>,<latitude>"); return 0; }

    int cnt = 0;

    char* endcnt = NULL;

    int r = 1;

    if (argc > 1) 
    { cnt = strtol(argv[1],&endcnt,10);

	if (cnt < 0) cnt = 1; 
	if (endcnt == argv[1]) cnt = 7; }

    if (cnt == 0) cnt = 7; if (cnt>15) cnt=15;

    struct forecast_data wdata[cnt];
    memset(&wdata,0,sizeof (struct forecast_data) *cnt);

    if (argc > 1) r = load_location((endcnt != argv[1]) ? argc-2 : argc-1,(endcnt != argv[1]) ? argv+2: argv+1,up,&wloc);
    if (r) r = ((wloc.city_id = up->cityid) == 0);

    if (r) respond(session,nick,channel,"Can't understand the parameters. Sorry."); else {

	cnt = get_long_forecast( &wloc, wdata, cnt);
	handle_long_forecast( session, nick, channel, &wloc, wdata, cnt);

    }
    return 0;
}	

#define MAXSEARCH 10

int weather_search_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    struct weather_loc wloc;
    memset(&wloc,0,sizeof wloc); //input location
    struct weather_loc o_wloc[MAXSEARCH];
    memset(o_wloc,0,sizeof o_wloc); //output location
    struct weather_data o_wdata[MAXSEARCH];
    memset(o_wdata,0,sizeof o_wdata); //output data

    if ((argc == 1) && (!up->cityid)) {
	respond(session,nick,channel,"Usage: .owm_s <location>");
	respond(session,nick,channel,"Location is one of:  #<OWM city ID>, @<zip code>[ <2char country code>], \"<city name>\"[ <2char country code>], <longitude>,<latitude>"); return 0; }

    else {
	int r = load_location(argc-1, argv+1,up,&wloc);

	if (r) r = ((wloc.city_id = up->cityid) == 0);
	if (r) {respond(session,nick,channel,"Can't understand the parameters. Sorry."); return 0;}
    }

    int c = search_weather( &wloc, MAXSEARCH, o_wloc, o_wdata);

    handle_weather_search(session, nick, channel, o_wloc, o_wdata, c);
    return 0;
}	
