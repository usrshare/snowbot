#ifndef WEATHER_H
#define WEATHER_H
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

struct weather_data {
	
	char main[256+21];
	char description[512+21];

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

struct weather_loc {
	float coord_lon;
	float coord_lat;
	
	int zipcode;
	char sys_country[3];
	int city_id;
	char city_name[64];
	bool accurate;
};

int get_current_weather( struct weather_loc* params, struct weather_data* o_response);

const char* describe_wind_direction(float deg); //returns a string that describes the degree value.

int get_short_status (struct weather_data* weather, char* o_str);

int get_weather_forecast(struct weather_loc* loc, struct weather_data* o_wd, int cnt);

#endif
