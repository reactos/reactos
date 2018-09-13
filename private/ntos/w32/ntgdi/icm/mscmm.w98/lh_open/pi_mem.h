/*
	File:		PI_Mem.h

	Contains:	

	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef PI_Memory_h
#define PI_Memory_h

#include <stdlib.h>
typedef INT32 Size;

#ifdef __cplusplus
extern "C" {
#endif
void *LH_malloc(long a);
void LH_free(void *a);
void LH_mallocInit();
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

Ptr  		DisposeIfPtr		( Ptr aPtr );

Ptr
SmartNewPtr(Size byteCount, OSErr *resultCode);

Ptr
SmartNewPtrClear(Size byteCount, OSErr *resultCode);

UINT32 TickCount(void);
double MyTickCount(void);

double rint(double a);
void BlockMove(const void* srcPtr,
			   void* destPtr,
			   Size byteCount);
#ifdef __cplusplus
}
#endif

#endif 
