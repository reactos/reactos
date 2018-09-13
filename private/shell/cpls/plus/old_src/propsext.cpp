//---------------------------------------------------------------------------
//
//  File: PROPSEXT.CPP
//
//  Implementation of the CPropSheetExt object.
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#pragma hdrstop


//Defines for getting registry settings
const TCHAR c_szHICKey[] = TEXT("Control Panel\\Desktop\\WindowMetrics"); // show icons using highest possible colors
const TCHAR c_szHICVal[] = TEXT("Shell Icon BPP"); // (4 if the checkbox is false, otherwise 16, don't set it to anything else)
const TCHAR c_szHIKey[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
const TCHAR c_szHIVal[]  = TEXT("HideIcons");
const TCHAR c_szWMSISVal[] = TEXT("Shell Icon Size"); // Normal Icon Size (default 32 x 32)
const TCHAR c_szTitle[]  = TEXT("Effects");
#ifdef INET_EXP_ICON
const TCHAR c_szIEXP[]   = TEXT("\\Program Files\\Microsoft Internet Explorer 4");
#endif

#define HSUBKEY_HIC     (LPTSTR)c_szHICKey
#define HVALUE_HIC      (LPTSTR)c_szHICVal

#define HSUBKEY_WM      (LPTSTR)c_szHICKey
#define HVALUE_WMSIS    (LPTSTR)c_szWMSISVal

#define ICON_DEFAULT_SMALL    16
#define ICON_DEFAULT_NORMAL   32
#define ICON_DEFAULT_LARGE    48

enum ICON_SIZE_TYPES {
   ICON_DEFAULT         = 0,
   ICON_LARGE           = 1,
   ICON_INDETERMINATE   = 2
};

// Registry Info for the icons
typedef struct tagIconRegKeys
{
    TCHAR szIconSubKey[128];
    TCHAR szIconValue[16];
    TCHAR szTitleSubKey[128];
    int  iTitleResource;
    int  iDefaultTitleResource;
    BOOL bNTOnly;
}ICONREGKEYS;

ICONREGKEYS sIconRegKeys[] =
{
     // "My Computer" icon
    { TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon"),
      TEXT("\0"),
      TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}"),
      NULL,
      IDS_MYCOMPUTER,
      FALSE
    },

    // "My Documents" icon
    { TEXT("CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon"),
      TEXT("\0"),
      TEXT("CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}"),
      NULL,
      IDS_MYDOCUMENTS,
      FALSE
     },

// This one doesn't seem to pick up updates, so disable for now...
#ifdef INET_EXP_ICON
    // "Internet Explorer" icon
    { TEXT("CLSID\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}\\DefaultIcon"),
      TEXT("\0"),
      TEXT("CLSID\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}"),
      NULL,
      IDS_INTERNET
     },
#endif

    // "Net Neighbourhood" icon
    { TEXT("CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon"),
      TEXT("\0"),
      TEXT("CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}"),
      NULL,
      IDS_NETNEIGHBOUR
    },

    // "Trash full" icon
    { TEXT("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon"),
      TEXT("full"),
      TEXT("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}"),
      IDS_FULL,
      IDS_TRASHFULL
     },

     // "Trash empty" icon
    { TEXT("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon"),
      TEXT("empty"),
      TEXT("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}"),
      IDS_EMPTY,
      IDS_TRASHEMPTY
     },

    //
    // This is not a desktop icon, so for now it's not included.
    //

/*
    // "Directory" icon
    { TEXT("CLSID\\{FE1290F0-CFBD-11CF-A330-00AA00C16E65}\\DefaultIcon"),
      TEXT("\0"),
      TEXT("CLSID\\{FE1290F0-CFBD-11CF-A330-00AA00C16E65}"),
      NULL,
      IDS_DIRECTORY
    },
*/


};

#define NUM_ICONS (ARRAYSIZE(sIconRegKeys))

#define PATH_WIN  0
#define PATH_SYS  1
#define PATH_IEXP 2

typedef struct tagDefIcons
{
    int     iIndex;
    UINT    uPath;
    TCHAR   szFile[16];
}DEFICONS;

DEFICONS sDefaultIcons[NUM_ICONS] =
{
    { 0,PATH_WIN ,TEXT("\\EXPLORER.EXE")},  // "My Computer" default icon
    { 0,PATH_SYS ,TEXT("\\mydocs.dll")},    // "My Documents" default icon
#ifdef INET_EXP_ICON
    { 0,PATH_IEXP,TEXT("\\iexplore.exe")},  // "Internet Explorer" default icon
#endif
    {17,PATH_SYS, TEXT("\\shell32.dll")},   // "Net Neighbourhood" default icon
    {32,PATH_SYS, TEXT("\\shell32.dll")},   // "Trash full" default icon
    {31,PATH_SYS, TEXT("\\shell32.dll")},   // "Trash empty" default icon
//    { 0,TEXT("\\dsfolder.dll")},  // "Directory" default icon
};

