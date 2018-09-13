/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* npalloc.h -- Definitions for new/delete functions.
 *
 * History:
 *	10/06/93	gregj	Created
 *	11/29/93	gregj	Added debug instrumentation
 */

#ifndef _INC_NPALLOC
#define _INC_NPALLOC

inline BOOL InitHeap(void)
{
	return TRUE;
}

#ifdef DEBUG

#ifndef _INC_NETLIB
#include <netlib.h>
#endif

class MEMWATCH
{
private:
	LPCSTR _lpszLabel;
	MemAllocInfo _info;

protected:
    BOOL   fStats;

public:
	MEMWATCH(LPCSTR lpszLabel);
	~MEMWATCH();
};

class MemLeak : MEMWATCH
{
public:
	MemLeak(LPCSTR lpszLabel);
	~MemLeak() {}
};

class MemOff 
{
private:
    LPVOID  pvContext;
public:
    MemOff();
    ~MemOff();
};
#endif

#ifdef DEBUG
#define MEMLEAK(d,t) MemLeak d ( t )
#define MEMOFF(d) MemOff d
#else
#define MEMLEAK(d,t)
#define MEMOFF(d)
#endif    

#endif	/* _INC_NPALLOC */
