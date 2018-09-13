/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    util.h

Abstract:

    This module contains the header information for the utility routines
    for this project.

Revision History:

--*/



#ifndef _UTIL_H
#define _UTIL_H



void
HourGlass(
    BOOL fOn);

int
MyMessageBox(
    HWND hWnd,
    UINT uText,
    UINT uCaption,
    UINT uType,
    ...);

void
TrackInit(
    HWND hwndScroll,
    int nCurrent,
    PARROWVSCROLL pAVS);

int
TrackMessage(
    WPARAM wParam,
    LPARAM lParam,
    PARROWVSCROLL pAVS);

typedef struct HWPAGEINFO {
    GUID    guidClass;                  // Setup device class
    UINT    idsTshoot;                  // Troubleshooter string
} HWPAGEINFO, *PHWPAGEINFO;
typedef const HWPAGEINFO *PCHWPAGEINFO;

HPROPSHEETPAGE
CreateHardwarePage(PCHWPAGEINFO phpi);

#endif
