//---------------------------------------------------------------------------
//
//  File: PROPSEXT.CPP
//
//  Implementation of the CPropSheetExt object.
//
//---------------------------------------------------------------------------

#include "propsext.h"
#include "rc.h"
#include "regutils.h"
#include "string.h"
#include "pickicon.h"
#include "addon.h"
#include <shlobj.h>


//Defines for getting registry settings
#define HSUBKEY_FWD     "Control Panel\\Desktop"    // full window drag
#define HVALUE_FWD      "DragfullWindows"           // = (0 for false, 1 for true)

#define HSUBKEY_FS      "Control Panel\\Desktop"    // font smoothing
#define HVALUE_FS       "FontSmoothing"             // =(0 for false, 1 for true)

#define HSUBKEY_HIC     "Control Panel\\Desktop\\WindowMetrics" // show icons using highest possible colors
#define HVALUE_HIC      "Shell Icon BPP"    // (4 if the checkbox is false, otherwise 16, don't set it to anything else)

#define HSUBKEY_SWFS    "Control Panel\\Desktop"    // stretch wallpaper to fit desktop
#define HVALUE_SWFS     "WallpaperStyle"            // = (2 for stretch, else false)

#define HSUBKEY_WM     "Control Panel\\Desktop\\WindowMetrics"   // stretch wallpaper to fit desktop
#define HVALUE_WMSIS    "Shell Icon Size"                         // Normal Icon Size (default 32 x 32)
#define NUM_ICONS   4

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
    char szIconSubKey[128];
    char szIconValue[16];
    char szTitleSubKey[128];
    int  iTitleResource;
    int  iDefaultTitleResource;
}ICONREGKEYS;

ICONREGKEYS sIconRegKeys[NUM_ICONS+1] =
{
    {"CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon","\0","CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}",NULL,IDS_MYCOMPUTER},        // "My Computer" icon
    {"CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon","\0","CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}",NULL,IDS_NETNEIGHBOUR},      // "Net Neighbourhood" icon
    {"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon","full","CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}",IDS_FULL,IDS_TRASHFULL},   // "Trash full" icon
    {"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon","empty","CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}",IDS_EMPTY,IDS_TRASHEMPTY},// "Trash empty" icon
    {"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon","\0","CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}",NULL,IDS_TRASHDEFAULT}       // "Trash default" icon
};

typedef struct tagDefIcons
{
    int     iIndex;
    char    szFile[16];
}DEFICONS;

DEFICONS sDefaultIcons[NUM_ICONS+1] =
{
    {0,"\\EXPLORER.EXE"},   // "My Computer" default icon
    {17,"\\shell32.dll"},   // "Net Neighbourhood" default icon
    {32,"\\shell32.dll"},   // "Trash full" default icon
    {31,"\\shell32.dll"},   // "Trash empty" default icon
    {32,"\\shell32.dll"}    // "Trash default" default icon
};

// Name of the file that holds each icon, and an index for which icon to use in the file
typedef struct tagIconKeys
{
    char szOldFile[MAX_PATH];
    int  iOldIndex;
    char szNewFile[MAX_PATH];
    int  iNewIndex;
}ICONDATA;

ICONDATA sIconData[NUM_ICONS+1];    // One extra to hold the default Recycle bin info (even though we don't show it)

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
   { (DWORD)IDC_FULLWINDOWDRAGGING,             (DWORD)IDH_DISPLAY_PLUSPACK_FULLWINDOW   },
   { (DWORD)IDC_FONTSMOOTHING,                  (DWORD)IDH_DISPLAY_PLUSPACK_FONTSMOOTH   },
   { (DWORD)IDC_STRETCHWALLPAPERFITSCREEN,      (DWORD)IDH_DISPLAY_PLUSPACK_STRETCH      },
   { (DWORD)IDC_ICONDEFAULT,                    (DWORD)IDH_DISPLAY_PLUSPACK_DEFAULT_ICON },
   { (DWORD)IDC_GRPBOX_1,                       (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)IDC_GRPBOX_2,                       (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)0, (DWORD)0 },
   { (DWORD)0, (DWORD)0 },          // double-null terminator NECESSARY!
};



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

