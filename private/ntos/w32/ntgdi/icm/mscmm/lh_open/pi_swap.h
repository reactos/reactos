/*
	File:		PI_SwapMem.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef PI_SwapMem_h
#define PI_SwapMem_h

#ifdef IntelMode
#if defined(__cplusplus)
extern "C" {
#endif
#define SwapLong(b) (*((unsigned long *)(b))) = ((unsigned long)(((unsigned char *)(b))[3]))         | (((unsigned long)(((unsigned char *)(b))[2])) << 8) | \
		        (((unsigned long)(((unsigned char *)(b))[1])) << 16) | (((unsigned long)(((unsigned char *)(b))[0])) << 24);
#define SwapShort(b) (*((unsigned short *)(b))) = ((unsigned short)(((unsigned char *)(b))[1])) | ((unsigned short)(((unsigned char *)(b))[0] << 8));

void SwapLongOffset( void *p, unsigned long a, unsigned long b); /* */
void SwapShortOffset( void *p, unsigned long a, unsigned long b);

#if defined(__cplusplus)
}
#endif
#endif
#endif
