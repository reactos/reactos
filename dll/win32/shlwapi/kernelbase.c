/*
 * SHLWAPI kernelbase functions
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1998-2000 Juergen Schmied
 * Copyright 2000 Huw D M Davies for CodeWeavers.
 * Copyright 2001-2003 Jon Griffiths
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
 */

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "shlobj.h"
#include "winternl.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "intshcut.h"
#include "mlang.h"
#include <shlwapi_undoc.h>
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(kernelbase);

/*************************************************************************
 *      @	[SHLWAPI.15]
 *
 * Get Explorers "AcceptLanguage" setting.
 *
 * PARAMS
 *  langbuf [O] Destination for language string
 *  buflen  [I] Length of langbuf in characters
 *          [0] Success: used length of langbuf
 *
 * RETURNS
 *  Success: S_OK.   langbuf is set to the language string found.
 *  Failure: E_FAIL, If any arguments are invalid, error occurred, or Explorer
 *           does not contain the setting.
 *           E_NOT_SUFFICIENT_BUFFER, If the buffer is not big enough
 */
HRESULT WINAPI GetAcceptLanguagesW( LPWSTR langbuf, LPDWORD buflen)
{
    static const WCHAR szkeyW[] = {
	'S','o','f','t','w','a','r','e','\\',
	'M','i','c','r','o','s','o','f','t','\\',
	'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\',
	'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
    static const WCHAR valueW[] = {
	'A','c','c','e','p','t','L','a','n','g','u','a','g','e',0};
    DWORD mystrlen, mytype;
    DWORD len;
    HKEY mykey;
    LCID mylcid;
    WCHAR *mystr;
    LONG lres;

    TRACE("(%p, %p) *%p: %d\n", langbuf, buflen, buflen, buflen ? *buflen : -1);

    if(!langbuf || !buflen || !*buflen)
	return E_FAIL;

    mystrlen = (*buflen > 20) ? *buflen : 20 ;
    len = mystrlen * sizeof(WCHAR);
    mystr = HeapAlloc(GetProcessHeap(), 0, len);
    mystr[0] = 0;
    RegOpenKeyW(HKEY_CURRENT_USER, szkeyW, &mykey);
    lres = RegQueryValueExW(mykey, valueW, 0, &mytype, (PBYTE)mystr, &len);
    RegCloseKey(mykey);
    len = lstrlenW(mystr);

    if (!lres && (*buflen > len)) {
        lstrcpyW(langbuf, mystr);
        *buflen = len;
        HeapFree(GetProcessHeap(), 0, mystr);
        return S_OK;
    }

    /* Did not find a value in the registry or the user buffer is too small */
    mylcid = GetUserDefaultLCID();
    LcidToRfc1766W(mylcid, mystr, mystrlen);
    len = lstrlenW(mystr);

    memcpy( langbuf, mystr, min(*buflen, len+1)*sizeof(WCHAR) );
    HeapFree(GetProcessHeap(), 0, mystr);

    if (*buflen > len) {
        *buflen = len;
        return S_OK;
    }

    *buflen = 0;
    return E_NOT_SUFFICIENT_BUFFER;
}

/*************************************************************************
 *      @	[SHLWAPI.14]
 *
 * Ascii version of GetAcceptLanguagesW.
 */
HRESULT WINAPI GetAcceptLanguagesA( LPSTR langbuf, LPDWORD buflen)
{
    WCHAR *langbufW;
    DWORD buflenW, convlen;
    HRESULT retval;

    TRACE("(%p, %p) *%p: %d\n", langbuf, buflen, buflen, buflen ? *buflen : -1);

    if(!langbuf || !buflen || !*buflen) return E_FAIL;

    buflenW = *buflen;
    langbufW = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * buflenW);
    retval = GetAcceptLanguagesW(langbufW, &buflenW);

    if (retval == S_OK)
    {
        convlen = WideCharToMultiByte(CP_ACP, 0, langbufW, -1, langbuf, *buflen, NULL, NULL);
        convlen--;  /* do not count the terminating 0 */
    }
    else  /* copy partial string anyway */
    {
        convlen = WideCharToMultiByte(CP_ACP, 0, langbufW, *buflen, langbuf, *buflen, NULL, NULL);
        if (convlen < *buflen)
        {
            langbuf[convlen] = 0;
            convlen--;  /* do not count the terminating 0 */
        }
        else
        {
            convlen = *buflen;
        }
    }
    *buflen = buflenW ? convlen : 0;

    HeapFree(GetProcessHeap(), 0, langbufW);
    return retval;
}

/*************************************************************************
 *      @	[SHLWAPI.30]
 *
 * Determine if a Unicode character is a blank.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a blank,
 *  FALSE otherwise.
 *
 */
BOOL WINAPI IsCharBlankW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_BLANK);
}

/*************************************************************************
 *      @	[SHLWAPI.31]
 *
 * Determine if a Unicode character is punctuation.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is punctuation,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharPunctW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_PUNCT);
}

/*************************************************************************
 *      @	[SHLWAPI.32]
 *
 * Determine if a Unicode character is a control character.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a control character,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharCntrlW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_CNTRL);
}

/*************************************************************************
 *      @	[SHLWAPI.33]
 *
 * Determine if a Unicode character is a digit.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a digit,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharDigitW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_DIGIT);
}

/*************************************************************************
 *      @	[SHLWAPI.34]
 *
 * Determine if a Unicode character is a hex digit.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a hex digit,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharXDigitW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_XDIGIT);
}

BOOL WINAPI IsCharSpaceA(CHAR c)
{
    WORD CharType;
    return GetStringTypeA(GetSystemDefaultLCID(), CT_CTYPE1, &c, 1, &CharType) && (CharType & C1_SPACE);
}

/*************************************************************************
 *      @	[SHLWAPI.29]
 *
 * Determine if a Unicode character is a space.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a space,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharSpaceW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_SPACE);
}

/*************************************************************************
 *      @	[SHLWAPI.219]
 *
 * Call IUnknown_QueryInterface() on a table of objects.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_POINTER or E_NOINTERFACE.
 */
