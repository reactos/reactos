/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            win32ss/gdi/gdi32/objects/font.c
 * PURPOSE:
 * PROGRAMMER:
 *
 */

#include <precomp.h>

#include <math.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static __inline INT GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

/*
 *  For TranslateCharsetInfo
 */
#define MAXTCIINDEX 32
static const CHARSETINFO FONT_tci[MAXTCIINDEX] =
{
    /* ANSI */
    { ANSI_CHARSET, 1252, {{0,0,0,0},{FS_LATIN1,0}} },
    { EASTEUROPE_CHARSET, 1250, {{0,0,0,0},{FS_LATIN2,0}} },
    { RUSSIAN_CHARSET, 1251, {{0,0,0,0},{FS_CYRILLIC,0}} },
    { GREEK_CHARSET, 1253, {{0,0,0,0},{FS_GREEK,0}} },
    { TURKISH_CHARSET, 1254, {{0,0,0,0},{FS_TURKISH,0}} },
    { HEBREW_CHARSET, 1255, {{0,0,0,0},{FS_HEBREW,0}} },
    { ARABIC_CHARSET, 1256, {{0,0,0,0},{FS_ARABIC,0}} },
    { BALTIC_CHARSET, 1257, {{0,0,0,0},{FS_BALTIC,0}} },
    { VIETNAMESE_CHARSET, 1258, {{0,0,0,0},{FS_VIETNAMESE,0}} },
    /* reserved by ANSI */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* ANSI and OEM */
    { THAI_CHARSET, 874, {{0,0,0,0},{FS_THAI,0}} },
    { SHIFTJIS_CHARSET, 932, {{0,0,0,0},{FS_JISJAPAN,0}} },
    { GB2312_CHARSET, 936, {{0,0,0,0},{FS_CHINESESIMP,0}} },
    { HANGEUL_CHARSET, 949, {{0,0,0,0},{FS_WANSUNG,0}} },
    { CHINESEBIG5_CHARSET, 950, {{0,0,0,0},{FS_CHINESETRAD,0}} },
    { JOHAB_CHARSET, 1361, {{0,0,0,0},{FS_JOHAB,0}} },
    /* reserved for alternate ANSI and OEM */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* reserved for system */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { SYMBOL_CHARSET, CP_SYMBOL, {{0,0,0,0},{FS_SYMBOL,0}} }
};

#define INITIAL_FAMILY_COUNT 64

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
VOID
FASTCALL
FONT_TextMetricWToA(const TEXTMETRICW *ptmW, LPTEXTMETRICA ptmA )
{
    ptmA->tmHeight = ptmW->tmHeight;
    ptmA->tmAscent = ptmW->tmAscent;
    ptmA->tmDescent = ptmW->tmDescent;
    ptmA->tmInternalLeading = ptmW->tmInternalLeading;
    ptmA->tmExternalLeading = ptmW->tmExternalLeading;
    ptmA->tmAveCharWidth = ptmW->tmAveCharWidth;
    ptmA->tmMaxCharWidth = ptmW->tmMaxCharWidth;
    ptmA->tmWeight = ptmW->tmWeight;
    ptmA->tmOverhang = ptmW->tmOverhang;
    ptmA->tmDigitizedAspectX = ptmW->tmDigitizedAspectX;
    ptmA->tmDigitizedAspectY = ptmW->tmDigitizedAspectY;
    ptmA->tmFirstChar = min(ptmW->tmFirstChar, 255);
    if (ptmW->tmCharSet == SYMBOL_CHARSET)
    {
        ptmA->tmFirstChar = 0x1e;
        ptmA->tmLastChar = 0xff;  /* win9x behaviour - we need the OS2 table data to calculate correctly */
    }
    else
    {
        ptmA->tmFirstChar = ptmW->tmDefaultChar - 1;
        ptmA->tmLastChar = min(ptmW->tmLastChar, 0xff);
    }
    ptmA->tmDefaultChar = (CHAR)ptmW->tmDefaultChar;
    ptmA->tmBreakChar = (CHAR)ptmW->tmBreakChar;
    ptmA->tmItalic = ptmW->tmItalic;
    ptmA->tmUnderlined = ptmW->tmUnderlined;
    ptmA->tmStruckOut = ptmW->tmStruckOut;
    ptmA->tmPitchAndFamily = ptmW->tmPitchAndFamily;
    ptmA->tmCharSet = ptmW->tmCharSet;
}

/***********************************************************************
 *           FONT_mbtowc
 *
 * Returns a Unicode translation of str using the charset of the
 * currently selected font in hdc.  If count is -1 then str is assumed
 * to be '\0' terminated, otherwise it contains the number of bytes to
 * convert.  If plenW is non-NULL, on return it will point to the
 * number of WCHARs that have been written.  If pCP is non-NULL, on
 * return it will point to the codepage used in the conversion.  The
 * caller should free the returned LPWSTR from the process heap
 * itself.
 */
static LPWSTR FONT_mbtowc(HDC hdc, LPCSTR str, INT count, INT *plenW, UINT *pCP)
{
    UINT cp = GdiGetCodePage( hdc );
    INT lenW;
    LPWSTR strW;

    if(count == -1) count = strlen(str);
    lenW = MultiByteToWideChar(cp, 0, str, count, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW*sizeof(WCHAR));
    if (!strW)
        return NULL;
    if(!MultiByteToWideChar(cp, 0, str, count, strW, lenW))
    {
        HeapFree(GetProcessHeap(), 0, strW);
        return NULL;
    }
    DPRINT("mapped %s -> %S\n", str, strW);
    if(plenW) *plenW = lenW;
    if(pCP) *pCP = cp;
    return strW;
}

static LPSTR FONT_GetCharsByRangeA(HDC hdc, UINT firstChar, UINT lastChar, PINT pByteLen)
{
    INT i, count = lastChar - firstChar + 1;
    UINT c;
    LPSTR str;

    if (count <= 0)
        return NULL;

    switch (GdiGetCodePage(hdc))
    {
    case 932:
    case 936:
    case 949:
    case 950:
    case 1361:
        if (lastChar > 0xffff)
            return NULL;
        if ((firstChar ^ lastChar) > 0xff)
            return NULL;
        break;
    default:
        if (lastChar > 0xff)
            return NULL;
        break;
    }

    str = HeapAlloc(GetProcessHeap(), 0, count * 2 + 1);
    if (str == NULL)
        return NULL;

    for(i = 0, c = firstChar; c <= lastChar; i++, c++)
    {
        if (c > 0xff)
            str[i++] = (BYTE)(c >> 8);
        str[i] = (BYTE)c;
    }
    str[i] = '\0';

    *pByteLen = i;

    return str;
}

VOID FASTCALL
NewTextMetricW2A(NEWTEXTMETRICA *tma, NEWTEXTMETRICW *tmw)
{
    FONT_TextMetricWToA((TEXTMETRICW *) tmw, (TEXTMETRICA *) tma);
    tma->ntmFlags = tmw->ntmFlags;
    tma->ntmSizeEM = tmw->ntmSizeEM;
    tma->ntmCellHeight = tmw->ntmCellHeight;
    tma->ntmAvgWidth = tmw->ntmAvgWidth;
}

VOID FASTCALL
NewTextMetricExW2A(NEWTEXTMETRICEXA *tma, NEWTEXTMETRICEXW *tmw)
{
    NewTextMetricW2A(&tma->ntmTm, &tmw->ntmTm);
    tma->ntmFontSig = tmw->ntmFontSig;
}

// IntFontFamilyCompareEx's flags
#define IFFCX_CHARSET 1
#define IFFCX_STYLE 2

FORCEINLINE int FASTCALL
IntFontFamilyCompareEx(const FONTFAMILYINFO *ffi1,
                       const FONTFAMILYINFO *ffi2, DWORD dwCompareFlags)
{
    const LOGFONTW *plf1 = &ffi1->EnumLogFontEx.elfLogFont;
    const LOGFONTW *plf2 = &ffi2->EnumLogFontEx.elfLogFont;
    ULONG WeightDiff1, WeightDiff2;
    int cmp = _wcsicmp(plf1->lfFaceName, plf2->lfFaceName);
    if (cmp)
        return cmp;
    if (dwCompareFlags & IFFCX_CHARSET)
    {
        if (plf1->lfCharSet < plf2->lfCharSet)
            return -1;
        if (plf1->lfCharSet > plf2->lfCharSet)
            return 1;
    }
    if (dwCompareFlags & IFFCX_STYLE)
    {
        WeightDiff1 = labs(plf1->lfWeight - FW_NORMAL);
        WeightDiff2 = labs(plf2->lfWeight - FW_NORMAL);
        if (WeightDiff1 < WeightDiff2)
            return -1;
        if (WeightDiff1 > WeightDiff2)
            return 1;
        if (plf1->lfItalic < plf2->lfItalic)
            return -1;
        if (plf1->lfItalic > plf2->lfItalic)
            return 1;
    }
    return 0;
}

static int __cdecl
IntFontFamilyCompare(const void *ffi1, const void *ffi2)
{
    return IntFontFamilyCompareEx(ffi1, ffi2, IFFCX_STYLE | IFFCX_CHARSET);
}

// IntEnumFontFamilies' flags:
#define IEFF_UNICODE 1
#define IEFF_EXTENDED 2

