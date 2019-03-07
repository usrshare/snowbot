#include "weather.h"
#include <json-c/json.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "http.h"

#define APPID "0e967742c178e97ee34f6fee12183a89"
#define APIURL "http://api.openweathermap.org/data/2.5/"

const struct weather_id wids[] = {           // symbols   format b format d format m, format off
	{0,   "???",                            "??","??",NULL    ,NULL    ,NULL    ,NULL},
	{201, "thunderstorm with rain",		"Tr","Tr","\00307","\00307","\002\037","\017"},
	{202, "thunderstorm with heavy rain",	"TR","TR","\00307","\00307","\002\037","\017"},
	{210, "light thunderstorm",		" t","░t","\00307","\00307","\002\037","\017"},
	{211, "thunderstorm",			" T","▒T","\00307","\00307","\002\037","\017"},
	{212, "heavy thunderstorm",		"TT","▓T","\00307","\00307","\002\037","\017"},
	{221, "ragged thunderstorm",		"T!","T!","\00307","\00307","\002\037","\017"},
	{230, "thunderstorm with light drizzle","Td","Td","\00307","\00307","\002\037","\017"},
	{231, "thunderstorm with drizzle",	"TD","TD","\00307","\00307","\002\037","\017"},
	{232, "thunderstorm with heavy drizzle","TD","TD","\00307","\00307","\002\037","\017"},
	{300, "light intensity drizzle",	" d","░d","\00311","\00310","\037","\017"},
	{301, "drizzle",			"dd","▒d","\00311","\00310","\037","\017"},
	{302, "heavy intensity drizzle",	" D","▓D","\00311","\00310","\037","\017"},
	{310, "light intensity drizzle rain",	"dr","dr","\00311","\00310","\037","\017"},
	{311, "drizzle rain",			"DR","DR","\00311","\00310","\037","\017"},
	{312, "heavy intensity drizzle rain",	"DR","DR","\00311","\00310","\037","\017"},
	{313, "shower rain and drizzle",	"dr","dr","\00310","\00310","\037","\017"},
	{314, "heavy shower rain and drizzle",	"DR","DR","\00310","\00310","\037","\017"},
	{321, "shower drizzle",			" d"," d","\00310","\00310","\037","\017"},
	{500, "light rain",			" r",".r","\00311","\00310","\037","\017"},
	{501, "moderate rain",			"rr","░r","\00311","\00310","\037","\017"},
	{502, "heavy intensity rain",		"Rr","▒r","\00311","\00310","\037","\017"},
	{503, "very heavy rain",		"RR","▓R","\00311","\00310","\037","\017"},
	{504, "extreme rain",			"R!","█R","\00311","\00310","\037","\017"},
	{511, "freezing rain",			"R-","R-","\00312","\00312","\037","\017"},
	{520, "light intensity shower rain",	" r","░r","\00310","\00310","\037","\017"},
	{521, "shower rain",			" R","▒R","\00310","\00310","\037","\017"},
	{522, "heavy intensity shower rain",	"RR","▓R","\00310","\00310","\037","\017"},
	{531, "ragged shower rain",		"R!","R!","\00310","\00310","\037","\017"},
	{600, "light snow",			" s","░s","\00312","\00312","\037","\017"},
	{601, "snow",				"ss","▒s","\00312","\00312","\037","\017"},
	{602, "heavy snow",			" S","▓S","\00312","\00312","\037","\017"},
	{611, "sleet",				"sl","sl",NULL    ,NULL    ,"\037"  ,NULL},
	{612, "shower sleet",			"sl","sl","\00314","\00314","\037","\017"},
	{615, "light rain and snow",		"rs","rs","\00312","\00312","\037","\017"},
	{616, "rain and snow",			"RS","RS","\00312","\00312","\037","\017"},
	{620, "light shower snow",		" s"," s","\00302","\00302","\037","\017"},
	{621, "shower snow",			"ss","ss","\00302","\00302","\037","\017"},
	{622, "heavy shower snow",		" S"," S","\00302","\00302","\037","\017"},
	{701, "mist",				"mi","mi",NULL    ,NULL    ,NULL    ,NULL},
	{711, "smoke",				"sm","sm",NULL    ,NULL    ,NULL    ,NULL},
	{721, "haze",				"hz","hz",NULL    ,NULL    ,NULL    ,NULL},
	{731, "sand, dust whirls",		"s@","s@",NULL    ,NULL    ,NULL    ,NULL},
	{741, "fog",				"fg","fg",NULL    ,NULL    ,NULL    ,NULL},
	{751, "sand",				"sd","sd",NULL    ,NULL    ,NULL    ,NULL},
	{761, "dust",				"dt","dt",NULL    ,NULL    ,NULL    ,NULL},
	{762, "volcanic ash",			"va","va",NULL    ,NULL    ,NULL    ,NULL},
	{771, "squalls",			"sq","sq",NULL    ,NULL    ,NULL    ,NULL},
	{781, "tornado",			" ?"," ?","\026"  ,"\026"  ,"\026"  ,"\017"}, //inv
	{800, "clear sky",		  	" *"," *","\00308","\00307","\002"  ,"\017"}, //y
	{801, "few clouds",			" c"," c",NULL    ,NULL    ,NULL    ,NULL},
	{802, "scattered clouds",		"cc","cc",NULL    ,NULL    ,NULL    ,NULL},
	{803, "broken clouds",			" C"," C",NULL    ,NULL    ,NULL    ,NULL},
	{804, "overcast clouds",		"CC","CC",NULL    ,NULL    ,NULL    ,NULL},
	{900, "tornado",			" ?"," ?","\026"  ,"\026"  ,"\026"  ,"\017"},
	{901, "tropical storm",			"t#","t#","\026"  ,"\026"  ,"\026"  ,"\017"},
	{902, "hurricane",			"H@","H@","\026"  ,"\026"  ,"\026"  ,"\017"},
	{903, "cold",			  	" -"," -",NULL    ,NULL    ,NULL    ,NULL},
	{904, "hot",				" +"," +",NULL    ,NULL    ,NULL    ,NULL},
	{905, "windy",				"~~","~~",NULL    ,NULL    ,NULL    ,NULL},
	{906, "hail",				"HL","HL","\00312","\00312","\002","\017"},
	{951, "calm",				"cm","cm",NULL    ,NULL    ,NULL    ,NULL},
	{952, "light breeze",			"~1","~1",NULL    ,NULL    ,NULL    ,NULL},
	{953, "gentle breeze",			"~2","~2",NULL    ,NULL    ,NULL    ,NULL},
	{954, "moderate breeze",		"~3","~3",NULL    ,NULL    ,NULL    ,NULL},
	{955, "fresh breeze",			"~4","~4",NULL    ,NULL    ,NULL    ,NULL},
	{956, "strong breeze",			"~5","~5",NULL    ,NULL    ,NULL    ,NULL},
	{957, "high wind, near gale",		"~6","~6",NULL    ,NULL    ,NULL    ,NULL},
	{958, "gale",				"~7","~7","\002"  ,"\002"  ,"\002"  ,"\017"}, //B
	{959, "severe gale",			"~8","~8","\002"  ,"\002"  ,"\002"  ,"\017"}, //B
	{960, "storm",				" #"," #","\026"  ,"\026"  ,"\026"  ,"\017"}, //I
	{961, "violent storm",			"##","##","\026"  ,"\026"  ,"\026"  ,"\017"}, //I
	{962, "hurricane",			"H@","H@","\026"  ,"\026"  ,"\026"  ,"\017"}, //I
};

