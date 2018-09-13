//------------------------------------------------------------------------------------
//
//	File: REGUTILS.CPP
//
//	Helper functions that handle reading and writing strings to the system registry.
//
//------------------------------------------------------------------------------------

#include <windows.h>
#include <shlobj.h>                 // for SHChangeNotify() and flag
#include <stdlib.h>

#include "regutils.h"


#define MAX_VALUELEN    16



//------------------------------------------------------------------------------------
//
//      IconSet/GetRegValueString()
//
//      Versions of Get/SetRegValueString that go to the user classes section first
//      and then the HKEY_CLASSES_ROOT section if the first attempt fails
//
//      Returns: success of string setting / retrieval
//
//------------------------------------------------------------------------------------
BOOL IconSetRegValueString(LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue ) {
    TCHAR szRegPath[MAX_PATH];

    wsprintf( szRegPath, TEXT("Software\\Classes\\%s"), lpszSubKey );

    return SetRegValueString(HKEY_CURRENT_USER, szRegPath, lpszValName, lpszValue );
}

BOOL IconGetRegValueString(LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue, int iMaxSize ) {
    TCHAR szRegPath[MAX_PATH];

    wsprintf( szRegPath, TEXT("Software\\Classes\\%s"), lpszSubKey );

    if (!GetRegValueString(HKEY_CURRENT_USER, szRegPath, lpszValName, lpszValue, iMaxSize ))
        return GetRegValueString(HKEY_CLASSES_ROOT, lpszSubKey, lpszValName, lpszValue, iMaxSize );

    return TRUE;
}


//------------------------------------------------------------------------------------
//
//	GetRegValueString()
//
//	Just a little helper routine, gets an individual string value from the 
//	registry and returns it to the caller. Takes care of registry headaches,
//	including a paranoid length check before getting the string.
//
//	Returns: success of string retrieval
//
//------------------------------------------------------------------------------------
BOOL GetRegValueString( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue, int iMaxSize )
{
LONG lRet;
HKEY hKey;                   // cur open key
BOOL bOK = TRUE;
DWORD dwSize, dwType;

	// get subkey
	lRet = RegOpenKeyEx( hMainKey, lpszSubKey, (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );
	if( lRet != ERROR_SUCCESS )
	{
//		Assert(FALSE, "problem on RegOpenKeyEx in GetRegValue\n");
		return FALSE;
	}

	// now do our paranoid check of data size
	lRet = RegQueryValueEx( hKey, lpszValName,(LPDWORD)NULL,(LPDWORD)&dwType, (LPBYTE)NULL,/* null for size info only */ (LPDWORD)&dwSize );

	if( ERROR_SUCCESS == lRet )
	{     // saw something there
		// here's the size check before getting the data
		if( dwSize > (DWORD)iMaxSize )
		{ // if string too big
//			Assert(FALSE, "Humongous registry string; can't GetRegValue...\n");
			bOK = FALSE;               // can't read, so very bad news
		}
		else
		{                        // size is OK to continue
			// now really get the value
			lRet = RegQueryValueEx( hKey, lpszValName, (LPDWORD)NULL,(LPDWORD)&dwType, (LPBYTE)lpszValue, /* getting actual value */ (LPDWORD)&dwSize );
//			Assert(lret == ERROR_SUCCESS, "bad return GetRegValue query\n");
//			Assert(dwType == (DWORD)REG_SZ, "non-string type in GetValue!\n");

			if( ERROR_SUCCESS != lRet )
				bOK = FALSE;
		}
	}
	else
	{
		bOK = FALSE;
	}

	// close subkey
	RegCloseKey( hKey );

	return (bOK);
}

//------------------------------------------------------------------------------------
//
//	GetRegValueInt()
//
//	Just a little helper routine, gets an individual string value from the 
//	registry and returns it to the caller as an int. Takes care of registry headaches,
//	including a paranoid length check before getting the string.
//
//	Returns: success of string retrieval
//
//------------------------------------------------------------------------------------
BOOL GetRegValueInt( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, int* piValue )
{
char lpszValue[16];
BOOL bOK = TRUE;

	bOK = GetRegValueString( hMainKey, lpszSubKey, lpszValName, lpszValue, MAX_VALUELEN );
	*piValue = atoi( lpszValue );

	return bOK;
}

//------------------------------------------------------------------------------------
//
//	SetRegValueString()
// 
//	Just a little helper routine that takes string and writes it to the	registry.
//
//	Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueString( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue )
{
HKEY hKey;                       // cur open key
LONG lRet;
BOOL bOK = TRUE;

	// open this subkey
	lRet = RegOpenKeyEx( hMainKey, (LPSTR)lpszSubKey, (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );

	// check that you got a good key here
	if( lRet != ERROR_SUCCESS )
	{
	DWORD dwDisposition;

//		Assert(FALSE, "problem on RegOpenKeyEx (write) of subkey ");
//		Assert(FALSE, szSubKey);
//		Assert(FALSE, "\n");

		// OK, you couldn't even open the key !!!

		// **********************************************************************************
		// based on the sketchy documentation we have for this Reg* and Error stuff, we're
		// guessing that you've ended up here because this totally standard, Windows-defined
		// subkey name just doesn't happen to be defined for the current user.
		// **********************************************************************************

		// SO: Just create this subkey for this user, and maybe it will get used after you create and set it.
		// still successful so long as can create new subkey to write to

		lRet = RegCreateKeyEx( hMainKey, (LPSTR)lpszSubKey, (DWORD)0, (LPSTR)NULL, REG_OPTION_NON_VOLATILE,
			KEY_SET_VALUE, (LPSECURITY_ATTRIBUTES)NULL, (PHKEY)&hKey, (LPDWORD)&dwDisposition );
		if( lRet != ERROR_SUCCESS )
		{
//			Assert(FALSE, "problem even with RegCreateKeyEx (write) of subkey ");
//			Assert(FALSE, szSubKey);
//			Assert(FALSE, "\n");

			// oh, oh, couldn't create the key
			bOK = FALSE;
		}
	}
	lRet = RegSetValueEx( hKey, lpszValName, (DWORD)NULL,(DWORD)REG_SZ,(LPBYTE)lpszValue, (DWORD)( lstrlen( lpszValue) + 1 ) );
	bOK = bOK && (lRet == ERROR_SUCCESS);
//	Assert(bOK, "couldn't write a string value to registry!\n");

	// close this key
	RegCloseKey( hKey );

//	Assert(bOK, "didn't SetRegValue well\n");
	return (bOK);
}

//------------------------------------------------------------------------------------
//
//	SetRegValueInt()
// 
//	Just a little helper routine that takes an int and writes it as a string to the
//	registry.
//
//	Returns: success writing to Registry, should be always TRUE.
//
//------------------------------------------------------------------------------------
BOOL SetRegValueInt( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, int iValue )
{
char lpszValue[16];

	_itoa( iValue, lpszValue, 10 );
	return SetRegValueString( hMainKey, lpszSubKey, lpszValName, lpszValue );
}