int FASTCALL
IntFontFamilyListUnique(FONTFAMILYINFO *InfoList, INT nCount,
                        const LOGFONTW *plf, DWORD dwFlags)
{
    FONTFAMILYINFO *first, *last, *result;
    DWORD dwCompareFlags = 0;

    if (plf->lfFaceName[0])
        dwCompareFlags |= IFFCX_STYLE;

    if ((dwFlags & IEFF_EXTENDED) && plf->lfCharSet == DEFAULT_CHARSET)
        dwCompareFlags |= IFFCX_CHARSET;

    first = InfoList;
    last = &InfoList[nCount];

    /* std::unique(first, last, IntFontFamilyCompareEx); */
    if (first == last)
        return 0;

    result = first;
    while (++first != last)
    {
        if (IntFontFamilyCompareEx(result, first, dwCompareFlags) != 0)
        {
            *(++result) = *first;
        }
    }
    nCount = (int)(++result - InfoList);

    return nCount;
}

static int FASTCALL
IntEnumFontFamilies(HDC Dc, const LOGFONTW *LogFont, PVOID EnumProc, LPARAM lParam,
                    DWORD dwFlags)
{
    int FontFamilyCount;
    PFONTFAMILYINFO Info;
    int Ret = 1;
    int i;
    ENUMLOGFONTEXA EnumLogFontExA;
    NEWTEXTMETRICEXA NewTextMetricExA;
    LOGFONTW lfW;
    LONG InfoCount;
    ULONG DataSize;
    NTSTATUS Status;

    DataSize = INITIAL_FAMILY_COUNT * sizeof(FONTFAMILYINFO);
    Info = RtlAllocateHeap(GetProcessHeap(), 0, DataSize);
    if (Info == NULL)
    {
        return 1;
    }

    /* Initialize the LOGFONT structure */
    ZeroMemory(&lfW, sizeof(lfW));
    if (!LogFont)
    {
        lfW.lfCharSet = DEFAULT_CHARSET;
    }
    else
    {
        lfW.lfCharSet = LogFont->lfCharSet;
        lfW.lfPitchAndFamily = LogFont->lfPitchAndFamily;
        StringCbCopyW(lfW.lfFaceName, sizeof(lfW.lfFaceName), LogFont->lfFaceName);
    }

    /* Retrieve the font information */
    InfoCount = INITIAL_FAMILY_COUNT;
    FontFamilyCount = NtGdiGetFontFamilyInfo(Dc, &lfW, Info, &InfoCount);
    if (FontFamilyCount < 0)
    {
        RtlFreeHeap(GetProcessHeap(), 0, Info);
        return 1;
    }

    /* Resize the buffer if the buffer is too small */
    if (INITIAL_FAMILY_COUNT < InfoCount)
    {
        RtlFreeHeap(GetProcessHeap(), 0, Info);

        Status = RtlULongMult(InfoCount, sizeof(FONTFAMILYINFO), &DataSize);
        if (!NT_SUCCESS(Status) || DataSize > LONG_MAX)
        {
            DPRINT1("Overflowed.\n");
            return 1;
        }
        Info = RtlAllocateHeap(GetProcessHeap(), 0, DataSize);
        if (Info == NULL)
        {
            return 1;
        }
        FontFamilyCount = NtGdiGetFontFamilyInfo(Dc, &lfW, Info, &InfoCount);
        if (FontFamilyCount < 0 || FontFamilyCount < InfoCount)
        {
            RtlFreeHeap(GetProcessHeap(), 0, Info);
            return 1;
        }
    }

    /* Sort and remove redundant information */
    qsort(Info, FontFamilyCount, sizeof(*Info), IntFontFamilyCompare);
    FontFamilyCount = IntFontFamilyListUnique(Info, FontFamilyCount, &lfW, dwFlags);

    /* call the callback */
    for (i = 0; i < FontFamilyCount; i++)
    {
        if (dwFlags & IEFF_UNICODE)
        {
            Ret = ((FONTENUMPROCW) EnumProc)(
                      (VOID*)&Info[i].EnumLogFontEx,
                      (VOID*)&Info[i].NewTextMetricEx,
                      Info[i].FontType, lParam);
        }
        else
        {
            // Could use EnumLogFontExW2A here?
            LogFontW2A(&EnumLogFontExA.elfLogFont, &Info[i].EnumLogFontEx.elfLogFont);
            WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfFullName, -1,
                                (LPSTR)EnumLogFontExA.elfFullName, LF_FULLFACESIZE, NULL, NULL);
            WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfStyle, -1,
                                (LPSTR)EnumLogFontExA.elfStyle, LF_FACESIZE, NULL, NULL);
            WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfScript, -1,
                                (LPSTR)EnumLogFontExA.elfScript, LF_FACESIZE, NULL, NULL);
            NewTextMetricExW2A(&NewTextMetricExA,
                               &Info[i].NewTextMetricEx);
            Ret = ((FONTENUMPROCA) EnumProc)(
                      (VOID*)&EnumLogFontExA,
                      (VOID*)&NewTextMetricExA,
                      Info[i].FontType, lParam);
        }

        if(Ret == 0)
            break;
    }

    RtlFreeHeap(GetProcessHeap(), 0, Info);

    return Ret;
}

/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesExW(HDC hdc, LPLOGFONTW lpLogfont, FONTENUMPROCW lpEnumFontFamExProc,
                    LPARAM lParam, DWORD dwFlags)
{
    if (lpLogfont)
    {
        DPRINT("EnumFontFamiliesExW(%p, %p(%S, %u, %u), %p, %p, 0x%08lX)\n",
               hdc, lpLogfont, lpLogfont->lfFaceName, lpLogfont->lfCharSet,
               lpLogfont->lfPitchAndFamily, lpEnumFontFamExProc, lParam, dwFlags);
    }
    else
    {
        DPRINT("EnumFontFamiliesExW(%p, NULL, %p, %p, 0x%08lX)\n",
               hdc, lpEnumFontFamExProc, lParam, dwFlags);
    }
    return IntEnumFontFamilies(hdc, lpLogfont, lpEnumFontFamExProc, lParam,
                               IEFF_UNICODE | IEFF_EXTENDED);
}


/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesW(HDC hdc, LPCWSTR lpszFamily, FONTENUMPROCW lpEnumFontFamProc,
                  LPARAM lParam)
{
    LOGFONTW LogFont;

    DPRINT("EnumFontFamiliesW(%p, %S, %p, %p)\n",
           hdc, lpszFamily, lpEnumFontFamProc, lParam);

    ZeroMemory(&LogFont, sizeof(LOGFONTW));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    if (NULL != lpszFamily)
    {
        if (!*lpszFamily) return 1;
        lstrcpynW(LogFont.lfFaceName, lpszFamily, LF_FACESIZE);
    }

    return IntEnumFontFamilies(hdc, &LogFont, lpEnumFontFamProc, lParam, IEFF_UNICODE);
}


/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesExA (HDC hdc, LPLOGFONTA lpLogfont, FONTENUMPROCA lpEnumFontFamExProc,
                     LPARAM lParam, DWORD dwFlags)
{
    LOGFONTW LogFontW, *pLogFontW;

    if (lpLogfont)
    {
        DPRINT("EnumFontFamiliesExA(%p, %p(%s, %u, %u), %p, %p, 0x%08lX)\n",
               hdc, lpLogfont, lpLogfont->lfFaceName, lpLogfont->lfCharSet,
               lpLogfont->lfPitchAndFamily, lpEnumFontFamExProc, lParam, dwFlags);
    }
    else
    {
        DPRINT("EnumFontFamiliesExA(%p, NULL, %p, %p, 0x%08lX)\n",
               hdc, lpEnumFontFamExProc, lParam, dwFlags);
    }

    if (lpLogfont)
    {
        LogFontA2W(&LogFontW,lpLogfont);
        pLogFontW = &LogFontW;
    }
    else pLogFontW = NULL;

    /* no need to convert LogFontW back to lpLogFont b/c it's an [in] parameter only */
    return IntEnumFontFamilies(hdc, pLogFontW, lpEnumFontFamExProc, lParam, IEFF_EXTENDED);
}


