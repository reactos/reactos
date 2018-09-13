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
#include "shares.h"
#include "util.hxx"
#include "shrpage.hxx"
#include "dlgnew.hxx"

//////////////////////////////////////////////////////////////////////////////

DECLARE_INFOLEVEL(Sharing)

#define NETMSG_DLL TEXT("netmsg.dll")

#ifdef WIZARDS
TCHAR g_szShareWizard[] = TEXT("ShrPubW.exe /folder "); // The Share Publishing Wizard
TCHAR g_szSfm[]  = TEXT("/sfm ");   // Services For Macintosh
TCHAR g_szFpnw[] = TEXT("/fpnw ");  // File and Print Services for NetWare
TCHAR g_szAll[]  = TEXT("/all ");   // all services
#endif // WIZARDS

//--------------------------------------------------------------------------
// Globals used elsewhere

UINT        g_NonOLEDLLRefs = 0;
HINSTANCE   g_hInstance = NULL;
UINT        g_uiMaxUsers = 0;   // max number of users based on product type
WCHAR       g_szAdminShare[] = L"ADMIN$";
WCHAR       g_szIpcShare[]   = L"IPC$";
UINT        g_cfHIDA = 0;

//////////////////////////////////////////////////////////////////////////////

DWORD
ConfirmStopShare(
    IN HWND hwnd,
    IN LPWSTR pszShare
    );

NET_API_STATUS
ShareConnectionInfo(
    IN LPWSTR pszShare,
    OUT LPDWORD pcConns,
    OUT LPDWORD pcOpens
    );

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
                             LANG_USER_DEFAULT,
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
//  Function:   MyFormatMessage
//
//  Synopsis:
//
//  Note:
//
//--------------------------------------------------------------------------
VOID
MyFormatMessage(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    ...
    )
{
    va_list arglist;
    va_start(arglist, dwBufferSize);
    MyFormatMessageText(dwMsgId, pszBuffer, dwBufferSize, &arglist);
    va_end(arglist);
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
//  Member:     CopySecurityDescriptor, public
//
//  Synopsis:   Copy an NT security descriptor. The security descriptor must
//              be in self-relative (not absolute) form. Delete the result
//              using LocalFree().
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
    PSECURITY_DESCRIPTOR pSelfSecDesc = reinterpret_cast<PSECURITY_DESCRIPTOR>(
		::LocalAlloc(LMEM_ZEROINIT, dwLen) );
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
                                 LANG_USER_DEFAULT,
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
                             LANG_USER_DEFAULT,
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
//  Returns:    Returns IDYES, IDNO, or IDCANCEL
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

    return ConfirmStopShare(hwnd, (PWSTR)pszShareName);
}

//+-------------------------------------------------------------------------
//
//  Member:     ConfirmStopShare, public
//
//  Synopsis:   Display the appropriate confirmations when stopping a share.
//
//  Arguments:  [hwnd] - parent window handle for messages
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
                        MB_YESNOCANCEL | MB_ICONEXCLAMATION,
                        cOpens,
                        cConns,
                        pszShare);
        }
        else
        {
            return MyConfirmationDialog(
                        hwnd,
                        MSG_STOPSHARECONNS,
                        MB_YESNOCANCEL | MB_ICONEXCLAMATION,
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

BOOL
DriveLetterShare(
    PWSTR pszShareName
    )
{
    if (NULL == pszShareName || lstrlen(pszShareName) != 2)
    {
        return FALSE;
    }

    // BUGBUG: what about non-English char sets?
    return (   ((pszShareName[0] >= TEXT('a')) && pszShareName[0] <= TEXT('z'))
            || ((pszShareName[0] >= TEXT('A')) && pszShareName[0] <= TEXT('Z'))
            )
           && (pszShareName[1] == TEXT('$'))
           ;
}


#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Function:   DumpNetEnum
//
//  Synopsis:   Dumps an array of SHARE_INFO_1 structures.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

VOID
DumpNetEnum(
    IN LPVOID pBufShares,
    IN ULONG entriesRead
    )
{
    SHARE_INFO_1* pBase = (SHARE_INFO_1*) pBufShares;

    appDebugOut((DEB_TRACE,
        "DumpNetEnum: %d entries\n",
        entriesRead));

    for (ULONG i = 0; i < entriesRead; i++)
    {
        SHARE_INFO_1* p = &(pBase[i]);

        appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t Share name: %ws\n"
"\t       Type: %d (0x%08lx)\n"
"\t    Comment: %ws\n"
"\n"
,
p->shi1_netname,
p->shi1_type, p->shi1_type,
p->shi1_remark
));

    }
}