// Name of the file that holds each icon, and an index for which icon to use in the file
typedef struct tagIconKeys
{
    TCHAR szOldFile[MAX_PATH];
    int   iOldIndex;
    TCHAR szNewFile[MAX_PATH];
    int   iNewIndex;
}ICONDATA;

ICONDATA sIconData[NUM_ICONS];

typedef struct
{
    DWORD dwControlID;
    DWORD dwHelpContextID;
}POPUP_HELP_ARRAY;

POPUP_HELP_ARRAY phaMainWin[] = {
   { (DWORD)IDC_ICONS,                          (DWORD)IDH_DISPLAY_PLUSPACK_LIST         },
   { (DWORD)IDC_CHANGEICON,                     (DWORD)IDH_DISPLAY_PLUSPACK_CHANGEICON   },
   { (DWORD)IDC_LARGEICONS,                     (DWORD)IDH_DISPLAY_PLUSPACK_LARGEICONS   },
   { (DWORD)IDC_ICONHIGHCOLOR,                  (DWORD)IDH_DISPLAY_PLUSPACK_ALLCOLORS    },
   { (DWORD)IDC_ICONDEFAULT,                    (DWORD)IDH_DISPLAY_PLUSPACK_DEFAULT_ICON },
   { (DWORD)IDC_GRPBOX_1,                       (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)IDC_GRPBOX_2,                       (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)0, (DWORD)0 },
   { (DWORD)0, (DWORD)0 },          // double-null terminator NECESSARY!
};

POPUP_HELP_ARRAY phaMainWinPlus[] = {
   { (DWORD)IDC_ICONS,                          (DWORD)IDH_PLUS_PLUSPACK_LIST         },
   { (DWORD)IDC_CHANGEICON,                     (DWORD)IDH_PLUS_PLUSPACK_CHANGEICON   },
   { (DWORD)IDC_LARGEICONS,                     (DWORD)IDH_PLUS_PLUSPACK_LARGEICONS   },
   { (DWORD)IDC_ICONHIGHCOLOR,                  (DWORD)IDH_PLUS_PLUSPACK_ALLCOLORS    },
   { (DWORD)IDC_GRPBOX_1,                       (DWORD)IDH_COMM_GROUPBOX              },
   { (DWORD)IDC_GRPBOX_2,                       (DWORD)IDH_COMM_GROUPBOX              },
   { (DWORD)0, (DWORD)0 },
   { (DWORD)0, (DWORD)0 },          // double-null terminator NECESSARY!
};

POPUP_HELP_ARRAY * g_phaHelp = NULL;


HWND hWndList;          // handle to the list view window
HIMAGELIST hIconList;   // handles to image lists for large icons

// Handle to the DLL
extern HINSTANCE g_hInst;

// vars needed for new shell api
#define SZ_SHELL32                  TEXT("shell32.dll")
#define SZ_SHUPDATERECYCLEBINICON   "SHUpdateRecycleBinIcon"    // Parameter for GetProcAddr()... DO NOT TEXT("") IT!

HINSTANCE hmodShell32 = NULL;
typedef void (* PFNSHUPDATERECYCLEBINICON)( void );
PFNSHUPDATERECYCLEBINICON pfnSHUpdateRecycleBinIcon = NULL;

// Function prototype
BOOL CALLBACK PlusPackDlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam );
HWND CreateListView( HWND hWndParent );

// Icon Stuff
int   GetIconState (void);
BOOL  ChangeIconSizes (HWND hDlg, int iOldState, int iNewState);

HRESULT ExtractPlusColorIcon(LPCTSTR szPath, int nIndex, HICON *phIcon, UINT uSizeLarge, UINT uSizeSmall);
BOOL  gfCoInitDone = FALSE;         // track state of OLE CoInitialize()


//---------------------------------------------------------------------------
//  Class Member functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//  Constructor
//---------------------------------------------------------------------------
CPropSheetExt::CPropSheetExt( LPUNKNOWN pUnkOuter, LPFNDESTROYED pfnDestroy )
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pfnDestroy = pfnDestroy;
    return;
}

//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------
CPropSheetExt::~CPropSheetExt( void )
{
    return;
}

