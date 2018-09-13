/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    util.c

Abstract:

    Utility functions for System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#include "sysdm.h"

//
// Constants
//
#define CCH_MAX_DEC 12             // Number of chars needed to hold 2^32


void
ErrMemDlg(
    IN HWND hParent
)
/*++

Routine Description:

    Displays "out of memory" message.

Arguments:

    hParent -
        Supplies parent window handle.

Return Value:

    None.

--*/
{
    MessageBox(
        hParent,
        g_szErrMem,
        g_szSystemApplet,
        MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
    );
    return;
}

LPTSTR
SkipWhiteSpace(
    IN LPTSTR sz
)
/*++

Routine Description:

    SkipWhiteSpace
    For the purposes of this fuction, whitespace is space, tab,
    cr, or lf.

Arguments:

    sz -
        Supplies a string (which presumably has leading whitespace)

Return Value:

    Pointer to string without leading whitespace if successful.

--*/
{
    while( IsWhiteSpace(*sz) )
        sz++;

    return sz;
}

int 
StringToInt( 
    IN LPTSTR sz 
) 
/*++

Routine Description:

    TCHAR version of atoi

Arguments:

    sz -
        Supplies the string to convert

Return Value:

    Integer representation of the string

--*/
{
    int i = 0;

    sz = SkipWhiteSpace(sz);

    while( IsDigit( *sz ) ) {
        i = i * 10 + DigitVal( *sz );
        sz++;
    }

    return i;
}

void 
IntToString( 
    IN INT i, 
    OUT LPTSTR sz
) 
/*++

Routine Description:

    TCHAR version of itoa

Arguments:

    i -
        Supplies the integer to convert

    sz -
        Returns the string form of the supplied int

Return Value:

    None.

--*/
{
    TCHAR szTemp[CCH_MAX_DEC];
    int iChr;


    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (i % 10);
        i = i / 10;
    } while (i != 0);

    do {
        iChr--;
        *sz++ = szTemp[iChr];
    } while (iChr != 0);

    *sz++ = TEXT('\0');
}


LPTSTR 
CheckSlash(
    IN LPTSTR lpDir
)
/*++

Routine Description:

    Checks for an ending backslash and adds one if
    it is missing.

Arguments:

    lpDir -
        Supplies the name of a directory.

Return Value:

    A string that ends with a backslash.

--*/
{
    DWORD dwStrLen;
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}


BOOL 
Delnode_Recurse(
    IN LPTSTR lpDir
)
/*++

Routine Description:

    Recursive delete function for Delnode

Arguments:

    lpDir -
        Supplies directory to delete

Return Value:

    TRUE if successful.
    FALSE if an error occurs.

--*/
{
    WIN32_FIND_DATA fd;
    HANDLE hFile;

    //
    // Setup the current working dir
    //

    if (!SetCurrentDirectory (lpDir)) {
        return FALSE;
    }


    //
    // Find the first file
    //

    hFile = FindFirstFile(TEXT("*.*"), &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return TRUE;
        } else {
            return FALSE;
        }
    }


    do {
        //
        // Check for "." and ".."
        //

        if (!lstrcmpi(fd.cFileName, TEXT("."))) {
            continue;
        }

        if (!lstrcmpi(fd.cFileName, TEXT(".."))) {
            continue;
        }


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Found a directory.
            //

            if (!Delnode_Recurse(fd.cFileName)) {
                FindClose(hFile);
                return FALSE;
            }

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                fd.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes (fd.cFileName, fd.dwFileAttributes);
            }


            RemoveDirectory (fd.cFileName);


        } else {

            //
            // We found a file.  Set the file attributes,
            // and try to delete it.
            //

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
                SetFileAttributes (fd.cFileName, FILE_ATTRIBUTE_NORMAL);
            }

            DeleteFile (fd.cFileName);

        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


    //
    // Close the search handle
    //

    FindClose(hFile);


    //
    // Reset the working directory
    //

    if (!SetCurrentDirectory (TEXT(".."))) {
        return FALSE;
    }


    //
    // Success.
    //

    return TRUE;
}


