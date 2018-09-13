#ifndef __MISC_UTILITIES_H
#define __MISC_UTILITIES_H
///////////////////////////////////////////////////////////////////////////////
/*  File: utils.h

    Description: Header for general utilities module.  It is expected that
        windows.h is included before this header.



    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

VOID FmtMsgSprintf(LPTSTR pszDest, UINT cchDest, DWORD id, ...);
LPTSTR FmtMsgSprintf(DWORD id, ...);
LPTSTR StringDup(LPCTSTR pszSource) throw(OutOfMemory);

INT SettingsMsgBox(HWND hWndParent,
                   LPCTSTR pszText,
                   LPCTSTR pszTitle,
                   UINT uType);

INT SettingsMsgBox(HWND hWndParent,
                    UINT idMsgText,
                    UINT idMsgTitle,
                    UINT uType);

INT SettingsMsgBox(HWND hWndParent,
                    UINT idMsgText,
                    LPCTSTR pszTitle,
                    UINT uType);

INT SettingsMsgBox(HWND hWndParent,
                    LPCTSTR pszText,
                    UINT idMsgTitle,
                    UINT uType);



#endif // __MISC_UTILITIES_H

