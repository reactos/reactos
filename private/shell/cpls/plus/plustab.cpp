//---------------------------------------------------------------------------
//
//  File: plustab.cpp
//
//  Main Implementation of the Effects page
//
//---------------------------------------------------------------------------


#include "precomp.hxx"
#include "shlwapip.h"
#pragma hdrstop

// OLE-Registry magic number
GUID g_CLSID_CplExt = { 0x41e300e0, 0x78b6, 0x11ce,
                        { 0x84, 0x9b, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
                      };

//need to be L"..." since SHGetRestriction takes only LPCWSTR and this file is compiled as ANSI
#define POLICY_KEY_EXPLORER       L"Explorer"
#define POLICY_VALUE_ANIMATION    L"NoChangeAnimation"
#define POLICY_VALUE_KEYBOARDNAV  L"NoChangeKeyboardNavigationIndicators"

#define SPI_GETKEYBOARDINDICATORS SPI_GETMENUUNDERLINES//0x100A
#define SPI_SETKEYBOARDINDICATORS SPI_SETMENUUNDERLINES//0x100B

#define SPI_GETFONTCLEARTYPE      116
#define SPI_SETFONTCLEARTYPE      117

//Defines for getting registry settings
const TCHAR c_szHICKey[] = TEXT("Control Panel\\Desktop\\WindowMetrics"); // show icons using highest possible colors
const TCHAR c_szHICVal[] = TEXT("Shell Icon BPP"); // (4 if the checkbox is false, otherwise 16, don't set it to anything else)
const TCHAR c_szSSKey[]  = TEXT("Control Panel\\Desktop");
const TCHAR c_szSSVal[]  = TEXT("SmoothScroll");
const TCHAR c_szWMSISVal[] = TEXT("Shell Icon Size"); // Normal Icon Size (default 32 x 32)
#ifdef INET_EXP_ICON
const TCHAR c_szIEXP[]   = TEXT("\\Program Files\\Microsoft Internet Explorer 4");
#endif

#define ICON_DEFAULT_SMALL    16
#define ICON_DEFAULT_NORMAL   32
#define ICON_DEFAULT_LARGE    48

enum ICON_SIZE_TYPES {
   ICON_DEFAULT         = 0,
   ICON_LARGE           = 1,
   ICON_INDETERMINATE   = 2
};

#define MENU_EFFECT_FADE        1
#define MENU_EFFECT_SCROLL      2

#define FONT_SMOOTHING_STANDARD    0
#define FONT_SMOOTHING_CLEARTYPE   1


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

POPUP_HELP_ARRAY phaMainDisplay[] = {
   { (DWORD)IDC_ICONS,              (DWORD)IDH_DISPLAY_EFFECTS_DESKTOP_ICONS },
   { (DWORD)IDC_CHANGEICON,         (DWORD)IDH_DISPLAY_EFFECTS_CHANGE_ICON_BUTTON },
   { (DWORD)IDC_LARGEICONS,         (DWORD)IDH_DISPLAY_EFFECTS_LARGE_ICONS_CHECKBOX },
   { (DWORD)IDC_ICONHIGHCOLOR,      (DWORD)IDH_DISPLAY_EFFECTS_ALL_COLORS_CHECKBOX  },
   { (DWORD)IDC_ICONDEFAULT,        (DWORD)IDH_DISPLAY_EFFECTS_DEFAULT_ICON_BUTTON },
   { (DWORD)IDC_MENUANIMATION,      (DWORD)IDH_DISPLAY_EFFECTS_ANIMATE_WINDOWS },
   { (DWORD)IDC_FONTSMOOTH,         (DWORD)IDH_DISPLAY_EFFECTS_SMOOTH_FONTS_CHECKBOX },
   { (DWORD)IDC_SHOWDRAG,           (DWORD)IDH_DISPLAY_EFFECTS_DRAG_WINDOW_CHECKBOX },
   { (DWORD)IDC_KEYBOARDINDICATORS, (DWORD)IDH_DISPLAY_EFFECTS_HIDE_KEYBOARD_INDICATORS },
   { (DWORD)IDC_GRPBOX_1,           (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)IDC_GRPBOX_2,           (DWORD)IDH_COMM_GROUPBOX                 },
   { (DWORD)IDC_COMBOEFFECT,        (DWORD)IDH_DISPLAY_EFFECTS_ANIMATE_LISTBOX },
   { (DWORD)IDC_COMBOFSMOOTH,       (DWORD)IDH_DISPLAY_EFFECTS_SMOOTH_FONTS_LISTBOX },
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

BOOL g_bMirroredOS = FALSE;
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

// animation stuff
WPARAM GetAnimations(DWORD *pdwEffect);
void SetAnimations(WPARAM wVal, DWORD dwEffect);

BOOL DisplayFontSmoothingDetails(DWORD *pdwSetting)
{
    return FALSE;
}

BOOL FadeEffectAvailable()
{
    BOOL fFade = FALSE, fTestFade = FALSE;
    
    SystemParametersInfo( SPI_GETMENUFADE, 0, (PVOID)&fFade, 0 );
    if (fFade) 
        return TRUE;
    
    SystemParametersInfo( SPI_SETMENUFADE, 0, (PVOID)1, 0);
    SystemParametersInfo( SPI_GETMENUFADE, 0, (PVOID)&fTestFade, 0 );
    SystemParametersInfo( SPI_SETMENUFADE, 0, (PVOID)fFade, 0);

    return (fTestFade);
}

//---------------------------------------------------------------------------
//
// PropertySheeDlgProc()
//
//  The dialog procedure for the "PlusPack" property sheet page.
//
//---------------------------------------------------------------------------
BOOL CALLBACK PropertySheeDlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)GetWindowLongPtr( hDlg, DWLP_USER );
    static int      iOldLI, iNewLI;         // Large Icon State
    static int      iOldHIC, iNewHIC;       // High Icon Colour
    static WPARAM   wOldMA, wNewMA;         // Menu Animation State
    static BOOL     bOldSF, bNewSF;         // Font Smoothing State
    static DWORD    dwOldSFT, dwNewSFT;     // Font Smoothing Type
    static BOOL     bOldDW, bNewDW;         // Drag Window State
    static BOOL     uOldKI, uNewKI;         // Keyboard Indicators
    BOOL            bDorked = FALSE, bRet;
    static int      iIndex, iX;
    static TCHAR    szHelpFile[32];
    TCHAR           szRes[100];
    HWND            hwndCombo;
    static DWORD    dwOldEffect, dwNewEffect; 

    switch( uMessage )
    {
        case WM_INITDIALOG:
        {
            UINT id = IDS_HELPFILE_PLUS;
    
            g_bMirroredOS = IS_MIRRORING_ENABLED();
            // Create our list view and fill it with the system icons
            CreateListView( hDlg );
            iIndex = 0;

            SetWindowLongPtr( hDlg, DWLP_USER, lParam );
            psp = (LPPROPSHEETPAGE)lParam;

            // Get the name of our help file.  For Memphis, it's
            // IDS_HELPFILE_PLUS for NT it's IDS_HELPFILE.
            g_phaHelp = phaMainWinPlus;

            // If running on NT...
            if ((int)GetVersion() >= 0)
            {
                id = IDS_HELPFILE;
                g_phaHelp = phaMainDisplay;
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
                                   c_szHICKey,
                                   c_szHICVal,
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

            
            hwndCombo = GetDlgItem(hDlg,IDC_COMBOEFFECT); 
            ComboBox_ResetContent(hwndCombo);
            LoadString (g_hInst, IDS_FADEEFFECT, szRes, ARRAYSIZE(szRes) );
            ComboBox_AddString(hwndCombo, szRes);
            LoadString (g_hInst, IDS_SCROLLEFFECT, szRes, ARRAYSIZE(szRes) );
            ComboBox_AddString(hwndCombo, szRes);

            // Use animations
            wOldMA = GetAnimations(&dwOldEffect);
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_MENUANIMATION ),
                         BM_SETCHECK,
                         (WPARAM)wOldMA,
                         0
                        );

            ComboBox_SetCurSel(hwndCombo, (MENU_EFFECT_FADE == dwOldEffect) ? 0 : 1);
             
            wNewMA = wOldMA;
            dwNewEffect = dwOldEffect;
            
            EnableWindow( GetDlgItem( hDlg, IDC_COMBOEFFECT ),
                (UINT)wOldMA);

            if (!FadeEffectAvailable()) 
                ShowWindow(GetDlgItem( hDlg, IDC_COMBOEFFECT ), SW_HIDE);

            if(0!=SHGetRestriction(NULL,POLICY_KEY_EXPLORER,POLICY_VALUE_ANIMATION))
            {//disable
                //0=     enable
                //non-0= disable
                //relies on the fact that if the key does not exist it returns 0 as well
                EnableWindow( (HWND)GetDlgItem( hDlg, IDC_MENUANIMATION ),
                    FALSE);
                EnableWindow( (HWND)GetDlgItem( hDlg, IDC_COMBOEFFECT ),
                    FALSE);
            }

            hwndCombo = GetDlgItem(hDlg,IDC_COMBOFSMOOTH); 
#ifdef CLEARTYPECOMBO
            ComboBox_ResetContent(hwndCombo);
            LoadString (g_hInst, IDS_STANDARDSMOOTHING, szRes, ARRAYSIZE(szRes) );
            ComboBox_AddString(hwndCombo, szRes);
            LoadString (g_hInst, IDS_CLEARTYPE, szRes, ARRAYSIZE(szRes) );
            ComboBox_AddString(hwndCombo, szRes);
#else
            ShowWindow(hwndCombo, SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_SHOWME), SW_HIDE);
#endif //CLEARTYPECOMBO

            // Smooth edges of screen fonts
            bOldSF = FALSE;
            SystemParametersInfo( SPI_GETFONTSMOOTHING, 0, (PVOID)&bOldSF, 0 );
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_FONTSMOOTH ),
                         BM_SETCHECK,
                         (WPARAM)bOldSF,
                         0
                        );
            bNewSF = bOldSF;

            dwOldSFT = FONT_SMOOTHING_STANDARD;
