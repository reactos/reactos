/*
 * Implementation of Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2005 Steven Edwards for CodeWeavers
 * Copyright 2006 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Notes:
 * Uniscribe allows for processing of complex scripts such as joining
 * and filtering characters and bi-directional text with custom line breaks.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "usp10.h"

#include "wine/debug.h"
#include "wine/unicode.h"

DWORD STDCALL GetGlyphIndicesW( HDC hdc, LPCWSTR lpstr, int c, LPWORD pgi, DWORD fl);
BOOL  STDCALL GetCharABCWidthsI(HDC hdc, UINT giFirst, UINT cgi, LPWORD pgi, LPABC lpabc);
/**
 * some documentation here:
 *   http://www.microsoft.com/typography/developers/uniscribe/uniscribe.htm
 */

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

static const SCRIPT_PROPERTIES props[] =
{
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 8, 0, 0, 0, 0, 161, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 25, 0, 0, 0, 0, 204, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 42, 0, 0, 0, 0, 163, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 9, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 18, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 18, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 9, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 13, 0, 1, 0, 1, 177, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 13, 0, 1, 0, 0, 177, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 1, 0, 0, 178, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
    { 1, 1, 1, 0, 0, 178, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 41, 1, 1, 0, 0, 178, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 32, 1, 1, 0, 0, 178, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 90, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0 },
    { 30, 0, 1, 1, 1, 222, 0, 0, 1, 0, 1, 0, 0, 0, 1 },
    { 30, 1, 1, 0, 0, 222, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 30, 0, 1, 0, 0, 222, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 57, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 57, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 73, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 73, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 69, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 69, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 69, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 70, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 70, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 71, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 71, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 72, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 72, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 74, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 74, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 75, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 75, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 76, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 76, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 81, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0 },
    { 81, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 84, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0 },
    { 84, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 83, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 83, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 85, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
    { 85, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 80, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 80, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 94, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 94, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 101, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 93, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 92, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 9, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 91, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 9, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

static const SCRIPT_PROPERTIES *script_props[] =
{
    &props[0], &props[1], &props[2], &props[3],
    &props[4], &props[5], &props[6], &props[7],
    &props[8], &props[9], &props[11], &props[12],
    &props[13], &props[14], &props[15], &props[16],
    &props[17], &props[18], &props[19], &props[20],
    &props[21], &props[22], &props[23], &props[24],
    &props[25], &props[26], &props[27], &props[28],
    &props[29], &props[30], &props[31], &props[32],
    &props[33], &props[34], &props[35], &props[36],
    &props[37], &props[38], &props[39], &props[40],
    &props[41], &props[42], &props[43], &props[44],
    &props[45], &props[46], &props[47], &props[48],
    &props[49], &props[50], &props[51], &props[52],
    &props[53], &props[54], &props[55], &props[56],
    &props[57], &props[58], &props[59], &props[60],
    &props[61], &props[62], &props[63], &props[64],
    &props[65], &props[66], &props[67], &props[68],
    &props[69], &props[70], &props[71], &props[72],
    &props[73]
};

typedef struct {
    HDC hdc;
    LONG height;
    WCHAR default_char;
} ScriptCache;

typedef struct {
    int numGlyphs;
    WORD* glyphs;
    WORD* pwLogClust;
    int* piAdvance;
    SCRIPT_VISATTR* psva;
    GOFFSET* pGoffset;
    ABC* abc;
} StringGlyphs;

typedef struct {
    BOOL invalid;
    int clip_len;
    ScriptCache *sc;
    int cItems;
    int cMaxGlyphs;
    SCRIPT_ITEM* pItem;
    int numItems;
    StringGlyphs* glyphs;
    SCRIPT_LOGATTR* logattrs;
    SIZE* sz;
} StringAnalysis;

static inline void *usp_alloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static inline void *usp_zero_alloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static inline void *usp_zero_realloc(LPVOID mem, SIZE_T size)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size);
}

static inline void usp_free(LPVOID mem)
{
    HeapFree(GetProcessHeap(), 0, mem);
}

static HDC get_cache_hdc(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->hdc;
}

static WCHAR get_cache_default_char(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->default_char;
}

static LONG get_cache_height(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->height;
}

static HRESULT init_script_cache(const HDC hdc, ScriptCache *sc)
{
    TEXTMETRICW metric;

    if (!GetTextMetricsW(hdc, &metric)) return E_INVALIDARG;
    sc->height = metric.tmHeight;
    sc->default_char = metric.tmDefaultChar;
    sc->hdc = hdc;
    return S_OK;
}

static HRESULT get_script_cache(const HDC hdc, SCRIPT_CACHE *psc)
{
    if (!psc) return E_INVALIDARG;
    if (!*psc)
    {
        HRESULT ret;
        ScriptCache *sc;

        if (!hdc) return E_PENDING;
        if (!(sc = usp_zero_alloc(sizeof(ScriptCache)))) return E_OUTOFMEMORY;
        if ((ret = init_script_cache(hdc, sc)))
        {
            usp_free(sc);
            return ret;
        }
        *psc = sc;
    }
    TRACE("<- %p\n", *psc);
    return S_OK;
}

/***********************************************************************
 *      DllMain
 *
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/***********************************************************************
 *      ScriptFreeCache (USP10.@)
 *
 * Free a script cache.
 *
 * PARAMS
 *   psc [I/O] Script cache.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptFreeCache(SCRIPT_CACHE *psc)
{
    TRACE("%p\n", psc);

    if (psc)
    {
       usp_free(*psc);
       *psc = NULL;
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptGetProperties (USP10.@)
 *
 * Retrieve a list of script properties.
 *
 * PARAMS
 *  props [I] Pointer to an array of SCRIPT_PROPERTIES pointers.
 *  num   [I] Pointer to the number of scripts.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 *
 * NOTES
 *  Behaviour matches WinXP.
 */
HRESULT WINAPI ScriptGetProperties(const SCRIPT_PROPERTIES ***props, int *num)
{
    TRACE("(%p,%p)\n", props, num);

    if (!props && !num) return E_INVALIDARG;

    if (num) *num = sizeof(script_props)/sizeof(script_props[0]);
    if (props) *props = script_props;

    return S_OK;
}

/***********************************************************************
 *      ScriptGetFontProperties (USP10.@)
 *
 * Get information on special glyphs.
 *
 * PARAMS
 *  hdc [I]   Device context.
 *  psc [I/O] Opaque pointer to a script cache.
 *  sfp [O]   Font properties structure.
 */
HRESULT WINAPI ScriptGetFontProperties(HDC hdc, SCRIPT_CACHE *psc, SCRIPT_FONTPROPERTIES *sfp)
{
    HRESULT hr;

    TRACE("%p,%p,%p\n", hdc, psc, sfp);

    if (!sfp) return E_INVALIDARG;
    if ((hr = get_script_cache(hdc, psc))) return hr;

    if (sfp->cBytes != sizeof(SCRIPT_FONTPROPERTIES))
        return E_INVALIDARG;

    /* return something sensible? */
    sfp->wgBlank = 0;
    sfp->wgDefault = get_cache_default_char(psc);
    sfp->wgInvalid = 0;
    sfp->wgKashida = 0xffff;
    sfp->iKashidaWidth = 0;

    return S_OK;
}

/***********************************************************************
 *      ScriptRecordDigitSubstitution (USP10.@)
 *
 *  Record digit substitution settings for a given locale.
 *
 *  PARAMS
 *   locale [I] Locale identifier.
 *   sds    [I] Structure to record substitution settings.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: E_POINTER if sds is NULL, E_INVALIDARG otherwise.
 *
 *  SEE ALSO
 *   http://blogs.msdn.com/michkap/archive/2006/02/22/536877.aspx
 */
HRESULT WINAPI ScriptRecordDigitSubstitution(LCID locale, SCRIPT_DIGITSUBSTITUTE *sds)
{
    DWORD plgid, sub;

    TRACE("0x%x, %p\n", locale, sds);

    /* This implementation appears to be correct for all languages, but it's
     * not clear if sds->DigitSubstitute is ever set to anything except
     * CONTEXT or NONE in reality */

    if (!sds) return E_POINTER;

    locale = ConvertDefaultLocale(locale);

    if (!IsValidLocale(locale, LCID_INSTALLED))
        return E_INVALIDARG;

    plgid = PRIMARYLANGID(LANGIDFROMLCID(locale));
    sds->TraditionalDigitLanguage = plgid;

    if (plgid == LANG_ARABIC || plgid == LANG_FARSI)
        sds->NationalDigitLanguage = plgid;
    else
        sds->NationalDigitLanguage = LANG_ENGLISH;

    if (!GetLocaleInfoW(locale, LOCALE_IDIGITSUBSTITUTION | LOCALE_RETURN_NUMBER,
                        (LPWSTR)&sub, sizeof(sub)/sizeof(WCHAR))) return E_INVALIDARG;

    switch (sub)
    {
    case 0:
        if (plgid == LANG_ARABIC || plgid == LANG_FARSI)
            sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_CONTEXT;
        else
            sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
        break;
    case 1:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
        break;
    case 2:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NATIONAL;
        break;
    default:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_TRADITIONAL;
        break;
    }

    sds->dwReserved = 0;
    return S_OK;
}

/***********************************************************************
 *      ScriptApplyDigitSubstitution (USP10.@)
 *
 *  Apply digit substitution settings.
 *
 *  PARAMS
 *   sds [I] Structure with recorded substitution settings.
 *   sc  [I] Script control structure.
 *   ss  [I] Script state structure.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: E_INVALIDARG if sds is invalid. Otherwise an HRESULT.
 */
HRESULT WINAPI ScriptApplyDigitSubstitution(const SCRIPT_DIGITSUBSTITUTE *sds,
                                            SCRIPT_CONTROL *sc, SCRIPT_STATE *ss)
{
    SCRIPT_DIGITSUBSTITUTE psds;

    TRACE("%p, %p, %p\n", sds, sc, ss);

    if (!sc || !ss) return E_POINTER;
    if (!sds)
    {
        sds = &psds;
        if (ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &psds) != S_OK)
            return E_INVALIDARG;
    }

    sc->uDefaultLanguage = LANG_ENGLISH;
    sc->fContextDigits = 0;
    ss->fDigitSubstitute = 0;

    switch (sds->DigitSubstitute) {
        case SCRIPT_DIGITSUBSTITUTE_CONTEXT:
        case SCRIPT_DIGITSUBSTITUTE_NATIONAL:
        case SCRIPT_DIGITSUBSTITUTE_NONE:
        case SCRIPT_DIGITSUBSTITUTE_TRADITIONAL:
            return S_OK;
        default:
            return E_INVALIDARG;
    }
}