const struct weather_id* getwid(int id) {

	for (unsigned int i=0; i < (sizeof(wids) / sizeof(*wids)); i++)
		if (wids[i].id == id) return &wids[i];

	return &wids[0];
}
float strtof2(char* string) {
	float res = strtof(string,NULL);
	if (errno) return nan("lol");
	return res;
}

#define s_eq(a,b) (strcmp((a),(b)) == 0) 

typedef int (*json_parse_cb) (void* out, enum json_type ft, const char* fn, json_object* fv);

int parse_json_object(json_object* obj, void* out, json_parse_cb callback) {
	struct json_object_iterator it_c, it_e;
	it_c = json_object_iter_begin(obj);
	it_e = json_object_iter_end(obj);

	const char* fn; json_object* fv; enum json_type ft;

	while (!json_object_iter_equal(&it_c,&it_e)) {

		fn = json_object_iter_peek_name(&it_c);
		fv = json_object_iter_peek_value(&it_c);
		ft = json_object_get_type(fv);

		if (callback(out,ft,fn,fv)) 
		{ /* printf("unparsed object field %s\n", fn); */ }

		json_object_iter_next(&it_c);

	}

	return 0;
}

int fill_json_coord_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;

	struct weather_loc* loc = out;

	errno = 0;	
	if (s_eq(fn,"lat")) { loc->coord_lat = json_object_get_double(fv); if (errno) loc->coord_lat = 91.0; return 0;}
	if (s_eq(fn,"lon")) { loc->coord_lon = json_object_get_double(fv); if (errno) loc->coord_lon = 181.0; return 0;}

	return 1;	
}
int fill_json_main_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;

	if (s_eq(fn,"temp_kf")) { return 0;}
	if (s_eq(fn,"temp")) { nw->main_temp = json_object_get_double(fv); if (errno) nw->main_temp = -1.0; return 0;}
	if (s_eq(fn,"pressure")) { nw->main_pressure = json_object_get_double(fv); if (errno) nw->main_pressure = -1.0; return 0;}
	if (s_eq(fn,"humidity")) { nw->main_humidity = json_object_get_int(fv); return 0;}
	if (s_eq(fn,"temp_min")) { nw->main_temp_min = json_object_get_double(fv); if (errno) nw->main_temp_min = -1.0; return 0;}
	if (s_eq(fn,"temp_max")) { nw->main_temp_max = json_object_get_double(fv); if (errno) nw->main_temp_max = -1.0; return 0;}

	return 1;	

}
int fill_json_wind_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;

	if (s_eq(fn,"speed")) { nw->wind_speed = json_object_get_double(fv); if (errno) nw->wind_speed = -1.0; return 0;}
	if (s_eq(fn,"deg")) { nw->wind_deg = json_object_get_double(fv); if (errno) nw->wind_deg = -1.0; return 0;}

	return 1;	

}
int fill_json_rain_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;
	if (s_eq(fn,"3h")) { nw->rain_3h = json_object_get_double(fv); if (errno) nw->rain_3h = -1.0; return 0;}

	return 1;	
}
int fill_json_snow_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;
	if (s_eq(fn,"3h")) { nw->snow_3h = json_object_get_double(fv); if (errno) nw->snow_3h = -1.0; return 0;}

	return 1;	
}
int fill_json_clouds_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;
	if (s_eq(fn,"all")) { nw->clouds_all = json_object_get_double(fv); if (errno) nw->clouds_all = -1.0; return 0;}
	return 1;	
}
int fill_json_sys_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_loc* loc = out;

	if (s_eq(fn,"country")) { strncpy(loc->sys_country,json_object_get_string(fv),2); return 0;}

	return 1;	
}

