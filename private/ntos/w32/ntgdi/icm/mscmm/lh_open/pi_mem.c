/*
	File:		PI_Memory.c

	Contains:	
				
	Written by:	U. J. Krabbenhoeft

	Version:

	Copyright:	© 1993-1997 by Heidelberger Druckmaschinen AG, all rights reserved.

*/
#ifndef PI_BasicTypes_h
#include "PI_Basic.h"
#endif

#ifndef PI_Memory_h
#include "PI_Mem.h"
#endif

#ifndef PI_Machine_h
#include "PI_Mach.h"
#endif

#include <string.h>	
#ifdef IntelMode
#include "PI_Swap.h"
#endif

#if __IS_MAC
void Debugger();
#endif

/* --------------------------------------------------------------------------

	Ptr SmartNewPtr(Size byteCount,
					OSErr* resultCode)

	Abstract:

	Params:
		
	Return:
		noErr		successful

   -------------------------------------------------------------------------- */
Ptr SmartNewPtr(Size byteCount,
				OSErr* resultCode)
{
	Ptr aPtr;
	aPtr = (Ptr)LH_malloc(byteCount);
	if (aPtr == 0)
		*resultCode = notEnoughMemoryErr;
	else
		*resultCode = 0;
	return aPtr;
}


/* --------------------------------------------------------------------------

	Ptr SmartNewPtrClear(Size byteCount,
						 OSErr* resultCode)

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
Ptr SmartNewPtrClear(Size byteCount,
					 OSErr* resultCode)
{
	Ptr ptr = NULL;

	ptr = SmartNewPtr(byteCount, resultCode);

	if (ptr != NULL)
	{
 		memset( ptr, 0, byteCount );
	}
	return ptr;

}


/* --------------------------------------------------------------------------

	Ptr DisposeIfPtr(Ptr thePtr)

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
Ptr DisposeIfPtr(Ptr thePtr)
{
	if (thePtr)
	{
		LH_free(thePtr);
	}
	return NULL;
}

#ifdef __MWERKS__
extern pascal Ptr NewPtr(Size byteCount);
extern pascal void DisposePtr(Ptr p);
#endif	

#ifdef LH_MEMORY_DEBUG
typedef struct
{
	void* p;
	long l;
} LH_PointerType;
static LH_PointerType PListe[2001];
static long PListeCount = 0;

/* --------------------------------------------------------------------------

	void LH_mallocInit()

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void LH_mallocInit()
{
	long i;
	for (i = 0; i < 2000; i++)
	{
		PListe[i].p = 0;
		PListe[i].l = 0;
	}
	PListeCount = 0;
}


/* --------------------------------------------------------------------------

	void* LH_malloc(long a)

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void* LH_malloc(long a)
{
	long i;
#ifdef __MWERKS__
	void* aPtr = NewPtr(a);
#else
	void* aPtr = malloc(a);
#endif	

	for (i = 0; i < PListeCount; i++)
	{
		if (aPtr < PListe[i].p)
			continue;
		if (aPtr >= (char*)PListe[i].p + PListe[i].l)
			continue;
		Debugger();
	}

	for (i = 0; i < PListeCount; i++)
	{
		if (PListe[i].p == 0)
			break;
	}
	PListe[i].p = aPtr;
	PListe[i].l = a;
	if (i >= PListeCount)
	{
		if (PListeCount < 2000)
			PListeCount++;
	}
	return aPtr;
}


/* --------------------------------------------------------------------------

	void LH_free(void* a)

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void LH_free(void* a)
{
	long i;
	for (i = 0; i < PListeCount; i++)
	{
		if (PListe[i].p == a)
			break;
	}
	if (i < PListeCount)
	{
		PListe[i].p = 0;
		PListe[i].l = 0;
#ifdef __MWERKS__
		DisposePtr(a);
#else
		free(a);
#endif	

	}
	else
	{
		Debugger();
	}
}
#else

/* --------------------------------------------------------------------------

	void LH_mallocInit()

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void LH_mallocInit()
{
}


/* --------------------------------------------------------------------------

	void* LH_malloc(long a)

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void* LH_malloc(long a)
{
#ifdef __MWERKS__
	return NewPtr(a);
#else
	return malloc(a);
#endif	

}


/* --------------------------------------------------------------------------

	void LH_free(void* a)

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void LH_free(void* a)
{
#ifdef __MWERKS__
	DisposePtr((Ptr)a);
#else
	free(a);
#endif	

}
#endif	


/* --------------------------------------------------------------------------

	void SetMem(void* bytePtr,
				size_t numBytes,
				unsigned char byteValue);

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void SetMem(void* bytePtr,
			size_t numBytes,
			unsigned char byteValue);
void SetMem(void* bytePtr,
			size_t numBytes,
			unsigned char byteValue)
{
	memset(bytePtr, byteValue, numBytes);
}

/*void SecondsToDate(unsigned long secs, DateTimeRec *d)
  {
  secs=secs;
  d->year = 55;
  d->month = 8;
  d->day = 8;
  d->hour = 0;
  d->minute = 0;
  d->second = 0;
  d->dayOfWeek = 0;
  }*/

#if !__IS_MAC
/* --------------------------------------------------------------------------

	void BlockMove(const void* srcPtr,
				   void* destPtr,
				   Size byteCount);

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void BlockMove(const void* srcPtr,
			   void* destPtr,
			   Size byteCount)
{
	memmove(destPtr, srcPtr, byteCount);
}
#endif

#ifdef IntelMode
/* --------------------------------------------------------------------------

	void SwapLongOffset(void* p,
						unsigned long a,
						unsigned long b)

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void SwapLongOffset(void* p,
					unsigned long a,
					unsigned long b)
{
	unsigned long* aPtr = (unsigned long*)((char*)p + a);
	unsigned long* bPtr = (unsigned long*)((char*)p + b);
	while (aPtr < bPtr)
	{
		SwapLong(aPtr);
		aPtr++;
	}
}


/* --------------------------------------------------------------------------

	void SwapShortOffset(void* p,
						 unsigned long a,
						 unsigned long b);

	Abstract:

	Params:
		
	Return:

   -------------------------------------------------------------------------- */
void SwapShortOffset(void* p,
					 unsigned long a,
					 unsigned long b);
void SwapShortOffset(void* p,
					 unsigned long a,
					 unsigned long b)
{
	unsigned short* aPtr = (unsigned short*)((char*)p + a);
	unsigned short* bPtr = (unsigned short*)((char*)p + b);
	while (aPtr < bPtr)
	{
		SwapShort(aPtr);
		aPtr++;
	}
}

#endif


