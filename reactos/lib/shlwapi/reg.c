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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
static const char *lpszContentTypeA = "Content Type";
static const WCHAR lpszContentTypeW[] = { 'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

static const char *szMimeDbContentA = "MIME\\Database\\Content Type\\";
static const WCHAR szMimeDbContentW[] = { 'M', 'I', 'M','E','\\',
  'D','a','t','a','b','a','s','e','\\','C','o','n','t','e','n','t',
  ' ','T','y','p','e','\\', 0 };
static const DWORD dwLenMimeDbContent = 27; /* strlen of szMimeDbContentA/W */

static const char *szExtensionA = "Extension";
static const WCHAR szExtensionW[] = { 'E', 'x', 't','e','n','s','i','o','n','\0' };

/* internal structure of what the HUSKEY points to */
typedef struct {
    HKEY     HKCUkey;                  /* HKEY of opened HKCU key      */
    HKEY     HKLMkey;                  /* HKEY of opened HKLM key      */
    HKEY     start;                    /* HKEY of where to start       */
    WCHAR    key_string[MAX_PATH];     /* additional path from 'start' */
} Internal_HUSKEY, *LPInternal_HUSKEY;

DWORD   WINAPI SHStringFromGUIDW(REFGUID,LPWSTR,INT);
HRESULT WINAPI SHRegGetCLSIDKeyW(REFGUID,LPCWSTR,BOOL,BOOL,PHKEY);


#define REG_HKCU  TRUE
#define REG_HKLM  FALSE
/*************************************************************************
 * REG_GetHKEYFromHUSKEY
 *
 * Function:  Return the proper registry key from the HUSKEY structure
 *            also allow special predefined values.
 */
static HKEY WINAPI REG_GetHKEYFromHUSKEY(HUSKEY hUSKey, BOOL which)
{
        HKEY test = (HKEY) hUSKey;
        LPInternal_HUSKEY mihk = (LPInternal_HUSKEY) hUSKey;

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
 * Opens a user-specific registry key
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from RegOpenKeyExA().
 */
LONG WINAPI SHRegOpenUSKeyA(
        LPCSTR Path, /* [I] Key name to open */
        REGSAM AccessType, /* [I] Access type */
        HUSKEY hRelativeUSKey, /* [I] Relative user key */
        PHUSKEY phNewUSKey, /* [O] Destination for created key */
        BOOL fIgnoreHKCU)  /* [I] TRUE=Don't check HKEY_CURRENT_USER */
{
    HKEY openHKCUkey=0;
    HKEY openHKLMkey=0;
    LONG ret2, ret1 = ~ERROR_SUCCESS;
    LPInternal_HUSKEY ihky;

    TRACE("(%s, 0x%lx, 0x%lx, %p, %s)\n", debugstr_a(Path),
	  (LONG)AccessType, (LONG)hRelativeUSKey, phNewUSKey,
	  (fIgnoreHKCU) ? "Ignoring HKCU" : "Process HKCU then HKLM");

    /* now create the internal version of HUSKEY */
    ihky = (LPInternal_HUSKEY)HeapAlloc(GetProcessHeap(), 0 ,
					sizeof(Internal_HUSKEY));
    MultiByteToWideChar(0, 0, Path, -1, ihky->key_string,
			sizeof(ihky->key_string)-1);

    if (hRelativeUSKey) {
	openHKCUkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKCUkey;
	openHKLMkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKLMkey;
    }
    else {
	openHKCUkey = HKEY_CURRENT_USER;
	openHKLMkey = HKEY_LOCAL_MACHINE;
    }

    ihky->HKCUkey = 0;
    ihky->HKLMkey = 0;
    if (!fIgnoreHKCU) {
	ret1 = RegOpenKeyExA(openHKCUkey, Path,
			     0, AccessType, &ihky->HKCUkey);
	/* if successful, then save real starting point */
	if (ret1 != ERROR_SUCCESS)
	    ihky->HKCUkey = 0;
    }
    ret2 = RegOpenKeyExA(openHKLMkey, Path,
			 0, AccessType, &ihky->HKLMkey);
    if (ret2 != ERROR_SUCCESS)
	ihky->HKLMkey = 0;

    if ((ret1 != ERROR_SUCCESS) || (ret2 != ERROR_SUCCESS))
	TRACE("one or more opens failed: HKCU=%ld HKLM=%ld\n", ret1, ret2);

    /* if all attempts have failed then bail */
    if ((ret1 != ERROR_SUCCESS) && (ret2 != ERROR_SUCCESS)) {
	HeapFree(GetProcessHeap(), 0, ihky);
	if (phNewUSKey)
	    *phNewUSKey = NULL;
	return ret2;
    }

    TRACE("HUSKEY=%p\n", ihky);
    if (phNewUSKey)
	*phNewUSKey = (HUSKEY)ihky;
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegOpenUSKeyW	[SHLWAPI.@]
 *
 * See SHRegOpenUSKeyA.
 */
LONG WINAPI SHRegOpenUSKeyW(
        LPCWSTR Path,
        REGSAM AccessType,
        HUSKEY hRelativeUSKey,
        PHUSKEY phNewUSKey,
        BOOL fIgnoreHKCU)
{
    HKEY openHKCUkey=0;
    HKEY openHKLMkey=0;
    LONG ret2, ret1 = ~ERROR_SUCCESS;
    LPInternal_HUSKEY ihky;

    TRACE("(%s, 0x%lx, 0x%lx, %p, %s)\n", debugstr_w(Path),
	  (LONG)AccessType, (LONG)hRelativeUSKey, phNewUSKey,
	  (fIgnoreHKCU) ? "Ignoring HKCU" : "Process HKCU then HKLM");

    /* now create the internal version of HUSKEY */
    ihky = (LPInternal_HUSKEY)HeapAlloc(GetProcessHeap(), 0 ,
					sizeof(Internal_HUSKEY));
    lstrcpynW(ihky->key_string, Path, sizeof(ihky->key_string));

    if (hRelativeUSKey) {
	openHKCUkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKCUkey;
	openHKLMkey = ((LPInternal_HUSKEY)hRelativeUSKey)->HKLMkey;
    }
    else {
	openHKCUkey = HKEY_CURRENT_USER;
	openHKLMkey = HKEY_LOCAL_MACHINE;
    }

    ihky->HKCUkey = 0;
    ihky->HKLMkey = 0;
    if (!fIgnoreHKCU) {
	ret1 = RegOpenKeyExW(openHKCUkey, Path,
			    0, AccessType, &ihky->HKCUkey);
	/* if successful, then save real starting point */
	if (ret1 != ERROR_SUCCESS)
	    ihky->HKCUkey = 0;
    }
    ret2 = RegOpenKeyExW(openHKLMkey, Path,
			0, AccessType, &ihky->HKLMkey);
    if (ret2 != ERROR_SUCCESS)
	ihky->HKLMkey = 0;

    if ((ret1 != ERROR_SUCCESS) || (ret2 != ERROR_SUCCESS))
	TRACE("one or more opens failed: HKCU=%ld HKLM=%ld\n", ret1, ret2);

    /* if all attempts have failed then bail */
    if ((ret1 != ERROR_SUCCESS) && (ret2 != ERROR_SUCCESS)) {
	HeapFree(GetProcessHeap(), 0, ihky);
	if (phNewUSKey)
	    *phNewUSKey = NULL;
	return ret2;
    }

    TRACE("HUSKEY=0x%08lx\n", (LONG)ihky);
    if (phNewUSKey)
	*phNewUSKey = (HUSKEY)ihky;
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
    LPInternal_HUSKEY mihk = (LPInternal_HUSKEY)hUSKey;
    LONG ret = ERROR_SUCCESS;

    if (mihk->HKCUkey)
	ret = RegCloseKey(mihk->HKCUkey);
    if (mihk->HKLMkey)
	ret = RegCloseKey(mihk->HKLMkey);
    HeapFree(GetProcessHeap(), 0, mihk);
    return ret;
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
	    TRACE("HKCU RegQueryValue returned %08lx\n", ret);
	}

	/* if HKCU did not work and HKLM exists, then try it */
	if ((ret != ERROR_SUCCESS) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    ret = RegQueryValueExA(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKLM RegQueryValue returned %08lx\n", ret);
	}

	/* if neither worked, and default data exists, then use it */
	if (ret != ERROR_SUCCESS) {
	    if (pvDefaultData && (dwDefaultDataSize != 0)) {
		maxmove = (dwDefaultDataSize >= *pcbData) ? *pcbData : dwDefaultDataSize;
		src = (CHAR*)pvDefaultData;
		dst = (CHAR*)pvData;
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
	    TRACE("HKCU RegQueryValue returned %08lx\n", ret);
	}

	/* if HKCU did not work and HKLM exists, then try it */
	if ((ret != ERROR_SUCCESS) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	    ret = RegQueryValueExW(dokey,
				   pszValue, 0, pdwType, pvData, pcbData);
	    TRACE("HKLM RegQueryValue returned %08lx\n", ret);
	}

	/* if neither worked, and default data exists, then use it */
	if (ret != ERROR_SUCCESS) {
	    if (pvDefaultData && (dwDefaultDataSize != 0)) {
		maxmove = (dwDefaultDataSize >= *pcbData) ? *pcbData : dwDefaultDataSize;
		src = (CHAR*)pvDefaultData;
		dst = (CHAR*)pvData;
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
	TRACE("key '%s', value '%s', datalen %ld,  %s\n",
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
	TRACE("key '%s', value '%s', datalen %ld,  %s\n",
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
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: An error code from SHRegOpenUSKeyA() or SHRegWriteUSValueA(), or
 *           ERROR_INVALID_FUNCTION if pvData is NULL.
 *
 * NOTES
 *   This function opens pszSubKey, sets the value, and then closes the key.
 */
LONG WINAPI SHRegSetUSValueA(
	LPCSTR pszSubKey, /* [I] Name of key to set the value in */
	LPCSTR pszValue, /* [I] Name of value under pszSubKey to set the value in */
	DWORD  dwType, /* [I] Type of the value */
	LPVOID pvData, /* [I] Data to set as the value */
	DWORD  cbData, /* [I] length of pvData */
	DWORD  dwFlags) /* [I] SHREGSET_ flags from "shlwapi.h" */
{
        HUSKEY myhuskey;
	LONG   ret;
	BOOL   ignoreHKCU;

        if (!pvData) return ERROR_INVALID_FUNCTION;
	TRACE("key '%s', value '%s', datalen %ld\n",
	      debugstr_a(pszSubKey), debugstr_a(pszValue), cbData);

	ignoreHKCU = ((dwFlags == SHREGSET_HKLM) || (dwFlags == SHREGSET_FORCE_HKLM));

	ret = SHRegOpenUSKeyA(pszSubKey, 0x1, 0, &myhuskey, ignoreHKCU);
	if (ret == ERROR_SUCCESS) {
 	  ret = SHRegWriteUSValueA(myhuskey, pszValue, dwType, pvData,
				   cbData, dwFlags);
	    SHRegCloseUSKey(myhuskey);
	}
	return ret;
}

/*************************************************************************
 * SHRegSetUSValueW   [SHLWAPI.@]
 *
 * See SHRegSetUSValueA.
 */
LONG WINAPI SHRegSetUSValueW(
	LPCWSTR pszSubKey,
	LPCWSTR pszValue,
	DWORD   dwType,
	LPVOID  pvData,
	DWORD   cbData,
	DWORD   dwFlags)
{
        HUSKEY myhuskey;
	LONG   ret;
	BOOL   ignoreHKCU;

        if (!pvData) return ERROR_INVALID_FUNCTION;
	TRACE("key '%s', value '%s', datalen %ld\n",
	      debugstr_w(pszSubKey), debugstr_w(pszValue), cbData);

	ignoreHKCU = ((dwFlags == SHREGSET_HKLM) || (dwFlags == SHREGSET_FORCE_HKLM));

	ret = SHRegOpenUSKeyW(pszSubKey, 0x1, 0, &myhuskey, ignoreHKCU);
	if (ret == ERROR_SUCCESS) {
 	  ret = SHRegWriteUSValueW(myhuskey, pszValue, dwType, pvData,
				   cbData, dwFlags);
	    SHRegCloseUSKey(myhuskey);
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
	LONG retvalue;
	DWORD type, datalen, work;
	BOOL ret = fDefault;
	CHAR data[10];

	TRACE("key '%s', value '%s', %s\n",
	      debugstr_a(pszSubKey), debugstr_a(pszValue),
	      (fIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	datalen = sizeof(data)-1;
	if (!(retvalue = SHRegGetUSValueA( pszSubKey, pszValue, &type,
					   data, &datalen,
					   fIgnoreHKCU, 0, 0))) {
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
		FIXME("Unsupported registry data type %ld\n", type);
		ret = FALSE;
	    }
	    TRACE("got value (type=%ld), returing <%s>\n", type,
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
	LONG retvalue;
	DWORD type, datalen, work;
	BOOL ret = fDefault;
	WCHAR data[10];

	TRACE("key '%s', value '%s', %s\n",
	      debugstr_w(pszSubKey), debugstr_w(pszValue),
	      (fIgnoreHKCU) ? "Ignoring HKCU" : "Tries HKCU then HKLM");

	datalen = (sizeof(data)-1) * sizeof(WCHAR);
	if (!(retvalue = SHRegGetUSValueW( pszSubKey, pszValue, &type,
					   data, &datalen,
					   fIgnoreHKCU, 0, 0))) {
	    /* process returned data via type into bool */
	    switch (type) {
	    case REG_SZ:
		data[9] = L'\0';     /* set end of string */
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
		    ret = (data[0] != L'\0');
		    break;
		}
	    default:
		FIXME("Unsupported registry data type %ld\n", type);
		ret = FALSE;
	    }
	    TRACE("got value (type=%ld), returing <%s>\n", type,
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

	TRACE("(%p,%ld,%p,%p(%ld),%d)\n",
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

	TRACE("(%p,%ld,%p,%p(%ld),%d)\n",
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
 *  Failure: An error code from RegSetValueExA().
 */
LONG  WINAPI SHRegWriteUSValueA(HUSKEY hUSKey, LPCSTR pszValue, DWORD dwType,
				LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
    HKEY dokey;

    TRACE("(%p,%s,%ld,%p,%ld,%ld)\n",
	      hUSKey, debugstr_a(pszValue), dwType, pvData, cbData, dwFlags);

    if ((dwFlags & SHREGSET_FORCE_HKCU) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	RegSetValueExA(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if ((dwFlags & SHREGSET_FORCE_HKLM) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	RegSetValueExA(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if (dwFlags & (SHREGSET_FORCE_HKCU | SHREGSET_FORCE_HKLM))
	return ERROR_SUCCESS;

    FIXME("SHREGSET_HKCU or SHREGSET_HKLM not supported\n");
    return ERROR_SUCCESS;
}

/*************************************************************************
 *      SHRegWriteUSValueW   	[SHLWAPI.@]
 *
 * See SHRegWriteUSValueA.
 */
LONG  WINAPI SHRegWriteUSValueW(HUSKEY hUSKey, LPCWSTR pszValue, DWORD dwType,
				LPVOID pvData, DWORD cbData, DWORD dwFlags)
{
    HKEY dokey;

    TRACE("(%p,%s,%ld,%p,%ld,%ld)\n",
	      hUSKey, debugstr_w(pszValue), dwType, pvData, cbData, dwFlags);

    if ((dwFlags & SHREGSET_FORCE_HKCU) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKCU))) {
	RegSetValueExW(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if ((dwFlags & SHREGSET_FORCE_HKLM) &&
	    (dokey = REG_GetHKEYFromHUSKEY(hUSKey,REG_HKLM))) {
	RegSetValueExW(dokey, pszValue, 0, dwType, pvData, cbData);
    }

    if (dwFlags & (SHREGSET_FORCE_HKCU | SHREGSET_FORCE_HKLM))
	return ERROR_SUCCESS;

    FIXME("SHREGSET_HKCU or SHREGSET_HKLM not supported\n");
    return ERROR_SUCCESS;
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

  TRACE("(hkey=%p,%s,%s,%p,%ld)\n", hKey, debugstr_a(lpszSubKey),
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

  TRACE("(hkey=%p,%s,%s,%p,%ld)\n", hKey, debugstr_w(lpszSubKey),
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

  FIXME("(hkey=%p,%s,%s,%p,%ld) - semi-stub\n",hKey, debugstr_a(lpszSubKey),
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

  FIXME("(hkey=%p,%s,%s,%p,%ld) - semi-stub\n",hKey, debugstr_w(lpszSubKey),
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
  char  szEmpty[] = "";

  TRACE("(hkey=%p,%s,%s,%ld,%p,%ld)\n", hKey, debugstr_a(lpszSubKey),
          debugstr_a(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExA(hKey, lpszSubKey, 0, szEmpty,
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
  WCHAR szEmpty[] = { '\0' };

  TRACE("(hkey=%p,%s,%s,%ld,%p,%ld)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExW(hKey, lpszSubKey, 0, szEmpty,
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
 *       subcase-1: the buffer is to small to hold the unexpanded string:
 *          function fails and returns the size of the unexpanded string.
 *
 *       subcase-2: buffer is to small to hold the expanded string:
 *          the function return success (!!) and the result is truncated
 *	    *** This is clearly a error in the native implementation. ***
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

  TRACE("(hkey=%p,%s,%p,%p,%p,%p=%ld)\n", hKey, debugstr_a(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwUnExpDataLen = *pcbData;

  dwRet = RegQueryValueExA(hKey, lpszValue, lpReserved, &dwType, pvData, &dwUnExpDataLen);

  if (pcbData && (dwType == REG_EXPAND_SZ))
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPSTR szData;

    /* If the caller didn't supply a buffer or the buffer is to small we have
     * to allocate our own
     */
    if ((!pvData) || (dwRet == ERROR_MORE_DATA) )
    {
      char cNull = '\0';
      nBytesToAlloc = (!pvData || (dwRet == ERROR_MORE_DATA)) ? dwUnExpDataLen : *pcbData;

      szData = (LPSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExA (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
    }
    else
    {
      nBytesToAlloc = lstrlenA(pvData) * sizeof (CHAR);
      szData = (LPSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc + 1);
      lstrcpyA(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, pvData, *pcbData / sizeof(CHAR));
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
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

  TRACE("(hkey=%p,%s,%p,%p,%p,%p=%ld)\n", hKey, debugstr_w(lpszValue),
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
      nBytesToAlloc = (!pvData || (dwRet == ERROR_MORE_DATA)) ? dwUnExpDataLen : *pcbData;

      szData = (LPWSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExW (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
    }
    else
    {
      nBytesToAlloc = lstrlenW(pvData) * sizeof(WCHAR);
      szData = (LPWSTR) LocalAlloc(GMEM_ZEROINIT, nBytesToAlloc + 1);
      lstrcpyW(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, pvData, *pcbData/sizeof(WCHAR) );
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree((HLOCAL) szData);
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
  DWORD dwRet, dwKeyCount = 0, dwMaxSubkeyLen = 0, dwSize, i;
  CHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_a(lpszSubKey));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    /* Find how many subkeys there are */
    dwRet = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(!dwRet)
    {
      dwMaxSubkeyLen++;
      if (dwMaxSubkeyLen > sizeof(szNameBuf))
        /* Name too big: alloc a buffer for it */
        lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxSubkeyLen*sizeof(CHAR));

      if(!lpszName)
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
      else
      {
        /* Recursively delete all the subkeys */
        for(i = 0; i < dwKeyCount && !dwRet; i++)
        {
          dwSize = dwMaxSubkeyLen;
          dwRet = RegEnumKeyExA(hSubKey, i, lpszName, &dwSize, NULL, NULL, NULL, NULL);
          if(!dwRet)
            dwRet = SHDeleteKeyA(hSubKey, lpszName);
        }
        if (lpszName != szNameBuf)
          HeapFree(GetProcessHeap(), 0, lpszName); /* Free buffer if allocated */
      }
    }

    RegCloseKey(hSubKey);
    if(!dwRet)
      dwRet = RegDeleteKeyA(hKey, lpszSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteKeyW   [SHLWAPI.@]
 *
 * See SHDeleteKeyA.
 */
DWORD WINAPI SHDeleteKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0, dwMaxSubkeyLen = 0, dwSize, i;
  WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    /* Find how many subkeys there are */
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
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
        /* Recursively delete all the subkeys */
        for(i = 0; i < dwKeyCount && !dwRet; i++)
        {
          dwSize = dwMaxSubkeyLen;
          dwRet = RegEnumKeyExW(hSubKey, i, lpszName, &dwSize, NULL, NULL, NULL, NULL);
          if(!dwRet)
            dwRet = SHDeleteKeyW(hSubKey, lpszName);
        }

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
  TRACE("(hkey=%p,%ld,%s,%p)\n", hKey, dwIndex, debugstr_a(lpszSubKey), pwLen);

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
  TRACE("(hkey=%p,%ld,%s,%p)\n", hKey, dwIndex, debugstr_w(lpszSubKey), pwLen);

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
  TRACE("(hkey=%p,%ld,%s,%p,%p,%p,%p)\n", hKey, dwIndex,
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
  TRACE("(hkey=%p,%ld,%s,%p,%p,%p,%p)\n", hKey, dwIndex,
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
  TRACE("(%s,%p,%ld)\n", debugstr_a(lpszType), lpszBuffer, dwLen);

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
  TRACE("(%s,%p,%ld)\n", debugstr_w(lpszType), lpszBuffer, dwLen);

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
 *  hKeyDst    [I] Destination key
 *  lpszSubKey [I] Sub key under hKeyDst, or NULL to use hKeyDst directly
 *  hKeySrc    [I] Source key to copy from
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
DWORD WINAPI SHCopyKeyA(HKEY hKeyDst, LPCSTR lpszSubKey, HKEY hKeySrc, DWORD dwReserved)
{
  WCHAR szSubKeyW[MAX_PATH];

  TRACE("(hkey=%p,%s,%p08x,%ld)\n", hKeyDst, debugstr_a(lpszSubKey), hKeySrc, dwReserved);

  if (lpszSubKey)
    MultiByteToWideChar(0, 0, lpszSubKey, -1, szSubKeyW, MAX_PATH);

  return SHCopyKeyW(hKeyDst, lpszSubKey ? szSubKeyW : NULL, hKeySrc, dwReserved);
}

/*************************************************************************
 * SHCopyKeyW	[SHLWAPI.@]
 *
 * See SHCopyKeyA.
 */
DWORD WINAPI SHCopyKeyW(HKEY hKeyDst, LPCWSTR lpszSubKey, HKEY hKeySrc, DWORD dwReserved)
{
  DWORD dwKeyCount = 0, dwValueCount = 0, dwMaxKeyLen = 0;
  DWORD  dwMaxValueLen = 0, dwMaxDataLen = 0, i;
  BYTE buff[1024];
  LPVOID lpBuff = (LPVOID)buff;
  WCHAR szName[MAX_PATH], *lpszName = szName;
  DWORD dwRet = S_OK;

  TRACE("hkey=%p,%s,%p08x,%ld)\n", hKeyDst, debugstr_w(lpszSubKey), hKeySrc, dwReserved);

  if(!hKeyDst || !hKeySrc)
    dwRet = ERROR_INVALID_PARAMETER;
  else
  {
    /* Open destination key */
    if(lpszSubKey)
      dwRet = RegOpenKeyExW(hKeyDst, lpszSubKey, 0, KEY_ALL_ACCESS, &hKeyDst);

    if(dwRet)
      hKeyDst = 0; /* Don't close this key since we didn't open it */
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
          dwRet = SHCopyKeyW(hSubKeyDst, NULL, hSubKeySrc, 0);
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

    dwRet = RegEnumValueW(hKeySrc, i, lpszName, &dwNameSize, NULL, &dwType, buff, &dwLen);

    if (!dwRet)
      dwRet = SHSetValueW(hKeyDst, NULL, lpszName, dwType, lpBuff, dwLen);
  }

  /* Free buffers if allocated */
  if (lpszName != szName)
    HeapFree(GetProcessHeap(), 0, lpszName);
  if (lpBuff != buff)
    HeapFree(GetProcessHeap(), 0, lpBuff);

  if (lpszSubKey && hKeyDst)
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
  TRACE("(%p,%s,%d)", hKey, debugstr_w(lpszValue), iDefault);

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