int fill_json_city_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_loc* loc = out;

	if (s_eq(fn,"country")) { strncpy(loc->sys_country,json_object_get_string(fv),2); return 0;}
	if (s_eq(fn,"id")) { loc->city_id = json_object_get_int(fv); return 0;}
	if (s_eq(fn,"name")) { strncpy(loc->city_name,json_object_get_string(fv),32); return 0;}

	return 1;	
}

int fill_json_wdesc_fields2(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;

	int i = nw->weather_c;

	if (s_eq(fn,"id")) { nw->weather_id[i] = json_object_get_int(fv); return 0;}
	if (s_eq(fn,"main")) { return 0;}
	if (s_eq(fn,"description")) { return 0;}
	if (s_eq(fn,"icon")) { return 0;}

	return 1;	
}

int fill_json_fdesc_fields2(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct forecast_data* nf = out;

	int i = nf->weather_c;

	if (s_eq(fn,"id")) { nf->weather_id[i] = json_object_get_int(fv); return 0;}
	if (s_eq(fn,"main")) { return 0;}
	if (s_eq(fn,"description")) { return 0;}

	return 1;	
}

int fill_json_temp_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct forecast_data* nf = out;

	if (s_eq(fn,"day")) { nf->temp_day = json_object_get_double(fv); if (errno) nf->temp_day = -1.0; return 0;}
	if (s_eq(fn,"night")) { nf->temp_night = json_object_get_double(fv); if (errno) nf->temp_night = -1.0; return 0;}
	if (s_eq(fn,"morn")) { nf->temp_morn = json_object_get_double(fv); if (errno) nf->temp_morn = -1.0; return 0;}
	if (s_eq(fn,"eve")) { nf->temp_eve = json_object_get_double(fv); if (errno) nf->temp_eve = -1.0; return 0;}
	if (s_eq(fn,"min")) { nf->temp_min = json_object_get_double(fv); if (errno) nf->temp_min = -1.0; return 0;}
	if (s_eq(fn,"max")) { nf->temp_max = json_object_get_double(fv); if (errno) nf->temp_max = -1.0; return 0;}

	return 1;	

}

