/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/concfg/font.c
 * PURPOSE:         Console Fonts Management
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"
#include <winuser.h>

#include "settings.h"
#include "font.h"
// #include "concfg.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

// RTL_STATIC_LIST_HEAD(TTFontCache);
LIST_ENTRY TTFontCache = {&TTFontCache, &TTFontCache};

/* FUNCTIONS ******************************************************************/

/* Retrieves the character set associated with a given code page */
BYTE
CodePageToCharSet(
    IN UINT CodePage)
{
    CHARSETINFO CharInfo;
    if (TranslateCharsetInfo(UlongToPtr(CodePage), &CharInfo, TCI_SRCCODEPAGE))
        return CharInfo.ciCharset;
    else
        return DEFAULT_CHARSET;
}

HFONT
CreateConsoleFontEx(
    IN LONG Height,
    IN LONG Width OPTIONAL,
    IN OUT LPWSTR FaceName, // Points to a WCHAR array of LF_FACESIZE elements
    IN ULONG FontFamily,
    IN ULONG FontWeight,
    IN UINT  CodePage)
{
    LOGFONTW lf;

    RtlZeroMemory(&lf, sizeof(lf));

    lf.lfHeight = Height;
    lf.lfWidth  = Width;

    lf.lfEscapement  = 0;
    lf.lfOrientation = 0; // TA_BASELINE; // TA_RTLREADING; when the console supports RTL?
    // lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = FALSE;
    lf.lfWeight  = FontWeight;
    lf.lfCharSet = CodePageToCharSet(CodePage);
    lf.lfOutPrecision  = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;

    /* Set the mandatory flags and remove those that we do not support */
    lf.lfPitchAndFamily = (BYTE)( (FIXED_PITCH | FF_MODERN | FontFamily) &
                                 ~(VARIABLE_PITCH | FF_DECORATIVE | FF_ROMAN | FF_SCRIPT | FF_SWISS));

    if (!IsValidConsoleFont(FaceName, CodePage))
        StringCchCopyW(FaceName, LF_FACESIZE, L"Terminal");

    StringCchCopyNW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName),
                    FaceName, LF_FACESIZE);

    return CreateFontIndirectW(&lf);
}

HFONT
CreateConsoleFont2(
    IN LONG Height,
    IN LONG Width OPTIONAL,
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo)
{
    return CreateConsoleFontEx(Height,
                               Width,
                               ConsoleInfo->FaceName,
                               ConsoleInfo->FontFamily,
                               ConsoleInfo->FontWeight,
                               ConsoleInfo->CodePage);
}

HFONT
CreateConsoleFont(
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo)
{
    /*
     * Format:
     * Width  = FontSize.X = LOWORD(FontSize);
     * Height = FontSize.Y = HIWORD(FontSize);
     */
    /* NOTE: FontSize is always in cell height/width units (pixels) */
    return CreateConsoleFontEx((LONG)(ULONG)ConsoleInfo->FontSize.Y,
                               (LONG)(ULONG)ConsoleInfo->FontSize.X,
                               ConsoleInfo->FaceName,
                               ConsoleInfo->FontFamily,
                               ConsoleInfo->FontWeight,
                               ConsoleInfo->CodePage);
}

BOOL
GetFontCellSize(
    IN HDC hDC OPTIONAL,
    IN HFONT hFont,
    OUT PUINT Height,
    OUT PUINT Width)
{
    BOOL Success = FALSE;
    HDC hOrgDC = hDC;
    HFONT hOldFont;
    // LONG LogSize, PointSize;
    LONG CharWidth, CharHeight;
    TEXTMETRICW tm;
    // SIZE CharSize;

    if (!hDC)
        hDC = GetDC(NULL);

    hOldFont = SelectObject(hDC, hFont);
    if (hOldFont == NULL)
    {
        DPRINT1("GetFontCellSize: SelectObject failed\n");
        goto Quit;
    }

/*
 * See also: Display_SetTypeFace in applications/fontview/display.c
 */

    /*
     * Note that the method with GetObjectW just returns
     * the original parameters with which the font was created.
     */
    if (!GetTextMetricsW(hDC, &tm))
    {
        DPRINT1("GetFontCellSize: GetTextMetrics failed\n");
        goto Cleanup;
    }

    CharHeight = tm.tmHeight + tm.tmExternalLeading;

#if 0
    /* Measure real char width more precisely if possible */
    if (GetTextExtentPoint32W(hDC, L"R", 1, &CharSize))
        CharWidth = CharSize.cx;
#else
    CharWidth = tm.tmAveCharWidth; // tm.tmMaxCharWidth;
#endif

#if 0
    /*** Logical to Point size ***/
    LogSize   = tm.tmHeight - tm.tmInternalLeading;
    PointSize = MulDiv(LogSize, 72, GetDeviceCaps(hDC, LOGPIXELSY));
    /*****************************/
#endif

    *Height = (UINT)CharHeight;
    *Width  = (UINT)CharWidth;
    Success = TRUE;

Cleanup:
    SelectObject(hDC, hOldFont);
Quit:
    if (!hOrgDC)
        ReleaseDC(NULL, hDC);

    return Success;
}

