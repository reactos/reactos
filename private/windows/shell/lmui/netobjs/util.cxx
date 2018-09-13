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



//+-------------------------------------------------------------------------
//
//  Function:   MessageFromError
//
//  Synopsis:   MessageFromError returns a message ID which is more or
//              less appropriate to a return code from a remoted
//              NetServerGetInfo.
//
//  History:    26-Sep-95   BruceFo     Stolen from Win95
//
//--------------------------------------------------------------------------

HRESULT
MessageFromError(
    NET_API_STATUS err
    )
{
    switch (err)
    {
        case ERROR_ACCESS_DENIED:
        case ERROR_NETWORK_ACCESS_DENIED:
            return MSG_ACCESSDENIED;
        case ERROR_BAD_NETPATH:
            return MSG_SERVERNOTFOUND;
        default:
            return MSG_CANTREMOTE;
    }

    return 0;
}


//----------------------------------------------------------------------------

CWaitCursor::CWaitCursor(UINT idResCursor)
{
    _hcurWait = _hcurOld = NULL;

    if (0 != idResCursor)
    {
        _hcurWait = LoadCursor(g_hInstance, MAKEINTRESOURCE(idResCursor));
        _hcurOld = SetCursor(_hcurWait);
    }
    else
    {
        _hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    }
}

CWaitCursor::~CWaitCursor()
{
    ::SetCursor( _hcurOld );
    if (_hcurWait)
    {
        ::DestroyCursor( _hcurWait );
    }
}

//----------------------------------------------------------------------------

NET_API_STATUS
MyNetpGetDomainNameEx (
    IN  LPWSTR MachineName,
    OUT LPWSTR* DomainNamePtr, // alloc and set ptr (free with NetApiBufferFree)
    OUT PBOOL IsWorkgroupName
    )

/*++

Routine Description:

    Returns the name of the domain or workgroup this machine belongs to.
    Stolen from netlib and hacked to remote it.

Arguments:

    MachineName - The machine in question

    DomainNamePtr - The name of the domain or workgroup

    IsWorkgroupName - Returns TRUE if the name is a workgroup name.
        Returns FALSE if the name is a domain name.

Return Value:

   NERR_Success - Success.
   NERR_CfgCompNotFound - There was an error determining the domain name

--*/
{
    NET_API_STATUS status;
    NTSTATUS ntstatus;
    LSA_HANDLE PolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO PrimaryDomainInfo;
    OBJECT_ATTRIBUTES ObjAttributes;
    UNICODE_STRING unicodeMachineName;

    //
    // Check for caller's errors.
    //
    if (DomainNamePtr == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Open a handle to the machine's security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    RtlInitUnicodeString(&unicodeMachineName, MachineName);

    ntstatus = LsaOpenPolicy(
                   &unicodeMachineName,
                   &ObjAttributes,
                   POLICY_VIEW_LOCAL_INFORMATION,
                   &PolicyHandle
                   );

    if (! NT_SUCCESS(ntstatus))
    {
        appDebugOut((DEB_ERROR,
            "NetpGetDomainName: LsaOpenPolicy returned 0x%08lx\n",
            ntstatus));
        return NERR_CfgCompNotFound;
    }

    //
    // Get the name of the primary domain from LSA
    //
    ntstatus = LsaQueryInformationPolicy(
                   PolicyHandle,
                   PolicyPrimaryDomainInformation,
                   (PVOID *) &PrimaryDomainInfo
                   );

    if (! NT_SUCCESS(ntstatus))
    {
        appDebugOut((DEB_ERROR,
            "NetpGetDomainName: LsaQueryInformationPolicy failed 0x%08lx\n",
            ntstatus));
        (void) LsaClose(PolicyHandle);
        return NERR_CfgCompNotFound;
    }

    (void) LsaClose(PolicyHandle);

    status = NetApiBufferAllocate(
                      PrimaryDomainInfo->DomainName.Length + sizeof(WCHAR),
                      (LPVOID*)DomainNamePtr);
    if (status != NERR_Success)
    {
        (void) LsaFreeMemory((PVOID) PrimaryDomainInfo);
        return status;
    }

    ZeroMemory(
        *DomainNamePtr,
        PrimaryDomainInfo->DomainName.Length + sizeof(WCHAR)
        );

    CopyMemory(
        *DomainNamePtr,
        PrimaryDomainInfo->DomainName.Buffer,
        PrimaryDomainInfo->DomainName.Length
        );

    *IsWorkgroupName = (PrimaryDomainInfo->DomainSid == NULL);

    (void) LsaFreeMemory((PVOID) PrimaryDomainInfo);

    return NERR_Success;
}