/***********************************************************************
 *      ScriptItemize (USP10.@)
 *
 * Split a Unicode string into shapeable parts.
 *
 * PARAMS
 *  pwcInChars [I] String to split.
 *  cInChars   [I] Number of characters in pwcInChars.
 *  cMaxItems  [I] Maximum number of items to return.
 *  psControl  [I] Pointer to a SCRIPT_CONTROL structure.
 *  psState    [I] Pointer to a SCRIPT_STATE structure.
 *  pItems     [O] Buffer to receive SCRIPT_ITEM structures.
 *  pcItems    [O] Number of script items returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptItemize(const WCHAR *pwcInChars, int cInChars, int cMaxItems,
                             const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState,
                             SCRIPT_ITEM *pItems, int *pcItems)
{

#define Numeric_start 0x0030
#define Numeric_stop  0x0039
#define Numeric_space 0x0020
#define Arabic_start  0x0600
#define Arabic_stop   0x06ff
#define Latin_start   0x0001
#define Latin_stop    0x024f
#define Script_Arabic  6
#define Script_Latin   1
#define Script_Numeric 5

    int   cnt = 0, index = 0;
    int   New_Script = SCRIPT_UNDEFINED;

    TRACE("%s,%d,%d,%p,%p,%p,%p\n", debugstr_wn(pwcInChars, cInChars), cInChars, cMaxItems,
          psControl, psState, pItems, pcItems);

    if (!pwcInChars || !cInChars || !pItems || cMaxItems < 2)
        return E_INVALIDARG;

    pItems[index].iCharPos = 0;
    memset(&pItems[index].a, 0, sizeof(SCRIPT_ANALYSIS));

    if  (pwcInChars[cnt] >= Numeric_start && pwcInChars[cnt] <= Numeric_stop)
        pItems[index].a.eScript = Script_Numeric;
    else
    if  (pwcInChars[cnt] >= Arabic_start && pwcInChars[cnt] <= Arabic_stop)
        pItems[index].a.eScript = Script_Arabic;
    else
    if  (pwcInChars[cnt] >= Latin_start && pwcInChars[cnt] <= Latin_stop)
        pItems[index].a.eScript = Script_Latin;

    if  (pItems[index].a.eScript  == Script_Arabic)
        pItems[index].a.s.uBidiLevel = 1;

    TRACE("New_Script=%d, eScript=%d index=%d cnt=%d iCharPos=%d\n",
          New_Script, pItems[index].a.eScript, index, cnt,
          pItems[index].iCharPos = cnt);

    for (cnt=0; cnt < cInChars; cnt++)
    {
        if  ((pwcInChars[cnt] >= Numeric_start && pwcInChars[cnt] <= Numeric_stop)
             || (New_Script == Script_Numeric && pwcInChars[cnt] == Numeric_space))
            New_Script = Script_Numeric;
        else
        if  ((pwcInChars[cnt] >= Arabic_start && pwcInChars[cnt] <= Arabic_stop)
             || (New_Script == Script_Arabic && pwcInChars[cnt] == Numeric_space))
            New_Script = Script_Arabic;
        else
        if  ((WCHAR) pwcInChars[cnt] >= Latin_start && (WCHAR) pwcInChars[cnt] <= Latin_stop)
            New_Script = Script_Latin;
        else
            New_Script = SCRIPT_UNDEFINED;

        if  (New_Script != pItems[index].a.eScript)
        {
            TRACE("New_Script=%d, eScript=%d ", New_Script, pItems[index].a.eScript);
            index++;
            if  (index+1 > cMaxItems)
                return E_OUTOFMEMORY;

            pItems[index].iCharPos = cnt;
            memset(&pItems[index].a, 0, sizeof(SCRIPT_ANALYSIS));

            if  (New_Script == Script_Arabic)
                pItems[index].a.s.uBidiLevel = 1;

            pItems[index].a.eScript = New_Script;
            if  (New_Script == Script_Arabic)
                pItems[index].a.s.uBidiLevel = 1;

            TRACE("index=%d cnt=%d iCharPos=%d\n", index, cnt, pItems[index].iCharPos = cnt);
        }
    }

    /* While not strictly necessary according to the spec, make sure the n+1
     * item is set up to prevent random behaviour if the caller erroneously
     * checks the n+1 structure                                              */
    memset(&pItems[index+1].a, 0, sizeof(SCRIPT_ANALYSIS));

    TRACE("index=%d cnt=%d iCharPos=%d\n", index+1, cnt, pItems[index+1].iCharPos = cnt);

    /*  Set one SCRIPT_STATE item being returned  */
    *pcItems = index + 1;

    /*  Set SCRIPT_ITEM                                     */
    pItems[index+1].iCharPos = cnt;       /* the last + 1 item
                                             contains the ptr to the lastchar */
    return S_OK;
}

