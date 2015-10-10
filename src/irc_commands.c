// vim: cin:sts=4:sw=4 
#include "irc_commands.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "irc_common.h"
#include "irc_user.h"
#include "irc_watch.h"

#include "weather.h"
#include "xrates.h"

#include <math.h>
#include <time.h>

typedef int (*irc_command_cb) (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);

struct irc_user_commands {

    const char* name;
    bool no_parse_parameters; //if true, argc is set to 1 and entire string is stored in argv[0].
    irc_command_cb cb;
};

int handle_weather_current(irc_session_t* session, const char* restrict nick, const char* restrict channel, struct weather_loc* wloc, struct weather_data* wdata) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

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

    if (up->wmode == WM_CELSIUS)
	snprintf (weathertmp,255,"%+.1f°C",wdata->main_temp - 273.15); else
	    snprintf (weathertmp,255,"%+.1f°F",32.0 + ((wdata->main_temp - 273.15) * 1.8f));

    weathermsg = strrecat(weathermsg,weathertmp);

    if (fabsf(wdata->main_temp_max - wdata->main_temp_min > 0.5f)) {

	if (up->wmode == WM_CELSIUS)
	    snprintf(weathertmp,255," (%+.1f° .. %+.1f°)",wdata->main_temp_min - 273.15,wdata->main_temp_max - 273.15); else
		snprintf(weathertmp,255," (%+.1f° .. %+.1f°)",32.0 + (wdata->main_temp_min - 273.15) * 1.8,32.0 + (wdata->main_temp_max - 273.15) * 1.8);
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

	if (wspd >= 30) fmtst = "\026"; else
	    if (wspd >= 25) fmtst = "\00304"; else
		if (wspd >= 17) fmtst = "\00308"; else
		    if (wspd >= 10) fmtst = "\00309";

	if (wspd >= 10) fmted = "\017";
	snprintf(weathertmp,255,", wind: %s%.1f%s m/s (%s%.1f%s mph)", fmtst,wdata->wind_speed,fmted, fmtst,wdata->wind_speed * 2.23694f,fmted);
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    if (wdata->wind_deg >= 0.0) {
	snprintf(weathertmp,255," %s", describe_wind_direction(wdata->wind_deg));
	weathermsg = strrecat(weathermsg,weathertmp);
    }

    respond(session,nick,channel,weathermsg);
    return 0;
}
int handle_long_forecast(irc_session_t* session, const char* restrict nick, const char* restrict channel, struct weather_loc* wloc, struct forecast_data* wdata, int cnt) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    char* weathermsg = malloc(128);

    char weathertmp[256];

    snprintf (weathermsg,128,"d#:");

    char weathertmp2[16];
    memset(weathertmp,0,sizeof weathertmp);

    struct tm weathertime;
    memset(&weathertime,0,sizeof weathertime);

    for (int i=0; i<cnt; i++) {

	gmtime_r(&((wdata+i)->dt), &weathertime);
	snprintf(weathertmp2,16,"  %2d  |",weathertime.tm_mday);
	strcat(weathertmp,weathertmp2);
    }

    weathermsg = strrecat(weathermsg,weathertmp);

    respond(session,nick,channel,weathermsg);

    switch(up->wmode) {
	case WM_CELSIUS: {

			     snprintf (weathermsg,128,"°C:");

			     memset(weathertmp,0,sizeof weathertmp);
			     for (int i=0; i<cnt; i++) {
				 snprintf(weathertmp2,16,"\00308%-3d\00312%3d\017|",(int)round((wdata+i)->temp_day - 273.15f),(int)round((wdata+i)->temp_night - 273.15f));
				 strcat(weathertmp,weathertmp2);
			     }
			     weathermsg = strrecat(weathermsg,weathertmp);
			     respond(session,nick,channel,weathermsg);

			     break; }
	case WM_FAHRENHEIT: {

				snprintf (weathermsg,128,"°F:");

				memset(weathertmp,0,sizeof weathertmp);
				for (int i=0; i<cnt; i++) {
				    snprintf(weathertmp2,16,"\00308%-3d\00312%3d\017|",(int)round(( ((wdata+i)->temp_day - 273.15f)*1.8f)+32.0f),(int)round(( ((wdata+i)->temp_night - 273.15f)*1.8f)+32.0f));
				    strcat(weathertmp,weathertmp2);
				}
				weathermsg = strrecat(weathermsg,weathertmp);
				respond(session,nick,channel,weathermsg);

				break; }
    }

    snprintf (weathermsg,128,"wd:");

    for (int i=0; i<cnt; i++) {

	int wspd = (int) round((wdata+i)->wind_speed);

	char* fmtst = ""; char* fmted = "";

	if (wspd >= 30) fmtst = "\026"; else
	    if (wspd >= 25) fmtst = "\00304"; else
		if (wspd >= 17) fmtst = "\00308"; else
		    if (wspd >= 10) fmtst = "\00309";

	if (wspd >= 10) fmted = "\017";
	snprintf(weathertmp,255,"%s %4.1f %s|", fmtst,(wdata+i)->wind_speed,fmted);
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
		snprintf(weathertmp2,16,"  %s%s%s  |",wid->format_on,wid->symbol,wid->format_off); else
		    snprintf(weathertmp2,16,"  %s  |",wid->symbol);
	    strcat(weathertmp,weathertmp2);
	}
	weathermsg = strrecat(weathermsg,weathertmp);
	respond(session,nick,channel,weathermsg);
    }

    return 0;
}
int handle_weather_forecast(irc_session_t* session, const char* restrict nick, const char* restrict channel, struct weather_loc* wloc, struct weather_data* wdata, int cnt) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    char* weathermsg = malloc(128);

    char weathertmp[256];

    snprintf (weathermsg,128,"hr:");

    char weathertmp2[16];
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

    switch(up->wmode) {
	case WM_CELSIUS: {

			     snprintf (weathermsg,128,"°C:");

			     memset(weathertmp,0,sizeof weathertmp);
			     for (int i=0; i<cnt; i++) {
				 snprintf(weathertmp2,16,"%3d",(int)round((wdata+i)->main_temp - 273.15f));
				 strcat(weathertmp,weathertmp2);
			     }
			     weathermsg = strrecat(weathermsg,weathertmp);
			     respond(session,nick,channel,weathermsg);

			     break; }
	case WM_FAHRENHEIT: {

				snprintf (weathermsg,128,"°F:");

				memset(weathertmp,0,sizeof weathertmp);
				for (int i=0; i<cnt; i++) {
				    snprintf(weathertmp2,16,"%3d",(int)round(( ((wdata+i)->main_temp - 273.15f)*1.8f)+32.0f));
				    strcat(weathertmp,weathertmp2);
				}
				weathermsg = strrecat(weathermsg,weathertmp);
				respond(session,nick,channel,weathermsg);

				break; }
    }

    snprintf(weathermsg,128,"wd:");

    memset(weathertmp,0,sizeof weathertmp);
    for (int i=0; i<cnt; i++) {

	int wspd = (int) round((wdata+i)->wind_speed);

	char* fmtst = ""; char* fmted = "";

	if (wspd >= 30) fmtst = "\026"; else
	    if (wspd >= 25) fmtst = "\00304"; else
		if (wspd >= 17) fmtst = "\00308"; else
		    if (wspd >= 10) fmtst = "\00309";

	if (wspd >= 10) fmted = "\017";

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

int load_location (const char* restrict params, struct irc_user_params* up, struct weather_loc* wloc) {

    bool parse_ok = false;

    wloc->city_id = 0; //overriding defaults
    int r = sscanf(params," #%d",&wloc->city_id); parse_ok = (r == 1);

    if (!parse_ok) { r = sscanf(params," %f, %f", &wloc->coord_lon, &wloc->coord_lat); parse_ok = (r == 2); }
    if (!parse_ok) { r = sscanf(params," %d, %2[A-Za-z]", &wloc->zipcode, wloc->sys_country); parse_ok = (r >= 1); }
    if (!parse_ok) { r = sscanf(params," %64[^#,\n\r], %2[A-Za-z]", wloc->city_name, wloc->sys_country); escape_location(wloc->city_name); parse_ok = (r >= 1); }

    if (parse_ok) return 0; else return 1;

}

int helpcmd_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    respond(session,nick,channel,"Read the documentation at https://github.com/usrshare/snowbot/wiki .");
    return 0;
}

