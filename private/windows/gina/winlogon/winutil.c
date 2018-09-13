/****************************** Module Header ******************************\
* Module Name: winutil.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements windows specific utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"

#if DBG
char *  DesktopNames[] = {"Winlogon", "ScreenSaver", "Application", "[Previous]"};
#define DbgGetDesktopName(x)    (x < (sizeof(DesktopNames) / sizeof(char *)) ? DesktopNames[x] : "Unknown!")
#endif


HDESK
GetActiveDesktop(
    PTERMINAL           pTerm,
    BOOL *              pCloseWhenDone,
    BOOL *              pLocked)
{
    HDESK           hDesk;
    PWINDOWSTATION  pWS = pTerm->pWinStaWinlogon;
    ActiveDesktops  Desktop;

    Desktop = pWS->ActiveDesktop;
    if (Desktop == -1)
    {
        Desktop = pWS->PreviousDesktop;
    }

    switch ( Desktop )
    {
        case Desktop_Application:
        {    
            hDesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
            *pCloseWhenDone = TRUE;
            *pLocked = FALSE;
            break;
        }

        case Desktop_Winlogon:
            hDesk = pWS->hdeskWinlogon;
            *pCloseWhenDone = FALSE;
            *pLocked = TRUE;
            break;

        case Desktop_ScreenSaver:
            hDesk = pWS->hdeskScreenSaver;
            *pCloseWhenDone = FALSE;
            *pLocked = FALSE;
            break;

        default:
            DebugLog((DEB_TRACE, "Unknown desktop: %d\n", Desktop));
            *pCloseWhenDone = FALSE;
            hDesk = NULL;
            *pLocked = FALSE;
            break;

    }

    return(hDesk);
}

BOOL
SetReturnDesktop(
    PWINDOWSTATION  pWS,
    PWLX_DESKTOP    pDesktop)
{
    WCHAR   DesktopName[TYPICAL_STRING_LENGTH];
    DWORD   Needed;
    PWSTR   pszDesktop;
    BOOL    FreeDesktopString = FALSE;
    HDESK   hDesk;

    if ( pWS->ActiveDesktop == Desktop_Application )
    {
        return( FALSE );
    }

    if ( (pDesktop->Flags & WLX_DESKTOP_NAME) == 0 )
    {
        if (!GetUserObjectInformation( pDesktop->hDesktop,
                                UOI_NAME,
                                DesktopName,
                                TYPICAL_STRING_LENGTH,
                                &Needed ) )
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            {
                return( FALSE );
            }

            pszDesktop = LocalAlloc( LMEM_FIXED, Needed );

            if ( !pszDesktop )
            {
                return( FALSE );
            }

            GetUserObjectInformation(  pDesktop->hDesktop,
                                UOI_NAME,
                                pszDesktop,
                                Needed,
                                &Needed );

            FreeDesktopString = TRUE;

        }
        else
        {
            pszDesktop = DesktopName;

            FreeDesktopString = FALSE;
        }

    }
    else
    {
        pszDesktop = pDesktop->pszDesktopName;

        FreeDesktopString = FALSE;

    }

    hDesk = OpenDesktop( pszDesktop, 0, FALSE, MAXIMUM_ALLOWED );

    if (!hDesk)
    {
        if (FreeDesktopString)
        {
            LocalFree( pszDesktop );
        }
        return( FALSE );
    }

    CloseDesktop( pWS->hdeskPrevious );
    pWS->hdeskPrevious = hDesk;

    if (FreeDesktopString)
    {
        LocalFree( pszDesktop );
    }

    return( TRUE );

}

BOOL
SetActiveDesktop(
    PTERMINAL       pTerm,
    ActiveDesktops  Desktop)
{
    HDESK           hDesk;
    HDESK           hPrevious;
    DWORD           LengthNeeded;
    PWINDOWSTATION  pWS = pTerm->pWinStaWinlogon;

    if (Desktop == pWS->ActiveDesktop)
    {
        return(TRUE);
    }

    if (pWS->ActiveDesktop == Desktop_Application)
    {
        LockWindowStation(pWS->hwinsta);
        pWS->hdeskPrevious = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);

        if (!GetUserObjectInformation(  pWS->hdeskPrevious,
                                        UOI_NAME,
                                        pTerm->pszDesktop,
                                        pTerm->DesktopLength,
                                        &LengthNeeded) )
        {
            if (pTerm->DesktopLength != TYPICAL_STRING_LENGTH &&
                pTerm->DesktopLength != 0)
            {
                LocalFree( pTerm->pszDesktop );
                pTerm->pszDesktop = NULL;
                pTerm->DesktopLength = 0;
            }
            pTerm->pszDesktop = LocalAlloc( LMEM_FIXED, LengthNeeded );
            if (pTerm->pszDesktop)
            {
                pTerm->DesktopLength = LengthNeeded;

                if (!GetUserObjectInformation(  pWS->hdeskPrevious,
                                            UOI_NAME,
                                            pTerm->pszDesktop,
                                            pTerm->DesktopLength,
                                            &LengthNeeded))
                {
                    pTerm->pszDesktop[0] = 0;
                }
            }
            else
            {
                pTerm->DesktopLength = 0;
            }
        }

        DebugLog((DEB_TRACE, "Source desktop was %ws\n", pTerm->pszDesktop));
    }

    switch (Desktop)
    {
        case Desktop_Winlogon:
            hDesk = pWS->hdeskWinlogon;
            break;
        case Desktop_ScreenSaver:
            hDesk = pWS->hdeskScreenSaver;
            break;
        case Desktop_Application:
            if (pWS->hdeskPrevious)
            {
                hDesk = pWS->hdeskPrevious;
            }
            else
            {
                hDesk = pWS->hdeskApplication;
            }
            break;
        default:
            DebugLog((DEB_ERROR, "Error:  Invalid desktop specified %d\n", Desktop));
            return(FALSE);
    }
    if (SwitchDesktop(hDesk))
    {
        DebugLog((DEB_TRACE, "Switching desktop from %s to %s\n",
                    DbgGetDesktopName(pWS->ActiveDesktop),
                    DbgGetDesktopName(Desktop) ));

        pWS->PreviousDesktop = pWS->ActiveDesktop;
        pWS->ActiveDesktop = Desktop;

        //
        // If we're switching back to the user's desktop, then unlock the
        // window station, so that the user can switch desktops again.  Also,
        // close our handle to the desktop.  Note!  Unlock before close, so
        // that if this is the last handle to the desktop, cleanup can occur
        // correctly.
        //

        if (pWS->ActiveDesktop == Desktop_Application)
        {
            UnlockWindowStation(pWS->hwinsta);
            if (pWS->hdeskPrevious)
            {
                DebugLog((DEB_TRACE, "Closing handle %x to users desktop\n", pWS->hdeskPrevious));
                CloseDesktop(pWS->hdeskPrevious);
                pWS->hdeskPrevious = NULL;
            }
        }


        return(TRUE);
    }
    DebugLog((DEB_WARN, "Could not switch desktop!\n"));
    return(FALSE);
}


/***************************************************************************\
* FUNCTION: AllocAndGetPrivateProfileString
*
* PURPOSE:  Allocates memory for and returns pointer to a copy of the
*           specified profile string
*           The returned string should be freed using Free()
*
* RETURNS:  Pointer to copy of profile string or NULL on failure.
*
* HISTORY:
*
*  12-Nov-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndGetPrivateProfileString(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    LPCTSTR lpDefault,
    LPCTSTR lpFileName
    )
{
    LPTSTR String;
    LPTSTR NewString ;
    LONG LengthAllocated;
    LONG LengthCopied;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = TYPICAL_STRING_LENGTH;

    String = Alloc(LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        DebugLog((DEB_ERROR, "AllocAndGetPrivateProfileString : Failed to allocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        LengthCopied = GetPrivateProfileString( lpAppName,
                                                lpKeyName,
                                                lpDefault,
                                                String,
                                                LengthAllocated,
                                                lpFileName
                                              );
        //
        // If the returned value is our passed size - 1 (weird way for error)
        // then our buffer is too small. Make it bigger and start over again.
        //

        if (LengthCopied == (LengthAllocated - 1)) {

            DebugLog((DEB_TRACE, "AllocAndGetPrivateProfileString: Failed with buffer length = %d, reallocating and retrying", LengthAllocated));

            LengthAllocated *= 2;
            NewString = ReAlloc(String, LengthAllocated * sizeof(TCHAR));
            if (NewString == NULL) {
                DebugLog((DEB_ERROR, "AllocAndGetPrivateProfileString : Failed to reallocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
                Free( String );
                String = NULL ;
                break;
            }

            String = NewString ;

            //
            // Go back and try to read it again
            //

        } else {

            //
            // Success!
            //

            break;
        }

    }

    return(String);
}


/***************************************************************************\
* FUNCTION: WritePrivateProfileInt
*
* PURPOSE:  Writes out an integer to a profile file
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*  12-Nov-92 Davidc       Created.
*
\***************************************************************************/

