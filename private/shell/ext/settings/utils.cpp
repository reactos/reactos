///////////////////////////////////////////////////////////////////////////////
/*  File: utils.cpp

    Description: Contains any general utility functions applicable to the
        dskquota project.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "precomp.hxx" // PCH
#pragma hdrstop

///////////////////////////////////////////////////////////////////////////////
/*  Function: FmtMsgSprintf

    Description: Loads a message resource and formats it using a variable 
        length argument list.  See the MS message compiler (MC) for 
        details on format specifications.

        This function was borrowed from JonPa's implementation of the 
        font viewer applet.  

    Arguments:
        id - Identifier of the message template.

    Returns:  Address of formatted string or the address of the c_szEllipsis
        constant string.  The caller MUST call LocalFree() to free the 
        string buffer.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
const TCHAR c_szEllipsis[] = TEXT("...");

LPTSTR FmtMsgSprintf(DWORD id, ...) 
{
    LPTSTR pszMsg = NULL;
    va_list marker;

    va_start(marker, id);

    if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_HMODULE | 
                      FORMAT_MESSAGE_MAX_WIDTH_MASK, 
                      g_hInstance,
                      id, 
                      0, 
                      (LPTSTR)&pszMsg, 
                      1, 
                      &marker) && NO_ERROR != GetLastError()) 
    {
        DebugMsg(DM_ERROR, TEXT("FmtMsgSprintf failed with error 0x%08X"), GetLastError());
        pszMsg = (LPTSTR)LocalAlloc(LPTR, (lstrlen(c_szEllipsis) + 1) * sizeof(TCHAR));
        if (NULL != pszMsg)
            lstrcpy(pszMsg, c_szEllipsis);
    }
    va_end( marker );

    return pszMsg;
}


VOID FmtMsgSprintf(LPTSTR pszDest, UINT cchDest, DWORD id, ...) 
{
    LPTSTR pszMsg = NULL;
    va_list marker;

    va_start(marker, id);

    Assert(NULL != pszDest);

    if(!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
                      FORMAT_MESSAGE_MAX_WIDTH_MASK, 
                      g_hInstance,
                      id, 
                      0, 
                      pszDest, 
                      cchDest, 
                      &marker)) 
    {
        DebugMsg(DM_ERROR, TEXT("FmtMsgSprintf failed with error 0x%08X"), GetLastError());
        lstrcpyn(pszDest, c_szEllipsis, cchDest);
    }
    va_end( marker );
}

   
//
// Duplicate a string.
//
LPTSTR StringDup(
    LPCTSTR pszSource
    ) throw(OutOfMemory)
{
    LPTSTR pszNew = new TCHAR[lstrlen(pszSource) + 1];
    lstrcpy(pszNew, pszSource);

    return pszNew;
}


INT SettingsMsgBox(
    HWND hWndParent,
    UINT idMsgText,
    UINT idMsgTitle,
    UINT uType
    )
{
    LPTSTR pszTitle = NULL;
    LPTSTR pszText  = NULL;
    INT iReturn     = 0;

    pszTitle = FmtMsgSprintf(idMsgTitle);
    pszText  = FmtMsgSprintf(idMsgText);

    iReturn = MessageBox(hWndParent, pszText, pszTitle, uType);

    delete[] pszTitle;
    delete[] pszText;

    return iReturn;
}


INT SettingsMsgBox(
    HWND hWndParent,
    LPCTSTR pszText,
    LPCTSTR pszTitle,
    UINT uType
    )
{
    return MessageBox(hWndParent, pszText, pszTitle, uType);
}


INT SettingsMsgBox(
    HWND hWndParent,
    UINT idMsgText,
    LPCTSTR pszTitle,
    UINT uType
    )
{
    LPTSTR pszText = NULL;
    INT iReturn    = 0;
 
    pszText = FmtMsgSprintf(idMsgText);

    iReturn = MessageBox(hWndParent, pszText, pszTitle, uType);

    delete[] pszText;

    return iReturn;
}

    

INT SettingsMsgBox(
    HWND hWndParent,
    LPCTSTR pszText,
    UINT idMsgTitle,
    UINT uType
    )
{
    LPTSTR pszTitle = NULL;
    INT iReturn     = 0;
 
    pszTitle = FmtMsgSprintf(idMsgTitle);

    iReturn = MessageBox(hWndParent, pszText, pszTitle, uType);

    delete[] pszTitle;

    return iReturn;
}