BOOL 
Delnode(
    IN LPTSTR lpDir
)
/*++

Routine Description:

    Recursive function that deletes files and
    directories.

Arguments:

    lpDir -
        Supplies directory to delete.

Return Value:

    TRUE if successful
    FALSE if an error occurs

--*/
{
    TCHAR szCurWorkingDir[MAX_PATH];

    if (GetCurrentDirectory(MAX_PATH, szCurWorkingDir)) {

        Delnode_Recurse (lpDir);

        SetCurrentDirectory (szCurWorkingDir);

        if (!RemoveDirectory (lpDir)) {
            return FALSE;
        }

    } else {
        return FALSE;
    }

    return TRUE;

}

LONG 
MyRegSaveKey(
    IN HKEY hKey, 
    IN LPCTSTR lpSubKey
)
/*++

Routine Description:

    Saves a registry key.

Arguments:

    hKey -
        Supplies handle to a registry key.

    lpSubKey -
        Supplies the name of the subkey to save.

Return Value:

    ERROR_SUCCESS if successful.
    Error code from RegSaveKey() if an error occurs.

--*/
{

    HANDLE hToken;
    LUID luid;
    DWORD dwSize = 1024;
    PTOKEN_PRIVILEGES lpPrevPrivilages;
    TOKEN_PRIVILEGES tp;
    LONG error;


    //
    // Allocate space for the old privileges
    //

    lpPrevPrivilages = GlobalAlloc(GPTR, dwSize);

    if (!lpPrevPrivilages) {
        return GetLastError();
    }


    if (!OpenProcessToken( GetCurrentProcess(),
                      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
         return GetLastError();
    }

    LookupPrivilegeValue( NULL, SE_BACKUP_NAME, &luid );

    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
         dwSize, lpPrevPrivilages, &dwSize )) {

        if (GetLastError() == ERROR_MORE_DATA) {
            PTOKEN_PRIVILEGES lpTemp;

            lpTemp = GlobalReAlloc(lpPrevPrivilages, dwSize, GMEM_MOVEABLE);

            if (!lpTemp) {
                GlobalFree (lpPrevPrivilages);
                return GetLastError();
            }

            lpPrevPrivilages = lpTemp;

            if (!AdjustTokenPrivileges( hToken, FALSE, &tp,
                 dwSize, lpPrevPrivilages, &dwSize )) {
                return GetLastError();
            }

        } else {
            return GetLastError();
        }

    }

    //
    // Save the hive
    //

    error = RegSaveKey(hKey, lpSubKey, NULL);


    AdjustTokenPrivileges( hToken, FALSE, lpPrevPrivilages,
                           0, NULL, NULL );

    CloseHandle (hToken);

    return error;
}

UINT 
CreateNestedDirectory(
    IN LPCTSTR lpDirectory, 
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
)
/*++

Routine Description:

    Creates a subdirectory and all its parents
    if necessary.

Arguments:

    lpDirectory -
        Name of directory to create.

    lpSecurityAttributes -
        Desired security attributes.

Return Value:

    Nonzero on success.
    Zero on failure.

--*/
{
    TCHAR szDirectory[MAX_PATH];
    LPTSTR lpEnd;


    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        return 0;
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if (CreateDirectory (lpDirectory, lpSecurityAttributes)) {
        return 1;
    }

    //
    // If this directory exists already, this is OK too.
    //

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    lstrcpy (szDirectory, lpDirectory);


    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        //
        // Skip the first two slashes
        //

        lpEnd += 2;

        //
        // Find the slash between the server name and
        // the share name.
        //

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Skip the slash, and find the slash between
        // the share name and the directory name.
        //

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Leave pointer at the beginning of the directory.
        //

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    return 0;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    //
    // Create the final directory
    //

    if (CreateDirectory (szDirectory, lpSecurityAttributes)) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // Failed
    //

    return 0;

}

LONG 
MyRegLoadKey(
    IN HKEY hKey, 
    IN LPTSTR lpSubKey, 
    IN LPTSTR lpFile
)
/*++

Routine Description:

    Loads a hive into the registry

Arguments:

    hKey -
        Supplies a handle to a registry key which will be the parent
        of the created key.

    lpSubKey -
        Supplies the name of the subkey to create.

    lpFile -
        Supplies the name of the file containing the hive.

Return Value:

    ERROR_SUCCESS if successful.
    Error code from RegLoadKey if unsuccessful.

--*/
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    int error;

    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegLoadKey(hKey, lpSubKey, lpFile);

        //
        // Restore the privilege to its previous state
        //

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);


    } else {

        error = GetLastError();
    }

    return error;
}


