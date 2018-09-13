//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       util.cxx
//
//  Contents:   Misc helper functions
//
//  History:    5-Apr-95    BruceFo Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "util.hxx"

//////////////////////////////////////////////////////////////////////////////

#define NETMSG_DLL TEXT("netmsg.dll")

//////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Function:   MyFormatMessageText
//
//  Synopsis:   Given a resource IDs, load strings from given instance
//              and format the string into a buffer
//
//  History:    11-Aug-93 WilliamW   Created.
//
//--------------------------------------------------------------------------
VOID
MyFormatMessageText(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    IN va_list * parglist
    )
{
    //
    // get message from system or app msg file.
    //

    DWORD dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE,
                             g_hInstance,
                             dwMsgId,
                             GetUserDefaultLangID(),
                             pszBuffer,
                             dwBufferSize,
                             parglist);

    if (0 == dwReturn)   // couldn't find message
    {
        appDebugOut((DEB_IERROR,
            "FormatMessage failed, 0x%08lx\n",
            GetLastError()));

        WCHAR szText[200];
        LoadString(g_hInstance, IDS_APP_MSG_NOT_FOUND, szText, ARRAYLEN(szText));
        wsprintf(pszBuffer,szText,dwMsgId);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   MyCommonDialog
//
//  Synopsis:   Common popup dialog routine - stole from diskadm directory
//
//--------------------------------------------------------------------------
DWORD
MyCommonDialog(
    IN HWND    hwnd,
    IN HRESULT dwMsgCode,
    IN PWSTR   pszCaption,
    IN DWORD   dwFlags,
    IN va_list arglist
    )
{
    WCHAR szMsgBuf[500];

    MyFormatMessageText(dwMsgCode, szMsgBuf, ARRAYLEN(szMsgBuf), &arglist);
    return MessageBox(hwnd, szMsgBuf, pszCaption, dwFlags);
}


//+-------------------------------------------------------------------------
//
//  Function:   MyConfirmationDialog
//
//  Synopsis:   This routine retreives a message from the app or system
//              message file and displays it in a message box.
//
//  Note:       Stole from diskadm directory
//
//--------------------------------------------------------------------------
DWORD
MyConfirmationDialog(
    IN HWND hwnd,
    IN HRESULT dwMsgCode,
    IN DWORD dwFlags,
    ...
    )
{
    WCHAR szCaption[100];
    DWORD dwReturn;
    va_list arglist;

    va_start(arglist, dwFlags);

    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));
    dwReturn = MyCommonDialog(hwnd, dwMsgCode, szCaption, dwFlags, arglist);
    va_end(arglist);

    return dwReturn;
}


//+-------------------------------------------------------------------------
//
//  Function:   MyErrorDialog
//
//  Synopsis:   This routine retreives a message from the app or system
//              message file and displays it in a message box.
//
//  Note:       Stole from diskadm directory
//
//--------------------------------------------------------------------------
VOID
MyErrorDialog(
    IN HWND hwnd,
    IN HRESULT dwErrorCode,
    ...
    )
{
    WCHAR szCaption[100];
    va_list arglist;

    va_start(arglist, dwErrorCode);

    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));
    MyCommonDialog(hwnd, dwErrorCode, szCaption, MB_ICONSTOP | MB_OK, arglist);

    va_end(arglist);
}


//+---------------------------------------------------------------------------
//
//  Function:   NewDup
//
//  Synopsis:   Duplicate a string using '::new'
//
//  History:    28-Dec-94   BruceFo   Created
//
//----------------------------------------------------------------------------

