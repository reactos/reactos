/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/concfg/font.h
 * PURPOSE:         Console Fonts Management
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

/* DEFINES ********************************************************************/

#define CP_SHIFTJIS 932 // Japanese Shift-JIS
#define CP_HANGUL   949 // Korean Hangul
#define CP_GB2312   936 // Chinese Simplified (GB2312)
#define CP_BIG5     950 // Chinese Traditional (Big5)

/* IsFarEastCP(CodePage) */
#define IsCJKCodePage(CodePage) \
    ((CodePage) == CP_SHIFTJIS || (CodePage) == CP_HANGUL || \
     (CodePage) == CP_BIG5     || (CodePage) == CP_GB2312)

/* FUNCTIONS ******************************************************************/

BYTE
CodePageToCharSet(
    IN UINT CodePage);

HFONT
CreateConsoleFontEx(
    IN LONG Height,
    IN LONG Width OPTIONAL,
    IN OUT LPWSTR FaceName, // Points to a WCHAR array of LF_FACESIZE elements
    IN ULONG FontFamily,
    IN ULONG FontWeight,
    IN UINT  CodePage);

HFONT
CreateConsoleFont2(
    IN LONG Height,
    IN LONG Width OPTIONAL,
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo);

HFONT
CreateConsoleFont(
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo);

BOOL
GetFontCellSize(
    IN HDC hDC OPTIONAL,
    IN HFONT hFont,
    OUT PUINT Height,
    OUT PUINT Width);

BOOL
IsValidConsoleFont2(
    IN PLOGFONTW lplf,
    IN PNEWTEXTMETRICW lpntm,
    IN DWORD FontType,
    IN UINT CodePage);

BOOL
IsValidConsoleFont(
    IN LPCWSTR FaceName,
    IN UINT CodePage);

/* EOF */