LONG 
MyRegUnLoadKey(
    IN HKEY hKey, 
    IN LPTSTR lpSubKey
)
/*++

Routine Description:

    Unloads a registry key.

Arguments:

    hKey -
        Supplies handle to parent key

    lpSubKey -
        Supplies name of subkey to delete

Return Value:

    ERROR_SUCCESS if successful
    Error code if unsuccessful

--*/
{
    LONG error;
    NTSTATUS Status;
    BOOLEAN WasEnabled;


    //
    // Enable the restore privilege
    //

    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (NT_SUCCESS(Status)) {

        error = RegUnLoadKey(hKey, lpSubKey);

        //
        // Restore the privilege to its previous state
        //

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);

    } else {

        error = GetLastError();
    }

    return error;
}

int 
GetSelectedItem(
    IN HWND hCtrl
)
/*++

Routine Description:
    
    Determines which item in a list view control is selected

Arguments:

    hCtrl -
        Supplies handle to the desired list view control.

Return Value:

    The index of the selected item, if an item is selected.
    -1 if no item is selected.

--*/
{
    int i, n;

    n = (int)SendMessage (hCtrl, LVM_GETITEMCOUNT, 0, 0L);

    if (n != LB_ERR)
    {
        for (i = 0; i < n; i++)
        {
            if (SendMessage (hCtrl, LVM_GETITEMSTATE,
                             i, (LPARAM) LVIS_SELECTED) == LVIS_SELECTED) {
                return i;
            }
        }
    }

    return -1;
}

BOOL
IsUserAdmin(
    VOID
)
/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/
{
    BOOL b = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    if(AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup
        ))
    {
        CheckTokenMembership(NULL, AdministratorsGroup, &b);

        FreeSid(AdministratorsGroup);
    }

    return(b);

} // IsUserAdmin

int 
MsgBoxParam( 
    IN HWND hWnd, 
    IN DWORD wText, 
    IN DWORD wCaption, 
    IN DWORD wType, 
    ... 
)
/*++

Routine Description:

    Combination of MessageBox and printf

Arguments:

    hWnd -
        Supplies parent window handle

    wText -
        Supplies ID of a printf-like format string to display as the
        message box text

    wCaption -
        Supplies ID of a string to display as the message box caption

    wType -
        Supplies flags to MessageBox()

Return Value:

    Whatever MessageBox() returns.

--*/

{
    TCHAR   szText[ 4 * MAX_PATH ], szCaption[ 2 * MAX_PATH ];
    int     ival;
    va_list parg;

    va_start( parg, wType );

    if( wText == INITS )
        goto NoMem;

    if( !LoadString( hInstance, wText, szCaption, ARRAYSIZE( szCaption ) ) )
        goto NoMem;

    wvsprintf( szText, szCaption, parg );

    if( !LoadString( hInstance, wCaption, szCaption, ARRAYSIZE( szCaption ) ) )
        goto NoMem;

    if( (ival = MessageBox( hWnd, szText, szCaption, wType ) ) == 0 )
        goto NoMem;

    va_end( parg );

    return( ival );

NoMem:
    va_end( parg );

    ErrMemDlg( hWnd );
    return 0;
}

LPTSTR 
CloneString( 
    IN LPTSTR pszSrc 
) 
/*++

Routine Description:

    Allocates a buffer and copies a string into it

Arguments:

    pszSrc -
        Supplies string to copy

Return Value:

    Valid LPTSTR if successful
    NULL if out of memory

--*/
{
    LPTSTR pszDst = NULL;

    if (pszSrc != NULL) {
        pszDst = MemAlloc(LMEM_FIXED, (lstrlen(pszSrc)+1) * SIZEOF(TCHAR));
        if (pszDst) {
            lstrcpy( pszDst, pszSrc );
        }
    }

    return pszDst;
}

