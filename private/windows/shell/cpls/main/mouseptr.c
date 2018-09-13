/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mouseptr.c

Abstract:

    This module contains the routines for the Mouse Pointer Property Sheet
    page.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "rc.h"
#include "mousehlp.h"
#include <regstr.h>

//
// From shell\inc\shsemip.h
//
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifdef DEBUG
void * __cdecl memset(void *pmem, int v, size_t s)
{
    char *p = (char *)pmem;

    while (s--)
    {
        *p++ = (char)v;
    }
    return (pmem);
}
#endif




//
//  Constant Declarations.
//

#define gcxAvgChar              8

#define MAX_SCHEME_NAME_LEN     32
#define MAX_SCHEME_SUFFIX       32      // length of " (system scheme)" - update if more space is needed
#define OVERWRITE_TITLE         32      // length of title for the confirm overwrite dialog
#define OVERWRITE_MSG           200     // length of the message for the overwrite dialog

#define PM_NEWCURSOR            (WM_USER + 1)
#define PM_PAUSEANIMATION       (WM_USER + 2)
#define PM_UNPAUSEANIMATION     (WM_USER + 3)

#define ID_PREVIEWTIMER         1

#define CCH_ANISTRING           80

#define CIF_FILE        0x0001
#define CIF_MODIFIED    0x0002
#define CIF_SHARED      0x0004

#define IDT_BROWSE 1




//
//  Typedef Declarations.
//

typedef struct _CURSOR_INFO
{
    DWORD    fl;
    HCURSOR  hcur;
    int      ccur;
    int      icur;
    TCHAR    szFile[MAX_PATH];
} CURSOR_INFO, *PCURSOR_INFO;

#pragma pack(2)
typedef struct tagNEWHEADER
{
    WORD reserved;
    WORD rt;
    WORD cResources;
} NEWHEADER, *LPNEWHEADER;
#pragma pack()

typedef struct
{
    UINT   idVisName;
    int    idResource;
    int    idDefResource;
    LPTSTR pszIniName;
    TCHAR  szVisName[MAX_PATH];
} CURSORDESC, *PCURSORDESC;

//
// Structure that contains data used within a preview window.  This
// data is unique for each preview window, and is used to optimize
// the painting.
//
typedef struct
{
    HDC          hdcMem;
    HBITMAP      hbmMem;
    HBITMAP      hbmOld;
    PCURSOR_INFO pcuri;
} PREVIEWDATA, *PPREVIEWDATA;


typedef struct _MOUSEPTRBR
{
    HWND        hDlg;
    CURSOR_INFO curi;
} MOUSEPTRBR, *PMOUSEPTRBR;




//
//  Global Variables.
//

extern HINSTANCE g_hInst;    // from main.c
int gcxCursor, gcyCursor;
HWND ghwndDlg, ghwndFile, ghwndFileH, ghwndTitle, ghwndTitleH;
HWND ghwndCreator, ghwndCreatorH, ghwndCursors, ghwndPreview, ghwndSchemeCB;
HBRUSH ghbrHighlight, ghbrHighlightText, ghbrWindow, ghbrButton;

UINT guTextHeight = 0;
UINT guTextGap = 0;

COLORREF gcrHighlightText;

TCHAR gszFileName2[MAX_PATH];

UINT wBrowseHelpMessage;

LPTSTR gszFileNotFound = NULL;
LPTSTR gszBrowse = NULL;
LPTSTR gszFilter = NULL;

TCHAR gszNoMem[256] = TEXT("No Memory");

HHOOK ghhkMsgFilter;         // hook handle for message filter function

static const TCHAR szRegStr_Setup[] = REGSTR_PATH_SETUP TEXT("\\Setup");
static const TCHAR szSharedDir[]    = TEXT("SharedDir");

BOOL gfCursorShadow = FALSE;

//
//  Make sure you add new cursors to the end of this array.
//  Otherwise the cursor schemes will not work
//
CURSORDESC gacd[] =
{
    { IDS_ARROW,       OCR_NORMAL,      OCR_ARROW_DEFAULT,       TEXT("Arrow"),       TEXT("") },
    { IDS_HELPCUR,     OCR_HELP,        OCR_HELP_DEFAULT,        TEXT("Help"),        TEXT("") },
    { IDS_APPSTARTING, OCR_APPSTARTING, OCR_APPSTARTING_DEFAULT, TEXT("AppStarting"), TEXT("") },
    { IDS_WAIT,        OCR_WAIT,        OCR_WAIT_DEFAULT,        TEXT("Wait"),        TEXT("") },
    { IDS_CROSS,       OCR_CROSS,       OCR_CROSS_DEFAULT,       TEXT("Crosshair"),   TEXT("") },
    { IDS_IBEAM,       OCR_IBEAM,       OCR_IBEAM_DEFAULT,       TEXT("IBeam"),       TEXT("") },
    { IDS_NWPEN,       OCR_NWPEN,       OCR_NWPEN_DEFAULT,       TEXT("NWPen"),       TEXT("") },
    { IDS_NO,          OCR_NO,          OCR_NO_DEFAULT,          TEXT("No"),          TEXT("") },
    { IDS_SIZENS,      OCR_SIZENS,      OCR_SIZENS_DEFAULT,      TEXT("SizeNS"),      TEXT("") },
    { IDS_SIZEWE,      OCR_SIZEWE,      OCR_SIZEWE_DEFAULT,      TEXT("SizeWE"),      TEXT("") },
    { IDS_SIZENWSE,    OCR_SIZENWSE,    OCR_SIZENWSE_DEFAULT,    TEXT("SizeNWSE"),    TEXT("") },
    { IDS_SIZENESW,    OCR_SIZENESW,    OCR_SIZENESW_DEFAULT,    TEXT("SizeNESW"),    TEXT("") },
    { IDS_SIZEALL,     OCR_SIZEALL,     OCR_SIZEALL_DEFAULT,     TEXT("SizeAll"),     TEXT("") },
    { IDS_UPARROW,     OCR_UP,          OCR_UPARROW_DEFAULT,     TEXT("UpArrow"),     TEXT("") },
    { IDS_HANDCUR,     OCR_HAND,        OCR_HAND_DEFAULT,        TEXT("Hand"),        TEXT("") },
};

#define CCURSORS   (sizeof(gacd) / sizeof(gacd[0]))

CURSOR_INFO acuri[CCURSORS];

//
//  Registry Keys.
//
const TCHAR szCursorSubdir[]  = TEXT("Cursors");
const TCHAR szCursorRegPath[] = REGSTR_PATH_CURSORS;

static const TCHAR c_szRegPathCursors[] = REGSTR_PATH_CURSORS;
static const TCHAR c_szSchemes[]        = TEXT("Schemes");

static const TCHAR c_szRegPathCursorSchemes[] = REGSTR_PATH_CURSORS TEXT( "\\Schemes" );

//
//  Strings used to read from the combo box must be larger than max length.
//
TCHAR gszSchemeName[MAX_SCHEME_SUFFIX + MAX_SCHEME_NAME_LEN + 1];    // used to store selected scheme name for saving
int iSchemeLocation;        // used to store scheme location (HKCU vs HKLM)

static const TCHAR c_szRegPathSystemSchemes[] = REGSTR_PATH_SETUP TEXT("\\Control Panel\\Cursors\\Schemes");
TCHAR szSystemScheme[MAX_SCHEME_SUFFIX];
TCHAR szNone[MAX_SCHEME_NAME_LEN + 1];
const TCHAR szSchemeSource[] = TEXT("Scheme Source");

TCHAR gszPreviousScheme[MAX_SCHEME_SUFFIX + MAX_SCHEME_NAME_LEN + 1];   // used to tell if a different scheme is selected

#define ID_NONE_SCHEME    0
#define ID_USER_SCHEME    1
#define ID_OS_SCHEME      2




//
//  Context Help Ids.
//

