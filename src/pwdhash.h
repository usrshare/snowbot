// vim: cin:sts=4:sw=4 
#ifndef PWDHASH_H
#define PWDHASH_H

int hash_pwd(int alg, const char* restrict salt, const char* restrict pwd, char* o_hash);

#endif
