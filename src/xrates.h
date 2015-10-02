#ifndef XRATES_H
#define XRATES_H

struct exchange_rate {
	char symbol[4];
	float rate;
};

int get_exchange_rates(const char* restrict base, const char* restrict dest, int o_c, struct exchange_rate* o_v);

#endif
