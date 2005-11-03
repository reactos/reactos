// MD5.H - header file for MD5.CPP

/*
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
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

#ifndef __MD5_H
#define __MD5_H

#ifdef __cplusplus
#include <string>
#endif//__cplusplus

#ifndef uchar
#define uchar unsigned char
#endif//uchar

#ifndef ushort
#define ushort unsigned short
#endif//ushort

#ifndef ulong
#define ulong unsigned long
#endif//ulong

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

typedef uchar *POINTER; // POINTER defines a generic pointer type
typedef ushort UINT2; // UINT2 defines a two byte word
typedef ulong UINT4; // UINT4 defines a four byte word

// MD5 context.
typedef struct
{
	UINT4 state[4];           // state (ABCD)
	UINT4 count[2];           // number of bits, modulo 2^64 (lsb first)
	unsigned char buffer[64]; // input buffer
} MD5_CTX;

void MD5Init ( MD5_CTX * );
void MD5Update ( MD5_CTX *, const char *, unsigned int );
void MD5Final ( uchar [16], MD5_CTX * );

void digest2ascii ( char *ascii, const uchar *digest );
void ascii2digest ( uchar *digest, const char *ascii );

#ifdef __cplusplus

} // extern "C"

class MD5
{
public:
	MD5();
	void Init();
	void Update ( const std::string& s );
	std::string Final ( char* digest = 0 );

private:
	MD5_CTX _ctx;
};

std::string MD5Hex ( const std::string& s );

std::string MD5Bin ( const std::string& s );

std::string HMAC_MD5 (
	const std::string& key,
	const std::string& text,
	char* out_bin = NULL );

#endif//__cplusplus

#endif//__MD5_H