BOOL
IsValidConsoleFont2(
    IN PLOGFONTW lplf,
    IN PNEWTEXTMETRICW lpntm,
    IN DWORD FontType,
    IN UINT CodePage)
{
    LPCWSTR FaceName = lplf->lfFaceName;

    /*
     * According to: https://web.archive.org/web/20140901124501/http://support.microsoft.com/kb/247815
     * "Necessary criteria for fonts to be available in a command window",
     * the criteria for console-eligible fonts are as follows:
     * - The font must be a fixed-pitch font.
     * - The font cannot be an italic font.
     * - The font cannot have a negative A or C space.
     * - If it is a TrueType font, it must be FF_MODERN.
     * - If it is not a TrueType font, it must be OEM_CHARSET.
     *
     * Non documented: vertical fonts are forbidden (their name start with a '@').
     *
     * Additional criteria for Asian installations:
     * - If it is not a TrueType font, the face name must be "Terminal".
     * - If it is an Asian TrueType font, it must also be an Asian character set.
     *
     * See also Raymond Chen's blog: https://devblogs.microsoft.com/oldnewthing/?p=26843
     * and MIT-licensed Microsoft Terminal source code: https://github.com/microsoft/Terminal/blob/master/src/propsheet/misc.cpp
     * for other details.
     *
     * To install additional TrueType fonts to be available for the console,
     * add entries of type REG_SZ named "0", "00" etc... in:
     * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
     * The names of the fonts listed there should match those in:
     * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts
     */

    /*
     * In ReactOS we relax some of the criteria:
     * - We allow fixed-pitch FF_MODERN (Monospace) TrueType fonts
     *   that can be italic or have negative A or C space.
     * - If it is not a TrueType font, it can be from another character set
     *   than OEM_CHARSET. When an Asian codepage is active however, we require
     *   that this non-TrueType font has an Asian character set.
     */

    /* Reject variable-width fonts ... */
    if ( ( ((lplf->lfPitchAndFamily & 0x03) != FIXED_PITCH)
#if 0 /* Reject italic and TrueType fonts with negative A or C space ... */
           || (lplf->lfItalic)
           || !(lpntm->ntmFlags & NTM_NONNEGATIVE_AC)
#endif
         ) &&
        /* ... if they are not in the list of additional TrueType fonts to include */
         !IsAdditionalTTFont(FaceName) )
    {
        DPRINT1("Font '%S' rejected because it%s (lfPitchAndFamily = %d)\n",
                FaceName,
                !(lplf->lfPitchAndFamily & FIXED_PITCH) ? "'s not FIXED_PITCH"
                    : (!(lpntm->ntmFlags & NTM_NONNEGATIVE_AC) ? " has negative A or C space"
                                                               : " is broken"),
                lplf->lfPitchAndFamily);
        return FALSE;
    }

    /* Reject TrueType fonts that are not FF_MODERN */
    if ((FontType == TRUETYPE_FONTTYPE) && ((lplf->lfPitchAndFamily & 0xF0) != FF_MODERN))
    {
        DPRINT1("TrueType font '%S' rejected because it's not FF_MODERN (lfPitchAndFamily = %d)\n",
                FaceName, lplf->lfPitchAndFamily);
        return FALSE;
    }

    /* Reject vertical fonts (tategaki) */
    if (FaceName[0] == L'@')
    {
        DPRINT1("Font '%S' rejected because it's vertical\n", FaceName);
        return FALSE;
    }

    /* Is the current code page Chinese, Japanese or Korean? */
    if (IsCJKCodePage(CodePage))
    {
        /* It's CJK */

        if (FontType == TRUETYPE_FONTTYPE)
        {
            /*
             * Here we are inclusive and check for any CJK character set,
             * instead of looking just at the current one via CodePageToCharSet().
             */
            if (!IsCJKCharSet(lplf->lfCharSet)
#if 1 // FIXME: Temporary HACK!
                && wcscmp(FaceName, L"Terminal") != 0
#endif
               )
            {
                DPRINT1("TrueType font '%S' rejected because it's not Asian charset (lfCharSet = %d)\n",
                        FaceName, lplf->lfCharSet);
                return FALSE;
            }

            /*
             * If this is a cached TrueType font that is used only for certain
             * code pages, verify that the charset it claims is the correct one.
             *
             * Since there may be multiple entries for a cached TrueType font,
             * a general one (code page == 0) and one or more for explicit
             * code pages, we need to perform two search queries instead of
             * just one and retrieving the code page for this entry.
             */
            if (IsAdditionalTTFont(FaceName) && !IsAdditionalTTFontCP(FaceName, 0) &&
                !IsCJKCharSet(lplf->lfCharSet))
            {
                DPRINT1("Cached TrueType font '%S' rejected because it claims a code page that is not Asian charset (lfCharSet = %d)\n",
                        FaceName, lplf->lfCharSet);
                return FALSE;
            }
        }
        else
        {
            /* Reject non-TrueType fonts that do not have an Asian character set */
            if (!IsCJKCharSet(lplf->lfCharSet) && (lplf->lfCharSet != OEM_CHARSET))
            {
                DPRINT1("Non-TrueType font '%S' rejected because it's not Asian charset or OEM_CHARSET (lfCharSet = %d)\n",
                        FaceName, lplf->lfCharSet);
                return FALSE;
            }

            /* Reject non-TrueType fonts that are not Terminal */
            if (wcscmp(FaceName, L"Terminal") != 0)
            {
                DPRINT1("Non-TrueType font '%S' rejected because it's not 'Terminal'\n", FaceName);
                return FALSE;
            }
        }
    }
    else
    {
        /* Not CJK */

        /* Reject non-TrueType fonts that are not OEM or similar */
        if ((FontType != TRUETYPE_FONTTYPE) &&
            (lplf->lfCharSet != ANSI_CHARSET) &&
            (lplf->lfCharSet != DEFAULT_CHARSET) &&
            (lplf->lfCharSet != OEM_CHARSET))
        {
            DPRINT1("Non-TrueType font '%S' rejected because it's not ANSI_CHARSET or DEFAULT_CHARSET or OEM_CHARSET (lfCharSet = %d)\n",
                    FaceName, lplf->lfCharSet);
            return FALSE;
        }
    }

    /* All good */
    return TRUE;
}