HRESULT ExtractPlusColorIcon(LPCSTR szPath, int nIndex, HICON *phIcon, UINT uSizeLarge, UINT uSizeSmall);
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
            (*m_pfnDestroy)();
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

    psp.dwSize = sizeof(PROPSHEETPAGE);
//  psp.dwFlags = PSP_USETITLE | PSP_HASHELP;
    psp.dwFlags = PSP_USETITLE;
    psp.hIcon = NULL;
    psp.hInstance = g_hInst;
    psp.pszTemplate =MAKEINTRESOURCE( PLUSPACK_DLG );
    psp.pfnDlgProc = (DLGPROC)PlusPackDlgProc;
    psp.pszTitle = "Plus!";
    psp.lParam = 0;

    if( ( hpage = CreatePropertySheetPage( &psp ) ) == NULL )
        return ( E_OUTOFMEMORY );

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
static int     iOldLI, iNewLI;      // Large Icon State
static BOOL     bOldFWD, bNewFWD;       // Full Window Dragging
static BOOL     bOldFS, bNewFS;     // Font Smoothing
static int      iOldHIC, iNewHIC;       // High Icon Colour
static int      iOldSWFS, iNewSWFS; // Stretch Wallpaper to Fit Screen
BOOL bDorked = FALSE, bRet, bDorkedIcons;
static int iIndex, iX;
static char szHelpFile[32];


    switch( uMessage )
    {
        case WM_INITDIALOG:
        {
            // Create our list view and fill it with the system icons
//          hWndListView = CreateListView( hDlg );
            CreateListView( hDlg );
         iIndex = 0;

            SetWindowLong( hDlg, DWL_USER, lParam );
            psp = (LPPROPSHEETPAGE)lParam;
//          EnableWindow( GetDlgItem( hDlg, IDC_CHANGEICON ), FALSE );  // Nothing selected in our listview yet
//          EnableWindow( GetDlgItem( hDlg, IDC_ICONDEFAULT ), FALSE ); // Nothing selected in our listview yet

            // Get the name of our help file
            LoadString( g_hInst, IDS_HELPFILE, szHelpFile, 32 );

            // Get the values for the settings from the registry and set the checkboxes

         // Large Icons
         iOldLI = GetIconState ();
         if (iOldLI == ICON_INDETERMINATE)
            {
            HWND hItem = GetDlgItem (hDlg, IDC_LARGEICONS);
            SendMessage (hItem, BM_SETSTYLE, (WPARAM)LOWORD(BS_AUTO3STATE), MAKELPARAM( FALSE,0));
            }
            iNewLI = iOldLI;
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_LARGEICONS ), BM_SETCHECK, (WPARAM)iOldLI, 0 );

            // Full Window Dragging
//          bRet = SystemParametersInfo( SPI_GETDRAGFULLWINDOWS, 0, (PVOID)&bOldFWD, 0 );
            bRet = GetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_FWD, HVALUE_FWD, &bOldFWD );
            if( bRet == FALSE )
                bOldFWD = FALSE;

            bNewFWD = bOldFWD;
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_FULLWINDOWDRAGGING ), BM_SETCHECK, (WPARAM)bOldFWD, 0 );

            // Stretch Window to Fit Screen
            bRet = GetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_SWFS, HVALUE_SWFS, &iOldSWFS );
            if( bRet == FALSE ) // Key not in registry yet
                iOldSWFS = FALSE;

            iNewSWFS = iOldSWFS;
            if( iOldSWFS == 2 )
                SendMessage( (HWND)GetDlgItem( hDlg, IDC_STRETCHWALLPAPERFITSCREEN ), BM_SETCHECK, (WPARAM)TRUE, 0 );
            else
                SendMessage( (HWND)GetDlgItem( hDlg, IDC_STRETCHWALLPAPERFITSCREEN ), BM_SETCHECK, (WPARAM)FALSE, 0 );

            // Font Smoothing
            bRet = SystemParametersInfo( SPI_GETFONTSMOOTHING, 0, (PVOID)&bOldFS, 0 );
