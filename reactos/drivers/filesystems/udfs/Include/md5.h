/* MD5.H - header file for MD5C.C
 * $FreeBSD: src/sys/sys/md5.h,v 1.16 2002/06/24 14:18:39 mux Exp $
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

#ifndef _SYS_MD5_H_
#define _SYS_MD5_H_
/* MD5 context. */
typedef struct UDF_MD5Context {
  uint32 state[4];	/* state (ABCD) */
  uint32 count[2];	/* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];	/* input buffer */
} UDF_MD5_CTX;

#ifdef __cplusplus
#define MD5_DECL extern "C"
#else
#define MD5_DECL extern
#endif

MD5_DECL
void   UDF_MD5Init (UDF_MD5_CTX *);

MD5_DECL
void   UDF_MD5Update (UDF_MD5_CTX *, const unsigned char *, unsigned int);

MD5_DECL
void   UDF_MD5Pad (UDF_MD5_CTX *);

MD5_DECL
void   UDF_MD5Final (unsigned char [16], UDF_MD5_CTX *);

MD5_DECL
char * UDF_MD5End(UDF_MD5_CTX *, char *);

typedef struct _UDF_MD5_DIGEST {
    uint8 d[16];
} UDF_MD5_DIGEST , *PUDF_MD5_DIGEST;

/*char * MD5File(const char *, char *);
char * MD5FileChunk(const char *, char *, off_t, off_t);
char * MD5Data(const unsigned char *, unsigned int, char *);*/

#endif /* _SYS_MD5_H_ */