//---------------------------------------------------------------------------
//  QueryInterface()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    if( IsEqualIID( riid, IID_IShellPropSheetExt ) )
    {
        *ppv = (LPVOID)this;
        ++m_cRef;
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

//---------------------------------------------------------------------------
//  AddRef()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::AddRef( void )
{
    return ++m_cRef;
}

//---------------------------------------------------------------------------
//  Release()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::Release( void )
{
ULONG cRefT;

    cRefT = --m_cRef;

    if( m_cRef == 0 )
    {
        // Tell the housing that an object is going away so that it
        // can shut down if appropriate.
        if( NULL != m_pfnDestroy )
        {
            (*m_pfnDestroy)();
        }
        delete this;
    }
    return cRefT;
}

//---------------------------------------------------------------------------
//  AddPages()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;
    TCHAR szTitle[ 30 ];

    LoadString( g_hInst, IDS_ICONS, szTitle, ARRAYSIZE(szTitle) );
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USETITLE;
    psp.hIcon = NULL;
    psp.hInstance = g_hInst;
    psp.pszTemplate =MAKEINTRESOURCE( PLUSPACK_DLG );
    psp.pfnDlgProc = (DLGPROC)PlusPackDlgProc;
    psp.pszTitle = szTitle;
    psp.lParam = 0;

    if( ( hpage = CreatePropertySheetPage( &psp ) ) == NULL )
    {
        return ( E_OUTOFMEMORY );
    }

    if( !lpfnAddPage( hpage, lParam ) )
    {
        DestroyPropertySheetPage( hpage );
        return ( E_FAIL );
    }
    return NOERROR;
}

//---------------------------------------------------------------------------
//  ReplacePage()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::ReplacePage( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    return NOERROR;
}


