/*
 * PROJECT:     ReactOS LPK
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Language Pack DLL.
 * PROGRAMMERS: Magnus Olsen  (greatlrd)
 *              Baruch Rutman (peterooch at gmail dot com)
 */

#include "ros_lpk.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

LPK_LPEDITCONTROL_LIST LpkEditControl = {EditCreate,       EditIchToXY,  EditMouseToIch, EditCchInWidth,
                                         EditGetLineWidth, EditDrawText, EditHScroll,    EditMoveSelection,
                                         EditVerifyText,   EditNextWord, EditSetMenu,    EditProcessMenu,
                                         EditCreateCaret, EditAdjustCaret};

#define PREFIX 38
#define ALPHA_PREFIX 30 /* Win16: Alphabet prefix */
#define KANA_PREFIX  31 /* Win16: Katakana prefix */

static int PSM_FindLastPrefix(LPCWSTR str, int count)
{
    int i, prefix_count = 0, index = -1;

    for (i = 0; i < count - 1; i++)
    {
        if (str[i] == PREFIX && str[i + 1] != PREFIX)
        {
            index = i - prefix_count;
            prefix_count++;
        }
        else if (str[i] == PREFIX && str[i + 1] == PREFIX)
        {
            i++;
        }
    }
    return index;
}

static void PSM_PrepareToDraw(LPCWSTR str, INT count, LPWSTR new_str, LPINT new_count)
{
    int len, i = 0, j = 0;

    while (i < count)
    {
        if (str[i] == PREFIX || (iswspace(str[i]) && str[i] != L' '))
        {
            if (i < count - 1 && str[i + 1] == PREFIX)
                new_str[j++] = str[i++];
            else
                i++;
        }
        else
        {
            new_str[j++] = str[i++];
        }
    }

    new_str[j] = L'\0';
    len = wcslen(new_str);
    *new_count = len;
}

/* Can be used with also LpkDrawTextEx() if it will be implemented */
static void LPK_DrawUnderscore(HDC hdc, int x, int y, LPCWSTR str, int count, int offset)
{
    SCRIPT_STRING_ANALYSIS ssa;
    DWORD dwSSAFlags = SSA_GLYPHS;
    int prefix_x;
    int prefix_end;
    int pos;
    SIZE size;
    HPEN hpen;
    HPEN oldPen;
    HRESULT hr = S_FALSE;

    if (offset == -1)
        return;

    if (ScriptIsComplex(str, count, SIC_COMPLEX) == S_OK)
    {
        if (GetLayout(hdc) & LAYOUT_RTL || GetTextAlign(hdc) & TA_RTLREADING)
            dwSSAFlags |= SSA_RTL;

        hr = ScriptStringAnalyse(hdc, str, count, (3 * count / 2 + 16),
                                 -1, dwSSAFlags, -1, NULL, NULL, NULL, NULL, NULL, &ssa);
    }

    if (hr == S_OK)
    {
        ScriptStringCPtoX(ssa, offset, FALSE, &pos);
        prefix_x = x + pos;
        ScriptStringCPtoX(ssa, offset, TRUE, &pos);
        prefix_end = x + pos;
        ScriptStringFree(&ssa);
    }
    else
    {
        GetTextExtentPointW(hdc, str, offset, &size);
        prefix_x = x + size.cx;
        GetTextExtentPointW(hdc, str, offset + 1, &size);
        prefix_end = x + size.cx - 1;
    }
    hpen = CreatePen(PS_SOLID, 1, GetTextColor(hdc));
    oldPen = SelectObject(hdc, hpen);
    MoveToEx(hdc, prefix_x, y, NULL);
    LineTo(hdc, prefix_end, y);
    SelectObject(hdc, oldPen);
    DeleteObject(hpen);
}

static
BOOL
GetProcessImageVersionInfo(
    _Out_ PVOID* pFileInfo,
    _Out_ LPDWORD pdwLen)
{
    HRSRC hResource;
    DWORD dwResourceSize;
    HGLOBAL hResourceData;
    PVOID pvResourceData;

    *pFileInfo = NULL;
    *pdwLen = 0;

    /* Try to get the VersionInfo resource */
    hResource = FindResourceW(NULL, MAKEINTRESOURCEW(VS_VERSION_INFO), RT_VERSION);
    if (hResource == NULL)
        return FALSE;

    /* Get resource size and do basic size check */
    dwResourceSize = SizeofResource(NULL, hResource);
    if (dwResourceSize < sizeof(VS_FIXEDFILEINFO))
        return FALSE;

    /* Load the resource */
    hResourceData = LoadResource(NULL, hResource);
    if (hResourceData == NULL)
        return FALSE;

    /* Lock the resource */
    pvResourceData = LockResource(hResourceData);
    if (pvResourceData == NULL)
        return FALSE;

    *pFileInfo = pvResourceData;
    *pdwLen = dwResourceSize;
    return TRUE;
}