#endif // DBG == 1

struct SHARE_PROPERTIES_DATA
{
    IUnknown* punk;
    LPTSTR    pszMachine;
    LPTSTR    pszShareName;
};

DWORD CALLBACK
SharePropertiesThreadProc(
    LPVOID lpThreadParameter
    )
{
    SHARE_PROPERTIES_DATA* pData = (SHARE_PROPERTIES_DATA*)lpThreadParameter;
    if (NULL == pData)
    {
        appAssert(!"Unexpected properties thread data");
        return 0;
    }

    WCHAR szCaption[MAX_PATH];
    LoadString(g_hInstance, IDS_SHARE_PROPTITLE, szCaption, ARRAYLEN(szCaption));

    SHARE_PROPSHEETPAGE sprop;

    sprop.psp.dwSize      = sizeof(sprop);    // no extra data.
    sprop.psp.dwFlags     = PSP_USEREFPARENT;
    sprop.psp.hInstance   = g_hInstance;
    sprop.psp.pszTemplate = MAKEINTRESOURCE(IDD_SHARE_PROPERTIES);
    sprop.psp.hIcon       = NULL;
    sprop.psp.pszTitle    = NULL;
    sprop.psp.pfnDlgProc  = CSharingPropertyPage::DlgProcPage;
    sprop.psp.lParam      = 0;
    sprop.psp.pfnCallback = NULL;
    sprop.psp.pcRefParent = &g_NonOLEDLLRefs;
    sprop.pszMachine      = pData->pszMachine;
    sprop.pszShareName    = pData->pszShareName;

    PROPSHEETHEADER psh;

    psh.dwSize     = sizeof(PROPSHEETHEADER);
    psh.dwFlags    = PSH_PROPSHEETPAGE | PSH_USEICONID;
    psh.hwndParent = NULL;
    psh.hInstance  = g_hInstance;
    psh.pszIcon    = MAKEINTRESOURCE(IDI_SHARESFLD);
    psh.pszCaption = szCaption;
    psh.nPages     = 1;
    psh.nStartPage = 0;
    psh.ppsp       = (LPCPROPSHEETPAGE)&sprop;
    psh.pfnCallback= NULL;

    PropertySheet(&psh);

    pData->punk->Release();
    LocalFree(pData);       // The strings are packed in the same allocation!
    return 0;
}

