// vim: cin:sts=4:sw=4 
#include "irc_commands.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "derail.h"
#include "irc_common.h"
#include "irc_user.h"
#include "irc_watch.h"

#include "convert.h"
#include "pwdhash.h"
#include "short.h"
#include "weather.h"
#include "xrates.h"

#include <math.h>
#include <time.h>

#include <pthread.h>

typedef int (*irc_command_cb) (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);

struct irc_user_commands {

    const char* name;
    bool no_parse_parameters; //if true, argc is set to 1 and entire string is stored in argv[0].
    bool use_full_nickname; //if true, nick will pass whole nickname, including hostname
    irc_command_cb cb;
};

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
    
    if (temp_k >= (273.15f + 30.0f)) *fmtst = "\00301,04"; else
    if (temp_k >= (273.15f + 25.0f)) *fmtst = "\00304"; else
    if (temp_k >= (273.15f + 20.0f)) *fmtst = "\00307"; else
    if (temp_k >= (273.15f + 15.0f)) *fmtst = "\00308"; else
    if (temp_k >= (273.15f + 10.0f)) *fmtst = "\00309"; else
    if (temp_k >= (273.15f + 5.0f))  *fmtst = "\00311"; else
    if (temp_k >= (273.15f - 5.0f))  *fmtst = "\00312"; else 
    if (temp_k >= (273.15f - 15.0f)) *fmtst = "\00302"; else
    if (temp_k < (273.15f - 15.0f))  *fmtst = "\00301,02";

    *fmted = "\017";
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

int helpcmd_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    respond(session,nick,channel,"Read the documentation at https://github.com/usrshare/snowbot/wiki .");
    return 0;
}

/*int weather_channel(irc_session_t* session, const char* restrict channel, struct weather_loc* wloc, struct weather_data* wdata) {
//
//    strcpy(wloc->sys_country,"EF");
//    strcpy(wloc->city_name,channel);
//    wloc->city_id = -1;
//    wloc->postcode[0] = 0; //empty the string
//
//    time_t t1h = time(NULL)-3600;
//    time_t t10m = time(NULL)-600;
//
//    unsigned int lasthour = watch_getlength(NULL,channel,t1h,0,NULL,NULL);
//
//    unsigned int bs1,bs2;
//
//    unsigned int snowmsgs = watch_getlength("snow_",channel,t1h,0,NULL,&bs1) + watch_getlength("snow^",channel,t1h,0,NULL,&bs2);
//    bs1 += bs2;
//    unsigned int snowmsg2 = watch_getlength("snow_",channel,t10m,0,NULL,NULL) + watch_getlength("snow^",channel,t10m,0,NULL,NULL);
//
//    float chantemp = 273.15f - 10.0f + powf((float)lasthour,(1.0f /3.0f));
//    //assume 30 messages on average. temperatures will range from -5C to whatever.
//
//    wdata->main_temp = chantemp;
//    wdata->main_temp_min = wdata->main_temp_max = chantemp;
//
//    float snowpct = lasthour ? ((float)snowmsgs / lasthour): 0.0f;
//
//    if (!snowmsg2) snowpct /= 2.0f;
//
//    wdata->main_pressure = -1.0f;
//    wdata->main_humidity = -1.0f;
//
//    wdata->weather_c = 1;
//
//    if (snowpct >= .40f) wdata->weather_id[0] = 602; else
//	if (snowpct >= .20f) wdata->weather_id[0] = 601; else
//	    if (snowpct >= .10f) wdata->weather_id[0] = 600; else
//		wdata->weather_id[0] = 800;
//
//    if (bs1) {
//	wdata->weather_c = 2;
//	if (bs1 >= 4) wdata->weather_id[1] = 212; else
//	    if (bs1 >= 2) wdata->weather_id[1] = 211; else
//		wdata->weather_id[1] = 210;
//    }
//
//    unsigned int windmsgs = watch_getlength("wind",channel,t1h,0,NULL,NULL);
//
//    float windspd = (float)windmsgs / 50.0f;
//    wdata->wind_speed = windspd;
//
//    return 0;
//}
 */	