/***********************************************************************
 *      ScriptStringAnalyse (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringAnalyse(HDC hdc, const void *pString, int cString,
                                   int cGlyphs, int iCharset, DWORD dwFlags,
                                   int iReqWidth, SCRIPT_CONTROL *psControl,
                                   SCRIPT_STATE *psState, const int *piDx,
                                   SCRIPT_TABDEF *pTabdef, const BYTE *pbInClass,
                                   SCRIPT_STRING_ANALYSIS *pssa)
{
    HRESULT hr = E_OUTOFMEMORY;
    StringAnalysis *analysis = NULL;
    int i, num_items = 255;

    TRACE("(%p,%p,%d,%d,%d,0x%x,%d,%p,%p,%p,%p,%p,%p)\n",
          hdc, pString, cString, cGlyphs, iCharset, dwFlags, iReqWidth,
          psControl, psState, piDx, pTabdef, pbInClass, pssa);

    if (iCharset != -1)
    {
        FIXME("Only Unicode strings are supported\n");
        return E_INVALIDARG;
    }
    if (cString < 1 || !pString) return E_INVALIDARG;
    if ((dwFlags & SSA_GLYPHS) && !hdc) return E_PENDING;

    if (!(analysis = usp_zero_alloc(sizeof(StringAnalysis)))) return E_OUTOFMEMORY;
    if (!(analysis->pItem = usp_zero_alloc(num_items * sizeof(SCRIPT_ITEM) + 1))) goto error;

    /* FIXME: handle clipping */
    analysis->clip_len = cString;

    hr = ScriptItemize(pString, cString, num_items, psControl, psState, analysis->pItem,
                       &analysis->numItems);

    while (hr == E_OUTOFMEMORY)
    {
        SCRIPT_ITEM *tmp;

        num_items *= 2;
        if (!(tmp = usp_zero_realloc(analysis->pItem, num_items * sizeof(SCRIPT_ITEM) + 1)))
            goto error;

        analysis->pItem = tmp;
        hr = ScriptItemize(pString, cString, num_items, psControl, psState, analysis->pItem,
                           &analysis->numItems);
    }
    if (hr) goto error;

    if ((analysis->logattrs = usp_alloc(sizeof(SCRIPT_LOGATTR) * cString)))
        ScriptBreak(pString, cString, (SCRIPT_STRING_ANALYSIS)analysis, analysis->logattrs);
    else
        goto error;

    if (!(analysis->glyphs = usp_zero_alloc(sizeof(StringGlyphs) * analysis->numItems)))
        goto error;

    for (i = 0; i < analysis->numItems; i++)
    {
        SCRIPT_CACHE *sc = (SCRIPT_CACHE *)&analysis->sc;
        int cChar = analysis->pItem[i+1].iCharPos - analysis->pItem[i].iCharPos;
        int numGlyphs = 1.5 * cChar + 16;
        WORD *glyphs = usp_zero_alloc(sizeof(WORD) * numGlyphs);
        WORD *pwLogClust = usp_zero_alloc(sizeof(WORD) * cChar);
        int *piAdvance = usp_zero_alloc(sizeof(int) * numGlyphs);
        SCRIPT_VISATTR *psva = usp_zero_alloc(sizeof(SCRIPT_VISATTR) * cChar);
        GOFFSET *pGoffset = usp_zero_alloc(sizeof(GOFFSET) * numGlyphs);
        ABC *abc = usp_zero_alloc(sizeof(ABC));
        int numGlyphsReturned;

        /* FIXME: non unicode strings */
        WCHAR* pStr = (WCHAR*)pString;
        hr = ScriptShape(hdc, sc, &pStr[analysis->pItem[i].iCharPos],
                         cChar, numGlyphs, &analysis->pItem[i].a,
                         glyphs, pwLogClust, psva, &numGlyphsReturned);
        hr = ScriptPlace(hdc, sc, glyphs, numGlyphsReturned, psva, &analysis->pItem[i].a,
                         piAdvance, pGoffset, abc);

        analysis->glyphs[i].numGlyphs = numGlyphsReturned;
        analysis->glyphs[i].glyphs = glyphs;
        analysis->glyphs[i].pwLogClust = pwLogClust;
        analysis->glyphs[i].piAdvance = piAdvance;
        analysis->glyphs[i].psva = psva;
        analysis->glyphs[i].pGoffset = pGoffset;
        analysis->glyphs[i].abc = abc;
    }

    *pssa = analysis;
    return S_OK;

error:
    usp_free(analysis->glyphs);
    usp_free(analysis->logattrs);
    usp_free(analysis->pItem);
    usp_free(analysis);
    return hr;
}

/***********************************************************************
 *      ScriptStringOut (USP10.@)
 *
 * This function takes the output of ScriptStringAnalyse and joins the segments
 * of glyphs and passes the resulting string to ScriptTextOut.  ScriptStringOut
 * only processes glyphs.
 *
 * Parameters:
 *  ssa       [I] buffer to hold the analysed string components
 *  iX        [I] X axis displacement for output
 *  iY        [I] Y axis displacement for output
 *  uOptions  [I] flags controling output processing
 *  prc       [I] rectangle coordinates
 *  iMinSel   [I] starting pos for substringing output string
 *  iMaxSel   [I] ending pos for substringing output string
 *  fDisabled [I] controls text highlighting
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: is the value returned by ScriptTextOut
 */
