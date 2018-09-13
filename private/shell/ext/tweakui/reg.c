/*
 * reg - registry wrappers
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  RegCanModifyKey
 *
 *	Returns nonzero if the current user has permission to modify the
 *	key.
 *
 *****************************************************************************/

BOOL PASCAL
RegCanModifyKey(HKEY hkRoot, LPCTSTR ptszSubkey)
{
    BOOL fRc;
    if (g_fNT) {
	HKEY hk;
	DWORD dw;

	if (RegCreateKeyEx(hkRoot, ptszSubkey, 0, c_tszNil,
			   REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hk,
			   &dw) == 0) {
	    RegCloseKey(hk);
	    fRc = 1;
	} else {
	    fRc = 0;
	}
    } else {
	fRc = 1;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  _RegOpenKey
 *
 *	Special version for NT that always asks for MAXIMUM_ALLOWED.
 *
 *****************************************************************************/

LONG PASCAL
_RegOpenKey(HKEY hk, LPCTSTR ptszSubKey, PHKEY phkResult)
{
    return RegOpenKeyEx(hk, ptszSubKey, 0, MAXIMUM_ALLOWED, phkResult);
}

/*****************************************************************************
 *
 *  _RegCreateKey
 *
 *	Special version for NT that always asks for MAXIMUM_ALLOWED.
 *
 *****************************************************************************/

LONG PASCAL
_RegCreateKey(HKEY hk, LPCTSTR ptszSubKey, PHKEY phkResult)
{
    DWORD dw;
    if (ptszSubKey) {
	return RegCreateKeyEx(hk, ptszSubKey, 0, c_tszNil,
			      REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, 0,
			      phkResult, &dw);
    } else {
	return RegOpenKey(hk, ptszSubKey, phkResult);
    }
}

/*****************************************************************************
 *
 *  RegDeleteValues
 *
 *  Deletes all the values under a key.
 *
 *****************************************************************************/

void PASCAL
RegDeleteValues(HKEY hkRoot, LPCTSTR ptszSubkey)
{
    HKEY hk;
    if (_RegOpenKey(hkRoot, ptszSubkey, &hk) == 0) {
	DWORD dw, ctch;
	TCHAR tszValue[ctchKeyMax];
	dw = 0;
	while (ctch = cA(tszValue),
	       RegEnumValue(hk, dw, tszValue, &ctch, 0, 0, 0, 0) == 0) {
	    if (RegDeleteValue(hk, tszValue) == 0) {
	    } else {
		dw++;
	    }
	}
	RegCloseKey(hk);
    }
}

/*****************************************************************************
 *
 *  RegDeleteTree
 *
 *  Deletes an entire registry tree.
 *
 *  Windows 95's RegDeleteKey will delete an entire tree, but Windows NT
 *  forces you to do it yourself.
 *
 *  Note that you need to watch out for the case where a key is undeletable,
 *  in which case you must skip over the key and continue as best you can.
 *
 *****************************************************************************/

LONG PASCAL
RegDeleteTree(HKEY hkRoot, LPCTSTR ptszSubkey)
{
    HKEY hk;
    LONG lRc;
    lRc = RegOpenKey(hkRoot, ptszSubkey, &hk);
    if (lRc == 0) {
	DWORD dw;
	TCHAR tszKey[ctchKeyMax];
	dw = 0;
	while (RegEnumKey(hk, dw, tszKey, cA(tszKey)) == 0) {
	    if (RegDeleteTree(hk, tszKey) == 0) {
	    } else {
		dw++;
	    }
	}
	RegCloseKey(hk);
	lRc = RegDeleteKey(hkRoot, ptszSubkey);
	if (lRc == 0) {
	} else {	/* Couldn't delete the key; at least nuke the values */
	    RegDeleteValues(hkRoot, ptszSubkey);
	}
    }
    return lRc;
}

/*****************************************************************************
 *
 *  hkOpenClsid
 *
 *	Open a class id (guid) registry key, returning the hkey.
 *
 *****************************************************************************/

HKEY PASCAL
hkOpenClsid(PCTSTR ptszClsid)
{
    HKEY hk = 0;
    _RegOpenKey(pcdii->hkClsid, ptszClsid, &hk);
    return hk;
}

/*****************************************************************************
 *
 *  GetRegStr
 *
 *	Generic wrapper that sucks out a registry key/subkey.
 *
 *****************************************************************************/

BOOL PASCAL
GetRegStr(HKEY hkRoot, LPCTSTR ptszKey, LPCTSTR ptszSubkey,
	  LPTSTR ptszBuf, int cbBuf)
{
    HKEY hk;
    BOOL fRc;
    if ((UINT)cbBuf >= cbCtch(1)) {
	ptszBuf[0] = TEXT('\0');
    }
    if (hkRoot && _RegOpenKey(hkRoot, ptszKey, &hk) == 0) {
	fRc = RegQueryValueEx(hk, ptszSubkey, 0, 0, ptszBuf, &cbBuf) == 0;
	RegCloseKey(hk);
    } else {
	fRc = 0;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  GetStrPkl
 *
 *	Read a registry key/subkey/string value given a key location.
 *
 *****************************************************************************/

BOOL PASCAL
GetStrPkl(LPTSTR ptszBuf, int cbBuf, PKL pkl)
{
    return GetRegStr(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey,
		     ptszBuf, cbBuf);
}

/*****************************************************************************
 *
 *  GetRegDword
 *
 *	Read a dword, returning the default if unable.
 *
 *****************************************************************************/

DWORD PASCAL
GetRegDword(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dwDefault)
{
    DWORD dw;
    if (GetRegStr(hkeyHhk(hhk), pszKey, pszSubkey, (LPBYTE)&dw, sizeof(dw))) {
	return dw;
    } else {
	return dwDefault;
    }
}

/*****************************************************************************
 *
 *  GetDwordPkl
 *
 *	Given a location, read a dword, returning the default if unable.
 *
 *****************************************************************************/

DWORD PASCAL
GetDwordPkl(PKL pkl, DWORD dwDefault)
{
    DWORD dw;
    if (GetRegStr(*pkl->phkRoot, pkl->ptszKey,
		  pkl->ptszSubkey, (LPBYTE)&dw, sizeof(dw))) {
	return dw;
    } else {
	return dwDefault;
    }
}

/*****************************************************************************
 *
 *  GetRegInt
 *
 *	Generic wrapper that sucks out a registry key/subkey as an unsigned.
 *
 *****************************************************************************/

UINT PASCAL
GetRegInt(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, UINT uiDefault)
{
    TCH tsz[20];
    if (GetRegStr(hkeyHhk(hhk), pszKey, pszSubkey, tsz, cA(tsz))) {
	int i = iFromPtsz(tsz);
	return i == iErr ? uiDefault : (UINT)i;
    } else {
	return uiDefault;
    }
}

/*****************************************************************************
 *
 *  GetIntPkl
 *
 *	Generic wrapper that sucks out a registry key/subkey as an unsigned.
 *
 *****************************************************************************/

UINT PASCAL
GetIntPkl(UINT uiDefault, PKL pkl)
{
    return GetRegInt(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, uiDefault);
}

/*****************************************************************************
 *
 *  RegSetValuePtsz
 *
 *	Generic wrapper that writes out a registry key/subkey as a string.
 *
 *****************************************************************************/

void PASCAL
RegSetValuePtsz(HKEY hk, LPCSTR pszSubkey, LPCTSTR ptszVal)
{
    RegSetValueEx(hk, pszSubkey, 0, REG_SZ, (LPBYTE)ptszVal,
		  1 + lstrlen(ptszVal));
}

/*****************************************************************************
 *
 *  SetRegStr
 *
 *	Generic wrapper that writes out a registry key/subkey.
 *
 *	It is an error to call this with a bad hhk.
 *
 *****************************************************************************/

void PASCAL
SetRegStr(HHK hhk, LPCTSTR ptszKey, LPCTSTR ptszSubkey, LPCTSTR ptszVal)
{
    HKEY hk;
    if (RegCreateKey(hkeyHhk(hhk), ptszKey, &hk) == 0) {
	RegSetValuePtsz(hk, ptszSubkey, ptszVal);
	RegCloseKey(hk);
    }
}

/*****************************************************************************
 *
 *  SetStrPkl
 *
 *	Set a registry key/subkey/string value given a key location.
 *
 *	It is an error to call this with a bad hkRoot.
 *
 *****************************************************************************/

void PASCAL
SetStrPkl(PKL pkl, LPCTSTR ptszVal)
{
    SetRegStr(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, ptszVal);
}


/*****************************************************************************
 *
 *  SetRegInt
 *
 *	Generic wrapper that writes out a registry key/subkey as an
 *	unsigned integer.
 *
 *****************************************************************************/

void PASCAL
SetRegInt(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, UINT ui)
{
    TCH tsz[20];
    wsprintf(tsz, c_tszPercentU, ui);
    SetRegStr(hhk, pszKey, pszSubkey, tsz);
}

/*****************************************************************************
 *
 *  SetIntPkl
 *
 *	Writes out a registry key/subkey as an unsigned integer.
 *
 *****************************************************************************/

void PASCAL
SetIntPkl(UINT ui, PKL pkl)
{
    SetRegInt(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, ui);
}

/*****************************************************************************
 *
 *  SetRegDword
 *
 *	Generic wrapper that writes out a registry key/subkey as a
 *	dword.
 *
 *****************************************************************************/

void PASCAL
SetRegDword(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dw)
{
    HKEY hk;
    if (RegCreateKey(hkeyHhk(hhk), pszKey, &hk) == 0) {
	/* Bad prototype for RegSetValueEx forces me to cast */
	RegSetValueEx(hk, pszSubkey, 0, REG_BINARY, (LPBYTE)&dw, sizeof(dw));
	RegCloseKey(hk);
    }
}

/*****************************************************************************
 *
 *  SetDwordPkl
 *
 *	Generic wrapper that writes out a registry key/subkey as a
 *	dword, given a key location.
 *
 *****************************************************************************/

void PASCAL
SetDwordPkl(PKL pkl, DWORD dw)
{
    SetRegDword(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, dw);
}


/*****************************************************************************
 *
 *  DelPkl
 *
 *	Generic wrapper that deletes a registry key/subkey.
 *
 *****************************************************************************/

void PASCAL
DelPkl(PKL pkl)
{
    HKEY hk;
    if (_RegOpenKey(*pkl->phkRoot, pkl->ptszKey, &hk) == 0) {
	RegDeleteValue(hk, pkl->ptszSubkey);
	RegCloseKey(hk);
    }
}

