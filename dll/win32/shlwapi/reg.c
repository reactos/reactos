/*
 * SHLWAPI registry functions
 *
 * Copyright 1998 Juergen Schmied
 * Copyright 2001 Guy Albertelli
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

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/debug.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Key/Value names for MIME content types */
static const char lpszContentTypeA[] = "Content Type";
static const WCHAR lpszContentTypeW[] = { 'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

static const char szMimeDbContentA[] = "MIME\\Database\\Content Type\\";
static const WCHAR szMimeDbContentW[] = { 'M', 'I', 'M','E','\\',
  'D','a','t','a','b','a','s','e','\\','C','o','n','t','e','n','t',
  ' ','T','y','p','e','\\', 0 };
static const DWORD dwLenMimeDbContent = 27; /* strlen of szMimeDbContentA/W */

static const char szExtensionA[] = "Extension";
static const WCHAR szExtensionW[] = { 'E', 'x', 't','e','n','s','i','o','n','\0' };

/* internal structure of what the HUSKEY points to */
typedef struct {
    HKEY     HKCUstart; /* Start key in CU hive */
    HKEY     HKCUkey;   /* Opened key in CU hive */
    HKEY     HKLMstart; /* Start key in LM hive */
    HKEY     HKLMkey;   /* Opened key in LM hive */
    WCHAR    lpszPath[MAX_PATH];
} SHUSKEY, *LPSHUSKEY;

INT     WINAPI SHStringFromGUIDW(REFGUID,LPWSTR,INT);
HRESULT WINAPI SHRegGetCLSIDKeyW(REFGUID,LPCWSTR,BOOL,BOOL,PHKEY);


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

    if (hKey->HKCUkey)
        ret = RegCloseKey(hKey->HKCUkey);
    if (hKey->HKCUstart && hKey->HKCUstart != HKEY_CURRENT_USER)
        ret = RegCloseKey(hKey->HKCUstart);
    if (hKey->HKLMkey)
        ret = RegCloseKey(hKey->HKLMkey);
    if (hKey->HKLMstart && hKey->HKLMstart != HKEY_LOCAL_MACHINE)
        ret = RegCloseKey(hKey->HKCUstart);

    HeapFree(GetProcessHeap(), 0, hKey);
    return ret;
}

/*************************************************************************
 * SHRegCreateUSKeyA  [SHLWAPI.@]
 *
 * Create or open a user-specific registry key.
 * 
 * PARAMS
 *  pszPath        [I] Key name to create or open.
 *  samDesired     [I] Wanted security access.
 *  hRelativeUSKey [I] Base path if pszPath is relative. NULL otherwise.
 *  phNewUSKey     [O] Receives a handle to the new or opened key.
 *  dwFlags        [I] Base key under which the key should be opened.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Nonzero error code from winerror.h
 */
LONG WINAPI SHRegCreateUSKeyA(LPCSTR pszPath, REGSAM samDesired, HUSKEY hRelativeUSKey,
                              PHUSKEY phNewUSKey, DWORD dwFlags)
{
    FIXME("(%s, 0x%08x, %p, %p, 0x%08x) stub\n", debugstr_a(pszPath), samDesired,
          hRelativeUSKey, phNewUSKey, dwFlags);
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegCreateUSKeyW  [SHLWAPI.@]
 *
 * See SHRegCreateUSKeyA.
 */
LONG WINAPI SHRegCreateUSKeyW(LPCWSTR pszPath, REGSAM samDesired, HUSKEY hRelativeUSKey,
                              PHUSKEY phNewUSKey, DWORD dwFlags)
{
    FIXME("(%s, 0x%08x, %p, %p, 0x%08x) stub\n", debugstr_w(pszPath), samDesired,
          hRelativeUSKey, phNewUSKey, dwFlags);
    return ERROR_SUCCESS;
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
    FIXME("(%p, 0x%08x, %s, %p, %p, %p, %p, 0x%08x) stub\n", hUSKey, dwIndex,
          debugstr_a(pszValueName), pcchValueNameLen, pdwType, pvData, pcbData, enumRegFlags);
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
    FIXME("(%p, 0x%08x, %s, %p, %p, %p, %p, 0x%08x) stub\n", hUSKey, dwIndex,
          debugstr_w(pszValueName), pcchValueNameLen, pdwType, pvData, pcbData, enumRegFlags);
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

/*************************************************************************
 * SHRegGetPathA   [SHLWAPI.@]
 *
 * Get a path from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing path to get
 *   lpszValue  [I] Name of value containing path to get
 *   lpszPath   [O] Buffer for returned path
 *   dwFlags    [I] Reserved
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. lpszPath contains the path.
 *   Failure: An error code from RegOpenKeyExA() or SHQueryValueExA().
 */
DWORD WINAPI SHRegGetPathA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                           LPSTR lpszPath, DWORD dwFlags)
{
  DWORD dwSize = MAX_PATH;

  TRACE("(hkey=%p,%s,%s,%p,%d)\n", hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), lpszPath, dwFlags);

  return SHGetValueA(hKey, lpszSubKey, lpszValue, 0, lpszPath, &dwSize);
}

/*************************************************************************
 * SHRegGetPathW   [SHLWAPI.@]
 *
 * See SHRegGetPathA.
 */
DWORD WINAPI SHRegGetPathW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                           LPWSTR lpszPath, DWORD dwFlags)
{
  DWORD dwSize = MAX_PATH;

  TRACE("(hkey=%p,%s,%s,%p,%d)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), lpszPath, dwFlags);

  return SHGetValueW(hKey, lpszSubKey, lpszValue, 0, lpszPath, &dwSize);
}


/*************************************************************************
 * SHRegSetPathA   [SHLWAPI.@]
 *
 * Write a path to the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing path to set
 *   lpszValue  [I] Name of value containing path to set
 *   lpszPath   [O] Path to write
 *   dwFlags    [I] Reserved, must be 0.
 *
 * RETURNS
 *   Success: ERROR_SUCCESS.
 *   Failure: An error code from SHSetValueA().
 */
DWORD WINAPI SHRegSetPathA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                           LPCSTR lpszPath, DWORD dwFlags)
{
  char szBuff[MAX_PATH];

  FIXME("(hkey=%p,%s,%s,%p,%d) - semi-stub\n",hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), lpszPath, dwFlags);

  lstrcpyA(szBuff, lpszPath);

  /* FIXME: PathUnExpandEnvStringsA(szBuff); */

  return SHSetValueA(hKey,lpszSubKey, lpszValue, REG_SZ, szBuff,
                     lstrlenA(szBuff));
}