#ifdef CLEARTYPECOMBO
            if (SystemParametersInfo( SPI_GETFONTCLEARTYPE, 0, (PVOID)&bSmoothType, 0 )) 
            {
                dwOldSFT = (bSmoothType ? FONT_SMOOTHING_CLEARTYPE : FONT_SMOOTHING_STANDARD);
                ComboBox_SetCurSel(hwndCombo, dwOldSFT);
                EnableWindow((HWND)hwndCombo, bOldSF);
            }
            else
            {
                ComboBox_SetCurSel(hwndCombo, FONT_SMOOTHING_STANDARD);
                ShowWindow(hwndCombo, SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_SHOWME), SW_HIDE);
            }
#endif //CLEARTYPECOMBO
            dwNewSFT = dwOldSFT;
            
            // Show contents while dragging
            bOldDW = FALSE;
            SystemParametersInfo( SPI_GETDRAGFULLWINDOWS, 0, (PVOID)&bOldDW, 0 );
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_SHOWDRAG ),
                         BM_SETCHECK,
                         (WPARAM)bOldDW,
                         0
                        );
            bNewDW = bOldDW;

            
            uOldKI = FALSE;
            SystemParametersInfo( SPI_GETKEYBOARDINDICATORS, 0, (PVOID)&uOldKI, 0 );
            SendMessage( (HWND)GetDlgItem( hDlg, IDC_KEYBOARDINDICATORS ),
                         BM_SETCHECK,
                         (WPARAM)(uOldKI ? BST_UNCHECKED : BST_CHECKED),
                         0
                        );
            uNewKI = uOldKI;

            if(0!=SHGetRestriction(NULL,POLICY_KEY_EXPLORER,POLICY_VALUE_KEYBOARDNAV))
            {//disable, see comment for animation
                EnableWindow( (HWND)GetDlgItem( hDlg, IDC_KEYBOARDINDICATORS ),
                    FALSE);
            }

            // Load SHUpdateRecycleBinIcon() if it exists
            hmodShell32 = LoadLibrary(SZ_SHELL32);
            pfnSHUpdateRecycleBinIcon = (PFNSHUPDATERECYCLEBINICON)GetProcAddress( hmodShell32, SZ_SHUPDATERECYCLEBINICON );

            //disable and uncheck things if we are on terminal server
            BOOL bEffectsEnabled;
            SystemParametersInfo(SPI_GETUIEFFECTS, 0, (PVOID)&bEffectsEnabled, 0);
            if (!bEffectsEnabled || SHGetMachineInfo(GMI_TSCLIENT))
            {
                EnableWindow( (HWND)GetDlgItem( hDlg, IDC_MENUANIMATION ),
                    FALSE);
                EnableWindow( (HWND)GetDlgItem( hDlg, IDC_ICONHIGHCOLOR ),
                    FALSE);
                EnableWindow( (HWND)GetDlgItem( hDlg, IDC_KEYBOARDINDICATORS ),
                    FALSE);
                EnableWindow( (HWND)GetDlgItem( hDlg, IDC_FONTSMOOTH),
                    FALSE);
                ShowWindow(GetDlgItem( hDlg, IDC_COMBOEFFECT ), SW_HIDE);
                SendDlgItemMessage(hDlg, IDC_MENUANIMATION, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hDlg, IDC_ICONHIGHCOLOR, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hDlg, IDC_KEYBOARDINDICATORS, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hDlg, IDC_FONTSMOOTH, BM_SETCHECK, 0, 0);
                   
            }
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
                    iNewLI = (int)SendMessage ( (HWND)lParam, BM_GETCHECK, 0, 0 );
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

                case IDC_MENUANIMATION:
                    switch( wNewMA )
                    {
                    case BST_UNCHECKED:
                        wNewMA = BST_CHECKED;
                        break;

                    case BST_CHECKED:
                        wNewMA = BST_UNCHECKED;
                        break;

                    case BST_INDETERMINATE:
                        wNewMA = BST_UNCHECKED;
                        break;
                    }
                    SendMessage( (HWND)lParam, BM_SETCHECK, (WPARAM)wNewMA, 0 );
                    EnableWindow((HWND)GetDlgItem( hDlg, IDC_COMBOEFFECT), (BST_CHECKED == wNewMA));
                    bDorked = TRUE;
                    break;

                case IDC_COMBOEFFECT:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        dwNewEffect = (DWORD)ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBOEFFECT)) + 1;
                        bDorked = TRUE;
                    }
                    break;
                    
