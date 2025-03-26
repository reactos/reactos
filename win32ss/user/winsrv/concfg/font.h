/*
 * PROJECT:     ReactOS Console Server DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Console GDI Fonts Management.
 * COPYRIGHT:   Copyright 2017-2022 Hermès Bélusca-Maïto
 *              Copyright 2017 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

/* DEFINES ********************************************************************/

#define INVALID_CP  ((UINT)-1)

#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

#define CP_USA      437  // United States (OEM)

#include <cjkcode.h>

#define IsBoldFont(Weight)  \
    ((Weight) >= FW_SEMIBOLD) /* Sometimes, just > FW_MEDIUM */

/*
 * @struct  TrueType font list, cached from
 * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
 *
 * See the definition of struct _TT_FONT_LIST
 * in https://github.com/microsoft/terminal/blob/main/dep/Console/winconp.h
 */
#define BOLD_MARK   L'*'

typedef struct _TT_FONT_ENTRY
{
    SINGLE_LIST_ENTRY Entry;
    UINT CodePage;
    BOOL DisableBold;
    WCHAR FaceName[LF_FACESIZE];
    WCHAR FaceNameAlt[LF_FACESIZE];
} TT_FONT_ENTRY, *PTT_FONT_ENTRY;


/* FUNCTIONS ******************************************************************/

BYTE
CodePageToCharSet(
    _In_ UINT CodePage);

// FIXME: Will be redefined once we support a font cache.
typedef struct _FONT_DATA
{
    _Inout_updates_z_(LF_FACESIZE) PWSTR FaceName;
    ULONG Weight;
    ULONG Family;
    COORD Size;
    BYTE  CharSet;
} FONT_DATA, *PFONT_DATA;

HFONT
CreateConsoleFontEx(
    _In_     LONG Height,
    _In_opt_ LONG Width,
    _Inout_updates_z_(LF_FACESIZE)
         PWSTR FaceName,
    _In_ ULONG FontWeight,
    _In_ ULONG FontFamily,
    _In_ UINT  CodePage,
    _In_ BOOL  UseDefaultFallback,
    _Out_ PFONT_DATA FontData);

HFONT
CreateConsoleFont2(
    _In_     LONG Height,
    _In_opt_ LONG Width,
    _Inout_  PCONSOLE_STATE_INFO ConsoleInfo);

HFONT
CreateConsoleFont(
    _Inout_ PCONSOLE_STATE_INFO ConsoleInfo);

_Success_(return)
BOOL
GetFontCellSize(
    _In_opt_ HDC hDC,
    _In_  HFONT hFont,
    _Out_ PUINT Height,
    _Out_ PUINT Width);

BOOL
IsValidConsoleFont2(
    _In_ PLOGFONTW lplf,
    _In_ PNEWTEXTMETRICW lpntm,
    _In_ DWORD FontType,
    _In_ UINT  CodePage);

BOOL
IsValidConsoleFont(
    // _In_reads_or_z_(LF_FACESIZE)
    _In_ PCWSTR FaceName,
    _In_ UINT CodePage);


VOID
InitTTFontCache(VOID);

VOID
ClearTTFontCache(VOID);

VOID
RefreshTTFontCache(VOID);

PTT_FONT_ENTRY
FindCachedTTFont(
    _In_reads_or_z_opt_(LF_FACESIZE)
         PCWSTR FaceName,
    _In_ UINT CodePage);

/**
 * @brief
 * Verifies whether the given font is an additional console TrueType font.
 * Wrapper macros around FindCachedTTFont().
 *
 * @remark
 * These macros are equivalents of the functions
 * IsAvailableTTFont() and IsAvailableTTFontCP() in
 * https://github.com/microsoft/terminal/blob/main/src/propsheet/dbcs.cpp
 **/

#define IsAdditionalTTFont(FaceName) \
    (FindCachedTTFont((FaceName), INVALID_CP) != NULL)

#define IsAdditionalTTFontCP(FaceName, CodePage) \
    (FindCachedTTFont((FaceName), (CodePage)) != NULL)

/* EOF */
