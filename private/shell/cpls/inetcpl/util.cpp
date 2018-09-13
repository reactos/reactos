
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  
//  12/21/94    jeremys     Created.
//

#include "inetcplp.h"

#include <advpub.h>         // For REGINSTALL

#include <mluisupp.h>

// function prototypes
VOID _cdecl FormatErrorMessage(TCHAR * pszMsg,DWORD cbMsg,TCHAR * pszFmt,va_list ArgList);
extern VOID GetRNAErrorText(UINT uErr,CHAR * pszErrText,DWORD cbErrText);
extern VOID GetMAPIErrorText(UINT uErr,CHAR * pszErrText,DWORD cbErrText);

/*******************************************************************

    NAME:       MsgBox

    SYNOPSIS:   Displays a message box with the specified string ID

********************************************************************/
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons)
{
    TCHAR szMsgBuf[MAX_RES_LEN+1];
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];

    MLLoadShellLangString(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));
    MLLoadShellLangString(nMsgID,szMsgBuf,sizeof(szMsgBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons));

}

/*******************************************************************

    NAME:       MsgBoxSz

    SYNOPSIS:   Displays a message box with the specified text

********************************************************************/
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons)
{
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];
    MLLoadShellLangString(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}

/*******************************************************************

    NAME:       MsgBoxParam

    SYNOPSIS:   Displays a message box with the specified string ID

    NOTES:      extra parameters are string pointers inserted into nMsgID.

********************************************************************/
int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,...)
{

        va_list nextArg;

    BUFFER Msg(3*MAX_RES_LEN+1);    // nice n' big for room for inserts
    BUFFER MsgFmt(MAX_RES_LEN+1);

    if (!Msg || !MsgFmt) {
        return MsgBox(hWnd,IDS_ERROutOfMemory,MB_ICONSTOP,MB_OK);
    }

        MLLoadShellLangString(nMsgID,MsgFmt.QueryPtr(),MsgFmt.QuerySize());

        va_start(nextArg, uButtons);

    FormatErrorMessage(Msg.QueryPtr(),Msg.QuerySize(),
        MsgFmt.QueryPtr(),nextArg);
        va_end(nextArg);
    return MsgBoxSz(hWnd,Msg.QueryPtr(),uIcon,uButtons);
}

BOOL EnableDlgItem(HWND hDlg,UINT uID,BOOL fEnable)
{
    return EnableWindow(GetDlgItem(hDlg,uID),fEnable);
}


/*******************************************************************

    NAME:       LoadSz

    SYNOPSIS:   Loads specified string resource into buffer

    EXIT:       returns a pointer to the passed-in buffer

    NOTES:      If this function fails (most likely due to low
                memory), the returned buffer will have a leading NULL
                so it is generally safe to use this without checking for
                failure.

********************************************************************/
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
{
    ASSERT(lpszBuf);

    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        MLLoadString( idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

/*******************************************************************

    NAME:       FormatErrorMessage

    SYNOPSIS:   Builds an error message by calling FormatMessage

    NOTES:      Worker function for DisplayErrorMessage

********************************************************************/
VOID _cdecl FormatErrorMessage(TCHAR * pszMsg,DWORD cbMsg,TCHAR * pszFmt,va_list ArgList)
{
    ASSERT(pszMsg);
    ASSERT(pszFmt);

    // build the message into the pszMsg buffer
    DWORD dwCount = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
        pszFmt,0,0,pszMsg,cbMsg,&ArgList);
    ASSERT(dwCount > 0);
}


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;

    STRENTRY seReg[] = {
#ifdef WINNT
        { "CHANNELBARINIT", "no" }, // channel bar off by default on NT
#else
        { "CHANNELBARINIT", "yes" } // channel bar on by default on Win95/98
#endif
    };
    STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

    RegInstall(ghInstance, szSection, &stReg);

    return hr;
}

/*----------------------------------------------------------
Purpose: Install/uninstall user settings

*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    ASSERT(IS_VALID_STRING_PTRW(pszCmdLine, -1));

#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllInstall");
        DEBUG_BREAK;
    }
#endif

    if (bInstall)
    {
        //
        // We use to delete the whole key here - that doesn't work anymore
        // because other people write to this key and we don't want 
        // to crush them. If you need to explicity delete a value
        // add it to ao_2 value
        // CallRegInstall("UnregDll");
        CallRegInstall("RegDll");
        
        // If we also have the integrated shell installed, throw in the options
        // related to the Integrated Shell.
        if (WhichPlatform() == PLATFORM_INTEGRATED)
            CallRegInstall("RegDll.IntegratedShell");

        // NT5 has special reg key settings
        if (IsOS(OS_NT5))
            CallRegInstall("RegDll.NT5");
    }
    else
    {
        CallRegInstall("UnregDll");
    }

    return S_OK;
}