/* Checks the VersionInfo of the main executable for RTL layout
 * and applies it to the process if found.
 * See https://web.archive.org/web/20050207001156/http://www.microsoft.com/globaldev/getwr/steps/WRG_mirror.mspx
 *
 * This function should be called from LpkInitialize(),
 * which is in turn called by GdiInitializeLanguagePack() (from gdi32).
 * TODO: Move call from LpkDllInitialize() to LpkInitialize() when latter
 * function is implemented.
 */
static void LPK_ApplyMirroring(void)
{
    PWSTR pwstrFileDescription = NULL;
    PVOID pvFileInfo;
    DWORD cbFileInfoSize;

    /* Get the VersionInfo resource of the main executable */
    if (!GetProcessImageVersionInfo(&pvFileInfo, &cbFileInfoSize))
    {
        return;
    }

    /* The length (in WCHARs) of the key name, including the null-terminator */
    const DWORD cchKeyLength = sizeof(L"FileDescription") / sizeof(WCHAR);

    /* Calculate the required length (in WCHARs) for a FileDescription entry:
       The key (incl. null) + alignment byte + 2 LTR markers (note: the key
       is always unaligned by 2 bytes) */
    const DWORD cchRequired = cchKeyLength + 1 + 2;

    /* Scan the VersionInfo resource for the "FileDescription" key */
    PWSTR pwchCurrent = (PWSTR)pvFileInfo;
    DWORD cchRemining = cbFileInfoSize / sizeof(WCHAR);
    while (cchRemining >= cchRequired)
    {
        /* Compare current position with the "FileDescription" key */
        if (lstrcmpW(pwchCurrent, L"FileDescription") == 0)
        {
            /* The value starts after the key, the null-terminator and 1 WCHAR alignment */
            pwstrFileDescription = pwchCurrent + cchKeyLength + 1;
            break;
        }

        /* Move to next position */
        pwchCurrent++;
        cchRemining--;
    }

    /* Check if the description starts with 2 LTR markers (u200E).
       Yes, this is how MS marks the layout as RTL (see article linked above). */
    if ((pwstrFileDescription != NULL) &&
        (pwstrFileDescription[0] == 0x200E) &&
        (pwstrFileDescription[1] == 0x200E))
    {
        SetProcessDefaultLayout(LAYOUT_RTL);
    }
}

BOOL
WINAPI
LpkDllInitialize(
    _In_ HANDLE hDll,
    _In_ ULONG dwReason,
    _In_opt_ PVOID pReserved)
{
    UNREFERENCED_PARAMETER(pReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDll);
            /* Tell usp10 it is activated usp10 */
            //LpkPresent();
            LPK_ApplyMirroring();
            break;

        default:
            break;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
LpkExtTextOut(
    HDC hdc,
    int x,
    int y,
    UINT fuOptions,
    const RECT *lprc,
    LPCWSTR lpString,
    UINT uCount,
    const INT *lpDx,
    INT unknown)
{
    LPWORD glyphs = NULL;
    LPWSTR reordered_str = NULL;
    INT cGlyphs;
    DWORD dwSICFlags = SIC_COMPLEX;
    BOOL bResult, bReorder;

    UNREFERENCED_PARAMETER(unknown);

    fuOptions |= ETO_IGNORELANGUAGE;

    /* Check text direction */
    if ((GetLayout(hdc) & LAYOUT_RTL) || (GetTextAlign(hdc) & TA_RTLREADING))
        fuOptions |= ETO_RTLREADING;

    /* If text direction is RTL change flag to account neutral characters */
    if (fuOptions & ETO_RTLREADING)
        dwSICFlags |= SIC_NEUTRAL;

    /* Check if the string requires complex script processing and not a "glyph indices" array */
    if (ScriptIsComplex(lpString, uCount, dwSICFlags) == S_OK && !(fuOptions & ETO_GLYPH_INDEX))
    {
        /* reordered_str is used as fallback in case the glyphs array fails to generate,
           BIDI_Reorder() doesn't attempt to write into reordered_str if memory allocation fails */
        reordered_str = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WCHAR));

        bReorder = BIDI_Reorder(hdc, lpString, uCount, GCP_REORDER,
                                (fuOptions & ETO_RTLREADING) ? WINE_GCPW_FORCE_RTL : WINE_GCPW_FORCE_LTR,
                                reordered_str, uCount, NULL, &glyphs, &cGlyphs);

        /* Now display the reordered text if any of the arrays is valid and if BIDI_Reorder() succeeded */
        if ((glyphs || reordered_str) && bReorder)
        {
            if (glyphs)
            {
                fuOptions |= ETO_GLYPH_INDEX;
                uCount = cGlyphs;
            }

            bResult = ExtTextOutW(hdc, x, y, fuOptions, lprc,
                                  glyphs ? (LPWSTR)glyphs : reordered_str, uCount, lpDx);
        }
        else
        {
            WARN("BIDI_Reorder failed, falling back to original string.\n");
            bResult = ExtTextOutW(hdc, x, y, fuOptions, lprc, lpString, uCount, lpDx);
        }

        HeapFree(GetProcessHeap(), 0, glyphs);
        HeapFree(GetProcessHeap(), 0, reordered_str);

        return bResult;
    }

    return ExtTextOutW(hdc, x, y, fuOptions, lprc, lpString, uCount, lpDx);
}

