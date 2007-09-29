/*
 * Generic hash-table with fixed-length keys.
 */

typedef struct fz_hashtable_s fz_hashtable;

fz_error *fz_newhash(fz_hashtable **tablep, int initialsize, int keylen);
fz_error *fz_resizehash(fz_hashtable *table, int newsize);
void fz_debughash(fz_hashtable *table);
void fz_emptyhash(fz_hashtable *table);
void fz_drophash(fz_hashtable *table);

void *fz_hashfind(fz_hashtable *table, void *key);
fz_error *fz_hashinsert(fz_hashtable *table, void *key, void *val);
fz_error *fz_hashremove(fz_hashtable *table, void *key);

int fz_hashlen(fz_hashtable *table);
void *fz_hashgetkey(fz_hashtable *table, int idx);
void *fz_hashgetval(fz_hashtable *table, int idx);