const static DWORD aMousePtrHelpIDs[] =
{
    IDC_GROUPBOX_1,    IDH_COMM_GROUPBOX,
    ID_SCHEMECOMBO,    IDH_MOUSE_POINT_SCHEME,
    ID_SAVESCHEME,     IDH_MOUSE_POINT_SAVEAS,
    ID_REMOVESCHEME,   IDH_MOUSE_POINT_DEL,
    ID_PREVIEW,        IDH_MOUSE_POINT_PREVIEW,
    ID_CURSORLIST,     IDH_MOUSE_POINT_LIST,
    ID_DEFAULT,        IDH_MOUSE_POINT_DEFAULT,
    ID_BROWSE,         IDH_MOUSE_POINT_BROWSE,
    ID_CURSORSHADOW,   IDH_MOUSE_CURSORSHADOW,

    0, 0
};

const static DWORD aMousePtrBrowseHelpIDs[] =
{
    IDC_GROUPBOX_1,    IDH_MOUSE_POINT_PREVIEW,
    ID_CURSORPREVIEW,  IDH_MOUSE_POINT_PREVIEW,

    0, 0
};

const static DWORD aHelpIDs[] =
{
    ID_SCHEMEFILENAME, IDH_MOUSE_NEW_SCHEME_NAME,

    0, 0
};




//
//  Forward Declarations.
//

void LoadCursorSet(HWND hwnd);

void CreateBrushes(void);

LPTSTR GetResourceString(HINSTANCE hmod,int id);

void DrawCursorListItem(DRAWITEMSTRUCT *pdis);

BOOL GetCursorFromFile(CURSOR_INFO *pcuri);

BOOL Browse(HWND hwndOwner);

void CleanUpEverything(void);

VOID UpdateCursorList(void);

VOID NextFrame(HWND hwnd);

void HourGlass(BOOL fOn);

BOOL TryToLoadCursor(
    HWND hwnd,
    int i,
    CURSOR_INFO *pcuri);

BOOL LoadScheme(void);

BOOL SaveScheme(void);

BOOL SaveSchemeAs(void);

void SaveCurSchemeName(void);

BOOL RemoveScheme(void);

BOOL InitSchemeComboBox(void);

BOOL SchemeUpdate(int i);

LPTSTR MakeFilename(LPTSTR sz);