/*int weather_is_channel(const char* restrict arg) {

    if ((arg[0] == '#') && (strtol(arg+1,NULL,10) == 0)) return 1; 

    return 0;
}*/

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

	//if ((r) && (argc > 1)) r = !weather_is_channel(argv[1]);
	if (r) r = ((wloc.city_id = up->cityid) == 0);
	if (r) {respond(session,nick,channel,"Can't understand the parameters. Sorry."); return 0;}
    }

    //if ( (argc > 1) && (weather_is_channel(argv[1])) ) 
    //weather_channel(session, argv[0]+5,&wloc,&wdata);
    //else
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

int load_cmd_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    int r = load_user_params(nick,up);
    if (r == 0) respond(session,nick,channel,"Your user parameters have been loaded.");
    else respond(session,nick,channel,"Error while loading your user parameters.");
    return 0;

}
int save_cmd_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    int r = save_user_params(nick,up);
    if (r == 0) respond(session,nick,channel,"Your user parameters have been saved.");
    else respond(session,nick,channel,"Error while saving your user parameters.");
    return 0;

}
int utc_cmd_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    time_t utc = time(NULL);
    char buf[30];

    ctime_r(&utc,buf);

    ircprintf(session,nick,channel,"Current time in UTC: %s", buf);
    return 0;

}

int set_cmd_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (argc == 3) {

	int r = setparam(up, irc_save_params, paramcnt, argv[1], argv[2]);
	if (r == 0) respond (session,nick,channel,"Parameter set successfully."); else respond (session,nick,channel,"Can't set parameter.");

	//set parameter to value
    } else if (argc == 2) {

	char val[128];

	int r = getparam(up, irc_save_params, paramcnt, argv[1], val, 128);
	if (r == 0) ircprintf (session,nick,channel,"%s = %s",argv[1],val); else respond (session,nick,channel,"Can't get parameter.");

	//get value of parameter and respond
    } else {

	char val[128];

	for (unsigned int i = 0; i < paramcnt; i++) {
	    if (irc_save_params[i].visibility == 0) continue;

	    int r = getparam(up, irc_save_params, paramcnt, irc_save_params[i].name, val, 128);
	    if (r == 0) ircprintf (session,nick,channel,"%s = %s",irc_save_params[i].name,val); else ircprintf (session,nick,channel,"%s = ???",irc_save_params[i].name);


	}
    }
    return 0;
}


int xr_cmd_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc == 1) {
	respond(session,nick,channel,"Usage: .xr [currency list], .xr [number] [src currency] [dest currency]"); return 0;
    }	

    if (argc == 2) {

	int c = cnt_tokens(argv[1],",");

	struct exchange_rate res[c];

	char* curlist = strdup(argv[1]);

	char* saveptr = NULL;

	int ci=0; char* curtok = strtok_r(curlist,",",&saveptr);

	do {

	    res[ci].rate = 0.0;
	    strncpy(res[ci].symbol,curtok,4);

	    ci++;
	    curtok = strtok_r(NULL,",",&saveptr);

	} while (curtok && (ci < c));

	int r = get_exchange_rates(ci,res);

	if (r == 0) {
	    for (int i=0; i < ci; i++)
		ircprintf(session,nick,channel,"%s = $%.3f ($1 = %.3f %s)",res[i].symbol,1 / res[i].rate, res[i].rate, res[i].symbol); }
	else ircprintf(session,nick,channel,"Error while retrieving exchange rates."); 
    }

    if (argc == 4) {

	float count = atof(argv[1]);

	struct exchange_rate res[2];

	memset(res, 0, 2* sizeof(res[0]));

	strncpy(res[0].symbol,argv[2],4);
	strncpy(res[1].symbol,argv[3],4);

	int r = get_exchange_rates(2,res);

	if (r == 0) 
	    ircprintf(session,nick,channel,"%.3f %s = %.3f %s",res[0].symbol,count, res[1].symbol, count * res[1].rate / res[0].rate);
	else ircprintf(session,nick,channel,"Error while retrieving exchange rates."); 
    }

    return 0;

}