int fill_json_wdesc_array(void* out, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;

	int descs = json_object_array_length(fv);

	nw->weather_c = 0;

	for (int i=0; i < descs; i++) {

		struct json_object* afv = json_object_array_get_idx(fv,i);

		parse_json_object(afv,nw,fill_json_wdesc_fields2);

		nw->weather_c++;
	}
	return 1;	
}

int fill_json_fdesc_array(void* out, json_object* fv) {
	if (fv == NULL) return 1;
	struct forecast_data* nf = out;

	int descs = json_object_array_length(fv);
	
	nf->weather_c = 0;

	for (int i=0; i < descs; i++) {

		struct json_object* afv = json_object_array_get_idx(fv,i);

		parse_json_object(afv,nf,fill_json_fdesc_fields2);

		nf->weather_c++;
	}
	return 1;	
}

struct weather_ptrs {
	struct weather_data* nw;
	struct weather_loc* loc;
};

int fill_json_weather_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;

	struct weather_data* nw = ((struct weather_ptrs*)out) -> nw;
	struct weather_loc* loc = ((struct weather_ptrs*)out) -> loc;

	if (s_eq(fn,"dt")) { nw->dt = json_object_get_int64(fv); return 0;}

	if (s_eq(fn,"coord")) { parse_json_object(fv,loc,fill_json_coord_fields); return 0;}
	if (s_eq(fn,"main")) { parse_json_object(fv,nw,fill_json_main_fields); return 0;}
	if (s_eq(fn,"rain")) { parse_json_object(fv,nw,fill_json_rain_fields); return 0;}
	if (s_eq(fn,"clouds")) { parse_json_object(fv,nw,fill_json_clouds_fields); return 0;}
	if (s_eq(fn,"snow")) { parse_json_object(fv,nw,fill_json_snow_fields); return 0;}
	if (s_eq(fn,"wind")) { parse_json_object(fv,nw,fill_json_wind_fields); return 0;}
	if (s_eq(fn,"sys")) { parse_json_object(fv,loc,fill_json_sys_fields); return 0;}
	if (s_eq(fn,"weather")) { fill_json_wdesc_array(nw,fv); return 0;}

	if (s_eq(fn,"id")) { loc->city_id = json_object_get_int(fv); return 0;}
	if (s_eq(fn,"name")) { strncpy(loc->city_name,json_object_get_string(fv),32); return 0;}
	if (s_eq(fn,"cod")) { nw->cod = json_object_get_int(fv); return 0; } 
	if (s_eq(fn,"base")) { return 0; } //ignore

	return 1;
}

int fill_json_wfore_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;

	if (s_eq(fn,"dt")) { nw->dt = json_object_get_int64(fv); return 0;}

	if (s_eq(fn,"main")) { parse_json_object(fv,nw,fill_json_main_fields); return 0;}
	if (s_eq(fn,"rain")) { parse_json_object(fv,nw,fill_json_rain_fields); return 0;}
	if (s_eq(fn,"clouds")) { parse_json_object(fv,nw,fill_json_clouds_fields); return 0;}
	if (s_eq(fn,"snow")) { parse_json_object(fv,nw,fill_json_snow_fields); return 0;}
	if (s_eq(fn,"wind")) { parse_json_object(fv,nw,fill_json_wind_fields); return 0;}
	if (s_eq(fn,"weather")) { fill_json_wdesc_array(nw,fv); return 0;}
	if (s_eq(fn,"sys")) { return 0;}

	if (s_eq(fn,"cod")) { nw->cod = json_object_get_int(fv); return 0; } 
	if (s_eq(fn,"base")) { return 0; } //ignore

	return 1;
}