BOOL
WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    UINT Value,
    LPCTSTR lpFileName
    )
{
    NTSTATUS Status;
    TCHAR String[30];
    UNICODE_STRING UniString;

    UniString.MaximumLength = 30;
    UniString.Buffer = String;

    Status = RtlIntegerToUnicodeString(Value,10,&UniString);

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    return (WritePrivateProfileString(lpAppName, lpKeyName, UniString.Buffer, lpFileName));

}


/***************************************************************************\
* FUNCTION: AllocAndExpandEnvironmentStrings
*
* PURPOSE:  Allocates memory for and returns pointer to buffer containing
*           the passed string expanded to include environment strings
*           The returned buffer should be freed using Free()
*
* RETURNS:  Pointer to expanded string or NULL on failure.
*
* HISTORY:
*
*  21-Dec-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndExpandEnvironmentStrings(
    LPCTSTR lpszSrc
    )
{
    LPTSTR String;
    LPTSTR NewString ;
    LONG LengthAllocated;
    LONG LengthCopied;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = lstrlen(lpszSrc) + TYPICAL_STRING_LENGTH;

    String = Alloc(LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        DebugLog((DEB_ERROR, "AllocAndExpandEnvironmentStrings : Failed to allocate %d bytes for string\n", LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        LengthCopied = ExpandEnvironmentStrings( lpszSrc,
                                                 String,
                                                 LengthAllocated
                                               );
        if (LengthCopied == 0) {
            DebugLog((DEB_ERROR, "AllocAndExpandEnvironmentStrings : ExpandEnvironmentStrings failed, error = %d\n", GetLastError()));
            Free(String);
            String = NULL;
            break;
        }

        //
        // If the buffer was too small, make it bigger and try again
        //

        if (LengthCopied > LengthAllocated) {

            DebugLog((DEB_TRACE, "AllocAndExpandEnvironmentStrings: Failed with buffer length = %d, reallocating to %d and retrying (retry should succeed)\n", LengthAllocated, LengthCopied));

            NewString = ReAlloc(String, LengthCopied * sizeof(TCHAR));
            LengthAllocated = LengthCopied;
            if (NewString == NULL) {
                DebugLog((DEB_ERROR, "AllocAndExpandEnvironmentStrings : Failed to reallocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
                Free( String );
                String = NULL ;
                break;
            }

            String = NewString ;

            //
            // Go back and try to expand the string again
            //

        } else {

            //
            // Success!
            //

            break;
        }

    }

    return(String);
}


/***************************************************************************\
* FUNCTION: AllocAndRegEnumKey
*
* PURPOSE:  Allocates memory for and returns pointer to buffer containing
*           the next registry sub-key name under the specified key
*           The returned buffer should be freed using Free()
*
* RETURNS:  Pointer to sub-key name or NULL on failure. The reason for the
*           error can be obtains using GetLastError()
*
* HISTORY:
*
*  21-Dec-92 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndRegEnumKey(
    HKEY hKey,
    DWORD iSubKey
    )
{
    LPTSTR String;
    LPTSTR NewString ;
    LONG LengthAllocated;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    LengthAllocated = TYPICAL_STRING_LENGTH;

    String = Alloc(LengthAllocated * sizeof(TCHAR));
    if (String == NULL) {
        DebugLog((DEB_ERROR, "AllocAndRegEnumKey : Failed to allocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
        return(NULL);
    }

    while (TRUE) {

        DWORD Error = RegEnumKey(hKey, iSubKey, String, LengthAllocated);
        if (Error == ERROR_SUCCESS) {
            break;
        }

        if (Error != ERROR_MORE_DATA) {

            if (Error != ERROR_NO_MORE_ITEMS) {
                DebugLog((DEB_ERROR, "AllocAndRegEnumKey : RegEnumKey failed, error = %d", Error));
            }

            Free(String);
            String = NULL;
            SetLastError(Error);
            break;
        }

        //
        // The buffer was too small, make it bigger and try again
        //

        DebugLog((DEB_TRACE, "AllocAndRegEnumKey: Failed with buffer length = %d, reallocating and retrying", LengthAllocated));

        LengthAllocated *= 2;
        NewString = ReAlloc(String, LengthAllocated * sizeof(TCHAR));
        if (NewString == NULL) {
            DebugLog((DEB_ERROR, "AllocAndRegEnumKey : Failed to reallocate %d bytes for string", LengthAllocated * sizeof(TCHAR)));
            Free( String );
            String = NULL ;
            break;
        }

        String = NewString ;
    }

    return(String);
}


/***************************************************************************\
* FUNCTION: AllocAndRegQueryValueEx
*
* PURPOSE:  Version of RegQueryValueEx that returns value in allocated buffer.
*           The returned buffer should be freed using Free()
*
* RETURNS:  Pointer to key value or NULL on failure. The reason for the
*           error can be obtains using GetLastError()
*
* HISTORY:
*
*  15-Jan-93 Davidc       Created.
*
\***************************************************************************/