int convert_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc == 1) {
	ircprintf(session,nick,channel,"Usage: %s [number] [src measure] [dest measure], %s [ft'in] [dest measure]",argv[0],argv[0]); return 0;
    }	

    if (argc == 3) {

	if (strchr(argv[1],'\'')) {

	    double ft, in, ftin;

	    int r = sscanf(argv[1],"%lf'%lf\"",&ft,&in);

	    if (r != 2) {ircprintf(session,nick,channel,"Couldn't recognize inch value."); return 0;}

	    if (ft < 0) in *= -1;
	    ftin = in + (ft * 12);

	    double res = convert_value(ftin, "in", argv[2]);

	    if (!isnan(res)) ircprintf(session,nick,channel,"%.0f'%.3f\" = %.3f %s",ft, fabs(in), res, argv[2]);
	    else ircprintf(session,nick,channel,"Error while converting: %s.",conv_strerror[conv_errno]); 
	} else {

	    char* measure = NULL;	    
	    double count = strtod(argv[1],&measure);

	    if (*measure == 0) {ircprintf(session,nick,channel,"Invalid measure."); return 0; }

	    double res = convert_value(count, measure, argv[2]);

	    if (!isnan(res)) ircprintf(session,nick,channel,"%.3f %s = %.3f %s",count, measure, res, argv[2]);
	    else ircprintf(session,nick,channel,"Error while converting: %s.",conv_strerror[conv_errno]); 

	}

    }

    if (argc == 4) {

	double count = atof(argv[1]);

	double res = convert_value(count, argv[2], argv[3]);

	if (!isnan(res)) ircprintf(session,nick,channel,"%.3f %s = %.3f %s",count, argv[2], res, argv[3]);
	else ircprintf(session,nick,channel,"Error while converting: %s.",conv_strerror[conv_errno]); 
    }

    return 0;

}

int weather_celsius_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    respond(session,nick,channel,"Weather responses set to Celsius.");
    up->wmode = WM_CELSIUS;
    return 0;
}

int weather_kelvin_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    respond(session,nick,channel,"Weather responses set to Kelvin.");
    up->wmode = WM_KELVIN;
    return 0;
}

int weather_fahrenheit_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    respond(session,nick,channel,"Weather responses set to Fahrenheit.");
    up->wmode = WM_FAHRENHEIT;
    return 0;
}

int start_paste_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    respond(session,nick,channel,"Paste mode enabled. All input not starting with a dot will be interpreted as strings to paste. End your document by sending a single dot. To insert a string starting with a dot, prepend another dot.");

    up->mode = BM_PASTE;
    up->paste_text = NULL;
    up->paste_size = 0;

    if (argc >= 2) up->paste_title = strdup(argv[1]);
    return 0;
}

char* risingvblocks[] = {" ","▁","▂","▃","▄","▅","▆","▇","█","▒"};
char* risinghblocks[] = {" ","▏","▎","▍","▌","▋","▊","▉","█","▒"};

int charcountgraph_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc != 4) { ircprintf(session,nick,channel,"Usage: %s <nickname> <seconds> <intervals>",argv[0]); return 0;}

    int seconds = atoi(argv[2]);
    int intervals = atoi(argv[3]);

    int bytes[intervals];
    memset(bytes,0,sizeof(int) * intervals);

    const char* nickparam = argv[1];
    if (strcmp(argv[1],"*") == 0) nickparam = NULL;

    count_by_period(nickparam,time(NULL) - (seconds * intervals),seconds,intervals,bytes);

    char* output = strdup("|");

    int maxbytes = 1;

    for (int i=0; i < intervals; i++)
	if (bytes[i] > maxbytes) maxbytes = bytes[i];

    for (int i=0; i < intervals; i++)
    {
	int fill = bytes[i] ? ((bytes[i] * 8 / maxbytes) + 1) : 0;
	output = strrecat(output,risingvblocks[fill]);
    }

    output = strrecat(output,"|");

    respond(session,nick,channel,output);

    free(output);

    return 0;
}

int charcount_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc == 1) { ircprintf(session,nick,channel,"Usage: %s <nickname> [seconds]",argv[0]); return 0;}

    int seconds = ((argc >= 3) ? atoi(argv[2]) : 0);

    time_t tmin = time(NULL) - seconds;

    unsigned int wc = 0;

    const char* nickparam = argv[1];
    if (strcmp(argv[1],"*") == 0) nickparam = NULL;

    unsigned int r = watch_getlength(nickparam,NULL,seconds ? tmin : 0,0,&wc,NULL);

    if (seconds)
	ircprintf(session,nick,channel,"I've seen %s post %d words (%d bytes) in the last %d seconds.",argv[1],wc,r,seconds);
    else
	ircprintf(session,nick,channel,"I remember %s posting %d words (%d bytes) in the last %d messages.",argv[1],wc,r,watch_countmsg());

    return 0;
}

