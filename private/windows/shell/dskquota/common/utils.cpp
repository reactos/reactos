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
#include "pch.h" // PCH
#pragma hdrstop

#include "resource.h"
#include "dskquota.h"
#include <advpub.h>         // For REGINSTALL


//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


///////////////////////////////////////////////////////////////////////////////
/*  Function: SidToString

    Description: Format a SID as a character string suitable for character
        output.  This code was taken from MSDN KB article Q131320.

    Arguments:
        pSid - Address of SID to format.

        pszSid - Address of output buffer for formatted SID.

    Returns:
        TRUE    - Success.
        FALSE   - Destination buffer too small, invalid SID or pSid == NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/07/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL SidToString(
    PSID pSid,
    LPTSTR pszSid,
    LPDWORD pcchBuffer
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD cchSid;

    //
    // test if Sid passed in is valid
    //
    if(NULL == pSid || !IsValidSid(pSid))
        return FALSE;

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSid = (15 + 12 + (12 * dwSubAuthorities) + 1);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if (*pcchBuffer < cchSid)
    {
        *pcchBuffer = cchSid;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSid = wsprintf(pszSid, TEXT("S-%lu-"), dwSidRev );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        cchSid += wsprintf(pszSid + lstrlen(pszSid),
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    }
    else
    {
        cchSid += wsprintf(pszSid + lstrlen(pszSid),
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        cchSid += wsprintf(pszSid + cchSid, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: SidToString

    Description: Format a SID as a character string suitable for character
        output.  Allocates the destination buffer so that the caller must free
        it when done with it.

    Arguments:
        pSid - Address of SID to format.

        ppszSid - Address of LPTSTR variable to receive address of formatted
            SID string.  If the function returns TRUE, the caller must free
            this memory when done with it.

    Returns:
        TRUE    - Success.
        FALSE   - Destination buffer too small, invalid SID or pSid == NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/07/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL SidToString(
    PSID pSid,
    LPTSTR *ppszSid
    )
{
    DWORD cchSid = 0;

    //
    // Call once to get required buffer size.
    //
    SidToString(pSid, *ppszSid, &cchSid);

    *ppszSid = new TCHAR[cchSid];
    return SidToString(pSid, *ppszSid, &cchSid);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: CreateSidList

    Description: Creates a structure required for the SID list argument to
        NtQueryQuotaInformationFile.  The caller passes the address of an
        array of SID pointers.  The function allocates an sufficient array
        and creates the following formatted structure:

        +--------+--------+--------+--------+--------+--------+-+
        | SID[0] | SID[1] | SID[2] |        |        |SID[n-1]|0|
        +--------+--------+--------+--------+--------+--------+-+
                 |        |
                 |        |
                /          \
              /              -------------------------------
            /                                                \
          /                                                    \
        +------------+------------+-----------------------------+
        | Next entry | SID length |          SID                |
        |   offset   |  (DWORD)   |   (variable length)         |
        |   (DWORD)  |            |                             |
        +------------+------------+-----------------------------+


    Arguments:
        rgpSids - Array of SID pointers.

        cpSids - Number of pointers in rgpSids.  If 0, the array must
            contain a terminating NULL pointer.

        ppSidList - Address of a PSIDLIST pointer variable to receive
            the address of the final structure.  The caller is reponsible
            for deleting the returned buffer using "delete".

        pcbSidList - Address of DWORD varible to receive the byte count
            for the returned SidList structure.  If the function returns
            hresult ERROR_INVALID_SID, the index in the source array of the invalid
            SID will be returned at this location.

    Returns:
        NO_ERROR            - Success.
        ERROR_INVALID_SID (hr)  - An invalid SID was found in rgpSids.  The
            index of the invalid SID is returned in *pcbSidList.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/13/96    Initial creation.                                    BrianAu
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
CreateSidList(
    PSID *rgpSids,
    DWORD cpSids,
    PSIDLIST *ppSidList,
    LPDWORD pcbSidList
    )
{
    HRESULT hResult = NO_ERROR;
    DBGASSERT((NULL != rgpSids));
    DBGASSERT((NULL != ppSidList));
    DBGASSERT((NULL != pcbSidList));

    DWORD cbBuffer = 0;
    PBYTE pbBuffer = NULL;

    //
    // Initialize return values.
    //
    *ppSidList  = NULL;
    *pcbSidList = 0;

    //
    // If caller passed 0 for cpSids, list is NULL-terminated.
    // Set cpSids to a large value so it is not a factor in controlling the
    // byte-counter loop.
    //
    if (0 == cpSids)
        cpSids = (DWORD)~0;

    //
    // Count how many bytes we'll need to create the SID list.
    // Note that a NULL SID pointer at any array location
    // will truncate all following SIDs from the final list.  Just like strncpy
    // with character strings.
    //
    for (UINT i = 0; NULL != rgpSids[i] && i < cpSids; i++)
    {
        if (IsValidSid(rgpSids[i]))
        {
            cbBuffer += (sizeof(DWORD) + sizeof(DWORD) + GetLengthSid(rgpSids[i]));
        }
        else
        {
            //
            // Tell caller they passed a ptr to an invalid SID and also tell them
            // which one it was.
            //
            hResult = HRESULT_FROM_WIN32(ERROR_INVALID_SID);
            *pcbSidList = i;
            break;
        }
    }
    //
    // Reset cpSids to the actual number of SIDs processed.
    //
    cpSids = i;

    if (SUCCEEDED(hResult))
    {
        //
        // Got a good byte count and all SIDs are valid.
        //
        DBGASSERT((0 < cpSids));

        pbBuffer = new BYTE [cbBuffer];  // Can throw OutOfMemory.

        PFILE_GET_QUOTA_INFORMATION pfgqi = NULL;
        DWORD cbRecord = 0;
        DWORD cbSid    = 0;

        //
        // Return buffer address and length to caller.
        //
        *ppSidList  = (PSIDLIST)pbBuffer;
        *pcbSidList = cbBuffer;

        for (UINT i = 0; i < cpSids; i++)
        {
            pfgqi = (PFILE_GET_QUOTA_INFORMATION)pbBuffer;

            DBGASSERT((0 == ((DWORD_PTR)pfgqi & 3)));  // record is DWORD aligned?

            //
            // Calculate offsets and sizes for this entry.
            //
            cbSid    = GetLengthSid(rgpSids[i]);
            cbRecord = sizeof(pfgqi->NextEntryOffset) +
                       sizeof(pfgqi->SidLength) +
                       cbSid;
            //
            // Write the entry information.
            // On last entry, NextEntryOffset is 0.
            //
            if (i < (cpSids - 1))
                pfgqi->NextEntryOffset = cbBuffer + cbRecord;
            else
                pfgqi->NextEntryOffset = 0;

            pfgqi->SidLength       = cbSid;
            CopyMemory(&(pfgqi->Sid), rgpSids[i], cbSid);

            pbBuffer += cbRecord;   // Advance write buffer pointer.
        }
    }

    return hResult;
}




///////////////////////////////////////////////////////////////////////////////
/*  Function: MessageBoxNYI

    Description: Simple message box for unimplemented features.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID MessageBoxNYI(VOID)
{
    MessageBox(NULL,
               TEXT("This feature has not been implemented."),
               TEXT("Under Construction"),
               MB_ICONWARNING | MB_OK);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaMsgBox

    Description: Several overloaded functions for displaying messages.
        The variations allow the caller to provide either string resource
        IDs or text strings as arguments.

    Arguments:

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT DiskQuotaMsgBox(
    HWND hWndParent,
    UINT idMsgText,
    UINT idMsgTitle,
    UINT uType
    )
{
    INT iReturn     = 0;

    CString strTitle(g_hInstDll, idMsgTitle);
    CString strText(g_hInstDll, idMsgText);

    iReturn = MessageBox(hWndParent, strText, strTitle, MB_SETFOREGROUND | uType);

    return iReturn;
}


INT DiskQuotaMsgBox(
    HWND hWndParent,
    LPCTSTR pszText,
    LPCTSTR pszTitle,
    UINT uType
    )
{
    return MessageBox(hWndParent, pszText, pszTitle, MB_SETFOREGROUND | uType);
}


INT DiskQuotaMsgBox(
    HWND hWndParent,
    UINT idMsgText,
    LPCTSTR pszTitle,
    UINT uType
    )
{
    INT iReturn    = 0;

    CString strText(g_hInstDll, idMsgText);

    iReturn = MessageBox(hWndParent, strText, pszTitle, MB_SETFOREGROUND | uType);

    return iReturn;
}



INT DiskQuotaMsgBox(
    HWND hWndParent,
    LPCTSTR pszText,
    UINT idMsgTitle,
    UINT uType
    )
{
    LPTSTR pszTitle = NULL;
    INT iReturn     = 0;

    CString strTitle(g_hInstDll, idMsgTitle);

    iReturn = MessageBox(hWndParent, pszText, strTitle, MB_SETFOREGROUND | uType);

    return iReturn;
}

//
// Center a popup window in it's parent.
// If hwndParent is NULL, the window's parent is used.
// If hwndParent is not NULL, hwnd is centered in it.
// If hwndParent is NULL and hwnd doesn't have a parent, it is centered
// on the desktop.
//
VOID
CenterPopupWindow(
    HWND hwnd,
    HWND hwndParent
    )
{
    RECT rcScreen;

    if (NULL != hwnd)
    {
        rcScreen.left   = rcScreen.top = 0;
        rcScreen.right  = GetSystemMetrics(SM_CXSCREEN);
        rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);

        if (NULL == hwndParent)
        {
            hwndParent = GetParent(hwnd);
            if (NULL == hwndParent)
                hwndParent = GetDesktopWindow();
        }

        RECT rcWnd;
        RECT rcParent;

        GetWindowRect(hwnd, &rcWnd);
        GetWindowRect(hwndParent, &rcParent);

        INT cxWnd    = rcWnd.right  - rcWnd.left;
        INT cyWnd    = rcWnd.bottom - rcWnd.top;
        INT cxParent = rcParent.right  - rcParent.left;
        INT cyParent = rcParent.bottom - rcParent.top;
        POINT ptParentCtr;

        ptParentCtr.x = rcParent.left + (cxParent / 2);
        ptParentCtr.y = rcParent.top  + (cyParent / 2);

        if ((ptParentCtr.x + (cxWnd / 2)) > rcScreen.right)
        {
            //
            // Window would run off the right edge of the screen.
            //
            rcWnd.left = rcScreen.right - cxWnd;
        }
        else if ((ptParentCtr.x - (cxWnd / 2)) < rcScreen.left)
        {
            //
            // Window would run off the left edge of the screen.
            //
            rcWnd.left = rcScreen.left;
        }
        else
        {
            rcWnd.left = ptParentCtr.x - (cxWnd / 2);
        }

        if ((ptParentCtr.y + (cyWnd / 2)) > rcScreen.bottom)
        {
            //
            // Window would run off the bottom edge of the screen.
            //
            rcWnd.top = rcScreen.bottom - cyWnd;
        }
        else if ((ptParentCtr.y - (cyWnd / 2)) < rcScreen.top)
        {
            //
            // Window would run off the top edge of the screen.
            //
            rcWnd.top = rcScreen.top;
        }
        else
        {
            rcWnd.top = ptParentCtr.y - (cyWnd / 2);
        }

        MoveWindow(hwnd, rcWnd.left, rcWnd.top, cxWnd, cyWnd, TRUE);
    }
}


//
// Duplicate a string.
//
LPTSTR StringDup(
    LPCTSTR pszSource
    )
{
    LPTSTR pszNew = new TCHAR[lstrlen(pszSource) + 1];
    lstrcpy(pszNew, pszSource);

    return pszNew;
}


//
// Duplicate a SID.
//
PSID SidDup(
    PSID pSid
    )
{
    DBGASSERT((IsValidSid(pSid)));
    DWORD cbSid = GetLengthSid(pSid);

    PSID pCopy = new BYTE [cbSid];

    CopySid(cbSid, pCopy, pSid);
    return pCopy;
}


//
// Similar to Win32's GetDlgItemText except that this one
// doesn't require you to anticipate the required size of the buffer.
//
void
GetDialogItemText(
    HWND hwnd,
    UINT idCtl,
    CString *pstrText
    )
{
    DBGASSERT((NULL != pstrText));
    HWND hwndCtl = GetDlgItem(hwnd, idCtl);
    if (NULL != hwndCtl)
    {
        int cch = (int)SendMessage(hwndCtl, WM_GETTEXTLENGTH, 0, 0) + 1;
        SendMessage(hwndCtl, WM_GETTEXT, (WPARAM)cch, (LPARAM)pstrText->GetBuffer(cch));
        pstrText->ReleaseBuffer();
    }
}


BOOL
UserIsAdministrator(
    PDISKQUOTA_USER pUser
    )
{
    DBGASSERT((NULL != pUser));

    BYTE Sid[MAX_SID_LEN];
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID pAdminSid               = NULL;
    BOOL bResult                 = FALSE;

    if (AllocateAndInitializeSid(&sia,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &pAdminSid))
    {
        if (SUCCEEDED(pUser->GetSid(Sid, sizeof(Sid))))
        {
            bResult = EqualSid(Sid, pAdminSid);
        }
        FreeSid(pAdminSid);
    }

    return bResult;
}


//
// Call ADVPACK for the given section of our resource based INF.
//
// hInstance  - Resource instance containing REGINST section.
// pszSection - Name of section to invoke.
//
HRESULT
CallRegInstall(
    HINSTANCE hInstance,
    LPSTR pszSection
    )
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

#ifdef UNICODE
        if ( pfnri )
        {
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(hInstance, pszSection, &stReg);
        }
#else
        if (pfnri)
        {
            hr = pfnri(hInstance, pszSection, NULL);
        }

#endif
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}


