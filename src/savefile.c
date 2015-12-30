#include "savefile.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

char* savedir = NULL;

#define HOMEPATH "/.snowbot"

int findsavedir(void) {

	char* home = getenv("HOME"); //find the home dir
	if (!home) return 1;

	size_t l = strlen(home) + strlen(HOMEPATH) + 1;

	char homedir[l];
	strcpy(homedir,home);
	strcat(homedir,HOMEPATH);

	struct stat buf;
	int r = stat(homedir,&buf);

	if (r == -1) {
		if (errno == ENOENT) {

			//TODO directory doesn't exist. create, then check if it exists again.
			r = mkdir(homedir,S_IRWXU); //rwx------

			if ((r == -1) && (errno)) {printf("Error when creating ~" HOMEPATH); return 1;}

		} else { perror ("Error when checking ~" HOMEPATH); return 1; }
	}

	if (savedir) free(savedir);
	savedir = strdup(homedir);

	printf("Data will be saved in %s\n",savedir);

	return 0;

}

FILE* sfopen(const char* restrict filename, const char* restrict mode) {

	//this function opens a file, prepending savedir (if not NULL) to the path.

	if (!savedir) return fopen(filename,mode);

	size_t l = strlen(savedir) + 1 + strlen(filename) + 1;

	char fullname[l];

	strcpy(fullname,savedir);
	strcat(fullname,"/");
	strcat(fullname,filename);

	return fopen(fullname,mode);
}

int setparam(void* data, struct saveparam* params, size_t paramcnt, const char* restrict name, const char* restrict value) {

	int parid = -1;

	for (size_t i = 0; i < paramcnt; i++)
		if (strcmp(name,(params+i)->name) == 0) parid = i;

	if (parid != -1) {

		struct saveparam* curpar = params+parid;

		switch(curpar->type) {
			case ST_STRING:

				strncpy(data + (curpar->offset), value, curpar->varsize);
				break;
			case ST_UINT8:
				*(uint8_t*) (data + curpar->offset) = (uint8_t) atoi(value);
				break;
			case ST_INT8:
				*(int8_t*) (data + curpar->offset) = (int8_t) atoi(value);
				break;
			case ST_UINT16:
				*(uint16_t*) (data + curpar->offset) = (uint16_t) atoi(value);
				break;
			case ST_INT16:
				*(int16_t*) (data + curpar->offset) = (int16_t) atoi(value);
				break;
			case ST_UINT32:
				*(uint32_t*) (data + curpar->offset) = (uint32_t) atoi(value);
				break;
			case ST_INT32:
				*(int32_t*) (data + curpar->offset) = (int32_t) atoi(value);
				break;
			case ST_UINT64:
				*(uint64_t*) (data + curpar->offset) = (uint64_t) atoll(value);
				break;
			case ST_INT64:
				*(int64_t*) (data + curpar->offset) = (int64_t) atoll(value);
				break;
		}

	} else return 1;

	return 0;
}

int getparam(void* data, struct saveparam* params, size_t paramcnt, const char* restrict name, char* o_value, size_t o_size) {

	int parid = -1;

	for (size_t i = 0; i < paramcnt; i++)
		if (strcmp(name,(params+i)->name) == 0) parid = i;

	if (parid != -1) {

		struct saveparam* curpar = params+parid;

		switch(curpar->type) {
			case ST_STRING:

				snprintf(o_value, o_size, "%s", (char*) data + curpar->offset);
				break;
			case ST_UINT8:
				snprintf(o_value, o_size, "%" PRIu8, *(uint8_t*) (data + curpar->offset));
				break;
			case ST_INT8:
				snprintf(o_value, o_size, "%" PRId8, *(int8_t*) (data + curpar->offset));
				break;
			case ST_UINT16:
				snprintf(o_value, o_size, "%" PRIu16, *(uint16_t*) (data + curpar->offset));
				break;
			case ST_INT16:
				snprintf(o_value, o_size, "%" PRId16, *(int16_t*) (data + curpar->offset));
				break;
			case ST_UINT32:
				snprintf(o_value, o_size, "%" PRIu32, *(uint32_t*) (data + curpar->offset));
				break;
			case ST_INT32:
				snprintf(o_value, o_size, "%" PRId32, *(int32_t*) (data + curpar->offset));
				break;
			case ST_UINT64:
				snprintf(o_value, o_size, "%" PRIu64, *(uint64_t*) (data + curpar->offset));
				break;
			case ST_INT64:
				snprintf(o_value, o_size, "%" PRId64, *(int64_t*) (data + curpar->offset));
				break;
		}

	} else return 1;

	return 0;



	return 0;
}

int savedata(char* filename, void* data, struct saveparam* params, size_t paramcnt) {

	FILE* sf = sfopen(filename,"w");
	if (!sf) { fprintf(stderr,"Opening save %s: %s",filename,strerror(errno)); return 1; }

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

	FILE* sf = sfopen(filename,"r");
	if (!sf) { if (errno != ENOENT) fprintf(stderr,"Opening load %s: %s\n",filename,strerror(errno)); return 1; }

	char* gline = NULL;
	size_t gsize = 0;

	while (!feof(sf)) {

		ssize_t r = getline(&gline,&gsize,sf);

		if (r <= 0) continue;

		if (r > 1) {
			//removing the \n from the line
			if (gline[r-1] == '\n') gline[r-1] = 0;
			if (gline[r-1] == '\r') gline[r-1] = 0;
		}

		char* saveptr;

		char* parname = strtok_r(gline,"=",&saveptr);
		char* parval = gline + strlen(parname) + 1;

		if ((parname) && (parval))
			setparam(data,params,paramcnt,parname,parval);

	}

	if (gline) free(gline);
	fclose(sf);
	return 0;
}