HRESULT
ShareDoProperties(
    IN IUnknown* punk,
    IN LPTSTR    pszMachine,
    IN LPTSTR    pszShareName
    )
{
    if (NULL == pszShareName)
    {
        return E_INVALIDARG;
    }

    DWORD cbMachine = 0;
    DWORD cbStrings = 0;
    if (NULL != pszMachine)
    {
        cbMachine = (lstrlen(pszMachine) + 1) * sizeof(TCHAR);
        cbStrings += cbMachine;
    }
    cbStrings += (lstrlen(pszShareName) + 1) * sizeof(TCHAR);

    HRESULT hr = S_OK;
    HANDLE hThread;
    DWORD idThread;
    SHARE_PROPERTIES_DATA* pData = (SHARE_PROPERTIES_DATA*)LocalAlloc(LPTR, sizeof(SHARE_PROPERTIES_DATA) + cbStrings);
    if (pData)
    {
        if (NULL != pszMachine)
        {
            pData->pszMachine = (LPWSTR)(((LPBYTE)pData) + sizeof(SHARE_PROPERTIES_DATA));
            lstrcpy(pData->pszMachine, pszMachine);
        }
        else
        {
            pData->pszMachine   = NULL;
        }
        pData->pszShareName = (LPWSTR)(((LPBYTE)pData) + sizeof(SHARE_PROPERTIES_DATA) + cbMachine);
        lstrcpy(pData->pszShareName, pszShareName);

        pData->punk = punk;
        pData->punk->AddRef();

        hThread = CreateThread(NULL, 0, SharePropertiesThreadProc, pData, 0, &idThread);

        if (hThread)
        {
            CloseHandle(hThread);
            return S_OK;
        }
        else
        {
            pData->punk->Release();
            LocalFree(pData);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT
ShareDoDelete(
    IN HWND hwndOwner,
    IN PWSTR pszMachine,
    IN PWSTR pszShareName
    )
{
    // Remove the share. We need to know the path that was
    // shared to be able to update the explorer. So, get
    // that.
    SHARE_INFO_1* pInfo1 = NULL;
    SHARE_INFO_2* pInfo2 = NULL;
    DWORD ret;
    HRESULT hr = S_OK;

    ret = NetShareGetInfo(pszMachine, pszShareName, 2, (LPBYTE*)&pInfo2);
    if (NERR_Success != ret)
    {
        // make sure it's null
        pInfo2 = NULL;
    }

    // Warn and confirm if it's a special share, either ADMIN$, IPC$,
    // or <drive>$
    if (NULL == pInfo2)
    {
        // Permissions problem? Try getting SHARE_INFO_1.

        ret = NetShareGetInfo(pszMachine, pszShareName, 1, (LPBYTE*)&pInfo1);
        if (NERR_Success != ret)
        {
            // make sure it's null
            pInfo1 = NULL;
        }
    }
    else
    {
        pInfo1 = (SHARE_INFO_1*)pInfo2; // I just need the type
    }

    if (NULL != pInfo1)
    {
        DWORD id = IDYES;
        if (pInfo1->shi1_type & STYPE_SPECIAL)
        {
            id = MyConfirmationDialog(
                            hwndOwner,
                            MSG_DELETESPECIAL,
                            MB_YESNO | MB_ICONEXCLAMATION,
                            pszShareName);
        }

        if (pInfo1 != (SHARE_INFO_1*)pInfo2)
        {
            NetApiBufferFree(pInfo1);
        }

        if (id != IDYES)
        {
            hr = S_OK;
            goto nodelete;
        }
    }

    // Actually delete the share
    ret = NetShareDel(pszMachine, pszShareName, 0);
    if (NERR_Success == ret)
    {
        if (NULL != pInfo2)
        {
            SHChangeNotify(SHCNE_NETUNSHARE, SHCNF_PATH, pInfo2->shi2_path, 0);
        }
    }
    else
    {
        // BUGBUG: error message to user

        hr = HRESULT_FROM_WIN32(GetLastError());
    }

nodelete:
    if (NULL != pInfo2)
    {
        NetApiBufferFree(pInfo2);
    }

    return hr;
}

struct SHARE_NEW_DATA
{
    IUnknown* punk;
    LPTSTR    pszMachine;
};

DWORD CALLBACK
ShareNewThreadProc(
    LPVOID lpThreadParameter
    )
{
    SHARE_NEW_DATA* pData = (SHARE_NEW_DATA*)lpThreadParameter;
    if (NULL == pData)
    {
        appAssert(!"Unexpected properties thread data");
        return 0;
    }

    CDlgNewShare dlg(NULL, pData->pszMachine);
    if (dlg.DoModal())
    {
    }

    pData->punk->Release();
    LocalFree(pData);       // The strings are packed in the same allocation!
    return 0;
}


HRESULT
ShareDoNew(
    IN IUnknown* punk,
    IN PWSTR pszMachine
    )
{
    DWORD cbStrings = 0;
    if (NULL != pszMachine)
    {
        cbStrings += (lstrlen(pszMachine) + 1) * sizeof(TCHAR);
    }

    HRESULT hr = S_OK;
    HANDLE hThread;
    DWORD idThread;
    SHARE_NEW_DATA* pData = (SHARE_NEW_DATA*)LocalAlloc(LPTR, sizeof(SHARE_NEW_DATA) + cbStrings);
    if (pData)
    {
        if (NULL != pszMachine)
        {
            pData->pszMachine = (LPWSTR)(((LPBYTE)pData) + sizeof(SHARE_NEW_DATA));
            lstrcpy(pData->pszMachine, pszMachine);
        }
        else
        {
            pData->pszMachine   = NULL;
        }

        pData->punk = punk;
        pData->punk->AddRef();

        hThread = CreateThread(NULL, 0, ShareNewThreadProc, pData, 0, &idThread);

        if (hThread)
        {
            CloseHandle(hThread);
            return S_OK;
        }
        else
        {
            pData->punk->Release();
            LocalFree(pData);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


#ifdef WIZARDS
HRESULT
ShareDoSpecial(
    IN HWND hwndOwner,
    IN PWSTR pszMachine,
    IN BYTE bType
    )
{
    // Construct the command line to pass to the Share Wizard

    TCHAR szCommandLine[MAX_PATH];
    lstrcpy(szCommandLine, g_szShareWizard);

    switch (bType)
    {
    case SHID_SHARE_NW:
        wcscat(szCommandLine, g_szFpnw);
        break;

    case SHID_SHARE_MAC:
        wcscat(szCommandLine, g_szSfm);
        break;

    case SHID_SHARE_ALL:
        wcscat(szCommandLine, g_szAll);
        break;

    case SHID_SHARE_NEW:
        // nothing special
        break;

    default: appAssert(!"Unknown object type");
    }

    if (NULL != pszMachine)
    {
        wcscat(szCommandLine, pszMachine);
    }

    appDebugOut((DEB_TRACE, "Invoking wizard with this command line: %ws\n", szCommandLine));

    // Looks like CreateProcess writes to this buffer!
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = { 0 };
    BOOL b = CreateProcess(
                    NULL,
                    szCommandLine,
                    NULL,   // pointer to process security attributes
                    NULL,   // pointer to thread security attributes
                    FALSE,  // handle inheritance flag
                    0,      // creation flags
                    NULL,   // pointer to new environment block
                    NULL,   // pointer to current directory name
                    &si,    // pointer to STARTUPINFO
                    &pi);   // pointer to PROCESS_INFORMATION
    if (b)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        appDebugOut((DEB_ERROR,
            "CreateProcess failed, 0x%08lx\n",
            GetLastError()));

        MyErrorDialog(hwndOwner, MSG_NOWIZARD);
    }

    return S_OK;
}
#endif // WIZARDS


VOID FSSetStatusText(HWND hwndOwner, LPTSTR* ppszText, int iStart, int iEnd)
{
    HWND hwndStatus = NULL;
    IShellBrowser* psb = FileCabinet_GetIShellBrowser(hwndOwner);

    if (psb)
    {
        psb->GetControlWindow(FCW_STATUS, &hwndStatus);
        if (hwndStatus)
        {
            for (; iStart <= iEnd; iStart++)
            {
                LPTSTR lpsz;

                if (ppszText)
                {
                    lpsz = *ppszText;
                    ppszText++;
                }
                else
                {
                    lpsz = (LPTSTR)TEXT("");
                }
#ifdef WINDOWS_ME
                SendMessage(hwndStatus, SB_SETTEXT, SB_RTLREADING | (WPARAM)iStart, (LPARAM)lpsz);
#else
                SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)iStart, (LPARAM)lpsz);
#endif
            }
        }
    }
}


BOOL
IsLevelOk(
    IN PWSTR pszMachine,
    IN DWORD level
    )
{
    LPBYTE pBuf = NULL;
    DWORD entriesread, totalentries;
    NET_API_STATUS ret;

    // we want to get the minimum amount of data, because all we care about
    // is whether it succeeds the access check
    DWORD prefmaxlen = 300;
    for (;; prefmaxlen *= 2)
    {
        ret = NetShareEnum(
                        pszMachine,
                        level,
                        &pBuf,
                        prefmaxlen,
                        &entriesread,
                        &totalentries,
                        NULL);
        if (NERR_BufTooSmall != ret)
        {
            NetApiBufferFree(pBuf);
            break;
        }
    }

    if (ERROR_ACCESS_DENIED == ret)
    {
        return FALSE;
    }
    else if (NERR_Success == ret || ERROR_MORE_DATA == ret)
    {
        return TRUE;
    }
    else
    {
        // some other error
        return FALSE;
    }
}


VOID
SetDialogIconBig(
    IN HWND hwnd,
    WORD idIcon
    )
{
    HICON hiconLarge = (HICON)LoadImage(
                            g_hInstance,
                            MAKEINTRESOURCE(idIcon),
                            IMAGE_ICON,
                            GetSystemMetrics(SM_CXICON),
                            GetSystemMetrics(SM_CYICON),
                            LR_DEFAULTCOLOR);
    if (NULL == hiconLarge)
    {
        appDebugOut((DEB_ERROR,
            "LoadImage for large image failed, 0x%08lx\n",
            GetLastError()));
    }
    else
    {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hiconLarge);
    }
}


VOID
SetDialogIconSmall(
    IN HWND hwnd,
    WORD idIcon
    )
{
    HICON hiconSmall = (HICON)LoadImage(
                            g_hInstance,
                            MAKEINTRESOURCE(idIcon),
                            IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON),
                            GetSystemMetrics(SM_CYSMICON),
                            LR_DEFAULTCOLOR);
    if (NULL == hiconSmall)
    {
        appDebugOut((DEB_ERROR,
            "LoadImage for small image failed, 0x%08lx\n",
            GetLastError()));
    }
    else
    {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hiconSmall);
    }
}

