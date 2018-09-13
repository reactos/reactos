//*************************************************************
//
//  General.c   -   General property sheet page
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************
// NT base apis
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>

#include <sysdm.h>
#include <regstr.h>
#include <help.h>
#include <shellapi.h>
#include <shlapip.h>

//
// Globals for this page
//

static const TCHAR c_szEmpty[] = TEXT("");
static const TCHAR c_szCRLF[] = TEXT("\r\n");

static const TCHAR c_szAboutKey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");
static const TCHAR c_szAboutRegisteredOwner[] = REGSTR_VAL_REGOWNER;
static const TCHAR c_szAboutRegisteredOrganization[] = REGSTR_VAL_REGORGANIZATION;
static const TCHAR c_szAboutProductId[] = REGSTR_VAL_PRODUCTID;
static const TCHAR c_szAboutAnotherSerialNumber[] = TEXT("Plus! VersionNumber");
static const TCHAR c_szAboutAnotherProductId[] = TEXT("Plus! ProductId");

//
// oeminfo stuff
//

static const TCHAR c_szOemFile[] = TEXT("OemInfo.Ini");
static const TCHAR c_szOemImageFile[] = TEXT("OemLogo.Bmp");
static const TCHAR c_szOemGenSection[] = TEXT("General");
static const TCHAR c_szOemSupportSection[] = TEXT("Support Information");
static const TCHAR c_szOemName[] = TEXT("Manufacturer");
static const TCHAR c_szOemModel[] = TEXT("Model");
static const TCHAR c_szOemSupportLinePrefix[] = TEXT("line");
static const TCHAR c_szDefSupportLineText[] = TEXT("@");

static const TCHAR c_szProcessorInfo[] = TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
static const TCHAR c_szHardwareSystem[] = TEXT("HARDWARE\\DESCRIPTION\\System");
static const TCHAR c_szIndentifier[] = TEXT("Identifier");
static const TCHAR c_szNameString[] = TEXT("ProcessorNameString");


//
// Help ID's
//

DWORD aGeneralHelpIds[] = {
    IDC_GEN_WINDOWS_IMAGE,         NO_HELP,
    IDC_TEXT_1,                    (IDH_GENERAL + 0),
    IDC_TEXT_2,                    (IDH_GENERAL + 1),
    IDC_GEN_SERIAL_NUMBER,         (IDH_GENERAL + 1),
    IDC_GEN_SERVICE_PACK,          (IDH_GENERAL + 1),
    IDC_TEXT_3,                    (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_0,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_1,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_2,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_3,          (IDH_GENERAL + 3),
    IDC_GEN_OEM_IMAGE,             NO_HELP,
    IDC_TEXT_4,                    (IDH_GENERAL + 6),
    IDC_GEN_MACHINE_0,             (IDH_GENERAL + 7),
    IDC_GEN_MACHINE_1,             (IDH_GENERAL + 8),
    IDC_GEN_MACHINE_2,             (IDH_GENERAL + 9),
    IDC_GEN_MACHINE_3,             (IDH_GENERAL + 10),
    IDC_GEN_MACHINE_4,             (IDH_GENERAL + 11),
    IDC_GEN_OEM_SUPPORT,           (IDH_GENERAL + 12),
    IDC_GEN_REGISTERED_2,          (IDH_GENERAL + 14),
    IDC_GEN_REGISTERED_3,          (IDH_GENERAL + 15),
    IDC_GEN_MACHINE,               (IDH_GENERAL + 7),
    IDC_GEN_OEM_NUDGE,             (IDH_GENERAL + 16),
    0, 0
};


//
// Macros
//

#define BytesToK(pDW)   (*(pDW) = (*(pDW) + 512) / 1024)        // round up

//
// Function proto-types
//

BOOL APIENTRY PhoneSupportProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//*************************************************************
//
//  CreateGeneralPage()
//
//  Purpose:    Creates the general page
//
//  Parameters: hInst   -   hInstance
//
//
//  Return:     hPage if successful
//              NULL if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/17/95    ericflo    Created
//
//*************************************************************

HPROPSHEETPAGE CreateGeneralPage (HINSTANCE hInst)
{
    PROPSHEETPAGE psp;

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = hInst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
    psp.pfnDlgProc = GeneralDlgProc;
    psp.pszTitle = NULL;
    psp.lParam = 0;

    return CreatePropertySheetPage(&psp);
}

//*************************************************************
//
//  SetClearBitmap()
//
//  Purpose:    Sets or clears an image in a static control.
//
//  Parameters: control  -   handle of static control
//              resource -   resource / filename of bitmap
//              fl       -   SCB_ flags:
//                SCB_FROMFILE      'resource' specifies a filename instead of a resource
//                SCB_REPLACEONLY   only put the new image up if there was an old one
//      
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//
//  Comments:
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Ported
//
//*************************************************************

#define SCB_FROMFILE     (0x1)
#define SCB_REPLACEONLY  (0x2)

BOOL SetClearBitmap( HWND control, LPCTSTR resource, UINT fl )
{
    HBITMAP hbm = (HBITMAP)SendMessage(control, STM_GETIMAGE, IMAGE_BITMAP, 0);

    if( hbm )
    {
        DeleteObject( hbm );
    }
    else if( fl & SCB_REPLACEONLY )
    {
        return FALSE;
    }

    if( resource )
    {
        SendMessage(control, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)
            LoadImage( hInstance, resource, IMAGE_BITMAP, 0, 0,
            LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS |
            ( ( fl & SCB_FROMFILE )? LR_LOADFROMFILE : 0 ) ) );
    }

    return
        ((HBITMAP)SendMessage(control, STM_GETIMAGE, IMAGE_BITMAP, 0) != NULL);
}