//          bRet = GetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_FS, HVALUE_FS, &bOldFS );
            if( bRet == FALSE )
                bOldFS = FALSE;

            bNewFS = bOldFS;
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_FONTSMOOTHING ), BM_SETCHECK, (WPARAM)bOldFS, 0 );

            // Full Color Icons
            bRet = GetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_HIC, HVALUE_HIC, &iOldHIC );
            if( bRet == FALSE ) // Key not in registry yet
                iOldHIC = iNewHIC = 4;

            iNewHIC = iOldHIC;
            if( iOldHIC == 16 )
                SendMessage( (HWND)GetDlgItem( hDlg, IDC_ICONHIGHCOLOR ), BM_SETCHECK, (WPARAM)TRUE, 0 );
            else
                SendMessage( (HWND)GetDlgItem( hDlg, IDC_ICONHIGHCOLOR ), BM_SETCHECK, (WPARAM)FALSE, 0 );

            // these two lines fix bug #3214
            DWORD dwStyle = GetWindowLong( hWndList, GWL_STYLE );
            SetWindowLong( hWndList, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | LVS_ICON );

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

                case IDC_FULLWINDOWDRAGGING:
                    bNewFWD = SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 );
                    bDorked = TRUE;
                    break;

                case IDC_FONTSMOOTHING:
                    bNewFS = SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 );
                    bDorked = TRUE;
                    break;

                case IDC_ICONHIGHCOLOR:
                    iNewHIC = 4;
                    if( SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == TRUE )
                        iNewHIC = 16;
                    bDorked = TRUE;
                    break;

                case IDC_STRETCHWALLPAPERFITSCREEN:
                    iNewSWFS = FALSE;
                    if( SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == TRUE )
                        iNewSWFS = 2;
                    bDorked = TRUE;
                    break;

                case IDC_CHANGEICON:
                    if( PickIconDlg( hDlg, sIconData[iIndex].szNewFile, sizeof(sIconData[iIndex].szNewFile), &(sIconData[iIndex].iNewIndex) ) == TRUE )
                    {
                        HICON hIcon;
                        ExtractPlusColorIcon( sIconData[iIndex].szNewFile, sIconData[iIndex].iNewIndex, &hIcon, 0, 0);

                        ImageList_ReplaceIcon( hIconList, iIndex, hIcon );
                        ListView_RedrawItems( hWndList, iIndex, iIndex );
                        bDorked = TRUE;
                    }
                    break;

                case IDC_ICONDEFAULT:
                    {
                    TCHAR szTemp[_MAX_PATH];

                        if( iIndex == 0 )       // 0 for explorer.exe, anything else is shell32.dll
                            GetWindowsDirectory( szTemp, sizeof(szTemp) );
                        else
                            GetSystemDirectory( szTemp, sizeof(szTemp) );

                        lstrcat( szTemp,  sDefaultIcons[iIndex].szFile );
                        lstrcpy( sIconData[iIndex].szNewFile, szTemp );
                        sIconData[iIndex].iNewIndex = sDefaultIcons[iIndex].iIndex;

                        // Set the Default recycle bin if necessary
                        if( iIndex > 1 )
                        {
//                          if( (lstrcmpi( sIconData[iIndex].szOldFile, sIconData[NUM_ICONS].szOldFile ) == 0) && (sIconData[iIndex].iOldIndex == sIconData[NUM_ICONS].iOldIndex) )
                            {
                                lstrcpy( sIconData[NUM_ICONS].szNewFile, sIconData[iIndex].szNewFile );
                                sIconData[NUM_ICONS].iNewIndex = sIconData[iIndex].iNewIndex;
                            }
                        }

                        HICON hIcon;
                        ExtractPlusColorIcon( sIconData[iIndex].szNewFile, sIconData[iIndex].iNewIndex, &hIcon, 0, 0);

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
                SendMessage( GetParent( hDlg ), PSM_CHANGED, (WPARAM)hDlg, 0L );
            break;

        case WM_NOTIFY:
            switch( ((NMHDR *)lParam)->code )
            {
                case LVN_ITEMCHANGED:   // The selection changed in our listview
                    if( wParam == IDC_ICONS )
                    {
                        BOOL bEnable = FALSE;

                        // Find out who's selected now
                        for( iIndex = 0;iIndex < NUM_ICONS;iIndex++ )
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

                    // Full Window Dragging
                    if( bOldFWD != bNewFWD )
                    {
                        bRet = SetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_FWD, HVALUE_FWD, bNewFWD );
                        bRet = SystemParametersInfo( SPI_SETDRAGFULLWINDOWS, bNewFWD, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE );
                        bOldFWD = bNewFWD;
                    }
                    // Font Smoothing
                    if( bOldFS != bNewFS )
                    {
                        char szTemp1[512];
                        char szTemp2[256];

                        bRet = SetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_FS, HVALUE_FS, bNewFS );
                        bRet = SystemParametersInfo( SPI_SETFONTSMOOTHING, bNewFS, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE );
                        bOldFS = bNewFS;
                        if ((iBitsPerPixel < 16) && bNewFS) // Display mode won't support font smoothing
                        {
                            LoadString (g_hInst, IDS_256COLORPROBLEM, szTemp1, 512 );
                            LoadString (g_hInst, IDS_FONTSMOOTHWONTWORK, szTemp2, 256);
                            lstrcat (szTemp1, szTemp2);
                            MessageBox (hDlg, szTemp1, "Plus!", MB_OK );
                        }

                    }
                    // Full Color Icons
                    if( iOldHIC != iNewHIC )
                    {
                        char szTemp1[512];
                        char szTemp2[256];
                        bRet = SetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_HIC, HVALUE_HIC, iNewHIC );
                        iOldHIC = iNewHIC;
                        if ((iBitsPerPixel < 16) && (iNewHIC == 16)) // Display mode won't support icon high colors
                        {
                            LoadString (g_hInst, IDS_256COLORPROBLEM, szTemp1, 512 );
                            LoadString( g_hInst, IDS_ICONCOLORWONTWORK, szTemp2, 256 );
                            lstrcat (szTemp1, szTemp2);
                            MessageBox( hDlg, szTemp1, "Plus!", MB_OK );
                        }
                        else
                        {
                           LoadString( g_hInst, IDS_REBOOTFORCHANGE, szTemp1, 256 );
                           MessageBox( hDlg, szTemp1, "Plus!", MB_OK );
                        }
                    }
                    // Stretch Window to Fit Screen
                    if( iOldSWFS != iNewSWFS )
                    {
                    char szTemp[MAX_PATH];
                        bRet = SetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_SWFS, HVALUE_SWFS, iNewSWFS );
                        bRet = GetRegValueString( HKEY_CURRENT_USER, "Control Panel\\Desktop", "Wallpaper", (LPSTR)szTemp, MAX_PATH );
                        bRet = SystemParametersInfo( SPI_SETDESKWALLPAPER, 0, szTemp, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE );
                        iOldSWFS = iNewSWFS;
                    }

                    // Change the system icons
                    for( iX = 0;iX < NUM_ICONS;iX++ )
                    {
                        if( (lstrcmpi( sIconData[iX].szNewFile, sIconData[iX].szOldFile ) != 0) || (sIconData[iX].iNewIndex != sIconData[iX].iOldIndex) )
                        {
                        char szTemp[MAX_PATH];
                            wsprintf( szTemp, "%s,%d", sIconData[iX].szNewFile, sIconData[iX].iNewIndex );
                            bRet = IconSetRegValueString( sIconRegKeys[iX].szIconSubKey, sIconRegKeys[iX].szIconValue, (LPSTR)szTemp );

                            // Set the Default recycle bin
                            if (pfnSHUpdateRecycleBinIcon == NULL) {
                                if( (lstrcmpi( sIconData[iX].szOldFile, sIconData[NUM_ICONS].szOldFile ) == 0) && (sIconData[iX].iOldIndex == sIconData[NUM_ICONS].iOldIndex) )
                                {
                                    bRet = IconSetRegValueString( sIconRegKeys[NUM_ICONS].szIconSubKey, sIconRegKeys[NUM_ICONS].szIconValue, (LPSTR)szTemp );
                                    lstrcpy( sIconData[NUM_ICONS].szOldFile, sIconData[iX].szNewFile );
                                    // Necessary if the user does an Apply as opposed to OK
                                    sIconData[NUM_ICONS].iOldIndex = sIconData[iX].iNewIndex;
                                }
                            }
                            // Next two lines necessary if the user does an Apply as opposed to OK
                            lstrcpy( sIconData[iX].szOldFile, sIconData[iX].szNewFile );
                            sIconData[iX].iOldIndex = sIconData[iX].iNewIndex;
                            bDorked = TRUE;
                        }
                    }

                    // Make the system notice we changed the system icons
                    if( bDorked ) {
                        SHChangeNotify( SHCNE_ASSOCCHANGED, 0, NULL, NULL ); // should do the trick!

                        if (pfnSHUpdateRecycleBinIcon != NULL) {
                            pfnSHUpdateRecycleBinIcon();
                        }
                    }

                    if (bDorkedIcons)
                        SendMessage (HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);