//---------------------------------------------------------------------------
//
// PlusPackDlgProc()
//
//  The dialog procedure for the "PlusPack" property sheet page.
//
//---------------------------------------------------------------------------
BOOL CALLBACK PlusPackDlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)GetWindowLong( hDlg, DWL_USER );
    static int      iOldLI, iNewLI;         // Large Icon State
    static int      iOldHIC, iNewHIC;       // High Icon Colour
    static BOOL     bOldMA, bNewMA;         // Menu Animation State
    static BOOL     bOldSF, bNewSF;         // Font Smoothing State
    static BOOL     bOldDW, bNewDW;         // Drag Window State
    static BOOL     bOldHI, bNewHI;         // Hide Icons State
    BOOL            bDorked = FALSE, bRet, bDorkedIcons;
    static int      iIndex, iX;
    static char     szHelpFile[32];


    switch( uMessage )
    {
        case WM_INITDIALOG:
        {
            OSVERSIONINFO osvi;
            UINT id = IDS_HELPFILE_PLUS;


            // Create our list view and fill it with the system icons
            CreateListView( hDlg );
            iIndex = 0;

            SetWindowLong( hDlg, DWL_USER, lParam );
            psp = (LPPROPSHEETPAGE)lParam;

            // Get the name of our help file.  For Memphis, it's
            // IDS_HELPFILE_PLUS for NT it's IDS_HELPFILE.
            g_phaHelp = phaMainWinPlus;
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (GetVersionEx( &osvi ))
            {
                if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
                {
                    id = IDS_HELPFILE;
                    g_phaHelp = phaMainWin;
                }
            }

            LoadString( g_hInst, id, szHelpFile, 32 );


            // Get the values for the settings from the registry and set the checkboxes

            // Large Icons
            iOldLI = GetIconState ();
            if (iOldLI == ICON_INDETERMINATE)
            {
                HWND hItem = GetDlgItem (hDlg, IDC_LARGEICONS);
                SendMessage( hItem,
                             BM_SETSTYLE,
                             (WPARAM)LOWORD(BS_AUTO3STATE),
                             MAKELPARAM( FALSE,0)
                            );
            }
            iNewLI = iOldLI;
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_LARGEICONS ),
                         BM_SETCHECK,
                         (WPARAM)iOldLI,
                         0
                        );

            // Full Color Icons
            bRet = GetRegValueInt( HKEY_CURRENT_USER,
                                   HSUBKEY_HIC,
                                   HVALUE_HIC,
                                   &iOldHIC
                                  );

            if( bRet == FALSE ) // Key not in registry yet
            {
                iOldHIC = iNewHIC = 4;
            }

            iNewHIC = iOldHIC;
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_ICONHIGHCOLOR ),
                         BM_SETCHECK,
                         (WPARAM)(BOOL)(iOldHIC == 16),
                         0
                        );

            // Hide Icons
            bOldHI = (BOOL)GetRegValueDword( HKEY_CURRENT_USER,
                                             (LPTSTR)c_szHIKey,
                                             (LPTSTR)c_szHIVal
                                            );
            if (bOldHI == REG_BAD_DWORD)
            {
                bOldHI = FALSE;
            }
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_HIDEICONS ),
                         BM_SETCHECK,
                         (WPARAM)bOldHI,
                         0
                        );
            bNewHI = bOldHI;


            // Use menu animations
            bOldMA = FALSE;
            SystemParametersInfo( SPI_GETMENUANIMATION, 0, (PVOID)&bOldMA, 0 );
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_MENUANIMATION ),
                         BM_SETCHECK,
                         (WPARAM)bOldMA,
                         0
                        );
            bNewMA = bOldMA;

            // Smooth edges of screen fonts
            bOldSF = FALSE;
            SystemParametersInfo( SPI_GETFONTSMOOTHING, 0, (PVOID)&bOldSF, 0 );
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_FONTSMOOTH ),
                         BM_SETCHECK,
                         (WPARAM)bOldSF,
                         0
                        );
            bNewSF = bOldSF;

            // Show contents while dragging
            bOldDW = FALSE;
            SystemParametersInfo( SPI_GETDRAGFULLWINDOWS, 0, (PVOID)&bOldDW, 0 );
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_SHOWDRAG ),
                         BM_SETCHECK,
                         (WPARAM)bOldDW,
                         0
                        );
            bNewDW = bOldDW;

            // Load SHUpdateRecycleBinIcon() if it exists
            hmodShell32 = LoadLibrary(SZ_SHELL32);
            pfnSHUpdateRecycleBinIcon = (PFNSHUPDATERECYCLEBINICON)GetProcAddress( hmodShell32, SZ_SHUPDATERECYCLEBINICON );
        }
        break;

        case WM_DESTROY:
            if ( gfCoInitDone )
                CoUninitialize();
            if (hmodShell32)
                FreeLibrary(hmodShell32);
            break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_LARGEICONS:
                    iNewLI = SendMessage ( (HWND)lParam, BM_GETCHECK, 0, 0 );
                    bDorked = TRUE;
                    break;

                case IDC_ICONHIGHCOLOR:
                    iNewHIC = 4;
                    if( SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == TRUE )
                    {
                        iNewHIC = 16;
                    }
                    bDorked = TRUE;
                    break;

                case IDC_SHOWDRAG:
                    bNewDW = (SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
                    bDorked = TRUE;
                    break;

                case IDC_HIDEICONS:
                    bNewHI = (SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
                    bDorked = TRUE;
                    break;

                case IDC_MENUANIMATION:
                    bNewMA = (SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
                    bDorked = TRUE;
                    break;

                case IDC_FONTSMOOTH:
                    bNewSF = (SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
                    bDorked = TRUE;
                    break;

                case IDC_CHANGEICON:
                {
                    INT i = sIconData[iIndex].iOldIndex;
                    WCHAR szTemp[ MAX_PATH ];
                    TCHAR szExp[ MAX_PATH ];

                    ExpandEnvironmentStrings( sIconData[iIndex].szOldFile,
                                              szExp,
                                              ARRAYSIZE(szExp)
                                             );

                    if (g_RunningOnNT)
                    {
                        MultiByteToWideChar( CP_ACP, 0, szExp, -1, szTemp, ARRAYSIZE(szTemp) );
                    }
                    else
                    {
                        lstrcpy( (LPTSTR)szTemp, szExp );
                    }

                    if ( PickIconDlg( hDlg,
                                      (LPTSTR)szTemp,
                                      ARRAYSIZE(szTemp),
                                      &i
                                     ) == TRUE
                        )
                    {
                        HICON hIcon;

                        if (g_RunningOnNT)
                        {
                            WideCharToMultiByte( CP_ACP, 0,
                                                 szTemp, -1,
                                                 sIconData[iIndex].szNewFile,
                                                 ARRAYSIZE(sIconData[iIndex].szNewFile),
                                                 NULL, NULL
                                                );
                        }
                        else
                        {
                            lstrcpy( sIconData[iIndex].szNewFile, (LPTSTR)szTemp );
                        }
                        sIconData[iIndex].iNewIndex = i;
                        ExtractPlusColorIcon( sIconData[iIndex].szNewFile,
                                              sIconData[iIndex].iNewIndex,
                                              &hIcon,
                                              0,
                                              0
                                             );

                        ImageList_ReplaceIcon( hIconList, iIndex, hIcon );
                        ListView_RedrawItems( hWndList, iIndex, iIndex );
                        bDorked = TRUE;
                    }
                }
                    break;

                case IDC_ICONDEFAULT:
                    {
                        TCHAR szTemp[_MAX_PATH];
                        HICON hIcon;

                        switch( sDefaultIcons[iIndex].uPath )
                        {
                            case PATH_WIN:
                                GetWindowsDirectory( szTemp, ARRAYSIZE(szTemp) );
                                break;

#ifdef INET_EXP_ICON
                            case PATH_IEXP:
                                if (g_RunningOnNT)
                                {
                                    lstrcpy( szTemp, TEXT("%SystemDrive%") );
                                }
                                else
                                {
                                    GetWindowsDirectory( szTemp, ARRAYSIZE(szTemp) );

                                    //
                                    // Clear out path after drive, ie: C:
                                    //
                                    szTemp[ 2 ] = 0;
                                }
                                lstrcat( szTemp, c_szIEXP );
                                break;
#endif

                            case PATH_SYS:
                            default:
                                GetSystemDirectory( szTemp, ARRAYSIZE(szTemp) );
                                break;

                        }

                        lstrcat( szTemp,  sDefaultIcons[iIndex].szFile );
                        lstrcpy( sIconData[iIndex].szNewFile, szTemp );
                        sIconData[iIndex].iNewIndex = sDefaultIcons[iIndex].iIndex;

                        ExtractPlusColorIcon( sIconData[iIndex].szNewFile,
                                              sIconData[iIndex].iNewIndex,
                                              &hIcon,
                                              0,
                                              0
                                             );

                        ImageList_ReplaceIcon( hIconList, iIndex, hIcon );
                        ListView_RedrawItems( hWndList, iIndex, iIndex );
                        bDorked = TRUE;
                    }
                    break;
                default:
                    break;
            }

            // If the user dorked with a setting, tell the property manager we
            // have outstanding changes. This will enable the "Apply Now" button...
            if( bDorked )
            {
                SendMessage( GetParent( hDlg ), PSM_CHANGED, (WPARAM)hDlg, 0L );
            }
            break;

        case WM_NOTIFY:
            switch( ((NMHDR *)lParam)->code )
            {
                case LVN_ITEMCHANGED:   // The selection changed in our listview
                    if( wParam == IDC_ICONS )
                    {
                        BOOL bEnable = FALSE;
                        // Find out who's selected now
                        for( iIndex = 0; iIndex < NUM_ICONS;iIndex++ )
                        {
                            if( ListView_GetItemState( hWndList, iIndex, LVIS_SELECTED ) )
                            {
                                bEnable = TRUE;
                                break;
                            }
                        }
                        EnableWindow( GetDlgItem( hDlg, IDC_CHANGEICON ), bEnable );
                        EnableWindow( GetDlgItem( hDlg, IDC_ICONDEFAULT ), bEnable );
                    }
                    break;

                case PSN_APPLY: // OK or Apply clicked
                {
                    HDC hDC = GetDC( NULL );
                    int iBitsPerPixel;

                    iBitsPerPixel = GetDeviceCaps( hDC, BITSPIXEL );
                    ReleaseDC( NULL, hDC );

                    // Large Icons
                    bDorkedIcons = ChangeIconSizes (hDlg, iOldLI, iNewLI);
                    if (bDorkedIcons)
                    {
                        iOldLI = iNewLI;
                        bDorked = TRUE;
                    }

                    // Full Color Icons
                    if( iOldHIC != iNewHIC )
                    {
                        TCHAR szTemp1[512];
                        TCHAR szTemp2[256];

                        bRet = SetRegValueInt( HKEY_CURRENT_USER,
                                               HSUBKEY_HIC,
                                               HVALUE_HIC,
                                               iNewHIC
                                              );
                        iOldHIC = iNewHIC;
                        if ((iBitsPerPixel < 16) && (iNewHIC == 16)) // Display mode won't support icon high colors
                        {
                            LoadString (g_hInst, IDS_256COLORPROBLEM, szTemp1, ARRAYSIZE(szTemp1) );
                            LoadString( g_hInst, IDS_ICONCOLORWONTWORK, szTemp2, ARRAYSIZE(szTemp2) );
                            lstrcat (szTemp1, szTemp2);
                            MessageBox( hDlg, szTemp1, c_szTitle, MB_OK );
                        }
                        else
                        {
                           LoadString( g_hInst, IDS_REBOOTFORCHANGE, szTemp1, ARRAYSIZE(szTemp1) );
                           MessageBox( hDlg, szTemp1, c_szTitle, MB_OK );
                        }
                    }

                    // Full window drag
                    if ( bOldDW != bNewDW )
                    {
                        bOldDW = bNewDW;
                        SystemParametersInfo( SPI_SETDRAGFULLWINDOWS,
                                              bNewDW,
                                              NULL,
                                              SPIF_SENDCHANGE
                                             );
                    }

                    // Font smoothing
                    if ( bOldSF != bNewSF )
                    {
                        bOldSF = bNewSF;
                        SystemParametersInfo( SPI_SETFONTSMOOTHING,
                                              bNewSF,
                                              NULL,
                                              SPIF_SENDCHANGE
                                             );
                    }

                    // Menu animations
                    if ( bOldMA != bNewMA )
                    {
                        bOldMA = bNewMA;
                        SystemParametersInfo( SPI_SETMENUANIMATION,
                                              bNewMA,
                                              NULL,
                                              SPIF_SENDCHANGE
                                             );
                    }

                    // Hide Icons
                    if ( bOldHI != bNewHI )
                    {
                        bOldHI = bNewHI;
                        SetRegValueDword( HKEY_CURRENT_USER,
                                          (LPTSTR)c_szHIKey,
                                          (LPTSTR)c_szHIVal,
                                          (DWORD)bNewHI
                                         );
                    }

                    // Change the system icons
                    for( iX = 0;iX < NUM_ICONS;iX++ )
                    {

                        if( (lstrcmpi( sIconData[iX].szNewFile, sIconData[iX].szOldFile ) != 0) ||
                            (sIconData[iX].iNewIndex != sIconData[iX].iOldIndex)
                           )
                        {
                            TCHAR szTemp[MAX_PATH];

                            wsprintf( szTemp,
                                      TEXT("%s,%d"),
                                      sIconData[iX].szNewFile,
                                      sIconData[iX].iNewIndex
                                     );
                            bRet = IconSetRegValueString( sIconRegKeys[iX].szIconSubKey,
                                                          sIconRegKeys[iX].szIconValue,
                                                          (LPTSTR)szTemp
                                                         );

                            // Next two lines necessary if the user does an Apply as opposed to OK
                            lstrcpy( sIconData[iX].szOldFile, sIconData[iX].szNewFile );
                            sIconData[iX].iOldIndex = sIconData[iX].iNewIndex;
                            bDorked = TRUE;
                        }
                    }

                    // Make the system notice we changed the system icons
                    if( bDorked )
                    {
                        SHChangeNotify( SHCNE_ASSOCCHANGED, 0, NULL, NULL ); // should do the trick!

                        if (pfnSHUpdateRecycleBinIcon != NULL)
                        {
                            pfnSHUpdateRecycleBinIcon();
                        }
                    }

                    if (bDorkedIcons)
                    {
                        SendMessage (HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
                    }

                    break;
                }
                default:
                    break;
            }
            break;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPHELPINFO)lParam;

            if( lphi->iContextType == HELPINFO_WINDOW )
            {
                WinHelp( (HWND)lphi->hItemHandle,
                         (LPTSTR)szHelpFile,
                         HELP_WM_HELP,
                         (DWORD)((POPUP_HELP_ARRAY FAR *)g_phaHelp)
                        );
            }
        }
            break;

        case WM_CONTEXTMENU:
            // first check for dlg window
            if( (HWND)wParam == hDlg )
            {
                // let the def dlg proc decide whether to respond or ignore;
                // necessary for title bar sys menu on right click
                return FALSE;       // didn't process message EXIT
            }
            else
            {
                // else go for the controls
                WinHelp( (HWND)wParam,
                         (LPTSTR)szHelpFile,
                         HELP_CONTEXTMENU,
                         (DWORD)((POPUP_HELP_ARRAY FAR *)g_phaHelp)
                        );
            }
            break;

        default:
            return FALSE;
    }
    return(TRUE);
}