#ifdef CLEARTYPECOMBO
                case IDC_COMBOFSMOOTH:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        dwNewSFT = (DWORD)ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_COMBOFSMOOTH));
                        bDorked = TRUE;
                    }
                    break;
#endif                    
                case IDC_FONTSMOOTH:
                    bNewSF = (SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
#ifdef CLEARTYPECOMBO
                    EnableWindow((HWND)GetDlgItem( hDlg, IDC_COMBOFSMOOTH), bNewSF);
#endif
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
                        SHTCharToUnicode(szExp, szTemp, ARRAYSIZE(szTemp));
                    }
                    else
                    {
                        SHTCharToAnsi(szExp, (LPSTR)szTemp, ARRAYSIZE(szTemp));
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
                            SHUnicodeToTChar(szTemp,
                                             sIconData[iIndex].szNewFile,
                                             ARRAYSIZE(sIconData[iIndex].szNewFile));
                        }
                        else
                        {
                            SHAnsiToTChar((LPSTR)szTemp,
                                          sIconData[iIndex].szNewFile,
                                          ARRAYSIZE(sIconData[iIndex].szNewFile));
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
                    SetFocus( hWndList );
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
                        SetFocus( hWndList );
                    }
                    break;

                case IDC_KEYBOARDINDICATORS:
                    uNewKI = ((SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED)?
                                                FALSE : TRUE);
                    bDorked = TRUE;
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
#ifdef CLEARTYPECOMBO
                case NM_CLICK:
                    switch (wParam)
                    {
                        case IDC_SHOWME:
                            DWORD dwSmoothingSetting = dwNewSFT;
                            if (DisplayFontSmoothingDetails(&dwSmoothingSetting) &&
                                dwNewSFT != dwSmoothingSetting)
                            {
                                dwNewSFT = dwSmoothingSetting;
                                bDorked = TRUE;
                                SendMessage( GetParent( hDlg ), PSM_CHANGED, (WPARAM)hDlg, 0L );

                                // TODO: reset the controls
                            }
                            break;
                    }
                    break;
