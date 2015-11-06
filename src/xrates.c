// vim: cin:sts=4:sw=4 
#include "xrates.h"

#include "http.h"

#include <stddef.h>
#include <stdio.h>
#include <json-c/json.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define BASEURL "https://openexchangerates.org/api/"
#define APPID "d0a651407d51410b9f8ab5b03c8ac0a3"

#define s_ceq(a,b) (strcasecmp((a),(b)) == 0) 
#define s_eq(a,b) (strcmp((a),(b)) == 0) 

time_t last_update = 0;
#define CACHESZ 200
struct exchange_rate cache[CACHESZ];

int update_exchange_rate(int o_c, struct exchange_rate* o_v, const char* symbol, float value) {

    for (int i=0; i < o_c; i++) {

	if (s_ceq(symbol,o_v[i].symbol)) {o_v[i].rate = value; return 0;}
	if (strlen(o_v[i].symbol) == 0) {
	    strncpy(o_v[i].symbol,symbol,4);
	    o_v[i].rate = value;
	    return 0;
	}
    }

    return 1;
}

int fill_json_rates_fields(int o_c, struct exchange_rate* o_v, enum json_type ft, const char* fn, json_object* fv) {
    if (fv == NULL) return 1;

    float rate = json_object_get_double(fv);

    update_exchange_rate(o_c,o_v,fn,rate);
    update_exchange_rate(CACHESZ,cache,fn,rate);

    return 0;
}

int parse_json_xrates_2(json_object* xrates, int o_c, struct exchange_rate* o_v) {
    struct json_object_iterator it_c, it_e;
    it_c = json_object_iter_begin(xrates);
    it_e = json_object_iter_end(xrates);

    const char* fn; json_object* fv; enum json_type ft;

    while (!json_object_iter_equal(&it_c,&it_e)) {

	fn = json_object_iter_peek_name(&it_c);
	fv = json_object_iter_peek_value(&it_c);
	ft = json_object_get_type(fv);

	if (fill_json_rates_fields(o_c,o_v,ft,fn,fv))
	    printf("unparsed xrates field %s\n", fn);

	json_object_iter_next(&it_c);

    }

    return 0;
}

int fill_json_xrates_fields(int o_c, struct exchange_rate* o_v, enum json_type ft, const char* fn, json_object* fv) {
    if (fv == NULL) return 1;

    if (s_eq(fn,"rates")) { parse_json_xrates_2(fv,o_c,o_v); return 0;}

    if (s_eq(fn,"disclaimer")) { return 0; } //ignore
    if (s_eq(fn,"license")) { return 0; } //ignore
    if (s_eq(fn,"base")) { return 0; } //ignore
    if (s_eq(fn,"timestamp")) { last_update = json_object_get_int(fv); return 0; } //ignore

    return 1;
}

int parse_json_xrates(json_object* xrates, int o_c, struct exchange_rate* o_v) {
    struct json_object_iterator it_c, it_e;
    it_c = json_object_iter_begin(xrates);
    it_e = json_object_iter_end(xrates);

    const char* fn; json_object* fv; enum json_type ft;

    while (!json_object_iter_equal(&it_c,&it_e)) {

	fn = json_object_iter_peek_name(&it_c);
	fv = json_object_iter_peek_value(&it_c);
	ft = json_object_get_type(fv);

	if (fill_json_xrates_fields(o_c,o_v,ft,fn,fv))
	    printf("unparsed xrates field %s\n", fn);

	json_object_iter_next(&it_c);

    }

    return 0;
}

int parse_xrates_response(char* response, int o_c, struct exchange_rate* o_v) {

    struct json_tokener* jt = json_tokener_new();
    enum json_tokener_error jerr;

    struct json_object* xobj = json_tokener_parse_ex(jt,response,strlen(response));
    jerr = json_tokener_get_error(jt);
    if (jerr != json_tokener_success) {
	printf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
    }

    if (json_object_get_type(xobj) != json_type_object) {
	printf("Something is wrong. Timeline's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(xobj)));
	return 1; }

    int curw = parse_json_xrates(xobj,o_c,o_v);
    json_tokener_free(jt);
    return 0;
}

int get_exchange_cached(int o_c, struct exchange_rate* o_v) {

    for (int i=0; i < CACHESZ; i++) {
	for (int j=0; j < o_c; j++) {

	    if (s_ceq(cache[i].symbol,o_v[j].symbol)) {o_v[j].rate = cache[i].rate; }

	}
    }
    return 0;
}

int get_exchange_rates(const char* restrict base, const char* restrict dest, int o_c, struct exchange_rate* o_v) {

    if ((time(NULL) - last_update) > 3600) {

	char* xchgurl = BASEURL "latest.json?app_id=" APPID;

	printf("URL: %s\n",xchgurl);
	char* response = make_http_request(xchgurl,NULL,0);

	printf("Response: %s\n", response);
	parse_xrates_response(response,o_c,o_v);
    }
    else {
	get_exchange_cached(o_c,o_v);
    }
    return 0;
}