//                  if( (iOldHIC != iNewHIC) || (iOldSWFS != iNewSWFS) )
//                  {
//                      SHChangeNotify( SHCNE_ASSOCCHANGED, 0, NULL, NULL ); // should do the trick!
//                  }
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
                                WinHelp( (HWND)lphi->hItemHandle, (LPSTR)szHelpFile, HELP_WM_HELP,(DWORD)((POPUP_HELP_ARRAY FAR *)phaMainWin) );
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
                // else go for the controls
                WinHelp( (HWND)wParam, (LPSTR)szHelpFile, HELP_CONTEXTMENU,(DWORD)((POPUP_HELP_ARRAY FAR *)phaMainWin) );
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
char szTemp[MAX_PATH];

 /* Create a device independant size and location                       */
LONG lWndunits = GetDialogBaseUnits();
int iWndx = LOWORD(lWndunits);
int iWndy = HIWORD(lWndunits);
int iX = ((11 * iWndx) / 4);
int iY = ((15 * iWndy) / 8);
int iWidth = ((163 * iWndx) / 4);
int iHeight = ((40 * iWndy) / 8);

    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    // create the list view window
    // changed LVS_ICON to LVS_REPORT to fix bug #3214
//  hWndList = CreateWindowEx( WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
//      iX, iY, iWidth, iHeight, hWndParent, (HMENU)IDC_ICONS, g_hInst, NULL );

    // Get the list view window
   hWndList = GetDlgItem (hWndParent, IDC_ICONS);
    if( hWndList == NULL  )
        return NULL;

    // initialize the list view window
    // First, initialize the image lists we will need
    hIconList = ImageList_Create( 32, 32, ILC_MASK | ILC_COLOR24, NUM_ICONS, 0 );   // create an image list for the icons
