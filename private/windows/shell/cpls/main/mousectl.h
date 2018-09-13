/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mousectl.h

Abstract:

    This module contains the header information for the Mouse control.

Revision History:

--*/



#ifndef _MOUSECTL_H
#define _MOUSECTL_H



#define MOUSECTL_CLASSNAME  TEXT("PropertyMouseButtonControl")


BOOL
RegisterMouseControlStuff(
    HINSTANCE instance);

void
MouseControlSetSwap(
    HWND window,
    BOOL swap);



#endif