int suggest_derail_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc == 1) { ircprintf(session,nick,channel,"Usage: %s <\"suggestion text\">",argv[0]); return 0;}
    if (argc > 2) { ircprintf(session,nick,channel,"Please use quotation marks before and after the suggestion text."); return 0;}

    int r = insert_sug(nick,argv[1]);

    if (r)
	ircprintf(session,nick,channel,"Sorry, but the suggestion buffer is currently full.");
    else
	ircprintf(session,nick,channel,"Your suggestion has been added.");

    return 0;
}

int hash_sha512_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc == 1) { ircprintf(session,nick,channel,"Usage: %s <\"plaintext\">",argv[0]); return 0;}
    if (argc > 2) { ircprintf(session,nick,channel,"Please use quotation marks before and after the plaintext."); return 0;}

    char sha512[64];

    hash_pwd(0,NULL,argv[1],sha512);

    char hex[129];

    hash_hex(sha512,64,hex);

    ircprintf(session,nick,channel,hex);

    return 0;
}

int set_password_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (channel) {
	ircprintf(session,nick,channel,"\"%s\" is a private-only command. Do not use it in IRC channels.\n"); return 0;}

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (strlen(up->pwdhash)) {

	printf("User %s already has a password.\n",nick);

	if (argc != 3) { ircprintf(session,nick,channel,"Usage: %s <\"old_password\"> <\"new_password\">",argv[0]); return 0;}

	char sha512_o[64];
	hash_pwd(0,NULL,argv[1],sha512_o);
	char hex_o[129];
	hash_hex(sha512_o,64,hex_o);

	int r = strncmp(up->pwdhash,hex_o,129);
	if (r) {ircprintf(session,nick,channel,"Wrong password."); return 0;}

	char sha512[64];
	hash_pwd(0,NULL,argv[1],sha512);
	char hex[129];
	hash_hex(sha512,64,hex);

	strncpy(up->pwdhash,hex,129);
	ircprintf(session,nick,channel,"Password set."); 

    } else {

	printf("User %s doesn't have a password.\n",nick);

	if (argc != 2) { ircprintf(session,nick,channel,"Usage: %s <\"new_password\">",argv[0]); return 0;}

	char sha512[64];
	hash_pwd(0,NULL,argv[1],sha512);
	char hex[129];
	hash_hex(sha512,64,hex);
	strncpy(up->pwdhash,hex,129);
	ircprintf(session,nick,channel,"Password set."); 

    }
    return 0;
}

int check_login_cb (irc_session_t* session, const char* restrict origin, const char* restrict channel, size_t argc, const char** argv) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (up->logged_in) {

	ircprintf(session,nick,channel,"You're logged in."); 

    } else {

	ircprintf(session,nick,channel,"You're not logged in.");
	return 0;
    }
    return 0;
}

int login_cb (irc_session_t* session, const char* restrict origin, const char* restrict channel, size_t argc, const char** argv) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    if (channel) {
	ircprintf(session,nick,channel,"\"%s\" is a private-only command. Do not use it in IRC channels.\n"); return 0;}

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    if (strlen(up->pwdhash)) {

	if (argc != 2) { ircprintf(session,nick,channel,"Usage: %s <\"password\">",argv[0]); return 0;}

	char sha512_o[64];
	hash_pwd(0,NULL,argv[1],sha512_o);
	char hex_o[129];
	hash_hex(sha512_o,64,hex_o);

	int r = strncmp(up->pwdhash,hex_o,129);
	if (r) {ircprintf(session,nick,channel,"Wrong password."); return 0;}

	strncpy(up->fullnick,origin,129);
	up->logged_in = 1;
	ircprintf(session,nick,channel,"Logged in successfully."); 

    } else {

	ircprintf(session,nick,channel,"First, set your password using the \".setpwd\" command.");
	return 0;
    }
    return 0;
}

const char* roll_usage = "Usage: %1$s [sides], %1$s [dice]d[sides], %1$s [dice] [sides]";