LPTSTR
AllocAndRegQueryValueEx(
    HKEY hKey,
    LPTSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType
    )
{
    LPTSTR String;
    LPTSTR NewString ;
    DWORD BytesAllocated;

    //
    // Pick a random buffer length, if it's not big enough reallocate
    // it and try again until it is.
    //

    BytesAllocated = TYPICAL_STRING_LENGTH * sizeof(TCHAR);

    String = Alloc(BytesAllocated);
    if (String == NULL) {
        DebugLog((DEB_ERROR, "AllocAndRegQueryValueEx : Failed to allocate %d bytes for string", BytesAllocated));
        return(NULL);
    }

    while (TRUE) {

        DWORD Error;
        DWORD BytesReturned = BytesAllocated;

        Error = RegQueryValueEx(hKey,
                                lpValueName,
                                lpReserved,
                                lpType,
                                (LPBYTE)String,
                                &BytesReturned);
        if (Error == ERROR_SUCCESS) {
            break;
        }

        if (Error != ERROR_MORE_DATA) {

            DebugLog((DEB_ERROR, "AllocAndRegQueryValueEx : RegQueryValueEx failed, error = %d", Error));
            Free(String);
            String = NULL;
            SetLastError(Error);
            break;
        }

        //
        // The buffer was too small, make it bigger and try again
        //

        DebugLog((DEB_TRACE, "AllocAndRegQueryValueEx: Failed with buffer length = %d bytes, reallocating and retrying", BytesAllocated));

        BytesAllocated *= 2;
        NewString = ReAlloc(String, BytesAllocated);
        if (NewString == NULL) {
            DebugLog((DEB_ERROR, "AllocAndRegQueryValueEx : Failed to reallocate %d bytes for string", BytesAllocated));
            Free( String );
            String = NULL ;
            break;
        }

        String = NewString ;
    }

    return(String);
}


