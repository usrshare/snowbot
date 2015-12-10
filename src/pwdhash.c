// vim: cin:sts=4:sw=4 
#include "pwdhash.h"
#include <openssl/evp.h>
#include <string.h>

int hash_hex(const char* restrict hash, size_t hash_sz, char* hex) {

    // hex is assumed to be at least hash_sz*2+1 bytes long.

    for (int i=0; i < hash_sz; i++) {
	snprintf(hex+(2*i),3,"%02x", (unsigned char*)(hash)[i]);
    }
    return 0;
}

int hash_pwd(int alg, const char* restrict salt, const char* restrict pwd, char* o_hash) {

    EVP_MD_CTX *mctx;
    mctx = EVP_MD_CTX_create();

    const EVP_MD *md = EVP_sha512();

    EVP_DigestInit_ex(mctx, md, NULL);

    if (salt) EVP_DigestUpdate(mctx, salt, strlen(salt));
    EVP_DigestUpdate(mctx, pwd, strlen(pwd));

    unsigned int md_len = 0;
    unsigned char hash[EVP_MAX_MD_SIZE];

    EVP_DigestFinal_ex(mctx, hash, &md_len);
    if (o_hash) memcpy(o_hash,hash,md_len);

    for(unsigned int i = 0; i < md_len; i++)
	printf("%02x", hash[i]);
    printf("\n");

    EVP_MD_CTX_destroy(mctx);
    EVP_cleanup();
    return 0;
}
