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

#define INVALID_CP  ((UINT)-1)

#define CP_SHIFTJIS 932  // Japanese Shift-JIS
#define CP_HANGUL   949  // Korean Hangul/Wansung
#define CP_JOHAB    1361 // Korean Johab
#define CP_GB2312   936  // Chinese Simplified (GB2312)
#define CP_BIG5     950  // Chinese Traditional (Big5)

/* IsFarEastCP(CodePage) */
#define IsCJKCodePage(CodePage) \
    ((CodePage) == CP_SHIFTJIS || (CodePage) == CP_HANGUL || \
  /* (CodePage) == CP_JOHAB || */ \
     (CodePage) == CP_BIG5     || (CodePage) == CP_GB2312)

#if !defined(_WINGDI_) || defined(NOGDI)
#define SHIFTJIS_CHARSET    128
#define HANGEUL_CHARSET     129
#define HANGUL_CHARSET      129 // HANGEUL_CHARSET
#if(WINVER >= 0x0400)
#define JOHAB_CHARSET       130
#endif /* WINVER */
#define GB2312_CHARSET      134
#define CHINESEBIG5_CHARSET 136
#endif /* !defined(_WINGDI_) || defined(NOGDI) */

/* IsAnyDBCSCharSet(CharSet) */
#define IsCJKCharSet(CharSet)   \
    ((CharSet) == SHIFTJIS_CHARSET || (CharSet) == HANGUL_CHARSET || \
  /* (CharSet) == JOHAB_CHARSET || */ \
     (CharSet) == GB2312_CHARSET   || (CharSet) == CHINESEBIG5_CHARSET)

#define IsBoldFont(Weight)  \
    ((Weight) >= FW_SEMIBOLD) /* Sometimes, just > FW_MEDIUM */

typedef struct _TT_FONT_ENTRY
{
    LIST_ENTRY Entry;
    UINT CodePage;
    BOOL DisableBold;
    WCHAR FaceName[LF_FACESIZE];
    WCHAR FaceNameAlt[LF_FACESIZE];
} TT_FONT_ENTRY, *PTT_FONT_ENTRY;


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

/*
 * To install additional TrueType fonts to be available for the console,
 * add entries of type REG_SZ named "0", "00" etc... in:
 * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
 * The names of the fonts listed there should match those in:
 * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts
 *
 * This function initializes the cache of the fonts listed there.
 */
VOID
InitTTFontCache(VOID);

VOID
ClearTTFontCache(VOID);

VOID
RefreshTTFontCache(VOID);

PTT_FONT_ENTRY
FindCachedTTFont(
    IN LPCWSTR FaceName,
    IN UINT CodePage);

#define IsAdditionalTTFont(FaceName) \
    (FindCachedTTFont((FaceName), INVALID_CP) != NULL)

#define IsAdditionalTTFontCP(FaceName, CodePage) \
    (FindCachedTTFont((FaceName), (CodePage)) != NULL)

/* EOF */
