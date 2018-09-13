//-----------------------------------------------------------------------
//
//	File: REGUTILS.H
//
//-----------------------------------------------------------------------

#ifndef _REGUTILS_H_
#define _REGUTILS_H_

BOOL GetRegValueString( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue, int iMaxSize );
BOOL GetRegValueInt( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, int* piValue );
BOOL SetRegValueString( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue );
BOOL SetRegValueInt( HKEY hMainKey, LPSTR lpszSubKey, LPSTR lpszValName, int iValue );

BOOL IconSetRegValueString(LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue );
BOOL IconGetRegValueString(LPSTR lpszSubKey, LPSTR lpszValName, LPSTR lpszValue, int iMaxSize );

#endif //_REGUTILS_H_
