/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKLOCAL.C
 *  WOW32 16-bit Kernel API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wklocal.c);


ULONG FASTCALL WK32LocalAlloc(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALALLOC16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALALLOC16), parg16);

    ul = GETHLOCAL16(LocalAlloc(
	WORD32(parg16->f1),
	WORD32(parg16->f2)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalCompact(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALCOMPACT16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALCOMPACT16), parg16);

    ul = GETWORD16(LocalCompact(
	WORD32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalFlags(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALFLAGS16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALFLAGS16), parg16);

    ul = GETWORD16(LocalFlags(
	HLOCAL32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalFree(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALFREE16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALFREE16), parg16);

    ul = GETHLOCAL16(LocalFree(
	HLOCAL32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalHandle(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALHANDLE16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALHANDLE16), parg16);

    ul = GETHLOCAL16(LocalHandle(
	(LPSTR) WORD32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalInit(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALINIT16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALINIT16), parg16);

#ifdef API16
    ul = GETBOOL16(LocalInit(
	WORD32(parg16->f1),
	WORD32(parg16->f2),
	WORD32(parg16->f3)
    ));
#else
    ul = 0;
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalLock(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALLOCK16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALLOCK16), parg16);

    ul = GETNPSTRBOGUS(LocalLock(
	HLOCAL32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalNotify(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALNOTIFY16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALNOTIFY16), parg16);

#ifdef API16
    ul = GETPROC16(LocalNotify(
	PROC32(parg16->f1)
    ));
#else
    ul = 0;
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalReAlloc(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALREALLOC16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALREALLOC16), parg16);

    ul = GETHLOCAL16(LocalReAlloc(
	HLOCAL32(parg16->f1),
	WORD32(parg16->f2),
	WORD32(parg16->f3)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalShrink(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALSHRINK16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALSHRINK16), parg16);

    ul = GETWORD16(LocalShrink(
	HLOCAL32(parg16->f1),
	WORD32(parg16->f2)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalSize(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALSIZE16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALSIZE16), parg16);

    ul = GETWORD16(LocalSize(
	HLOCAL32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32LocalUnlock(PVDMFRAME pFrame)
{
    ULONG ul;
    register PLOCALUNLOCK16 parg16;

    GETARGPTR(pFrame, sizeof(LOCALUNLOCK16), parg16);

    ul = GETBOOL16(LocalUnlock(
	HLOCAL32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}
