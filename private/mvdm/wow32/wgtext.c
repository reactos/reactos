/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGTEXT.C
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
 *  10-Nov-1992 Modified GetTextMetrics to GetTextMetricsWOW by Chandan Chauhan
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wgtext.c);


ULONG FASTCALL WG32ExtTextOut(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t5;
    PSTR pstr6;
    PINT p8;
    register PEXTTEXTOUT16 parg16;
    INT      BufferT[256];

    GETARGPTR(pFrame, sizeof(EXTTEXTOUT16), parg16);
    GETRECT16(parg16->f5, &t5);
    GETSTRPTR(parg16->f6, parg16->f7, pstr6);
    if (DWORD32(parg16->f8)) {
       p8 = STACKORHEAPALLOC(parg16->f7 * sizeof(INT), sizeof(BufferT), BufferT);
       getintarray16((VPINT16)DWORD32(parg16->f8), parg16->f7, p8);   // *this* INT array is optional
    } else {
        p8 = NULL;
    }


    ul = GETBOOL16(ExtTextOut(
                    HDC32(parg16->f1),
                    INT32(parg16->f2),
                    INT32(parg16->f3),
                    (WORD32(parg16->f4) & (ETO_CLIPPED|ETO_OPAQUE)),
                    &t5,
                    pstr6,
                    WORD32(parg16->f7),
                    (LPINT)p8
                    ));

    FREESTRPTR(pstr6);
    STACKORHEAPFREE(p8, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetTextExtent(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSTR pstr2;
    SIZE size4;
    register PGETTEXTEXTENT16 parg16;

    GETARGPTR(pFrame, sizeof(GETTEXTEXTENT16), parg16);
    GETSTRPTR(parg16->f2, parg16->f3, pstr2);

    if (GETDWORD16(GetTextExtentPoint(
                    HDC32(parg16->f1),
                    pstr2,
                    INT32(parg16->f3),
                    &size4
                   )))
    {
        // check if either cx or cy are bigger than SHRT_MAX == 7fff
        // but do it in ONE SINGLE check

	    if ((size4.cx | size4.cy) & ~SHRT_MAX)
	    {
	        if (size4.cx > SHRT_MAX)
	           ul = SHRT_MAX;
	        else
	           ul = (ULONG)size4.cx;

	        if (size4.cy > SHRT_MAX)
	           ul |= (SHRT_MAX << 16);
	        else
	           ul |= (ULONG)(size4.cy << 16);
	    }
	    else
	    {
	        ul = (ULONG)(size4.cx | (size4.cy << 16));
	    }

    }
    FREESTRPTR(pstr2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32GetTextMetrics(PVDMFRAME pFrame)
{
    ULONG ul;
    TEXTMETRIC t2;
    register PGETTEXTMETRICS16 parg16;

    GETARGPTR(pFrame, sizeof(GETTEXTMETRICS16), parg16);

    ul = GETBOOL16(GetTextMetrics(
                    HDC32(parg16->f1),
                    &t2
                  ));

#ifdef FE_SB
    // original source code should be fixed
    // If GetTextMetrics return value is FALSE, don't need set data to 16bit
    // TEXTMETRICS STRUCTURE.
    // kksuzuka #3759 BC++40J is not see return value and used TEXTMETRICS
    // data.  1994.11.16 V-HIDEKK
    if( ul )
#endif // FE_SB

    PUTTEXTMETRIC16(parg16->f2, &t2);

    FREEARGPTR(parg16);
    RETURN(ul);
}
