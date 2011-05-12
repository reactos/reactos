
typedef struct _rc4_key
{
    unsigned char perm[256];
    unsigned char index1;
    unsigned char index2;
}rc4_key;

void rc4_init(rc4_key *const state, const unsigned char *key, int keylen);
void rc4_crypt(rc4_key *const state, const unsigned char *inbuf, unsigned char *outbuf, int buflen);