HRESULT WINAPI ScriptStringOut(SCRIPT_STRING_ANALYSIS ssa,
                               int iX,
                               int iY,
                               UINT uOptions,
                               const RECT *prc,
                               int iMinSel,
                               int iMaxSel,
                               BOOL fDisabled)
{
    StringAnalysis *analysis;
    WORD *glyphs;
    int   item, cnt, x;
    HRESULT hr;

    TRACE("(%p,%d,%d,0x%1x,%p,%d,%d,%d)\n",
         ssa, iX, iY, uOptions, prc, iMinSel, iMaxSel, fDisabled);

    if (!(analysis = ssa)) return E_INVALIDARG;

    /*
     * Get storage for the output buffer for the consolidated strings
     */
    cnt = 0;
    for (item = 0; item < analysis->numItems; item++)
    {
        cnt += analysis->glyphs[item].numGlyphs;
    }
    if (!(glyphs = usp_alloc(sizeof(WCHAR) * cnt))) return E_OUTOFMEMORY;

    /*
     * ScriptStringOut only processes glyphs hence set ETO_GLYPH_INDEX
     */
    uOptions |= ETO_GLYPH_INDEX;
    analysis->pItem[0].a.fNoGlyphIndex = FALSE; /* say that we have glyphs */

    /*
     * Copy the string items into the output buffer
     */

    TRACE("numItems %d\n", analysis->numItems);

    cnt = 0;
    for (item = 0; item < analysis->numItems; item++)
    {
        memcpy(&glyphs[cnt], analysis->glyphs[item].glyphs,
              sizeof(WCHAR) * analysis->glyphs[item].numGlyphs);

        TRACE("Item %d, Glyphs %d ", item, analysis->glyphs[item].numGlyphs);
        for (x = cnt; x < analysis->glyphs[item].numGlyphs + cnt; x ++)
            TRACE("%04x", glyphs[x]);
        TRACE("\n");

        cnt += analysis->glyphs[item].numGlyphs; /* point to the end of the copied text */
    }

    hr = ScriptTextOut(analysis->sc->hdc, (SCRIPT_CACHE *)&analysis->sc, iX, iY,
                       uOptions, prc, &analysis->pItem->a, NULL, 0, glyphs, cnt,
                       analysis->glyphs->piAdvance, NULL, analysis->glyphs->pGoffset);
    TRACE("ScriptTextOut hr=%08x\n", hr);

    /*
     * Free the output buffer and script cache
     */
    usp_free(glyphs);
    return hr;
}

/***********************************************************************
 *      ScriptStringCPtoX (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringCPtoX(SCRIPT_STRING_ANALYSIS ssa, int icp, BOOL fTrailing, int* pX)
{
    int i, j;
    int runningX = 0;
    int runningCp = 0;
    StringAnalysis* analysis = ssa;

    TRACE("(%p), %d, %d, (%p)\n", ssa, icp, fTrailing, pX);

    if (!ssa || !pX) return S_FALSE;

    /* icp out of range */
    if(icp < 0)
    {
        analysis->invalid = TRUE;
        return E_INVALIDARG;
    }

    for(i=0; i<analysis->numItems; i++)
    {
        for(j=0; j<analysis->glyphs[i].numGlyphs; j++)
        {
            if(runningCp == icp && fTrailing == FALSE)
            {
                *pX = runningX;
                return S_OK;
            }
            runningX += analysis->glyphs[i].piAdvance[j];
            if(runningCp == icp && fTrailing == TRUE)
            {
                *pX = runningX;
                return S_OK;
            }
            runningCp++;
        }
    }

    /* icp out of range */
    analysis->invalid = TRUE;
    return E_INVALIDARG;
}

/***********************************************************************
 *      ScriptStringXtoCP (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringXtoCP(SCRIPT_STRING_ANALYSIS ssa, int iX, int* piCh, int* piTrailing)
{
    StringAnalysis* analysis = ssa;
    int i;
    int j;
    int runningX = 0;
    int runningCp = 0;
    int width;

    TRACE("(%p), %d, (%p), (%p)\n", ssa, iX, piCh, piTrailing);

    if (!ssa || !piCh || !piTrailing) return S_FALSE;

    /* out of range */
    if(iX < 0)
    {
        *piCh = -1;
        *piTrailing = TRUE;
        return S_OK;
    }

    for(i=0; i<analysis->numItems; i++)
    {
        for(j=0; j<analysis->glyphs[i].numGlyphs; j++)
        {
            width = analysis->glyphs[i].piAdvance[j];
            if(iX < (runningX + width))
            {
                *piCh = runningCp;
                if((iX - runningX) > width/2)
                    *piTrailing = TRUE;
                else
                    *piTrailing = FALSE;
                return S_OK;
            }
            runningX += width;
            runningCp++;
        }
    }

    /* out of range */
    *piCh = analysis->pItem[analysis->numItems].iCharPos;
    *piTrailing = FALSE;

    return S_OK;
}


/***********************************************************************
 *      ScriptStringFree (USP10.@)
 *
 * Free a string analysis.
 *
 * PARAMS
 *  pssa [I] string analysis.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptStringFree(SCRIPT_STRING_ANALYSIS *pssa)
{
    StringAnalysis* analysis;
    BOOL invalid;
    int i;

    TRACE("(%p)\n", pssa);

    if (!pssa || !(analysis = *pssa)) return E_INVALIDARG;
    invalid = analysis->invalid;

    for (i = 0; i < analysis->numItems; i++)
    {
        usp_free(analysis->glyphs[i].glyphs);
        usp_free(analysis->glyphs[i].pwLogClust);
        usp_free(analysis->glyphs[i].piAdvance);
        usp_free(analysis->glyphs[i].psva);
        usp_free(analysis->glyphs[i].pGoffset);
        usp_free(analysis->glyphs[i].abc);
    }

    usp_free(analysis->glyphs);
    usp_free(analysis->pItem);
    usp_free(analysis->logattrs);
    usp_free(analysis->sz);
    usp_free(analysis);

    if (invalid) return E_INVALIDARG;
    return S_OK;
}

/***********************************************************************
 *      ScriptCPtoX (USP10.@)
 *
 */
HRESULT WINAPI ScriptCPtoX(int iCP,
                           BOOL fTrailing,
                           int cChars,
                           int cGlyphs,
                           const WORD *pwLogClust,
                           const SCRIPT_VISATTR *psva,
                           const int *piAdvance,
                           const SCRIPT_ANALYSIS *psa,
                           int *piX)
{
    int  item;
    int  iPosX;
    float  fMaxPosX = 0;
    TRACE("(%d,%d,%d,%d,%p,%p,%p,%p,%p)\n",
          iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance,
          psa, piX);
    for (item=0; item < cGlyphs; item++)            /* total piAdvance           */
        fMaxPosX += piAdvance[item];
    iPosX = (fMaxPosX/cGlyphs)*(iCP+fTrailing);
    if  (iPosX > fMaxPosX)
        iPosX = fMaxPosX;
    *piX = iPosX;                                    /* Return something in range */

    TRACE("*piX=%d\n", *piX);
    return S_OK;
}

/***********************************************************************
 *      ScriptXtoCP (USP10.@)
 *
 */
