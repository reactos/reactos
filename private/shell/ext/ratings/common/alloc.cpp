/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* alloc.c -- Implementation of operator new and operator delete.
 *
 * History:
 *	10/06/93	gregj	Created.
 *	11/29/93	gregj	Added debug instrumentation.
 *
 */

#include "npcommon.h"
#include <npalloc.h>
#include <netlib.h>


//====== Memory allocation functions =================================

// Alloc a chunk of memory, quickly, with no 64k limit on size of
// individual objects or total object size.
//
void * WINAPI MemAlloc(long cb)
{
	return (void *)::LocalAlloc(LPTR, cb);
}

// Realloc one of above.  If pb is NULL, then this function will do
// an alloc for you.
//
void * WINAPI MemReAlloc(void * pb, long cb)
{
	if (pb == NULL)
		return ::MemAlloc(cb);

	return (void *)::LocalReAlloc((HLOCAL)pb, cb, LMEM_MOVEABLE | LMEM_ZEROINIT);
}

// Free a chunk of memory alloced or realloced with above routines.
//
BOOL WINAPI MemFree(void * pb)
{
    return ::LocalFree((HLOCAL)pb) ? TRUE : FALSE;
}

// Overloaded allocation operators

void * _cdecl operator new(size_t size)
{
	return (void *)MemAlloc(size); 
}
void _cdecl operator delete(void *ptr)
{
	MemFree(ptr);
}

#ifdef DEBUG

MEMWATCH::MEMWATCH(LPCSTR lpszLabel)
	: _lpszLabel(lpszLabel)
{
	_info.pNext = NULL;
	_info.cAllocs = 0;
	_info.cFrees = 0;
	_info.cbAlloc = 0;
	_info.cbMaxAlloc = 0;
	_info.cbTotalAlloc = 0;
    fStats = TRUE;
	MemRegisterWatcher(&_info);
}

MEMWATCH::~MEMWATCH()
{
	MemDeregisterWatcher(&_info);
    if (fStats || ((_info.cAllocs - _info.cFrees) != 0)) {
        if (!fStats) {
            OutputDebugString("Memory leak: ");
        }
    	OutputDebugString(_lpszLabel);
    	char szBuf[100];
    	wsprintf(szBuf, "%d allocs, %d orphans, %d byte footprint, %d byte usage\r\n",
    			 _info.cAllocs,
    			 _info.cAllocs - _info.cFrees,
    			 _info.cbMaxAlloc,
    			 _info.cbTotalAlloc);
    	OutputDebugString(szBuf);
    }
}

MemLeak::MemLeak(LPCSTR lpszLabel)
	: MEMWATCH(lpszLabel)
{
    fStats = FALSE;
}

MemOff::MemOff()
{
    pvContext = MemUpdateOff();
}

MemOff::~MemOff()
{
    MemUpdateContinue(pvContext);
}

#endif	/* DEBUG */