/*************************************************************************
 * SHRegSetPathW   [SHLWAPI.@]
 *
 * See SHRegSetPathA.
 */
DWORD WINAPI SHRegSetPathW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                           LPCWSTR lpszPath, DWORD dwFlags)
{
  WCHAR szBuff[MAX_PATH];

  FIXME("(hkey=%p,%s,%s,%p,%d) - semi-stub\n",hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), lpszPath, dwFlags);

  lstrcpyW(szBuff, lpszPath);

  /* FIXME: PathUnExpandEnvStringsW(szBuff); */

  return SHSetValueW(hKey,lpszSubKey, lpszValue, REG_SZ, szBuff,
                     lstrlenW(szBuff));
}

/*************************************************************************
 * SHGetValueA   [SHLWAPI.@]
 *
 * Get a value from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing value to get
 *   lpszValue  [I] Name of value to get
 *   pwType     [O] Pointer to the values type
 *   pvData     [O] Pointer to the values data
 *   pcbData    [O] Pointer to the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters contain the details read.
 *   Failure: An error code from RegOpenKeyExA() or SHQueryValueExA().
 */
DWORD WINAPI SHGetValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s,%s,%p,%p,%p)\n", hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), pwType, pvData, pcbData);

  /*   lpszSubKey can be 0. In this case the value is taken from the
   *   current key.
   */
  if(lpszSubKey)
    dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);

  if (! dwRet)
  {
    /* SHQueryValueEx expands Environment strings */
    dwRet = SHQueryValueExA(hSubKey ? hSubKey : hKey, lpszValue, 0, pwType, pvData, pcbData);
    if (hSubKey) RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHRegGetValueA   [SHLWAPI.@]
 *
 * Get a value from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing value to get
 *   lpszValue  [I] Name of value to get
 *   srrf       [I] Flags for restricting returned data
 *   pwType     [O] Pointer to the values type
 *   pvData     [O] Pointer to the values data
 *   pcbData    [O] Pointer to the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters contain the details read.
 *   Failure: An error code from RegOpenKeyExA() or SHQueryValueExA().
 */
DWORD WINAPI SHRegGetValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue, DWORD srrfFlags,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s,%s,%p,%p,%p)\n", hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), pwType, pvData, pcbData);
  FIXME("Semi-Stub: Find meaning and implement handling of SRFF Flags 0x%08x\n", srrfFlags);

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);
  if (! dwRet)
  {
    /* SHQueryValueEx expands Environment strings */
    dwRet = SHQueryValueExA(hSubKey, lpszValue, 0, pwType, pvData, pcbData);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHReg GetRegValueW   [SHLWAPI.@]
 *
 * See SHGetValueA.
 */
DWORD WINAPI SHRegGetValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue, DWORD srrfFlags,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s,%s,0x%08x, %p,%p,%p)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), srrfFlags,pwType, pvData, pcbData);
  FIXME("Semi-Stub: Find meaning and implement handling of SRFF Flags 0x%08x\n", srrfFlags);

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);
  if (! dwRet)
  {
    dwRet = SHQueryValueExW(hSubKey, lpszValue, 0, pwType, pvData, pcbData);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHGetValueW   [SHLWAPI.@]
 *
 * See SHGetValueA.
 */
DWORD WINAPI SHGetValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s,%s,%p,%p,%p)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), pwType, pvData, pcbData);

  if(lpszSubKey)
    dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);

  if (! dwRet)
  {
    dwRet = SHQueryValueExW(hSubKey ? hSubKey : hKey, lpszValue, 0, pwType, pvData, pcbData);
    if (hSubKey) RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHSetValueA   [SHLWAPI.@]
 *
 * Set a value in the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key under hKey
 *   lpszValue  [I] Name of value to set
 *   dwType     [I] Type of the value
 *   pvData     [I] Data of the value
 *   cbData     [I] Size of the value
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The value is set with the data given.
 *   Failure: An error code from RegCreateKeyExA() or RegSetValueExA()
 *
 * NOTES
 *   If lpszSubKey does not exist, it is created before the value is set. If
 *   lpszSubKey is NULL or an empty string, then the value is added directly
 *   to hKey instead.
 */
DWORD WINAPI SHSetValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                         DWORD dwType, LPCVOID pvData, DWORD cbData)
{
  DWORD dwRet = ERROR_SUCCESS, dwDummy;
  HKEY  hSubKey;

  TRACE("(hkey=%p,%s,%s,%d,%p,%d)\n", hKey, debugstr_a(lpszSubKey),
          debugstr_a(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExA(hKey, lpszSubKey, 0, NULL,
                            0, KEY_SET_VALUE, NULL, &hSubKey, &dwDummy);
  else
    hSubKey = hKey;
  if (!dwRet)
  {
    dwRet = RegSetValueExA(hSubKey, lpszValue, 0, dwType, pvData, cbData);
    if (hSubKey != hKey)
      RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHSetValueW   [SHLWAPI.@]
 *
 * See SHSetValueA.
 */
DWORD WINAPI SHSetValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                         DWORD dwType, LPCVOID pvData, DWORD cbData)
{
  DWORD dwRet = ERROR_SUCCESS, dwDummy;
  HKEY  hSubKey;

  TRACE("(hkey=%p,%s,%s,%d,%p,%d)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExW(hKey, lpszSubKey, 0, NULL,
                            0, KEY_SET_VALUE, NULL, &hSubKey, &dwDummy);
  else
    hSubKey = hKey;
  if (!dwRet)
  {
    dwRet = RegSetValueExW(hSubKey, lpszValue, 0, dwType, pvData, cbData);
    if (hSubKey != hKey)
      RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHQueryInfoKeyA   [SHLWAPI.@]
 *
 * Get information about a registry key. See RegQueryInfoKeyA().
 *
 * RETURNS
 *  The result of calling RegQueryInfoKeyA().
 */
LONG WINAPI SHQueryInfoKeyA(HKEY hKey, LPDWORD pwSubKeys, LPDWORD pwSubKeyMax,
                            LPDWORD pwValues, LPDWORD pwValueMax)
{
  TRACE("(hkey=%p,%p,%p,%p,%p)\n", hKey, pwSubKeys, pwSubKeyMax,
        pwValues, pwValueMax);
  return RegQueryInfoKeyA(hKey, NULL, NULL, NULL, pwSubKeys, pwSubKeyMax,
                          NULL, pwValues, pwValueMax, NULL, NULL, NULL);
}

/*************************************************************************
 * SHQueryInfoKeyW   [SHLWAPI.@]
 *
 * See SHQueryInfoKeyA.
 */
LONG WINAPI SHQueryInfoKeyW(HKEY hKey, LPDWORD pwSubKeys, LPDWORD pwSubKeyMax,
                            LPDWORD pwValues, LPDWORD pwValueMax)
{
  TRACE("(hkey=%p,%p,%p,%p,%p)\n", hKey, pwSubKeys, pwSubKeyMax,
        pwValues, pwValueMax);
  return RegQueryInfoKeyW(hKey, NULL, NULL, NULL, pwSubKeys, pwSubKeyMax,
                          NULL, pwValues, pwValueMax, NULL, NULL, NULL);
}

/*************************************************************************
 * SHQueryValueExA   [SHLWAPI.@]
 *
 * Get a value from the registry, expanding environment variable strings.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszValue  [I] Name of value to query
 *   lpReserved [O] Reserved for future use; must be NULL
 *   pwType     [O] Optional pointer updated with the values type
 *   pvData     [O] Optional pointer updated with the values data
 *   pcbData    [O] Optional pointer updated with the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Any non NULL output parameters are updated with
 *            information about the value.
 *   Failure: ERROR_OUTOFMEMORY if memory allocation fails, or the type of the
 *            data is REG_EXPAND_SZ and pcbData is NULL. Otherwise an error
 *            code from RegQueryValueExA() or ExpandEnvironmentStringsA().
 *
 * NOTES
 *   Either pwType, pvData or pcbData may be NULL if the caller doesn't want
 *   the type, data or size information for the value.
 *
 *   If the type of the data is REG_EXPAND_SZ, it is expanded to REG_SZ. The
 *   value returned will be truncated if it is of type REG_SZ and bigger than
 *   the buffer given to store it.
 *
 *   REG_EXPAND_SZ:
 *     case-1: the unexpanded string is smaller than the expanded one
 *       subcase-1: the buffer is too small to hold the unexpanded string:
 *          function fails and returns the size of the unexpanded string.
 *
 *       subcase-2: buffer is too small to hold the expanded string:
 *          the function return success (!!) and the result is truncated
 *	    *** This is clearly an error in the native implementation. ***
 *
 *     case-2: the unexpanded string is bigger than the expanded one
 *       The buffer must have enough space to hold the unexpanded
 *       string even if the result is smaller.
 *
 */
DWORD WINAPI SHQueryValueExA( HKEY hKey, LPCSTR lpszValue,
                              LPDWORD lpReserved, LPDWORD pwType,
                              LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = 0, dwExpDataLen;

  TRACE("(hkey=%p,%s,%p,%p,%p,%p=%d)\n", hKey, debugstr_a(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwUnExpDataLen = *pcbData;

  dwRet = RegQueryValueExA(hKey, lpszValue, lpReserved, &dwType, pvData, &dwUnExpDataLen);

  if (pcbData && (dwType == REG_EXPAND_SZ))
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPSTR szData;

    /* If the caller didn't supply a buffer or the buffer is too small we have
     * to allocate our own
     */
    if ((!pvData) || (dwRet == ERROR_MORE_DATA) )
    {
      char cNull = '\0';
      nBytesToAlloc = dwUnExpDataLen;

      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExA (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
    else
    {
      nBytesToAlloc = (lstrlenA(pvData)+1) * sizeof (CHAR);
      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      lstrcpyA(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, pvData, *pcbData / sizeof(CHAR));
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
  }

  /* Update the type and data size if the caller wanted them */
  if ( dwType == REG_EXPAND_SZ ) dwType = REG_SZ;
  if ( pwType ) *pwType = dwType;
  if ( pcbData ) *pcbData = dwUnExpDataLen;
  return dwRet;
}


/*************************************************************************
 * SHQueryValueExW   [SHLWAPI.@]
 *
 * See SHQueryValueExA.
 */
DWORD WINAPI SHQueryValueExW(HKEY hKey, LPCWSTR lpszValue,
                             LPDWORD lpReserved, LPDWORD pwType,
                             LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = 0, dwExpDataLen;

  TRACE("(hkey=%p,%s,%p,%p,%p,%p=%d)\n", hKey, debugstr_w(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwUnExpDataLen = *pcbData;

  dwRet = RegQueryValueExW(hKey, lpszValue, lpReserved, &dwType, pvData, &dwUnExpDataLen);
  if (dwRet!=ERROR_SUCCESS && dwRet!=ERROR_MORE_DATA)
      return dwRet;

  if (pcbData && (dwType == REG_EXPAND_SZ))
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPWSTR szData;

    /* If the caller didn't supply a buffer or the buffer is too small we have
     * to allocate our own
     */
    if ((!pvData) || (dwRet == ERROR_MORE_DATA) )
    {
      WCHAR cNull = '\0';
      nBytesToAlloc = dwUnExpDataLen;

      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExW (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
    else
    {
      nBytesToAlloc = (lstrlenW(pvData) + 1) * sizeof(WCHAR);
      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      lstrcpyW(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, pvData, *pcbData/sizeof(WCHAR) );
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
  }

  /* Update the type and data size if the caller wanted them */
  if ( dwType == REG_EXPAND_SZ ) dwType = REG_SZ;
  if ( pwType ) *pwType = dwType;
  if ( pcbData ) *pcbData = dwUnExpDataLen;
  return dwRet;
}

/*************************************************************************
 * SHDeleteKeyA   [SHLWAPI.@]
 *
 * Delete a registry key and any sub keys/values present
 *
 * This function forwards to the unicode version directly, to avoid
 * handling subkeys that are not representable in ASCII.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key to delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The key is deleted.
 *   Failure: An error code from RegOpenKeyExA(), RegQueryInfoKeyA(),
 *            RegEnumKeyExA() or RegDeleteKeyA().
 */
DWORD WINAPI SHDeleteKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
  WCHAR subkeyW[MAX_PATH];

  MultiByteToWideChar (CP_ACP, 0, lpszSubKey, -1, subkeyW, sizeof(subkeyW)/sizeof(WCHAR));
  return SHDeleteKeyW(hKey, subkeyW);
}

/*************************************************************************
 * SHDeleteKeyW   [SHLWAPI.@]
 *
 * See SHDeleteKeyA.
 */
DWORD WINAPI SHDeleteKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  DWORD dwRet, dwMaxSubkeyLen = 0, dwSize;
  WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    /* Find the maximum subkey length so that we can allocate a buffer */
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
                             &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(!dwRet)
    {
      dwMaxSubkeyLen++;
      if (dwMaxSubkeyLen > sizeof(szNameBuf)/sizeof(WCHAR))
        /* Name too big: alloc a buffer for it */
        lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxSubkeyLen*sizeof(WCHAR));

      if(!lpszName)
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
      else
      {
        while (dwRet == ERROR_SUCCESS)
        {
          dwSize = dwMaxSubkeyLen;
          dwRet = RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL, NULL, NULL, NULL);
          if (dwRet == ERROR_SUCCESS || dwRet == ERROR_MORE_DATA)
            dwRet = SHDeleteKeyW(hSubKey, lpszName);
        }
        if (dwRet == ERROR_NO_MORE_ITEMS)
          dwRet = ERROR_SUCCESS;
    
        if (lpszName != szNameBuf)
          HeapFree(GetProcessHeap(), 0, lpszName); /* Free buffer if allocated */
      }
    }

    RegCloseKey(hSubKey);
    if(!dwRet)
      dwRet = RegDeleteKeyW(hKey, lpszSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteEmptyKeyA   [SHLWAPI.@]
 *
 * Delete a registry key with no sub keys.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key to delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The key is deleted.
 *   Failure: If the key is not empty, returns ERROR_KEY_HAS_CHILDREN. Otherwise
 *            returns an error code from RegOpenKeyExA(), RegQueryInfoKeyA() or
 *            RegDeleteKeyA().
 */
DWORD WINAPI SHDeleteEmptyKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_a(lpszSubKey));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    dwRet = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    RegCloseKey(hSubKey);
    if(!dwRet)
    {
      if (!dwKeyCount)
        dwRet = RegDeleteKeyA(hKey, lpszSubKey);
      else
        dwRet = ERROR_KEY_HAS_CHILDREN;
    }
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteEmptyKeyW   [SHLWAPI.@]
 *
 * See SHDeleteEmptyKeyA.
 */
DWORD WINAPI SHDeleteEmptyKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p, %s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    RegCloseKey(hSubKey);
    if(!dwRet)
    {
      if (!dwKeyCount)
        dwRet = RegDeleteKeyW(hKey, lpszSubKey);
      else
        dwRet = ERROR_KEY_HAS_CHILDREN;
    }
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteOrphanKeyA   [SHLWAPI.@]
 *
 * Delete a registry key with no sub keys or values.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key to possibly delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The key has been deleted if it was an orphan.
 *   Failure: An error from RegOpenKeyExA(), RegQueryValueExA(), or RegDeleteKeyA().
 */
DWORD WINAPI SHDeleteOrphanKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
  HKEY hSubKey;
  DWORD dwKeyCount = 0, dwValueCount = 0, dwRet;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_a(lpszSubKey));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);

  if(!dwRet)
  {
    /* Get subkey and value count */
    dwRet = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL);

    if(!dwRet && !dwKeyCount && !dwValueCount)
    {
      dwRet = RegDeleteKeyA(hKey, lpszSubKey);
    }
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteOrphanKeyW   [SHLWAPI.@]
 *
 * See SHDeleteOrphanKeyA.
 */
DWORD WINAPI SHDeleteOrphanKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  HKEY hSubKey;
  DWORD dwKeyCount = 0, dwValueCount = 0, dwRet;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);

  if(!dwRet)
  {
    /* Get subkey and value count */
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL);

    if(!dwRet && !dwKeyCount && !dwValueCount)
    {
      dwRet = RegDeleteKeyW(hKey, lpszSubKey);
    }
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteValueA   [SHLWAPI.@]
 *
 * Delete a value from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing value to delete
 *   lpszValue  [I] Name of value to delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The value is deleted.
 *   Failure: An error code from RegOpenKeyExA() or RegDeleteValueA().
 */
DWORD WINAPI SHDeleteValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue)
{
  DWORD dwRet;
  HKEY hSubKey;

  TRACE("(hkey=%p,%s,%s)\n", hKey, debugstr_a(lpszSubKey), debugstr_a(lpszValue));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey);
  if (!dwRet)
  {
    dwRet = RegDeleteValueA(hSubKey, lpszValue);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteValueW   [SHLWAPI.@]
 *
 * See SHDeleteValueA.
 */
DWORD WINAPI SHDeleteValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue)
{
  DWORD dwRet;
  HKEY hSubKey;

  TRACE("(hkey=%p,%s,%s)\n", hKey, debugstr_w(lpszSubKey), debugstr_w(lpszValue));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey);
  if (!dwRet)
  {
    dwRet = RegDeleteValueW(hSubKey, lpszValue);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHEnumKeyExA   [SHLWAPI.@]
 *
 * Enumerate sub keys in a registry key.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   dwIndex    [I] Index of key to enumerate
 *   lpszSubKey [O] Pointer updated with the subkey name
 *   pwLen      [O] Pointer updated with the subkey length
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. lpszSubKey and pwLen are updated.
 *   Failure: An error code from RegEnumKeyExA().
 */
LONG WINAPI SHEnumKeyExA(HKEY hKey, DWORD dwIndex, LPSTR lpszSubKey,
                         LPDWORD pwLen)
{
  TRACE("(hkey=%p,%d,%s,%p)\n", hKey, dwIndex, debugstr_a(lpszSubKey), pwLen);

  return RegEnumKeyExA(hKey, dwIndex, lpszSubKey, pwLen, NULL, NULL, NULL, NULL);
}

/*************************************************************************
 * SHEnumKeyExW   [SHLWAPI.@]
 *
 * See SHEnumKeyExA.
 */
LONG WINAPI SHEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpszSubKey,
                         LPDWORD pwLen)
{
  TRACE("(hkey=%p,%d,%s,%p)\n", hKey, dwIndex, debugstr_w(lpszSubKey), pwLen);

  return RegEnumKeyExW(hKey, dwIndex, lpszSubKey, pwLen, NULL, NULL, NULL, NULL);
}

/*************************************************************************
 * SHEnumValueA   [SHLWAPI.@]
 *
 * Enumerate values in a registry key.
 *
 * PARAMS
 *   hKey      [I] Handle to registry key
 *   dwIndex   [I] Index of key to enumerate
 *   lpszValue [O] Pointer updated with the values name
 *   pwLen     [O] Pointer updated with the values length
 *   pwType    [O] Pointer updated with the values type
 *   pvData    [O] Pointer updated with the values data
 *   pcbData   [O] Pointer updated with the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters are updated.
 *   Failure: An error code from RegEnumValueA().
 */
LONG WINAPI SHEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpszValue,
                         LPDWORD pwLen, LPDWORD pwType,
                         LPVOID pvData, LPDWORD pcbData)
{
  TRACE("(hkey=%p,%d,%s,%p,%p,%p,%p)\n", hKey, dwIndex,
        debugstr_a(lpszValue), pwLen, pwType, pvData, pcbData);

  return RegEnumValueA(hKey, dwIndex, lpszValue, pwLen, NULL,
                       pwType, pvData, pcbData);
}

/*************************************************************************
 * SHEnumValueW   [SHLWAPI.@]
 *
 * See SHEnumValueA.
 */
LONG WINAPI SHEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpszValue,
                         LPDWORD pwLen, LPDWORD pwType,
                         LPVOID pvData, LPDWORD pcbData)
{
  TRACE("(hkey=%p,%d,%s,%p,%p,%p,%p)\n", hKey, dwIndex,
        debugstr_w(lpszValue), pwLen, pwType, pvData, pcbData);

  return RegEnumValueW(hKey, dwIndex, lpszValue, pwLen, NULL,
                       pwType, pvData, pcbData);
}

/*************************************************************************
 * @   [SHLWAPI.205]
 *
 * Get a value from the registry.
 *
 * PARAMS
 *   hKey    [I] Handle to registry key
 *   pSubKey [I] Name of sub key containing value to get
 *   pValue  [I] Name of value to get
 *   pwType  [O] Destination for the values type
 *   pvData  [O] Destination for the values data
 *   pbData  [O] Destination for the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters contain the details read.
 *   Failure: An error code from RegOpenKeyExA() or SHQueryValueExA(),
 *            or ERROR_INVALID_FUNCTION in the machine is in safe mode.
 */
DWORD WINAPI SHGetValueGoodBootA(HKEY hkey, LPCSTR pSubKey, LPCSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueA(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * @   [SHLWAPI.206]
 *
 * Unicode version of SHGetValueGoodBootW.
 */
DWORD WINAPI SHGetValueGoodBootW(HKEY hkey, LPCWSTR pSubKey, LPCWSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueW(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * @   [SHLWAPI.320]
 *
 * Set a MIME content type in the registry.
 *
 * PARAMS
 *   lpszSubKey [I] Name of key under HKEY_CLASSES_ROOT.
 *   lpszValue  [I] Value to set
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI RegisterMIMETypeForExtensionA(LPCSTR lpszSubKey, LPCSTR lpszValue)
{
  DWORD dwRet;

  if (!lpszValue)
  {
    WARN("Invalid lpszValue would crash under Win32!\n");
    return FALSE;
  }

  dwRet = SHSetValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA,
                      REG_SZ, lpszValue, strlen(lpszValue));
  return dwRet ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.321]
 *
 * Unicode version of RegisterMIMETypeForExtensionA.
 */
BOOL WINAPI RegisterMIMETypeForExtensionW(LPCWSTR lpszSubKey, LPCWSTR lpszValue)
{
  DWORD dwRet;

  if (!lpszValue)
  {
    WARN("Invalid lpszValue would crash under Win32!\n");
    return FALSE;
  }

  dwRet = SHSetValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW,
                      REG_SZ, lpszValue, strlenW(lpszValue));
  return dwRet ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.322]
 *
 * Delete a MIME content type from the registry.
 *
 * PARAMS
 *   lpszSubKey [I] Name of sub key
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI UnregisterMIMETypeForExtensionA(LPCSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA);
  return ret ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.323]
 *
 * Unicode version of UnregisterMIMETypeForExtensionA.
 */
BOOL WINAPI UnregisterMIMETypeForExtensionW(LPCWSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW);
  return ret ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.328]
 *
 * Get the registry path to a MIME content key.
 *
 * PARAMS
 *   lpszType   [I] Content type to get the path for
 *   lpszBuffer [O] Destination for path
 *   dwLen      [I] Length of lpszBuffer
 *
 * RETURNS
 *   Success: TRUE. lpszBuffer contains the full path.
 *   Failure: FALSE.
 *
 * NOTES
 *   The base path for the key is "MIME\Database\Content Type\"
 */
BOOL WINAPI GetMIMETypeSubKeyA(LPCSTR lpszType, LPSTR lpszBuffer, DWORD dwLen)
{
  TRACE("(%s,%p,%d)\n", debugstr_a(lpszType), lpszBuffer, dwLen);

  if (dwLen > dwLenMimeDbContent && lpszType && lpszBuffer)
  {
    size_t dwStrLen = strlen(lpszType);

    if (dwStrLen < dwLen - dwLenMimeDbContent)
    {
      memcpy(lpszBuffer, szMimeDbContentA, dwLenMimeDbContent);
      memcpy(lpszBuffer + dwLenMimeDbContent, lpszType, dwStrLen + 1);
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************
 * @   [SHLWAPI.329]
 *
 * Unicode version of GetMIMETypeSubKeyA.
 */
BOOL WINAPI GetMIMETypeSubKeyW(LPCWSTR lpszType, LPWSTR lpszBuffer, DWORD dwLen)
{
  TRACE("(%s,%p,%d)\n", debugstr_w(lpszType), lpszBuffer, dwLen);

  if (dwLen > dwLenMimeDbContent && lpszType && lpszBuffer)
  {
    DWORD dwStrLen = strlenW(lpszType);

    if (dwStrLen < dwLen - dwLenMimeDbContent)
    {
      memcpy(lpszBuffer, szMimeDbContentW, dwLenMimeDbContent * sizeof(WCHAR));
      memcpy(lpszBuffer + dwLenMimeDbContent, lpszType, (dwStrLen + 1) * sizeof(WCHAR));
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************
 * @   [SHLWAPI.330]
 *
 * Get the file extension for a given Mime type.
 *
 * PARAMS
 *  lpszType [I] Mime type to get the file extension for
 *  lpExt    [O] Destination for the resulting extension
 *  iLen     [I] Length of lpExt in characters
 *
 * RETURNS
 *  Success: TRUE. lpExt contains the file extension.
 *  Failure: FALSE, if any parameter is invalid or the extension cannot be
 *           retrieved. If iLen > 0, lpExt is set to an empty string.
 *
 * NOTES
 *  - The extension returned in lpExt always has a leading '.' character, even
 *  if the registry Mime database entry does not.
 *  - iLen must be long enough for the file extension for this function to succeed.
 */
BOOL WINAPI MIME_GetExtensionA(LPCSTR lpszType, LPSTR lpExt, INT iLen)
{
  char szSubKey[MAX_PATH];
  DWORD dwlen = iLen - 1, dwType;
  BOOL bRet = FALSE;

  if (iLen > 0 && lpExt)
    *lpExt = '\0';

  if (lpszType && lpExt && iLen > 2 &&
      GetMIMETypeSubKeyA(lpszType, szSubKey, MAX_PATH) &&
      !SHGetValueA(HKEY_CLASSES_ROOT, szSubKey, szExtensionA, &dwType, lpExt + 1, &dwlen) &&
      lpExt[1])
  {
    if (lpExt[1] == '.')
      memmove(lpExt, lpExt + 1, strlen(lpExt + 1) + 1);
    else
      *lpExt = '.'; /* Supply a '.' */
    bRet = TRUE;
  }
  return bRet;
}

/*************************************************************************
 * @   [SHLWAPI.331]
 *
 * Unicode version of MIME_GetExtensionA.
 */
BOOL WINAPI MIME_GetExtensionW(LPCWSTR lpszType, LPWSTR lpExt, INT iLen)
{
  WCHAR szSubKey[MAX_PATH];
  DWORD dwlen = iLen - 1, dwType;
  BOOL bRet = FALSE;

  if (iLen > 0 && lpExt)
    *lpExt = '\0';

  if (lpszType && lpExt && iLen > 2 &&
      GetMIMETypeSubKeyW(lpszType, szSubKey, MAX_PATH) &&
      !SHGetValueW(HKEY_CLASSES_ROOT, szSubKey, szExtensionW, &dwType, lpExt + 1, &dwlen) &&
      lpExt[1])
  {
    if (lpExt[1] == '.')
      memmove(lpExt, lpExt + 1, (strlenW(lpExt + 1) + 1) * sizeof(WCHAR));
    else
      *lpExt = '.'; /* Supply a '.' */
    bRet = TRUE;
  }
  return bRet;
}

/*************************************************************************
 * @   [SHLWAPI.324]
 *
 * Set the file extension for a MIME content key.
 *
 * PARAMS
 *   lpszExt  [I] File extension to set
 *   lpszType [I] Content type to set the extension for
 *
 * RETURNS
 *   Success: TRUE. The file extension is set in the registry.
 *   Failure: FALSE.
 */
BOOL WINAPI RegisterExtensionForMIMETypeA(LPCSTR lpszExt, LPCSTR lpszType)
{
  DWORD dwLen;
  char szKey[MAX_PATH];

  TRACE("(%s,%s)\n", debugstr_a(lpszExt), debugstr_a(lpszType));

  if (!GetMIMETypeSubKeyA(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  dwLen = strlen(lpszExt) + 1;

  if (SHSetValueA(HKEY_CLASSES_ROOT, szKey, szExtensionA, REG_SZ, lpszExt, dwLen))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.325]
 *
 * Unicode version of RegisterExtensionForMIMETypeA.
 */
BOOL WINAPI RegisterExtensionForMIMETypeW(LPCWSTR lpszExt, LPCWSTR lpszType)
{
  DWORD dwLen;
  WCHAR szKey[MAX_PATH];

  TRACE("(%s,%s)\n", debugstr_w(lpszExt), debugstr_w(lpszType));

  /* Get the full path to the key */
  if (!GetMIMETypeSubKeyW(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  dwLen = (lstrlenW(lpszExt) + 1) * sizeof(WCHAR);

  if (SHSetValueW(HKEY_CLASSES_ROOT, szKey, szExtensionW, REG_SZ, lpszExt, dwLen))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.326]
 *
 * Delete a file extension from a MIME content type.
 *
 * PARAMS
 *   lpszType [I] Content type to delete the extension for
 *
 * RETURNS
 *   Success: TRUE. The file extension is deleted from the registry.
 *   Failure: FALSE. The extension may have been removed but the key remains.
 *
 * NOTES
 *   If deleting the extension leaves an orphan key, the key is removed also.
 */
BOOL WINAPI UnregisterExtensionForMIMETypeA(LPCSTR lpszType)
{
  char szKey[MAX_PATH];

  TRACE("(%s)\n", debugstr_a(lpszType));

  if (!GetMIMETypeSubKeyA(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  if (!SHDeleteValueA(HKEY_CLASSES_ROOT, szKey, szExtensionA))
    return FALSE;

  if (!SHDeleteOrphanKeyA(HKEY_CLASSES_ROOT, szKey))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.327]
 *
 * Unicode version of UnregisterExtensionForMIMETypeA.
 */
BOOL WINAPI UnregisterExtensionForMIMETypeW(LPCWSTR lpszType)
{
  WCHAR szKey[MAX_PATH];

  TRACE("(%s)\n", debugstr_w(lpszType));

  if (!GetMIMETypeSubKeyW(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  if (!SHDeleteValueW(HKEY_CLASSES_ROOT, szKey, szExtensionW))
    return FALSE;

  if (!SHDeleteOrphanKeyW(HKEY_CLASSES_ROOT, szKey))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * SHRegDuplicateHKey   [SHLWAPI.@]
 *
 * Create a duplicate of a registry handle.
 *
 * PARAMS
 *  hKey [I] key to duplicate.
 *
 * RETURNS
 *  A new handle pointing to the same key as hKey.
 */
HKEY WINAPI SHRegDuplicateHKey(HKEY hKey)
{
    HKEY newKey = 0;

    RegOpenKeyExA(hKey, 0, 0, MAXIMUM_ALLOWED, &newKey);
    TRACE("new key is %p\n", newKey);
    return newKey;
}


/*************************************************************************
 * SHCopyKeyA	[SHLWAPI.@]
 *
 * Copy a key and its values/sub keys to another location.
 *
 * PARAMS
 *  hKeySrc    [I] Source key to copy from
 *  lpszSrcSubKey [I] Sub key under hKeySrc, or NULL to use hKeySrc directly
 *  hKeyDst    [I] Destination key
 *  dwReserved [I] Reserved, must be 0
 *
 * RETURNS
 *  Success: ERROR_SUCCESS. The key is copied to the destination key.
 *  Failure: A standard windows error code.
 *
 * NOTES
 *  If hKeyDst is a key under hKeySrc, this function will misbehave
 *  (It will loop until out of stack, or the registry is full). This
 *  bug is present in Win32 also.
 */
DWORD WINAPI SHCopyKeyA(HKEY hKeySrc, LPCSTR lpszSrcSubKey, HKEY hKeyDst, DWORD dwReserved)
{
  WCHAR szSubKeyW[MAX_PATH];

  TRACE("(hkey=%p,%s,%p08x,%d)\n", hKeySrc, debugstr_a(lpszSrcSubKey), hKeyDst, dwReserved);

  if (lpszSrcSubKey)
    MultiByteToWideChar(0, 0, lpszSrcSubKey, -1, szSubKeyW, MAX_PATH);

  return SHCopyKeyW(hKeySrc, lpszSrcSubKey ? szSubKeyW : NULL, hKeyDst, dwReserved);
}

/*************************************************************************
 * SHCopyKeyW	[SHLWAPI.@]
 *
 * See SHCopyKeyA.
 */
DWORD WINAPI SHCopyKeyW(HKEY hKeySrc, LPCWSTR lpszSrcSubKey, HKEY hKeyDst, DWORD dwReserved)
{
  DWORD dwKeyCount = 0, dwValueCount = 0, dwMaxKeyLen = 0;
  DWORD  dwMaxValueLen = 0, dwMaxDataLen = 0, i;
  BYTE buff[1024];
  LPVOID lpBuff = buff;
  WCHAR szName[MAX_PATH], *lpszName = szName;
  DWORD dwRet = S_OK;

  TRACE("hkey=%p,%s,%p08x,%d)\n", hKeySrc, debugstr_w(lpszSrcSubKey), hKeyDst, dwReserved);

  if(!hKeyDst || !hKeySrc)
    dwRet = ERROR_INVALID_PARAMETER;
  else
  {
    /* Open source key */
    if(lpszSrcSubKey)
      dwRet = RegOpenKeyExW(hKeySrc, lpszSrcSubKey, 0, KEY_ALL_ACCESS, &hKeySrc);

    if(dwRet)
      hKeyDst = NULL; /* Don't close this key since we didn't open it */
    else
    {
      /* Get details about sub keys and values */
      dwRet = RegQueryInfoKeyW(hKeySrc, NULL, NULL, NULL, &dwKeyCount, &dwMaxKeyLen,
                               NULL, &dwValueCount, &dwMaxValueLen, &dwMaxDataLen,
                               NULL, NULL);
      if(!dwRet)
      {
        if (dwMaxValueLen > dwMaxKeyLen)
          dwMaxKeyLen = dwMaxValueLen; /* Get max size for key/value names */

        if (dwMaxKeyLen++ > MAX_PATH - 1)
          lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxKeyLen * sizeof(WCHAR));

        if (dwMaxDataLen > sizeof(buff))
          lpBuff = HeapAlloc(GetProcessHeap(), 0, dwMaxDataLen);

        if (!lpszName || !lpBuff)
          dwRet = ERROR_NOT_ENOUGH_MEMORY;
      }
    }
  }

  /* Copy all the sub keys */
  for(i = 0; i < dwKeyCount && !dwRet; i++)
  {
    HKEY hSubKeySrc, hSubKeyDst;
    DWORD dwSize = dwMaxKeyLen;

    dwRet = RegEnumKeyExW(hKeySrc, i, lpszName, &dwSize, NULL, NULL, NULL, NULL);

    if(!dwRet)
    {
      /* Open source sub key */
      dwRet = RegOpenKeyExW(hKeySrc, lpszName, 0, KEY_READ, &hSubKeySrc);

      if(!dwRet)
      {
        /* Create destination sub key */
        dwRet = RegCreateKeyW(hKeyDst, lpszName, &hSubKeyDst);

        if(!dwRet)
        {
          /* Recursively copy keys and values from the sub key */
          dwRet = SHCopyKeyW(hSubKeySrc, NULL, hSubKeyDst, 0);
          RegCloseKey(hSubKeyDst);
        }
      }
      RegCloseKey(hSubKeySrc);
    }
  }

  /* Copy all the values in this key */
  for (i = 0; i < dwValueCount && !dwRet; i++)
  {
    DWORD dwNameSize = dwMaxKeyLen, dwType, dwLen = dwMaxDataLen;

    dwRet = RegEnumValueW(hKeySrc, i, lpszName, &dwNameSize, NULL, &dwType, lpBuff, &dwLen);

    if (!dwRet)
      dwRet = SHSetValueW(hKeyDst, NULL, lpszName, dwType, lpBuff, dwLen);
  }

  /* Free buffers if allocated */
  if (lpszName != szName)
    HeapFree(GetProcessHeap(), 0, lpszName);
  if (lpBuff != buff)
    HeapFree(GetProcessHeap(), 0, lpBuff);

  if (lpszSrcSubKey && hKeyDst)
    RegCloseKey(hKeyDst);
  return dwRet;
}

/*
 * The following functions are ORDINAL ONLY:
 */

/*************************************************************************
 *      @     [SHLWAPI.280]
 *
 * Read an integer value from the registry, falling back to a default.
 *
 * PARAMS
 *  hKey      [I] Registry key to read from
 *  lpszValue [I] Value name to read
 *  iDefault  [I] Default value to return
 *
 * RETURNS
 *  The value contained in the given registry value if present, otherwise
 *  iDefault.
 */
int WINAPI SHRegGetIntW(HKEY hKey, LPCWSTR lpszValue, int iDefault)
{
  TRACE("(%p,%s,%d)\n", hKey, debugstr_w(lpszValue), iDefault);

  if (hKey)
  {
    WCHAR szBuff[32];
    DWORD dwSize = sizeof(szBuff);
    szBuff[0] = '\0';
    SHQueryValueExW(hKey, lpszValue, 0, 0, szBuff, &dwSize);

    if(*szBuff >= '0' && *szBuff <= '9')
      return StrToIntW(szBuff);
  }
  return iDefault;
}

/*************************************************************************
 *      @	[SHLWAPI.343]
 *
 * Create or open an explorer ClassId Key.
 *
 * PARAMS
 *  guid      [I] Explorer ClassId key to open
 *  lpszValue [I] Value name under the ClassId Key
 *  bUseHKCU  [I] TRUE=Use HKEY_CURRENT_USER, FALSE=Use HKEY_CLASSES_ROOT
 *  bCreate   [I] TRUE=Create the key if it doesn't exist, FALSE=Don't
 *  phKey     [O] Destination for the resulting key handle
 *
 * RETURNS
 *  Success: S_OK. phKey contains the resulting registry handle.
 *  Failure: An HRESULT error code indicating the problem.
 */
HRESULT WINAPI SHRegGetCLSIDKeyA(REFGUID guid, LPCSTR lpszValue, BOOL bUseHKCU, BOOL bCreate, PHKEY phKey)
{
  WCHAR szValue[MAX_PATH];

  if (lpszValue)
    MultiByteToWideChar(CP_ACP, 0, lpszValue, -1, szValue, sizeof(szValue)/sizeof(WCHAR));

  return SHRegGetCLSIDKeyW(guid, lpszValue ? szValue : NULL, bUseHKCU, bCreate, phKey);
}

/*************************************************************************
 *      @	[SHLWAPI.344]
 *
 * Unicode version of SHRegGetCLSIDKeyA.
 */
HRESULT WINAPI SHRegGetCLSIDKeyW(REFGUID guid, LPCWSTR lpszValue, BOOL bUseHKCU,
                           BOOL bCreate, PHKEY phKey)
{
  static const WCHAR szClassIdKey[] = { 'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'E','x','p','l','o','r','e','r','\\','C','L','S','I','D','\\' };
#define szClassIdKeyLen (sizeof(szClassIdKey)/sizeof(WCHAR))
  WCHAR szKey[MAX_PATH];
  DWORD dwRet;
  HKEY hkey;

  /* Create the key string */
  memcpy(szKey, szClassIdKey, sizeof(szClassIdKey));
  SHStringFromGUIDW(guid, szKey + szClassIdKeyLen, 39); /* Append guid */

  if(lpszValue)
  {
    szKey[szClassIdKeyLen + 39] = '\\';
    strcpyW(szKey + szClassIdKeyLen + 40, lpszValue); /* Append value name */
  }

  hkey = bUseHKCU ? HKEY_CURRENT_USER : HKEY_CLASSES_ROOT;

  if(bCreate)
    dwRet = RegCreateKeyW(hkey, szKey, phKey);
  else
    dwRet = RegOpenKeyExW(hkey, szKey, 0, KEY_READ, phKey);

  return dwRet ? HRESULT_FROM_WIN32(dwRet) : S_OK;
}

/*************************************************************************
 * SHRegisterValidateTemplate	[SHLWAPI.@]
 *
 * observed from the ie 5.5 installer:
 *  - allocates a buffer with the size of the given file
 *  - read the file content into the buffer
 *  - creates the key szTemplateKey
 *  - sets "205523652929647911071668590831910975402"=dword:00002e37 at
 *    the key
 *
 * PARAMS
 *  filename [I] An existing file its content is read into an allocated
 *               buffer
 *  unknown  [I]
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 */
HRESULT WINAPI SHRegisterValidateTemplate(LPCWSTR filename, BOOL unknown)
{
/* static const WCHAR szTemplateKey[] = { 'S','o','f','t','w','a','r','e','\\',
 *  'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
 *  'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
 *  'E','x','p','l','o','r','e','r','\\',
 *  'T','e','m','p','l','a','t','e','R','e','g','i','s','t','r','y',0 };
 */
  FIXME("stub: %s, %08x\n", debugstr_w(filename), unknown);

  return S_OK;
}