#if 0 // Product ID is already hypenated in the Registry on NT 5
//*************************************************************
//
//  ConfigureProductID()
//
//  Purpose:    Hyphenates the product id in this format:
//
//                    12345-123-1234567-12345
//
//  Parameters: lpPid    -  Product ID
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/20/95    ericflo    Created
//
//*************************************************************

void ConfigureProductID(LPTSTR lpPid)
{
    TCHAR szBuf[64];


    if (!lpPid || !(*lpPid) || (lstrlen(lpPid) < 20) ) {
        return;
    }

    szBuf[0] = lpPid[0];
    szBuf[1] = lpPid[1];
    szBuf[2] = lpPid[2];
    szBuf[3] = lpPid[3];
    szBuf[4] = lpPid[4];

    szBuf[5] = TEXT('-');

    szBuf[6] = lpPid[5];
    szBuf[7] = lpPid[6];
    szBuf[8] = lpPid[7];

    szBuf[9] = TEXT('-');

    szBuf[10] = lpPid[8];
    szBuf[11] = lpPid[9];
    szBuf[12] = lpPid[10];
    szBuf[13] = lpPid[11];
    szBuf[14] = lpPid[12];
    szBuf[15] = lpPid[13];
    szBuf[16] = lpPid[14];

    szBuf[17] = TEXT('-');

    szBuf[18] = lpPid[15];
    szBuf[19] = lpPid[16];
    szBuf[20] = lpPid[17];
    szBuf[21] = lpPid[18];
    szBuf[22] = lpPid[19];

    szBuf[23] = TEXT('\0');

    lstrcpy (lpPid, szBuf);

}
#endif // 0

//*************************************************************
//
//  InitGeneralDlg()
//
//  Purpose:    Initalize the general page
//
//  Parameters: hDlg -  Handle to the dialog box
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/20/95    ericflo    Ported
//
//*************************************************************

