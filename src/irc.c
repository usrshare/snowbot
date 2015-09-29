// vim: cin:sts=4:sw=4 
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <search.h>

#include <math.h>
#include <stddef.h>
#include <time.h>

#include "irc_user.h"

#include "paste.h"
#include "weather.h"

irc_callbacks_t callbacks;

#define ACTIVITY_CIRCBUF_SIZE 20

#define NOTIFY_USERS_MAX 16

#define WALL_BEGINS 200
#define WALL_ENDS 100

typedef int (*irc_command_cb) (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv);

struct irc_bot_params{

    char* irc_channel;
    char* irc_nickname;
    bool channel_joined;

    char msg_current_nickname[10]; //nickname of user who posted last message
    unsigned int cons_count; //number of consecutive messages from that user.
    unsigned int cons_length; //length of consecutive messages from that user.

};


char* strrecat(char* orig, const char* append) {
    char* new = realloc(orig,strlen(orig) + strlen(append) + 1);
    new = strcat(new,append);
    return new;
}


struct irc_user_commands {

    const char* name;
    irc_command_cb cb;
};

struct irc_user_commands cmds[] = {
    {".help",NULL},
    {".about",NULL},
};


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

int add_paste_line(irc_session_t* session, struct irc_user_params* up, const char* restrict string) {

    size_t cursize = (up->paste_text) ? strlen(up->paste_text) : 0;
    up->paste_size = cursize + 2 + strlen(string);
    up->paste_text = realloc(up->paste_text,up->paste_size);

    if (cursize) {
	strcpy(up->paste_text + cursize,"\n");
	strcat(up->paste_text,string); }
    else strcpy(up->paste_text,string);

    return 0;
}

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
	weathermsg = strrecat(weathermsg,getwid(wdata->weather_id[i])->description);
	weathermsg = strrecat(weathermsg,", ");
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
	snprintf(weathertmp,255,", wind: %.1f m/s (%.1f mph)", wdata->wind_speed, wdata->wind_speed * 2.23694f);
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
	snprintf(weathertmp2,16,"  %2d  .",weathertime.tm_mday);
	strcat(weathertmp,weathertmp2);
    }

    weathermsg = strrecat(weathermsg,weathertmp);

    respond(session,nick,channel,weathermsg);

    switch(up->wmode) {
	case WM_CELSIUS: {

			     snprintf (weathermsg,128,"°C:");

			     memset(weathertmp,0,sizeof weathertmp);
			     for (int i=0; i<cnt; i++) {
				 snprintf(weathertmp2,16,"%3d",(int)round((wdata+i)->temp_day - 273.15f));
				 strcat(weathertmp,weathertmp2);
			     }
			     weathermsg = strrecat(weathermsg,weathertmp);
			     respond(session,nick,channel,weathermsg);

			     break; }
	case WM_FAHRENHEIT: {

				snprintf (weathermsg,128,"°F:");

				memset(weathertmp,0,sizeof weathertmp);
				for (int i=0; i<cnt; i++) {
				    snprintf(weathertmp2,16,"%3d",(int)round(( ((wdata+i)->temp_day - 273.15f)*1.8f)+32.0f));
				    strcat(weathertmp,weathertmp2);
				}
				weathermsg = strrecat(weathermsg,weathertmp);
				respond(session,nick,channel,weathermsg);

				break; }
    }




    snprintf (weathermsg,128,"sp:");

    memset(weathertmp,0,sizeof weathertmp);
    char weathertmp3[16];
    for (int i=0; i<cnt; i++) {

	//get_short_status(wdata+i,weathertmp3);
	snprintf(weathertmp2,16,"%3s",weathertmp3);
	strcat(weathertmp,weathertmp2);
    }
    weathermsg = strrecat(weathermsg,weathertmp);
    respond(session,nick,channel,weathermsg);
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




    snprintf (weathermsg,128,"sp:");

    memset(weathertmp,0,sizeof weathertmp);
    char weathertmp3[16];
    for (int i=0; i<cnt; i++) {

	get_short_status(wdata+i,weathertmp3);
	snprintf(weathertmp2,16,"%3s",weathertmp3);
	strcat(weathertmp,weathertmp2);
    }
    weathermsg = strrecat(weathermsg,weathertmp);
    respond(session,nick,channel,weathermsg);
    return 0;
}

