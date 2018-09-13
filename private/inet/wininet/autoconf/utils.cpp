#include "utils.h"

//----------------------------------------------------------------------------
// HELPER FUNCTIONS
//----------------------------------------------------------------------------


// Function to get the coclass ClassId of a script engine given its name

HRESULT GetScriptEngineClassIDFromName(
	LPCSTR pszLanguage,
	LPSTR pszBuff,
	UINT cBuffSize)
{
	HKEY hKey = NULL;
	HKEY hKeySub;
	LONG result;
	HRESULT hr;
	LONG cClassIdLen;

	// Open \HKEY_CLASSES_ROOT\[pszLanguage]

	// LONG RegOpenKeyEx(
    //	HKEY	hKey,		// handle of open key
    //	LPCTSTR	lpSubKey,	// address of name of subkey to open
    //	DWORD	ulOptions,	// reserved
    //	REGSAM	samDesired,	// security access mask
    //	PHKEY	phkResult 	// address of handle of open key
	// );	

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT, pszLanguage, 0, KEY_READ, &hKey);

	if (result != ERROR_SUCCESS) {
		hr = E_FAIL;
		goto exit;
	}

	// Make sure this object supports OLE Scripting

	result = RegOpenKeyEx(hKey, "OLEScript", 0, KEY_READ, &hKeySub);

	if (result != ERROR_SUCCESS) {
		hr = E_FAIL;
		goto exit;
	}

	RegCloseKey(hKeySub);

	// Get the class ID

	// LONG RegQueryValueEx(
    //	HKEY	hKey,			// handle of key to query
    //	LPTSTR	lpValueName,	// address of name of value to query
    //	LPDWORD	lpReserved,		// reserved
    //	LPDWORD	lpType,			// address of buffer for value type
    //	LPBYTE	lpData,			// address of data buffer
    //	LPDWORD	lpcbData	 	// address of data buffer size
    // );

	result = RegOpenKeyEx(hKey, "CLSID", 0, KEY_READ, &hKeySub);

	if (result != ERROR_SUCCESS) {
		hr = E_FAIL;
		goto exit;
	}

	cClassIdLen = cBuffSize;
	result = RegQueryValue(hKeySub, NULL, pszBuff, &cClassIdLen);

	RegCloseKey(hKeySub);

	if (result != ERROR_SUCCESS) {
		hr = E_FAIL;
		goto exit;
	}

	pszBuff[cBuffSize-1] = '\0';

	hr = S_OK;

exit:
	if (hKey) {
		RegCloseKey(hKey);
	}

	return hr;
}


//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi
(
    LPCSTR psz,
    BYTE  bType
)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate the widestr, +1 for terminating null
    //
    switch (bType) {
      case STR_BSTR:
        // -1 since it'll add it's own space for a NULL terminator
        //
        pwsz = (LPWSTR) SysAllocStringLen(NULL, i - 1);
        break;
      case STR_OLESTR:
        pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
        break;
      default:
        return NULL;
                ;
    }

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

int ConvertAnsiDayToInt(LPSTR szday)
{
	int today = -1;
	if (szday)  // GetDateFormat always returns mixed caps and since this comes from a Win32 API I will
	{			// assume a properly formatted string! :)
		switch (szday[0])
		{
		case 'S' :
			if (lstrcmp(szday,"SUN") == 0)
				today = 0;
			else
			{
				if (lstrcmp(szday,"SAT") == 0)	
					today = 6;
			}
			break;

		case 'M' :
			if (lstrcmp(szday,"MON") == 0)
				today = 1;
			break;

		case 'T' :
			if (lstrcmp(szday,"TUE") == 0)
				today = 2;
			else
			{
				if (lstrcmp(szday,"THU") == 0)	
				today = 4;
			}
			break;

		case 'W' :
			if (lstrcmp(szday,"WED") == 0)
				today = 3;
			break;

		case 'F' :
			if (lstrcmp(szday,"FRI") == 0)
				today = 5;
			break;
		
		}
	}
	return today;
}

