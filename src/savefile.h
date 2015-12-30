#ifndef SAVEFILE_H
#define SAVEFILE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum savetypes {
	ST_STRING,
	ST_UINT8,
	ST_INT8,
	ST_UINT16,
	ST_INT16,
	ST_UINT32,
	ST_INT32,
	ST_UINT64,
	ST_INT64,
};

enum savevis {
	SV_HIDDEN,
	SV_READONLY,
	SV_VISIBLE,
};

struct saveparam {
	const char* name;
	enum savetypes type;
	size_t varsize; //for strings
	ptrdiff_t offset;
	enum savevis visibility;
};

int findsavedir(void); //initialize
FILE* sfopen(const char* restrict filename, const char* restrict mode);

int setparam(void* data, struct saveparam* params, size_t paramcnt, const char* restrict name, const char* restrict value);
int getparam(void* data, struct saveparam* params, size_t paramcnt, const char* restrict name, char* o_value, size_t o_size);

int savedata(char* filename, void* data, struct saveparam* params, size_t paramcnt);
int loaddata(char* filename, void* data, struct saveparam* params, size_t paramcnt);

#endif
