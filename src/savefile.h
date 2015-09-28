#ifndef SAVEFILE_H
#define SAVEFILE_H

#include <stddef.h>
#include <stdint.h>

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

struct saveparam {
	const char* name;
	enum savetypes type;
	size_t varsize; //for strings
	ptrdiff_t offset;
};

int savedata(char* filename, void* data, struct saveparam* params, size_t paramcnt);
int loaddata(char* filename, void* data, struct saveparam* params, size_t paramcnt);

#endif