/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesA(HDC hdc, LPCSTR lpszFamily, FONTENUMPROCA lpEnumFontFamProc,
                  LPARAM lParam)
{
    LOGFONTW LogFont;

    DPRINT("EnumFontFamiliesA(%p, %s, %p, %p)\n",
           hdc, lpszFamily, lpEnumFontFamProc, lParam);

    ZeroMemory(&LogFont, sizeof(LOGFONTW));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    if (NULL != lpszFamily)
    {
        if (!*lpszFamily) return 1;
        MultiByteToWideChar(CP_THREAD_ACP, 0, lpszFamily, -1, LogFont.lfFaceName, LF_FACESIZE);
    }

    return IntEnumFontFamilies(hdc, &LogFont, lpEnumFontFamProc, lParam, 0);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetCharacterPlacementA(
    HDC hdc,
    LPCSTR lpString,
    INT uCount,
    INT nMaxExtent,
    GCP_RESULTSA *lpResults,
    DWORD dwFlags)
{
    WCHAR *lpStringW;
    INT uCountW;
    GCP_RESULTSW resultsW;
    DWORD ret;
    UINT font_cp;

    if ( !lpString || uCount <= 0 || !lpResults || (nMaxExtent < 0 && nMaxExtent != -1 ) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /*    TRACE("%s, %d, %d, 0x%08x\n",
              debugstr_an(lpString, uCount), uCount, nMaxExtent, dwFlags);
    */
    /* both structs are equal in size */
    memcpy(&resultsW, lpResults, sizeof(resultsW));

    lpStringW = FONT_mbtowc(hdc, lpString, uCount, &uCountW, &font_cp);
    if (lpStringW == NULL)
    {
        return 0;
    }
    if(lpResults->lpOutString)
    {
        resultsW.lpOutString = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*uCountW);
        if (resultsW.lpOutString == NULL)
        {
            HeapFree(GetProcessHeap(), 0, lpStringW);
            return 0;
        }
    }

    ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, &resultsW, dwFlags);

    lpResults->nGlyphs = resultsW.nGlyphs;
    lpResults->nMaxFit = resultsW.nMaxFit;

    if(lpResults->lpOutString)
    {
        WideCharToMultiByte(font_cp, 0, resultsW.lpOutString, uCountW,
                            lpResults->lpOutString, uCount, NULL, NULL );
    }

    HeapFree(GetProcessHeap(), 0, lpStringW);
    HeapFree(GetProcessHeap(), 0, resultsW.lpOutString);

    return ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetCharacterPlacementW(
    HDC hdc,
    LPCWSTR lpString,
    INT uCount,
    INT nMaxExtent,
    GCP_RESULTSW *lpResults,
    DWORD dwFlags
)
{
    DWORD ret=0;
    SIZE size;
    UINT i, nSet;
    DPRINT("GetCharacterPlacementW\n");

    if (dwFlags&(~GCP_REORDER)) DPRINT("flags 0x%08lx ignored\n", dwFlags);
    if (lpResults->lpClass) DPRINT("classes not implemented\n");

    nSet = (UINT)uCount;
    if (nSet > lpResults->nGlyphs)
        nSet = lpResults->nGlyphs;

    /* return number of initialized fields */
    lpResults->nGlyphs = nSet;

    if (dwFlags & GCP_REORDER)
    {
        if (LoadLPK(LPK_GCP))
            return LpkGetCharacterPlacement(hdc, lpString, uCount, nMaxExtent, lpResults, dwFlags, 0);
    }

    /* Treat the case where no special handling was requested in a fastpath way */
    /* copy will do if the GCP_REORDER flag is not set */
    if (lpResults->lpOutString)
        lstrcpynW( lpResults->lpOutString, lpString, nSet );

    if (lpResults->lpOrder)
    {
        for (i = 0; i < nSet; i++)
            lpResults->lpOrder[i] = i;
    }

    /* FIXME: Will use the placement chars */
    if (lpResults->lpDx)
    {
        int c;
        for (i = 0; i < nSet; i++)
        {
            if (GetCharWidth32W(hdc, lpString[i], lpString[i], &c))
                lpResults->lpDx[i]= c;
        }
    }

    if (lpResults->lpCaretPos && !(dwFlags & GCP_REORDER))
    {
        int pos = 0;

        lpResults->lpCaretPos[0] = 0;
        for (i = 1; i < nSet; i++)
            if (GetTextExtentPoint32W(hdc, &(lpString[i - 1]), 1, &size))
                lpResults->lpCaretPos[i] = (pos += size.cx);
    }

    if (lpResults->lpGlyphs)
        NtGdiGetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);

    if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
        ret = MAKELONG(size.cx, size.cy);

    return ret;
}