/*
 * @implemented
 */
DWORD
WINAPI
LpkGetCharacterPlacement(
    HDC hdc,
    LPCWSTR lpString,
    INT uCount,
    INT nMaxExtent,
    LPGCP_RESULTSW lpResults,
    DWORD dwFlags,
    DWORD dwUnused)
{
    DWORD ret = 0;
    HRESULT hr;
    SCRIPT_STRING_ANALYSIS ssa;
    LPWORD lpGlyphs = NULL;
    SIZE size;
    UINT nSet, i;
    INT cGlyphs;

    UNREFERENCED_PARAMETER(dwUnused);

    /* Sanity check (most likely a direct call) */
    if (!(dwFlags & GCP_REORDER))
       return GetCharacterPlacementW(hdc, lpString, uCount, nMaxExtent, lpResults, dwFlags);

    nSet = (UINT)uCount;
    if (nSet > lpResults->nGlyphs)
        nSet = lpResults->nGlyphs;

    BIDI_Reorder(hdc, lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpResults->lpOutString,
                 nSet, lpResults->lpOrder, &lpGlyphs, &cGlyphs);

    lpResults->nGlyphs = (UINT)cGlyphs;

    if (lpResults->lpGlyphs)
    {
        if (lpGlyphs)
            StringCchCopyW(lpResults->lpGlyphs, cGlyphs, lpGlyphs);
        else if (lpResults->lpOutString)
            GetGlyphIndicesW(hdc, lpResults->lpOutString, nSet, lpResults->lpGlyphs, 0);
    }

    if (lpResults->lpDx)
    {
        int c;

        /* If glyph shaping was requested */
        if (dwFlags & GCP_GLYPHSHAPE)
        {
            if (lpResults->lpGlyphs)
            {
                for (i = 0; i < lpResults->nGlyphs; i++)
                {
                    if (GetCharWidthI(hdc, 0, 1, (WORD *)&lpResults->lpGlyphs[i], &c))
                        lpResults->lpDx[i] = c;
                }
            }
        }

        else
        {
            for (i = 0; i < nSet; i++)
            {
                if (GetCharWidth32W(hdc, lpResults->lpOutString[i], lpResults->lpOutString[i], &c))
                    lpResults->lpDx[i] = c;
            }
        }
    }

    if (lpResults->lpCaretPos)
    {
        int pos = 0;

        hr = ScriptStringAnalyse(hdc, lpString, nSet, (3 * nSet / 2 + 16), -1, SSA_GLYPHS, -1,
                                 NULL, NULL, NULL, NULL, NULL, &ssa);
        if (hr == S_OK)
        {
            for (i = 0; i < nSet; i++)
            {
                if (ScriptStringCPtoX(ssa, i, FALSE, &pos) == S_OK)
                    lpResults->lpCaretPos[i] = pos;
            }
            ScriptStringFree(&ssa);
        }
        else
        {
            lpResults->lpCaretPos[0] = 0;
            for (i = 1; i < nSet; i++)
            {
                if (GetTextExtentPoint32W(hdc, &(lpString[i - 1]), 1, &size))
                    lpResults->lpCaretPos[i] = (pos += size.cx);
            }
        }
    }

    if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
        ret = MAKELONG(size.cx, size.cy);

    HeapFree(GetProcessHeap(), 0, lpGlyphs);

    return ret;
}