typedef struct _IS_VALID_CONSOLE_FONT_PARAM
{
    BOOL IsValidFont;
    UINT CodePage;
} IS_VALID_CONSOLE_FONT_PARAM, *PIS_VALID_CONSOLE_FONT_PARAM;

static BOOL CALLBACK
IsValidConsoleFontProc(
    IN PLOGFONTW lplf,
    IN PNEWTEXTMETRICW lpntm,
    IN DWORD  FontType,
    IN LPARAM lParam)
{
    PIS_VALID_CONSOLE_FONT_PARAM Param = (PIS_VALID_CONSOLE_FONT_PARAM)lParam;
    Param->IsValidFont = IsValidConsoleFont2(lplf, lpntm, FontType, Param->CodePage);

    /* Stop the enumeration now */
    return FALSE;
}

BOOL
IsValidConsoleFont(
    IN LPCWSTR FaceName,
    IN UINT CodePage)
{
    IS_VALID_CONSOLE_FONT_PARAM Param;
    HDC hDC;
    LOGFONTW lf;

    Param.IsValidFont = FALSE;
    Param.CodePage = CodePage;

    RtlZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET; // CodePageToCharSet(CodePage);
    // lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    StringCchCopyW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), FaceName);

    hDC = GetDC(NULL);
    EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)IsValidConsoleFontProc, (LPARAM)&Param, 0);
    ReleaseDC(NULL, hDC);

    return Param.IsValidFont;
}

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
InitTTFontCache(VOID)
{
    BOOLEAN Success;
    HKEY  hKeyTTFonts; // hKey;
    DWORD dwNumValues = 0;
    DWORD dwIndex;
    DWORD dwType;
    WCHAR szValueName[MAX_PATH];
    DWORD dwValueName;
    WCHAR szValue[LF_FACESIZE] = L"";
    DWORD dwValue;
    PTT_FONT_ENTRY FontEntry;
    PWCHAR pszNext = NULL;
    UINT CodePage;

    if (!IsListEmpty(&TTFontCache))
        return;
    // InitializeListHead(&TTFontCache);

    /* Open the key */
    // "\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont"
    Success = (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                             L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
                             0,
                             KEY_READ,
                             &hKeyTTFonts) == ERROR_SUCCESS);
    if (!Success)
        return;

    /* Enumerate each value */
    if (RegQueryInfoKeyW(hKeyTTFonts, NULL, NULL, NULL, NULL, NULL, NULL,
                         &dwNumValues, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        DPRINT("ConCfgReadUserSettings: RegQueryInfoKeyW failed\n");
        RegCloseKey(hKeyTTFonts);
        return;
    }

    for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
    {
        dwValue = sizeof(szValue);
        dwValueName = ARRAYSIZE(szValueName);
        if (RegEnumValueW(hKeyTTFonts, dwIndex, szValueName, &dwValueName, NULL, &dwType, (BYTE*)szValue, &dwValue) != ERROR_SUCCESS)
        {
            DPRINT1("InitTTFontCache: RegEnumValueW failed, continuing...\n");
            continue;
        }
        /* Only (multi-)string values are supported */
        if ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))
            continue;

        /* The value name is a code page (in decimal), validate it */
        CodePage = wcstoul(szValueName, &pszNext, 10);
        if (*pszNext)
            continue; // Non-numerical garbage followed...
        // IsValidCodePage(CodePage);

        FontEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*FontEntry));
        if (!FontEntry)
        {
            DPRINT1("InitTTFontCache: Failed to allocate memory, continuing...\n");
            continue;
        }

        FontEntry->CodePage = CodePage;

        pszNext = szValue;

        /* Check whether bold is disabled for this font */
        if (*pszNext == L'*')
        {
            FontEntry->DisableBold = TRUE;
            ++pszNext;
        }
        else
        {
            FontEntry->DisableBold = FALSE;
        }

        /* Copy the font name */
        StringCchCopyNW(FontEntry->FaceName, ARRAYSIZE(FontEntry->FaceName),
                        pszNext, wcslen(pszNext));

        if (dwType == REG_MULTI_SZ)
        {
            /* There may be an alternate face name as the second string */
            pszNext += wcslen(pszNext) + 1;

            /* Check whether bold is disabled for this font */
            if (*pszNext == L'*')
            {
                FontEntry->DisableBold = TRUE;
                ++pszNext;
            }
            // else, keep the original setting.

            /* Copy the alternate font name */
            StringCchCopyNW(FontEntry->FaceNameAlt, ARRAYSIZE(FontEntry->FaceNameAlt),
                            pszNext, wcslen(pszNext));
        }

        InsertTailList(&TTFontCache, &FontEntry->Entry);
    }

    /* Close the key and quit */
    RegCloseKey(hKeyTTFonts);
}