int fill_json_wlfore_fields(void* out, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;
	struct forecast_data* nf = out;

	if (s_eq(fn,"dt")) { nf->dt = json_object_get_int64(fv); return 0;}

	if (s_eq(fn,"temp")) { parse_json_object(fv,nf,fill_json_temp_fields); return 0;}
	if (s_eq(fn,"weather")) { fill_json_fdesc_array(nf,fv); return 0;}

	if (s_eq(fn,"speed")) { nf->wind_speed = json_object_get_double(fv); if (errno) nf->wind_speed = -1.0; return 0;}
	if (s_eq(fn,"deg")) { nf->wind_deg = json_object_get_int(fv); return 0;}
	if (s_eq(fn,"clouds")) { nf->clouds = json_object_get_int(fv); return 0;}
	if (s_eq(fn,"rain")) { nf->rain = json_object_get_double(fv); if (errno) nf->rain = -1.0; return 0;}
	if (s_eq(fn,"snow")) { nf->snow = json_object_get_double(fv); if (errno) nf->snow = -1.0; return 0;}
	if (s_eq(fn,"humidity")) { nf->humidity = json_object_get_int(fv); return 0;}

	if (s_eq(fn,"cod")) { nf->cod = json_object_get_int(fv); return 0; } 
	if (s_eq(fn,"base")) { return 0; } //ignore

	return 1;
}

int fill_json_forecast_list_array(struct weather_data* nw, json_object* fv, int cnt) {
	if (fv == NULL) return 1;

	int descs = json_object_array_length(fv);

	for (int i=0; (i<8) && (i < descs); i++) {

		struct json_object* afv = json_object_array_get_idx(fv,i);
		parse_json_object(afv,nw+i,fill_json_wfore_fields);

	}
	return 1;	
}

int fill_json_longforecast_list_array(struct forecast_data* nf, json_object* fv, int cnt) {
	if (fv == NULL) return 1;

	int descs = json_object_array_length(fv);

	for (int i=0; (i<8) && (i < descs); i++) {

		struct json_object* afv = json_object_array_get_idx(fv,i);
		parse_json_object(afv,nf+i,fill_json_wlfore_fields);

	}
	return 1;	
}

int fill_json_search_list_array(struct weather_loc* loc, struct weather_data* nw, json_object* fv, int cnt) {
	if (fv == NULL) return 1;

	int descs = json_object_array_length(fv);

	int i=0;
	for (i=0; (i<cnt) && (i < descs); i++) {

		struct json_object* afv = json_object_array_get_idx(fv,i);

		struct weather_ptrs cur = {nw+i, loc+i};

		parse_json_object(afv,&cur,fill_json_weather_fields);

	}
	return i;	
}

int fill_json_forecast_fields(struct weather_data* nw, struct weather_loc* loc, enum json_type ft, const char* fn, json_object* fv, int* cnt) {
	if (fv == NULL) return 1;

	int _cnt=0;

	if (s_eq(fn,"cnt")) { _cnt = json_object_get_int64(fv); if (*cnt < _cnt) {return 2;} else {*cnt = _cnt;} return 0;}

	if (s_eq(fn,"city")) { parse_json_object(fv,loc,fill_json_city_fields); return 0;}

	if (s_eq(fn,"list")) { fill_json_forecast_list_array(nw,fv,*cnt); return 0;}

	if (s_eq(fn,"cod")) { 
		
		int cod = json_object_get_int(fv); 
		for (int i=0; i < *cnt; i++) nw[i].cod = cod;
		
		return 0; }

	if (s_eq(fn,"message")) { return 0; } //ignore

	return 1;
}

int fill_json_longforecast_fields(struct forecast_data* nf, struct weather_loc* loc, enum json_type ft, const char* fn, json_object* fv, int* cnt) {
	if (fv == NULL) return 1;

	int _cnt=0;

	if (s_eq(fn,"cnt")) { _cnt = json_object_get_int64(fv); if (*cnt < _cnt) {return 2;} else {*cnt = _cnt;} return 0;}

	if (s_eq(fn,"city")) { parse_json_object(fv,loc,fill_json_city_fields); return 0;}

	if (s_eq(fn,"list")) { fill_json_longforecast_list_array(nf,fv,*cnt); return 0;}

	if (s_eq(fn,"cod")) { 
		
		int cod = json_object_get_int(fv); 
		for (int i=0; i < *cnt; i++) nf[i].cod = cod;
		
		return 0; }
	if (s_eq(fn,"message")) { return 0; } //ignore

	return 1;
}

int parse_json_weather(json_object* weather, struct weather_data* o_wd, struct weather_loc* o_loc) {
	struct json_object_iterator it_c, it_e;
	it_c = json_object_iter_begin(weather);
	it_e = json_object_iter_end(weather);

	const char* fn; json_object* fv; enum json_type ft;

	while (!json_object_iter_equal(&it_c,&it_e)) {

		fn = json_object_iter_peek_name(&it_c);
		fv = json_object_iter_peek_value(&it_c);
		ft = json_object_get_type(fv);

		struct weather_ptrs cur = {o_wd, o_loc};

		if (fill_json_weather_fields(&cur,ft,fn,fv))
		{ /* printf("unparsed weather field %s\n", fn); */ }

		json_object_iter_next(&it_c);

	}

	return 0;
}