DWORD 
SetLBWidthEx(
    IN HWND hwndLB, 
    IN LPTSTR szBuffer, 
    IN DWORD cxCurWidth, 
    IN DWORD cxExtra
)
/*++

Routine Description:

    Set the width of a listbox, in pixels, acording to the size of the
    string passed in

Arguments:

    hwndLB -
        Supples listbox to resize

    szBuffer -
        Supplies string to resize listbox to

    cxCurWidth -
        Supplies current width of the listbox

    cxExtra -
        Supplies some kind of slop factor

Return Value:

    The new width of the listbox

--*/
{
    HDC     hDC;
    SIZE    Size;
    LONG    cx;
    HFONT   hfont, hfontOld;

    // Get the new Win4.0 thin dialog font
    hfont = (HFONT)SendMessage(hwndLB, WM_GETFONT, 0, 0);

    hDC = GetDC(hwndLB);

    // if we got a font back, select it in this clean hDC
    if (hfont != NULL)
        hfontOld = SelectObject(hDC, hfont);


    // If cxExtra is 0, then give our selves a little breathing space.
    if (cxExtra == 0) {
        GetTextExtentPoint(hDC, TEXT("1234"), 4 /* lstrlen("1234") */, &Size);
        cxExtra = Size.cx;
    }

    // Set scroll width of listbox

    GetTextExtentPoint(hDC, szBuffer, lstrlen(szBuffer), &Size);

    Size.cx += cxExtra;

    // Get the name length and adjust the longest name

    if ((DWORD) Size.cx > cxCurWidth)
    {
        cxCurWidth = Size.cx;
        SendMessage (hwndLB, LB_SETHORIZONTALEXTENT, (DWORD)Size.cx, 0L);
    }

    // retstore the original font if we changed it
    if (hfont != NULL)
        SelectObject(hDC, hfontOld);

    ReleaseDC(NULL, hDC);

    return cxCurWidth;
}

VOID 
SetDefButton(
    IN HWND hwndDlg,
    IN int idButton
)
/*++

Routine Description:

    Sets the default button for a dialog box or proppage
    The old default button, if any,  has its default status removed

Arguments:

    hwndDlg -
        Supplies window handle

    idButton -
        Supplies ID of button to make default

Return Value:

    None

--*/
{
    LRESULT lr;

    if (HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));

        SendMessage (hwndOldDefButton,
                     BM_SETSTYLE,
                     MAKEWPARAM(BS_PUSHBUTTON, 0),
                     MAKELPARAM(TRUE, 0));
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                 MAKELPARAM( TRUE, 0 ));
}


void 
HourGlass( 
    IN BOOL bOn 
)
/*++

Routine Description:

    Turns hourglass mouse cursor on or off

Arguments:

    bOn -
        Supplies desired status of hourglass mouse cursor

Return Value:

    None

--*/
{
    if( !GetSystemMetrics( SM_MOUSEPRESENT ) )
        ShowCursor( bOn );

    SetCursor( LoadCursor( NULL, bOn ? IDC_WAIT : IDC_ARROW ) );
}

VCREG_RET 
OpenRegKey( 
    IN LPTSTR pszKeyName, 
    OUT PHKEY phk 
) 
/*++

Routine Description:

    Opens a subkey of HKEY_LOCAL_MACHINE

Arguments:

    pszKeyName -
        Supplies the name of the subkey to open

    phk -
        Returns a handle to the key if successfully opened
        Returns NULL if an error occurs

Return Value:

    VCREG_OK if successful
    VCREG_READONLY if the key was opened with read-only access
    VCREG_OK if an error occurred

*/
{
    LONG Error;

    Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0,
            KEY_READ | KEY_WRITE, phk);

    if (Error != ERROR_SUCCESS)
    {
        Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0, KEY_READ, phk);
        if (Error != ERROR_SUCCESS)
        {
            *phk = NULL;
            return VCREG_ERROR;
        }

        /*
         * We only have Read access.
         */
        return VCREG_READONLY;
    }

    return VCREG_OK;
}

LONG    
CloseRegKey( 
    IN HKEY hkey 
) 
/*++

Routine Description:

    Closes a registry key opened by OpenRegKey()

Arguments:

    hkey -
        Supplies handle to key to close

Return Value:

    Whatever RegCloseKey() returns

--*/
{
    return RegCloseKey(hkey);
}

BOOL
IsWorkstationProduct(
)
/*++

Routine Description:

    Determines whether the currently running system is a Workstation
    product or a Server product.

Arguments:

    None.

Return Value:

    TRUE if the currently running system is a Workstation product.
    FALSE if the currently running system is some other kind of product.

--*/
{
    NT_PRODUCT_TYPE ProdType;

    RtlGetNtProductType(&ProdType);

    return(NtProductWinNt == ProdType);
}