/****************************************************************************
*
*    FUNCTION: CreateListView(HWND)
*
*    PURPOSE:  Creates the list view window and initializes it
*
****************************************************************************/
HWND CreateListView( HWND hWndParent )
{
    LV_ITEM lvI;            // List view item structure
    TCHAR   szTemp[MAX_PATH];
    BOOL bEnable = FALSE;
    RECT rc;

    // Create a device independant size and location
    LONG lWndunits = GetDialogBaseUnits();
    int iWndx = LOWORD(lWndunits);
    int iWndy = HIWORD(lWndunits);
    int iX = ((11 * iWndx) / 4);
    int iY = ((15 * iWndy) / 8);
    int iWidth = ((163 * iWndx) / 4);
    int iHeight = ((40 * iWndy) / 8);

    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    // Get the list view window
    hWndList = GetDlgItem (hWndParent, IDC_ICONS);
    if( hWndList == NULL  )
        return NULL;

    // initialize the list view window
    // First, initialize the image lists we will need
    hIconList = ImageList_Create( 32, 32, ILC_MASK | ILC_COLOR24, NUM_ICONS, 0 );   // create an image list for the icons

    // load the icons and add them to the image lists
    // get the icon files and indexes from the registry, including for the Default recycle bin
    for( iX = 0; iX < NUM_ICONS; iX++ )
    {
        TCHAR* pPos;
        HICON hIcon;
        BOOL bRet;

        bRet = IconGetRegValueString( sIconRegKeys[iX].szIconSubKey,
                                      sIconRegKeys[iX].szIconValue,
                                      (LPSTR)szTemp,
                                      MAX_PATH
                                     );

        int iIndex = PathParseIconLocation( szTemp );

        // store the icon information
        lstrcpy( sIconData[iX].szOldFile, szTemp );
        lstrcpy( sIconData[iX].szNewFile, szTemp );
        sIconData[iX].iOldIndex = iIndex;
        sIconData[iX].iNewIndex = iIndex;

        ExtractPlusColorIcon( szTemp, iIndex, &hIcon, 0, 0);

        // Added this "if" to fix bug 2831.  We want to use SHELL32.DLL
        // icon 0 if there is no icon in the file specified in the
        // registry (or if the registry didn't specify a file).
        if( hIcon == NULL )
        {
            GetSystemDirectory( szTemp, sizeof(szTemp) );
            lstrcat( szTemp,  TEXT("\\shell32.dll") );
            lstrcpy( sIconData[iX].szOldFile, szTemp );
            lstrcpy( sIconData[iX].szNewFile, szTemp );
            sIconData[iX].iOldIndex = sIconData[iX].iNewIndex = 0;

            hIcon = ExtractIcon( g_hInst, szTemp, iIndex );
        }

        if (ImageList_AddIcon( hIconList, hIcon ) == -1)
        {
            return NULL;
        }
    }

    // Make sure that all of the icons were added
    if( ImageList_GetImageCount( hIconList ) < NUM_ICONS )
        return FALSE;

    ListView_SetImageList( hWndList, hIconList, LVSIL_NORMAL );

    // Make sure the listview has WS_HSCROLL set on it.
    DWORD dwStyle = GetWindowLong( hWndList, GWL_STYLE );
    SetWindowLong( hWndList, GWL_STYLE, (dwStyle & (~WS_VSCROLL)) | WS_HSCROLL );

    // Finally, let's add the actual items to the control.  Fill in the LV_ITEM
    // structure for each of the items to add to the list.  The mask specifies
    // the the .pszText, .iImage, and .state members of the LV_ITEM structure are valid.
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    for( iX = 0; iX < NUM_ICONS; iX++ )
    {
        TCHAR szAppend[64];
        BOOL bRet;

        bRet = IconGetRegValueString( sIconRegKeys[iX].szTitleSubKey,
                                      NULL,
                                      (LPTSTR)szTemp,
                                      MAX_PATH
                                     );

        // if the title string was in the registry, else we have to use the default in our resources
        if( (bRet) && (lstrlen(szTemp) > 0) )
        {
            if( LoadString( g_hInst, sIconRegKeys[iX].iTitleResource, szAppend, 64 ) != 0 )
            {
                lstrcat( szTemp, szAppend );
            }
        }
        else
        {
            LoadString( g_hInst,
                        sIconRegKeys[iX].iDefaultTitleResource,
                        szTemp,
                        MAX_PATH
                       );
        }

        lvI.iItem = iX;
        lvI.iSubItem = 0;
        lvI.pszText = (LPSTR)&(szTemp);
        lvI.cchTextMax = lstrlen(szTemp);
        lvI.iImage = iX;

        if( ListView_InsertItem( hWndList, &lvI ) == -1 )
            return NULL;

    }
/*
    // To fix long standing listview bug, we need to "jiggle" the listview
    // window size so that it will do a recompute and realize that we need a
    // scroll bar...
    GetWindowRect( hWndList, &rc );
    MapWindowPoints( NULL, hWndParent, (LPPOINT)&rc, 2 );
    MoveWindow( hWndList, rc.left, rc.top, rc.right - rc.left+1, rc.bottom - rc.top, FALSE );
    MoveWindow( hWndList, rc.left, rc.top, rc.right - rc.left,   rc.bottom - rc.top, FALSE );
*/
    // Set First item to selected
    ListView_SetItemState (hWndList, 0, LVIS_SELECTED, LVIS_SELECTED);

    // Get Selected item
    for(int iIndex = 0;iIndex < NUM_ICONS;iIndex++ )
    {
        if( ListView_GetItemState( hWndList, iIndex, LVIS_SELECTED ) )
        {
            bEnable = TRUE;
            break;
        }
    }

    EnableWindow( GetDlgItem( hWndParent, IDC_CHANGEICON ), bEnable );
    EnableWindow( GetDlgItem( hWndParent, IDC_ICONDEFAULT ), bEnable );

    return (hWndList);
}


