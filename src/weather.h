#ifndef WEATHER_H
#define WEATHER_H
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

struct weather_id {
	int id;
	const char* description;
	const char* symbol;
	const char* symbol_u;
	const char* format_on;
	const char* format_off;
};

extern const struct weather_id wids[];

const struct weather_id* getwid(int id);

struct weather_data {
	
	int weather_id[8];
	int weather_c;

	float main_temp;
	float main_pressure;
	int main_humidity;
	float main_temp_min;
	float main_temp_max;

	float wind_speed;
	float wind_deg;
	float clouds_all;
	float rain_3h;
	float snow_3h;

	time_t dt;
	time_t sys_sunrise;
	time_t sys_sunset;
};

struct forecast_data {

	time_t dt;
	float temp_day;
	float temp_min;
	float temp_max;
	float temp_night;
	float temp_eve;
	float temp_morn;

	float pressure;
	int humidity;
	
	int weather_id[8];
	int weather_c;

	float wind_speed;
	float wind_deg;
	float clouds;
	float rain;
	float snow;

};

struct weather_loc {
	float coord_lon;
	float coord_lat;
	
	char postcode[16];
	char sys_country[3];
	int city_id;
	char city_name[64];
	bool accurate;
};

int get_current_weather( struct weather_loc* params, struct weather_data* o_response);

const char* describe_wind_direction(float deg); //returns a string that describes the degree value.

int get_short_status (int idc, int* idv, char* o_str);

int get_weather_forecast(struct weather_loc* loc, struct weather_data* o_wd, int cnt);

int get_long_forecast(struct weather_loc* loc, struct forecast_data* o_fc, int cnt);

int search_weather(struct weather_loc* loc, int count, struct weather_loc* o_loc, struct weather_data* o_wd); 

#endif