HRESULT WINAPI QISearch(
	void *base,         /* [in]   Table of interfaces */
	const QITAB *table, /* [in]   Array of REFIIDs and indexes into the table */
	REFIID riid,        /* [in]   REFIID to get interface for */
	void **ppv)         /* [out]  Destination for interface pointer */
{
	HRESULT ret;
	IUnknown *a_vtbl;
	const QITAB *xmove;

	TRACE("(%p %p %s %p)\n", base, table, debugstr_guid(riid), ppv);
	if (ppv) {
	    xmove = table;
	    while (xmove->piid) {
		TRACE("trying (offset %d) %s\n", xmove->dwOffset, debugstr_guid(xmove->piid));
		if (IsEqualIID(riid, xmove->piid)) {
		    a_vtbl = (IUnknown*)(xmove->dwOffset + (LPBYTE)base);
		    TRACE("matched, returning (%p)\n", a_vtbl);
                    *ppv = a_vtbl;
		    IUnknown_AddRef(a_vtbl);
		    return S_OK;
		}
		xmove++;
	    }

	    if (IsEqualIID(riid, &IID_IUnknown)) {
		a_vtbl = (IUnknown*)(table->dwOffset + (LPBYTE)base);
		TRACE("returning first for IUnknown (%p)\n", a_vtbl);
                *ppv = a_vtbl;
		IUnknown_AddRef(a_vtbl);
		return S_OK;
	    }
	    *ppv = 0;
	    ret = E_NOINTERFACE;
	} else
	    ret = E_POINTER;

	TRACE("-- 0x%08x\n", ret);
	return ret;
}

/* internal structure of what the HUSKEY points to */
typedef struct {
    HKEY     HKCUstart; /* Start key in CU hive */
    HKEY     HKCUkey;   /* Opened key in CU hive */
    HKEY     HKLMstart; /* Start key in LM hive */
    HKEY     HKLMkey;   /* Opened key in LM hive */
    WCHAR    lpszPath[MAX_PATH];
} SHUSKEY, *LPSHUSKEY;


#define REG_HKCU  TRUE
#define REG_HKLM  FALSE
/*************************************************************************
 * REG_GetHKEYFromHUSKEY
 *
 * Function:  Return the proper registry key from the HUSKEY structure
 *            also allow special predefined values.
 */
static HKEY REG_GetHKEYFromHUSKEY(HUSKEY hUSKey, BOOL which)
{
        HKEY test = hUSKey;
        LPSHUSKEY mihk = hUSKey;

	if ((test == HKEY_CLASSES_ROOT)        ||
	    (test == HKEY_CURRENT_CONFIG)      ||
	    (test == HKEY_CURRENT_USER)        ||
	    (test == HKEY_DYN_DATA)            ||
	    (test == HKEY_LOCAL_MACHINE)       ||
	    (test == HKEY_PERFORMANCE_DATA)    ||
/* FIXME:  need to define for Win2k, ME, XP
 *	    (test == HKEY_PERFORMANCE_TEXT)    ||
 *	    (test == HKEY_PERFORMANCE_NLSTEXT) ||
 */
	    (test == HKEY_USERS)) return test;
	if (which == REG_HKCU) return mihk->HKCUkey;
	return mihk->HKLMkey;
}


/*************************************************************************
 * SHRegOpenUSKeyA	[SHLWAPI.@]
 *
 * Open a user-specific registry key.
 *
 * PARAMS
 *  Path           [I] Key name to open
 *  AccessType     [I] Access type
 *  hRelativeUSKey [I] Relative user key
 *  phNewUSKey     [O] Destination for created key
 *  fIgnoreHKCU    [I] TRUE=Don't check HKEY_CURRENT_USER
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from RegOpenKeyExA().
 */
LONG WINAPI SHRegOpenUSKeyA(LPCSTR Path, REGSAM AccessType, HUSKEY hRelativeUSKey,
                            PHUSKEY phNewUSKey, BOOL fIgnoreHKCU)
{
    WCHAR szPath[MAX_PATH];

    if (Path)
      MultiByteToWideChar(CP_ACP, 0, Path, -1, szPath, MAX_PATH);

    return SHRegOpenUSKeyW(Path ? szPath : NULL, AccessType, hRelativeUSKey,
                           phNewUSKey, fIgnoreHKCU);
}

/*************************************************************************
 * SHRegOpenUSKeyW	[SHLWAPI.@]
 *
 * See SHRegOpenUSKeyA.
 */