HRESULT WINAPI ScriptXtoCP(int iX,
                           int cChars,
                           int cGlyphs,
                           const WORD *pwLogClust,
                           const SCRIPT_VISATTR *psva,
                           const int *piAdvance,
                           const SCRIPT_ANALYSIS *psa,
                           int *piCP,
                           int *piTrailing)
{
    int item;
    int iPosX;
    float fMaxPosX = 1;
    float fAvePosX;
    TRACE("(%d,%d,%d,%p,%p,%p,%p,%p,%p)\n",
          iX, cChars, cGlyphs, pwLogClust, psva, piAdvance,
          psa, piCP, piTrailing);
    if  (iX < 0)                                    /* iX is before start of run */
    {
        *piCP = -1;
        *piTrailing = TRUE;
        return S_OK;
    }

    for (item=0; item < cGlyphs; item++)            /* total piAdvance           */
        fMaxPosX += piAdvance[item];

    if  (iX >= fMaxPosX)                            /* iX too large              */
    {
        *piCP = cChars;
        *piTrailing = FALSE;
        return S_OK;
    }

    fAvePosX = fMaxPosX / cGlyphs;
    iPosX = fAvePosX;
    for (item = 1; item < cGlyphs  && iPosX < iX; item++)
        iPosX += fAvePosX;
    if  (iPosX - iX > fAvePosX/2)
        *piTrailing = 0;
    else
        *piTrailing = 1;                            /* yep we are over halfway */

    *piCP = item -1;                                /* Return character position */
    TRACE("*piCP=%d iPposX=%d\n", *piCP, iPosX);
    return S_OK;
}

/***********************************************************************
 *      ScriptBreak (USP10.@)
 *
 *  Retrieve line break information.
 *
 *  PARAMS
 *   chars [I] Array of characters.
 *   sa    [I] String analysis.
 *   la    [I] Array of logical attribute structures.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: S_FALSE
 */
HRESULT WINAPI ScriptBreak(const WCHAR *chars, int count, const SCRIPT_ANALYSIS *sa, SCRIPT_LOGATTR *la)
{
    unsigned int i;

    TRACE("(%s, %d, %p, %p)\n", debugstr_wn(chars, count), count, sa, la);

    if (!la) return S_FALSE;

    for (i = 0; i < count; i++)
    {
        memset(&la[i], 0, sizeof(SCRIPT_LOGATTR));

        /* FIXME: set the other flags */
        la[i].fWhiteSpace = isspaceW(chars[i]);
        la[i].fCharStop = 1;
    }
    return S_OK;
}

