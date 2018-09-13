/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    convarea.h

Abstract:

    This module contains the internal structures and definitions used
    by the conversion area.

Author:

    KazuM Mar.8,1993

Revision History:

--*/

#ifndef _CONVAREA_H_
#define _CONVAREA_H_

#if defined(FE_IME)
typedef struct _CONVERSION_AREA_BUFFER_INFO {
    COORD      coordCaBuffer;
    SMALL_RECT rcViewCaWindow;
    COORD      coordConView;
} CONVERSION_AREA_BUFFER_INFO,*PCONVERSION_AREA_BUFFER_INFO;



typedef struct _CONVERSIONAREA_INFORMATION {
    DWORD  ConversionAreaMode;
        #define CA_HIDDEN      0x01        // Set:Hidden    Reset:Active
        #define CA_STATUS_LINE 0x04
        #define CA_HIDE_FOR_SCROLL              0x10

    CONVERSION_AREA_BUFFER_INFO CaInfo;
    struct _SCREEN_INFORMATION *ScreenBuffer;

    struct _CONVERSIONAREA_INFORMATION *ConvAreaNext;
} CONVERSIONAREA_INFORMATION, *PCONVERSIONAREA_INFORMATION;



typedef struct _CONSOLE_IME_INFORMATION {
    DWORD ScrollFlag;
        #define HIDE_FOR_SCROLL               0x01
    LONG ScrollWaitTimeout;
        #define SCROLL_WAIT_TIMER         2
    LONG ScrollWaitCountDown;

    //
    // Composition String information
    //
    LPCONIME_UICOMPMESSAGE CompStrData;
    BOOLEAN SavedCursorVisible;  // whether cursor is visible (set by user)

    //
    // IME status/mode information
    //
    PCONVERSIONAREA_INFORMATION ConvAreaMode;
    PCONVERSIONAREA_INFORMATION ConvAreaSystem;
    DWORD  ConvAreaModePosition;
        // VIEW_LEFT  0
        // VIEW_RIGHT 1


    //
    // IME compositon string information
    //
    ULONG NumberOfConvAreaCompStr;
    PCONVERSIONAREA_INFORMATION *ConvAreaCompStr;

    //
    // Root of conversion area information
    //
    PCONVERSIONAREA_INFORMATION ConvAreaRoot;

} CONSOLE_IME_INFORMATION, *PCONSOLE_IME_INFORMATION;

#endif // FE_IME

#endif  // _CONVAREA_H_