//////////////////////////////////////////////////////////////////////////////

HRESULT
STRRETLoadString(
    UINT ids,
    STRRET* pstr
    )
{
#ifdef UNICODE
    TCHAR szTemp[MAX_PATH];
    szTemp[0] = TEXT('\0');

    LoadString(g_hInstance, ids, szTemp, ARRAYLEN(szTemp));
    pstr->pOleStr = (LPOLESTR)SHAlloc((lstrlen(szTemp)+1) * sizeof(TCHAR));
    if (NULL != pstr->pOleStr)
    {
        pstr->uType = STRRET_OLESTR;
        lstrcpy(pstr->pOleStr, szTemp);
    }
    else
    {
        pstr->uType = STRRET_CSTR;
        pstr->cStr[0] = '\0';
        return E_OUTOFMEMORY;
    }
#else
    pstr->uType = STRRET_CSTR;
    LoadString(g_hInstance, ids, pstr->cStr, ARRAYLEN(pstr->cStr));
#endif

    return S_OK;
}

HRESULT
STRRETCopy(
    LPTSTR pszString,
    STRRET* pstr
    )
{
#ifdef UNICODE
    pstr->pOleStr = (LPOLESTR)SHAlloc((lstrlen(pszString)+1) * sizeof(TCHAR));
    if (NULL != pstr->pOleStr)
    {
        pstr->uType = STRRET_OLESTR;
        lstrcpy(pstr->pOleStr, pszString);
    }
    else
    {
        pstr->uType = STRRET_CSTR;
        pstr->cStr[0] = '\0';
        return E_OUTOFMEMORY;
    }
#else
    pstr->uType = STRRET_CSTR;
    int cch = lstrlen(pszString);
    cch = min(cch, ARRAYLEN(pstr->cStr) - 1);
    strncpy(pstr->cStr, pszString, cch);
    pszString[cch] = '\0';
#endif

    return S_OK;
}