static const struct
{
    WCHAR start;
    WCHAR end;
    DWORD flag;
}
complex_ranges[] =
{
    { 0, 0x0b, SIC_COMPLEX },
    { 0x0c, 0x0c, SIC_NEUTRAL },
    { 0x0d, 0x1f, SIC_COMPLEX },
    { 0x20, 0x2f, SIC_NEUTRAL },
    { 0x30, 0x39, SIC_ASCIIDIGIT },
    { 0x3a, 0x40, SIC_NEUTRAL },
    { 0x5b, 0x60, SIC_NEUTRAL },
    { 0x7b, 0x7e, SIC_NEUTRAL },
    { 0x7f, 0x9f, SIC_COMPLEX },
    { 0xa0, 0xa5, SIC_NEUTRAL },
    { 0xa7, 0xa8, SIC_NEUTRAL },
    { 0xab, 0xab, SIC_NEUTRAL },
    { 0xad, 0xad, SIC_NEUTRAL },
    { 0xaf, 0xaf, SIC_NEUTRAL },
    { 0xb0, 0xb1, SIC_NEUTRAL },
    { 0xb4, 0xb4, SIC_NEUTRAL },
    { 0xb6, 0xb8, SIC_NEUTRAL },
    { 0xbb, 0xbf, SIC_NEUTRAL },
    { 0xd7, 0xd7, SIC_NEUTRAL },
    { 0xf7, 0xf7, SIC_NEUTRAL },
    { 0x2b9, 0x2ba, SIC_NEUTRAL },
    { 0x2c2, 0x2cf, SIC_NEUTRAL },
    { 0x2d2, 0x2df, SIC_NEUTRAL },
    { 0x2e5, 0x2e9, SIC_COMPLEX },
    { 0x2ea, 0x2ed, SIC_NEUTRAL },
    { 0x300, 0x362, SIC_COMPLEX },
    { 0x530, 0x60b, SIC_COMPLEX },
    { 0x60c, 0x60d, SIC_NEUTRAL },
    { 0x60e, 0x669, SIC_COMPLEX },
    { 0x66a, 0x66a, SIC_NEUTRAL },
    { 0x66b, 0x6e8, SIC_COMPLEX },
    { 0x6e9, 0x6e9, SIC_NEUTRAL },
    { 0x6ea, 0x7bf, SIC_COMPLEX },
    { 0x900, 0x1360, SIC_COMPLEX },
    { 0x137d, 0x137f, SIC_COMPLEX },
    { 0x1680, 0x1680, SIC_NEUTRAL },
    { 0x1780, 0x18af, SIC_COMPLEX },
    { 0x2000, 0x200a, SIC_NEUTRAL },
    { 0x200b, 0x200f, SIC_COMPLEX },
    { 0x2010, 0x2016, SIC_NEUTRAL },
    { 0x2018, 0x2022, SIC_NEUTRAL },
    { 0x2024, 0x2028, SIC_NEUTRAL },
    { 0x2029, 0x202e, SIC_COMPLEX },
    { 0x202f, 0x2037, SIC_NEUTRAL },
    { 0x2039, 0x203c, SIC_NEUTRAL },
    { 0x2044, 0x2046, SIC_NEUTRAL },
    { 0x206a, 0x206f, SIC_COMPLEX },
    { 0x207a, 0x207e, SIC_NEUTRAL },
    { 0x208a, 0x20aa, SIC_NEUTRAL },
    { 0x20ac, 0x20cf, SIC_NEUTRAL },
    { 0x20d0, 0x20ff, SIC_COMPLEX },
    { 0x2103, 0x2103, SIC_NEUTRAL },
    { 0x2105, 0x2105, SIC_NEUTRAL },
    { 0x2109, 0x2109, SIC_NEUTRAL },
    { 0x2116, 0x2116, SIC_NEUTRAL },
    { 0x2121, 0x2122, SIC_NEUTRAL },
    { 0x212e, 0x212e, SIC_NEUTRAL },
    { 0x2153, 0x2154, SIC_NEUTRAL },
    { 0x215b, 0x215e, SIC_NEUTRAL },
    { 0x2190, 0x2199, SIC_NEUTRAL },
    { 0x21b8, 0x21b9, SIC_NEUTRAL },
    { 0x21d2, 0x21d2, SIC_NEUTRAL },
    { 0x21d4, 0x21d4, SIC_NEUTRAL },
    { 0x21e7, 0x21e7, SIC_NEUTRAL },
    { 0x2200, 0x2200, SIC_NEUTRAL },
    { 0x2202, 0x2203, SIC_NEUTRAL },
    { 0x2207, 0x2208, SIC_NEUTRAL },
    { 0x220b, 0x220b, SIC_NEUTRAL },
    { 0x220f, 0x220f, SIC_NEUTRAL },
    { 0x2211, 0x2213, SIC_NEUTRAL },
    { 0x2215, 0x2215, SIC_NEUTRAL },
    { 0x221a, 0x221a, SIC_NEUTRAL },
    { 0x221d, 0x2220, SIC_NEUTRAL },
    { 0x2223, 0x2223, SIC_NEUTRAL },
    { 0x2225, 0x2225, SIC_NEUTRAL },
    { 0x2227, 0x222c, SIC_NEUTRAL },
    { 0x222e, 0x222e, SIC_NEUTRAL },
    { 0x2234, 0x2237, SIC_NEUTRAL },
    { 0x223c, 0x223d, SIC_NEUTRAL },
    { 0x2248, 0x2248, SIC_NEUTRAL },
    { 0x224c, 0x224c, SIC_NEUTRAL },
    { 0x2252, 0x2252, SIC_NEUTRAL },
    { 0x2260, 0x2261, SIC_NEUTRAL },
    { 0x2264, 0x2267, SIC_NEUTRAL },
    { 0x226a, 0x226b, SIC_NEUTRAL },
    { 0x226e, 0x226f, SIC_NEUTRAL },
    { 0x2282, 0x2283, SIC_NEUTRAL },
    { 0x2286, 0x2287, SIC_NEUTRAL },
    { 0x2295, 0x2295, SIC_NEUTRAL },
    { 0x2299, 0x2299, SIC_NEUTRAL },
    { 0x22a5, 0x22a5, SIC_NEUTRAL },
    { 0x22bf, 0x22bf, SIC_NEUTRAL },
    { 0x2312, 0x2312, SIC_NEUTRAL },
    { 0x24ea, 0x24ea, SIC_COMPLEX },
    { 0x2500, 0x254b, SIC_NEUTRAL },
    { 0x2550, 0x256d, SIC_NEUTRAL },
    { 0x256e, 0x2574, SIC_NEUTRAL },
    { 0x2581, 0x258f, SIC_NEUTRAL },
    { 0x2592, 0x2595, SIC_NEUTRAL },
    { 0x25a0, 0x25a1, SIC_NEUTRAL },
    { 0x25a3, 0x25a9, SIC_NEUTRAL },
    { 0x25b2, 0x25b3, SIC_NEUTRAL },
    { 0x25b6, 0x25b7, SIC_NEUTRAL },
    { 0x25bc, 0x25bd, SIC_NEUTRAL },
    { 0x25c0, 0x25c1, SIC_NEUTRAL },
    { 0x25c6, 0x25c8, SIC_NEUTRAL },
    { 0x25cb, 0x25cb, SIC_NEUTRAL },
    { 0x25ce, 0x25d1, SIC_NEUTRAL },
    { 0x25e2, 0x25e5, SIC_NEUTRAL },
    { 0x25ef, 0x25ef, SIC_NEUTRAL },
    { 0x2605, 0x2606, SIC_NEUTRAL },
    { 0x2609, 0x2609, SIC_NEUTRAL },
    { 0x260e, 0x260f, SIC_NEUTRAL },
    { 0x261c, 0x261c, SIC_NEUTRAL },
    { 0x261e, 0x261e, SIC_NEUTRAL },
    { 0x2640, 0x2640, SIC_NEUTRAL },
    { 0x2642, 0x2642, SIC_NEUTRAL },
    { 0x2660, 0x2661, SIC_NEUTRAL },
    { 0x2663, 0x2665, SIC_NEUTRAL },
    { 0x2667, 0x266a, SIC_NEUTRAL },
    { 0x266c, 0x266d, SIC_NEUTRAL },
    { 0x266f, 0x266f, SIC_NEUTRAL },
    { 0x273d, 0x273d, SIC_NEUTRAL },
    { 0x2e80, 0x312f, SIC_COMPLEX },
    { 0x3190, 0x31bf, SIC_COMPLEX },
    { 0x31f0, 0x31ff, SIC_COMPLEX },
    { 0x3220, 0x325f, SIC_COMPLEX },
    { 0x3280, 0xa4ff, SIC_COMPLEX },
    { 0xd800, 0xdfff, SIC_COMPLEX },
    { 0xe000, 0xf8ff, SIC_NEUTRAL },
    { 0xf900, 0xfaff, SIC_COMPLEX },
    { 0xfb13, 0xfb28, SIC_COMPLEX },
    { 0xfb29, 0xfb29, SIC_NEUTRAL },
    { 0xfb2a, 0xfb4f, SIC_COMPLEX },
    { 0xfd3e, 0xfd3f, SIC_NEUTRAL },
    { 0xfdd0, 0xfdef, SIC_COMPLEX },
    { 0xfe20, 0xfe6f, SIC_COMPLEX },
    { 0xfeff, 0xfeff, SIC_COMPLEX },
    { 0xff01, 0xff5e, SIC_COMPLEX },
    { 0xff61, 0xff9f, SIC_COMPLEX },
    { 0xffe0, 0xffe6, SIC_COMPLEX },
    { 0xffe8, 0xffee, SIC_COMPLEX },
    { 0xfff9, 0xfffb, SIC_COMPLEX },
    { 0xfffe, 0xfffe, SIC_COMPLEX }
};

/***********************************************************************
 *      ScriptIsComplex (USP10.@)
 *
 *  Determine if a string is complex.
 *
 *  PARAMS
 *   chars [I] Array of characters to test.
 *   len   [I] Length in characters.
 *   flag  [I] Flag.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: S_FALSE
 *
 *  NOTES
 *   Behaviour matches that of WinXP.
 */
HRESULT WINAPI ScriptIsComplex(const WCHAR *chars, int len, DWORD flag)
{
    unsigned int i, j;

    TRACE("(%s,%d,0x%x)\n", debugstr_wn(chars, len), len, flag);

    for (i = 0; i < len; i++)
    {
        for (j = 0; j < sizeof(complex_ranges)/sizeof(complex_ranges[0]); j++)
        {
            if (chars[i] >= complex_ranges[j].start &&
                chars[i] <= complex_ranges[j].end &&
                (flag & complex_ranges[j].flag)) return S_OK;
        }
    }
    return S_FALSE;
}