PWSTR
NewDup(
    IN const WCHAR* psz
    )
{
    if (NULL == psz)
    {
        appDebugOut((DEB_IERROR,"Illegal string to duplicate: NULL\n"));
        return NULL;
    }

    PWSTR pszRet = new WCHAR[wcslen(psz) + 1];
    if (NULL == pszRet)
    {
        appDebugOut((DEB_ERROR,"OUT OF MEMORY\n"));
        return NULL;
    }

    wcscpy(pszRet, psz);
    return pszRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetResourceString
//
//  Synopsis:   Load a resource string, are return a "new"ed copy
//
//  Arguments:  [dwId] -- a resource string ID
//
//  Returns:    new memory copy of a string
//
//  History:    5-Apr-95    BruceFo Created
//
//----------------------------------------------------------------------------

PWSTR
GetResourceString(
    IN DWORD dwId
    )
{
    WCHAR sz[50];
    if (0 == LoadString(g_hInstance, dwId, sz, ARRAYLEN(sz)))
    {
        return NULL;
    }
    else
    {
        return NewDup(sz);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     FindLastComponent, public
//
//  Synopsis:   Parse a string to find the last component.
//
//  History:    21-Nov-94   BruceFo
//
//--------------------------------------------------------------------------

PWSTR
FindLastComponent(
    IN WCHAR* pszStr
    )
{
    PWSTR pszTmp = wcsrchr(pszStr, L'\\');
    if (pszTmp != NULL)
    {
        return pszTmp + 1;
    }
    else
    {
        return pszStr;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CopySecurityDescriptor, public
//
//  Synopsis:   Copy an NT security descriptor. The security descriptor must
//              be in self-relative (not absolute) form. Delete the result
//              using "delete[] (BYTE*)pSecDesc".
//
//  History:    19-Apr-95   BruceFo     Created
//
//--------------------------------------------------------------------------

PSECURITY_DESCRIPTOR
CopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecDesc
    )
{
    appDebugOut((DEB_ITRACE, "CopySecurityDescriptor, pSecDesc = 0x%08lx\n", pSecDesc));

    if (NULL == pSecDesc)
    {
        return NULL;
    }

    appAssert(IsValidSecurityDescriptor(pSecDesc));

    LONG err;

    DWORD dwLen = GetSecurityDescriptorLength(pSecDesc);
    PSECURITY_DESCRIPTOR pSelfSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[dwLen];
    if (NULL == pSelfSecDesc)
    {
        appDebugOut((DEB_ERROR, "new SECURITY_DESCRIPTOR (2) failed\n"));
        return NULL;    // actually, should probably return an error
    }

    DWORD cbSelfSecDesc = dwLen;
    if (!MakeSelfRelativeSD(pSecDesc, pSelfSecDesc, &cbSelfSecDesc))
    {
        appDebugOut((DEB_TRACE, "MakeSelfRelativeSD failed, 0x%08lx\n", GetLastError()));

        // assume it failed because it was already self-relative
        CopyMemory(pSelfSecDesc, pSecDesc, dwLen);
    }

    appAssert(IsValidSecurityDescriptor(pSelfSecDesc));

    return pSelfSecDesc;
}


//+-------------------------------------------------------------------------
//
//  Member:     WarnDelShare, public
//
//  Synopsis:   Function to warn a user that a share will be deleted, and give
//              the user a chance to cancel.
//
//  Arguments:  [hwnd] - parent window handle for messages
//              [idMsg] - message ID to display (rmdir vs. move)
//              [pszShare] - share name
//              [pszPath] - path that share affects
//
//  Returns:    IDYES if share was deleted, IDNO if we don't want to delete,
//              but keep going, IDCANCEL to stop going.
//
//  History:    19-Apr-95   BruceFo     Created
//
//--------------------------------------------------------------------------

UINT
WarnDelShare(
    IN HWND hwnd,
    IN UINT idMsg,
    IN PWSTR pszShare,
    IN PWSTR pszPath
    )
{
    DWORD id = MyConfirmationDialog(
                    hwnd,
                    idMsg,
                    MB_YESNOCANCEL | MB_ICONEXCLAMATION,
                    pszPath,
                    pszShare);
    if (id != IDYES)
    {
        return id;
    }

    id = ConfirmStopShare(hwnd, MB_YESNOCANCEL, pszShare);
    if (id != IDYES)
    {
        return id;
    }

    UINT ret = NetShareDel(NULL, pszShare, 0);
    if (ret != NERR_Success)
    {
        DisplayError(hwnd, IERR_CANT_DEL_SHARE, ret, pszShare);
        return IDYES;   // allow the stop anyway
    }

    return IDYES;
}


//+-------------------------------------------------------------------------
//
//  Member:     ConfirmStopShare, public
//
//  Synopsis:   Display the appropriate confirmations when stopping a share.
//
//  Arguments:  [hwnd] - parent window handle for messages
//              [uType] - either MB_YESNO or MB_YESNOCANCEL
//              [pszShare] - ptr to affected share name
//
//  Returns:    IDYES if share should be deleted, IDNO if we don't want to
//              delete, but keep going, IDCANCEL to stop going.
//
//  History:    19-Apr-95   BruceFo     Created
//
//--------------------------------------------------------------------------

DWORD
ConfirmStopShare(
    IN HWND hwnd,
    IN UINT uType,
    IN LPWSTR pszShare
    )
{
    DWORD cConns, cOpens;
    NET_API_STATUS err = ShareConnectionInfo(pszShare, &cConns, &cOpens);
    if (err != NERR_Success)
    {
        DisplayError(hwnd, IERR_CANT_DEL_SHARE, err, pszShare);
        return IDYES;   // allow the stop anyway
    }

    if (cConns != 0)
    {
        // If there are any open files, just give the more detailed
        // message about there being open files. Otherwise, just say how
        // many connections there are.

        if (cOpens != 0)
        {
            return MyConfirmationDialog(
                        hwnd,
                        MSG_STOPSHAREOPENS,
                        uType | MB_ICONEXCLAMATION,
                        cOpens,
                        cConns,
                        pszShare);
        }
        else
        {
            return MyConfirmationDialog(
                        hwnd,
                        MSG_STOPSHARECONNS,
                        uType | MB_ICONEXCLAMATION,
                        cConns,
                        pszShare);
        }
    }

    return IDYES;           /* OK to delete */
}



//+-------------------------------------------------------------------------
//
//  Member:     ShareConnectionInfo, public
//
//  Synopsis:   Determine how many connections and file opens exist for a
//              share, for use by confirmation dialogs.
//
//  Arguments:  [pszShare] - ptr to affected share name
//              [pcConns]  - *pcConns get the number of connections
//              [pcOpens]  - *pcOpens get the number of file opens
//
//  Returns:    standard net api code, NERR_Success if everything ok.
//
//  History:    19-Apr-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

NET_API_STATUS
ShareConnectionInfo(
    IN LPWSTR pszShare,
    OUT LPDWORD pcConns,
    OUT LPDWORD pcOpens
    )
{
    CONNECTION_INFO_1* pBuf;

    DWORD iEntry, iTotal;
    NET_API_STATUS err = NetConnectionEnum(
                            NULL,
                            pszShare,
                            1,
                            (LPBYTE*)&pBuf,
                            0xffffffff,     // no buffer limit; get them all!
                            &iEntry,
                            &iTotal,
                            NULL);

   if ((err == NERR_Success) || (err == ERROR_MORE_DATA))
   {
      int iConnections = 0;
      for (DWORD i = 0; i < iEntry; i++)
      {
          iConnections += pBuf[i].coni1_num_opens;
      }

      *pcConns = iTotal;
      *pcOpens = iConnections;
      err = NERR_Success;
   }
   else
   {
      *pcConns = 0;
      *pcOpens = 0;
   }
   NetApiBufferFree(pBuf);

   appDebugOut((DEB_ITRACE,"Share '%ws' has %d connections and %d opens\n", pszShare, *pcConns, *pcOpens));

   return err;
}


//+-------------------------------------------------------------------------
//
//  Function:   DisplayError
//
//  Synopsis:   Display an error message
//
//  History:    24-Apr-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

VOID
DisplayError(
    IN HWND           hwnd,
    IN HRESULT        dwErrorCode, // message file number. not really an HRESULT
    IN NET_API_STATUS err,
    IN PWSTR          pszShare
    )
{
    if (   err < MIN_LANMAN_MESSAGE_ID
        || err > MAX_LANMAN_MESSAGE_ID
        )
    {
        // a Win32 error?

        WCHAR szMsg[500];
        DWORD dwReturn = FormatMessage(
                                 FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 err,
                                 GetUserDefaultLangID(),
                                 szMsg,
                                 ARRAYLEN(szMsg),
                                 NULL);
        if (0 == dwReturn)   // couldn't find message
        {
            appDebugOut((DEB_IERROR,
                "FormatMessage (from system) failed, 0x%08lx\n",
                GetLastError()));

            MyErrorDialog(hwnd, IERR_UNKNOWN, err);
        }
        else
        {
            MyErrorDialog(hwnd, dwErrorCode, pszShare, szMsg);
        }
    }
    else
    {
        DisplayLanmanError(hwnd, dwErrorCode, err, pszShare);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   DisplayLanmanError
//
//  Synopsis:   Display an error message from a LanMan error.
//
//  History:    24-Apr-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

VOID
DisplayLanmanError(
    IN HWND           hwnd,
    IN HRESULT        dwErrorCode, // message file number. not really an HRESULT
    IN NET_API_STATUS err,
    IN PWSTR          pszShare
    )
{
    if (   err < MIN_LANMAN_MESSAGE_ID
        || err > MAX_LANMAN_MESSAGE_ID
        )
    {
        MyErrorDialog(hwnd, IERR_UNKNOWN, err);
        return;
    }

    WCHAR szCaption[100];
    LoadString(g_hInstance, IDS_MSGTITLE, szCaption, ARRAYLEN(szCaption));

    //
    // get LanMan message from system message file.
    //

    WCHAR szNetMsg[500];
    WCHAR szBuf[500];

    HINSTANCE hInstanceNetMsg = LoadLibrary(NETMSG_DLL);
    if (NULL == hInstanceNetMsg)
    {
        appDebugOut((DEB_IERROR,
            "LoadLibrary(netmsg.dll) failed, 0x%08lx\n",
            GetLastError()));

        LoadString(g_hInstance, IDS_NO_NET_MSG, szBuf, ARRAYLEN(szBuf));
        MessageBox(hwnd, szBuf, szCaption, MB_ICONSTOP | MB_OK);
        return;
    }

    DWORD dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE,
                             hInstanceNetMsg,
                             err,
                             GetUserDefaultLangID(),
                             szNetMsg,
                             ARRAYLEN(szNetMsg),
                             NULL);
    if (0 == dwReturn)   // couldn't find message
    {
        appDebugOut((DEB_IERROR,
            "FormatMessage failed, 0x%08lx\n",
            GetLastError()));

        LoadString(g_hInstance, IDS_NET_MSG_NOT_FOUND, szBuf, ARRAYLEN(szBuf));
        wsprintf(szNetMsg, szBuf, GetLastError());
        MessageBox(hwnd, szNetMsg, szCaption, MB_ICONSTOP | MB_OK);
    }
    else
    {
        MyErrorDialog(hwnd, dwErrorCode, pszShare, szNetMsg);
    }

    FreeLibrary(hInstanceNetMsg);
}


//+-------------------------------------------------------------------------
//
//  Function:   IsValidShareName
//
//  Synopsis:   Checks if the proposed share name is valid or not. If not,
//              it will return a message id for the reason why.
//
//  Arguments:  [pszShareName] - Proposed share name
//              [puId] - If name is invalid, this will contain the reason why.
//
//  Returns:    TRUE if name is valid, else FALSE.
//
//  History:    3-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

BOOL
IsValidShareName(
    IN  PCWSTR pszShareName,
    OUT HRESULT* uId
    )
{
    if (NetpNameValidate(NULL, (PWSTR)pszShareName, NAMETYPE_SHARE, 0L) != NERR_Success)
    {
        *uId = IERR_InvalidShareName;
        return FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetErrorFocus
//
//  Synopsis:   Set focus to an edit control and select its text.
//
//  Arguments:  [hwnd] - dialog window
//              [idCtrl] - edit control to set focus to (and select)
//
//  Returns:    nothing
//
//  History:    3-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

VOID
SetErrorFocus(
    IN HWND hwnd,
    IN UINT idCtrl
    )
{
    HWND hCtrl = ::GetDlgItem(hwnd, idCtrl);
    ::SetFocus(hCtrl);
    ::SendMessage(hCtrl, EM_SETSEL, 0, -1);
}


//+-------------------------------------------------------------------------
//
//  Function:   ConfirmReplaceShare
//
//  Synopsis:   Display confirmations for replacing an existing share
//
//  Arguments:  [hwnd] - dialog window
//              [pszShareName] - name of share being replaced
//              [pszOldPath] - current path for the share
//              [pszNewPath] - directory the user's trying to share
//
//  Returns:    Returns IDYES or IDNO
//
//  History:    4-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

DWORD
ConfirmReplaceShare(
    IN HWND hwnd,
    IN PCWSTR pszShareName,
    IN PCWSTR pszOldPath,
    IN PCWSTR pszNewPath
    )
{
    DWORD id = MyConfirmationDialog(
                    hwnd,
                    MSG_RESHARENAMECONFIRM,
                    MB_YESNO | MB_ICONEXCLAMATION,
                    pszOldPath,
                    pszShareName,
                    pszNewPath);
    if (id != IDYES)
    {
        return id;
    }

    return ConfirmStopShare(hwnd, MB_YESNO, (PWSTR)pszShareName);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsWorkstationProduct
//
//  Synopsis:   Determines the NT product type (server or workstation),
//              and returns TRUE if it is workstation.
//
//  Arguments:  (none)
//
//  Returns:    TRUE if running on workstation products
//
//  History:    11-Sep-95 BruceFo   Created
//
//----------------------------------------------------------------------------

BOOL
IsWorkstationProduct(
    VOID
    )
{
    //
    // Determine whether this is the workstation or server product by looking
    // at HKEY_LOCAL_MACHINE, System\CurrentControlSet\Control\ProductOptions.
    // The ProductType value therein is interpreted as follows:
    //
    // LanmanNt -- server product, running as domain controller
    // ServerNt -- server product, not a domain controller
    // WinNT    -- workstation product
    //

    LONG    ec;
    HKEY    hkey;
    DWORD   type;
    DWORD   size;
    UCHAR   buf[100];
    BOOL    fIsWorkstation = TRUE;

    ec = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                0,
                KEY_QUERY_VALUE,
                &hkey
                );

    if (ec == NO_ERROR)
    {
        size = sizeof(buf);
        ec = RegQueryValueEx(hkey,
                             TEXT("ProductType"),
                             NULL,
                             &type,
                             buf,
                             &size);

        if ((ec == NO_ERROR) && (type == REG_SZ))
        {
            if (0 == lstrcmpi((LPTSTR)buf, TEXT("lanmannt")))
            {
                fIsWorkstation = FALSE;
            }

            if (0 == lstrcmpi((LPTSTR)buf, TEXT("servernt")))
            {
                fIsWorkstation = FALSE;
            }
        }

        RegCloseKey(hkey);
    }

    return fIsWorkstation;
}