int escape_location (char* city_name) {

    for (int i=0; i < strlen(city_name); i++)
	if (city_name[i] == ' ') city_name[i] = '+';

    return 0;
}

int load_location (const char* restrict params, struct irc_user_params* up, struct weather_loc* wloc) {

    bool parse_ok = false;
    
    wloc->city_id = 0; //overriding defaults
    int r = sscanf(params," #%d",&wloc->city_id); parse_ok = (r == 1);

    if (!parse_ok) { r = sscanf(params," %f, %f", &wloc->coord_lon, &wloc->coord_lat); parse_ok = (r == 2); }
    if (!parse_ok) { r = sscanf(params," %d, %2[A-Za-z]", &wloc->zipcode, wloc->sys_country); parse_ok = (r >= 1); }
    if (!parse_ok) { r = sscanf(params," %64[^,\n\r], %2[A-Za-z]", wloc->city_name, wloc->sys_country); escape_location(wloc->city_name); parse_ok = (r >= 1); }

    if (!parse_ok) { wloc->city_id = up->cityid; parse_ok = (up->cityid); }
    if (parse_ok) return 0; else return 1;

}

int handle_msg(irc_session_t* session, const char* restrict nick, const char* restrict channel, const char* restrict msg) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    if (strlen(msg) == 0) return 1;

    struct irc_user_params* up = get_user_params(nick, EB_LOAD);

    char* msgparse = malloc(strlen(msg) + 1);
    memset(msgparse,0,strlen(msg)+1);

    const char* msgv[20]; msgv[0] = msgparse; int msgcur = 0;

    int i=0,o=0; bool escaping = false;
    
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

    for (int i=0; i < (sizeof(cmds) / sizeof(*cmds)); i++) {

	if ((strcmp(cmds[i].name, msgv[0]) == 0) && (cmds[i].cb))
	    cmds[i].cb(session,nick,channel,msgcur,msgv);
    }	

    free(msgparse);




    if (msg[0] == '.') {
	if (strcmp(msg,".help") == 0) {
	    respond(session,nick,channel,"Read the documentation at https://github.com/usrshare/snowbot/wiki .");
	    return 0;
	}
	if (strcmp(msg, ".startp") == 0) {

	    respond(session,nick,channel,"Paste mode enabled. All input not starting with a dot will be interpreted as strings to paste. End your document by sending a single dot. To insert a string starting with a dot, prepend another dot.");

	    up->mode = BM_PASTE;
	    up->paste_text = NULL;
	    up->paste_size = 0;

	}

	if (strncmp(msg,".set",strlen(".set")) == 0) {

	    char* msgcopy = strdup(msg);

	    char* saveptr = NULL;

	    char* cmd = strtok_r(msgcopy," ",&saveptr);
	    char* param = strtok_r(NULL," ",&saveptr);
	    char* pval = strtok_r(NULL," ",&saveptr);

	    if (pval) {

		int r = setparam(up, irc_save_params, paramcnt, param, pval);
		if (r == 0) respond (session,nick,channel,"Parameter set successfully."); else respond (session,nick,channel,"Can't set parameter.");

		//set parameter to value
	    } else if (param) {

		char val[128];

		int r = getparam(up, irc_save_params, paramcnt, param, val, 128);
		if (r == 0) ircprintf (session,nick,channel,"%s = %s",param,val); else respond (session,nick,channel,"Can't get parameter.");

		//get value of parameter and respond
	    } else {

		char val[128];

		for (unsigned int i = 0; i < paramcnt; i++) {

		    int r = getparam(up, irc_save_params, paramcnt, irc_save_params[i].name, val, 128);
		    if (r == 0) ircprintf (session,nick,channel,"%s = %s",irc_save_params[i].name,val); else ircprintf (session,nick,channel,"%s = ???",irc_save_params[i].name);


		}
	    }

	    free(msgcopy);
	}
	if (strcmp(msg,".w_c") == 0) {
	    respond(session,nick,channel,"Weather responses set to Celsius.");
	    up->wmode = WM_CELSIUS;
	    return 0;
	}
	if (strcmp(msg,".save") == 0) {
	    int r = save_user_params(nick,up);
	    if (r == 0) respond(session,nick,channel,"Your user parameters have been saved.");
	    else respond(session,nick,channel,"Error while saving your user parameters.");
	    return 0;
	}
	if (strcmp(msg,".load") == 0) {
	    int r = load_user_params(nick,up);
	    if (r == 0) respond(session,nick,channel,"Your user parameters have been loaded.");
	    else respond(session,nick,channel,"Error while loading your user parameters.");
	    return 0;
	}

	if (strcmp(msg,".w_f") == 0) {
	    respond(session,nick,channel,"Weather responses set to Fahrenheit.");
	    up->wmode = WM_FAHRENHEIT;
	    return 0;
	}

	if (strncmp(msg, ".owm",4) == 0) {

	    struct weather_loc wloc;
	    memset(&wloc,0,sizeof wloc);
	    struct weather_data wdata;
	    memset(&wdata,0,sizeof wdata);

	    if ((strlen(msg) == 4) && (!up->cityid)) {
		respond(session,nick,channel,"Usage: .owm #<OWM city ID>, .owm <zip code>[,<2char country code>], .owm <city name>[,<2char country code>], .owm <longitude>,<latitude>"); return 0; }

	    else {
		const char* oswparams = msg+4;

		int r = load_location(oswparams,up,&wloc);
		if (r) {respond(session,nick,channel,"Can't understand the parameters. Sorry."); return 0;} }

	    get_current_weather( &wloc, &wdata);
	    handle_weather_current(session, nick, channel, &wloc, &wdata);

	}

	if (strncmp(msg, ".owf",4) == 0) {

	    struct weather_loc wloc;
	    memset(&wloc,0,sizeof wloc);

	    if ((strlen(msg) == 4) && (!up->cityid)) {
		respond(session,nick,channel,"Usage: .owf <# of 3-hour intervals> #<OWM city ID>, .owm <zip code>[,<2char country code>], .owm <city name>[,<2char country code>], .owm <longitude>,<latitude>"); return 0; }

	    const char* _oswparams = msg+4;

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
	}

    }


    switch(up->mode) {
	case BM_NONE:

	    switch(msg[0]) {
		case 'k': {
			      if (strncmp(msg, "kill all humans",15) == 0) respond(session,nick,channel,"kill all humans!!");
			      break;}
		case 'h': {
			      if (strcmp(msg, "hello") == 0) respond(session,nick,channel,"hi");
			      break;}
		case 'p': {
			      if (strcmp(msg, "ping") == 0) respond(session,nick,channel,"pong");
			      break;}
		default: {
			     break; }
	    }
	    break;
	case BM_PASTE:
	    switch(msg[0]) {
		case '.': {
			      if (strlen(msg) == 1) {
				  char* resurl = upload_to_pastebin(nick,"test upload",up->paste_text);	
				  free(up->paste_text);
				  up->paste_text = NULL;
				  up->paste_size = 0;
				  respond(session,nick,channel,resurl);
				  up->mode = BM_NONE;
				  free(resurl);
			      } else {
				  if (msg[1] == '.') add_paste_line(session,up,msg+1);
			      }

			      break; }
		default: {
			     add_paste_line(session,up,msg);
			     break; }
	    }
	    break;
    }

    return 0;
}