/***********************************************************************
 *      ScriptShape (USP10.@)
 *
 * Produce glyphs and visual attributes for a run.
 *
 * PARAMS
 *  hdc         [I]   Device context.
 *  psc         [I/O] Opaque pointer to a script cache.
 *  pwcChars    [I]   Array of characters specifying the run.
 *  cChars      [I]   Number of characters in pwcChars.
 *  cMaxGlyphs  [I]   Length of pwOutGlyphs.
 *  psa         [I/O] String analysis.
 *  pwOutGlyphs [O]   Array of glyphs.
 *  pwLogClust  [O]   Array of logical cluster info.
 *  psva        [O]   Array of visual attributes.
 *  pcGlyphs    [O]   Number of glyphs returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptShape(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcChars,
                           int cChars, int cMaxGlyphs,
                           SCRIPT_ANALYSIS *psa, WORD *pwOutGlyphs, WORD *pwLogClust,
                           SCRIPT_VISATTR *psva, int *pcGlyphs)
{
    int cnt;
    HRESULT hr;
    *pcGlyphs = cChars;

    TRACE("(%p, %p, %p, %d, %d, %p)\n",  hdc, psc, pwcChars, cChars, cMaxGlyphs, psa);
    if (psa) TRACE("psa values: %d, %d, %d, %d, %d, %d, %d\n", psa->eScript, psa->fRTL, psa->fLayoutRTL,
                   psa->fLinkBefore, psa->fLinkAfter, psa->fLogicalOrder, psa->fNoGlyphIndex);

    if (cChars > cMaxGlyphs) return E_OUTOFMEMORY;
    if ((hr = get_script_cache(hdc, psc))) return hr;

    TRACE("Before: ");
    for (cnt = 0; cnt < cChars; cnt++)
         TRACE("%4x",pwcChars[cnt]);
    TRACE("\n");

    if  (!psa->fNoGlyphIndex) {                                         /* Glyph translate */
        if (!(GetGlyphIndicesW((HDC)get_cache_hdc(psc), (LPCWSTR) pwcChars, (int)cChars, (LPWORD) pwOutGlyphs,(DWORD) 0)))
            return S_FALSE;

        TRACE("After:  ");
        for (cnt = 0; cnt < cChars; cnt++) {
             TRACE("%04x",pwOutGlyphs[cnt]);
        }
        TRACE("\n");
    }
    else {
        TRACE("After:  ");
        for (cnt = 0; cnt < cChars; cnt++) {                           /* no translate so set up */
             pwOutGlyphs[cnt] = pwcChars[cnt];                         /* copy in to out and     */
             TRACE("%04x",pwOutGlyphs[cnt]);
        }
       TRACE("\n");
    }

    /*  Set up a valid SCRIPT_VISATTR and LogClust for each char in this run */
    for (cnt = 0;  cnt < cChars; cnt++) {
        /* FIXME:  set to better values */
         psva[cnt].uJustification = 2;
         psva[cnt].fClusterStart = 1;
         psva[cnt].fDiacritic = 0;
         psva[cnt].fZeroWidth = 0;
         pwLogClust[cnt] = cnt;
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptPlace (USP10.@)
 *
 * Produce advance widths for a run.
 *
 * PARAMS
 *  hdc       [I]   Device context.
 *  psc       [I/O] Opaque pointer to a script cache.
 *  pwGlyphs  [I]   Array of glyphs.
 *  cGlyphs   [I]   Number of glyphs in pwGlyphs.
 *  psva      [I]   Array of visual attributes.
 *  psa       [I/O] String analysis.
 *  piAdvance [O]   Array of advance widths.
 *  pGoffset  [O]   Glyph offsets.
 *  pABC      [O]   Combined ABC width.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptPlace(HDC hdc, SCRIPT_CACHE *psc, const WORD *pwGlyphs,
                           int cGlyphs, const SCRIPT_VISATTR *psva,
                           SCRIPT_ANALYSIS *psa, int *piAdvance, GOFFSET *pGoffset, ABC *pABC )
{
    int wcnt;
    HRESULT hr;
    LPABC lpABC;

    TRACE("(%p, %p, %p, %s, %d, %p, %p, %p)\n",  hdc, psc, pwGlyphs,
          debugstr_wn(pwGlyphs, cGlyphs), cGlyphs, psva, psa, piAdvance);

    if ((hr = get_script_cache(hdc, psc))) return hr;

    /*   Here we need to calculate the width of the run unit.  At this point the input string
     *   has been converted to glyphs and we still need to translate back to the original chars
     *   to get the correct ABC widths.   */

     if (!(lpABC = usp_zero_alloc(sizeof(ABC) * cGlyphs))) return E_OUTOFMEMORY;

     memset(pABC, 0, sizeof(ABC));

    /* FIXME: set pGoffset to more reasonable values */
     if (!GetCharABCWidthsI(get_cache_hdc(psc), 0, cGlyphs, (WORD *) pwGlyphs, lpABC ))
     {
         WARN("Could not get ABC values\n");
         for (wcnt = 0; wcnt < cGlyphs; wcnt++) {
             piAdvance[wcnt] = 0;
             pGoffset[wcnt].du = 0;
             pGoffset[wcnt].dv = 0;
         }
     }
     else
     {
         for (wcnt = 0; wcnt < cGlyphs ; wcnt++) {          /* add up the char lengths  */
             TRACE("     Glyph=%04x,  abcA=%d,  abcB=%d,  abcC=%d  wcnt=%d\n",
                                  pwGlyphs[wcnt],
                                  lpABC[wcnt].abcA,
                                  lpABC[wcnt].abcB,
                                  lpABC[wcnt].abcC, wcnt);
             pABC->abcA += lpABC[wcnt].abcA;
             pABC->abcB += lpABC[wcnt].abcB;
             pABC->abcC += lpABC[wcnt].abcC;
             piAdvance[wcnt] = lpABC[wcnt].abcA + lpABC[wcnt].abcB + lpABC[wcnt].abcC;
             pGoffset[wcnt].du = 0;
             pGoffset[wcnt].dv = 0;
         }
     }
     TRACE("Total for run:   abcA=%d,  abcB=%d,  abcC=%d\n", pABC->abcA, pABC->abcB, pABC->abcC);

     usp_free(lpABC);
     return S_OK;
}

/***********************************************************************
 *      ScriptGetCMap (USP10.@)
 *
 * Retrieve glyph indices.
 *
 * PARAMS
 *  hdc         [I]   Device context.
 *  psc         [I/O] Opaque pointer to a script cache.
 *  pwcInChars  [I]   Array of Unicode characters.
 *  cChars      [I]   Number of characters in pwcInChars.
 *  dwFlags     [I]   Flags.
 *  pwOutGlyphs [O]   Buffer to receive the array of glyph indices.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptGetCMap(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcInChars,
                             int cChars, DWORD dwFlags, WORD *pwOutGlyphs)
{
    int cnt;
    HRESULT hr;

    TRACE("(%p,%p,%s,%d,0x%x,%p)\n", hdc, psc, debugstr_wn(pwcInChars, cChars),
          cChars, dwFlags, pwOutGlyphs);

    if ((hr = get_script_cache(hdc, psc))) return hr;

    TRACE("Before: ");
    for (cnt = 0; cnt < cChars; cnt++)
         TRACE("%4x",pwcInChars[cnt]);
    TRACE("\n");

    GetGlyphIndicesW(get_cache_hdc(psc), pwcInChars, cChars, pwOutGlyphs, 0);

    TRACE("After:  ");
    for (cnt = 0; cnt < cChars; cnt++) {
         TRACE("%04x",pwOutGlyphs[cnt]);
    }
    TRACE("\n");

    return S_OK;
}

/***********************************************************************
 *      ScriptTextOut (USP10.@)
 *
 */
HRESULT WINAPI ScriptTextOut(const HDC hdc, SCRIPT_CACHE *psc, int x, int y, UINT fuOptions,
                             const RECT *lprc, const SCRIPT_ANALYSIS *psa, const WCHAR *pwcReserved,
                             int iReserved, const WORD *pwGlyphs, int cGlyphs, const int *piAdvance,
                             const int *piJustify, const GOFFSET *pGoffset)
{
    HRESULT hr;

    TRACE("(%p, %p, %d, %d, %04x, %p, %p, %p, %d, %p, %d, %p, %p, %p)\n",
         hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved, pwGlyphs, cGlyphs,
         piAdvance, piJustify, pGoffset);

    if (!hdc && psc && !*psc) return E_INVALIDARG;
    if (!piAdvance || !psa || !pwGlyphs) return E_INVALIDARG;
    if ((hr = get_script_cache(hdc, psc))) return hr;

    fuOptions &= ETO_CLIPPED + ETO_OPAQUE;
    if  (!psa->fNoGlyphIndex)                                     /* Have Glyphs?                      */
        fuOptions |= ETO_GLYPH_INDEX;                             /* Say don't do translation to glyph */

    if (!ExtTextOutW(get_cache_hdc(psc), x, y, fuOptions, lprc, pwGlyphs, cGlyphs, NULL))
        return S_FALSE;

    return S_OK;
}

/***********************************************************************
 *      ScriptCacheGetHeight (USP10.@)
 *
 * Retrieve the height of the font in the cache.
 *
 * PARAMS
 *  hdc    [I]    Device context.
 *  psc    [I/O]  Opaque pointer to a script cache.
 *  height [O]    Receives font height.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptCacheGetHeight(HDC hdc, SCRIPT_CACHE *psc, LONG *height)
{
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", hdc, psc, height);

    if (!height) return E_INVALIDARG;
    if ((hr = get_script_cache(hdc, psc))) return hr;

    *height = get_cache_height(psc);
    return S_OK;
}

/***********************************************************************
 *      ScriptGetGlyphABCWidth (USP10.@)
 *
 * Retrieve the width of a glyph.
 *
 * PARAMS
 *  hdc    [I]    Device context.
 *  psc    [I/O]  Opaque pointer to a script cache.
 *  glyph  [I]    Glyph to retrieve the width for.
 *  abc    [O]    ABC widths of the glyph.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptGetGlyphABCWidth(HDC hdc, SCRIPT_CACHE *psc, WORD glyph, ABC *abc)
{
    HRESULT hr;

    TRACE("(%p, %p, 0x%04x, %p)\n", hdc, psc, glyph, abc);

    if ((hr = get_script_cache(hdc, psc))) return hr;

    /* FIXME: get this from the cache */
    if (!GetCharABCWidthsW(get_cache_hdc(psc), glyph, glyph, abc)) return E_HANDLE;
    return S_OK;
}