/***************************************************************************\
* CentreWindow
*
* Purpose : Positions a window so that it is centred in its parent
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
CentreWindow(
    HWND    hwnd
    )
{
    RECT    rect;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {

        // Return the desktop windows size (size of main screen)
        dxParent = GetSystemMetrics(SM_CXSCREEN);
        dyParent = GetSystemMetrics(SM_CYSCREEN);
    } else {
        HWND    hwndParent;
        RECT    rectParent;

        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }

        GetWindowRect(hwndParent, &rectParent);

        dxParent = rectParent.right - rectParent.left;
        dyParent = rectParent.bottom - rectParent.top;
    }

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOSIZE);

    SetForegroundWindow(hwnd);
}


/***************************************************************************\
* SetupSystemMenu
*
* Purpose : Does any manipulation required for a dialog system menu.
*           Should be called during WM_INITDIALOG processing for a dialog
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
SetupSystemMenu(
    HWND hDlg
    )
{
    // Remove the Close item from the system menu if we don't
    // have a CANCEL button

    if (GetDlgItem(hDlg, IDCANCEL) == NULL) {

        HMENU hMenu = GetSystemMenu(hDlg, FALSE);

        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    }

}

/***************************************************************************\
* FUNCTION: OpenIniFileUserMapping
*
* PURPOSE:  Forces the ini file mapping apis to reference the current user's
*           registry.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   24-Aug-92 Davidc       Created.
*
\***************************************************************************/

