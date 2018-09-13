/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    conv.h

Abstract:

    This module contains the internal structures and definitions used
    by the conversion area.

Author:

    KazuM Mar.8,1993

Revision History:

--*/

#ifndef _CONV_H_
#define _CONV_H_

#if defined(FE_IME)
//
//  Externs
//


//
// Attributes flags:
//
#define COMMON_LVB_GRID_SINGLEFLAG 0x2000 // DBCS: Grid attribute: use for ime cursor.

/*
 * Exported function
 */

/*
 * Internal function
 */

DWORD
NtUserCheckImeHotKey(
    UINT uVKey,
    LPARAM lParam
    );

BOOL
NtUserGetImeHotKey(
    IN DWORD dwID,
    OUT PUINT puModifiers,
    OUT PUINT puVKey,
    OUT HKL  *phkl);


/*
 * Prototype definition
 */

VOID
LinkConversionArea(
    IN PCONSOLE_INFORMATION Console,
    IN PCONVERSIONAREA_INFORMATION ConvAreaInfo
    );

NTSTATUS
FreeConvAreaScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo
    );

NTSTATUS
AllocateConversionArea(
    IN PCONSOLE_INFORMATION Console,
    IN COORD dwScreenBufferSize,
    OUT PCONVERSIONAREA_INFORMATION *ConvAreaInfo
    );

NTSTATUS
SetUpConversionArea(
    IN PCONSOLE_INFORMATION Console,
    IN COORD coordCaBuffer,
    IN SMALL_RECT rcViewCaWindow,
    IN COORD coordConView,
    IN DWORD dwOption,
    OUT PCONVERSIONAREA_INFORMATION *ConvAreaInfo
    );


VOID
WriteConvRegionToScreen(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PCONVERSIONAREA_INFORMATION ConvAreaInfo,
    IN PSMALL_RECT ClippedRegion
    );

BOOL
ConsoleImeBottomLineUse(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN SHORT ScrollOffset
    );

VOID
ConsoleImeBottomLineInUse(
    IN PSCREEN_INFORMATION ScreenInfo
    );


NTSTATUS
CreateConvAreaUndetermine(
    PCONSOLE_INFORMATION Console
    );

NTSTATUS
CreateConvAreaModeSystem(
    PCONSOLE_INFORMATION Console
    );

NTSTATUS
WriteUndetermineChars(
    PCONSOLE_INFORMATION Console,
    LPWSTR lpString,
    PBYTE  lpAtr,
    PWORD  lpAtrIdx,
    DWORD  NumChars
    );

NTSTATUS
FillUndetermineChars(
    PCONSOLE_INFORMATION Console,
    PCONVERSIONAREA_INFORMATION ConvAreaInfo
    );

NTSTATUS
ConsoleImeCompStr(
    IN PCONSOLE_INFORMATION Console,
    IN LPCONIME_UICOMPMESSAGE CompStr
    );

NTSTATUS
ConsoleImeResizeModeSystemView(
    PCONSOLE_INFORMATION Console,
    SMALL_RECT WindowRect
    );

NTSTATUS
ConsoleImeResizeCompStrView(
    PCONSOLE_INFORMATION Console,
    SMALL_RECT WindowRect
    );

NTSTATUS
ConsoleImeResizeModeSystemScreenBuffer(
    PCONSOLE_INFORMATION Console,
    COORD NewScreenSize
    );

NTSTATUS
ConsoleImeResizeCompStrScreenBuffer(
    PCONSOLE_INFORMATION Console,
    COORD NewScreenSize
    );

SHORT
CalcWideCharToColumn(
    IN PCONSOLE_INFORMATION Console,
    IN PCHAR_INFO Buffer,
    IN DWORD NumberOfChars
    );




LONG
ConsoleImePaint(
    IN PCONSOLE_INFORMATION Console,
    IN PCONVERSIONAREA_INFORMATION ConvAreaInfo
    );




VOID
ConsoleImeViewInfo(
    IN PCONSOLE_INFORMATION Console,
    IN PCONVERSIONAREA_INFORMATION ConvAreaInfo,
    IN COORD coordConView
    );

VOID
ConsoleImeWindowInfo(
    IN PCONSOLE_INFORMATION Console,
    IN PCONVERSIONAREA_INFORMATION ConvAreaInfo,
    IN SMALL_RECT rcViewCaWindow
    );

NTSTATUS
ConsoleImeResizeScreenBuffer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD NewScreenSize,
    PCONVERSIONAREA_INFORMATION ConvAreaInfo
    );

NTSTATUS
ConsoleImeWriteOutput(
    IN PCONSOLE_INFORMATION Console,
    IN PCONVERSIONAREA_INFORMATION ConvAreaInfo,
    IN PCHAR_INFO Buffer,
    IN SMALL_RECT CharRegion,
    IN BOOL fUnicode
    );


NTSTATUS
ImeControl(
    IN PCONSOLE_INFORMATION Console,
    IN HWND hWndConsoleIME,
    IN PCOPYDATASTRUCT lParam
    ) ;

BOOL
InsertConverTedString(
    IN PCONSOLE_INFORMATION Console,
    LPWSTR lpStr
    ) ;


VOID
SetUndetermineAttribute(
    IN PCONSOLE_INFORMATION Console
    ) ;

VOID
StreamWriteToScreenBufferIME(
    IN PWCHAR String,
    IN SHORT StringLength,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PCHAR StringA
    ) ;


//
// windows\imm\server\hotkey.c
//
DWORD
CheckImeHotKey(
    UINT uVKey,         // virtual key
    LPARAM lParam       // lparam of WM_KEYxxx message
    ) ;

//
// output.c (for use convarea.c\StreamWriteToScreenBufferIME() )
//

NTSTATUS
MergeAttrStrings(
    IN PATTR_PAIR Source,
    IN WORD SourceLength,
    IN PATTR_PAIR Merge,
    IN WORD MergeLength,
    OUT PATTR_PAIR *Target,
    OUT LPWORD TargetLength,
    IN SHORT StartIndex,
    IN SHORT EndIndex,
    IN PROW Row,
    IN PSCREEN_INFORMATION ScreenInfo
    ) ;


VOID
ResetTextFlags(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN SHORT StartY,
    IN SHORT EndY
    ) ;

#endif // FE_IME

#endif  // _CONV_H_