DWORD
WINAPI
NewGetCharacterPlacementW(
    HDC hdc,
    LPCWSTR lpString,
    INT uCount,
    INT nMaxExtent,
    GCP_RESULTSW *lpResults,
    DWORD dwFlags
)
{
    ULONG nSet;
    SIZE Size = {0,0};

    if ( !lpString || uCount <= 0 || (nMaxExtent < 0 && nMaxExtent != -1 ) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if ( !lpResults )
    {
        if ( GetTextExtentPointW(hdc, lpString, uCount, &Size) )
        {
            return MAKELONG(Size.cx, Size.cy);
        }
        return 0;
    }

    nSet = uCount;
    if ( nSet > lpResults->nGlyphs )
        nSet = lpResults->nGlyphs;

    return NtGdiGetCharacterPlacementW( hdc,
                                        (LPWSTR)lpString,
                                        nSet,
                                        nMaxExtent,
                                        lpResults,
                                        dwFlags);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharABCWidthsFloatW(HDC hdc,
                       UINT FirstChar,
                       UINT LastChar,
                       LPABCFLOAT abcF)
{
    DPRINT("GetCharABCWidthsFloatW\n");
    if ((!abcF) || (FirstChar > LastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharABCWidthsW( hdc,
                                   FirstChar,
                                   (ULONG)(LastChar - FirstChar + 1),
                                   (PWCHAR) NULL,
                                   0,
                                   (PVOID)abcF);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharWidthFloatW(HDC hdc,
                   UINT iFirstChar,
                   UINT iLastChar,
                   PFLOAT pxBuffer)
{
    DPRINT("GetCharWidthsFloatW\n");
    if ((!pxBuffer) || (iFirstChar > iLastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharWidthW( hdc,
                               iFirstChar,
                               (ULONG)(iLastChar - iFirstChar + 1),
                               (PWCHAR) NULL,
                               0,
                               (PVOID) pxBuffer);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharWidthW(HDC hdc,
              UINT iFirstChar,
              UINT iLastChar,
              LPINT lpBuffer)
{
    DPRINT("GetCharWidthsW\n");
    if ((!lpBuffer) || (iFirstChar > iLastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharWidthW( hdc,
                               iFirstChar,
                               (ULONG)(iLastChar - iFirstChar + 1),
                               (PWCHAR) NULL,
                               GCW_NOFLOAT,
                               (PVOID) lpBuffer);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharWidth32W(HDC hdc,
                UINT iFirstChar,
                UINT iLastChar,
                LPINT lpBuffer)
{
    DPRINT("GetCharWidths32W\n");
    if ((!lpBuffer) || (iFirstChar > iLastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharWidthW( hdc,
                               iFirstChar,
                               (ULONG)(iLastChar - iFirstChar + 1),
                               (PWCHAR) NULL,
                               GCW_NOFLOAT|GCW_WIN32,
                               (PVOID) lpBuffer);
}


/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharABCWidthsW(HDC hdc,
                  UINT FirstChar,
                  UINT LastChar,
                  LPABC lpabc)
{
    DPRINT("GetCharABCWidthsW\n");
    if ((!lpabc) || (FirstChar > LastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharABCWidthsW( hdc,
                                   FirstChar,
                                   (ULONG)(LastChar - FirstChar + 1),
                                   (PWCHAR) NULL,
                                   GCABCW_NOFLOAT,
                                   (PVOID)lpabc);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharWidthA(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    LPINT	lpBuffer
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharWidthsA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

    ret = NtGdiGetCharWidthW( hdc,
                              wstr[0],
                              (ULONG) count,
                              (PWCHAR) wstr,
                              GCW_NOFLOAT,
                              (PVOID) lpBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharWidth32A(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    LPINT	lpBuffer
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharWidths32A\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

    ret = NtGdiGetCharWidthW( hdc,
                              wstr[0],
                              (ULONG) count,
                              (PWCHAR) wstr,
                              GCW_NOFLOAT|GCW_WIN32,
                              (PVOID) lpBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharWidthFloatA(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    PFLOAT	pxBuffer
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharWidthsFloatA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }
    ret = NtGdiGetCharWidthW( hdc, wstr[0], (ULONG) count, (PWCHAR) wstr, 0, (PVOID) pxBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsA(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    LPABC	lpabc
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharABCWidthsA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

    ret = NtGdiGetCharABCWidthsW( hdc,
                                  wstr[0],
                                  wlen - 1,
                                  (PWCHAR)wstr,
                                  GCABCW_NOFLOAT,
                                  (PVOID)lpabc);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsFloatA(
    HDC		hdc,
    UINT		iFirstChar,
    UINT		iLastChar,
    LPABCFLOAT	lpABCF
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharABCWidthsFloatA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc( hdc, str, count+1, &wlen, NULL );
    if (!wstr)
    {
        HeapFree( GetProcessHeap(), 0, str );
        return FALSE;
    }
    ret = NtGdiGetCharABCWidthsW( hdc,wstr[0],(ULONG)count, (PWCHAR)wstr, 0, (PVOID)lpABCF);

    HeapFree( GetProcessHeap(), 0, str );
    HeapFree( GetProcessHeap(), 0, wstr );

    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharABCWidthsI(HDC hdc,
                  UINT giFirst,
                  UINT cgi,
                  LPWORD pgi,
                  LPABC lpabc)
{
    DPRINT("GetCharABCWidthsI\n");
    return NtGdiGetCharABCWidthsW( hdc,
                                   giFirst,
                                   (ULONG) cgi,
                                   (PWCHAR) pgi,
                                   GCABCW_NOFLOAT|GCABCW_INDICES,
                                   (PVOID)lpabc);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharWidthI(HDC hdc,
              UINT giFirst,
              UINT cgi,
              LPWORD pgi,
              LPINT lpBuffer
             )
{
    DPRINT("GetCharWidthsI\n");
    if (!lpBuffer || (!pgi && (giFirst == MAXUSHORT))) // Cannot be at max.
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!cgi) return TRUE;
    return NtGdiGetCharWidthW( hdc,
                               giFirst,
                               cgi,
                               (PWCHAR) pgi,
                               GCW_INDICES|GCW_NOFLOAT|GCW_WIN32,
                               (PVOID) lpBuffer );
}

/*
 * @implemented
 */
DWORD
WINAPI
GetFontLanguageInfo(
    HDC 	hDc
)
{
    DWORD Gcp = 0, Ret = 0;
    if (gbLpk)
    {
        Ret = NtGdiGetTextCharsetInfo(hDc, NULL, 0);
        if ((Ret == ARABIC_CHARSET) || (Ret == HEBREW_CHARSET))
            Ret = (GCP_KASHIDA|GCP_DIACRITIC|GCP_LIGATE|GCP_GLYPHSHAPE|GCP_REORDER);
    }
    Gcp = GetDCDWord(hDc, GdiGetFontLanguageInfo, GCP_ERROR);
    if ( Gcp == GCP_ERROR)
        return Gcp;
    else
        Ret = Gcp | Ret;
    return Ret;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetGlyphIndicesA(
    HDC hdc,
    LPCSTR lpstr,
    INT count,
    LPWORD pgi,
    DWORD flags
)
{
    DWORD Ret;
    WCHAR *lpstrW;
    INT countW;

    lpstrW = FONT_mbtowc(hdc, lpstr, count, &countW, NULL);

    if (lpstrW == NULL)
        return GDI_ERROR;

    Ret = NtGdiGetGlyphIndicesW(hdc, lpstrW, countW, pgi, flags);
    HeapFree(GetProcessHeap(), 0, lpstrW);
    return Ret;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetGlyphOutlineA(
    HDC		hdc,
    UINT		uChar,
    UINT		uFormat,
    LPGLYPHMETRICS	lpgm,
    DWORD		cbBuffer,
    LPVOID		lpvBuffer,
    CONST MAT2	*lpmat2
)
{

    LPWSTR p = NULL;
    DWORD ret;
    UINT c;
    DPRINT("GetGlyphOutlineA uChar %x\n", uChar);
    if (!lpgm || !lpmat2) return GDI_ERROR;
    if(!(uFormat & GGO_GLYPH_INDEX))
    {
        int len;
        char mbchs[2];
        if(uChar > 0xff)   /* but, 2 bytes character only */
        {
            len = 2;
            mbchs[0] = (uChar & 0xff00) >> 8;
            mbchs[1] = (uChar & 0xff);
        }
        else
        {
            len = 1;
            mbchs[0] = (uChar & 0xff);
        }
        p = FONT_mbtowc(hdc, mbchs, len, NULL, NULL);
        if(!p)
            return GDI_ERROR;
        c = p[0];
    }
    else
        c = uChar;
    ret = NtGdiGetGlyphOutline(hdc, c, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
    HeapFree(GetProcessHeap(), 0, p);
    return ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetGlyphOutlineW(
    HDC		hdc,
    UINT		uChar,
    UINT		uFormat,
    LPGLYPHMETRICS	lpgm,
    DWORD		cbBuffer,
    LPVOID		lpvBuffer,
    CONST MAT2	*lpmat2
)
{
    DPRINT("GetGlyphOutlineW uChar %x\n", uChar);
    if (!lpgm || !lpmat2) return GDI_ERROR;
    if (!lpvBuffer) cbBuffer = 0;
    return NtGdiGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetGlyphOutlineWow(
    DWORD	a0,
    DWORD	a1,
    DWORD	a2,
    DWORD	a3,
    DWORD	a4,
    DWORD	a5,
    DWORD	a6
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
UINT
APIENTRY
GetOutlineTextMetricsA(
    HDC			hdc,
    UINT			cbData,
    LPOUTLINETEXTMETRICA	lpOTM
)
{
    char buf[512], *ptr;
    UINT ret, needed;
    OUTLINETEXTMETRICW *lpOTMW = (OUTLINETEXTMETRICW *)buf;
    OUTLINETEXTMETRICA *output = lpOTM;
    INT left, len;

    if((ret = GetOutlineTextMetricsW(hdc, 0, NULL)) == 0)
        return 0;
    if(ret > sizeof(buf))
    {
        lpOTMW = HeapAlloc(GetProcessHeap(), 0, ret);
        if (lpOTMW == NULL)
        {
            return 0;
        }
    }
    GetOutlineTextMetricsW(hdc, ret, lpOTMW);

    needed = sizeof(OUTLINETEXTMETRICA);
    if(lpOTMW->otmpFamilyName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpFamilyName), -1,
                                      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFaceName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpFaceName), -1,
                                      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpStyleName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpStyleName), -1,
                                      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFullName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpFullName), -1,
                                      NULL, 0, NULL, NULL);

    if(!lpOTM)
    {
        ret = needed;
        goto end;
    }

    DPRINT("needed = %u\n", needed);
    if(needed > cbData)
    {
        /* Since the supplied buffer isn't big enough, we'll alloc one
           that is and memcpy the first cbData bytes into the lpOTM at
           the end. */
        output = HeapAlloc(GetProcessHeap(), 0, needed);
        if (output == NULL)
        {
            goto end;
        }
    }

    ret = output->otmSize = min(needed, cbData);
    FONT_TextMetricWToA( &lpOTMW->otmTextMetrics, &output->otmTextMetrics );
    output->otmFiller = 0;
    output->otmPanoseNumber = lpOTMW->otmPanoseNumber;
    output->otmfsSelection = lpOTMW->otmfsSelection;
    output->otmfsType = lpOTMW->otmfsType;
    output->otmsCharSlopeRise = lpOTMW->otmsCharSlopeRise;
    output->otmsCharSlopeRun = lpOTMW->otmsCharSlopeRun;
    output->otmItalicAngle = lpOTMW->otmItalicAngle;
    output->otmEMSquare = lpOTMW->otmEMSquare;
    output->otmAscent = lpOTMW->otmAscent;
    output->otmDescent = lpOTMW->otmDescent;
    output->otmLineGap = lpOTMW->otmLineGap;
    output->otmsCapEmHeight = lpOTMW->otmsCapEmHeight;
    output->otmsXHeight = lpOTMW->otmsXHeight;
    output->otmrcFontBox = lpOTMW->otmrcFontBox;
    output->otmMacAscent = lpOTMW->otmMacAscent;
    output->otmMacDescent = lpOTMW->otmMacDescent;
    output->otmMacLineGap = lpOTMW->otmMacLineGap;
    output->otmusMinimumPPEM = lpOTMW->otmusMinimumPPEM;
    output->otmptSubscriptSize = lpOTMW->otmptSubscriptSize;
    output->otmptSubscriptOffset = lpOTMW->otmptSubscriptOffset;
    output->otmptSuperscriptSize = lpOTMW->otmptSuperscriptSize;
    output->otmptSuperscriptOffset = lpOTMW->otmptSuperscriptOffset;
    output->otmsStrikeoutSize = lpOTMW->otmsStrikeoutSize;
    output->otmsStrikeoutPosition = lpOTMW->otmsStrikeoutPosition;
    output->otmsUnderscoreSize = lpOTMW->otmsUnderscoreSize;
    output->otmsUnderscorePosition = lpOTMW->otmsUnderscorePosition;


    ptr = (char*)(output + 1);
    left = needed - sizeof(*output);

    if(lpOTMW->otmpFamilyName)
    {
        output->otmpFamilyName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpFamilyName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
        ptr += len;
    }
    else
        output->otmpFamilyName = 0;

    if(lpOTMW->otmpFaceName)
    {
        output->otmpFaceName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpFaceName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
        ptr += len;
    }
    else
        output->otmpFaceName = 0;

    if(lpOTMW->otmpStyleName)
    {
        output->otmpStyleName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpStyleName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
        ptr += len;
    }
    else
        output->otmpStyleName = 0;

    if(lpOTMW->otmpFullName)
    {
        output->otmpFullName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (intptr_t)lpOTMW->otmpFullName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
    }
    else
        output->otmpFullName = 0;

    ASSERT(left == 0);

    if(output != lpOTM)
    {
        memcpy(lpOTM, output, cbData);
        HeapFree(GetProcessHeap(), 0, output);

        /* check if the string offsets really fit into the provided size */
        /* FIXME: should we check string length as well? */
        if ((UINT_PTR)lpOTM->otmpFamilyName >= lpOTM->otmSize)
            lpOTM->otmpFamilyName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpFaceName >= lpOTM->otmSize)
            lpOTM->otmpFaceName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpStyleName >= lpOTM->otmSize)
            lpOTM->otmpStyleName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpFullName >= lpOTM->otmSize)
            lpOTM->otmpFullName = 0; /* doesn't fit */
    }

end:
    if(lpOTMW != (OUTLINETEXTMETRICW *)buf)
        HeapFree(GetProcessHeap(), 0, lpOTMW);

    return ret;
}

/* Performs a device to world transformation on the specified size (which
 * is in integer format).
 */
static inline INT INTERNAL_YDSTOWS(XFORM *xForm, INT height)
{
    double floatHeight;

    /* Perform operation with floating point */
    floatHeight = (double)height * xForm->eM22;
    /* Round to integers */
    return GDI_ROUND(floatHeight);
}

/* scale width and height but don't mirror them */
static inline INT width_to_LP( XFORM *xForm, INT width )
{
    return GDI_ROUND( (double)width * fabs( xForm->eM11));
}

static inline INT height_to_LP( XFORM *xForm, INT height )
{
    return GDI_ROUND( (double)height * fabs( xForm->eM22 ));
}

/*
 * @implemented
 */
UINT
APIENTRY
GetOutlineTextMetricsW(
    HDC			hdc,
    UINT			cbData,
    LPOUTLINETEXTMETRICW	lpOTM
)
{
    TMDIFF Tmd;   // Should not be zero.
    UINT Size, AvailableSize = 0, StringSize;
    XFORM DevToWorld;
    OUTLINETEXTMETRICW* LocalOTM;
    WCHAR* Str;
    BYTE* Ptr;

    /* Get the structure */
    Size = NtGdiGetOutlineTextMetricsInternalW(hdc, 0, NULL, &Tmd);
    if (!Size)
        return 0;
    if (!lpOTM || (cbData < sizeof(*lpOTM)))
        return Size;

    LocalOTM = HeapAlloc(GetProcessHeap(), 0, Size);
    LocalOTM->otmSize = Size;
    Size = NtGdiGetOutlineTextMetricsInternalW(hdc, Size, LocalOTM, &Tmd);
    if (!Size)
    {
        HeapFree(GetProcessHeap(), 0, LocalOTM);
        return 0;
    }

    if (!NtGdiGetTransform(hdc, GdiDeviceSpaceToWorldSpace, &DevToWorld))
    {
        DPRINT1("NtGdiGetTransform failed!\n");
        HeapFree(GetProcessHeap(), 0, LocalOTM);
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Fill in DC specific data */
    LocalOTM->otmTextMetrics.tmDigitizedAspectX = GetDeviceCaps(hdc, LOGPIXELSX);
    LocalOTM->otmTextMetrics.tmDigitizedAspectY = GetDeviceCaps(hdc, LOGPIXELSY);
    LocalOTM->otmTextMetrics.tmHeight = height_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmHeight );
    LocalOTM->otmTextMetrics.tmAscent = height_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmAscent );
    LocalOTM->otmTextMetrics.tmDescent = height_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmDescent );
    LocalOTM->otmTextMetrics.tmInternalLeading = height_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmInternalLeading );
    LocalOTM->otmTextMetrics.tmExternalLeading = height_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmExternalLeading );
    LocalOTM->otmTextMetrics.tmAveCharWidth = width_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmAveCharWidth );
    LocalOTM->otmTextMetrics.tmMaxCharWidth = width_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmMaxCharWidth );
    LocalOTM->otmTextMetrics.tmOverhang = width_to_LP( &DevToWorld, LocalOTM->otmTextMetrics.tmOverhang );
    LocalOTM->otmAscent                = height_to_LP( &DevToWorld, LocalOTM->otmAscent);
    LocalOTM->otmDescent               = height_to_LP( &DevToWorld, LocalOTM->otmDescent);
    LocalOTM->otmLineGap               = abs(INTERNAL_YDSTOWS(&DevToWorld,LocalOTM->otmLineGap));
    LocalOTM->otmsCapEmHeight          = abs(INTERNAL_YDSTOWS(&DevToWorld,LocalOTM->otmsCapEmHeight));
    LocalOTM->otmsXHeight              = abs(INTERNAL_YDSTOWS(&DevToWorld,LocalOTM->otmsXHeight));
    LocalOTM->otmrcFontBox.top         = height_to_LP( &DevToWorld, LocalOTM->otmrcFontBox.top);
    LocalOTM->otmrcFontBox.bottom      = height_to_LP( &DevToWorld, LocalOTM->otmrcFontBox.bottom);
    LocalOTM->otmrcFontBox.left        = width_to_LP( &DevToWorld, LocalOTM->otmrcFontBox.left);
    LocalOTM->otmrcFontBox.right       = width_to_LP( &DevToWorld, LocalOTM->otmrcFontBox.right);
    LocalOTM->otmMacAscent             = height_to_LP( &DevToWorld, LocalOTM->otmMacAscent);
    LocalOTM->otmMacDescent            = height_to_LP( &DevToWorld, LocalOTM->otmMacDescent);
    LocalOTM->otmMacLineGap            = abs(INTERNAL_YDSTOWS(&DevToWorld,LocalOTM->otmMacLineGap));
    LocalOTM->otmptSubscriptSize.x     = width_to_LP( &DevToWorld, LocalOTM->otmptSubscriptSize.x);
    LocalOTM->otmptSubscriptSize.y     = height_to_LP( &DevToWorld, LocalOTM->otmptSubscriptSize.y);
    LocalOTM->otmptSubscriptOffset.x   = width_to_LP( &DevToWorld, LocalOTM->otmptSubscriptOffset.x);
    LocalOTM->otmptSubscriptOffset.y   = height_to_LP( &DevToWorld, LocalOTM->otmptSubscriptOffset.y);
    LocalOTM->otmptSuperscriptSize.x   = width_to_LP( &DevToWorld, LocalOTM->otmptSuperscriptSize.x);
    LocalOTM->otmptSuperscriptSize.y   = height_to_LP( &DevToWorld, LocalOTM->otmptSuperscriptSize.y);
    LocalOTM->otmptSuperscriptOffset.x = width_to_LP( &DevToWorld, LocalOTM->otmptSuperscriptOffset.x);
    LocalOTM->otmptSuperscriptOffset.y = height_to_LP( &DevToWorld, LocalOTM->otmptSuperscriptOffset.y);
    LocalOTM->otmsStrikeoutSize        = abs(INTERNAL_YDSTOWS(&DevToWorld,LocalOTM->otmsStrikeoutSize));
    LocalOTM->otmsStrikeoutPosition    = height_to_LP( &DevToWorld, LocalOTM->otmsStrikeoutPosition);
    LocalOTM->otmsUnderscoreSize       = height_to_LP( &DevToWorld, LocalOTM->otmsUnderscoreSize);
    LocalOTM->otmsUnderscorePosition   = height_to_LP( &DevToWorld, LocalOTM->otmsUnderscorePosition);

    /* Copy what we can */
    CopyMemory(lpOTM, LocalOTM, min(Size, cbData));

    lpOTM->otmpFamilyName = NULL;
    lpOTM->otmpFaceName = NULL;
    lpOTM->otmpStyleName = NULL;
    lpOTM->otmpFullName = NULL;

    Size = sizeof(*lpOTM);
    AvailableSize = cbData - Size;
    Ptr = (BYTE*)lpOTM + sizeof(*lpOTM);

    /* Fix string values up */
    if (LocalOTM->otmpFamilyName)
    {
        Str = (WCHAR*)((char*)LocalOTM + (ptrdiff_t)LocalOTM->otmpFamilyName);
        StringSize = (wcslen(Str) + 1) * sizeof(WCHAR);
        if (AvailableSize >= StringSize)
        {
            CopyMemory(Ptr, Str, StringSize);
            lpOTM->otmpFamilyName = (PSTR)(Ptr - (BYTE*)lpOTM);
            Ptr += StringSize;
            AvailableSize -= StringSize;
            Size += StringSize;
        }
    }

    if (LocalOTM->otmpFaceName)
    {
        Str = (WCHAR*)((char*)LocalOTM + (ptrdiff_t)LocalOTM->otmpFaceName);
        StringSize = (wcslen(Str) + 1) * sizeof(WCHAR);
        if (AvailableSize >= StringSize)
        {
            CopyMemory(Ptr, Str, StringSize);
            lpOTM->otmpFaceName = (PSTR)(Ptr - (BYTE*)lpOTM);
            Ptr += StringSize;
            AvailableSize -= StringSize;
            Size += StringSize;
        }
    }

    if (LocalOTM->otmpStyleName)
    {
        Str = (WCHAR*)((char*)LocalOTM + (ptrdiff_t)LocalOTM->otmpStyleName);
        StringSize = (wcslen(Str) + 1) * sizeof(WCHAR);
        if (AvailableSize >= StringSize)
        {
            CopyMemory(Ptr, Str, StringSize);
            lpOTM->otmpStyleName = (PSTR)(Ptr - (BYTE*)lpOTM);
            Ptr += StringSize;
            AvailableSize -= StringSize;
            Size += StringSize;
        }
    }

    if (LocalOTM->otmpFullName)
    {
        Str = (WCHAR*)((char*)LocalOTM + (ptrdiff_t)LocalOTM->otmpFullName);
        StringSize = (wcslen(Str) + 1) * sizeof(WCHAR);
        if (AvailableSize >= StringSize)
        {
            CopyMemory(Ptr, Str, StringSize);
            lpOTM->otmpFullName = (PSTR)(Ptr - (BYTE*)lpOTM);
            Ptr += StringSize;
            AvailableSize -= StringSize;
            Size += StringSize;
        }
    }

    lpOTM->otmSize = Size;

    HeapFree(GetProcessHeap(), 0, LocalOTM);

    return Size;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetKerningPairsW(HDC hdc,
                 ULONG cPairs,
                 LPKERNINGPAIR pkpDst)
{
    if ((cPairs != 0) || (pkpDst == 0))
    {
        return NtGdiGetKerningPairs(hdc,cPairs,pkpDst);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
}

/*
 * @implemented
 */
DWORD
WINAPI
GetKerningPairsA( HDC hDC,
                  DWORD cPairs,
                  LPKERNINGPAIR kern_pairA )
{
    INT charset;
    CHARSETINFO csi;
    CPINFO cpi;
    DWORD i, total_kern_pairs, kern_pairs_copied = 0;
    KERNINGPAIR *kern_pairW;

    if (!cPairs && kern_pairA)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    charset = GetTextCharset(hDC);
    if (!TranslateCharsetInfo(ULongToPtr(charset), &csi, TCI_SRCCHARSET))
    {
        DPRINT1("Can't find codepage for charset %d\n", charset);
        return 0;
    }
    /* GetCPInfo() will fail on CP_SYMBOL, and WideCharToMultiByte is supposed
     * to fail on an invalid character for CP_SYMBOL.
     */
    cpi.DefaultChar[0] = 0;
    if (csi.ciACP != CP_SYMBOL && !GetCPInfo(csi.ciACP, &cpi))
    {
        DPRINT1("Can't find codepage %u info\n", csi.ciACP);
        return 0;
    }
    DPRINT("charset %d => codepage %u\n", charset, csi.ciACP);

    total_kern_pairs = NtGdiGetKerningPairs(hDC, 0, NULL);
    if (!total_kern_pairs) return 0;

    if (!cPairs && !kern_pairA) return total_kern_pairs;

    kern_pairW = HeapAlloc(GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pairW));
    if (kern_pairW == NULL)
    {
        return 0;
    }
    GetKerningPairsW(hDC, total_kern_pairs, kern_pairW);

    for (i = 0; i < total_kern_pairs; i++)
    {
        char first, second;

        if (!WideCharToMultiByte(csi.ciACP, 0, &kern_pairW[i].wFirst, 1, &first, 1, NULL, NULL))
            continue;

        if (!WideCharToMultiByte(csi.ciACP, 0, &kern_pairW[i].wSecond, 1, &second, 1, NULL, NULL))
            continue;

        if (first == cpi.DefaultChar[0] || second == cpi.DefaultChar[0])
            continue;

        if (kern_pairA)
        {
            if (kern_pairs_copied >= cPairs) break;

            kern_pairA->wFirst = (BYTE)first;
            kern_pairA->wSecond = (BYTE)second;
            kern_pairA->iKernAmount = kern_pairW[i].iKernAmount;
            kern_pairA++;
        }
        kern_pairs_copied++;
    }

    HeapFree(GetProcessHeap(), 0, kern_pairW);

    return kern_pairs_copied;
}



/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectExA(const ENUMLOGFONTEXDVA *elfexd)
{
    if (elfexd)
    {
        ENUMLOGFONTEXDVW Logfont;

        EnumLogFontExW2A( (LPENUMLOGFONTEXA) elfexd,
                          &Logfont.elfEnumLogfontEx );

        RtlCopyMemory( &Logfont.elfDesignVector,
                       (PVOID) &elfexd->elfDesignVector,
                       sizeof(DESIGNVECTOR));

        return NtGdiHfontCreate( &Logfont, 0, 0, 0, NULL);
    }
    else return NULL;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectExW(const ENUMLOGFONTEXDVW *elfexd)
{
    /* Msdn: Note, this function ignores the elfDesignVector member in
             ENUMLOGFONTEXDV.
     */
    if ( elfexd )
    {
        return NtGdiHfontCreate((PENUMLOGFONTEXDVW) elfexd, 0, 0, 0, NULL );
    }
    else return NULL;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectA(
    CONST LOGFONTA		*lplf
)
{
    if (lplf)
    {
        LOGFONTW tlf;

        LogFontA2W(&tlf, lplf);
        return CreateFontIndirectW(&tlf);
    }
    else return NULL;
}


#if DBG
VOID DumpFamilyInfo(const FONTFAMILYINFO *Info, LONG Count)
{
    LONG i;
    const LOGFONTW *plf;

    DPRINT1("---\n");
    DPRINT1("Count: %d\n", Count);
    for (i = 0; i < Count; ++i)
    {
        plf = &Info[i].EnumLogFontEx.elfLogFont;
        DPRINT1("%d: '%S',%u,'%S', %ld:%ld, %ld, %d, %d\n", i,
            plf->lfFaceName, plf->lfCharSet, Info[i].EnumLogFontEx.elfFullName,
            plf->lfHeight, plf->lfWidth, plf->lfWeight, plf->lfItalic, plf->lfPitchAndFamily);
    }
}

VOID DoFontSystemUnittest(VOID)
{
#ifndef RTL_SOFT_ASSERT
#define RTL_SOFT_ASSERT(exp) \
  (void)((!(exp)) ? \
    DbgPrint("%s(%d): Soft assertion failed\n Expression: %s\n", __FILE__, __LINE__, #exp), FALSE : TRUE)
#define RTL_SOFT_ASSERT_defined
#endif

    LOGFONTW LogFont;
    FONTFAMILYINFO Info[4];
    UNICODE_STRING Str1, Str2;
    LONG ret, InfoCount;

    //DumpFontInfo(TRUE);

    /* L"" DEFAULT_CHARSET */
    RtlZeroMemory(&LogFont, sizeof(LogFont));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    InfoCount = RTL_NUMBER_OF(Info);
    ret = NtGdiGetFontFamilyInfo(NULL, &LogFont, Info, &InfoCount);
    DPRINT1("ret: %ld, InfoCount: %ld\n", ret, InfoCount);
    DumpFamilyInfo(Info, ret);
    RTL_SOFT_ASSERT(ret == RTL_NUMBER_OF(Info));
    RTL_SOFT_ASSERT(InfoCount > 32);

    /* L"Microsoft Sans Serif" ANSI_CHARSET */
    RtlZeroMemory(&LogFont, sizeof(LogFont));
    LogFont.lfCharSet = ANSI_CHARSET;
    StringCbCopyW(LogFont.lfFaceName, sizeof(LogFont.lfFaceName), L"Microsoft Sans Serif");
    InfoCount = RTL_NUMBER_OF(Info);
    ret = NtGdiGetFontFamilyInfo(NULL, &LogFont, Info, &InfoCount);
    DPRINT1("ret: %ld, InfoCount: %ld\n", ret, InfoCount);
    DumpFamilyInfo(Info, ret);
    RTL_SOFT_ASSERT(ret != -1);
    RTL_SOFT_ASSERT(InfoCount > 0);
    RTL_SOFT_ASSERT(InfoCount < 16);

    RtlInitUnicodeString(&Str1, Info[0].EnumLogFontEx.elfLogFont.lfFaceName);
    RtlInitUnicodeString(&Str2, L"Microsoft Sans Serif");
    ret = RtlCompareUnicodeString(&Str1, &Str2, TRUE);
    RTL_SOFT_ASSERT(ret == 0);

    RtlInitUnicodeString(&Str1, Info[0].EnumLogFontEx.elfFullName);
    RtlInitUnicodeString(&Str2, L"Tahoma");
    ret = RtlCompareUnicodeString(&Str1, &Str2, TRUE);
    RTL_SOFT_ASSERT(ret == 0);

    /* L"Non-Existent" DEFAULT_CHARSET */
    RtlZeroMemory(&LogFont, sizeof(LogFont));
    LogFont.lfCharSet = ANSI_CHARSET;
    StringCbCopyW(LogFont.lfFaceName, sizeof(LogFont.lfFaceName), L"Non-Existent");
    InfoCount = RTL_NUMBER_OF(Info);
    ret = NtGdiGetFontFamilyInfo(NULL, &LogFont, Info, &InfoCount);
    DPRINT1("ret: %ld, InfoCount: %ld\n", ret, InfoCount);
    DumpFamilyInfo(Info, ret);
    RTL_SOFT_ASSERT(ret == 0);
    RTL_SOFT_ASSERT(InfoCount == 0);

#ifdef RTL_SOFT_ASSERT_defined
#undef RTL_SOFT_ASSERT_defined
#undef RTL_SOFT_ASSERT
#endif
}
#endif

/* EOF */
/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectW(
    CONST LOGFONTW		*lplf
)
{
#if 0
    static BOOL bDidTest = FALSE;
    if (!bDidTest)
    {
        bDidTest = TRUE;
        DoFontSystemUnittest();
    }
#endif
    if (lplf)
    {
        ENUMLOGFONTEXDVW Logfont;

        RtlCopyMemory( &Logfont.elfEnumLogfontEx.elfLogFont, lplf, sizeof(LOGFONTW));
        // Need something other than just cleaning memory here.
        // Guess? Use caller data to determine the rest.
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfFullName,
                       sizeof(Logfont.elfEnumLogfontEx.elfFullName));
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfStyle,
                       sizeof(Logfont.elfEnumLogfontEx.elfStyle));
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfScript,
                       sizeof(Logfont.elfEnumLogfontEx.elfScript));

        Logfont.elfDesignVector.dvNumAxes = 0; // No more than MM_MAX_NUMAXES

        RtlZeroMemory( &Logfont.elfDesignVector, sizeof(DESIGNVECTOR));

        return CreateFontIndirectExW(&Logfont);
    }
    else return NULL;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontA(
    int	nHeight,
    int	nWidth,
    int	nEscapement,
    int	nOrientation,
    int	fnWeight,
    DWORD	fdwItalic,
    DWORD	fdwUnderline,
    DWORD	fdwStrikeOut,
    DWORD	fdwCharSet,
    DWORD	fdwOutputPrecision,
    DWORD	fdwClipPrecision,
    DWORD	fdwQuality,
    DWORD	fdwPitchAndFamily,
    LPCSTR	lpszFace
)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    HFONT ret;

    RtlInitAnsiString(&StringA, (LPSTR)lpszFace);
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    ret = CreateFontW(nHeight,
                      nWidth,
                      nEscapement,
                      nOrientation,
                      fnWeight,
                      fdwItalic,
                      fdwUnderline,
                      fdwStrikeOut,
                      fdwCharSet,
                      fdwOutputPrecision,
                      fdwClipPrecision,
                      fdwQuality,
                      fdwPitchAndFamily,
                      StringU.Buffer);

    RtlFreeUnicodeString(&StringU);

    return ret;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontW(
    int	nHeight,
    int	nWidth,
    int	nEscapement,
    int	nOrientation,
    int	nWeight,
    DWORD	fnItalic,
    DWORD	fdwUnderline,
    DWORD	fdwStrikeOut,
    DWORD	fdwCharSet,
    DWORD	fdwOutputPrecision,
    DWORD	fdwClipPrecision,
    DWORD	fdwQuality,
    DWORD	fdwPitchAndFamily,
    LPCWSTR	lpszFace
)
{
    LOGFONTW logfont;

    logfont.lfHeight = nHeight;
    logfont.lfWidth = nWidth;
    logfont.lfEscapement = nEscapement;
    logfont.lfOrientation = nOrientation;
    logfont.lfWeight = nWeight;
    logfont.lfItalic = (BYTE)fnItalic;
    logfont.lfUnderline = (BYTE)fdwUnderline;
    logfont.lfStrikeOut = (BYTE)fdwStrikeOut;
    logfont.lfCharSet = (BYTE)fdwCharSet;
    logfont.lfOutPrecision = (BYTE)fdwOutputPrecision;
    logfont.lfClipPrecision = (BYTE)fdwClipPrecision;
    logfont.lfQuality = (BYTE)fdwQuality;
    logfont.lfPitchAndFamily = (BYTE)fdwPitchAndFamily;

    if (NULL != lpszFace)
    {
        int Size = sizeof(logfont.lfFaceName) / sizeof(WCHAR);
        wcsncpy((wchar_t *)logfont.lfFaceName, lpszFace, Size - 1);
        /* Be 101% sure to have '\0' at end of string */
        logfont.lfFaceName[Size - 1] = '\0';
    }
    else
    {
        logfont.lfFaceName[0] = L'\0';
    }

    return CreateFontIndirectW(&logfont);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CreateScalableFontResourceA(
    DWORD		fdwHidden,
    LPCSTR		lpszFontRes,
    LPCSTR		lpszFontFile,
    LPCSTR		lpszCurrentPath
)
{
    return FALSE;
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceExW ( LPCWSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
    if (fl & ~(FR_PRIVATE | FR_NOT_ENUM))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    return GdiAddFontResourceW(lpszFilename, fl,0);
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceExA ( LPCSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
    NTSTATUS Status;
    PWSTR FilenameW;
    int rc;

    if (fl & ~(FR_PRIVATE | FR_NOT_ENUM))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    Status = HEAP_strdupA2W ( &FilenameW, lpszFilename );
    if ( !NT_SUCCESS (Status) )
    {
        SetLastError (RtlNtStatusToDosError(Status));
        return 0;
    }

    rc = GdiAddFontResourceW ( FilenameW, fl, 0 );
    HEAP_free ( FilenameW );
    return rc;
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceA ( LPCSTR lpszFilename )
{
    NTSTATUS Status;
    PWSTR FilenameW;
    int rc = 0;

    Status = HEAP_strdupA2W ( &FilenameW, lpszFilename );
    if ( !NT_SUCCESS (Status) )
    {
        SetLastError (RtlNtStatusToDosError(Status));
    }
    else
    {
        rc = GdiAddFontResourceW ( FilenameW, 0, 0);

        HEAP_free ( FilenameW );
    }
    return rc;
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceW ( LPCWSTR lpszFilename )
{
    return GdiAddFontResourceW ( lpszFilename, 0, 0 );
}


/*
 * @implemented
 */
BOOL
WINAPI
RemoveFontResourceW(LPCWSTR lpFileName)
{
    return RemoveFontResourceExW(lpFileName,0,0);
}


/*
 * @implemented
 */
BOOL
WINAPI
RemoveFontResourceA(LPCSTR lpFileName)
{
    return RemoveFontResourceExA(lpFileName,0,0);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RemoveFontResourceExA(LPCSTR lpFileName,
                      DWORD fl,
                      PVOID pdv
                     )
{
    NTSTATUS Status;
    LPWSTR lpFileNameW;

    /* FIXME the flags */
    /* FIXME the pdv */
    /* FIXME NtGdiRemoveFontResource handle flags and pdv */

    Status = HEAP_strdupA2W ( &lpFileNameW, lpFileName );
    if (!NT_SUCCESS (Status))
        SetLastError (RtlNtStatusToDosError(Status));
    else
    {

        HEAP_free ( lpFileNameW );
    }

    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RemoveFontResourceExW(LPCWSTR lpFileName,
                      DWORD fl,
                      PVOID pdv)
{
    /* FIXME the flags */
    /* FIXME the pdv */
    /* FIXME NtGdiRemoveFontResource handle flags and pdv */
    DPRINT("RemoveFontResourceExW\n");
    return 0;
}


/***********************************************************************
 *           GdiGetCharDimensions
 *
 * Gets the average width of the characters in the English alphabet.
 *
 * PARAMS
 *  hdc    [I] Handle to the device context to measure on.
 *  lptm   [O] Pointer to memory to store the text metrics into.
 *  height [O] On exit, the maximum height of characters in the English alphabet.
 *
 * RETURNS
 *  The average width of characters in the English alphabet.
 *
 * NOTES
 *  This function is used by the dialog manager to get the size of a dialog
 *  unit. It should also be used by other pieces of code that need to know
 *  the size of a dialog unit in logical units without having access to the
 *  window handle of the dialog.
 *  Windows caches the font metrics from this function, but we don't and
 *  there doesn't appear to be an immediate advantage to do so.
 *
 * SEE ALSO
 *  GetTextExtentPointW, GetTextMetricsW, MapDialogRect.
 *
 * Despite most of MSDN insisting that the horizontal base unit is
 * tmAveCharWidth it isn't.  Knowledge base article Q145994
 * "HOWTO: Calculate Dialog Units When Not Using the System Font",
 * says that we should take the average of the 52 English upper and lower
 * case characters.
 */
/*
 * @implemented
 */
LONG
WINAPI
GdiGetCharDimensions(HDC hdc, LPTEXTMETRICW lptm, LONG *height)
{
    SIZE sz;
    TEXTMETRICW tm;
    static const WCHAR alphabet[] =
    {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0
    };

    if(!GetTextMetricsW(hdc, &tm)) return 0;

    if(!GetTextExtentPointW(hdc, alphabet, 52, &sz)) return 0;

    if (lptm) *lptm = tm;
    if (height) *height = tm.tmHeight;

    return (sz.cx / 26 + 1) / 2;
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.@]
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondance between different labelings
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges)
 * of the same encoding.
 *
 * Only one codepage will be set in lpCs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *lpSrc.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
/*
 * @implemented
 */
BOOL
WINAPI
TranslateCharsetInfo(
    LPDWORD lpSrc, /* [in]
       if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
       if flags == TCI_SRCCHARSET: a character set value
       if flags == TCI_SRCCODEPAGE: a code page value
		 */
    LPCHARSETINFO lpCs, /* [out] structure to receive charset information */
    DWORD flags /* [in] determines interpretation of lpSrc */)
{
    int index = 0;
    switch (flags)
    {
    case TCI_SRCFONTSIG:
        while (index < MAXTCIINDEX && !(*lpSrc>>index & 0x0001)) index++;
        break;
    case TCI_SRCCODEPAGE:
        while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciACP) index++;
        break;
    case TCI_SRCCHARSET:
        while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciCharset) index++;
        break;
    case TCI_SRCLOCALE:
    {
        LCID lCid = (LCID)PtrToUlong(lpSrc);
        LOCALESIGNATURE LocSig;
        INT Ret = GetLocaleInfoW(lCid, LOCALE_FONTSIGNATURE, (LPWSTR)&LocSig, 0);
        if ( GetLocaleInfoW(lCid, LOCALE_FONTSIGNATURE, (LPWSTR)&LocSig, Ret))
        {
           while (index < MAXTCIINDEX && !(LocSig.lsCsbDefault[0]>>index & 0x0001)) index++;
           break;
        }
    }
    default:
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (index >= MAXTCIINDEX || FONT_tci[index].ciCharset == DEFAULT_CHARSET) return FALSE;
    DPRINT("Index %d Charset %u CodePage %u FontSig %lu\n",
             index,FONT_tci[index].ciCharset,FONT_tci[index].ciACP,FONT_tci[index].fs.fsCsb[0]);
    memcpy(lpCs, &FONT_tci[index], sizeof(CHARSETINFO));
    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetMapperFlags(
    HDC	hDC,
    DWORD	flags
)
{
    DWORD Ret = GDI_ERROR;
    PDC_ATTR Dc_Attr;

    /* Get the DC attribute */
    Dc_Attr = GdiGetDcAttr(hDC);
    if (Dc_Attr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return GDI_ERROR;
    }

    if (NtCurrentTeb()->GdiTebBatch.HDC == hDC)
    {
        if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
        {
            NtGdiFlush();
            Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    if ( flags & ~1 )
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        Ret = Dc_Attr->flFontMapper;
        Dc_Attr->flFontMapper = flags;
    }
    return Ret;
}


/*
 * @unimplemented
 */
int
WINAPI
EnumFontsW(
    HDC  hDC,
    LPCWSTR lpFaceName,
    FONTENUMPROCW  FontFunc,
    LPARAM  lParam
)
{
#if 0
    return NtGdiEnumFonts ( hDC, lpFaceName, FontFunc, lParam );
#else
    return EnumFontFamiliesW( hDC, lpFaceName, FontFunc, lParam );
#endif
}

/*
 * @unimplemented
 */
int
WINAPI
EnumFontsA (
    HDC  hDC,
    LPCSTR lpFaceName,
    FONTENUMPROCA  FontFunc,
    LPARAM  lParam
)
{
#if 0
    NTSTATUS Status;
    LPWSTR lpFaceNameW;
    int rc = 0;

    Status = HEAP_strdupA2W ( &lpFaceNameW, lpFaceName );
    if (!NT_SUCCESS (Status))
        SetLastError (RtlNtStatusToDosError(Status));
    else
    {
        rc = NtGdiEnumFonts ( hDC, lpFaceNameW, FontFunc, lParam );

        HEAP_free ( lpFaceNameW );
    }
    return rc;
#else
    return EnumFontFamiliesA( hDC, lpFaceName, FontFunc, lParam );
#endif
}

#define EfdFontFamilies 3

INT
WINAPI
NewEnumFontFamiliesExW(
    HDC hDC,
    LPLOGFONTW lpLogfont,
    FONTENUMPROCW lpEnumFontFamExProcW,
    LPARAM lParam,
    DWORD dwFlags)
{
    ULONG_PTR idEnum;
    ULONG cbDataSize, cbRetSize;
    PENUMFONTDATAW pEfdw;
    PBYTE pBuffer;
    PBYTE pMax;
    INT ret = 1;

    /* Open enumeration handle and find out how much memory we need */
    idEnum = NtGdiEnumFontOpen(hDC,
                               EfdFontFamilies,
                               0,
                               LF_FACESIZE,
                               (lpLogfont && lpLogfont->lfFaceName[0])? lpLogfont->lfFaceName : NULL,
                               lpLogfont? lpLogfont->lfCharSet : DEFAULT_CHARSET,
                               &cbDataSize);
    if (idEnum == 0)
    {
        return 0;
    }
    if (cbDataSize == 0)
    {
        NtGdiEnumFontClose(idEnum);
        return 0;
    }

    /* Allocate memory */
    pBuffer = HeapAlloc(GetProcessHeap(), 0, cbDataSize);
    if (pBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        NtGdiEnumFontClose(idEnum);
        return 0;
    }

    /* Do the enumeration */
    if (!NtGdiEnumFontChunk(hDC, idEnum, cbDataSize, &cbRetSize, (PVOID)pBuffer))
    {
        HeapFree(GetProcessHeap(), 0, pBuffer);
        NtGdiEnumFontClose(idEnum);
        return 0;
    }

    /* Get start and end address */
    pEfdw = (PENUMFONTDATAW)pBuffer;
    pMax = pBuffer + cbDataSize;

    /* Iterate through the structures */
    while ((PBYTE)pEfdw < pMax && ret)
    {
        PNTMW_INTERNAL pNtmwi = (PNTMW_INTERNAL)((ULONG_PTR)pEfdw + pEfdw->ulNtmwiOffset);

        ret = lpEnumFontFamExProcW((VOID*)&pEfdw->elfexdv.elfEnumLogfontEx,
                                   (VOID*)&pNtmwi->ntmw,
                                   pEfdw->dwFontType,
                                   lParam);

        pEfdw = (PENUMFONTDATAW)((ULONG_PTR)pEfdw + pEfdw->cbSize);
    }

    /* Release the memory and close handle */
    HeapFree(GetProcessHeap(), 0, pBuffer);
    NtGdiEnumFontClose(idEnum);

    return ret;
}

/*
 * @implemented
 */
int
WINAPI
GdiAddFontResourceW(
    LPCWSTR lpszFilename,
    FLONG fl,
    DESIGNVECTOR *pdv)
{
    INT Ret;
    WCHAR lpszBuffer[MAX_PATH];
    WCHAR lpszAbsPath[MAX_PATH];
    UNICODE_STRING NtAbsPath;

    /* FIXME: We don't support multiple files passed in lpszFilename
     *        as L"abcxxxxx.pfm|abcxxxxx.pfb"
     */

    /* Does the file exist in CurrentDirectory or in the Absolute Path passed? */
    GetCurrentDirectoryW(MAX_PATH, lpszBuffer);

    if (!SearchPathW(lpszBuffer, lpszFilename, NULL, MAX_PATH, lpszAbsPath, NULL))
    {
        /* Nope. Then let's check Fonts folder */
        GetWindowsDirectoryW(lpszBuffer, MAX_PATH);
        StringCbCatW(lpszBuffer, sizeof(lpszBuffer), L"\\Fonts");

        if (!SearchPathW(lpszBuffer, lpszFilename, NULL, MAX_PATH, lpszAbsPath, NULL))
        {
            DPRINT1("Font not found. The Buffer is: %ls, the FileName is: %S\n", lpszBuffer, lpszFilename);
            return 0;
        }
    }

    /* We found the font file so: */
    if (!RtlDosPathNameToNtPathName_U(lpszAbsPath, &NtAbsPath, NULL, NULL))
    {
        DPRINT1("Can't convert Path! Path: %ls\n", lpszAbsPath);
        return 0;
    }

    /* The Nt call expects a null-terminator included in cwc param. */
    ASSERT(NtAbsPath.Buffer[NtAbsPath.Length / sizeof(WCHAR)] == UNICODE_NULL);
    Ret = NtGdiAddFontResourceW(NtAbsPath.Buffer, NtAbsPath.Length / sizeof(WCHAR) + 1, 1, fl, 0, pdv);

    RtlFreeUnicodeString(&NtAbsPath);

    return Ret;
}

/*
 * @implemented
 */
HANDLE
WINAPI
AddFontMemResourceEx(
    PVOID pbFont,
    DWORD cbFont,
    PVOID pdv,
    DWORD *pcFonts
)
{
    if ( pbFont && cbFont && pcFonts)
    {
        return NtGdiAddFontMemResourceEx(pbFont, cbFont, NULL, 0, pcFonts);
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
}

/*
 * @implemented
 */
BOOL
WINAPI
RemoveFontMemResourceEx(HANDLE fh)
{
    if (fh)
    {
        return NtGdiRemoveFontMemResourceEx(fh);
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/*
 * @unimplemented
 */
int
WINAPI
AddFontResourceTracking(
    LPCSTR lpString,
    int unknown
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
int
WINAPI
RemoveFontResourceTracking(LPCSTR lpString,int unknown)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

BOOL
WINAPI
CreateScalableFontResourceW(
    DWORD fdwHidden,
    LPCWSTR lpszFontRes,
    LPCWSTR lpszFontFile,
    LPCWSTR lpszCurrentPath
)
{
    HANDLE f;

    UNIMPLEMENTED;

    /* fHidden=1 - only visible for the calling app, read-only, not
     * enumerated with EnumFonts/EnumFontFamilies
     * lpszCurrentPath can be NULL
     */

    /* If the output file already exists, return the ERROR_FILE_EXISTS error as specified in MSDN */
    if ((f = CreateFileW(lpszFontRes, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE)
    {
        CloseHandle(f);
        SetLastError(ERROR_FILE_EXISTS);
        return FALSE;
    }
    return FALSE; /* create failed */
}

/*
 * @unimplemented
 */
BOOL
WINAPI
bInitSystemAndFontsDirectoriesW(LPWSTR *SystemDir,LPWSTR *FontsDir)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
EudcLoadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath,INT iPriority,INT iFontLinkType)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
EudcUnloadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
ULONG
WINAPI
GetEUDCTimeStamp(VOID)
{
    return NtGdiGetEudcTimeStampEx(NULL,0,TRUE);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetEUDCTimeStampExW(LPWSTR lpBaseFaceName)
{
    DWORD retValue = 0;

    if (!lpBaseFaceName)
    {
        retValue = NtGdiGetEudcTimeStampEx(NULL,0,FALSE);
    }
    else
    {
        retValue = NtGdiGetEudcTimeStampEx(lpBaseFaceName, wcslen(lpBaseFaceName), FALSE);
    }

    return retValue;
}

/*
 * @implemented
 */
ULONG
WINAPI
GetFontAssocStatus(HDC hdc)
{
    ULONG retValue = 0;

    if (hdc)
    {
        retValue = NtGdiQueryFontAssocInfo(hdc);
    }

    return retValue;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
QueryFontAssocStatus(VOID)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
WINAPI
UnloadNetworkFonts(DWORD unknown)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @implemented
 *
 */
DWORD
WINAPI
GetFontData(HDC hdc,
            DWORD dwTable,
            DWORD dwOffset,
            LPVOID lpvBuffer,
            DWORD cbData)
{
    if (!lpvBuffer)
    {
        cbData = 0;
    }
    return NtGdiGetFontData(hdc, dwTable, dwOffset, lpvBuffer, cbData);
}

DWORD
WINAPI
cGetTTFFromFOT(DWORD x1 ,DWORD x2 ,DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