int parse_json_forecast(json_object* weather, struct weather_data* o_wd, struct weather_loc* o_loc, int cnt) {
	struct json_object_iterator it_c, it_e;
	it_c = json_object_iter_begin(weather);
	it_e = json_object_iter_end(weather);

	const char* fn; json_object* fv; enum json_type ft;

	while (!json_object_iter_equal(&it_c,&it_e)) {

		fn = json_object_iter_peek_name(&it_c);
		fv = json_object_iter_peek_value(&it_c);
		ft = json_object_get_type(fv);

		if (fill_json_forecast_fields(o_wd,o_loc,ft,fn,fv,&cnt))
		{ /* printf("unparsed forecast field %s\n", fn); */ }

		json_object_iter_next(&it_c);

	}

	return 0;
}

int parse_json_longforecast(json_object* weather, struct forecast_data* o_fd, struct weather_loc* o_loc, int cnt) {
	struct json_object_iterator it_c, it_e;
	it_c = json_object_iter_begin(weather);
	it_e = json_object_iter_end(weather);

	const char* fn; json_object* fv; enum json_type ft;

	while (!json_object_iter_equal(&it_c,&it_e)) {

		fn = json_object_iter_peek_name(&it_c);
		fv = json_object_iter_peek_value(&it_c);
		ft = json_object_get_type(fv);

		if (fill_json_longforecast_fields(o_fd,o_loc,ft,fn,fv,&cnt))
		{ /* printf("unparsed longforecast field %s\n", fn); */ }

		json_object_iter_next(&it_c);

	}

	return 0;
}