LONG WINAPI SHRegOpenUSKeyW(LPCWSTR Path, REGSAM AccessType, HUSKEY hRelativeUSKey,
                            PHUSKEY phNewUSKey, BOOL fIgnoreHKCU)
{
    LONG ret2, ret1 = ~ERROR_SUCCESS;
    LPSHUSKEY hKey;

    TRACE("(%s,0x%x,%p,%p,%d)\n", debugstr_w(Path),(LONG)AccessType,
          hRelativeUSKey, phNewUSKey, fIgnoreHKCU);

    if (phNewUSKey)
        *phNewUSKey = NULL;

    /* Create internal HUSKEY */
    hKey = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*hKey));
    lstrcpynW(hKey->lpszPath, Path, sizeof(hKey->lpszPath)/sizeof(WCHAR));

    if (hRelativeUSKey)
    {
        hKey->HKCUstart = SHRegDuplicateHKey(REG_GetHKEYFromHUSKEY(hRelativeUSKey, REG_HKCU));
        hKey->HKLMstart = SHRegDuplicateHKey(REG_GetHKEYFromHUSKEY(hRelativeUSKey, REG_HKLM));

        /* FIXME: if either of these keys is NULL, create the start key from
         *        the relative keys start+path
         */
    }
    else
    {
        hKey->HKCUstart = HKEY_CURRENT_USER;
        hKey->HKLMstart = HKEY_LOCAL_MACHINE;
    }

    if (!fIgnoreHKCU)
    {
        ret1 = RegOpenKeyExW(hKey->HKCUstart, hKey->lpszPath, 0, AccessType, &hKey->HKCUkey);
        if (ret1)
            hKey->HKCUkey = 0;
    }

    ret2 = RegOpenKeyExW(hKey->HKLMstart, hKey->lpszPath, 0, AccessType, &hKey->HKLMkey);
    if (ret2)
        hKey->HKLMkey = 0;

    if (ret1 || ret2)
        TRACE("one or more opens failed: HKCU=%d HKLM=%d\n", ret1, ret2);

    if (ret1 && ret2)
    {
        /* Neither open succeeded: fail */
        SHRegCloseUSKey(hKey);
        return ret2;
    }

    TRACE("HUSKEY=%p\n", hKey);
    if (phNewUSKey)
        *phNewUSKey = hKey;
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegCloseUSKey	[SHLWAPI.@]
 *
 * Close a user-specific registry key
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from RegCloseKey().
 */
LONG WINAPI SHRegCloseUSKey(
        HUSKEY hUSKey) /* [I] Key to close */
{
    LPSHUSKEY hKey = hUSKey;
    LONG ret = ERROR_SUCCESS;

    if (!hKey)
        return ERROR_INVALID_PARAMETER;

    if (hKey->HKCUkey)
        ret = RegCloseKey(hKey->HKCUkey);
    if (hKey->HKCUstart && hKey->HKCUstart != HKEY_CURRENT_USER)
        ret = RegCloseKey(hKey->HKCUstart);
    if (hKey->HKLMkey)
        ret = RegCloseKey(hKey->HKLMkey);
    if (hKey->HKLMstart && hKey->HKLMstart != HKEY_LOCAL_MACHINE)
        ret = RegCloseKey(hKey->HKLMstart);

    HeapFree(GetProcessHeap(), 0, hKey);
    return ret;
}

/*************************************************************************
 * SHRegCreateUSKeyA  [SHLWAPI.@]
 *
 * See SHRegCreateUSKeyW.
 */
LONG WINAPI SHRegCreateUSKeyA(LPCSTR path, REGSAM samDesired, HUSKEY relative_key,
                              PHUSKEY new_uskey, DWORD flags)
{
    WCHAR *pathW;
    LONG ret;

    TRACE("(%s, 0x%08x, %p, %p, 0x%08x)\n", debugstr_a(path), samDesired, relative_key,
        new_uskey, flags);

    if (path)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        pathW = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        if (!pathW)
            return ERROR_NOT_ENOUGH_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, len);
    }
    else
        pathW = NULL;

    ret = SHRegCreateUSKeyW(pathW, samDesired, relative_key, new_uskey, flags);
    HeapFree(GetProcessHeap(), 0, pathW);
    return ret;
}

/*************************************************************************
 * SHRegCreateUSKeyW  [SHLWAPI.@]
 *
 * Create or open a user-specific registry key.
 *
 * PARAMS
 *  path         [I] Key name to create or open.
 *  samDesired   [I] Wanted security access.
 *  relative_key [I] Base path if 'path' is relative. NULL otherwise.
 *  new_uskey    [O] Receives a handle to the new or opened key.
 *  flags        [I] Base key under which the key should be opened.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Nonzero error code from winerror.h
 */
LONG WINAPI SHRegCreateUSKeyW(LPCWSTR path, REGSAM samDesired, HUSKEY relative_key,
                              PHUSKEY new_uskey, DWORD flags)
{
    LONG ret = ERROR_CALL_NOT_IMPLEMENTED;
    SHUSKEY *ret_key;

    TRACE("(%s, 0x%08x, %p, %p, 0x%08x)\n", debugstr_w(path), samDesired,
          relative_key, new_uskey, flags);

    if (!new_uskey) return ERROR_INVALID_PARAMETER;

    *new_uskey = NULL;

    if (flags & ~SHREGSET_FORCE_HKCU)
    {
        FIXME("unsupported flags 0x%08x\n", flags);
        return ERROR_SUCCESS;
    }

    ret_key = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret_key));
    lstrcpynW(ret_key->lpszPath, path, sizeof(ret_key->lpszPath)/sizeof(WCHAR));

    if (relative_key)
    {
        ret_key->HKCUstart = SHRegDuplicateHKey(REG_GetHKEYFromHUSKEY(relative_key, REG_HKCU));
        ret_key->HKLMstart = SHRegDuplicateHKey(REG_GetHKEYFromHUSKEY(relative_key, REG_HKLM));
    }
    else
    {
        ret_key->HKCUstart = HKEY_CURRENT_USER;
        ret_key->HKLMstart = HKEY_LOCAL_MACHINE;
    }

    if (flags & SHREGSET_FORCE_HKCU)
    {
        ret = RegCreateKeyExW(ret_key->HKCUstart, path, 0, NULL, 0, samDesired, NULL, &ret_key->HKCUkey, NULL);
        if (ret == ERROR_SUCCESS)
            *new_uskey = ret_key;
        else
            HeapFree(GetProcessHeap(), 0, ret_key);
    }

    return ret;
}