VOID InitGeneralDlg(HWND hDlg)
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    OSVERSIONINFO ver;
    TCHAR _scr1[64];
    TCHAR _scr2[64];
    TCHAR oemfile[MAX_PATH];
    TCHAR szNumBuf1[32];
    DWORD cbData, dwTotalPhys;
    HKEY hkey;
    int ctlid;
    NTSTATUS Status;


    //
    // Set the default bitmap
    //

    SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_WINDOWS_IMAGE ),
        MAKEINTRESOURCE( IDB_WINDOWS ), 0 );


    //
    // Query for the build number information
    //

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&ver)) {
        return;
    }

    //
    // Build and set the string
    //

    if (GetSystemMetrics(SM_DEBUG)) {
        _scr1[0] = TEXT(' ');
        LoadString(hInstance, IDS_DEBUG, &_scr1[1], ARRAYSIZE(_scr1));
    } else {
        _scr1[0] = TEXT('\0');
    }


    wsprintf(_scr2, TEXT("%d.%02d.%d%s"), ver.dwMajorVersion, ver.dwMinorVersion,
             LOWORD(ver.dwBuildNumber), _scr1);

    SetDlgItemText(hDlg, IDC_GEN_SERIAL_NUMBER, _scr2);
    SetDlgItemText(hDlg, IDC_GEN_SERVICE_PACK, ver.szCSDVersion);



    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szAboutKey, &hkey) == ERROR_SUCCESS)
    {

        //
        // Do registered user info
        //
        ctlid = IDC_GEN_REGISTERED_0;  // start here and use more as needed

        cbData = ARRAYSIZE( _scr2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutRegisteredOwner,
            NULL, NULL, (LPBYTE)_scr2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, _scr2);
        }

        cbData = ARRAYSIZE( _scr2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutRegisteredOrganization,
            NULL, NULL, (LPBYTE)_scr2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, _scr2);
        }

        cbData = ARRAYSIZE( _scr2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutProductId,
            NULL, NULL, (LPBYTE)_scr2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, _scr2);
        }

        cbData = ARRAYSIZE( _scr2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutAnotherProductId,
            NULL, NULL, (LPBYTE)_scr2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, _scr2);
        }

        RegCloseKey(hkey);
    }


    //
    // Do machine info
    //

    ctlid = IDC_GEN_MACHINE_0;  // start here and use controls as needed

    //
    // if OEM name is present, show logo and check for phone support info
    //

    GetSystemDirectory(oemfile, ARRAYSIZE(oemfile));

    if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
        lstrcat( oemfile, TEXT("\\"));
    lstrcat( oemfile, c_szOemFile );

    if( GetPrivateProfileString( c_szOemGenSection, c_szOemName, c_szEmpty,
        _scr1, ARRAYSIZE( _scr1 ), oemfile ) )
    {
        SetDlgItemText( hDlg, ctlid++, _scr1 );

        if( GetPrivateProfileString( c_szOemGenSection, c_szOemModel,
            c_szEmpty, _scr1, ARRAYSIZE( _scr1 ), oemfile ) )
        {
            SetDlgItemText( hDlg, ctlid++, _scr1 );
        }

        lstrcpy( _scr2, c_szOemSupportLinePrefix );
        lstrcat( _scr2, TEXT("1") );

        if( GetPrivateProfileString( c_szOemSupportSection,
            _scr2, c_szEmpty, _scr1, ARRAYSIZE( _scr1 ), oemfile ) )
        {
            HWND wnd = GetDlgItem( hDlg, IDC_GEN_OEM_SUPPORT );

            EnableWindow( wnd, TRUE );
            ShowWindow( wnd, SW_SHOW );
        }

        GetSystemDirectory( oemfile, ARRAYSIZE( oemfile ) );
        if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
            lstrcat( oemfile, TEXT("\\") );
        lstrcat( oemfile, c_szOemImageFile );

        if( SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_OEM_IMAGE ), oemfile,
            SCB_FROMFILE ) )
        {
            ShowWindow( GetDlgItem( hDlg, IDC_GEN_OEM_NUDGE ), SW_SHOWNA );
            ShowWindow( GetDlgItem( hDlg, IDC_GEN_MACHINE ), SW_HIDE );
        }
    }

    //
    // Processor
    //

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szProcessorInfo, &hkey) == ERROR_SUCCESS) {

        //
        // Try for ProcessorNameString if present.
        //

        cbData = ARRAYSIZE( _scr2 ) * sizeof (TCHAR);
        if (RegQueryValueEx(hkey, c_szNameString, 0, 0, (LPBYTE)_scr2, &cbData) == ERROR_SUCCESS) {
            SetDlgItemText(hDlg, ctlid++, _scr2);
        } else {

            //
            // No ProcessorNameString, try the generic Identifier.
            //

            cbData = ARRAYSIZE( _scr2 ) * sizeof (TCHAR);
            if (RegQueryValueEx(hkey, c_szIndentifier, 0, 0, (LPBYTE)_scr2, &cbData) == ERROR_SUCCESS) {
                SetDlgItemText(hDlg, ctlid++, _scr2);
            }
        }
        RegCloseKey(hkey);
    }

    //
    // System identifier
    //

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szHardwareSystem, &hkey) == ERROR_SUCCESS) {

        cbData = ARRAYSIZE( _scr2 ) * sizeof (TCHAR);
        if (RegQueryValueEx(hkey, c_szIndentifier, 0, 0, (LPBYTE)_scr2, &cbData) == ERROR_SUCCESS) {
            SetDlgItemText(hDlg, ctlid++, _scr2);
        }

        RegCloseKey(hkey);
    }


    //
    // Memory
    //

    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if (!NT_SUCCESS(Status)) {

        return;
    }

    dwTotalPhys = BasicInfo.NumberOfPhysicalPages *
                                          (BasicInfo.PageSize / 1024);

    LoadString(hInstance, IDS_XDOTX_MB, _scr2, sizeof(_scr2));
    wsprintf(_scr1, _scr2, AddCommas(dwTotalPhys, szNumBuf1));
    SetDlgItemText( hDlg, ctlid++, _scr1 );

}