int parse_forecast_response(char* response, struct weather_data* o_wd, struct weather_loc* o_loc, int cnt) {

	struct json_tokener* jt = json_tokener_new();
	enum json_tokener_error jerr;

	struct json_object* wobj = json_tokener_parse_ex(jt,response,strlen(response));
	jerr = json_tokener_get_error(jt);
	if (jerr != json_tokener_success) {
		printf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
	}

	if (json_object_get_type(wobj) != json_type_object) {
		printf("Something is wrong. Timeline's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(wobj)));
		return 1; }

	int curw = parse_json_forecast(wobj,o_wd,o_loc,cnt);
	json_tokener_free(jt);
	return 0;
}

int parse_json_search(json_object* search, int count, struct weather_data* o_wd, struct weather_loc* o_loc) {
	struct json_object_iterator it_c, it_e;
	it_c = json_object_iter_begin(search);
	it_e = json_object_iter_end(search);

	const char* fn; json_object* fv; enum json_type ft;

	int ret_count = 0;

	while (!json_object_iter_equal(&it_c,&it_e)) {
		
		fn = json_object_iter_peek_name(&it_c);
		fv = json_object_iter_peek_value(&it_c);
		ft = json_object_get_type(fv);
		
		if (s_eq(fn,"count")) count = json_object_get_int(fv);
		
		if (s_eq(fn,"list")) { ret_count = fill_json_search_list_array(o_loc,o_wd,fv,count);}
		
		json_object_iter_next(&it_c);

	}

	return ret_count;
}

int parse_longforecast_response(char* response, struct forecast_data* o_fd, struct weather_loc* o_loc, int cnt) {

	struct json_tokener* jt = json_tokener_new();
	enum json_tokener_error jerr;

	struct json_object* wobj = json_tokener_parse_ex(jt,response,strlen(response));
	jerr = json_tokener_get_error(jt);
	if (jerr != json_tokener_success) {
		printf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
	}

	if (json_object_get_type(wobj) != json_type_object) {
		printf("Something is wrong. Timeline's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(wobj)));
		return 1; }

	int curw = parse_json_longforecast(wobj,o_fd,o_loc,cnt);
	json_tokener_free(jt);
	return 0;
}

int parse_current_response(char* response, struct weather_data* o_wd, struct weather_loc* o_loc) {

	struct json_tokener* jt = json_tokener_new();
	enum json_tokener_error jerr;

	struct json_object* wobj = json_tokener_parse_ex(jt,response,strlen(response));
	jerr = json_tokener_get_error(jt);
	if (jerr != json_tokener_success) {
		printf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
	}

	if (json_object_get_type(wobj) != json_type_object) {
		printf("Something is wrong. Timeline's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(wobj)));
		return 1; }

	int curw = parse_json_weather(wobj,o_wd,o_loc);
	json_tokener_free(jt);
	return 0;
}

int parse_search_response(char* response, int count, struct weather_data* o_wd, struct weather_loc* o_loc) {

	struct json_tokener* jt = json_tokener_new();
	enum json_tokener_error jerr;

	struct json_object* wobj = json_tokener_parse_ex(jt,response,strlen(response));
	jerr = json_tokener_get_error(jt);
	if (jerr != json_tokener_success) {
		printf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
	}

	if (json_object_get_type(wobj) != json_type_object) {
		printf("Something is wrong. Timeline's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(wobj)));
		return -1; }

	int cnt = parse_json_search(wobj,count,o_wd,o_loc);
	json_tokener_free(jt);
	return cnt;
}

int make_search_url(struct weather_loc* loc, int count, char* fullurl) {

	if (loc->city_id) {
		snprintf(fullurl,256,APIURL "find?APPID=" APPID "&id=%d&cnt=%d",loc->city_id,count); return 0; }

	if (strlen(loc->postcode)) {
		snprintf(fullurl,256,APIURL "find?APPID=" APPID "&zip=%s,%s&cnt=%d",
				loc->postcode,strlen(loc->sys_country) ? loc->sys_country : "us",count); return 0; } //default country

	if (strlen(loc->city_name) && strlen(loc->sys_country)) {
		snprintf(fullurl,256,APIURL "find?APPID=" APPID "&q=%s,%s&cnt=%d",loc->city_name,loc->sys_country,count); return 0; }

	if (strlen(loc->city_name)) {
		snprintf(fullurl,256,APIURL "find?APPID=" APPID "&q=%s&cnt=%d",loc->city_name,count); return 0; }

	if ((fabs(loc->coord_lon) < 180.0f) && (fabs(loc->coord_lat) < 90.0f)) {
		snprintf(fullurl,256,APIURL "find?APPID=" APPID "&lat=%f&lon=%f&cnt=%d",loc->coord_lat,loc->coord_lon,count); return 0;}

	return 1;
}
int make_weather_url(struct weather_loc* loc, char* fullurl) {

	if (loc->city_id) {
		snprintf(fullurl,256,APIURL "weather?APPID=" APPID "&id=%d",loc->city_id); return 0; }

	if (strlen(loc->postcode)) {
		snprintf(fullurl,256,APIURL "weather?APPID=" APPID "&zip=%s,%s",
				loc->postcode,strlen(loc->sys_country) ? loc->sys_country : "us"); return 0; } //default country

	if (strlen(loc->city_name) && strlen(loc->sys_country)) {
		snprintf(fullurl,256,APIURL "weather?APPID=" APPID "&q=%s,%s",loc->city_name,loc->sys_country); return 0; }

	if (strlen(loc->city_name)) {
		snprintf(fullurl,256,APIURL "weather?APPID=" APPID "&q=%s",loc->city_name); return 0; }

	if ((fabs(loc->coord_lon) < 180.0f) && (fabs(loc->coord_lat) < 90.0f)) {
		snprintf(fullurl,256,APIURL "weather?APPID=" APPID "&lat=%f&lon=%f",loc->coord_lat,loc->coord_lon); return 0;}

	return 1;
}

int make_weather_url_f(struct weather_loc* loc, char* fullurl,int cnt) {

	if (loc->city_id) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&id=%d",cnt,loc->city_id); return 0; }

	if (strlen(loc->postcode)) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&zip=%s,%s",
				cnt,loc->postcode,strlen(loc->sys_country) ? loc->sys_country : "us"); return 0; } //default country

	if (strlen(loc->city_name) && strlen(loc->sys_country)) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&q=%s,%s",cnt,loc->city_name,loc->sys_country); return 0; }

	if (strlen(loc->city_name)) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&q=%s",cnt,loc->city_name); return 0; }

	if ((fabs(loc->coord_lon) < 180.0f) && (fabs(loc->coord_lat) < 90.0f)) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&lat=%f&lon=%f",cnt,loc->coord_lat,loc->coord_lon); return 0;}

	return 1;
}

int make_weather_url_lf(struct weather_loc* loc, char* fullurl,int cnt) {

	if (loc->city_id) {
		snprintf(fullurl,256,APIURL "forecast/daily?APPID=" APPID "&cnt=%d&id=%d",cnt,loc->city_id); return 0; }

	if (strlen(loc->postcode)) {
		snprintf(fullurl,256,APIURL "forecast/daily?APPID=" APPID "&cnt=%d&zip=%s,%s",
				cnt,loc->postcode,strlen(loc->sys_country) ? loc->sys_country : "us"); return 0; } //default country

	if (strlen(loc->city_name) && strlen(loc->sys_country)) {
		snprintf(fullurl,256,APIURL "forecast/daily?APPID=" APPID "&cnt=%d&q=%s,%s",cnt,loc->city_name,loc->sys_country); return 0; }

	if (strlen(loc->city_name)) {
		snprintf(fullurl,256,APIURL "forecast/daily?APPID=" APPID "&cnt=%d&q=%s",cnt,loc->city_name); return 0; }

	if ((fabs(loc->coord_lon) < 180.0f) && (fabs(loc->coord_lat) < 90.0f)) {
		snprintf(fullurl,256,APIURL "forecast/daily?APPID=" APPID "&cnt=%d&lat=%f&lon=%f",cnt,loc->coord_lat,loc->coord_lon); return 0;}

	return 1;
}

int search_weather(struct weather_loc* loc, int count, struct weather_loc* o_loc, struct weather_data* o_wd) {

	o_wd->main_humidity = -1;

	char fullurl[256];
	make_search_url(loc,count,fullurl);

	printf("URL: %s\n",fullurl);

	char* response = make_http_request(fullurl,NULL,0);

	printf("Response: %s\n", response);

	int cnt = parse_search_response(response,count,o_wd,o_loc);
	return cnt;
}
int get_current_weather(struct weather_loc* loc, struct weather_data* o_wd) {

	o_wd->main_humidity = -1;

	char fullurl[256];
	make_weather_url(loc,fullurl);

	printf("URL: %s\n",fullurl);

	char* response = make_http_request(fullurl,NULL,0);

	printf("Response: %s\n", response);

	parse_current_response(response,o_wd,loc);
	return 0;
}

int get_weather_forecast(struct weather_loc* loc, struct weather_data* o_wd, int cnt) {

	char fullurl[256];
	make_weather_url_f(loc,fullurl,cnt);

	printf("URL: %s\n",fullurl);

	char* response = make_http_request(fullurl,NULL,0);

	//printf("Response: %s\n", response);

	parse_forecast_response(response,o_wd,loc,cnt);
	return cnt; //returns the count of responses received.
}

int get_long_forecast(struct weather_loc* loc, struct forecast_data* o_fd, int cnt) {

	char fullurl[256];
	make_weather_url_lf(loc,fullurl,cnt);

	printf("URL: %s\n",fullurl);

	char* response = make_http_request(fullurl,NULL,0);

	//printf("Response: %s\n", response);

	parse_longforecast_response(response,o_fd,loc,cnt);
	return cnt; //returns the count of responses received.
}

const char* describe_wind_direction(float deg) {

	if (deg < 0.0) return "WTF";
	if (deg < 11.25f) return "N";
	if (deg < 33.75f) return "NNE";
	if (deg < 56.25f) return "NE";
	if (deg < 78.75f) return "ENE";
	if (deg < 101.25f) return "E";
	if (deg < 123.75f) return "ESE";
	if (deg < 146.25f) return "SE";
	if (deg < 168.75f) return "SSE";
	if (deg < 191.25f) return "S";
	if (deg < 213.75f) return "SSW";
	if (deg < 236.25f) return "SW";
	if (deg < 258.75f) return "WSW";
	if (deg < 281.25f) return "W";
	if (deg < 303.75f) return "WNW";
	if (deg < 326.75f) return "NW";
	if (deg < 348.75f) return "NNW";
	return "N";
}

int strcatunique(char* dest, const char* restrict src) {

	if (!src) return 0;
	if (!strstr(dest,src)) { strcat(dest,src); return 0;}
	return 0;
}

int get_short_status (int idc, int* idv, char* o_str){

	o_str[0] = 0;

	for (int i=0; i < idc; i++) {

		int w_id = idv[i];
		strcatunique(o_str,getwid(w_id)->symbol);
	}

	return 0;
}

