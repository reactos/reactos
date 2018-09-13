/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUTEXT.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wutext.c);


ULONG FASTCALL WU32GetTabbedTextExtent(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    PINT p5;
    register PGETTABBEDTEXTEXTENT16 parg16;
    INT BufferT[256];

    GETARGPTR(pFrame, sizeof(GETTABBEDTEXTEXTENT16), parg16);
    GETPSZPTR(parg16->f2, psz2);
    p5 = STACKORHEAPALLOC(parg16->f4 * sizeof(INT), sizeof(BufferT), BufferT);
    getintarray16(parg16->f5, parg16->f4, p5);

    ul = GETDWORD16(GetTabbedTextExtent(HDC32(parg16->f1), psz2,
                     INT32(parg16->f3),  INT32(parg16->f4), p5 ));

    STACKORHEAPFREE(p5, BufferT);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32TabbedTextOut(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz4;
    PINT p7;
    register PTABBEDTEXTOUT16 parg16;
    INT BufferT[256];

    GETARGPTR(pFrame, sizeof(TABBEDTEXTOUT16), parg16);
    GETPSZPTR(parg16->f4, psz4);
    p7 = STACKORHEAPALLOC(parg16->f6 * sizeof(INT), sizeof(BufferT), BufferT);
    getintarray16(parg16->f7, parg16->f6, p7);

    ul = GETLONG16(TabbedTextOut(
        HDC32(parg16->f1),
        INT32(parg16->f2),
        INT32(parg16->f3),
        psz4,
        INT32(parg16->f5),
        INT32(parg16->f6),
        p7,
        INT32(parg16->f8)
        ));

    STACKORHEAPFREE(p7, BufferT);
    FREEPSZPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}
