// vim: cin:sts=4:sw=4 

#include <stdint.h>

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

enum ht_returncodes {
    HT_SUCCESS = 0,
    HT_ERROR = 1,
    HT_ALREADYEXISTS = 2,
    HT_NOTFOUND = 3,
};

struct ht_item {
	char* key;
	void* value;
	struct ht_item* next;
};

struct hashtable {
	unsigned int n;
	int unique; //all items must have unique keys
	struct ht_item** items;
};

unsigned int hash(unsigned int n,const char* restrict key);

struct hashtable* ht_create2(unsigned int n, int unique);
struct hashtable* ht_create(unsigned int n);

int ht_insert(struct hashtable* ht,  const char* restrict key, void* value);
void* ht_search(struct hashtable* ht,const char* restrict key);
int ht_delete(struct hashtable* ht,  const char* restrict key);

#endif