void count_msg(irc_session_t* session, const char* restrict nick, const char* restrict msg) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    if (strcmp(nick, ibp->msg_current_nickname) == 0) {

	ibp->cons_count++;
	ibp->cons_length += strlen(msg); 
    } else {
	ibp->cons_count = 0;
	ibp->cons_length = strlen(msg);
    }

}

void quit_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    printf("User %s quit the server.\n",nick);

    struct irc_user_params* up = get_user_params(nick, EB_NULL);

    if (up) {
	save_user_params(nick,up);
	del_user_params(nick,up);
    }

}

void channel_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    struct irc_bot_params* ibp = irc_get_ctx(session);

    bool handle_this_message = false;
    const char* handle_ptr = params[1];

    if (strncmp(params[1],".",1) == 0) handle_this_message = true;

    if (strncmp(params[1],ibp->irc_nickname,strlen(ibp->irc_nickname)) == 0) {
	handle_this_message = true;
	handle_ptr = params[1] + strlen(ibp->irc_nickname);
	while ((strlen(handle_ptr) > 0) && (strchr(",: ",handle_ptr[0])) ) handle_ptr++; //skip delimiters.
    }

    if (handle_this_message) {
	handle_msg(session, nick, params[0], handle_ptr);
    } else {

	count_msg(session,nick,params[1]);
    }
}

