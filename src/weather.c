#include "weather.h"
#include <json-c/json.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "http.h"

#define APPID "0e967742c178e97ee34f6fee12183a89"
#define APIURL "http://api.openweathermap.org/data/2.5/"

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
			printf("unparsed object field %s\n", fn);

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
	if (s_eq(fn,"main")) { if (i>0) strcat(nw->main," / "); strncat(nw->main,json_object_get_string(fv),32); return 0;}
	if (s_eq(fn,"description")) { if (i>0) strcat(nw->description," / "); strncat(nw->description,json_object_get_string(fv),32); return 0;}

	return 1;	
}

int fill_json_wdesc_array(void* out, json_object* fv) {
	if (fv == NULL) return 1;
	struct weather_data* nw = out;

	int descs = json_object_array_length(fv);

	for (int i=0; i < descs; i++) {

		struct json_object* afv = json_object_array_get_idx(fv,i);

		parse_json_object(afv,nw,fill_json_wdesc_fields2);

		nw->weather_c++;
	}
	return 1;	
}



int fill_json_weather_fields(struct weather_data* nw, struct weather_loc* loc, enum json_type ft, const char* fn, json_object* fv) {
	if (fv == NULL) return 1;

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
	if (s_eq(fn,"cod")) { return 0; } //ignore
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

	if (s_eq(fn,"cod")) { return 0; } //ignore
	if (s_eq(fn,"base")) { return 0; } //ignore

	return 1;
}

int fill_json_forecast_list_array(struct weather_data* nw, json_object* fv, int cnt) {
	if (fv == NULL) return 1;

	int descs = json_object_array_length(fv);

	for (int i=0; i < descs; i++) {

		struct json_object* afv = json_object_array_get_idx(fv,i);
		parse_json_object(afv,nw+i,fill_json_wfore_fields);

	}
	return 1;	
}

int fill_json_forecast_fields(struct weather_data* nw, struct weather_loc* loc, enum json_type ft, const char* fn, json_object* fv, int* cnt) {
	if (fv == NULL) return 1;

	int _cnt=0;

	if (s_eq(fn,"cnt")) { _cnt = json_object_get_int64(fv); if (*cnt < _cnt) {return 2;} else {*cnt = _cnt;} return 0;}

	if (s_eq(fn,"city")) { parse_json_object(fv,loc,fill_json_city_fields); return 0;}

	if (s_eq(fn,"list")) { fill_json_forecast_list_array(nw,fv,*cnt); return 0;}

	if (s_eq(fn,"cod")) { return 0; } //ignore
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

		if (fill_json_weather_fields(o_wd,o_loc,ft,fn,fv))
			printf("unparsed weather field %s\n", fn);

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
			printf("unparsed forecast field %s\n", fn);

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

int make_weather_url(struct weather_loc* loc, char* fullurl) {

	if (loc->city_id) {
		snprintf(fullurl,256,APIURL "weather?APPID=" APPID "&id=%d",loc->city_id); return 0; }

	if (loc->zipcode) {
		snprintf(fullurl,256,APIURL "weather?APPID=" APPID "&zip=%d,%s",
				loc->zipcode,strlen(loc->sys_country) ? loc->sys_country : "us"); return 0; } //default country

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

	if (loc->zipcode) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&zip=%d,%s",
				cnt,loc->zipcode,strlen(loc->sys_country) ? loc->sys_country : "us"); return 0; } //default country

	if (strlen(loc->city_name) && strlen(loc->sys_country)) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&q=%s,%s",cnt,loc->city_name,loc->sys_country); return 0; }

	if (strlen(loc->city_name)) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&q=%s",cnt,loc->city_name); return 0; }

	if ((fabs(loc->coord_lon) < 180.0f) && (fabs(loc->coord_lat) < 90.0f)) {
		snprintf(fullurl,256,APIURL "forecast?APPID=" APPID "&cnt=%d&lat=%f&lon=%f",cnt,loc->coord_lat,loc->coord_lon); return 0;}

	return 1;
}

int get_current_weather(struct weather_loc* loc, struct weather_data* o_wd) {

	o_wd->main_humidity = -1;

	char fullurl[256];
	make_weather_url(loc,fullurl);

	printf("URL: %s\n",fullurl);

	char* response = make_http_request(fullurl,NULL);

	printf("Response: %s\n", response);

	parse_current_response(response,o_wd,loc);
	return 0;
}

int get_weather_forecast(struct weather_loc* loc, struct weather_data* o_wd, int cnt) {

	char fullurl[256];
	make_weather_url_f(loc,fullurl,cnt);

	printf("URL: %s\n",fullurl);

	char* response = make_http_request(fullurl,NULL);

	//printf("Response: %s\n", response);

	parse_forecast_response(response,o_wd,loc,cnt);
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
	if (!strstr(dest,src)) return strcat(dest,src);
	return 0;
}

const char* get_status_string(int w_id) {
	switch(w_id) {
		case 210: return "t";
		case 211: return "T";
		case 212: return "\00308T\017";
		case 500: return "r";
		case 501: return "R";
		case 502: return "\00308R\017";
		case 503: return "\00304R\017";
		case 504: return "\00313R\017";
		case 600: return "s";
		case 601: return "S";
		case 602: return "\00308S\017";
		case 900: return "@";
		case 901: return "#";
		case 902: return "?";
		default: return NULL;
	}
}

int get_short_status (struct weather_data* weather, char* o_str){

	o_str[0] = 0;

	for (int i=0; i < (weather->weather_c); i++) {

		int w_id = weather->weather_id[i];
		strcatunique(o_str,get_status_string(w_id));
	}

	return 0;
}