//*************************************************************
//
//  GeneralDlgProc()
//
//  Purpose:    Dialog box procedure for general tab
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/17/95    ericflo    Created
//
//*************************************************************

BOOL APIENTRY GeneralDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {
    case WM_INITDIALOG:

        InitGeneralDlg(hDlg);
        break;


    case WM_NOTIFY:

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_APPLY:
            SetWindowLong (hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
            return TRUE;

        default:
            return FALSE;
        }
        break;


    case WM_COMMAND:

        if (wParam == IDC_GEN_OEM_SUPPORT) {
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_PHONESUP),
                      GetParent(hDlg), PhoneSupportProc);
        }
        break;

    case WM_SYSCOLORCHANGE:
        {
        TCHAR oemfile[MAX_PATH];

        GetSystemDirectory( oemfile, ARRAYSIZE( oemfile ) );
        if( oemfile[ lstrlen(oemfile) - 1 ] != TEXT('\\'))
            lstrcat(oemfile, TEXT("\\"));
        lstrcat( oemfile, c_szOemImageFile );

        SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_OEM_IMAGE ), oemfile,
            SCB_FROMFILE | SCB_REPLACEONLY );

        SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_WINDOWS_IMAGE ),
            MAKEINTRESOURCE( IDB_WINDOWS ), 0 );
        }
        break;

    case WM_DESTROY:
        SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_OEM_IMAGE ), NULL, 0 );
        SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_WINDOWS_IMAGE ), NULL, 0 );
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (DWORD) (LPSTR) aGeneralHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD) (LPSTR) aGeneralHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  PhoneSupportProc()
//
//  Purpose:    Dialog box procedure for OEM phone support dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/17/95    ericflo    Created
//
//*************************************************************

BOOL APIENTRY PhoneSupportProc(HWND hDlg, UINT uMsg,
                               WPARAM wParam, LPARAM lParam )
{
    switch( uMsg ) {

        case WM_INITDIALOG:
        {
            HWND edit = GetDlgItem(hDlg, IDC_SUPPORT_TEXT);
            UINT i = 1;  // 1-based by design
            TCHAR oemfile[MAX_PATH];
            TCHAR text[ 256 ];
            TCHAR line[ 12 ];
            LPTSTR endline = line + lstrlen( c_szOemSupportLinePrefix );

            GetSystemDirectory( oemfile, ARRAYSIZE( oemfile ) );
            if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
                lstrcat( oemfile, TEXT("\\") );
            lstrcat( oemfile, c_szOemFile );

            GetPrivateProfileString( c_szOemGenSection, c_szOemName, c_szEmpty,
                text, sizeof( text ), oemfile );
            SetWindowText( hDlg, text );

            lstrcpy( line, c_szOemSupportLinePrefix );

            SendMessage (edit, WM_SETREDRAW, FALSE, 0);

            for( ;; i++ )
            {
                wsprintf( endline, TEXT("%u"), i );

                GetPrivateProfileString( c_szOemSupportSection,
                    line, c_szDefSupportLineText, text, sizeof( text ) - 2,
                    oemfile );

                if( !lstrcmpi( text, c_szDefSupportLineText ) )
                    break;

                lstrcat( text, c_szCRLF );

                SendMessage( edit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

                SendMessage( edit, EM_REPLACESEL, 0, (LPARAM)text );
            }

            SendMessage (edit, WM_SETREDRAW, TRUE, 0);
            break;
        }

        case WM_COMMAND:

            switch (LOWORD(wParam)) {
                 case IDOK:
                 case IDCANCEL:
                     EndDialog( hDlg, 0 );
                     break;

                 default:
                     return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}