#endif                    
                case LVN_ITEMCHANGED:   // The selection changed in our listview
                    if( wParam == IDC_ICONS )
                    {
                        // Find out who's selected now
                        for( iIndex = 0; iIndex < NUM_ICONS;iIndex++ )
                        {
                            if( ListView_GetItemState( hWndList, iIndex, LVIS_SELECTED ) )
                            {
                                break;
                            }
                        }

                    }
                    break;

                case PSN_APPLY: // OK or Apply clicked
                {
                    HDC hDC = GetDC( NULL );
                    int iBitsPerPixel;

                    iBitsPerPixel = GetDeviceCaps( hDC, BITSPIXEL );
                    ReleaseDC( NULL, hDC );

                    // Large Icons
                    BOOL bSendSettingsChange = ChangeIconSizes (hDlg, iOldLI, iNewLI);
                    if (bSendSettingsChange)
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
                                               c_szHICKey,
                                               c_szHICVal,
                                               iNewHIC
                                              );
                        iOldHIC = iNewHIC;
                        if ((iBitsPerPixel < 16) && (iNewHIC == 16)) // Display mode won't support icon high colors
                        {
                            LoadString (g_hInst, IDS_256COLORPROBLEM, szTemp1, ARRAYSIZE(szTemp1) );
                            LoadString( g_hInst, IDS_ICONCOLORWONTWORK, szTemp2, ARRAYSIZE(szTemp2) );
                            lstrcat (szTemp1, szTemp2);

                            LoadString(g_hInst, IDS_EFFECTS, szTemp2, ARRAYSIZE(szTemp2) );

                            MessageBox( hDlg, szTemp1, szTemp2, MB_OK|MB_ICONINFORMATION );
                        }
                        else
                        {
                           bSendSettingsChange = TRUE;
                        }
                    }

                    // Full window drag
                    if ( bOldDW != bNewDW )
                    {
                        bOldDW = bNewDW;
                        SystemParametersInfo( SPI_SETDRAGFULLWINDOWS,
                                              bNewDW,
                                              0,
                                              SPIF_UPDATEINIFILE
                                             );
                        // we need to send this because the tray's autohide switches off this
                        bSendSettingsChange = TRUE;
                    }

                    // Font smoothing
                    if ( bOldSF != bNewSF || dwOldSFT != dwNewSFT)
                    {

#ifdef CLEARTYPECOMBO
                        BOOL bST;

                        bST = (dwNewSFT == FONT_SMOOTHING_CLEARTYPE);
                        SystemParametersInfo( SPI_SETFONTCLEARTYPE,
                                              bST,
                                              0,
                                              SPIF_UPDATEINIFILE
                                             );
#endif
                        dwOldSFT = dwNewSFT;
                        bOldSF = bNewSF;
                        SystemParametersInfo( SPI_SETFONTSMOOTHING,
                                              bNewSF,
                                              0,
                                              SPIF_UPDATEINIFILE
                                             );
                    }

                    // Menu animations
                    if ( wOldMA != wNewMA || dwOldEffect != dwNewEffect)
                    {
                        wOldMA = wNewMA;
                        SetAnimations( wNewMA, dwNewEffect );

                        dwOldEffect = dwNewEffect;

                    }


                    // Change the system icons
                    for( iX = 0;iX < NUM_ICONS;iX++ )
                    {

                        if( (lstrcmpi( sIconData[iX].szNewFile, sIconData[iX].szOldFile ) != 0) ||
                            (sIconData[iX].iNewIndex != sIconData[iX].iOldIndex)
                           )
                        {
                            TCHAR   szTemp[MAX_PATH];

                            wnsprintf( szTemp, ARRAYSIZE(szTemp),
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

                    // Keyboard indicators
                    if ( uOldKI != uNewKI )
                    {
                        uOldKI = uNewKI;

                        DWORD_PTR dwResult;

                        // Are we turning this on? (!uNewKI means "don't show" -> hide)
                        if (!uNewKI)
                        {
                            // Yes, on: hide the key cues, turn on the mechanism
                            SystemParametersInfo(SPI_SETKEYBOARDINDICATORS, 0,
                               (PVOID)uNewKI, SPIF_UPDATEINIFILE);

                            SendMessageTimeout(HWND_BROADCAST, WM_CHANGEUISTATE, 
                                MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL),
                                0, SMTO_ABORTIFHUNG, 10*1000, &dwResult);
                        }
                        else
                        {
                            // No, off: means show the keycues, turn off the mechanism
                            SendMessageTimeout(HWND_BROADCAST, WM_CHANGEUISTATE, 
                                MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS | UISF_HIDEACCEL),
                                0, SMTO_ABORTIFHUNG, 10*1000, &dwResult);

                            SystemParametersInfo(SPI_SETKEYBOARDINDICATORS, 0,
                               (PVOID)uNewKI, SPIF_UPDATEINIFILE);
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

                    if (bSendSettingsChange)
                    {
                        DWORD_PTR dwResult = 0;

                        SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0,
                            SMTO_ABORTIFHUNG, 10*1000, &dwResult);
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
                         (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)g_phaHelp)
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
                         (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)g_phaHelp)
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
#ifdef JIGGLE_FIX
    RECT rc;
#endif
    UINT flags = ILC_MASK | ILC_COLOR24;
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
    if(IS_WINDOW_RTL_MIRRORED(hWndParent))
    {
        flags |= ILC_MIRROR;
    }
    // initialize the list view window
    // First, initialize the image lists we will need
    hIconList = ImageList_Create( 32, 32, flags, NUM_ICONS, 0 );   // create an image list for the icons

    // load the icons and add them to the image lists
    // get the icon files and indexes from the registry, including for the Default recycle bin
    for( iX = 0; iX < NUM_ICONS; iX++ )
    {
        HICON hIcon;
        BOOL bRet;

        bRet = IconGetRegValueString( sIconRegKeys[iX].szIconSubKey,
                                      sIconRegKeys[iX].szIconValue,
                                      szTemp,
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

            ExtractPlusColorIcon( szTemp, 0, &hIcon, 0, 0 );
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
        lvI.pszText = szTemp;
        lvI.iImage = iX;

        if( ListView_InsertItem( hWndList, &lvI ) == -1 )
            return NULL;

    }
#ifdef JIGGLE_FIX
    // To fix long standing listview bug, we need to "jiggle" the listview
    // window size so that it will do a recompute and realize that we need a
    // scroll bar...
    GetWindowRect( hWndList, &rc );
    MapWindowPoints( NULL, hWndParent, (LPPOINT)&rc, 2 );
    MoveWindow( hWndList, rc.left, rc.top, rc.right - rc.left+1, rc.bottom - rc.top, FALSE );
    MoveWindow( hWndList, rc.left, rc.top, rc.right - rc.left,   rc.bottom - rc.top, FALSE );
#endif
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

    bRet = GetRegValueInt (HKEY_CURRENT_USER, c_szHICKey, c_szWMSISVal, &iSize);
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
    bRet = GetRegValueInt (HKEY_CURRENT_USER, c_szHICKey, c_szWMSISVal, &iOldSize);
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
    bRet = SetRegValueInt( HKEY_CURRENT_USER, c_szHICKey, c_szWMSISVal, iNewSize );
    if (!bRet)
        return FALSE;

    SystemParametersInfo( SPI_ICONHORIZONTALSPACING, iHorz, NULL, SPIF_UPDATEINIFILE );
    SystemParametersInfo( SPI_ICONVERTICALSPACING, iVert, NULL, SPIF_UPDATEINIFILE );

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


//
//  GetAnimations
//
//  Get current state of animations (windows / menus / etc.).
//
WPARAM GetAnimations(DWORD *pdwEffect)
{

    BOOL fMenu = FALSE, fWindow = FALSE, fCombo = FALSE, fSmooth = FALSE, fList = FALSE, fFade = FALSE;
    ANIMATIONINFO ai;

    ai.cbSize = sizeof(ai);
    if (SystemParametersInfo( SPI_GETANIMATION, sizeof(ai), (PVOID)&ai, 0 ))
    {
        fWindow = (ai.iMinAnimate) ? TRUE : FALSE;
    }

    SystemParametersInfo( SPI_GETCOMBOBOXANIMATION, 0, (PVOID)&fCombo, 0 );
    SystemParametersInfo( SPI_GETLISTBOXSMOOTHSCROLLING, 0, (PVOID)&fList, 0 );
    SystemParametersInfo( SPI_GETMENUANIMATION, 0, (PVOID)&fMenu, 0 );
    fSmooth = (BOOL)GetRegValueDword( HKEY_CURRENT_USER,
                                      (LPTSTR)c_szSSKey,
                                      (LPTSTR)c_szSSVal
                                     );

    if (fSmooth == REG_BAD_DWORD)
    {
        fSmooth = 1;
    }
    
    SystemParametersInfo( SPI_GETMENUFADE, 0, (PVOID)&fFade, 0 );
    *pdwEffect = (fFade ? MENU_EFFECT_FADE : MENU_EFFECT_SCROLL);
    
    if (fMenu && fWindow && fCombo && fSmooth && fList)
        return BST_CHECKED;

    if ((!fMenu) && (!fWindow) && (!fCombo) && (!fSmooth) && (!fList))
        return BST_UNCHECKED;

    return BST_INDETERMINATE;
}

//
//  SetAnimations
//
//  Set animations according (windows / menus / etc.) according to flag.
//
void SetAnimations(WPARAM wVal, DWORD dwEffect)
{
    ANIMATIONINFO ai;

    if (wVal != BST_INDETERMINATE)
    {
        BOOL bVal = (wVal == BST_CHECKED) ? 1 : 0;
        BOOL bEfx = (dwEffect == MENU_EFFECT_FADE) ? 1 : 0;
            
        ai.cbSize = sizeof(ai);
        ai.iMinAnimate = bVal;
        SystemParametersInfo( SPI_SETANIMATION, sizeof(ai), (PVOID)&ai, SPIF_UPDATEINIFILE );
        SystemParametersInfo( SPI_SETCOMBOBOXANIMATION, 0, (PVOID)bVal, SPIF_UPDATEINIFILE );
        SystemParametersInfo( SPI_SETLISTBOXSMOOTHSCROLLING, 0, (PVOID)bVal, SPIF_UPDATEINIFILE );
        SystemParametersInfo( SPI_SETMENUANIMATION, 0, (PVOID)bVal, SPIF_UPDATEINIFILE );
        SystemParametersInfo( SPI_SETTOOLTIPANIMATION, 0, (PVOID)bVal, SPIF_UPDATEINIFILE );
        SetRegValueDword( HKEY_CURRENT_USER,
                          (LPTSTR)c_szSSKey,
                          (LPTSTR)c_szSSVal,
                          bVal
                        );
        SystemParametersInfo( SPI_SETMENUFADE, 0, (PVOID)bEfx, SPIF_UPDATEINIFILE);
        SystemParametersInfo( SPI_SETTOOLTIPFADE, 0, (PVOID)bEfx, SPIF_UPDATEINIFILE);
        SystemParametersInfo( SPI_SETSELECTIONFADE, 0, bVal ? (PVOID)bEfx : (PVOID)0, SPIF_UPDATEINIFILE);
    }
}