/***********************************************************************
 *      ScriptLayout (USP10.@)
 *
 * Map embedding levels to visual and/or logical order.
 *
 * PARAMS
 *  runs     [I] Size of level array.
 *  level    [I] Array of embedding levels.
 *  vistolog [O] Map of embedding levels from visual to logical order.
 *  logtovis [O] Map of embedding levels from logical to visual order.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 *
 * BUGS
 *  This stub works correctly for any sequence of a single
 *  embedding level but not for sequences of different
 *  embedding levels, i.e. mixtures of RTL and LTR scripts.
 */
HRESULT WINAPI ScriptLayout(int runs, const BYTE *level, int *vistolog, int *logtovis)
{
    int i, j = runs - 1, k = 0;

    TRACE("(%d, %p, %p, %p)\n", runs, level, vistolog, logtovis);

    if (!level || (!vistolog && !logtovis))
        return E_INVALIDARG;

    for (i = 0; i < runs; i++)
    {
        if (level[i] % 2)
        {
            if (vistolog) *vistolog++ = j;
            if (logtovis) *logtovis++ = j;
            j--;
        }
        else
        {
            if (vistolog) *vistolog++ = k;
            if (logtovis) *logtovis++ = k;
            k++;
        }
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptStringGetLogicalWidths (USP10.@)
 *
 * Returns logical widths from a string analysis.
 *
 * PARAMS
 *  ssa  [I] string analysis.
 *  piDx [O] logical widths returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptStringGetLogicalWidths(SCRIPT_STRING_ANALYSIS ssa, int *piDx)
{
    int i, j, next = 0;
    StringAnalysis *analysis = ssa;

    TRACE("%p, %p\n", ssa, piDx);

    if (!analysis) return S_FALSE;

    for (i = 0; i < analysis->numItems; i++)
    {
        for (j = 0; j < analysis->glyphs[i].numGlyphs; j++)
        {
            piDx[next] = analysis->glyphs[i].piAdvance[j];
            next++;
        }
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptStringValidate (USP10.@)
 *
 * Validate a string analysis.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: S_FALSE if invalid sequences are found
 *           or a non-zero HRESULT if it fails.
 */
HRESULT WINAPI ScriptStringValidate(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return E_INVALIDARG;
    return (analysis->invalid) ? S_FALSE : S_OK;
}

/***********************************************************************
 *      ScriptString_pSize (USP10.@)
 *
 * Retrieve width and height of an analysed string.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: Pointer to a SIZE structure.
 *  Failure: NULL
 */
const SIZE * WINAPI ScriptString_pSize(SCRIPT_STRING_ANALYSIS ssa)
{
    unsigned int i, j;
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;

    if (!analysis->sz)
    {
        if (!(analysis->sz = usp_alloc(sizeof(SIZE)))) return NULL;
        analysis->sz->cy = analysis->sc->height;

        analysis->sz->cx = 0;
        for (i = 0; i < analysis->numItems; i++)
            for (j = 0; j < analysis->glyphs[i].numGlyphs; j++)
                analysis->sz->cx += analysis->glyphs[i].piAdvance[j];
    }
    return analysis->sz;
}

/***********************************************************************
 *      ScriptString_pLogAttr (USP10.@)
 *
 * Retrieve logical attributes of an analysed string.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: Pointer to an array of SCRIPT_LOGATTR structures.
 *  Failure: NULL
 */
const SCRIPT_LOGATTR * WINAPI ScriptString_pLogAttr(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;
    return analysis->logattrs;
}

/***********************************************************************
 *      ScriptString_pcOutChars (USP10.@)
 *
 * Retrieve the length of a string after clipping.
 *
 * PARAMS
 *  ssa [I] String analysis.
 *
 * RETURNS
 *  Success: Pointer to the length.
 *  Failure: NULL
 */
const int * WINAPI ScriptString_pcOutChars(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;
    return &analysis->clip_len;
}

/***********************************************************************
 *      ScriptStringGetOrder (USP10.@)
 *
 * Retrieve a glyph order map.
 *
 * PARAMS
 *  ssa   [I]   String analysis.
 *  order [I/O] Array of glyph positions.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptStringGetOrder(SCRIPT_STRING_ANALYSIS ssa, UINT *order)
{
    unsigned int i, j, k;
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return S_FALSE;

    /* FIXME: handle RTL scripts */
    for (i = 0, k = 0; i < analysis->numItems; i++)
        for (j = 0; j < analysis->glyphs[i].numGlyphs; j++, k++)
            order[k] = k;

    return S_OK;
}
