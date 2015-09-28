#include "savefile.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int savedata(char* filename, void* data, struct saveparam* params, size_t paramcnt) {

	FILE* sf = fopen(filename,"w");
	if (!sf) { perror("Opening save file"); return 1; }

	for (size_t i=0; i < paramcnt; i++) {
		struct saveparam* curpar = params+i;

		switch(curpar->type) {
			case ST_STRING:
				fprintf(sf,"%s=%s\n",curpar->name,(char*)(data + curpar->offset));
				break;
			case ST_UINT8:
				fprintf(sf,"%s=%" PRIu8 "\n",curpar->name,*(uint8_t*)(data+curpar->offset));
				break;
			case ST_INT8:
				fprintf(sf,"%s=%" PRId8 "\n",curpar->name,*(int8_t*)(data+curpar->offset));
				break;
			case ST_UINT16:
				fprintf(sf,"%s=%" PRIu16 "\n",curpar->name,*(uint16_t*)(data+curpar->offset));
				break;
			case ST_INT16:
				fprintf(sf,"%s=%" PRId16 "\n",curpar->name,*(int16_t*)(data+curpar->offset));
				break;
			case ST_UINT32:
				fprintf(sf,"%s=%" PRIu32 "\n",curpar->name,*(uint32_t*)(data+curpar->offset));
				break;
			case ST_INT32:
				fprintf(sf,"%s=%" PRId32 "\n",curpar->name,*(int32_t*)(data+curpar->offset));
				break;
			case ST_UINT64:
				fprintf(sf,"%s=%" PRIu64 "\n",curpar->name,*(uint64_t*)(data+curpar->offset));
				break;
			case ST_INT64:
				fprintf(sf,"%s=%" PRId64 "\n",curpar->name,*(int64_t*)(data+curpar->offset));
				break;
		}
	}

	fclose(sf);
	return 0;
}

int loaddata(char* filename, void* data, struct saveparam* params, size_t paramcnt) {
	
	FILE* sf = fopen(filename,"r");
	if (!sf) { perror("Opening load file"); return 1; }
		
	char* gline = NULL;
	size_t gsize = 0;

	while (!feof(sf)) {

		ssize_t r = getline(&gline,&gsize,sf);

		if (r <= 0) continue;

		int parid = -1;

		for (size_t i = 0; i < paramcnt; i++)
			if ((strncmp(gline,(params+i)->name,strlen((params+i)->name)) == 0) && (gline[strlen((params+i)->name)] == '=')) parid = i;

		if (parid != -1) {

		struct saveparam* curpar = params+parid;
		char* parval = (gline + strlen(curpar->name) + 1);
	
		switch(curpar->type) {
			case ST_STRING:

				strncpy(data + (curpar->offset), parval, curpar->varsize);
				break;
			case ST_UINT8:
				*(uint8_t*) (data + curpar->offset) = (uint8_t) atoi(parval);
				break;
			case ST_INT8:
				*(int8_t*) (data + curpar->offset) = (int8_t) atoi(parval);
				break;
			case ST_UINT16:
				*(uint16_t*) (data + curpar->offset) = (uint16_t) atoi(parval);
				break;
			case ST_INT16:
				*(int16_t*) (data + curpar->offset) = (int16_t) atoi(parval);
				break;
			case ST_UINT32:
				*(uint32_t*) (data + curpar->offset) = (uint32_t) atoi(parval);
				break;
			case ST_INT32:
				*(int32_t*) (data + curpar->offset) = (int32_t) atoi(parval);
				break;
			case ST_UINT64:
				*(uint64_t*) (data + curpar->offset) = (uint64_t) atoll(parval);
				break;
			case ST_INT64:
				*(int64_t*) (data + curpar->offset) = (int64_t) atoll(parval);
				break;
		}

		}

	}

	if (gline) free(gline);
	fclose(sf);
	return 0;
}