int roll_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    unsigned int numdice = 1, sides = 6;

    if (argc == 2) {

	if (strchr(argv[1],'d')) {

	    if (argv[1][0] == 'd') {

		if (strlen(argv[1]) == 1) return 1;
		sides = atoi(argv[1]+1);

	    } else {

		int r = sscanf(argv[1],"%dd%d",&numdice,&sides);
		if (r == 0) {ircprintf(session,nick,channel,roll_usage,argv[0]); return 1;} }
	} else sides = atoi(argv[1]);

    } else if (argc == 3) {
	numdice = atoi(argv[1]);
	sides = atoi(argv[2]);
    } else { ircprintf(session,nick,channel,roll_usage,argv[0]); return 1;}

    if ((numdice == 0) || (sides == 0)) { ircprintf(session,nick,channel,roll_usage,argv[0]); return 1;}
    if ((numdice > 20) || (sides > 100)) { ircprintf(session,nick,channel,"Values too large to parse."); return 1;}

    char res[512]; char restemp[16];

    sprintf(res,"%dd%d = ",numdice,sides); 

    int r_i, r_s=0;

    for (unsigned int i=0; i < numdice; i++) {

	r_i = (rand() % sides) + 1; r_s += r_i;

	sprintf(restemp,"%2d%s", r_i, (i == (numdice-1)) ? "" : "+");
	strncat(res, restemp, 512 - strlen(res) - 1);

    }

    if (numdice > 1) {    
	sprintf(restemp," = %d", r_s);
	strncat(res, restemp, 512 - strlen(res) - 1);
    }

    ircprintf(session,nick,channel,res);
    return 0;

}

int shorten_url_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    char last_url[512]; last_url[0] = 0;  

    int r = search_url((argc >= 2) ? argv[1] : NULL,last_url);

    if (r == 1) {

	if (argc >= 2) 
	    ircprintf(session,nick,channel,"No URL matching \"%s\" found in buffer.\n",argv[1]);
	else
	    ircprintf(session,nick,channel,"No URLs found in buffer.\n");
	return 0;}

    if (r == 2) { ircprintf(session,nick,channel,"Request \"%s\" matches more than 1 URL. Please use a more specific request.\n",argv[1]); return 0;}

    if (strlen(last_url) > 0) {	    
	char* shurl = irc_shorten(last_url);
	ircprintf(session,nick,channel,"Short URL: \00312%s\017",shurl);
	if (shurl) free(shurl);
    }

    return 0;

}

struct ison_param {
    irc_session_t* session;
    char* restrict nick;
    char* restrict channel;
    int count;
    const char** nicknames;
    bool* statuses;
};
void* ison_thread_func (void* param) {

    struct ison_param* ctx = param;

    int r = ison_request(ctx->session,ctx->count,ctx->nicknames,ctx->statuses);

    if (r) { ircprintf(ctx->session,ctx->nick,ctx->channel,"Error %d.",r); return 0; }

    for (int i=0; i < ctx->count; i++) {
	ircprintf(ctx->session,ctx->nick,ctx->channel,"%s is %s.",ctx->nicknames[i],ctx->statuses[i] ? "online" : "offline");
    }

    free(ctx->nick);
    if (ctx->channel) free(ctx->channel);
    for (int i=0; i < ctx->count; i++) free((void*)ctx->nicknames[i]);
    free(ctx->nicknames);
    free(ctx->statuses);
    free(ctx);
    return NULL;
}

int ison_test_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc == 1) { ircprintf(session,nick,channel,"Usage: %s <nickname> [nickname] ...",argv[0]); return 0;}

    struct ison_param* ip = malloc(sizeof(struct ison_param));

    ip->session = session;
    ip->nick = strdup(nick);
    ip->channel = channel ? strdup(channel) : NULL;
    ip->count = argc-1;
    ip->nicknames = malloc(sizeof(const char*) * ip->count);
    for (int i=0; i < ip->count; i++) ip->nicknames[i] = strdup(argv[i+1]);
    ip->statuses = malloc(sizeof(bool) * ip->count);
    memset(ip->statuses,0,sizeof(bool) * ip->count);

    pthread_t ison_thread;

    pthread_create(&ison_thread,NULL,ison_thread_func,ip);

    return 0;
}

