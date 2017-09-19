// vim: cin:sts=4:sw=4 
#include "irc_commands.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "derail.h"
#include "irc_common.h"
#include "irc_user.h"
#include "irc_watch.h"

#include "irc_commands_weather.h"

#include "convert.h"
#include "pwdhash.h"
#include "short.h"
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

int helpcmd_cb(irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    respond(session,nick,channel,"Read the documentation at https://github.com/usrshare/snowbot/wiki .");
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
	up->modified = true;
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
	respond(session,nick,channel,"Usage: .xr [currency list], .xr $[dollars] [dest currency], .xr [number] [src currency] [dest currency]"); return 0;
    }	

    if ((argc == 2) && (argv[1][0] == '$')) {

	//dollars to currency

	float srcv = atof(argv[1]);

	const char* srccur = "USD";
	const char* destcur = argv[2];


    } else if (argc == 2) {

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
		ircprintf(session,nick,channel,"%s = $%.2f ($1 = %.2f %s)",res[i].symbol,1 / res[i].rate, res[i].rate, res[i].symbol); }
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
	    ircprintf(session,nick,channel,"%.2f %s = %.2f %s",res[0].symbol,count, res[1].symbol, count * res[1].rate / res[0].rate);
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
    up->modified = true;
    return 0;
}

int weather_kelvin_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    respond(session,nick,channel,"Weather responses set to Kelvin.");
    up->wmode = WM_KELVIN;
    up->modified = true;
    return 0;
}

int weather_fahrenheit_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
    struct irc_user_params* up = get_user_params(nick, EB_LOAD);
    respond(session,nick,channel,"Weather responses set to Fahrenheit.");
    up->wmode = WM_FAHRENHEIT;
    up->modified = true;
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

int say_cb (irc_session_t* session, const char* restrict nick, const char* restrict channel, size_t argc, const char** argv) {
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
    {".owm_s",  false, false, weather_search_cb},
    {".cc",     false, false, charcount_cb},
    {".ccg",    false, false, charcountgraph_cb},
    {".xr",     false, false, xr_cmd_cb},
    {".startp", false, false, start_paste_cb},
    {".paste",  false, false, start_paste_cb},
    {".rant",   false, false, start_paste_cb},
    {".sug",    false, false, suggest_derail_cb},
    {".su",     false, false, shorten_url_cb},
    {".cv",	false, false, convert_cb},
    {".roll2",  false, false, roll_cb}, // renamed because InfoAngel@#gi already has .roll
    {".say",	false, false, say_cb},
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

    while (i < strlen(msg) && (msgcur < 19) ) {
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