/*************************************************************************
 * SHRegDeleteEmptyUSKeyA  [SHLWAPI.@]
 *
 * Delete an empty user-specific registry key.
 *
 * PARAMS
 *  hUSKey      [I] Handle to an open registry key.
 *  pszValue    [I] Empty key name.
 *  delRegFlags [I] Flag that specifies the base from which to delete 
 *                  the key.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Nonzero error code from winerror.h
 */
LONG WINAPI SHRegDeleteEmptyUSKeyA(HUSKEY hUSKey, LPCSTR pszValue, SHREGDEL_FLAGS delRegFlags)
{
    FIXME("(%p, %s, 0x%08x) stub\n", hUSKey, debugstr_a(pszValue), delRegFlags);
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegDeleteEmptyUSKeyW  [SHLWAPI.@]
 *
 * See SHRegDeleteEmptyUSKeyA.
 */
LONG WINAPI SHRegDeleteEmptyUSKeyW(HUSKEY hUSKey, LPCWSTR pszValue, SHREGDEL_FLAGS delRegFlags)
{
    FIXME("(%p, %s, 0x%08x) stub\n", hUSKey, debugstr_w(pszValue), delRegFlags);
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegDeleteUSValueA  [SHLWAPI.@]
 *
 * Delete a user-specific registry value.
 *
 * PARAMS
 *  hUSKey      [I] Handle to an open registry key.
 *  pszValue    [I] Specifies the value to delete.
 *  delRegFlags [I] Flag that specifies the base of the key from which to
 *                  delete the value.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Nonzero error code from winerror.h
 */
LONG WINAPI SHRegDeleteUSValueA(HUSKEY hUSKey, LPCSTR pszValue, SHREGDEL_FLAGS delRegFlags)
{
    FIXME("(%p, %s, 0x%08x) stub\n", hUSKey, debugstr_a(pszValue), delRegFlags);
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegDeleteUSValueW  [SHLWAPI.@]
 *
 * See SHRegDeleteUSValueA.
 */
LONG WINAPI SHRegDeleteUSValueW(HUSKEY hUSKey, LPCWSTR pszValue, SHREGDEL_FLAGS delRegFlags)
{
    FIXME("(%p, %s, 0x%08x) stub\n", hUSKey, debugstr_w(pszValue), delRegFlags);
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegEnumUSValueA  [SHLWAPI.@]
 *
 * Enumerate values of a specified registry key.
 *
 * PARAMS
 *  hUSKey           [I]   Handle to an open registry key.
 *  dwIndex          [I]   Index of the value to be retrieved.
 *  pszValueName     [O]   Buffer to receive the value name.
 *  pcchValueNameLen [I]   Size of pszValueName in characters.
 *  pdwType          [O]   Receives data type of the value.
 *  pvData           [O]   Receives value data. May be NULL.
 *  pcbData          [I/O] Size of pvData in bytes.
 *  enumRegFlags     [I]   Flag that specifies the base key under which to
 *                         enumerate values.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Nonzero error code from winerror.h
 */
LONG WINAPI SHRegEnumUSValueA(HUSKEY hUSKey, DWORD dwIndex, LPSTR pszValueName,
                              LPDWORD pcchValueNameLen, LPDWORD pdwType, LPVOID pvData,
                              LPDWORD pcbData, SHREGENUM_FLAGS enumRegFlags)
{
    HKEY dokey;

    TRACE("(%p, 0x%08x, %p, %p, %p, %p, %p, 0x%08x)\n", hUSKey, dwIndex,
          pszValueName, pcchValueNameLen, pdwType, pvData, pcbData, enumRegFlags);

    if (((enumRegFlags == SHREGENUM_HKCU) ||
         (enumRegFlags == SHREGENUM_DEFAULT)) &&
        (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
        return RegEnumValueA(dokey, dwIndex, pszValueName, pcchValueNameLen,
                             NULL, pdwType, pvData, pcbData);
    }

    if (((enumRegFlags == SHREGENUM_HKLM) ||
         (enumRegFlags == SHREGENUM_DEFAULT)) &&
        (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
        return RegEnumValueA(dokey, dwIndex, pszValueName, pcchValueNameLen,
                             NULL, pdwType, pvData, pcbData);
    }
    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 * SHRegEnumUSValueW  [SHLWAPI.@]
 *
 * See SHRegEnumUSValueA.
 */
LONG WINAPI SHRegEnumUSValueW(HUSKEY hUSKey, DWORD dwIndex, LPWSTR pszValueName,
                              LPDWORD pcchValueNameLen, LPDWORD pdwType, LPVOID pvData,
                              LPDWORD pcbData, SHREGENUM_FLAGS enumRegFlags)
{
    HKEY dokey;

    TRACE("(%p, 0x%08x, %p, %p, %p, %p, %p, 0x%08x)\n", hUSKey, dwIndex,
          pszValueName, pcchValueNameLen, pdwType, pvData, pcbData, enumRegFlags);

    if (((enumRegFlags == SHREGENUM_HKCU) ||
         (enumRegFlags == SHREGENUM_DEFAULT)) &&
        (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
        return RegEnumValueW(dokey, dwIndex, pszValueName, pcchValueNameLen,
                             NULL, pdwType, pvData, pcbData);
    }

    if (((enumRegFlags == SHREGENUM_HKLM) ||
         (enumRegFlags == SHREGENUM_DEFAULT)) &&
        (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
        return RegEnumValueW(dokey, dwIndex, pszValueName, pcchValueNameLen,
                             NULL, pdwType, pvData, pcbData);
    }
    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegQueryUSValueA	[SHLWAPI.@]
 *
 * Query a user-specific registry value.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from RegQueryValueExA().
 */
LONG WINAPI SHRegQueryUSValueA(
	HUSKEY hUSKey, /* [I] Key to query */
	LPCSTR pszValue, /* [I] Value name under hUSKey */
	LPDWORD pdwType, /* [O] Destination for value type */
	LPVOID pvData, /* [O] Destination for value data */
	LPDWORD pcbData, /* [O] Destination for value length */
	BOOL fIgnoreHKCU,  /* [I] TRUE=Don't check HKEY_CURRENT_USER */
	LPVOID pvDefaultData, /* [I] Default data if pszValue does not exist */
	DWORD dwDefaultDataSize) /* [I] Length of pvDefaultData */
{
	LONG ret = ~ERROR_SUCCESS;
	LONG i, maxmove;
	HKEY dokey;
	CHAR *src, *dst;

	/* if user wants HKCU, and it exists, then try it */
	if (!fIgnoreHKCU && (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryValueExA(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKCU RegQueryValue returned %08x\n", ret);
	}

	/* if HKCU did not work and HKLM exists, then try it */
	if ((ret != ERROR_SUCCESS) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    ret = RegQueryValueExA(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKLM RegQueryValue returned %08x\n", ret);
	}

	/* if neither worked, and default data exists, then use it */
	if (ret != ERROR_SUCCESS) {
	    if (pvDefaultData && (dwDefaultDataSize != 0)) {
		maxmove = (dwDefaultDataSize >= *pcbData) ? *pcbData : dwDefaultDataSize;
                src = pvDefaultData;
                dst = pvData;
		for(i=0; i<maxmove; i++) *dst++ = *src++;
		*pcbData = maxmove;
		TRACE("setting default data\n");
		ret = ERROR_SUCCESS;
	    }
	}
	return ret;
}


/*************************************************************************
 *      SHRegQueryUSValueW	[SHLWAPI.@]
 *
 * See SHRegQueryUSValueA.
 */
LONG WINAPI SHRegQueryUSValueW(
	HUSKEY hUSKey,
	LPCWSTR pszValue,
	LPDWORD pdwType,
	LPVOID pvData,
	LPDWORD pcbData,
	BOOL fIgnoreHKCU,
	LPVOID pvDefaultData,
	DWORD dwDefaultDataSize)
{
	LONG ret = ~ERROR_SUCCESS;
	LONG i, maxmove;
	HKEY dokey;
	CHAR *src, *dst;

	/* if user wants HKCU, and it exists, then try it */
	if (!fIgnoreHKCU && (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryValueExW(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKCU RegQueryValue returned %08x\n", ret);
	}

	/* if HKCU did not work and HKLM exists, then try it */
	if ((ret != ERROR_SUCCESS) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    ret = RegQueryValueExW(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKLM RegQueryValue returned %08x\n", ret);
	}

	/* if neither worked, and default data exists, then use it */
	if (ret != ERROR_SUCCESS) {
	    if (pvDefaultData && (dwDefaultDataSize != 0)) {
		maxmove = (dwDefaultDataSize >= *pcbData) ? *pcbData : dwDefaultDataSize;
                src = pvDefaultData;
                dst = pvData;
		for(i=0; i<maxmove; i++) *dst++ = *src++;
		*pcbData = maxmove;
		TRACE("setting default data\n");
		ret = ERROR_SUCCESS;
	    }
	}
	return ret;
}

/*************************************************************************
 * SHRegGetUSValueA	[SHLWAPI.@]
 *
 * Get a user-specific registry value.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from SHRegOpenUSKeyA() or SHRegQueryUSValueA().
 *
 * NOTES
 *   This function opens pSubKey, queries the value, and then closes the key.
 */
LONG WINAPI SHRegGetUSValueA(
	LPCSTR   pSubKey, /* [I] Key name to open */
	LPCSTR   pValue, /* [I] Value name to open */
	LPDWORD  pwType, /* [O] Destination for the type of the value */
	LPVOID   pvData, /* [O] Destination for the value */
	LPDWORD  pcbData, /* [I] Destination for the length of the value **/
	BOOL     flagIgnoreHKCU, /* [I] TRUE=Don't check HKEY_CURRENT_USER */
	LPVOID   pDefaultData, /* [I] Default value if it doesn't exist */
	DWORD    wDefaultDataSize) /* [I] Length of pDefaultData */
{
	HUSKEY myhuskey;
	LONG ret;

	if (!pvData || !pcbData) return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/
	TRACE("key '%s', value '%s', datalen %d,  %s\n",
	      debugstr_a(pSubKey), debugstr_a(pValue), *pcbData,
	      (flagIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	ret = SHRegOpenUSKeyA(pSubKey, 0x1, 0, &myhuskey, flagIgnoreHKCU);
	if (ret == ERROR_SUCCESS) {
	    ret = SHRegQueryUSValueA(myhuskey, pValue, pwType, pvData,
				     pcbData, flagIgnoreHKCU, pDefaultData,
				     wDefaultDataSize);
	    SHRegCloseUSKey(myhuskey);
	}
	return ret;
}

/*************************************************************************
 * SHRegGetUSValueW	[SHLWAPI.@]
 *
 * See SHRegGetUSValueA.
 */
LONG WINAPI SHRegGetUSValueW(
	LPCWSTR  pSubKey,
	LPCWSTR  pValue,
	LPDWORD  pwType,
	LPVOID   pvData,
	LPDWORD  pcbData,
	BOOL     flagIgnoreHKCU,
	LPVOID   pDefaultData,
	DWORD    wDefaultDataSize)
{
	HUSKEY myhuskey;
	LONG ret;

	if (!pvData || !pcbData) return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/
	TRACE("key '%s', value '%s', datalen %d,  %s\n",
	      debugstr_w(pSubKey), debugstr_w(pValue), *pcbData,
	      (flagIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	ret = SHRegOpenUSKeyW(pSubKey, 0x1, 0, &myhuskey, flagIgnoreHKCU);
	if (ret == ERROR_SUCCESS) {
	    ret = SHRegQueryUSValueW(myhuskey, pValue, pwType, pvData,
				     pcbData, flagIgnoreHKCU, pDefaultData,
				     wDefaultDataSize);
	    SHRegCloseUSKey(myhuskey);
	}
	return ret;
}

/*************************************************************************
 * SHRegSetUSValueA   [SHLWAPI.@]
 *
 * Set a user-specific registry value.
 *
 * PARAMS
 *  pszSubKey [I] Name of key to set the value in
 *  pszValue  [I] Name of value under pszSubKey to set the value in
 *  dwType    [I] Type of the value
 *  pvData    [I] Data to set as the value
 *  cbData    [I] length of pvData
 *  dwFlags   [I] SHREGSET_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from SHRegOpenUSKeyA() or SHRegWriteUSValueA(), or
 *           ERROR_INVALID_FUNCTION if pvData is NULL.
 *
 * NOTES
 *   This function opens pszSubKey, sets the value, and then closes the key.
 */
LONG WINAPI SHRegSetUSValueA(LPCSTR pszSubKey, LPCSTR pszValue, DWORD dwType,
                             LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
  BOOL ignoreHKCU = TRUE;
  HUSKEY hkey;
  LONG ret;

  TRACE("(%s,%s,%d,%p,%d,0x%08x\n", debugstr_a(pszSubKey), debugstr_a(pszValue),
        dwType, pvData, cbData, dwFlags);

  if (!pvData)
    return ERROR_INVALID_FUNCTION;

  if (dwFlags & SHREGSET_HKCU || dwFlags & SHREGSET_FORCE_HKCU)
    ignoreHKCU = FALSE;

  ret = SHRegOpenUSKeyA(pszSubKey, KEY_ALL_ACCESS, 0, &hkey, ignoreHKCU);
  if (ret == ERROR_SUCCESS)
  {
    ret = SHRegWriteUSValueA(hkey, pszValue, dwType, pvData, cbData, dwFlags);
    SHRegCloseUSKey(hkey);
  }
  return ret;
}

/*************************************************************************
 * SHRegSetUSValueW   [SHLWAPI.@]
 *
 * See SHRegSetUSValueA.
 */
LONG WINAPI SHRegSetUSValueW(LPCWSTR pszSubKey, LPCWSTR pszValue, DWORD dwType,
                             LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
  BOOL ignoreHKCU = TRUE;
  HUSKEY hkey;
  LONG ret;

  TRACE("(%s,%s,%d,%p,%d,0x%08x\n", debugstr_w(pszSubKey), debugstr_w(pszValue),
        dwType, pvData, cbData, dwFlags);

  if (!pvData)
    return ERROR_INVALID_FUNCTION;

  if (dwFlags & SHREGSET_HKCU || dwFlags & SHREGSET_FORCE_HKCU)
    ignoreHKCU = FALSE;

  ret = SHRegOpenUSKeyW(pszSubKey, KEY_ALL_ACCESS, 0, &hkey, ignoreHKCU);
  if (ret == ERROR_SUCCESS)
  {
    ret = SHRegWriteUSValueW(hkey, pszValue, dwType, pvData, cbData, dwFlags);
    SHRegCloseUSKey(hkey);
  }
  return ret;
}

/*************************************************************************
 * SHRegGetBoolUSValueA   [SHLWAPI.@]
 *
 * Get a user-specific registry boolean value.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from SHRegOpenUSKeyA() or SHRegQueryUSValueA().
 *
 * NOTES
 *   This function opens pszSubKey, queries the value, and then closes the key.
 *
 *   Boolean values are one of the following:
 *   True: YES,TRUE,non-zero
 *   False: NO,FALSE,0
 */
BOOL WINAPI SHRegGetBoolUSValueA(
	LPCSTR pszSubKey, /* [I] Key name to open */
	LPCSTR pszValue, /* [I] Value name to open */
	BOOL fIgnoreHKCU, /* [I] TRUE=Don't check HKEY_CURRENT_USER */
	BOOL fDefault) /* [I] Default value to use if pszValue is not present */
{
	DWORD type, datalen, work;
	BOOL ret = fDefault;
	CHAR data[10];

	TRACE("key '%s', value '%s', %s\n",
	      debugstr_a(pszSubKey), debugstr_a(pszValue),
	      (fIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	datalen = sizeof(data)-1;
	if (!SHRegGetUSValueA( pszSubKey, pszValue, &type,
			       data, &datalen,
			       fIgnoreHKCU, 0, 0)) {
	    /* process returned data via type into bool */
	    switch (type) {
	    case REG_SZ:
		data[9] = '\0';     /* set end of string */
		if (lstrcmpiA(data, "YES") == 0) ret = TRUE;
		if (lstrcmpiA(data, "TRUE") == 0) ret = TRUE;
		if (lstrcmpiA(data, "NO") == 0) ret = FALSE;
		if (lstrcmpiA(data, "FALSE") == 0) ret = FALSE;
		break;
	    case REG_DWORD:
		work = *(LPDWORD)data;
		ret = (work != 0);
		break;
	    case REG_BINARY:
		if (datalen == 1) {
		    ret = (data[0] != '\0');
		    break;
		}
	    default:
		FIXME("Unsupported registry data type %d\n", type);
		ret = FALSE;
	    }
	    TRACE("got value (type=%d), returning <%s>\n", type,
		  (ret) ? "TRUE" : "FALSE");
	}
	else {
	    ret = fDefault;
	    TRACE("returning default data <%s>\n",
		  (ret) ? "TRUE" : "FALSE");
	}
	return ret;
}

/*************************************************************************
 * SHRegGetBoolUSValueW	  [SHLWAPI.@]
 *
 * See SHRegGetBoolUSValueA.
 */
BOOL WINAPI SHRegGetBoolUSValueW(
	LPCWSTR pszSubKey,
	LPCWSTR pszValue,
	BOOL fIgnoreHKCU,
	BOOL fDefault)
{
	static const WCHAR wYES[]=  {'Y','E','S','\0'};
	static const WCHAR wTRUE[]= {'T','R','U','E','\0'};
	static const WCHAR wNO[]=   {'N','O','\0'};
	static const WCHAR wFALSE[]={'F','A','L','S','E','\0'};
	DWORD type, datalen, work;
	BOOL ret = fDefault;
	WCHAR data[10];

	TRACE("key '%s', value '%s', %s\n",
	      debugstr_w(pszSubKey), debugstr_w(pszValue),
	      (fIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	datalen = (sizeof(data)-1) * sizeof(WCHAR);
	if (!SHRegGetUSValueW( pszSubKey, pszValue, &type,
			       data, &datalen,
			       fIgnoreHKCU, 0, 0)) {
	    /* process returned data via type into bool */
	    switch (type) {
	    case REG_SZ:
		data[9] = '\0';     /* set end of string */
		if (lstrcmpiW(data, wYES)==0 || lstrcmpiW(data, wTRUE)==0)
		    ret = TRUE;
		else if (lstrcmpiW(data, wNO)==0 || lstrcmpiW(data, wFALSE)==0)
		    ret = FALSE;
		break;
	    case REG_DWORD:
		work = *(LPDWORD)data;
		ret = (work != 0);
		break;
	    case REG_BINARY:
		if (datalen == 1) {
		    ret = (data[0] != '\0');
		    break;
		}
	    default:
		FIXME("Unsupported registry data type %d\n", type);
		ret = FALSE;
	    }
	    TRACE("got value (type=%d), returning <%s>\n", type,
		  (ret) ? "TRUE" : "FALSE");
	}
	else {
	    ret = fDefault;
	    TRACE("returning default data <%s>\n",
		  (ret) ? "TRUE" : "FALSE");
	}
	return ret;
}

/*************************************************************************
 *      SHRegQueryInfoUSKeyA	[SHLWAPI.@]
 *
 * Get information about a user-specific registry key.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from RegQueryInfoKeyA().
 */
LONG WINAPI SHRegQueryInfoUSKeyA(
	HUSKEY hUSKey, /* [I] Key to query */
	LPDWORD pcSubKeys, /* [O] Destination for number of sub keys */
	LPDWORD pcchMaxSubKeyLen, /* [O] Destination for the length of the biggest sub key name */
	LPDWORD pcValues, /* [O] Destination for number of values */
	LPDWORD pcchMaxValueNameLen,/* [O] Destination for the length of the biggest value */
	SHREGENUM_FLAGS enumRegFlags)  /* [in] SHREGENUM_ flags from "shlwapi.h" */
{
	HKEY dokey;
	LONG ret;

	TRACE("(%p,%p,%p,%p,%p,%d)\n",
	      hUSKey,pcSubKeys,pcchMaxSubKeyLen,pcValues,
	      pcchMaxValueNameLen,enumRegFlags);

	/* if user wants HKCU, and it exists, then try it */
	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryInfoKeyA(dokey, 0, 0, 0,
				   pcSubKeys, pcchMaxSubKeyLen, 0,
				   pcValues, pcchMaxValueNameLen, 0, 0, 0);
	    if ((ret == ERROR_SUCCESS) ||
		(enumRegFlags == SHREGENUM_HKCU))
		return ret;
	}
	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegQueryInfoKeyA(dokey, 0, 0, 0,
				    pcSubKeys, pcchMaxSubKeyLen, 0,
				    pcValues, pcchMaxValueNameLen, 0, 0, 0);
	}
	return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegQueryInfoUSKeyW	[SHLWAPI.@]
 *
 * See SHRegQueryInfoUSKeyA.
 */
LONG WINAPI SHRegQueryInfoUSKeyW(
	HUSKEY hUSKey,
	LPDWORD pcSubKeys,
	LPDWORD pcchMaxSubKeyLen,
	LPDWORD pcValues,
	LPDWORD pcchMaxValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)
{
	HKEY dokey;
	LONG ret;

	TRACE("(%p,%p,%p,%p,%p,%d)\n",
	      hUSKey,pcSubKeys,pcchMaxSubKeyLen,pcValues,
	      pcchMaxValueNameLen,enumRegFlags);

	/* if user wants HKCU, and it exists, then try it */
	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    ret = RegQueryInfoKeyW(dokey, 0, 0, 0,
				   pcSubKeys, pcchMaxSubKeyLen, 0,
				   pcValues, pcchMaxValueNameLen, 0, 0, 0);
	    if ((ret == ERROR_SUCCESS) ||
		(enumRegFlags == SHREGENUM_HKCU))
		return ret;
	}
	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegQueryInfoKeyW(dokey, 0, 0, 0,
				    pcSubKeys, pcchMaxSubKeyLen, 0,
				    pcValues, pcchMaxValueNameLen, 0, 0, 0);
	}
	return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegEnumUSKeyA   	[SHLWAPI.@]
 *
 * Enumerate a user-specific registry key.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from RegEnumKeyExA().
 */
LONG WINAPI SHRegEnumUSKeyA(
	HUSKEY hUSKey,                 /* [in] Key to enumerate */
	DWORD dwIndex,                 /* [in] Index within hUSKey */
	LPSTR pszName,                 /* [out] Name of the enumerated value */
	LPDWORD pcchValueNameLen,      /* [in/out] Length of pszName */
	SHREGENUM_FLAGS enumRegFlags)  /* [in] SHREGENUM_ flags from "shlwapi.h" */
{
	HKEY dokey;

	TRACE("(%p,%d,%p,%p(%d),%d)\n",
	      hUSKey, dwIndex, pszName, pcchValueNameLen,
	      *pcchValueNameLen, enumRegFlags);

	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    return RegEnumKeyExA(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}

	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegEnumKeyExA(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}
	FIXME("no support for SHREGENUM_BOTH\n");
	return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 *      SHRegEnumUSKeyW   	[SHLWAPI.@]
 *
 * See SHRegEnumUSKeyA.
 */
LONG WINAPI SHRegEnumUSKeyW(
	HUSKEY hUSKey,
	DWORD dwIndex,
	LPWSTR pszName,
	LPDWORD pcchValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)
{
	HKEY dokey;

	TRACE("(%p,%d,%p,%p(%d),%d)\n",
	      hUSKey, dwIndex, pszName, pcchValueNameLen,
	      *pcchValueNameLen, enumRegFlags);

	if (((enumRegFlags == SHREGENUM_HKCU) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	    return RegEnumKeyExW(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}

	if (((enumRegFlags == SHREGENUM_HKLM) ||
	     (enumRegFlags == SHREGENUM_DEFAULT)) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    return RegEnumKeyExW(dokey, dwIndex, pszName, pcchValueNameLen,
				0, 0, 0, 0);
	}
	FIXME("no support for SHREGENUM_BOTH\n");
	return ERROR_INVALID_FUNCTION;
}


/*************************************************************************
 *      SHRegWriteUSValueA   	[SHLWAPI.@]
 *
 * Write a user-specific registry value.
 *
 * PARAMS
 *  hUSKey   [I] Key to write the value to
 *  pszValue [I] Name of value under hUSKey to write the value as
 *  dwType   [I] Type of the value
 *  pvData   [I] Data to set as the value
 *  cbData   [I] length of pvData
 *  dwFlags  [I] SHREGSET_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: ERROR_INVALID_PARAMETER, if any parameter is invalid, otherwise
 *           an error code from RegSetValueExA().
 *
 * NOTES
 *  dwFlags must have at least SHREGSET_FORCE_HKCU or SHREGSET_FORCE_HKLM set.
 */
LONG  WINAPI SHRegWriteUSValueA(HUSKEY hUSKey, LPCSTR pszValue, DWORD dwType,
                                LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
    WCHAR szValue[MAX_PATH];

    if (pszValue)
      MultiByteToWideChar(CP_ACP, 0, pszValue, -1, szValue, MAX_PATH);

    return SHRegWriteUSValueW(hUSKey, pszValue ? szValue : NULL, dwType,
                               pvData, cbData, dwFlags);
}

/*************************************************************************
 *      SHRegWriteUSValueW   	[SHLWAPI.@]
 *
 * See SHRegWriteUSValueA.
 */
LONG  WINAPI SHRegWriteUSValueW(HUSKEY hUSKey, LPCWSTR pszValue, DWORD dwType,
                                LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
    DWORD dummy;
    LPSHUSKEY hKey = hUSKey;
    LONG ret = ERROR_SUCCESS;

    TRACE("(%p,%s,%d,%p,%d,%d)\n", hUSKey, debugstr_w(pszValue),
          dwType, pvData, cbData, dwFlags);

    if (!hUSKey || IsBadWritePtr(hUSKey, sizeof(SHUSKEY)) ||
        !(dwFlags & (SHREGSET_FORCE_HKCU|SHREGSET_FORCE_HKLM)))
        return ERROR_INVALID_PARAMETER;

    if (dwFlags & (SHREGSET_FORCE_HKCU|SHREGSET_HKCU))
    {
        if (!hKey->HKCUkey)
        {
            /* Create the key */
            ret = RegCreateKeyW(hKey->HKCUstart, hKey->lpszPath, &hKey->HKCUkey);
            TRACE("Creating HKCU key, ret = %d\n", ret);
            if (ret && (dwFlags & (SHREGSET_FORCE_HKCU)))
            {
                hKey->HKCUkey = 0;
                return ret;
            }
        }

        if (!ret)
        {
            if ((dwFlags & SHREGSET_FORCE_HKCU) ||
                RegQueryValueExW(hKey->HKCUkey, pszValue, NULL, NULL, NULL, &dummy))
            {
                /* Doesn't exist or we are forcing: Write value */
                ret = RegSetValueExW(hKey->HKCUkey, pszValue, 0, dwType, pvData, cbData);
                TRACE("Writing HKCU value, ret = %d\n", ret);
            }
        }
    }

    if (dwFlags & (SHREGSET_FORCE_HKLM|SHREGSET_HKLM))
    {
        if (!hKey->HKLMkey)
        {
            /* Create the key */
            ret = RegCreateKeyW(hKey->HKLMstart, hKey->lpszPath, &hKey->HKLMkey);
            TRACE("Creating HKLM key, ret = %d\n", ret);
            if (ret && (dwFlags & (SHREGSET_FORCE_HKLM)))
            {
                hKey->HKLMkey = 0;
                return ret;
            }
        }

        if (!ret)
        {
            if ((dwFlags & SHREGSET_FORCE_HKLM) ||
                RegQueryValueExW(hKey->HKLMkey, pszValue, NULL, NULL, NULL, &dummy))
            {
                /* Doesn't exist or we are forcing: Write value */
                ret = RegSetValueExW(hKey->HKLMkey, pszValue, 0, dwType, pvData, cbData);
                TRACE("Writing HKLM value, ret = %d\n", ret);
            }
        }
    }

    return ret;
}