INT_PTR CALLBACK SaveSchemeDlgProc(
    HWND  hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

void CurStripBlanks(LPTSTR pszString);

int SystemOrUser(TCHAR *pszSchemeName);

BOOL UnExpandPath( LPTSTR pszPath );




////////////////////////////////////////////////////////////////////////////
//
//  RegisterPointerStuff
//
////////////////////////////////////////////////////////////////////////////

BOOL RegisterPointerStuff(
    HINSTANCE hi)
{
    gcxCursor = GetSystemMetrics(SM_CXCURSOR);
    gcyCursor = GetSystemMetrics(SM_CYCURSOR);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitCursorsShadow
//
////////////////////////////////////////////////////////////////////////////

void InitCursorShadow(HWND hwnd)
{
    BOOL fPalette;
    HDC hdc;
    int nCommand;

    hdc = GetDC(NULL);
    fPalette = (GetDeviceCaps(hdc, NUMCOLORS) != -1);
    ReleaseDC(NULL, hdc);

    if (!fPalette) {
        nCommand = SW_SHOW;
    } else {
        nCommand = SW_HIDE;
    }
    ShowWindow(GetDlgItem(hwnd, ID_CURSORSHADOW), nCommand);

    if (nCommand = SW_SHOW) {
        SystemParametersInfo(SPI_GETCURSORSHADOW, 0, (PVOID)&gfCursorShadow, 0);
        CheckDlgButton(hwnd, ID_CURSORSHADOW, gfCursorShadow);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  InitCursorsDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL InitCursorsDlg(
    HWND hwnd)
{
    int i;

    ghwndDlg = hwnd;
    gszPreviousScheme[0] = TEXT('\0');

    //
    //  Register the help message from the File Open (Browse) dialog.
    //
    wBrowseHelpMessage = RegisterWindowMessage(HELPMSGSTRING);

    //
    //  Load Strings.
    //
    if (gszFileNotFound == NULL)
    {
        gszFileNotFound = GetResourceString(g_hInst, IDS_CUR_BADFILE);

        if (gszFileNotFound == NULL)
        {
            return (FALSE);
        }
    }

    if (gszBrowse == NULL)
    {
        gszBrowse = GetResourceString(g_hInst, IDS_CUR_BROWSE);

        if (gszBrowse == NULL)
        {
            return (FALSE);
        }
    }

#ifdef WINNT
    if (gszFilter == NULL)
    {
        gszFilter = GetResourceString(g_hInst, IDS_ANICUR_FILTER);

        if (!gszFilter)
        {
            return (FALSE);
        }
    }
#else
    if (gszFilter == NULL)
    {
        HDC  dc = GetDC(NULL);
        BOOL fAni = (GetDeviceCaps(dc, CAPS1) & C1_COLORCURSOR) != 0;

        ReleaseDC(NULL, dc);

        gszFilter = GetResourceString( g_hInst,
                                       fAni
                                         ? IDS_ANICUR_FILTER
                                         : IDS_CUR_FILTER );

        if (!gszFilter)
        {
            return (FALSE);
        }
    }
#endif

    //
    //  Load description strings from the resource file.
    //
    for (i = 0; i < CCURSORS; i++)
    {
        if ((!gacd[i].idVisName) ||
            (LoadString( g_hInst,
                         gacd[i].idVisName,
                         gacd[i].szVisName,
                         ARRAYSIZE(gacd[i].szVisName) ) <= 0))
        {
            //
            //  Show something.
            //
            lstrcpy(gacd[i].szVisName, gacd[i].pszIniName);
        }
    }

    //
    //  As an optimization, remember the window handles of the cursor
    //  information fields.
    //
    ghwndPreview  = GetDlgItem(hwnd, ID_PREVIEW);
    ghwndFile     = GetDlgItem(hwnd, ID_FILE);
    ghwndFileH    = GetDlgItem(hwnd, ID_FILEH);
    ghwndTitle    = GetDlgItem(hwnd, ID_TITLE);
    ghwndTitleH   = GetDlgItem(hwnd, ID_TITLEH);
    ghwndCreator  = GetDlgItem(hwnd, ID_CREATOR);
    ghwndCreatorH = GetDlgItem(hwnd, ID_CREATORH);
    ghwndCursors  = GetDlgItem(hwnd, ID_CURSORLIST);
    ghwndSchemeCB = GetDlgItem(hwnd, ID_SCHEMECOMBO);

    //
    //  Create some brushes we'll be using.
    //
    CreateBrushes();

    //
    //  Initialize the scheme combo box.
    //
    InitSchemeComboBox();

    //
    //  Pre-clear the cursor info array.
    //
    memset(acuri, 0, sizeof(acuri));

    //
    //  Load the cursors.
    //
    LoadCursorSet(hwnd);

    //
    //  Force an update of the preview window and the cursor details.
    //
    UpdateCursorList();

    InitCursorShadow(hwnd);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadCursorSet
//
////////////////////////////////////////////////////////////////////////////

void LoadCursorSet(
    HWND hwnd)
{
    CURSOR_INFO *pcuri;
    HKEY hkCursors;
    int i;

    if (RegOpenKey( HKEY_CURRENT_USER,
                    szCursorRegPath,
                    &hkCursors ) != ERROR_SUCCESS)
    {
        hkCursors = NULL;
    }

    for (pcuri = &acuri[0], i = 0; i < CCURSORS; i++, pcuri++)
    {
        if ( hkCursors )
        {
            DWORD dwType;
            DWORD dwCount = sizeof(pcuri->szFile);

            DWORD dwErr = RegQueryValueEx( hkCursors,
                              gacd[i].pszIniName,
                              NULL,
                              &dwType,
                              (LPBYTE)pcuri->szFile,
                              &dwCount );

            if (dwErr == ERROR_SUCCESS)
            {
                if (TryToLoadCursor(hwnd, i, pcuri))
                {
                    goto EverythingWorked;
                }
            }
        }

        // This is actually the failure case.  We load the default cursor.
        pcuri->hcur =
            (HCURSOR)LoadImage( NULL,
                                MAKEINTRESOURCE(gacd[i].idResource),
                                IMAGE_CURSOR,
                                0,
                                0,
                                LR_SHARED | LR_DEFAULTSIZE | LR_ENVSUBST );

        pcuri->fl |= CIF_SHARED;

EverythingWorked:

        SendMessage(ghwndCursors, LB_ADDSTRING, 0, i);
    }

    if (hkCursors)
    {
        RegCloseKey(hkCursors);
    }

    SendMessage(ghwndCursors, LB_SETCURSEL, 0, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateBrushes
//
//  Creates the brushes that are used to paint within the Cursors applet.
//
////////////////////////////////////////////////////////////////////////////

VOID CreateBrushes()
{
    ghbrHighlight     = GetSysColorBrush(COLOR_HIGHLIGHT);
    gcrHighlightText  = GetSysColor(COLOR_HIGHLIGHTTEXT);
    ghbrHighlightText = GetSysColorBrush(COLOR_HIGHLIGHTTEXT);
    ghbrWindow        = GetSysColorBrush(COLOR_WINDOW);
    ghbrButton        = GetSysColorBrush(COLOR_BTNFACE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetResourceString
//
//  Gets a string out of the resource file.
//
////////////////////////////////////////////////////////////////////////////

LPTSTR GetResourceString(
    HINSTANCE hmod,
    int id)
{
    TCHAR szBuffer[256];
    LPTSTR psz;
    int cch;

    if ((cch = LoadString(hmod, id, szBuffer, ARRAYSIZE(szBuffer))) == 0)
    {
        return (NULL);
    }

    psz = LocalAlloc(LPTR, (cch + 1) * sizeof(TCHAR));

    if (psz != NULL)
    {
        int i;

        for (i = 0; i <= cch; i++)
        {
            psz[i] = (szBuffer[i] == TEXT('\1')) ? TEXT('\0') : szBuffer[i];
        }
    }

    return (psz);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeItemCursor
//
////////////////////////////////////////////////////////////////////////////

void FreeItemCursor(
    CURSOR_INFO *pcuri)
{
    if (pcuri->hcur)
    {
        if (!(pcuri->fl & CIF_SHARED))
        {
            DestroyCursor(pcuri->hcur);
        }
        pcuri->hcur = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MousePtrDlg
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MousePtrDlg(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    CURSOR_INFO *pcuri;
    HKEY hkCursors;
    int i;

    switch (msg)
    {
        case ( WM_INITDIALOG ) :
        {
            return InitCursorsDlg(hwnd);
        }
        case ( WM_DISPLAYCHANGE ) :
        {
            InitCursorShadow(hwnd);
            SHPropagateMessage(hwnd, msg, wParam, lParam, TRUE);
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            ((MEASUREITEMSTRUCT *)lParam)->itemHeight = gcyCursor + 2;
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            DrawCursorListItem((DRAWITEMSTRUCT *)lParam);
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( ID_SCHEMECOMBO ) :
                {
                    switch (HIWORD(wParam))
                    {
                        case ( CBN_SELCHANGE ) :
                        {
                            LoadScheme();
                            break;
                        }
                    }
                    break;
                }
                case ( ID_DEFAULT ) :
                {
                    //
                    //  Throw away any fancy new cursor and replace it with
                    //  the system's original.
                    //
                    i = (int)SendMessage(ghwndCursors, LB_GETCURSEL, 0, 0);

                    pcuri = &acuri[i];

                    if (!(pcuri->fl & CIF_FILE))
                    {
                        break;
                    }
                    pcuri->fl = CIF_MODIFIED;

                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0L);

                    FreeItemCursor(pcuri);

                    pcuri->hcur =
                        (HCURSOR)LoadImage( NULL,
                                            MAKEINTRESOURCE(gacd[i].idDefResource),
                                            IMAGE_CURSOR,
                                            0,
                                            0,
                                            LR_DEFAULTSIZE | LR_ENVSUBST );

                    *pcuri->szFile = TEXT('\0');

                    EnableWindow(GetDlgItem(hwnd, ID_SAVESCHEME), TRUE);

                    UpdateCursorList();

                    break;
                }
                case ( ID_CURSORLIST ) :
                {
                    switch (HIWORD(wParam))
                    {
                        case ( LBN_SELCHANGE ) :
                        {
                            i = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                            pcuri = &acuri[i];

                            //
                            //  Show a preview (including animation) in the
                            //  preview window.
                            //
                            SendMessage( ghwndPreview,
                                         STM_SETICON,
                                         (WPARAM)pcuri->hcur,
                                         0L );

                            //
                            //  Enable the "Set Default" button if the cursor
                            //  is from a file.
                            //
                            EnableWindow( GetDlgItem(hwnd, ID_DEFAULT),
                                          (pcuri->fl & CIF_FILE ) ? TRUE : FALSE );
                            break;
                        }
                        case ( LBN_DBLCLK ) :
                        {
                            Browse(hwnd);
                            break;
                        }
                    }
                    break;
                }
                case ( ID_BROWSE ) :
                {
                    Browse(hwnd);
                    break;
                }
                case ( ID_SAVESCHEME ) :
                {
                    SaveSchemeAs();
                    break;
                }
                case ( ID_REMOVESCHEME ) :
                {
                    RemoveScheme();
                    break;
                }
                case ( ID_CURSORSHADOW ) :
                {
                    gfCursorShadow = IsDlgButtonChecked(hwnd, ID_CURSORSHADOW);
                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0L);
                    break;
                }
            }
            break;
        }
        case ( WM_NOTIFY ) :
        {
            switch(((NMHDR *)lParam)->code)
            {
                case ( PSN_APPLY ) :
                {
                    //
                    //  Change cursor to hour glass.
                    //
                    HourGlass(TRUE);

                    // Set cursor shadow
                    SystemParametersInfo( SPI_SETCURSORSHADOW,
                                          0,
                                          (PVOID)gfCursorShadow,
                                          SPIF_UPDATEINIFILE);

                    //
                    //  Save the modified scheme, order of calls important.
                    //
                    SaveCurSchemeName();

                    //
                    //  Set the system cursors.
                    //
                    if (RegCreateKey( HKEY_CURRENT_USER,
                                      szCursorRegPath,
                                      &hkCursors ) == ERROR_SUCCESS)
                    {
                        for (pcuri = &acuri[0], i = 0; i < CCURSORS; i++, pcuri++)
                        {
                            if (pcuri->fl & CIF_MODIFIED)
                            {
                                LPCTSTR data;
                                UINT count;

                                // Always unexpand before we save a filename
                                UnExpandPath(pcuri->szFile);

                                data = (pcuri->fl & CIF_FILE) ? pcuri->szFile : TEXT("");
                                count = (pcuri->fl & CIF_FILE) ? (lstrlen(pcuri->szFile) + 1) * sizeof(TCHAR) : sizeof(TCHAR);

                                RegSetValueEx( hkCursors,
                                               gacd[i].pszIniName,
                                               0L,
                                               REG_EXPAND_SZ,
                                               (CONST LPBYTE)data,
                                               count );
                            }
                        }

                        RegCloseKey(hkCursors);

                        SystemParametersInfo( SPI_SETCURSORS,
                                              0,
                                              0,
                                              SPIF_SENDCHANGE );
                    }

                    HourGlass(FALSE);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_SYSCOLORCHANGE ) :
        {
            gcrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
            SHPropagateMessage(hwnd, msg, wParam, lParam, TRUE);
            break;
        }

        case ( WM_WININICHANGE ) :
        {
            SHPropagateMessage(hwnd, msg, wParam, lParam, TRUE);
            break;
        }

        case ( WM_DESTROY ) :
        {
            //
            //  Clean up global allocs.
            //
            CleanUpEverything();

            if (gszFileNotFound != NULL)
            {
                LocalFree(gszFileNotFound);
                gszFileNotFound = NULL;
            }

            if (gszBrowse != NULL)
            {
                LocalFree(gszBrowse);
                gszBrowse = NULL;
            }

            if (gszFilter != NULL)
            {
                LocalFree(gszFilter);
                gszFilter = NULL;
            }
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( ((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aMousePtrHelpIDs );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPVOID)aMousePtrHelpIDs );
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawCursorListItem
//
////////////////////////////////////////////////////////////////////////////

void DrawCursorListItem(
    DRAWITEMSTRUCT *pdis)
{
    CURSOR_INFO *pcuri;
    COLORREF clrOldText, clrOldBk;
    RECT rc;

    if (!guTextHeight || !guTextGap)
    {
        TEXTMETRIC tm;

        tm.tmHeight = 0;
        GetTextMetrics(pdis->hDC, &tm);

        if (tm.tmHeight < 0)
        {
            tm.tmHeight *= -1;
        }
        guTextHeight = (UINT)tm.tmHeight;
        guTextGap = (UINT)tm.tmAveCharWidth;
    }

    pcuri = &acuri[pdis->itemData];

    if (pdis->itemState & ODS_SELECTED)
    {
        clrOldText = SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        clrOldBk = SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
    }
    else
    {
        clrOldText = SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
        clrOldBk = SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
    }

    ExtTextOut( pdis->hDC,
                pdis->rcItem.left + guTextGap,   // fudge factor
                (pdis->rcItem.top + pdis->rcItem.bottom - guTextHeight) / 2,
                ETO_OPAQUE,
                &pdis->rcItem,
                gacd[pdis->itemData].szVisName,
                lstrlen(gacd[pdis->itemData].szVisName),
                NULL );

    if (pcuri->hcur != NULL)
    {
        DrawIcon( pdis->hDC,
                  pdis->rcItem.right - (gcxCursor + guTextGap),
                  pdis->rcItem.top + 1, pcuri->hcur );
    }

    if (pdis->itemState & ODS_FOCUS)
    {
        CopyRect(&rc, &pdis->rcItem);
        InflateRect(&rc, -1, -1);
        DrawFocusRect(pdis->hDC, &rc);
    }

    SetTextColor(pdis->hDC, clrOldText);
    SetBkColor(pdis->hDC, clrOldBk);
}


////////////////////////////////////////////////////////////////////////////
//
//  TryToLoadCursor
//
////////////////////////////////////////////////////////////////////////////

BOOL TryToLoadCursor(
    HWND hwnd,
    int i,
    CURSOR_INFO *pcuri)
{
    BOOL fRet    = TRUE;
    BOOL bCustom = (*pcuri->szFile != 0);


    if (bCustom && !GetCursorFromFile(pcuri))
    {
        HWND hwndControl = GetParent(hwnd);
        LPTSTR pszText;
        LPTSTR pszFilename;

        //
        //  MakeFilename returns the address of a global, so we don't
        //  need to free pszFilename.
        //
        pszFilename = MakeFilename(pcuri->szFile);

        pszText = LocalAlloc( LPTR,
                              (lstrlen(gszFileNotFound) +
                               lstrlen(gacd[i].szVisName) +
                               lstrlen(pszFilename) +
                               1) * sizeof(TCHAR) );

        if (pszText == NULL)
        {
            return (FALSE);
        }

        wsprintf(pszText, gszFileNotFound, pszFilename, gacd[i].szVisName);

        MessageBeep(MB_ICONEXCLAMATION);

        MessageBox(hwndControl, pszText, NULL, MB_ICONEXCLAMATION | MB_OK);

        pcuri->fl = CIF_MODIFIED;

        SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0L);

        bCustom = FALSE;

        LocalFree(pszText);
    }

    if (!bCustom)
    {
        FreeItemCursor(pcuri);

        pcuri->hcur =
            (HCURSOR)LoadImage( NULL,
                                MAKEINTRESOURCE(gacd[i].idDefResource),
                                IMAGE_CURSOR,
                                0,
                                0,
                                LR_DEFAULTSIZE | LR_ENVSUBST );

        *pcuri->szFile = TEXT('\0');

        EnableWindow(GetDlgItem(hwnd, ID_SAVESCHEME), TRUE);
        UpdateCursorList();
    }

    return (pcuri->hcur != NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCursorFromFile
//
////////////////////////////////////////////////////////////////////////////

BOOL GetCursorFromFile(
    CURSOR_INFO *pcuri)
{
    pcuri->fl = 0;
    pcuri->hcur =
        (HCURSOR)LoadImage( NULL,
                            MakeFilename(pcuri->szFile),
                            IMAGE_CURSOR,
                            0,
                            0,
                            LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_ENVSUBST );

    if (pcuri->hcur)
    {
        pcuri->fl |= CIF_FILE;
    }

    return (pcuri->hcur != NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  MousePtrBrowsePreview
//
////////////////////////////////////////////////////////////////////////////

void MousePtrBrowsePreview(
    HWND hDlg)
{
    PMOUSEPTRBR pPtrBr;
    HCURSOR hcurOld;

    pPtrBr = (PMOUSEPTRBR)GetWindowLongPtr(hDlg, DWLP_USER);

    hcurOld = pPtrBr->curi.hcur;

    CommDlg_OpenSave_GetFilePath( GetParent(hDlg),
                                  pPtrBr->curi.szFile,
                                  ARRAYSIZE(pPtrBr->curi.szFile) );

    if (!GetCursorFromFile(&pPtrBr->curi))
    {
        pPtrBr->curi.hcur = NULL;
    }

    SendDlgItemMessage( hDlg,
                        ID_CURSORPREVIEW,
                        STM_SETICON,
                        (WPARAM)pPtrBr->curi.hcur, 0L );

    if (hcurOld)
    {
        DestroyCursor(hcurOld);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MousePtrBrowseNotify
//
////////////////////////////////////////////////////////////////////////////

BOOL MousePtrBrowseNotify(
    HWND hDlg,
    LPOFNOTIFY pofn)
{
    switch (pofn->hdr.code)
    {
        case ( CDN_SELCHANGE ) :
        {
            //
            //  Don't show the cursor until the user stops moving around.
            //
            if (SetTimer(hDlg, IDT_BROWSE, 250, NULL))
            {
                //
                //  Don't destroy the old cursor.
                //
                SendDlgItemMessage( hDlg,
                                    ID_CURSORPREVIEW,
                                    STM_SETICON,
                                    0,
                                    0L );
            }
            else
            {
                MousePtrBrowsePreview(hDlg);
            }
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MousePtrBrowseDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MousePtrBrowseDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            PMOUSEPTRBR pPtrBr = (PMOUSEPTRBR)((LPOPENFILENAME)lParam)->lCustData;

            if (pPtrBr)
            {
                pPtrBr->hDlg = hDlg;
            }

            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pPtrBr);
            break;
        }
        case ( WM_DESTROY ) :
        {
            KillTimer(hDlg, IDT_BROWSE);

            //
            //  Don't destroy the old cursor.
            //
            SendDlgItemMessage(hDlg, ID_CURSORPREVIEW, STM_SETICON, 0, 0L);
            break;
        }
        case ( WM_TIMER ) :
        {
            KillTimer(hDlg, IDT_BROWSE);

            MousePtrBrowsePreview(hDlg);
            break;
        }
        case ( WM_NOTIFY ) :
        {
            return (MousePtrBrowseNotify(hDlg, (LPOFNOTIFY) lParam));
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aMousePtrBrowseHelpIDs );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPVOID)aMousePtrBrowseHelpIDs );
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Browse
//
//  Browse the file system for a new cursor for the selected item.
//
////////////////////////////////////////////////////////////////////////////

BOOL Browse(HWND hwndOwner)
{
    static TCHAR szCustomFilter[80] = TEXT("");
    static TCHAR szStartDir[MAX_PATH] = TEXT("");

    OPENFILENAME ofn;
    CURSOR_INFO curi;
    int i;
    BOOL fRet = FALSE;
    MOUSEPTRBR sPtrBr;

    if (!*szStartDir)
    {
        HKEY key = NULL;

        if (RegOpenKey(HKEY_LOCAL_MACHINE, szRegStr_Setup, &key) == ERROR_SUCCESS)
        {
            LONG len = sizeof(szStartDir) / sizeof(szStartDir[0]);

            if (RegQueryValueEx( key,
                                 szSharedDir,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szStartDir,
                                 &len ) != ERROR_SUCCESS)
            {
                *szStartDir = TEXT('\0');
            }

            RegCloseKey(key);
        }

        if (!*szStartDir)
        {
            GetWindowsDirectory(szStartDir, MAX_PATH);
        }

        PathAppend(szStartDir, szCursorSubdir);
    }

    curi.szFile[0] = TEXT('\0');

    sPtrBr.curi.szFile[0] = TEXT('\0');
    sPtrBr.curi.hcur      = NULL;

    ofn.lStructSize       = sizeof(ofn);
    ofn.hwndOwner         = hwndOwner;
    ofn.hInstance         = g_hInst;
    ofn.lpstrFilter       = gszFilter;
    ofn.lpstrCustomFilter = szCustomFilter;
    ofn.nMaxCustFilter    = ARRAYSIZE(szCustomFilter);
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = curi.szFile;
    ofn.nMaxFile          = ARRAYSIZE(curi.szFile);
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = szStartDir;
    ofn.lpstrTitle        = gszBrowse;
    ofn.Flags             = OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK |
                            OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt       = NULL;
    ofn.lpfnHook          = MousePtrBrowseDlgProc;
    ofn.lpTemplateName    = MAKEINTRESOURCE(DLG_MOUSE_POINTER_BROWSE);
    ofn.lCustData         = (LPARAM)(PMOUSEPTRBR)&sPtrBr;

    fRet = GetOpenFileName(&ofn);

    GetCurrentDirectory(ARRAYSIZE(szStartDir), szStartDir);

    if (!fRet)
    {
        goto brErrExit;
    }

    fRet = FALSE;

    //
    //  We have probably already gotten this cursor.
    //
    if (lstrcmpi(curi.szFile, sPtrBr.curi.szFile) == 0)
    {
        if (!sPtrBr.curi.hcur)
        {
            goto brErrExit;
        }

        curi = sPtrBr.curi;

        //
        //  Clear this so it will not get destroyed in the cleanup code.
        //
        sPtrBr.curi.hcur = NULL;
    }
    else
    {
        //
        //  The user must have typed in a name.
        //
        if (!GetCursorFromFile(&curi))
        {
            goto brErrExit;
        }
    }

    //
    //  Convert mapped drive letters to UNC.
    //
#ifdef UNICODE
    if (curi.szFile[1] == TEXT(':'))
#else
    if (!IsDBCSLeadByte(curi.szFile[0]) && (curi.szFile[1] == TEXT(':')))
#endif
    {
        TCHAR szDrive[3];
        TCHAR szNet[MAX_PATH];
        int lenNet = ARRAYSIZE(szNet);

        lstrcpyn(szDrive, curi.szFile, ARRAYSIZE(szDrive));

        if ((WNetGetConnection(szDrive, szNet, &lenNet) == NO_ERROR) &&
            (szNet[0] == TEXT('\\')) &&
            (szNet[1] == TEXT('\\')))
        {
            lstrcat(szNet, curi.szFile + 2);
            lstrcpy(curi.szFile, szNet);
        }
    }

    i = (int)SendMessage(ghwndCursors, LB_GETCURSEL, 0, 0);

    curi.fl |= CIF_MODIFIED;

    SendMessage(GetParent(ghwndDlg), PSM_CHANGED, (WPARAM)ghwndDlg, 0L);

    EnableWindow(GetDlgItem(ghwndDlg, ID_SAVESCHEME), TRUE);

    //
    //  Destroy the old cursor before we retain the new one.
    //
    FreeItemCursor(acuri + i);

    acuri[i] = curi;

    UpdateCursorList();

    fRet = TRUE;

brErrExit:
    if (sPtrBr.curi.hcur)
    {
        DestroyCursor(sPtrBr.curi.hcur);
    }

    return (fRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  CleanUpEverything
//
//  Destroy all the outstanding cursors.
//
////////////////////////////////////////////////////////////////////////////

void CleanUpEverything()
{
    CURSOR_INFO *pcuri;
    int i;

    for (pcuri = &acuri[0], i = 0; i < CCURSORS; i++, pcuri++)
    {
        FreeItemCursor(pcuri);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateCursorList
//
//  Force the Cursor ListBox to repaint and the cursor information below the
//  listbox to be refreshed as well.
//
////////////////////////////////////////////////////////////////////////////

VOID UpdateCursorList()
{
    int i = (int)SendMessage(ghwndCursors, LB_GETCURSEL, 0, 0);
    PCURSOR_INFO pcuri = ((i >= 0) ? &acuri[i] : NULL);
    HCURSOR hcur = pcuri ? pcuri->hcur : NULL;
    HWND hDefaultButton = GetDlgItem(ghwndDlg, ID_DEFAULT);
    BOOL fEnableDefaultButton = (pcuri && (pcuri->fl & CIF_FILE));

    InvalidateRect(ghwndCursors, NULL, FALSE);

    SendMessage(ghwndPreview, STM_SETICON, (WPARAM)hcur, 0L);

    if (!fEnableDefaultButton && (GetFocus() == hDefaultButton))
    {
        SendMessage(ghwndDlg, WM_NEXTDLGCTL, (WPARAM)ghwndCursors, TRUE);
    }

    EnableWindow(hDefaultButton, fEnableDefaultButton);
}


////////////////////////////////////////////////////////////////////////////
//
//  SaveSchemeAs
//
////////////////////////////////////////////////////////////////////////////

BOOL SaveSchemeAs()
{
    BOOL fSuccess = TRUE;

    //
    //  Dialog proc returns TRUE & sets gszSchemeName to filename entered
    //  on OK.
    //
    if (DialogBox( g_hInst,
                   MAKEINTRESOURCE(DLG_MOUSE_POINTER_SCHEMESAVE),
                   ghwndDlg,
                   SaveSchemeDlgProc ))
    {
        fSuccess = SaveScheme();

        if (fSuccess)
        {
            int index = (int)SendMessage( ghwndSchemeCB,
                                          CB_FINDSTRINGEXACT,
                                          (WPARAM)-1,
                                          (LPARAM)gszSchemeName );
            //
            //  If not found, add it.
            //
            if (index < 0)
            {
                index = (int)SendMessage( ghwndSchemeCB,
                                          CB_ADDSTRING,
                                          0,
                                          (LPARAM)gszSchemeName );
            }

            //
            //  Select the name.
            //
            SendMessage(ghwndSchemeCB, CB_SETCURSEL, (WPARAM) index, 0);

            //
            //  Since this is now a user saved scheme, activate the delete
            //  button.
            //
            EnableWindow(GetDlgItem(ghwndDlg, ID_REMOVESCHEME), TRUE);
        }
    }

    return (fSuccess);
}


////////////////////////////////////////////////////////////////////////////
//
//  SubstituteString
//
//  Replaces the string pszRemove with the string pszReplace in the
//  string pszInput and places the output in pszResult.  Only looks
//  at the begining of the input string.
//
////////////////////////////////////////////////////////////////////////////

BOOL SubstituteString(LPCTSTR pszInput, LPCTSTR pszRemove, LPCTSTR pszReplace, LPTSTR pszResult, UINT cchResult)
{
    DWORD cchRemove = lstrlen(pszRemove);

    if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                       pszRemove, cchRemove, pszInput, cchRemove) == 2)
    {
        if (lstrlen(pszInput) + cchRemove < cchResult)
        {
            lstrcpy(pszResult, pszReplace);
            lstrcat(pszResult, pszInput + cchRemove);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL UnExpandPath( LPTSTR pszPath )
{
    static TCHAR szUserProfile[MAX_PATH];
    static TCHAR szSystemRoot[MAX_PATH];
    static TCHAR szProgramFiles[MAX_PATH];
    static BOOL bInit = FALSE;
    TCHAR szUnexpandedFilename[MAX_PATH];

    if ( !bInit )
    {
        ExpandEnvironmentStrings( TEXT("%USERPROFILE%"),  szUserProfile,  ARRAYSIZE(szUserProfile)  );
        ExpandEnvironmentStrings( TEXT("%SYSTEMROOT%"),   szSystemRoot,   ARRAYSIZE(szSystemRoot)   );
        ExpandEnvironmentStrings( TEXT("%ProgramFiles%"), szProgramFiles, ARRAYSIZE(szProgramFiles) );
        bInit = TRUE;
    }

    if (!SubstituteString(pszPath, szUserProfile, TEXT("%USERPROFILE%"), szUnexpandedFilename, ARRAYSIZE(szUnexpandedFilename)))
    {
        if (!SubstituteString(pszPath, szSystemRoot, TEXT("%SYSTEMROOT%"), szUnexpandedFilename, ARRAYSIZE(szUnexpandedFilename)))
        {
            if (!SubstituteString(pszPath, szProgramFiles, TEXT("%ProgramFiles%"), szUnexpandedFilename, ARRAYSIZE(szUnexpandedFilename)))
            {
                return FALSE;
            }
        }
    }
    lstrcpy(pszPath, szUnexpandedFilename);
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  SaveScheme
//
////////////////////////////////////////////////////////////////////////////

BOOL SaveScheme()
{
    BOOL fSuccess = FALSE;

    if (*gszSchemeName)
    {
        const BUFFER_SIZE = CCURSORS * (MAX_PATH + 1) + 1;
        LPTSTR pszBuffer = (LPTSTR)LocalAlloc( LMEM_FIXED,
                                               BUFFER_SIZE * sizeof(TCHAR) );

        HKEY hk;
        int i;

        if (!pszBuffer)
        {
            return (FALSE);
        }

        pszBuffer[0] = TEXT('\0');

        for (i = 0; i < CCURSORS; i++)
        {
            if (i)
            {
                lstrcat(pszBuffer, TEXT(","));
            }

            // Replace path with evnironment variables.
            UnExpandPath(acuri[i].szFile);

            lstrcat(pszBuffer, acuri[i].szFile);
        }

        if (RegCreateKey( HKEY_CURRENT_USER,
                          c_szRegPathCursors,
                          &hk ) == ERROR_SUCCESS)
        {
            HKEY hks;

            if (RegCreateKey(hk, c_szSchemes, &hks) == ERROR_SUCCESS)
            {
                LPTSTR pszOldValue = (LPTSTR)LocalAlloc( LMEM_FIXED,
                                               BUFFER_SIZE * sizeof(TCHAR) );
                DWORD dwType;
                DWORD dwSize = BUFFER_SIZE*sizeof(TCHAR);
                BOOL bSave = FALSE;

                int ret = RegQueryValueEx(hks, gszSchemeName, NULL, &dwType, (LPBYTE)pszOldValue, &dwSize);

                //
                //  If the key already exists, ask to confirm the overwrite.
                //
                if (ret == ERROR_SUCCESS && (dwType==REG_SZ || dwType==REG_EXPAND_SZ))
                {
                    // only need to save if value is different from old value
                    if (lstrcmp(pszOldValue,pszBuffer)!=0)
                    {
                        TCHAR szTitle[OVERWRITE_TITLE];
                        TCHAR szMsg[OVERWRITE_MSG];
                        LoadString(g_hInst, IDS_OVERWRITE_TITLE, szTitle, OVERWRITE_TITLE);
                        LoadString(g_hInst, IDS_OVERWRITE_MSG, szMsg, OVERWRITE_MSG);

                        if (MessageBox( ghwndDlg,
                                        szMsg,
                                        szTitle,
                                        MB_ICONQUESTION | MB_YESNO ) == IDYES)
                        {
                            //
                            //  Overwrite confirmed.  Safe to save.
                            //
                            bSave = TRUE;
                        }
                    }
                    else
                    {
                        // no need to save since the new value is the same as the old value.
                        fSuccess = TRUE;
                    }
                }
                else
                {
                    //
                    //  The key doesn't exist, so it's safe to create it.
                    //
                    bSave = TRUE;
                }

                if (bSave)
                {
                    if (RegSetValueEx( hks,
                                       gszSchemeName,
                                       0,
                                       REG_EXPAND_SZ,
                                       (LPBYTE)pszBuffer,
                                       (lstrlen(pszBuffer) + 1) * sizeof(TCHAR) )
                          == ERROR_SUCCESS)
                    {
                        fSuccess = TRUE;
                    }
                }

                RegCloseKey(hks);
                LocalFree( pszOldValue );
            }

            RegCloseKey(hk);
        }

        LocalFree(pszBuffer);
    }

    return (fSuccess);
}


////////////////////////////////////////////////////////////////////////////
//
//  SaveCurSchemeName
//
////////////////////////////////////////////////////////////////////////////

void SaveCurSchemeName()
{
    HKEY hk;

    if (RegCreateKey( HKEY_CURRENT_USER,
                      c_szRegPathCursors,
                      &hk ) == ERROR_SUCCESS)
    {
        int index = (int)SendMessage(ghwndSchemeCB, CB_GETCURSEL, 0, 0L);

        SendMessage( ghwndSchemeCB,
                     CB_GETLBTEXT,
                     (WPARAM)index,
                     (LPARAM)gszSchemeName );
        //
        //  Exclude the "none" pattern.
        //
        if (lstrcmpi(gszSchemeName, szNone) == 0)
        {
            *gszSchemeName = 0;
            iSchemeLocation = ID_NONE_SCHEME;
        }
        else
        {
            iSchemeLocation = SystemOrUser(gszSchemeName);
        }

        RegSetValue( hk,
                     NULL,
                     REG_SZ,
                     gszSchemeName,
                     (lstrlen(gszSchemeName) + 1) * sizeof(TCHAR) );

        RegSetValueEx( hk,
                       szSchemeSource,
                       0,
                       REG_DWORD,
                       (unsigned char *)&iSchemeLocation,
                       sizeof(iSchemeLocation) );

        RegCloseKey(hk);

        if (iSchemeLocation == ID_USER_SCHEME)
        {
            SaveScheme();
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadScheme
//
//  This is called whenever a selection is made from the schemes combo box.
//
////////////////////////////////////////////////////////////////////////////

BOOL LoadScheme()
{
    const BUFFER_SIZE = CCURSORS * (MAX_PATH + 1) + 1;
    TCHAR pszSchemeName[MAX_SCHEME_SUFFIX + MAX_SCHEME_NAME_LEN + 1];
    LPTSTR pszBuffer;
    BOOL fSuccess = FALSE;
    int index, ret;
    HKEY hk;

    //
    //  Allocate buffer for cursor paths.
    //
    pszBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, BUFFER_SIZE * sizeof(TCHAR));
    if (pszBuffer == NULL)
    {
        return (FALSE);
    }

    HourGlass(TRUE);

    *pszBuffer = *pszSchemeName = 0;

    index = (int)SendMessage(ghwndSchemeCB, CB_GETCURSEL, 0, 0L);

    //
    //  Get current scheme name.
    //
    SendMessage( ghwndSchemeCB,
                 CB_GETLBTEXT,
                 (WPARAM)index,
                 (LPARAM)pszSchemeName );

    // Get the text for the item at index, compare to the previous value to see
    // if it changed.  We can't simply compare the previous index because new items
    // get inserted into the list so the index can change and still be the same or
    // can be different when nothing has changed.
    if ( 0 == lstrcmp(gszPreviousScheme, pszSchemeName) )
    {
        // nothing to do, we're loading the already selected scheme
        return FALSE;
    }

    // We're loading a different scheme, enable the apply button.
    SendMessage(GetParent(ghwndDlg), PSM_CHANGED, (WPARAM)ghwndDlg, 0L);
    lstrcpy(gszPreviousScheme, pszSchemeName);

    //
    //  Exclude the "none" pattern.
    //
    if (lstrcmpi(pszSchemeName, szNone) != 0)
    {
        //
        //  If we have an os scheme, then search for the scheme in HKLM,
        //  otherwise look in HKCU.
        //
        if ((((ret = SystemOrUser(pszSchemeName)) == ID_OS_SCHEME)
               ? (RegOpenKey(HKEY_LOCAL_MACHINE, c_szRegPathSystemSchemes, &hk))
               : (RegOpenKey(HKEY_CURRENT_USER, c_szRegPathCursorSchemes, &hk)))
             == ERROR_SUCCESS)
        {
            DWORD len = BUFFER_SIZE * sizeof(TCHAR);

            if (RegQueryValueEx( hk,
                                 pszSchemeName, 0, NULL,
                                 (LPBYTE)pszBuffer,
                                 &len ) == ERROR_SUCCESS)
            {
                fSuccess = TRUE;       // can be reset to FALSE below
            }

            RegCloseKey(hk);
        }
    }
    else
    {
        //
        //  "none" pattern is a valid choice.
        //
        ret = ID_NONE_SCHEME;
        fSuccess = TRUE;
    }

    if (fSuccess)
    {
        LPTSTR pszNextFile, pszFile = pszBuffer;
        BOOL fEOL = FALSE;
        int i = 0;

        //
        //  Parse string of format TEXT("filename1, filename2, filename3...")
        //  into cursor info array.
        //
        do
        {
            while (*pszFile &&
                   (*pszFile == TEXT(' ')  ||
                    *pszFile == TEXT('\t') ||
                    *pszFile == TEXT('\n')))
            {
                pszFile++;
            }

            pszNextFile = pszFile;

            while (*pszNextFile != TEXT('\0'))
            {
                if (*pszNextFile == TEXT(','))
                {
                    break;
                }

                pszNextFile = CharNext(pszNextFile);
            }

            if (*pszNextFile == TEXT('\0'))
            {
                fEOL = TRUE;
            }
            else
            {
                *pszNextFile = TEXT('\0');
            }

            if (lstrcmp(pszFile, acuri[i].szFile))
            {
                //
                //  It's different than current, update.
                //
                lstrcpy(acuri[i].szFile, pszFile);

                fSuccess &= SchemeUpdate(i);
            }

            pszFile = pszNextFile;

            if (!fEOL)
            {
                pszFile++;        // skip TEXT('\0') and move to next path
            }

            i++;

        } while (i < CCURSORS);
    }

    LocalFree(pszBuffer);

    UpdateCursorList();

    EnableWindow(GetDlgItem(ghwndDlg, ID_REMOVESCHEME), (ret == ID_USER_SCHEME));

    HourGlass(FALSE);

    return (fSuccess);
}


////////////////////////////////////////////////////////////////////////////
//
//  SchemeUpdate
//
//  Updates the cursor at index i in acuri.
//
////////////////////////////////////////////////////////////////////////////

BOOL SchemeUpdate(int i)
{
    BOOL fSuccess = TRUE;

    if (acuri[i].hcur)
    {
        FreeItemCursor(acuri + i);
    }

    //
    //  If TEXT("Set Default").
    //
    if (*(acuri[i].szFile) == TEXT('\0'))
    {
        acuri[i].hcur =
            (HCURSOR)LoadImage( NULL,
                                MAKEINTRESOURCE(gacd[i].idDefResource),
                                IMAGE_CURSOR,
                                0,
                                0,
                                LR_DEFAULTSIZE | LR_ENVSUBST );
        acuri[i].fl = 0;
    }
    else
    {
        fSuccess = TryToLoadCursor(ghwndDlg, i, &acuri[i]);
    }

    acuri[i].fl |= CIF_MODIFIED;

    return (fSuccess);
}


////////////////////////////////////////////////////////////////////////////
//
//  RemoveScheme
//
////////////////////////////////////////////////////////////////////////////

BOOL RemoveScheme()
{
    //
    //  Only user schemes can be removed, so this only needs to
    //  be MAX_SCHEME_NAME_LEN + 1 long.
    //
    TCHAR szSchemeName[MAX_SCHEME_NAME_LEN + 1];
    int index;
    HKEY hk;

    index = (int)SendMessage(ghwndSchemeCB, CB_GETCURSEL, 0, 0L);

    //
    //  Get current scheme name.
    //
    SendMessage( ghwndSchemeCB,
                 CB_GETLBTEXT,
                 (WPARAM)index,
                 (LPARAM)szSchemeName );

    //
    //  Exclude the "none" pattern from removal.
    //
    if (lstrcmpi(szSchemeName, szNone) == 0)
    {
        return (FALSE);
    }

    //
    //  HACK: assume deleting noname needs no confirmation -
    //  this is because the scheme won't save properly anyway.
    //
    if (*szSchemeName)
    {
        TCHAR RemoveMsg[MAX_PATH];
        TCHAR DialogMsg[MAX_PATH];

        LoadString(g_hInst, IDS_REMOVESCHEME, RemoveMsg, MAX_PATH);

        wsprintf(DialogMsg, RemoveMsg, (LPTSTR)szSchemeName);

        LoadString(g_hInst, IDS_NAME, RemoveMsg, MAX_PATH);

        if (MessageBox( ghwndDlg,
                        DialogMsg,
                        RemoveMsg,
                        MB_ICONQUESTION | MB_YESNO ) != IDYES)
        {
            return (TRUE);
        }
    }

    if (RegOpenKey( HKEY_CURRENT_USER,
                    c_szRegPathCursors,
                    &hk ) == ERROR_SUCCESS)
    {
        HKEY hks;

        if (RegOpenKey(hk, c_szSchemes, &hks) == ERROR_SUCCESS)
        {
            RegDeleteValue(hks, szSchemeName);
            RegCloseKey(hks);
        }

        RegCloseKey(hk);
    }

    //
    //  Delete from list box.
    //
    index = (int)SendMessage( ghwndSchemeCB,
                              CB_FINDSTRINGEXACT,
                              (WPARAM)-1,
                              (LPARAM)szSchemeName );

    SendMessage(ghwndSchemeCB, CB_DELETESTRING, (WPARAM)index, 0);

    SendMessage(ghwndSchemeCB, CB_SETCURSEL, 0, 0);
    SendMessage(ghwndDlg, WM_NEXTDLGCTL, 1, 0L);

    EnableWindow(GetDlgItem(ghwndDlg, ID_REMOVESCHEME), FALSE);
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  InitSchemeComboBox
//
////////////////////////////////////////////////////////////////////////////

BOOL InitSchemeComboBox()
{
    TCHAR pszSchemeName[MAX_SCHEME_NAME_LEN + 1];
    TCHAR pszDefaultSchemeName[MAX_SCHEME_NAME_LEN + 1];
    TCHAR pszLongName[MAX_SCHEME_SUFFIX + MAX_SCHEME_NAME_LEN + 1];
    int index;
    HKEY hk;
    DWORD len;

    LoadString(g_hInst, IDS_NONE, szNone, ARRAYSIZE(szNone));
    LoadString(g_hInst, IDS_SUFFIX, szSystemScheme, ARRAYSIZE(szSystemScheme));

    if (RegOpenKey(HKEY_CURRENT_USER, c_szRegPathCursors, &hk) == ERROR_SUCCESS)
    {
        HKEY hks;

        //
        //  Enumerate the schemes.
        //
        if (RegOpenKey(hk, c_szSchemes, &hks) == ERROR_SUCCESS)
        {
            DWORD i;

            for (i = 0; ;i++)
            {
                LONG ret;

                //
                //  Reset each pass.
                //
                len = ARRAYSIZE(pszSchemeName);

                ret = RegEnumValue( hks,
                                    i,
                                    pszSchemeName,
                                    &len,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL );

                if (ret == ERROR_MORE_DATA)
                {
                    continue;
                }

                if (ret != ERROR_SUCCESS)
                {
                    break;
                }

                //
                //  HACK to keep "NONE" pure.
                //
                if (lstrcmpi(pszSchemeName, szNone) != 0)
                {
                    SendMessage( ghwndSchemeCB,
                                 CB_ADDSTRING,
                                 0,
                                 (LPARAM)pszSchemeName );
                }
            }

            //
            //  At this point, all of the user defined scheme names have been
            //  added to the combo box.
            //
            RegCloseKey(hks);
        }

        //
        //  Get name of current one.
        //
        //  Reset again.
        //
        len = sizeof(pszDefaultSchemeName);

        RegQueryValue(hk, NULL, pszDefaultSchemeName, &len);

        //
        //  Try to read the value of Scheme Source.  If this value doesn't
        //  exist, then we have a pre NT 5.0 implementation, so all schemes
        //  will be user schemes.
        //
        len = sizeof(iSchemeLocation);
        if (RegQueryValueEx( hk,
                             szSchemeSource,
                             NULL,
                             NULL,
                             (unsigned char *)&iSchemeLocation,
                             &len ) != ERROR_SUCCESS)
        {
            iSchemeLocation = ID_USER_SCHEME;
        }

        RegCloseKey(hk);
    }

    //
    //  Now add the system defined pointer schemes.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szRegPathSystemSchemes, &hk) == ERROR_SUCCESS)
    {
        DWORD i;

        for (i = 0; ;i++)
        {
            LONG ret;

            //
            //  Reset each pass.
            //
            len = ARRAYSIZE(pszSchemeName);

            ret = RegEnumValue( hk,
                                i,
                                pszSchemeName,
                                &len,
                                NULL,
                                NULL,
                                NULL,
                                NULL );

            //
            //  If the Scheme name is longer than the allowed length, skip it.
            //
            if (ret == ERROR_MORE_DATA)
            {
                continue;
            }

            //
            //  If there's an error, then we're done.
            //
            if (ret != ERROR_SUCCESS)
            {
                break;
            }

            //
            //  When we add the system identifier to the string, it could be
            //  longer than MAX_SCHEME_NAME, however we only want to read
            //  max length from the registry.
            //
            lstrcpy(pszLongName, pszSchemeName);
            lstrcat(pszLongName, szSystemScheme);
            SendMessage(ghwndSchemeCB, CB_ADDSTRING, 0, (LPARAM)pszLongName);
        }

        RegCloseKey(hk);
    }

    //
    //  Add the "none" scheme.
    //
    SendMessage(ghwndSchemeCB, CB_INSERTSTRING, 0, (LPARAM)szNone);

    //
    //  Try to find current one in the combobox.
    //
    lstrcpy(pszLongName, pszDefaultSchemeName);
    if (iSchemeLocation == ID_OS_SCHEME)
    {
        lstrcat(pszLongName, szSystemScheme);
    }
    index = (int)SendMessage( ghwndSchemeCB,
                              CB_FINDSTRINGEXACT,
                              0xFFFF,
                              (LPARAM)pszLongName );

    //
    //  If found, select it.
    //
    if (index < 0)           // if we are on the None scheme
    {
        iSchemeLocation = ID_NONE_SCHEME;
        index = 0;
    }

    // We keep around a selection indicator so we know when selection has changed.
    // Initialize that value here.
    lstrcpy(gszPreviousScheme, pszLongName);

    SendMessage(ghwndSchemeCB, CB_SETCURSEL, (WPARAM)index, 0);

    EnableWindow( GetDlgItem(ghwndDlg, ID_REMOVESCHEME),
                  (iSchemeLocation == ID_USER_SCHEME) );

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SaveSchemeDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK SaveSchemeDlgProc(
    HWND  hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szSchemeName[MAX_SCHEME_SUFFIX + MAX_SCHEME_NAME_LEN + 1];

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            HourGlass(TRUE);

            GetWindowText(ghwndSchemeCB, szSchemeName, ARRAYSIZE(szSchemeName));

            //
            //  CANNOT SAVE "NONE" SCHEME.
            //
            if (lstrcmpi(szSchemeName, szNone) == 0)
            {
                *szSchemeName = 0;
            }

            iSchemeLocation = SystemOrUser(szSchemeName);

            SetDlgItemText(hWnd, ID_SCHEMEFILENAME,  szSchemeName);

            SendDlgItemMessage(hWnd, ID_SCHEMEFILENAME, EM_SETSEL, 0, 32767);

            SendDlgItemMessage( hWnd,
                                ID_SCHEMEFILENAME,
                                EM_LIMITTEXT,
                                MAX_SCHEME_NAME_LEN,
                                0L );

            EnableWindow(GetDlgItem(hWnd, IDOK), szSchemeName[0] != TEXT('\0'));

            HourGlass(FALSE);
            return (TRUE);
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aHelpIDs );
            return (TRUE);
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPVOID)aHelpIDs );
            return (TRUE);
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( ID_SCHEMEFILENAME ) :
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        //
                        //  CANNOT SAVE "NONE" SCHEME
                        //  cannot save a scheme ending with szSystemScheme
                        //
                        EnableWindow(
                            GetDlgItem(hWnd, IDOK),
                            ((GetDlgItemText( hWnd,
                                              ID_SCHEMEFILENAME,
                                              szSchemeName,
                                              ARRAYSIZE(szSchemeName) ) > 0) &&
                             (lstrcmpi(szSchemeName, szNone) != 0) &&
                             (SystemOrUser(szSchemeName) != ID_OS_SCHEME)) );
                    }
                    break;
                }
                case ( IDOK ) :
                {
                    GetDlgItemText( hWnd,
                                    ID_SCHEMEFILENAME,
                                    szSchemeName,
                                    MAX_SCHEME_NAME_LEN + 1 );

                    CurStripBlanks(szSchemeName);

                    if (*szSchemeName == TEXT('\0'))
                    {
                        MessageBeep(0);
                        break;
                    }

                    lstrcpy(gszSchemeName, szSchemeName);

                    // fall through...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hWnd, LOWORD(wParam) == IDOK);
                    return (TRUE);
                }
            }
        }
    }

    //
    //  Didn't process a message.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MakeFilename
//
//  Returns Filename with a default path in system directory if no path
//  is already specified.
//
////////////////////////////////////////////////////////////////////////////

LPTSTR MakeFilename(
    LPTSTR sz)
{
    TCHAR szTemp[MAX_PATH];

    ExpandEnvironmentStrings(sz, szTemp, MAX_PATH);

    if (szTemp[0] == TEXT('\\') || szTemp[1] == TEXT(':'))
    {
        lstrcpy(gszFileName2, szTemp);

        return (gszFileName2);
    }
    else
    {
        GetSystemDirectory(gszFileName2, ARRAYSIZE(gszFileName2));

        lstrcat(gszFileName2, TEXT("\\"));
        lstrcat(gszFileName2, szTemp);

        return (gszFileName2);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CurStripBlanks
//
//  Strips leading and trailing blanks from a string.
//  Alters the memory where the string sits.
//
////////////////////////////////////////////////////////////////////////////

void CurStripBlanks(LPTSTR pszString)
{
    LPTSTR pszPosn;

    //
    //  Strip leading blanks.
    //
    pszPosn = pszString;

    while (*pszPosn == TEXT(' '))
    {
        pszPosn++;
    }

    if (pszPosn != pszString)
    {
        lstrcpy(pszString, pszPosn);
    }

    //
    //  Strip trailing blanks.
    //
    if ((pszPosn = pszString + lstrlen(pszString)) != pszString)
    {
       pszPosn = CharPrev(pszString, pszPosn);

       while (*pszPosn == TEXT(' '))
       {
           pszPosn = CharPrev(pszString, pszPosn);
       }

       pszPosn = CharNext(pszPosn);

       *pszPosn = TEXT('\0');
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SystemOrUser
//
//  Attempts to determine if the scheme name selected from the combo
//  box ends with the string szSystemScheme and retuns ID_OS_SCHEME
//  if it does, ID_USER_SCHEME if it doesn't.
//
////////////////////////////////////////////////////////////////////////////

int SystemOrUser(TCHAR *pszSchemeName)
{
    TCHAR *pszSN;
    int lenSS, lenSN;
    int i;

    lenSS = lstrlen(szSystemScheme);
    lenSN = lstrlen(pszSchemeName);

    if (lenSN <= lenSS)
    {
        return (ID_USER_SCHEME);
    }

    pszSN = pszSchemeName + (lenSN - lenSS);

    //
    //  If these strings are different, it's a user scheme.
    //
    if (lstrcmpi(pszSN, szSystemScheme))
    {
        return (ID_USER_SCHEME);
    }

    //
    //  For system schemes, this function also removes the
    //  szSystemScheme string from the end.
    //
    *pszSN = TEXT('\0');

    return (ID_OS_SCHEME);
}