/* Stripped down version of DrawText(), can only draw single line text and Prefix underscore
 * (only on the last found amperstand).
 * Only flags to be found to be of use in testing:
 *
 * DT_NOPREFIX   - Draw the string as is without removal of the amperstands and without underscore
 * DT_HIDEPREFIX - Draw the string without underscore
 * DT_PREFIXONLY - Draw only the underscore
 *
 * Without any of these flags the behavior is the string being drawn without the amperstands and
 * with the underscore.
 * user32 has an equivalent function - UserLpkPSMTextOut().
 *
 * Note: lpString does not need to be null terminated.
 */
INT WINAPI LpkPSMTextOut(HDC hdc, int x, int y, LPCWSTR lpString, int cString, DWORD dwFlags)
{
    SIZE size;
    TEXTMETRICW tm;
    int prefix_offset, len;
    LPWSTR display_str = NULL;

    if (!lpString || cString <= 0)
        return 0;

    if (dwFlags & DT_NOPREFIX)
    {
        LpkExtTextOut(hdc, x, y, 0, NULL, lpString, cString, NULL, 0);
        GetTextExtentPointW(hdc, lpString, cString, &size);
        return size.cx;
    }

    display_str = HeapAlloc(GetProcessHeap(), 0, (cString + 1) * sizeof(WCHAR));

    if (!display_str)
        return 0;

    PSM_PrepareToDraw(lpString, cString, display_str, &len);

    if (!(dwFlags & DT_PREFIXONLY))
        LpkExtTextOut(hdc, x, y, 0, NULL, display_str, len, NULL, 0);

    if (!(dwFlags & DT_HIDEPREFIX))
    {
        prefix_offset = PSM_FindLastPrefix(lpString, cString);
        GetTextMetricsW(hdc, &tm);
        LPK_DrawUnderscore(hdc, x, y + tm.tmAscent + 1, display_str, len, prefix_offset);
    }

    GetTextExtentPointW(hdc, display_str, len + 1, &size);
    HeapFree(GetProcessHeap(), 0, display_str);

    return size.cx;
}

/*
 * @implemented
 */
BOOL
WINAPI
LpkGetTextExtentExPoint(
    HDC hdc,
    LPCWSTR lpString,
    INT cString,
    INT nMaxExtent,
    LPINT lpnFit,
    LPINT lpnDx,
    LPSIZE lpSize,
    DWORD dwUnused,
    int unknown)
{
    SCRIPT_STRING_ANALYSIS ssa;
    HRESULT hr;
    INT i, extent, *Dx;
    TEXTMETRICW tm;

    UNREFERENCED_PARAMETER(dwUnused);
    UNREFERENCED_PARAMETER(unknown);

    if (cString < 0 || !lpSize)
        return FALSE;

    if (cString == 0 || !lpString)
    {
        lpSize->cx = 0;
        lpSize->cy = 0;
        return TRUE;
    }

    /* Check if any processing is required */
    if (ScriptIsComplex(lpString, cString, SIC_COMPLEX) != S_OK)
        goto fallback;

    hr = ScriptStringAnalyse(hdc, lpString, cString, 3 * cString / 2 + 16, -1,
                             SSA_GLYPHS, 0, NULL, NULL, NULL, NULL, NULL, &ssa);
    if (hr != S_OK)
        goto fallback;

    /* Use logic from TextIntGetTextExtentPoint() */
    Dx = HeapAlloc(GetProcessHeap(), 0, cString * sizeof(INT));
    if (!Dx)
    {
        ScriptStringFree(&ssa);
        goto fallback;
    }

    if (lpnFit)
        *lpnFit = 0;

    ScriptStringGetLogicalWidths(ssa, Dx);

    for (i = 0, extent = 0; i < cString; i++)
    {
        extent += Dx[i];

        if (extent <= nMaxExtent && lpnFit)
            *lpnFit = i + 1;

        if (lpnDx)
            lpnDx[i] = extent;
    }

    HeapFree(GetProcessHeap(), 0, Dx);
    ScriptStringFree(&ssa);

    if (!GetTextMetricsW(hdc, &tm))
        return GetTextExtentExPointWPri(hdc, lpString, cString, 0, NULL, NULL, lpSize);

    lpSize->cx = extent;
    lpSize->cy = tm.tmHeight;

    return TRUE;

fallback:
    return GetTextExtentExPointWPri(hdc, lpString, cString, nMaxExtent, lpnFit, lpnDx, lpSize);
}
