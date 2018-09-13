/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mapctl.h

Abstract:

    This module contains the information for the map control of the
    Date/Time applet.

Revision History:

--*/



#ifndef _MAPCTL_H
#define _MAPCTL_H



//
//  Constant Declarations.
//

#define MAPN_TOUCH           0
#define MAPN_SELECT          1




//
//  Typedef Declarations.
//

typedef struct
{
    NMHDR hdr;
    int index;

} NFYMAPEVENT;

#define MAPCTL_MAX_INDICES  256




//
//  Function Prototypes.
//

BOOL
RegisterMapControlStuff(
    HINSTANCE instance);

void
MapControlSetSeaRegionHighlight(
    HWND window,
    int index,
    int value,
    int x,
    int cx);

void
MapControlSetLandRegionHighlight(
    HWND window,
    int index,
    BOOL highlighted,
    int x,
    int cx);

void
MapControlInvalidateDirtyRegions(
    HWND window);

void
MapControlRotateTo(
    HWND window,
    int x,
    BOOL animate);


#endif // _MAPCTL_H