VOID
ClearTTFontCache(VOID)
{
    PLIST_ENTRY Entry;
    PTT_FONT_ENTRY FontEntry;

    while (!IsListEmpty(&TTFontCache))
    {
        Entry = RemoveHeadList(&TTFontCache);
        FontEntry = CONTAINING_RECORD(Entry, TT_FONT_ENTRY, Entry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FontEntry);
    }
    InitializeListHead(&TTFontCache);
}

VOID
RefreshTTFontCache(VOID)
{
    ClearTTFontCache();
    InitTTFontCache();
}

PTT_FONT_ENTRY
FindCachedTTFont(
    IN LPCWSTR FaceName,
    IN UINT CodePage)
{
    PLIST_ENTRY Entry;
    PTT_FONT_ENTRY FontEntry;

    /* Search for the font in the cache */
    for (Entry = TTFontCache.Flink;
         Entry != &TTFontCache;
         Entry = Entry->Flink)
    {
        FontEntry = CONTAINING_RECORD(Entry, TT_FONT_ENTRY, Entry);

        /* NOTE: The font face names are case-sensitive */
        if ((wcscmp(FontEntry->FaceName   , FaceName) == 0) ||
            (wcscmp(FontEntry->FaceNameAlt, FaceName) == 0))
        {
            /* Return a match if we don't look at the code pages, or when they match */
            if ((CodePage == INVALID_CP) || (CodePage == FontEntry->CodePage))
            {
                return FontEntry;
            }
        }
    }
    return NULL;
}

/* EOF */