BOOL
OpenIniFileUserMapping(
    PTERMINAL pTerm
    )
{
    BOOL Result;
    HANDLE ImpersonationHandle;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;

    //
    // Impersonate the user
    //

    if (pTerm->IniRef == 0)
    {

        ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

        if (ImpersonationHandle == NULL) {
            DebugLog((DEB_ERROR, "OpenIniFileUserMapping failed to impersonate user\n"));
            return(FALSE);
        }
        
        DebugLog((DEB_TRACE, "Actually opening user mapping.  User %s logged on\n",
                        pTerm->UserLoggedOn ? "is" : "is not"));
        
        Result = OpenProfileUserMapping();

        if (!Result) {
            DebugLog((DEB_ERROR, "OpenProfileUserMapping failed, error = %d\n", GetLastError()));
        }

        //
        // Revert to being 'ourself'
        //

        if (!StopImpersonating(ImpersonationHandle)) {
            DebugLog((DEB_ERROR, "OpenIniFileUserMapping failed to revert to self\n"));
        }
    }
    else
    {
        Result = TRUE;
    }

    pTerm->IniRef++;

    DebugLog((DEB_TRACE, "ProfileUserMapping Refs = %d\n", pTerm->IniRef));

    return(Result);
}

/***************************************************************************\
* FUNCTION: CloseIniFileUserMapping
*
* PURPOSE:  Closes the ini file mapping to the user's registry such
*           that future use of the ini apis will fail if they reference
*           the user's registry.
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   24-Aug-92 Davidc       Created.
*
\***************************************************************************/

VOID
CloseIniFileUserMapping(
    PTERMINAL pTerm
    )
{
    BOOL Result;

    if (pTerm->IniRef)
    {
        if (--pTerm->IniRef == 0)
        {

            DebugLog((DEB_TRACE, "Actually closing user mapping\n"));

            Result = CloseProfileUserMapping();

            if (!Result) {
                DebugLog((DEB_ERROR, "CloseProfileUserMapping failed, error = %d", GetLastError()));
            }

        }

    }

    DebugLog((DEB_TRACE, "ProfileUserMapping Refs = %d\n", pTerm->IniRef));
}
