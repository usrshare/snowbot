// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "hashtable.h"

struct hashtable* ht_create2(unsigned int n, int unique) {
    if (n == 0) return NULL;
    struct hashtable* new = malloc(sizeof(struct hashtable));

    new->n = n;
    new->unique = unique;
    new->items = malloc(sizeof(struct hashtable_items*) * n);
    for (unsigned int i=0; i<n; i++)
	new->items[i] = NULL;

    return new;
}

struct hashtable* ht_create(unsigned int n) {
    return ht_create2(n,1);
}

unsigned int hash(unsigned int n, const char* restrict key) {
    unsigned int r = 0;
    for (size_t i=0; i<strlen(key); i++) {
	r += ( key[i] << (8 * (i % sizeof(unsigned int))) );
    }
    return (r % n);
}

int ht_insert(struct hashtable* ht, const char* restrict key, void* value) {
    //returns 0 if OK, 1 if error and 2 if item with same key already in table.
    unsigned int h = hash(ht->n,key);

    struct ht_item* new = malloc(sizeof(struct ht_item));
    if (new == NULL) return HT_ERROR;
    new->key = strdup(key);
    new->value = value;
    new->next = NULL;

    struct ht_item* hi = ht->items[h];

    if (hi == NULL) {
	ht->items[h] = new; return HT_SUCCESS; }

    if ( (strcmp(hi->key, key) == 0) && ht->unique) { free(new); return HT_ALREADYEXISTS; }

    while (hi->next != NULL) {
	hi = hi->next;
	if ( (strcmp(hi->key, key) == 0) && ht->unique) {free(new); return HT_ALREADYEXISTS;}
    }

    hi->next = new;
    return HT_SUCCESS;
}

void* ht_search_ind(struct hashtable* ht, const char* restrict key, int index) {
    unsigned int h = hash(ht->n,key);

    struct ht_item* i = ht->items[h];

    int n = 0;

    while (i != NULL) {
	if (strcmp(i->key, key) == 0) {
	    if (n == index) return (i->value); else n++; }
	i = i->next;
    }

    return NULL;
}

int ht_all(struct hashtable* ht, ht_all_cb cb, void* ctx) {

    int c = 0;
    for (unsigned int i=0; i < ht->n; i++) {

	    struct ht_item* hti = ht->items[i];

	    while (hti) {
	    if (cb) cb(hti->key,hti->value,ctx);
	    c++;
	    hti = hti->next;
	    }
    }
    return c;
}

void* ht_search(struct hashtable* ht, const char* restrict key) {
    return ht_search_ind(ht,key,0);
}

int ht_delete_ind(struct hashtable* ht, const char* restrict key, int index) {
    //returns 0 if OK, 1 if error and 2 if item was not in table.
    unsigned int h = hash(ht->n,key);

    struct ht_item* i = ht->items[h];

    int n = 0;

    if (i == NULL) return HT_ERROR;

    if (strcmp(i->key, key) == 0) {
	if (n == index) {free(i); ht->items[h] = NULL; return HT_SUCCESS;} else n++;}

    while ((i != NULL) && (i->next != NULL)) {
	if (strcmp(i->next->key, key) == 0) {
	    if (n == index) {
		if (i->next->next != NULL) {
		    free(i->next);
		    i->next = i->next->next;
		} else {free(i->next); i->next = NULL; }
	    } else n++; }
	i = i->next;
    }
    return HT_NOTFOUND;
}
int ht_delete(struct hashtable* ht, const char* restrict key) {
    return ht_delete_ind(ht,key,0);
}