void privmsg_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    char nick[10];

    irc_target_get_nick(origin,nick,10);
    //printf("[   !!PRIVATE!!] [%10s]:%s\n",nick,params[1]);

    handle_msg(session,nick,NULL,params[1]); // handle private message


}

void join_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    struct irc_bot_params* ibp = irc_get_ctx(session);

    char nick[10];
    irc_target_get_nick(origin,nick,10);

    if (strcmp(nick,ibp->irc_nickname) == 0) {
	printf("Joined channel %s.\n",params[0]);
	ibp->channel_joined = 1;
    }


}

void connect_cb(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count) {

    printf("Successfully connected to the network.\n");
    struct irc_bot_params* ibp = irc_get_ctx(session);

    int r = irc_cmd_join( session, ibp->irc_channel, 0);
    if (r != 0) printf("Can't join %s: %s\n",ibp->irc_channel,irc_strerror(irc_errno(session)));

    r = hcreate(512);
    if (r == 0) perror("Can't create user hashtable");
}

void numeric_cb(irc_session_t* session, unsigned int event, const char* origin, const char** params, unsigned int count) {

    switch(event) {

	case 1: //welcome
	case 2: //your host
	case 3: //created
	case 4: //information

	case 5: //bounce?

	case 251: // user, service and server count.
	case 252: // operator count
	case 253: // unknown connection count
	case 254: // channel count
	case 255: // client and server count

	case 265: // + local user count 
	case 266: // + global user count
	case 250: // + connection count

	case 332: // topic
	case 333: // + who set the topic and when
	case 353: // names list
	case 366: // names list end

	case 375: // MOTD start
	case 372: // MOTD
	case 376: // MOTD end
	    break;
	default:
	    printf("Received event %u with %u parameters:\n",event,count);
	    for (unsigned int i = 0; i < count; i++)
		printf("Param %3u: %s\n",i,params[i]);
	    break;
	    break;
    }

}

void* create_bot(char* irc_channel) {
    memset( &callbacks, 0, sizeof(callbacks));

    struct irc_bot_params* new_params = malloc(sizeof(struct irc_bot_params));
    if (!new_params) {
	perror("Unable to allocate memory for bot structure");
	return NULL;
    }

    memset(new_params,0,sizeof(struct irc_bot_params));

    new_params->irc_channel = irc_channel;

    callbacks.event_connect = connect_cb;
    callbacks.event_numeric = numeric_cb;
    callbacks.event_join = join_cb;
    callbacks.event_quit = quit_cb;
    callbacks.event_channel = channel_cb;
    callbacks.event_privmsg = privmsg_cb;

    irc_session_t* session = irc_create_session (&callbacks);
    irc_set_ctx(session,new_params);

    return session;
}

int connect_bot(void* session, char* address, int port, bool use_ssl, char* nickname) {

    struct irc_bot_params* ibp = irc_get_ctx(session);
    ibp->irc_nickname = nickname;

    char address_copy[128];

    if (use_ssl) {
	strcpy(address_copy,"#");
	strncat(address_copy,address,126);
    } else strncpy(address_copy,address,127);

    int r = irc_connect(session,address_copy,port,NULL,nickname,NULL,"snowbot");
    if (r != 0) fprintf(stderr,"IRC connection error: %s.\n",irc_strerror(irc_errno(session)));
    return r;
}

int loop_bot(void* session) {

    irc_run(session);
    return 0;
}


int disconnect_bot(void* session) {

    free(irc_get_ctx(session));
    irc_disconnect(session);
    hdestroy();
    return 0;
}