//  hIconList = ImageList_Create( 32, 32, ILC_MASK, NUM_ICONS, 0 ); // create an image list for the icons

    // load the icons and add them to the image lists
    // get the icon files and indexes from the registry, including for the Default recycle bin
    for( iX = 0; iX < NUM_ICONS+1; iX++ )
    {
        BOOL bRet = IconGetRegValueString( sIconRegKeys[iX].szIconSubKey, sIconRegKeys[iX].szIconValue, (LPSTR)szTemp, MAX_PATH );
        int iIndex = 0;
        char* pPos;

        if( (pPos = strchr( szTemp, ',' )) != NULL )
        {
            *pPos = '\0';
            *pPos++;
            iIndex = atoi( pPos );
        }
        // store the icon information
        lstrcpy( sIconData[iX].szOldFile, szTemp );
        lstrcpy( sIconData[iX].szNewFile, szTemp );
        sIconData[iX].iOldIndex = iIndex;
        sIconData[iX].iNewIndex = iIndex;

        HICON hIcon;
        ExtractPlusColorIcon( szTemp, iIndex, &hIcon, 0, 0);

        // Added this "if" to fix bug 2831.  We want to use SHELL32.DLL
        // icon 0 if there is no icon in the file specified in the
        // registry (or if the registry didn't specify a file).
        if( hIcon == NULL )
        {
            GetSystemDirectory( szTemp, sizeof(szTemp) );
            lstrcat( szTemp,  "\\shell32.dll" );
            lstrcpy( sIconData[iX].szOldFile, szTemp );
            lstrcpy( sIconData[iX].szNewFile, szTemp );
            sIconData[iX].iOldIndex = sIconData[iX].iNewIndex = 0;

            hIcon = ExtractIcon( g_hInst, szTemp, iIndex );
        }

        if( iX < NUM_ICONS ) // We don't need to add an icon for the Default recycle bin
        {
            if( ImageList_AddIcon( hIconList, hIcon ) == -1 )
                return NULL;
        }
    }

    // Make sure that all of the icons were added
    if( ImageList_GetImageCount( hIconList ) < NUM_ICONS )
        return FALSE;

    ListView_SetImageList( hWndList, hIconList, LVSIL_NORMAL );

    // Finally, let's add the actual items to the control.  Fill in the LV_ITEM
    // structure for each of the items to add to the list.  The mask specifies
    // the the .pszText, .iImage, and .state members of the LV_ITEM structure are valid.
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.state = 0;      //
    lvI.stateMask = 0;  //

    for( iX = 0; iX < NUM_ICONS; iX++ )
    {
    BOOL bRet = IconGetRegValueString( sIconRegKeys[iX].szTitleSubKey, NULL, (LPSTR)szTemp, MAX_PATH );
    char szAppend[64];

        // if the title string was in the registry, else we have to use the default in our resources
        if( (bRet) && (lstrlen(szTemp) > 0) )
        {
            if( LoadString( g_hInst, sIconRegKeys[iX].iTitleResource, szAppend, 64 ) != 0 )
                lstrcat( szTemp, szAppend );
        }
        else
            LoadString( g_hInst, sIconRegKeys[iX].iDefaultTitleResource, szTemp, MAX_PATH );

        lvI.iItem = iX;
        lvI.iSubItem = 0;
        lvI.pszText = (LPSTR)&(szTemp);
        lvI.cchTextMax = lstrlen(szTemp);
        lvI.iImage = iX;

        if( ListView_InsertItem( hWndList, &lvI ) == -1 )
            return NULL;

    }

      // Set First item to selected
   ListView_SetItemState (hWndList, 0, LVIS_SELECTED, LVIS_SELECTED);

      // Get Selected item
   BOOL bEnable = FALSE;
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
        iHorz = 0;
    iHorz += iNewSize;

        // Get new vertical spacing
    iVert = GetSystemMetrics (SM_CYICONSPACING);
    iVert -= iOldSize;
    if (iVert < 0)
        iVert = 0;
    iVert += iNewSize;

        // Set New sizes and spacing
    bRet = SetRegValueInt( HKEY_CURRENT_USER, HSUBKEY_WM, HVALUE_WMSIS, iNewSize );
    if (!bRet)
        return FALSE;

    SystemParametersInfo (SPI_ICONHORIZONTALSPACING, (WPARAM)iHorz, NULL, SPIF_UPDATEINIFILE);
    SystemParametersInfo (SPI_ICONVERTICALSPACING, (WPARAM)iVert, NULL, SPIF_UPDATEINIFILE);

        // Turn from Tri-State back to normal check box
    if (iOldState == ICON_INDETERMINATE)
        {
        HWND hItem = GetDlgItem (hDlg, IDC_LARGEICONS);
        SendMessage (hItem, BM_SETSTYLE, (WPARAM)LOWORD(BS_AUTOCHECKBOX), MAKELPARAM( FALSE,0));
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
HRESULT ExtractPlusColorIcon(LPCSTR szPath, int nIndex, HICON *phIcon,
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
