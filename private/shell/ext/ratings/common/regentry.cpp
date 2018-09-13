/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**			  Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/

/*
	regentry.cxx
	registry access

	This file contains those routines which enable net providers to
    conveniently access the registry for their entries.

	FILE HISTORY:
		lens	03/15/94	Created
*/

#include "npcommon.h"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <npassert.h>

#include <npstring.h>
#include <regentry.h>

RegEntry::RegEntry(const char *pszSubKey, HKEY hkey)
{
	_error = RegCreateKey(hkey, pszSubKey, &_hkey);
	if (_error) {
		bhkeyValid = FALSE;
	}
	else {
		bhkeyValid = TRUE;
	}
}

RegEntry::~RegEntry()
{ 
    if (bhkeyValid) {
        RegCloseKey(_hkey); 
    }
}

long RegEntry::SetValue(const char *pszValue, const char *string)
{
    if (bhkeyValid) {
    	_error = RegSetValueEx(_hkey, pszValue, 0, REG_SZ,
    				(unsigned char *)string, lstrlen(string)+1);
    }
	return _error;
}

long RegEntry::SetValue(const char *pszValue, unsigned long dwNumber)
{
    if (bhkeyValid) {
    	_error = RegSetValueEx(_hkey, pszValue, 0, REG_BINARY,
    				(unsigned char *)&dwNumber, sizeof(dwNumber));
    }
	return _error;
}

long RegEntry::DeleteValue(const char *pszValue)
{
    if (bhkeyValid) {
    	_error = RegDeleteValue(_hkey, (LPTSTR) pszValue);
	}
	return _error;
}


char *RegEntry::GetString(const char *pszValue, char *string, unsigned long length)
{
	DWORD 	dwType = REG_SZ;
	
    if (bhkeyValid) {
    	_error = RegQueryValueEx(_hkey, (LPTSTR) pszValue, 0, &dwType, (LPBYTE)string,
    				&length);
    }
	if (_error)
		*string = '\0';

	return string;
}

long RegEntry::GetNumber(const char *pszValue, long dwDefault)
{
 	DWORD 	dwType = REG_BINARY;
 	long	dwNumber = 0L;
 	DWORD	dwSize = sizeof(dwNumber);

    if (bhkeyValid) {
    	_error = RegQueryValueEx(_hkey, (LPTSTR) pszValue, 0, &dwType, (LPBYTE)&dwNumber,
    				&dwSize);
	}
	if (_error)
		dwNumber = dwDefault;
	
	return dwNumber;
}

VOID RegEntry::GetValue(const char *pszValueName, NLS_STR *pnlsString)
{
	DWORD 	dwType = REG_SZ;
    DWORD   length;
    CHAR *  string;
    BOOL    bReallocDoneOK = FALSE;

    if (bhkeyValid) {
        _error = RegQueryValueEx( _hkey,
                                  (LPTSTR) pszValueName,
                                  0,
                                  &dwType,
                                  NULL,
                                  &length );
    	if (_error == ERROR_SUCCESS) {
            if (!pnlsString->IsOwnerAlloc()) {
                bReallocDoneOK = pnlsString->realloc(length);
            }
            else if (length <= (UINT)pnlsString->QueryAllocSize()) {
                bReallocDoneOK = TRUE;
            }
            else {
                _error = ERROR_MORE_DATA;
            }
        }
    	string = pnlsString->Party();
        if (bReallocDoneOK) {
        	_error = RegQueryValueEx( _hkey,
                                      (LPTSTR) pszValueName,
                                      0,
                                      &dwType,
                                      (LPBYTE) string,
                                      &length );
            if (_error == ERROR_SUCCESS) {
                if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ)) {
                    _error = ERROR_INVALID_PARAMETER;
                }
            }
        }
        else {
            _error = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    if (_error != ERROR_SUCCESS) {
        if (string != NULL) {
    		*string = '\0';
        }
    }
    pnlsString->DonePartying();
}

VOID RegEntry::MoveToSubKey(const char *pszSubKeyName)
{
    HKEY	_hNewKey;

    if (bhkeyValid) {
        _error = RegOpenKey ( _hkey,
                              pszSubKeyName,
                              &_hNewKey );
        if (_error == ERROR_SUCCESS) {
            RegCloseKey(_hkey);
            _hkey = _hNewKey;
        }
    }
}

long RegEntry::FlushKey()
{
    if (bhkeyValid) {
    	_error = RegFlushKey(_hkey);
    }
	return _error;
}

RegEnumValues::RegEnumValues(RegEntry *pReqRegEntry)
 : pRegEntry(pReqRegEntry),
   iEnum(0),
   pchName(NULL),
   pbValue(NULL)
{
    _error = pRegEntry->GetError();
    if (_error == ERROR_SUCCESS) {
        _error = RegQueryInfoKey ( pRegEntry->GetKey(), // Key
                                   NULL,                // Buffer for class string
                                   NULL,                // Size of class string buffer
                                   NULL,                // Reserved
                                   NULL,                // Number of subkeys
                                   NULL,                // Longest subkey name
                                   NULL,                // Longest class string
                                   &cEntries,           // Number of value entries
                                   &cMaxValueName,      // Longest value name
                                   &cMaxData,           // Longest value data
                                   NULL,                // Security descriptor
                                   NULL );              // Last write time
    }
    if (_error == ERROR_SUCCESS) {
        if (cEntries != 0) {
            cMaxValueName = cMaxValueName + 1; // REG_SZ needs one more for null
            cMaxData = cMaxData + 1;           // REG_SZ needs one more for null
            pchName = new CHAR[cMaxValueName];
            if (!pchName) {
                _error = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                if (cMaxData) {
                    pbValue = new BYTE[cMaxData];
                    if (!pbValue) {
                        _error = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }
        }
    }
}

RegEnumValues::~RegEnumValues()
{
    delete pchName;
    delete pbValue;
}

long RegEnumValues::Next()
{
    if (_error != ERROR_SUCCESS) {
        return _error;
    }
    if (cEntries == iEnum) {
        return ERROR_NO_MORE_ITEMS;
    }

    DWORD   cchName = cMaxValueName;

    dwDataLength = cMaxData;
    _error = RegEnumValue ( pRegEntry->GetKey(), // Key
                            iEnum,               // Index of value
                            pchName,             // Address of buffer for value name
                            &cchName,            // Address for size of buffer
                            NULL,                // Reserved
                            &dwType,             // Data type
                            pbValue,             // Address of buffer for value data
                            &dwDataLength );     // Address for size of data
    iEnum++;
    return _error;
}

NPMachineEntries::NPMachineEntries(const char *pszReqSectionName)
: RegEntry("System\\CurrentControlSet\\Services", HKEY_LOCAL_MACHINE),
  pszSectionName(pszReqSectionName)
{
    if (GetError() == ERROR_SUCCESS) {
        MoveToSubKey(pszSectionName);
        if (GetError() == ERROR_SUCCESS) {
            MoveToSubKey("NetworkProvider");
        }
    }
}