VOID
FillSpecialID(
    LPIDSHARE pids,
    BYTE bFlags,        // SHID_SHARE_*
    UINT idsName
    )
{
    WCHAR szBuf[MAX_PATH];
    szBuf[0] = L'\0';
    LoadString(g_hInstance, idsName, szBuf, ARRAYLEN(szBuf));
    LPWSTR pszName    = szBuf;
    USHORT nameLength = (USHORT)lstrlen(pszName);
    USHORT nameOffset = 0;

    pids->bFlags      = bFlags;
    pids->bReserved   = 0;
    pids->maxUses     = 0xffffffff;     // bogus

    // we don't store nameOffset
    pids->oComment = 0xffff;            // bogus
    pids->oPath    = 0xffff;            // bogus

    lstrcpy(&pids->cBuf[nameOffset],    pszName);

    pids->cb = offsetof(IDSHARE, cBuf)
               + (nameLength + 1) * sizeof(WCHAR);

    //
    // null terminate pidl
    //

    *(USHORT *)((LPBYTE)pids + pids->cb) = 0;
}

VOID
FillID1(
    LPIDSHARE pids,
    LPSHARE_INFO_1 pInfo
    )
{
    LPWSTR pszName    = pInfo->shi1_netname;
    LPWSTR pszComment = pInfo->shi1_remark;

    USHORT  nameLength, commentLength;
    USHORT  nameOffset, commentOffset;

    nameLength    = (USHORT)lstrlen(pszName);
    commentLength = (USHORT)lstrlen(pszComment);

    nameOffset    = 0;
    commentOffset = nameOffset + nameLength + 1;

    pids->bFlags      = SHID_SHARE_1;
    pids->bReserved   = 0;
    pids->type        = pInfo->shi1_type;
    pids->maxUses     = 0xffffffff;     // bogus

    // we don't store nameOffset
    pids->oComment = commentOffset;
    pids->oPath    = 0xffff;            // bogus

    lstrcpy(&pids->cBuf[nameOffset],    pszName);
    lstrcpy(&pids->cBuf[commentOffset], pszComment);

    pids->cb = offsetof(IDSHARE, cBuf)
               + (nameLength + 1 + commentLength + 1) * sizeof(WCHAR);

    //
    // null terminate pidl
    //

    *(USHORT *)((LPBYTE)pids + pids->cb) = 0;
}


