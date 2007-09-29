/*
 * Basic crypto functions.
 * Independent of the rest of fitz.
 * For further encapsulation in filters, or not.
 */

/* crc-32 checksum */

unsigned long fz_crc32(unsigned long crc, unsigned char *buf, int len);

/* md5 digests */

typedef struct fz_md5_s fz_md5;

struct fz_md5_s
{
	unsigned long state[4];
	unsigned long count[2];
	unsigned char buffer[64];
};

void fz_md5init(fz_md5 *state);
void fz_md5update(fz_md5 *state, unsigned char *input, unsigned inlen);
void fz_md5final(fz_md5 *state, unsigned char digest[16]);

/* arc4 crypto */

typedef struct fz_arc4_s fz_arc4;

struct fz_arc4_s
{
	unsigned x;
	unsigned y;
	unsigned char state[256];
};

void fz_arc4init(fz_arc4 *state, unsigned char *key, unsigned len);
unsigned char fz_arc4next(fz_arc4 *state);
void fz_arc4encrypt(fz_arc4 *state, unsigned char *dest, unsigned char *src, unsigned len);

/* TODO: sha1 */
/* TODO: aes */

