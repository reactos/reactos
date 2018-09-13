/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSMAN.C
 *  WOW32 16-bit Sound API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wsman.c);

ULONG FASTCALL WS32DoBeep(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    return (ULONG)MessageBeep(0);
}