VOID
FillID2(
    LPIDSHARE pids,
    LPSHARE_INFO_2 pInfo
    )
{
    LPWSTR pszName    = pInfo->shi2_netname;
    LPWSTR pszComment = pInfo->shi2_remark;
    LPWSTR pszPath    = pInfo->shi2_path;

    USHORT  nameLength, commentLength, pathLength;
    USHORT  nameOffset, commentOffset, pathOffset;

    nameLength    = (USHORT)lstrlen(pszName);
    commentLength = (USHORT)lstrlen(pszComment);
    pathLength    = (USHORT)lstrlen(pszPath);

    nameOffset    = 0;
    commentOffset = nameOffset + nameLength + 1;
    pathOffset    = commentOffset + commentLength + 1;

    pids->bFlags      = SHID_SHARE_2;
    pids->bReserved   = 0;
    pids->type        = pInfo->shi2_type;
    pids->maxUses     = pInfo->shi2_max_uses;

    // we don't store nameOffset
    pids->oComment = commentOffset;
    pids->oPath    = pathOffset;

    lstrcpy(&pids->cBuf[nameOffset],    pszName);
    lstrcpy(&pids->cBuf[commentOffset], pszComment);
    lstrcpy(&pids->cBuf[pathOffset],    pszPath);

    pids->cb = offsetof(IDSHARE, cBuf)
               + (nameLength + 1 + commentLength + 1 + pathLength + 1) * sizeof(WCHAR);

    //
    // null terminate pidl
    //

    *(USHORT *)((LPBYTE)pids + pids->cb) = 0;
}


VOID
StrNCopy(
    OUT LPWSTR pszTarget,
    IN LPCWSTR pszSource,
    IN DWORD cchTarget
    )
{
    DWORD cch = lstrlen(pszSource) + 1;
    cch = min(cch, cchTarget);
    wcsncpy(pszTarget, pszSource, cch - 1);
    pszTarget[cch - 1] = TEXT('\0');
}



//+---------------------------------------------------------------------------
//
//  Function:   TrimLeadingAndTrailingSpaces
//
//  Synopsis:   Trims the leading and trailing spaces from a null-terminated string.
//              Used primarily for share names.
//
//  History:    18-Jul-97 JonN      Created
//
//----------------------------------------------------------------------------

VOID
TrimLeadingAndTrailingSpaces(
    IN OUT PWSTR psz
	)
{
	int cchStrlen = ::wcslen(psz);
	int cchLeadingSpaces = 0;
	int cchTrailingSpaces = 0;
	while (L' ' == psz[cchLeadingSpaces])
		cchLeadingSpaces++;
	if (cchLeadingSpaces < cchStrlen)
	{
		while (L' ' == psz[cchStrlen-(cchTrailingSpaces+1)])
			cchTrailingSpaces++;
	}
	if ((cchLeadingSpaces+cchTrailingSpaces) > 0)
	{
		cchStrlen -= (cchLeadingSpaces+cchTrailingSpaces);
		(void)memmove( psz,
		               psz+cchLeadingSpaces,
					   cchStrlen*sizeof(WCHAR) );
		psz[cchStrlen] = L'\0';
	}
}