int weather_channel(irc_session_t* session, const char* restrict channel, struct weather_loc* wloc, struct weather_data* wdata) {

    strcpy(wloc->sys_country,"EF");
    strcpy(wloc->city_name,channel);
    wloc->city_id = -1;
    wloc->zipcode = 99999;

    time_t hour_ago = time(NULL)-3600;

    unsigned int lasthour = watch_getlength(NULL,channel,hour_ago,0);

    unsigned int snowmsgs = watch_getlength("snow_",channel,hour_ago,0) + watch_getlength("snow^",channel,hour_ago,0);

    float chantemp = 273.15f - 10.0f + ((float)lasthour / 16.0f);
    //assume 30 messages on average. temperatures will range from -5C to whatever.

    wdata->main_temp = chantemp;
    wdata->main_temp_min = wdata->main_temp_max = chantemp;

    float snowpct = lasthour ? (snowmsgs / lasthour): 0.0f;

    wdata->main_pressure = -1.0f;
    wdata->main_humidity = -1.0f;

    wdata->weather_c = 1;

    if (snowpct >= .40f) wdata->weather_id[0] = 602; else
	if (snowpct >= .20f) wdata->weather_id[0] = 601; else
	    if (snowpct >= .10f) wdata->weather_id[0] = 600; else
		wdata->weather_id[0] = 800;

    unsigned int windmsgs = watch_getlength("wind",channel,hour_ago,0);

    float windspd = (float)windmsgs / 50.0f;
    wdata->wind_speed = windspd;

    return 0;
}	