int GetIconState (void)
{
    BOOL bRet;
    int iSize;

    bRet = GetRegValueInt (HKEY_CURRENT_USER, HSUBKEY_WM, HVALUE_WMSIS, &iSize);
    if (bRet == FALSE)
        return ICON_DEFAULT;

    if (iSize == ICON_DEFAULT_NORMAL)
        return ICON_DEFAULT;
    else if (iSize == ICON_DEFAULT_LARGE)
        return ICON_LARGE;
    return ICON_INDETERMINATE;
}


BOOL ChangeIconSizes (HWND hDlg, int iOldState, int iNewState)
{
    BOOL bRet;
    int  iOldSize, iNewSize;
    int  iHorz;
    int  iVert;

    // Don't bother if nothing changed
    if (iOldState == iNewState)
        return FALSE;

    // Get New Size
    switch (iNewState)
        {
        case ICON_DEFAULT:
            iNewSize = ICON_DEFAULT_NORMAL;
            break;

        case ICON_LARGE:
            iNewSize = ICON_DEFAULT_LARGE;
            break;

        case ICON_INDETERMINATE:
            // Don't bother to change anything
            return FALSE;

        default:
            return FALSE;
        }

    // Get Original Size
    bRet = GetRegValueInt (HKEY_CURRENT_USER, HSUBKEY_WM, HVALUE_WMSIS, &iOldSize);
    if (!bRet)
    {
        // Try geting system default instead
        iOldSize = GetSystemMetrics (SM_CXICON);
    }


    // Don't need to change size if nothing has really changed
    if (iNewSize == iOldSize)
        return FALSE;

    // Get new horizontal spacing
    iHorz = GetSystemMetrics (SM_CXICONSPACING);
    iHorz -= iOldSize;
    if (iHorz < 0)
    {
        iHorz = 0;
    }
    iHorz += iNewSize;

    // Get new vertical spacing
    iVert = GetSystemMetrics (SM_CYICONSPACING);
    iVert -= iOldSize;
    if (iVert < 0)
    {
        iVert = 0;
    }
    iVert += iNewSize;

        // Set New sizes and spacing
    bRet = SetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_WM, HVALUE_WMSIS, iNewSize );
    if (!bRet)
        return FALSE;

    SystemParametersInfo( SPI_ICONHORIZONTALSPACING, (WPARAM)iHorz, NULL, SPIF_UPDATEINIFILE );
    SystemParametersInfo( SPI_ICONVERTICALSPACING, (WPARAM)iVert, NULL, SPIF_UPDATEINIFILE );

        // Turn from Tri-State back to normal check box
    if (iOldState == ICON_INDETERMINATE)
    {
        HWND hItem = GetDlgItem (hDlg, IDC_LARGEICONS);
        SendMessage( hItem,
                     BM_SETSTYLE,
                     (WPARAM)LOWORD(BS_AUTOCHECKBOX),
                     MAKELPARAM( FALSE,0)
                    );
    }

    // We did change the sizes
    return TRUE;
}


