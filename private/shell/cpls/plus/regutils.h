//-----------------------------------------------------------------------
//
//      File: REGUTILS.H
//
//-----------------------------------------------------------------------

#ifndef _REGUTILS_H_
#define _REGUTILS_H_

BOOL GetRegValueString( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int iMaxSize );
BOOL GetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int* piValue );
BOOL SetRegValueString( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue );
BOOL SetRegValueInt( HKEY hMainKey, LPCTSTR lpszSubKey, LPCTSTR lpszValName, int iValue );
BOOL SetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue, DWORD dwVal );
DWORD GetRegValueDword( HKEY hk, LPCTSTR pSubKey, LPCTSTR pValue );
#define REG_BAD_DWORD 0xF0F0F0F0

BOOL IconSetRegValueString(LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPCTSTR lpszValue );
BOOL IconGetRegValueString(LPCTSTR lpszSubKey, LPCTSTR lpszValName, LPTSTR lpszValue, int iMaxSize );

#endif //_REGUTILS_H_