int weather_is_channel(const char* restrict arg) {

    if ((arg[0] == '#') && (strtol(arg+1,NULL,10) == 0)) return 1; 

    return 0;
}

int weather_current_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    struct weather_loc wloc;
    memset(&wloc,0,sizeof wloc);
    struct weather_data wdata;
    memset(&wdata,0,sizeof wdata);

    if ((strlen(argv[0]) == 4) && (!up->cityid)) {
	respond(session,nick,channel,"Usage: .owm #<OWM city ID>, .owm <zip code>[,<2char country code>], .owm <city name>[,<2char country code>], .owm <longitude>,<latitude>"); return 0; }

    else {
	int r = load_location(argv[0]+4,up,&wloc);

	if (r) r = !weather_is_channel(argv[0]+5);
	if (r) r = wloc.city_id = up->cityid;
	if (r) {respond(session,nick,channel,"Can't understand the parameters. Sorry."); return 0;}
    }

    if (!weather_is_channel(argv[0]+5)) 
	get_current_weather( &wloc, &wdata); else
	    weather_channel(session, argv[0]+5,&wloc,&wdata);

    handle_weather_current(session, nick, channel, &wloc, &wdata);
    return 0;
}	

int weather_forecast_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    struct weather_loc wloc;
    memset(&wloc,0,sizeof wloc);

    if ((strlen(argv[0]) == 4) && (!up->cityid)) {
	respond(session,nick,channel,"Usage: .owf <# of 3-hour intervals> #<OWM city ID>, .owm <zip code>[,<2char country code>], .owm <city name>[,<2char country code>], .owm <longitude>,<latitude>"); return 0; }

    const char* _oswparams = argv[0]+4;

    int cnt = 0;

    char* oswparams = NULL;

    cnt = strtol(_oswparams,&oswparams,10);

    if (cnt < 0) cnt = 1; if (cnt == 0) cnt = 16; if (cnt>40) cnt=40;

    struct weather_data wdata[cnt];
    memset(&wdata,0,sizeof (struct weather_data) *cnt);

    if (!oswparams) {respond(session,nick,channel,"Can't see the number of intervals. Sorry."); return 0;} 

    int r = load_location(oswparams,up,&wloc);

    if (r) respond(session,nick,channel,"Can't understand the parameters. Sorry."); else {

	get_weather_forecast( &wloc, wdata, cnt);
	handle_weather_forecast( session, nick, channel, &wloc, wdata, cnt);

    }
    return 0;
}	

int weather_longforecast_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    struct weather_loc wloc;
    memset(&wloc,0,sizeof wloc);

    if ((strlen(argv[0]) == 4) && (!up->cityid)) {
	respond(session,nick,channel,"Usage: .owl <# of days> #<OWM city ID>, .owm <zip code>[,<2char country code>], .owm <city name>[,<2char country code>], .owm <longitude>,<latitude>"); return 0; }

    const char* _oswparams = argv[0]+4;

    int cnt = 0;

    char* oswparams = NULL;

    cnt = strtol(_oswparams,&oswparams,10);

    if (cnt < 0) cnt = 1; if (cnt == 0) cnt = 7; if (cnt>16) cnt=16;

    struct forecast_data wdata[cnt];
    memset(&wdata,0,sizeof wdata);

    if (!oswparams) {respond(session,nick,channel,"Can't see the number of intervals. Sorry."); return 0;} 

    int r = load_location(oswparams,up,&wloc);

    if (r) respond(session,nick,channel,"Can't understand the parameters. Sorry."); else {

	get_long_forecast( &wloc, wdata, cnt);
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

	    int r = getparam(up, irc_save_params, paramcnt, irc_save_params[i].name, val, 128);
	    if (r == 0) ircprintf (session,nick,channel,"%s = %s",irc_save_params[i].name,val); else ircprintf (session,nick,channel,"%s = ???",irc_save_params[i].name);


	}
    }
    return 0;
}