//
//  ExtractPlusColorIcon
//
//  Extract Icon from a file in proper Hi or Lo color for current system display
//
// from FrancisH on 6/22/95 with mods by TimBragg
HRESULT ExtractPlusColorIcon( LPCTSTR szPath, int nIndex, HICON *phIcon,
                              UINT uSizeLarge, UINT uSizeSmall)
{
    IShellLink *psl;
    HRESULT hres;
    HICON hIcons[2];    // MUST! - provide for TWO return icons

    if ( !gfCoInitDone )
    {
        if (SUCCEEDED(CoInitialize(NULL)))
            gfCoInitDone = TRUE;
    }

    *phIcon = NULL;
    if (SUCCEEDED(hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl)))
    {
        if (SUCCEEDED(hres = psl->SetIconLocation(szPath, nIndex)))
        {
            IExtractIcon *pei;
            if (SUCCEEDED(hres = psl->QueryInterface(IID_IExtractIcon, (void**)&pei)))
            {
                if (SUCCEEDED(hres = pei->Extract(szPath, nIndex,
                    &hIcons[0], &hIcons[1], (UINT)MAKEWPARAM((WORD)uSizeLarge,
                    (WORD)uSizeSmall))))
                {
                    *phIcon = hIcons[0];    // Return first icon to caller
                }

                pei->Release();
            }
        }

        psl->Release();
    }

    return hres;
}   // end ExtractPlusColorIcon()
