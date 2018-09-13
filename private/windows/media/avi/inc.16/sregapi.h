/* *********************************************************************
 *	SRegAPI.h Header file for registry base api function prototypes 
 *      for Chicago SETUP only
 *
 * Microsoft Corporation 
 * Copyright 1994
 * 
 * Author:  Nagarajan Subramaniyan 
 * Created: 2/18/94
 *  
 * Modification history:
 * **********************************************************************
*/

#undef HKEY_CLASSES_ROOT

#include <regapi.h>		// Include the real mode stuff

#ifndef _INC_WINDOWS
LONG    WINAPI RegOpenKey(HKEY, LPCSTR, LPHKEY);
LONG    WINAPI RegCreateKey(HKEY, LPCSTR, LPHKEY);
LONG    WINAPI RegCloseKey(HKEY);
LONG    WINAPI RegDeleteKey(HKEY, LPCSTR);
LONG    WINAPI RegSetValue(HKEY, LPCSTR, DWORD, LPCSTR, DWORD);
LONG    WINAPI RegQueryValue(HKEY, LPCSTR, LPSTR, LONG FAR*);
LONG    WINAPI RegEnumKey(HKEY, DWORD, LPSTR, DWORD);
#endif

#if !defined(_INC_WINDOWS ) || (WINVER < 0x0400)
LONG    WINAPI RegDeleteValue(HKEY, LPCSTR);
LONG    WINAPI RegEnumValue(HKEY, DWORD, LPCSTR,
                         LONG FAR *, DWORD, LONG FAR *, LPBYTE,
                         LONG FAR *);
LONG    WINAPI RegQueryValueEx(HKEY, LPCSTR, LONG FAR *, LONG FAR *,
			    LPBYTE, LONG FAR *);
LONG    WINAPI RegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, LPBYTE, DWORD);
LONG    WINAPI RegFlushKey(HKEY);
LONG	WINAPI RegSaveKey(HKEY, LPCSTR,LPVOID);
LONG	WINAPI RegLoadKey(HKEY, LPCSTR,LPCSTR);
LONG	WINAPI RegUnLoadKey(HKEY, LPCSTR);
#endif  // #if !defined(_INC_WINDOWS ) || (WINVER < 0x4000)