int xr_cmd_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

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

	int r = get_exchange_rates(NULL,NULL,ci,res);

	for (int i=0; i < ci; i++)
	    ircprintf(session,nick,channel,"%s = $%.3f ($1 = %.3f %s)",res[i].symbol,1 / res[i].rate, res[i].rate, res[i].symbol);
    }

    if (argc == 4) {

	float count = atof(argv[1]);

	struct exchange_rate res[2];

	strncpy(res[0].symbol,argv[2],4);
	strncpy(res[1].symbol,argv[3],4);

	int r = get_exchange_rates(NULL,NULL,2,res);

	ircprintf(session,nick,channel,"%.3f %s = %.3f %s",res[0].symbol,count, res[1].symbol, count * res[1].rate / res[0].rate);
    }

    return 0;

}

int weather_celsius_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    respond(session,nick,channel,"Weather responses set to Celsius.");
    up->wmode = WM_CELSIUS;
    return 0;
}
int weather_fahrenheit_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    respond(session,nick,channel,"Weather responses set to Fahrenheit.");
    up->wmode = WM_FAHRENHEIT;
    return 0;
}

char* risingblocks[] = {" ","▁","▂","▃","▄","▅","▆","▇","█","▒"};

int charcountgraph_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {

    if (argc != 4) { ircprintf(session,nick,channel,"Usage: %s <nickname> <seconds> <intervals>",argv[0]); return 0;}

    int seconds = atoi(argv[2]);
    int intervals = atoi(argv[3]);

    int bytes[intervals];
    memset(bytes,0,sizeof(int) * intervals);

    count_by_period(argv[1],time(NULL) - (seconds * intervals),seconds,intervals,bytes);

    char* output = strdup("|");

    int maxbytes = 1;

    for (int i=0; i < intervals; i++)
	if (bytes[i] > maxbytes) maxbytes = bytes[i];

    for (int i=0; i < intervals; i++)
	if (bytes[i] == 0) output = strrecat(output," "); else
	{
	    int fill = (bytes[i] * 8 / maxbytes) + 1;
	    output = strrecat(output,risingblocks[fill]);
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

    unsigned int r = watch_getlength(argv[1],NULL,tmin,0);

    if (seconds)
	ircprintf(session,nick,channel,"I've seen %s post %d bytes in the last %d seconds.",argv[1],r,seconds);
    else
	ircprintf(session,nick,channel,"I remember %s posting %d bytes in the last %d messages.",argv[1],r,watch_countmsg());

    return 0;
}

struct irc_user_commands cmds[] = {
    {".help", false, helpcmd_cb},
    {".load", false, load_cmd_cb},
    {".save", false, save_cmd_cb},
    {".set", false, set_cmd_cb},
    {".utc", false, utc_cmd_cb},
    {".w_c", false, weather_celsius_cb},
    {".w_f", false, weather_fahrenheit_cb},
    {".owm", true, weather_current_cb},
    {".owf", true, weather_forecast_cb},
    {".owl", true, weather_longforecast_cb},
    {".cc", false, charcount_cb},
    {".ccg", false, charcountgraph_cb},
    {".xr", false, xr_cmd_cb},
    {".about", false, NULL},
};

int handle_commands(irc_session_t* session, const char* restrict nick,const char* restrict channel, const char* msg) {

    char* msgparse = malloc(strlen(msg) + 1);
    memset(msgparse,0,strlen(msg)+1);

    const char* msgv[20]; msgv[0] = msgparse; int msgcur = 0;

    unsigned int i=0,o=0; bool escaping = false;

    while (i < strlen(msg)) {
	switch (msg[i]) {

	    case '\\':
		msgparse[o] = msg[i+1]; i+=2; o++; break;
	    case '"':
		escaping = !escaping; i++; break;
	    case ' ':
		if (!escaping) { msgparse[o] = 0; i++; o++; if (strlen(msgv[msgcur])) {msgcur++; msgv[msgcur] = msgparse+o; }}
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
		r = cmds[i].cb(session,nick,channel,1,&msg); else
		    r = cmds[i].cb(session,nick,channel,msgcur,msgv);
	}
    }

    free(msgparse);
    return r;
}