struct irc_user_commands cmds[] = {
    {".help",   false, false, helpcmd_cb},
    {".load",   false, false, load_cmd_cb},
    {".save",   false, false, save_cmd_cb},
    {".set",    false, false, set_cmd_cb},
    {".utc",    false, false, utc_cmd_cb},
    {".w_c",    false, false, weather_celsius_cb},
    {".w_k",    false, false, weather_kelvin_cb},
    {".w_f",    false, false, weather_fahrenheit_cb},
    {".owm",    false, false, weather_current_cb},
    {".owf",    false, false, weather_forecast_cb},
    {".owl",    false, false, weather_longforecast_cb},
    {".cc",     false, false, charcount_cb},
    {".ccg",    false, false, charcountgraph_cb},
    {".xr",     false, false, xr_cmd_cb},
    {".startp", false, false, start_paste_cb},
    {".paste",  false, false, start_paste_cb},
    {".rant",   false, false, start_paste_cb},
    {".sug",    false, false, suggest_derail_cb},
    {".sha512", false, false, hash_sha512_cb},
    {".setpwd", false, false, set_password_cb},
    {".su",     false, false, shorten_url_cb},
    {".login",  false, true,  login_cb},
    {".checkl", false, true,  check_login_cb},
    {".cv",	false, false, convert_cb},
    {".roll",   false, false, roll_cb},
    {".about",  false, false, NULL},
};

struct floodwatch {
    char nick[10];
    time_t timestamp;
};

#define HIST_LEN 20
struct floodwatch lastcmds[HIST_LEN];
int curcmd = 0; bool hist_empty = true;

void add_history(const char* restrict nick) {

    if (hist_empty) { memset(lastcmds, 0, sizeof lastcmds); hist_empty = false;}

    time_t ct = time(NULL);

    strncpy(lastcmds[curcmd].nick,nick,10);
    lastcmds[curcmd].timestamp = ct;

    curcmd = (curcmd + 1) % HIST_LEN;
}

int get_speed(const char* restrict nick, int secs) {

    time_t mint = time(NULL) - secs;

    int n = 0;

    for (int i=0; i < HIST_LEN; i++)
	if ((lastcmds[i].timestamp >= mint) && (strncmp(nick,lastcmds[i].nick,10) == 0)) n++;

    return n;
}

int handle_commands(irc_session_t* session, const char* restrict origin, const char* restrict nick,const char* restrict channel, const char* msg) {

    char msgparse [(strlen(msg) + 1)];
    memset(msgparse,0,strlen(msg)+1);

    add_history(nick);

    if (get_speed(nick,5) > 5) { printf("I'm so sick of this %s guy!\n",nick); return 0; }

    const char* msgv[20]; msgv[0] = msgparse; int msgcur = 0;

    unsigned int i=0,o=0; bool escaping = false;

    while (i < strlen(msg)) {
	switch (msg[i]) {

	    case '\\':
		msgparse[o] = msg[i+1]; i+=2; o++; break;
	    case '"':
		if ((escaping) || (strchr(msg+i+1,'"')))
		{ escaping = !escaping; i++; }
		else
		{ msgparse[o] = msg[i]; i++; o++; }
		break;
	    case ' ':
		if (!escaping) {
		    while (msg[i+1] == ' ') i++; //skip
		    if (msg[i+1] == 0) {i++; break;}
		    msgparse[o] = 0; i++; o++;
		    if (strlen(msgv[msgcur])) {
			msgcur++; msgv[msgcur] = msgparse+o; }}
		else { msgparse[o] = msg[i]; i++; o++; }	break;
	    default:
		msgparse[o] = msg[i]; i++; o++; break;

	}
    }
    msgcur++;

    /*
       printf("%d parameters:\n",msgcur);
       for (int i=0; i < msgcur; i++)
       printf("%2d = %s\n",i,msgv[i]);
     */

    int r = 1;

    for (unsigned int i=0; i < (sizeof(cmds) / sizeof(*cmds)); i++) {

	if ((strcmp(cmds[i].name, msgv[0]) == 0) && (cmds[i].cb)) {

	    if (cmds[i].no_parse_parameters)
		r = cmds[i].cb(session,(cmds[i].use_full_nickname ? origin : nick),channel,1,&msg); else
		    r = cmds[i].cb(session,(cmds[i].use_full_nickname ? origin : nick),channel,msgcur,msgv);
	}
    }

    return r;
}
