/////////////////////////////////////////////////////////////////////////////
// REGINI.H
//
// Registry helper functions declaration
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     09/18/96    Adapted from ccteng's source
// jaym     04/29/97    Cleanup and simplification
/////////////////////////////////////////////////////////////////////////////
#ifndef __REGINI_H__
#define __REGINI_H__

DWORD ReadRegValue(HKEY hKeyRoot, LPCTSTR lpszSubKey, LPCTSTR lpszValue, BYTE * pbReturn, DWORD * lpdwReturnSize);
DWORD ReadRegString(HKEY hKeyRoot, LPCTSTR lpszSection, LPCTSTR lpszKey, LPTSTR lpszDefault, LPTSTR lpszReturn, DWORD cchReturnSize);
DWORD ReadRegDWORD(HKEY hKeyRoot, LPCTSTR lpszSection, LPCTSTR lpszKey, DWORD dwDefault);
BOOL WriteRegValue(HKEY hKeyRoot, LPCTSTR lpszSubKey, LPCTSTR lpszValue, DWORD dwValueType, BYTE * lpValue, DWORD dwSize);

#endif  //__REGINI_H
