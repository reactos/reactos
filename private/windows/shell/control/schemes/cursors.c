/** FILE: cursors.c ********** Module Header ********************************
 *
 *  Control panel applet for Cursors configuration.  This file holds
 *  everything to do with the "Cursors" dialog box in the Control Panel.
 *
 *  If this applet gets incorporated into the MAIN.CPL module, look
 *  for the define NOTINMAIN below.  This marks some code that will
 *  need to be modified.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-    Steve Cathcart   [stevecat]
 *      Took base code from Win 3.1 source
 * 12-22-91 DarrinM     Created from MOUSE.C
 * 07-31-92 DarrinM     Revived.
 * 01-04-93 ByronD      Cleaned up, etc.
 * 03-25-93 JonPa       Changed .ANI file format from RAD to RIFF
 * 04-29-93 JonPa       Pull strings out of resource file
 * 05-20-93 ErikGav     Added schemes
 *
 *  Copyright (C) 1990-1993 Microsoft Corporation
 *
 *************************************************************************/

//
// Marks code that will need to be modified if this module is ever
// incorporated into MAIN.CPL instead of being a separate applet.
//
#define NOTINMAIN


//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Windows SDK
/* cut out unnec stuff from windows.h */
#define NOCLIPBOARD
#define NOMETAFILE
#define NOSYSCOMMANDS
#define NOGDICAPMASKS

#include <windows.h>

// Common dialog includes.
#include <dlgs.h>
#include <commdlg.h>

// Utility library
#include <cplib.h>

// Application specific
#include "cursors.h"
#include <winuserp.h>

#ifdef NOTINMAIN
#include "..\main\cphelp.h"         //For the help id's.
#endif //NOTINMAIN

#ifndef RAD_FORMAT
#include <asdf.h>
#endif

//==========================================================================
//                          Local Definitions
//==========================================================================

#ifndef LATER
// darrinm - 07/31/92
// Replace with something good

#define gcxAvgChar              8
#endif

#define CCURSORS                11
#define MAX_SCHEME_NAME_LEN     32

#define PM_NEWCURSOR            (WM_USER + 1)
#define PM_PAUSEANIMATION       (WM_USER + 2)
#define PM_UNPAUSEANIMATION     (WM_USER + 3)

#define ID_PREVIEWTIMER         1


typedef struct _CURSORINFO {    // curi
    DWORD fl;
    HCURSOR hcur;
    int ccur;
    int icur;
    char szFile[FILENAME_MAX];
    char szTitle[80];
    char szCreator[80];
} CURSORINFO, *PCURSORINFO;

#define CIF_FILE        0x0001
#define CIF_ANICURSOR   0x0002
#define CIF_MODIFIED    0x0004
#define CIF_TITLE       0x0008
#define CIF_CREATOR     0x0010

#pragma pack(2)
typedef struct tagNEWHEADER {
    WORD reserved;
    WORD rt;
    WORD cResources;
} NEWHEADER, *LPNEWHEADER;
#pragma pack()

typedef struct
{
    UINT  idVisName;
    LPSTR idResource;
    LPSTR pszIniName;
    LPSTR pszVisName;
} CURSORDESC, *PCURSORDESC;

//
// Structure that contains data used within a preview window.  This
// data is unique for each preview window, and is used to optimize
// the painting.
//
typedef struct
{
    HDC hdcMem;
    HBITMAP hbmMem;
    HBITMAP hbmOld;
    PCURSORINFO pcuri;
} PREVIEWDATA, *PPREVIEWDATA;


//==========================================================================
//                          Local Data Declarations
//==========================================================================

extern HANDLE ghmod;                // These guys are defined in INIT.C
extern int gcxCursor, gcyCursor;

HWND ghwndDlg, ghwndFile, ghwndFileH, ghwndTitle, ghwndTitleH;
HWND ghwndCreator, ghwndCreatorH, ghwndLB, ghwndPreview;
HWND ghwndSchemeCB;
HBRUSH ghbrHighlight, ghbrHighlightText, ghbrWindow;
COLORREF gcrHighlightText;
TCHAR gszFileName[MAX_PATH];
TCHAR gszFileName2[MAX_PATH];
HFONT ghfontLabels;
UINT gnMsgLBSelChString;
UINT gnMsgFileOK;
UINT wBrowseHelpMessage;
LPTSTR gszFileNotFound = NULL;
LPTSTR gszBrowse = NULL;
LPTSTR gszFilter = NULL;
TCHAR gszNoMem[256] = "No Memory";
HHOOK ghhkMsgFilter;                // Hook handle for message filter func.

CURSORDESC gacd[] =
{
    { IDS_ARROW,    IDC_ARROW,    "Arrow",    NULL },
    { IDS_WAIT,     IDC_WAIT,     "Wait",     NULL },
    { IDS_APPSTARTING, IDC_APPSTARTING, "AppStarting", NULL },
    { IDS_NO,       IDC_NO,       "No",       NULL },
    { IDS_IBEAM,    IDC_IBEAM,    "IBeam",    NULL },
    { IDS_CROSS,    IDC_CROSS,    "CrossHair",NULL },
    { IDS_SIZENS,   IDC_SIZENS,   "SizeNS",   NULL },
    { IDS_SIZEWE,   IDC_SIZEWE,   "SizeWE",   NULL },
    { IDS_SIZENWSE, IDC_SIZENWSE, "SizeNWSE", NULL },
    { IDS_SIZENESW, IDC_SIZENESW, "SizeNESW", NULL },
    { IDS_SIZEALL,  IDC_SIZEALL,  "SizeAll",  NULL }
};

CURSORINFO acuri[CCURSORS];

// registry keys
char szSchemeINI[] = "Cursor Schemes";
char szCurrentINI[] = "Current";
char gszSchemeName[MAX_SCHEME_NAME_LEN+1];

//==========================================================================
//                          Local Function Prototypes
//==========================================================================

BOOL InitCursorsDlg(HWND hwnd);
VOID CreateBrushes(VOID);
VOID DestroyBrushes(VOID);
LPTSTR GetResourceString(HINSTANCE hmod, int id);
void DrawCursorListItem(DRAWITEMSTRUCT *pdis);
BOOL GetCursorFromFile(CURSORINFO *pcuri);
BOOL Browse(HWND hwndOwner);
BOOL CALLBACK OpenFileNameHookDlgProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
void CleanUpEverything(void);
VOID UpdateCursorList(VOID);
VOID NextFrame(HWND hwnd);
BOOL ReadTag( HANDLE hf, PRTAG ptag);
BOOL ReadChunk(HANDLE hf, PRTAG ptag, PVOID pv);
BOOL ReadChunkN(HANDLE hf, PRTAG ptag, PVOID pv, DWORD cbMax);
DWORD Skip( HANDLE hf, DWORD cbSkip);
void HourGlass(BOOL fOn);
DWORD FAR PASCAL MsgFilterHookFunc(INT nCode, WPARAM wParam, LPMSG lpMsg);

BOOL TryToLoadCursor(HWND hwnd,int i,CURSORINFO *pcuri);
BOOL LoadScheme(void);
BOOL SaveScheme(void);
BOOL SaveSchemeAs(void);
BOOL RemoveScheme(void);
BOOL InitSchemeComboBox(void);
BOOL SchemeUpdate(int);
LPSTR MakeFilename(LPSTR sz);
BOOL CALLBACK SaveSchemeDlgProc(HWND  hWnd, UINT message, DWORD wParam, LONG lParam);
void StripBlanks (LPSTR pszString);


//==========================================================================
//                              Functions
//==========================================================================

#ifdef NOTINMAIN //NOTINMAIN

//
// This is a hack to get around the mess in CPLIB.H.  It can all go away
// when this applet is incorporated into MAIN.CPL.
//
#define wHelpMessage    xwHelpMessage
#define dwContext       xdwContext
#define szControlHlp    xszControlHlp
#define CPHelp          xCPHelp


UINT    wHelpMessage;           // stuff for help
DWORD   dwContext = 0L;
char szControlHlp[] = "control.hlp";

void CPHelp (HWND hWnd)
{
//    char    szBuf[80];

//    wsprintf (szBuf, "Help Context %ld", dwContext);

    WinHelp (hWnd, szControlHlp, HELP_CONTEXT, dwContext);
//    hWnd = hWnd;
}

#endif //NOTINMAIN


/***************************************************************************\
* InitCursorsDlg
*
*
* History:
* 12-22-91 DarrinM      Created.
* 04-10-93 ErikGav      Added init for schemes
\***************************************************************************/

BOOL InitCursorsDlg(
    HWND hwnd)
{
    CURSORINFO *pcuri;
    LOGFONT lf;
    int i;
    DWORD dwDummy;
    static TCHAR szBuffer[256];
    LPTSTR p,p2;

#ifdef NOTINMAIN
    wHelpMessage = RegisterWindowMessage("ShellHelp");
    dwContext = IDH_CHILD_CURSORS;
#endif //NOTINMAIN

    GetProfileString(szCurrentINI,szSchemeINI,"",szBuffer,sizeof(szBuffer));

    if (szBuffer[0] == '\0')
    {
        //
        // this key doesn't exist-- this is the first time
        // the applet has been run.  Create the default
        // scheme sets.
        //

        LoadString(ghmod, IDS_DEFAULTSCHEME, szBuffer, sizeof(szBuffer));
        WriteProfileString(szCurrentINI,szSchemeINI,szBuffer);

        // fill in the schemes for
        // the listbox

        for (i=IDS_FIRSTSCHEME;i<=IDS_LASTSCHEME;i++)
        {
            LoadString(ghmod, i, szBuffer, sizeof(szBuffer));
            p = szBuffer;

            while (*p != '=') p = CharNext(p);
            p2 = CharNext(p);
            *p = '\0';

            WriteProfileString(szSchemeINI,szBuffer,p2);
        }
    }

    //
    // Register the help message from the File Open (Browse) dialog.
    //

    wBrowseHelpMessage = RegisterWindowMessage(HELPMSGSTRING);
    LoadAccelerators(ghmod, MAKEINTRESOURCE(CP_ACCEL));

    /*
     * Load Strings
     */

    if (gszFileNotFound == NULL) {

        gszFileNotFound = GetResourceString(ghmod, IDS_CUR_BADFILE);

        if (gszFileNotFound == NULL) {
            return FALSE;
        }
    }


    if (gszBrowse == NULL) {

        gszBrowse = GetResourceString(ghmod, IDS_CUR_BROWSE);

        if (gszBrowse == NULL) {
            return FALSE;
        }
    }

    if (gszFilter == NULL) {
        gszFilter = GetResourceString(ghmod, IDS_CUR_FILTER);
        if (gszFilter == NULL) {
            return FALSE;
        }
    }

    /*
     * Load description strings from the resource file
     */
    for( i = 0; i < CCURSORS; i++ ) {

        if (gacd[i].idVisName != 0) {
            gacd[i].pszVisName =  GetResourceString(ghmod, gacd[i].idVisName);

            if (gacd[i].pszVisName != NULL) {
                /*
                 * We got a buffer for the string,
                 * clear the string id so we won't try to load it again.
                 */
                gacd[i].idVisName = 0;
            } else {
                /*
                 * Could not get the string.  Use the registry name in this
                 * emergency case.
                 */
                gacd[i].pszVisName = gacd[i].pszIniName;
            }
        }
    }


    /*
     * As an optimization, remember the window handles of the cursor
     * information fields.
     */
    ghwndPreview = GetDlgItem(hwnd, ID_PREVIEW);
    ghwndFile = GetDlgItem(hwnd, ID_FILE);
    ghwndFileH = GetDlgItem(hwnd, ID_FILEH);
    ghwndTitle = GetDlgItem(hwnd, ID_TITLE);
    ghwndTitleH = GetDlgItem(hwnd, ID_TITLEH);
    ghwndCreator = GetDlgItem(hwnd, ID_CREATOR);
    ghwndCreatorH = GetDlgItem(hwnd, ID_CREATORH);
    ghwndLB = GetDlgItem(hwnd, ID_CURSORLIST);
    ghwndSchemeCB = GetDlgItem(hwnd, ID_SCHEMECOMBO);
    /*
     * Create some brushes we'll be using.
     */
    CreateBrushes();

    // ErikGav -
    // Initialize the scheme list box

    InitSchemeComboBox();

    /*
     * Get a nice small font for some of the controls.
     * LATER: How many of these fields do I really need to initialize?
     */
    memset(&lf, 0, sizeof(LOGFONT));
    strcpy(lf.lfFaceName, "Helv");
    lf.lfHeight = 8;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    ghfontLabels = CreateFontIndirect(&lf);

    /*
     * Change the font for the info fields.
     */
    SendMessage(ghwndFile, WM_SETFONT, (DWORD)ghfontLabels, FALSE);
    SendMessage(ghwndTitle, WM_SETFONT, (DWORD)ghfontLabels, FALSE);
    SendMessage(ghwndCreator, WM_SETFONT, (DWORD)ghfontLabels, FALSE);

    /*
     * Pre-clear the cursor info array.
     */
    memset(acuri, 0, sizeof(acuri));

    /*
     * Loop through all cursors checking WIN.INI to see if they've been
     * superceded by a 'soft' cursor.  If so, get the soft cursor's filename.
     */
    for (pcuri = &acuri[0], i = 0; i < CCURSORS; i++, pcuri++) {

        GetProfileString("Cursors", gacd[i].pszIniName, "", pcuri->szFile,
                sizeof(pcuri->szFile));

        if ( (*(pcuri->szFile) == '\0') || !TryToLoadCursor(hwnd,i,pcuri))
        {
            /*
             * The cursor is either not redirected, or we could not load
             * the file, either way we should use the default system cursor.
             */
            pcuri->hcur = LoadCursor(NULL, gacd[i].idResource);
            GetCursorInfo(pcuri->hcur, NULL, 0, &dwDummy, &pcuri->ccur);
            if (pcuri->ccur > 1) {
                pcuri->fl |= CIF_ANICURSOR;
            }
        }
        SendMessage(ghwndLB, LB_ADDSTRING, 0, i);
    }

    /*
     * Select the first cursor in the list ('Arrow').
     */
    SendMessage(ghwndLB, LB_SETCURSEL, 0, 0);

    /*
     * Force an update of the preview window and the cursor details.
     */
    UpdateCursorList();

    gnMsgLBSelChString = RegisterWindowMessage(LBSELCHSTRING);
    gnMsgFileOK = RegisterWindowMessage(FILEOKSTRING);

    return TRUE;
}



/*****************************************************************************\
* CreateBrushes
*
* Creates the brushes that are used to paint within the Cursors applet.
*
\*****************************************************************************/

VOID
CreateBrushes(
    VOID
    )
{
    ghbrHighlight = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
    gcrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    ghbrHighlightText = CreateSolidBrush(gcrHighlightText);
    ghbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
}



/*****************************************************************************\
* DestroyBrushes
*
* Cleans up the brushes that were used to paint within the Cursors applet.
*
\*****************************************************************************/

VOID
DestroyBrushes(
    VOID
    )
{
    DeleteObject(ghbrHighlight);
    DeleteObject(ghbrHighlightText);
    DeleteObject(ghbrWindow);
}


/*****************************************************************************\
* LPTSTR GetResourceString(HINSTANCE hmod, int id);
*
* Gets a string out of the resouce file.
*
\*****************************************************************************/
LPTSTR GetResourceString(HINSTANCE hmod, int id) {
    TCHAR szBuffer[256];
    LPTSTR psz;
    int cch;

    if ( (cch = LoadString(hmod, id, szBuffer, sizeof(szBuffer) /
            sizeof(TCHAR) )) == 0){
        return NULL;
    }

    psz = LocalAlloc(LPTR, cch * sizeof(TCHAR));

    if (psz != NULL) {
        int i;

        for( i = 0; i <= cch; i++ ) {
            psz[i] = (szBuffer[i] == TEXT('\1')) ?
                    TEXT('\0') : szBuffer[i];
        }
    }

    return psz;
}


/***************************************************************************\
* CursorsDlgProc
*
*
* History:
* 12-22-91 DarrinM      Created.
\***************************************************************************/

BOOL CALLBACK
CursorsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CURSORINFO *pcuri;
    DWORD dwDummy;
    HDWP hdwp;
    INT i;

    switch (msg) {
    case WM_INITDIALOG:
        ghwndDlg = hwnd;
        if(!InitCursorsDlg(hwnd)) {
            MessageBox(hwnd, gszNoMem, NULL, MB_ICONSTOP | MB_OK);
            EndDialog(hwnd, 0);
        }
        break;

    case WM_MEASUREITEM:
        ((MEASUREITEMSTRUCT *)lParam)->itemHeight = gcyCursor + 2;
        break;

    case WM_DRAWITEM:
        DrawCursorListItem((DRAWITEMSTRUCT *)lParam);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case ID_SCHEMECOMBO:
            switch (HIWORD(wParam))
            {
                case CBN_SELCHANGE:
                    LoadScheme();
                break;
            }

            break;

        case ID_DEFAULT:
            /*
             * Throw away any fancy new cursor and replace it with the
             * system's original.
             */
            i = SendMessage(ghwndLB, LB_GETCURSEL, 0, 0);
            pcuri = &acuri[i];
            if (!(pcuri->fl & CIF_FILE))
                break;

            pcuri->fl = CIF_MODIFIED;

            if (pcuri->hcur != NULL)
                DestroyCursor(pcuri->hcur);

            pcuri->hcur = GetCursorInfo(NULL, (LPWSTR)gacd[i].idResource, 0,
                &dwDummy, (LPINT)&dwDummy);
            *pcuri->szFile = '\0';

            EnableWindow(GetDlgItem(hwnd, ID_SAVESCHEME), TRUE);

            UpdateCursorList();
            break;

        case ID_CURSORLIST:
            switch (HIWORD(wParam)) {
            case LBN_SELCHANGE:
                i = SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                pcuri = &acuri[i];

                /*
                 * Show a preview (including animation) in the preview window.
                 */
                PreviewWndProc(ghwndPreview, PM_NEWCURSOR, 0, (LPARAM)pcuri);

                /*
                 * Show/Hide and update if necessary the information text
                 * controls that show the cursor's filename, title, and
                 * creator.
                 */
                hdwp = BeginDeferWindowPos(6);
                if (pcuri->fl & CIF_TITLE) {
                    SetWindowText(ghwndTitle, pcuri->szTitle);
                    hdwp = DeferWindowPos(hdwp, ghwndTitleH, NULL, 0, 0, 0, 0,
                        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                    hdwp = DeferWindowPos(hdwp, ghwndTitle, NULL, 0, 0, 0, 0,
                        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                } else {
                    hdwp = DeferWindowPos(hdwp, ghwndTitleH, NULL, 0, 0, 0, 0,
                        SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                    hdwp = DeferWindowPos(hdwp, ghwndTitle, NULL, 0, 0, 0, 0,
                        SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                }

                if (pcuri->fl & CIF_CREATOR) {
                    SetWindowText(ghwndCreator, pcuri->szCreator);
                    hdwp = DeferWindowPos(hdwp, ghwndCreatorH, NULL, 0, 0, 0, 0,
                        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                    hdwp = DeferWindowPos(hdwp, ghwndCreator, NULL, 0, 0, 0, 0,
                        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                } else {
                    hdwp = DeferWindowPos(hdwp, ghwndCreatorH, NULL, 0, 0, 0, 0,
                        SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                    hdwp = DeferWindowPos(hdwp, ghwndCreator, NULL, 0, 0, 0, 0,
                        SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                }

                if (pcuri->fl & CIF_FILE) {
                    SetWindowText(ghwndFile, pcuri->szFile);
                    hdwp = DeferWindowPos(hdwp, ghwndFileH, NULL, 0, 0, 0, 0,
                        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                    hdwp = DeferWindowPos(hdwp, ghwndFile, NULL, 0, 0, 0, 0,
                        SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                } else {
                    hdwp = DeferWindowPos(hdwp, ghwndFileH, NULL, 0, 0, 0, 0,
                        SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                    hdwp = DeferWindowPos(hdwp, ghwndFile, NULL, 0, 0, 0, 0,
                        SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
                }
                EndDeferWindowPos(hdwp);

                //
                // Enable the "Set Default" button if the cursor is
                // from a file.
                //
                EnableWindow(GetDlgItem(hwnd, ID_DEFAULT),
                    (pcuri->fl & CIF_FILE) ? TRUE : FALSE);

                break;

            case LBN_DBLCLK:
                Browse(hwnd);
                break;
            }
            break;

        case ID_BROWSE:
            Browse(hwnd);
            break;


        case IDD_HELP:
            goto DoHelp;

        case IDOK:
            /*
             * change cursor to hourglass
             */
            HourGlass(TRUE);

            // Save the modified scheme
            SaveScheme();

            /*
             * Save any modified cursor information to WIN.INI.
             */
            for (pcuri = &acuri[0], i = 0; i < CCURSORS; i++, pcuri++) {
//                if (pcuri->fl & CIF_MODIFIED) {
                    WriteProfileString("Cursors", gacd[i].pszIniName,
                        pcuri->fl & CIF_FILE ? pcuri->szFile : NULL);
                    SetSystemCursor(pcuri->hcur, (DWORD)gacd[i].idResource);
//                }
            }

            HourGlass(FALSE);

            /*
             * Free everything.
             */
            CleanUpEverything();

            EndDialog(hwnd, 0L);
            break;

        case IDCANCEL:
            /*
             * Don't leave a mess.
             */
            CleanUpEverything();

            EndDialog(hwnd, 0L);
            break;

        case ID_SAVESCHEME:
            SaveSchemeAs();
            break;

        case ID_REMOVESCHEME:
            RemoveScheme();
            break;
        }
        break;

    case WM_SYSCOLORCHANGE:
        //
        // Recreate the brushes with the new colors.
        //
        DestroyBrushes();
        CreateBrushes();
        break;

    default:
        if (msg == wHelpMessage || msg == wBrowseHelpMessage)
        {
DoHelp:
            CPHelp(hwnd);

            return TRUE;
        }
        else
            return FALSE;

        break;
    }

    return TRUE;
}


/***************************************************************************\
* DrawCursorListItem
*
*
* History:
* 12-22-91 DarrinM      Created.
\***************************************************************************/

void DrawCursorListItem(
    DRAWITEMSTRUCT *pdis)
{
    CURSORINFO *pcuri;
    COLORREF crPrev;

    /*
     * LATER: Deal with focus rect.
     */
    if (pdis->itemAction == ODA_FOCUS)
        return;

    SetBkMode(pdis->hDC, TRANSPARENT);
    pcuri = &acuri[pdis->itemData];

    if (pdis->itemState & ODS_SELECTED) {
        FillRect(pdis->hDC, &pdis->rcItem, ghbrHighlight);
        crPrev = SetTextColor(pdis->hDC, gcrHighlightText);
    } else {
        FillRect(pdis->hDC, &pdis->rcItem, ghbrWindow);
    }
    if (pcuri->hcur != NULL) {
        DrawIcon(pdis->hDC, pdis->rcItem.left + 2, pdis->rcItem.top + 1,
                pcuri->hcur);
    }
    pdis->rcItem.left += gcxCursor + 2 + gcxAvgChar;
    DrawText(pdis->hDC, gacd[pdis->itemData].pszVisName,
            strlen(gacd[pdis->itemData].pszVisName), &pdis->rcItem,
            DT_SINGLELINE | DT_LEFT | DT_VCENTER);

    if (pdis->itemState & ODS_SELECTED) {
        SetTextColor(pdis->hDC, crPrev);
    }
}



/***************************************************************************\
* TryToLoadCursor
*
*
* History:
* 01-28-93 JonPa        Created.
\***************************************************************************/
BOOL TryToLoadCursor(
    HWND hwnd,
    int i,
    CURSORINFO *pcuri)
{
    DWORD dwDummy;
    BOOL fRet = TRUE;

    if (!GetCursorFromFile(pcuri)) {
        HWND hwndControl = GetParent(hwnd);
        LPSTR pszText;

        pszText = LocalAlloc(LPTR, strlen(gszFileNotFound) +
                strlen(gacd[i].pszVisName) + strlen(pcuri->szFile));

        if (pszText == NULL)
            return FALSE;

        wsprintf(pszText, gszFileNotFound, pcuri->szFile, gacd[i].pszVisName);

        /* do a little multimedia action here */
        MessageBeep(MB_ICONEXCLAMATION);

        if(MessageBox(hwndControl, pszText, NULL,
                MB_ICONEXCLAMATION | MB_YESNO) == IDYES) {
            /*
             * Cursor file is bad or can't be found.  User wants to
             * reset registry to use the default instead.
             */

            pcuri->fl = CIF_MODIFIED;

            if (pcuri->hcur != NULL)
                DestroyCursor(pcuri->hcur);

            pcuri->hcur = GetCursorInfo(NULL, (LPWSTR)gacd[i].idResource, 0,
                &dwDummy, (LPINT)&dwDummy);

            GetCursorInfo(pcuri->hcur, NULL, 0, &dwDummy, &pcuri->ccur);
            if (pcuri->ccur > 1) {
                pcuri->fl |= CIF_ANICURSOR;
            }

        } else {
            /* load file failed, use the default */
            fRet = FALSE;
        }

        LocalFree(pszText);
    }
    /* Load file worked! Display what we read */
    return fRet;
}

/***************************************************************************\
* GetCursorFromFile
*
*
* History:
* 12-25-91 DarrinM      Created.
* 03-25-93 JonPa        Rewote to use RIFF format
\***************************************************************************/

BOOL GetCursorFromFile(
    CURSORINFO *pcuri)
{
    HANDLE hf;
    RTAG tag;
    DWORD cbRead;
    BOOL fBadCur = FALSE;
    ANIHEADER anih;

    pcuri->fl = 0;

    hf = CreateFile(MakeFilename(pcuri->szFile), GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, 0, NULL);

    if (hf == INVALID_HANDLE_VALUE) {
        pcuri->fl |= CIF_FILE;
        return FALSE;
    }

    /*
     * Determine if this is an .ICO/.CUR file or an .ANI file.
     */
    if (ReadTag(hf, &tag) && tag.ckID == FOURCC_RIFF &&
            ReadFile(hf, &tag.ckID, sizeof(tag.ckID), &cbRead, NULL) &&
            cbRead >= sizeof(tag.ckID) && tag.ckID == FOURCC_ACON) {
        BOOL fNeedANI = TRUE;
        BOOL fNeedInfo = TRUE;

        /*
         * It's an ANICURSOR!
         *
         * Search for each chunk (LIST, anih, rate, seq, and icon).
         * while we search, we will assume the ani file is bad.
         */
        fBadCur = TRUE;
        while((fNeedANI || fNeedInfo) && ReadTag(hf, &tag)) {
            if( tag.ckID == FOURCC_LIST) {
                /* look for type INFO */
                DWORD cbChunk = (tag.ckSize + 1) & ~1;

                if (ReadFile(hf, &tag.ckID, sizeof(tag.ckID), &cbRead, NULL) &&
                        cbRead >= sizeof(tag.ckID) && tag.ckID == FOURCC_INFO){

                    //BUGBUG I think this should be here???
                    cbChunk -= cbRead;

                    /* now look for INAM and IART chunks */

                    while( cbChunk >= sizeof(tag) &&
                                ((pcuri->fl & (CIF_TITLE | CIF_CREATOR)) !=
                                (CIF_TITLE | CIF_CREATOR))) {

                        if (!ReadTag(hf, &tag))
                            goto gcffExit;

                        cbChunk -= sizeof(tag);

                        switch( tag.ckID ) {
                        case FOURCC_INAM:
                            if (cbChunk < tag.ckSize ||
                                    !ReadChunkN(hf, &tag, pcuri->szTitle,
                                    sizeof(pcuri->szTitle)))
                                goto gcffExit;

                            pcuri->fl |= CIF_TITLE;
                            cbChunk -= (tag.ckSize + 1) & ~1;
                            break;

                        case FOURCC_IART:
                            if (cbChunk < tag.ckSize ||
                                    !ReadChunkN(hf, &tag, pcuri->szCreator,
                                    sizeof(pcuri->szCreator)))
                                goto gcffExit;

                            pcuri->fl |= CIF_CREATOR;
                            cbChunk -= (tag.ckSize + 1) & ~1;
                            break;

                        default:
                            cbChunk -= Skip( hf, tag.ckSize );
                            break;
                        }
                    }

                    fNeedInfo = FALSE;

                    if (cbChunk != 0) {
                        //BUGBUG This is right, isn't it?
                        Skip( hf, cbChunk );
                    }

                } else {
                    Skip( hf, cbChunk - cbRead);
                }

            } else if (tag.ckID == FOURCC_anih) {
                if (!ReadChunk(hf, &tag, &anih))
                    goto gcffExit;

                if (!(anih.fl & AF_ICON) || (anih.cFrames == 0))
                    goto gcffExit;

                fNeedANI = FALSE;

            } else {
                Skip(hf, tag.ckSize);
            }
        }

        /* if we get here, it must be a real ani cursor */
        fBadCur = FALSE;
        pcuri->fl |= CIF_ANICURSOR;
    }

gcffExit:
    CloseHandle(hf);

    if (!fBadCur) {
        pcuri->hcur = LoadCursorFromFile(MakeFilename(pcuri->szFile));
        GetCursorInfo(pcuri->hcur, NULL, 0, &cbRead, &pcuri->ccur);
        pcuri->fl |= CIF_FILE;
    }

    return fBadCur ? FALSE : pcuri->hcur != NULL;
}



/***************************************************************************\
* Browse
*
* Browse the file system for a new cursor for the selected item.
*
* History:
* 12-25-91 DarrinM      Created.
\***************************************************************************/

BOOL Browse(
    HWND hwndOwner)
{
    static OPENFILENAME ofn;
    static char szCustomFilter[80];
    CHAR szSystemDir[MAX_PATH];
    CURSORINFO curi;
    int i;
    DWORD dwContextSave;
    HOOKPROC lpfnMsgFilterHookFunc;   // The message filter proc instance.
    BOOL fRet = FALSE;

    /*
     * Hook the message filter stream so that we can detect F1 keystrokes.
     */
    lpfnMsgFilterHookFunc =
            MakeProcInstance((HOOKPROC)MsgFilterHookFunc, ghInst);
    ghhkMsgFilter =
            SetWindowsHook(WH_MSGFILTER, lpfnMsgFilterHookFunc);


    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = ghmod;
    ofn.lpstrFilter = gszFilter;
    ofn.lpstrCustomFilter = szCustomFilter;
    ofn.nMaxCustFilter = sizeof(szCustomFilter);
    ofn.nFilterIndex = 1;
    gszFileName[0] = 0;
    ofn.lpstrFile = gszFileName;
    ofn.nMaxFile = sizeof(gszFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    GetSystemDirectory(szSystemDir, MAX_PATH);
    ofn.lpstrInitialDir = szSystemDir;
    ofn.lpstrTitle = gszBrowse;
    ofn.Flags = OFN_SHOWHELP | OFN_FILEMUSTEXIST |
        OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = NULL;
    ofn.lpfnHook = (LPOFNHOOKPROC)OpenFileNameHookDlgProc;
    ofn.lpTemplateName = MAKEINTRESOURCE(DLG_FILEOPEN);

    SendDlgItemMessage(hwndOwner, ID_PREVIEW, PM_PAUSEANIMATION, 0, 0);

    dwContextSave = dwContext;
    dwContext = IDH_DLG_CURBROWSE;
    if (!GetOpenFileName(&ofn))
    {
        dwContext = dwContextSave;
        SendDlgItemMessage(hwndOwner, ID_PREVIEW, PM_UNPAUSEANIMATION, 0, 0);
        goto brErrExit;
    }
    dwContext = dwContextSave;

    strcpy(curi.szFile, gszFileName);
    if (!GetCursorFromFile(&curi))
        goto brErrExit;

    i = SendMessage(ghwndLB, LB_GETCURSEL, 0, 0);
    curi.fl |= CIF_MODIFIED;

    EnableWindow(GetDlgItem(ghwndDlg, ID_SAVESCHEME), TRUE);

    /*
     * Destroy the old cursor before we retain the new one.
     */
    DestroyCursor(acuri[i].hcur);

    acuri[i] = curi;
    UpdateCursorList();
    fRet = TRUE;

brErrExit:

    if (ghhkMsgFilter != NULL) {
        UnhookWindowsHook(WH_MSGFILTER, lpfnMsgFilterHookFunc);
        FreeProcInstance(lpfnMsgFilterHookFunc);
    }
    return fRet;
}



/*****************************************************************************\
* OpenFileNameHookDlgProc
*
* Hook function for the GetOpenFileName common dialog function.
* Used to make the preview window work.
*
\*****************************************************************************/

BOOL CALLBACK
OpenFileNameHookDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static CURSORINFO curiPreview;
    static BOOL fInSelMode;

    if (msg == gnMsgLBSelChString && wParam == lst1)
    {
        fInSelMode = TRUE;
        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, 0),
            (LPARAM)GetDlgItem(hwnd, IDOK));
    }
    else if (msg == gnMsgFileOK)
    {
        if (fInSelMode)
        {
            HCURSOR hcurOld;
            PCURSORINFO pcuri;

            hcurOld = curiPreview.hcur;
            strcpy(curiPreview.szFile, gszFileName);
            if (GetCursorFromFile(&curiPreview))
                pcuri = &curiPreview;
            else
                pcuri = NULL;

            if (hcurOld)
            {
                DestroyCursor(hcurOld);
            }

            /*
             * Show a preview (including animation) in the preview window,
             * or else clear the preview window (if pcuri is NULL).
             */
            SendMessage(GetDlgItem(hwnd, ID_PREVIEW), PM_NEWCURSOR,
                0, (LPARAM)pcuri);

            fInSelMode = FALSE;

            //
            // Do NOT close the dialog.
            //
            return 1;
        }
        else
        {
            //
            // OK to close the dialog.
            //
            return 0;
        }
    }
    else
    {
        switch (msg)
        {
            case WM_INITDIALOG:
                curiPreview.hcur = 0;
                fInSelMode = FALSE;
                return TRUE;

            case WM_DESTROY:
                if (curiPreview.hcur)
                {
                    DestroyCursor(curiPreview.hcur);
                    curiPreview.hcur = 0;
                }

                break;
        }
    }

    return FALSE;
}



/***************************************************************************\
* CleanUpEverything
*
* Destroy all the outstanding cursors.
*
* History:
* 12-25-91 DarrinM      Created.
\***************************************************************************/

void CleanUpEverything(void)
{
    CURSORINFO *pcuri;
    int i;

    for (pcuri = &acuri[0], i = 0; i < CCURSORS; i++, pcuri++) {
        if (pcuri->hcur != NULL)
            DestroyCursor(pcuri->hcur);
    }

    DestroyBrushes();

    DeleteObject(ghfontLabels);
}



/***************************************************************************\
* UpdateCursorList
*
* Force the Cursor ListBox to repaint and the cursor information below the
* listbox to be refreshed as well.
*
* History:
* 12-25-91 DarrinM      Created.
\***************************************************************************/

VOID
UpdateCursorList(
    VOID
    )
{
    InvalidateRect(ghwndLB, NULL, FALSE);
    SendMessage(ghwndDlg, WM_COMMAND, MAKELONG(ID_CURSORLIST, LBN_SELCHANGE),
        (LPARAM)ghwndLB);
}



/***************************************************************************\
* PreviewWndProc
*
*
* History:
* 08-07-92 DarrinM      Created.
\***************************************************************************/

LRESULT CALLBACK
PreviewWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HDC hdc;
    RECT rc;
    int ccur;
    JIF jifRate;
    HCURSOR hcur;
    PAINTSTRUCT ps;
    PPREVIEWDATA ppd;

    switch (msg) {
    case WM_CREATE:
        if (!(ppd = (PPREVIEWDATA)LocalAlloc(LPTR, sizeof(PREVIEWDATA))))
            return -1;

        SetWindowLong(hwnd, GWL_USERDATA, (LONG)ppd);

        /*
         * Create a temp DC and bitmap to be used for buffering the
         * preview rendering.
         */
        hdc = GetDC(hwnd);
        ppd->hdcMem = CreateCompatibleDC(hdc);
        ppd->hbmMem = CreateCompatibleBitmap(hdc, gcxCursor, gcyCursor);
        ppd->hbmOld = SelectObject(ppd->hdcMem, ppd->hbmMem);
        ppd->pcuri = NULL;
        ReleaseDC(hwnd, hdc);
        break;

    case WM_DESTROY:
        ppd = (PPREVIEWDATA)GetWindowLong(hwnd, GWL_USERDATA);
        SelectObject(ppd->hdcMem, ppd->hbmOld);
        DeleteObject(ppd->hbmMem);
        DeleteDC(ppd->hdcMem);
        LocalFree(ppd);
        break;

    case PM_NEWCURSOR:
        KillTimer(hwnd, ID_PREVIEWTIMER);
        ppd = (PPREVIEWDATA)GetWindowLong(hwnd, GWL_USERDATA);

        ppd->pcuri = (PCURSORINFO)lParam;

        if (ppd->pcuri)
        {
            if (ppd->pcuri->fl & CIF_ANICURSOR)
            {
                ppd->pcuri->icur = 0;
                GetCursorInfo(ppd->pcuri->hcur, NULL, 0, &jifRate, &ccur);
                SetTimer(hwnd, ID_PREVIEWTIMER, jifRate * 16, NULL);
            }
        }

        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case PM_PAUSEANIMATION:
        KillTimer(hwnd, ID_PREVIEWTIMER);
        break;

    case PM_UNPAUSEANIMATION:
        NextFrame(hwnd);
        break;

    case WM_TIMER:
        if (wParam != ID_PREVIEWTIMER)
            break;

        NextFrame(hwnd);
        break;

    case WM_PAINT:
        BeginPaint(hwnd, &ps);

        ppd = (PPREVIEWDATA)GetWindowLong(hwnd, GWL_USERDATA);

        if (ppd->pcuri && ppd->pcuri->hcur != NULL)
        {
            rc.left = rc.top = 0;
            rc.right = gcxCursor;
            rc.bottom = gcyCursor;
            FillRect(ppd->hdcMem, &rc, ghbrWindow);

            hcur = GetCursorInfo(ppd->pcuri->hcur, NULL, ppd->pcuri->icur,
                &jifRate, &ccur);
            DrawIcon(ppd->hdcMem, 0, 0, hcur);
            BitBlt(ps.hdc, 0, 0, gcxCursor, gcyCursor, ppd->hdcMem,
                0, 0, SRCCOPY);
        }
        else
        {
            FillRect(ps.hdc, &ps.rcPaint, ghbrWindow);
        }

        EndPaint(hwnd, &ps);
        break;

    case WM_ERASEBKGND:
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}



/*****************************************************************************\
* NextFrame
*
* Sets up for the next frame in the preview window.
*
* Arguments:
*   HWND hwnd - Dialog window handle.
*
\*****************************************************************************/

VOID
NextFrame(
    HWND hwnd
    )
{
    INT ccur;
    JIF jifRate;
    PPREVIEWDATA ppd;

    ppd = (PPREVIEWDATA)GetWindowLong(hwnd, GWL_USERDATA);

    //
    // Be sure there is a cursor specified.  If not, or it is
    // not an animated cursor, we are done.
    //
    if (!ppd->pcuri || !(ppd->pcuri->fl & CIF_ANICURSOR))
        return;

    if (++(ppd->pcuri->icur) >= ppd->pcuri->ccur)
        ppd->pcuri->icur = 0;

    /*
     * Find how long this frame should be displayed (i.e. get jifRate)
     */
    GetCursorInfo(ppd->pcuri->hcur, NULL, ppd->pcuri->icur, &jifRate, &ccur);
    SetTimer(hwnd, ID_PREVIEWTIMER, jifRate * 16, NULL);

    /*
     * Redraw this frame of the cursor.
     */
    InvalidateRect(hwnd, NULL, FALSE);
}


/***************************************************************************\
* ReadTag, ReadChunk, SkipChunk
*
* Some handy functions for reading RIFF files.
*
* History:
* 10-02-91 DarrinM      Created.
* 03-25-93 Jonpa        Changed to use RIFF format instead of ASDF
\***************************************************************************/
BOOL ReadTag(
    HANDLE hf,
    PRTAG ptag)
{
    DWORD cbActual;

    ptag->ckID = ptag->ckSize = 0L;

    if (!ReadFile(hf, ptag, sizeof(RTAG), &cbActual, NULL) ||
            (cbActual != sizeof(RTAG)))
        return FALSE;

    /* no need to align file pointer since RTAG is already word aligned */
    return TRUE;
}


BOOL ReadChunk(
    HANDLE hf,
    PRTAG ptag,
    PVOID pv)
{
    DWORD cbActual;

    if (!ReadFile(hf, pv, ptag->ckSize, &cbActual, NULL) ||
            (cbActual != ptag->ckSize))
        return FALSE;

    /* WORD align file pointer */
    if( ptag->ckSize & 1 )
        SetFilePointer(hf, 1, NULL, FILE_CURRENT);

    return TRUE;
}


BOOL ReadChunkN(
    HANDLE hf,
    PRTAG ptag,
    PVOID pv,
    DWORD cbMax)
{
    DWORD cbActual;
    DWORD cbRead = min( cbMax, ptag->ckSize );

    if (!ReadFile(hf, pv, ptag->ckSize, &cbActual, NULL) ||
            (cbActual != cbRead))
        return FALSE;

    /* WORD align file pointer */

    //BUGBUG - this is right isn't it?
    cbRead = ptag->ckSize - cbActual;

    if( ptag->ckSize & 1 )
        cbRead++;

    return SetFilePointer(hf, cbRead, NULL, FILE_CURRENT) != 0xFFFFFFFF;
}

DWORD Skip(
    HANDLE hf,
    DWORD cbSkip)
{
    /* Round cbSize up to nearest word boundary to maintain alignment */
    DWORD cb = (cbSkip + 1) & ~1;

    if (SetFilePointer(hf, cb, NULL, FILE_CURRENT) == 0xFFFFFFFF)
        cb = 0;

    return cb;
}



/***************************************************************************\
* HourGlass
*
* Turn hourglass cursor on or off.
*
* History:
* 07-30-92 DarrinM      Pulled from control\main\util.c
\***************************************************************************/

void HourGlass(BOOL fOn)
{
   if (!GetSystemMetrics (SM_MOUSEPRESENT))
      ShowCursor (fOn);

   SetCursor (LoadCursor (NULL, fOn ? IDC_WAIT : IDC_ARROW));
}

/************************************************************************
* MsgFilterHookFunc
*
* This is the exported message filter function that is hooked into
* the message stream for detecting the pressing of the F1 key, at
* which time it calls up the appropriate help.
*
* Arguments:
*
* History:
*
************************************************************************/

DWORD FAR PASCAL MsgFilterHookFunc(
    INT nCode,
    WPARAM wParam,
    LPMSG lpMsg)
{
    if ((nCode == MSGF_MENU || nCode == MSGF_DIALOGBOX) &&
            (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1)) {
        /*
         * Display help.
         */
        CPHelp(lpMsg->hwnd);

        /*
         * Tell Windows to swallow this message.
         */
        return 1;
    }

    return DefHookProc(nCode, wParam, (LONG)lpMsg, &ghhkMsgFilter);
}

/***************************************************************************\
* Scheme Functions
*
* History:
* 04-10-93 ErikGav Created
\***************************************************************************/

BOOL SaveSchemeAs()
{
    BOOL fSuccess=TRUE;
    LRESULT lr;

    // dialog proc returns TRUE & sets filename entered
    // to gszSchemeName if OK clicked

    if (DialogBox (ghmod, MAKEINTRESOURCE(DLG_SCHEMESAVE), ghwndDlg, (DLGPROC) SaveSchemeDlgProc))
    {
        lr = SendMessage(ghwndSchemeCB,CB_FINDSTRINGEXACT,0xFFFF,(LPARAM) gszSchemeName);

        // if not found, add it

        if (lr==CB_ERR)
        {
            lr = SendMessage(ghwndSchemeCB,CB_ADDSTRING,0,(LPARAM) gszSchemeName);
        }

        // select the name

        SendMessage(ghwndSchemeCB,CB_SETCURSEL,(WPARAM) lr,0);

        fSuccess=SaveScheme();
    }

    return fSuccess;
}

BOOL SaveScheme()
{
    const BUFFER_SIZE = CCURSORS * (FILENAME_MAX+1) + 1;
    char pszSchemeName[MAX_SCHEME_NAME_LEN+1];
    LPSTR pszBuffer;
    BOOL fSuccess=TRUE;
    int i;

    // allocate buffer for string
    pszBuffer = (LPSTR) LocalAlloc(LMEM_FIXED,BUFFER_SIZE);
    if (pszBuffer==NULL) return FALSE;

    // get current scheme name
    GetWindowText(ghwndSchemeCB,pszSchemeName,sizeof(pszSchemeName));

    // store in registry
    fSuccess = WriteProfileString(szCurrentINI,szSchemeINI,pszSchemeName);

    // concatenate the filenames into a string, write it to the registry
    pszBuffer[0]='\0';
    for (i = 0; i < CCURSORS; i++)
    {
        lstrcat(pszBuffer,acuri[i].szFile);
        lstrcat(pszBuffer,",");
    }

    pszBuffer[lstrlen(pszBuffer)-1]='\0';  // strip last comma

    fSuccess &= WriteProfileString(szSchemeINI,pszSchemeName,pszBuffer);

    LocalFree(pszBuffer);

    if (fSuccess)
    {
        EnableWindow(GetDlgItem(ghwndDlg, ID_SAVESCHEME), FALSE);
    }

    return fSuccess;
}

BOOL LoadScheme()
{
    const BUFFER_SIZE = CCURSORS * (FILENAME_MAX+1) + 1;
    char pszSchemeName[MAX_SCHEME_NAME_LEN+1];
    LPSTR pszBuffer;
    BOOL fSuccess=TRUE;
    LPSTR pszFile;
    LPSTR pszNextFile;
    BOOL fEOL=FALSE;
    LRESULT lr;
    int i=0;

    // allocate buffer for cursor paths
    pszBuffer = (LPSTR) LocalAlloc(LMEM_FIXED,BUFFER_SIZE);
    if (pszBuffer==NULL) return FALSE;

    HourGlass(TRUE);

    // get current scheme name
    GetWindowText(ghwndSchemeCB,pszSchemeName,sizeof(pszSchemeName));

    // read cursor paths from the registry
    GetProfileString(szSchemeINI,pszSchemeName,"",pszBuffer,BUFFER_SIZE);

    // parse string of format "filename1,filename2,filename3..."
    // into cursor info array

    pszFile=pszBuffer;

    do
    {
        pszNextFile=pszFile;
        while (*pszNextFile!='\0')
        {
            if (*pszNextFile==',')
                break;

            pszNextFile=CharNext(pszNextFile);
        }

        if (*pszNextFile=='\0') fEOL=TRUE;
        else *pszNextFile='\0';

        if (lstrcmp(pszFile,acuri[i].szFile))
        {
            // it's different than current, update

            lstrcpy(acuri[i].szFile,pszFile);
            fSuccess &= SchemeUpdate(i);
        }

        pszFile = pszNextFile;

        if (!fEOL) pszFile++;   // skip '\0' and move to next path

        i++;
    } while (i < CCURSORS);

    LocalFree(pszBuffer);

    // select "Arrow" in list box
    lr = SendMessage(ghwndLB, LB_GETCURSEL, 0, 0);      // avoid a "flash"
    if (lr!=0)
    {
        SendMessage(ghwndLB, LB_SETCURSEL, 0, 0);
    }

    // repaint
    UpdateCursorList();

    // disable the SAVE button
    EnableWindow(GetDlgItem(ghwndDlg, ID_SAVESCHEME), FALSE);

    HourGlass(FALSE);

    return fSuccess;
}

// helper function --
// updates the cursor at index i
// in acuri
BOOL SchemeUpdate(int i)
{
    DWORD dwDummy;
    BOOL fSuccess=TRUE;

    if (acuri[i].hcur != NULL)
    {
        DestroyCursor(acuri[i].hcur);
    }

    acuri[i].fl = CIF_MODIFIED;

    // if "Set Default"
    if (*(acuri[i].szFile) == '\0')
    {
        acuri[i].hcur = GetCursorInfo(NULL, (LPWSTR)gacd[i].idResource, 0, &dwDummy, (LPINT)&dwDummy);
    }
    else
    {
        fSuccess=TryToLoadCursor(ghwndDlg,i,&acuri[i]);
    }

    return fSuccess;
}

BOOL RemoveScheme()
{
    char pszSchemeName[MAX_SCHEME_NAME_LEN+1];
    LRESULT lr;
    char RemoveMsg[PATHMAX];
    char DialogMsg[PATHMAX];

    GetWindowText(ghwndSchemeCB,pszSchemeName,sizeof(pszSchemeName));

    if (*pszSchemeName=='\0')
    {
        return FALSE;
    }

    // put up a warning dialog

    LoadString (ghmod, IDS_REMOVESCHEME, RemoveMsg, PATHMAX);
    wsprintf(DialogMsg, RemoveMsg, (LPSTR) pszSchemeName);

//    DebugBreak();

    LoadString (ghmod, IDS_NAME, RemoveMsg, PATHMAX);

    if (MessageBox (ghwndDlg, DialogMsg, RemoveMsg, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
        return TRUE;

    // delete from registry
    WriteProfileString(szSchemeINI,pszSchemeName,"");

    // delete from list box
    lr = SendMessage(ghwndSchemeCB, CB_FINDSTRINGEXACT, 0xFFFF, (LPARAM) pszSchemeName);
    SendMessage(ghwndSchemeCB, CB_DELETESTRING, (WPARAM) lr, 0);

    // set new selection
    SendMessage(ghwndSchemeCB, CB_SETCURSEL, 0, 0);

    // Refresh everything
    return LoadScheme();
}

BOOL InitSchemeComboBox()
{
    const BUFFER_SIZE=4096;
    LPSTR pszBuffer, pszSchemeNames;
    LRESULT lr;
    BOOL fSuccess=TRUE;

    // allocate a buffer for the scheme names

    pszBuffer = (LPSTR) LocalAlloc(LMEM_FIXED,BUFFER_SIZE);
    if (pszBuffer==NULL) return FALSE;

    pszSchemeNames=pszBuffer;

    // copy the scheme names

    GetProfileString(szSchemeINI,NULL,"",pszSchemeNames,BUFFER_SIZE);

    // add each scheme name to the combo box

    while (*pszSchemeNames!='\0')
    {
        SendMessage(ghwndSchemeCB,CB_ADDSTRING,0,(LPARAM)pszSchemeNames);

        while (*pszSchemeNames!='\0')
        {
            pszSchemeNames=CharNext(pszSchemeNames);  // skip to next string
        }

        pszSchemeNames++;   // skip '\0' between strings
    }

    // determine which scheme is currently selected by the user

    GetProfileString(szCurrentINI,szSchemeINI,"",pszBuffer,BUFFER_SIZE);

    // try and find it in the combobox

    lr = SendMessage(ghwndSchemeCB,CB_FINDSTRINGEXACT,0xFFFF,(LPARAM) pszBuffer);

    // if found, select it

    if (lr!=CB_ERR)
    {
        SendMessage(ghwndSchemeCB,CB_SETCURSEL,(WPARAM) lr,0);
        fSuccess=LoadScheme();
    }
    else
    {
        // scheme was not found
        // set text to nothing
        SetWindowText(ghwndSchemeCB,"");

        // and disable the REMOVE/SAVE buttons
        EnableWindow(GetDlgItem(ghwndDlg, ID_SAVESCHEME), FALSE);
        EnableWindow(GetDlgItem(ghwndDlg, ID_REMOVESCHEME), FALSE);
    }

    LocalFree(pszBuffer);

    return fSuccess;
}

BOOL CALLBACK SaveSchemeDlgProc(HWND  hWnd, UINT message, DWORD wParam, LONG lParam)
{
    char    szSchemeName[MAX_SCHEME_NAME_LEN+1];

    switch (message)
    {
    case WM_INITDIALOG:

        HourGlass (TRUE);

        GetWindowText(ghwndSchemeCB,szSchemeName,sizeof(szSchemeName));

        SetDlgItemText (hWnd, ID_SCHEMEFILENAME,  szSchemeName);
        SendDlgItemMessage (hWnd, ID_SCHEMEFILENAME, EM_SETSEL, 0, 32767);
        SendDlgItemMessage (hWnd, ID_SCHEMEFILENAME, EM_LIMITTEXT, MAX_SCHEME_NAME_LEN, 0L);
        EnableWindow (GetDlgItem (hWnd, IDOK), szSchemeName[0] != '\0');
        HourGlass (FALSE);
        return (TRUE);

        break;

    case WM_COMMAND:

        switch (LOWORD (wParam))
        {
        case ID_SCHEMEFILENAME:
            if (HIWORD (wParam) == EN_CHANGE)
            {
                EnableWindow (GetDlgItem (hWnd, IDOK),
                    GetDlgItemText (hWnd, ID_SCHEMEFILENAME,  szSchemeName, 2));
            }
            break;

        case IDD_HELP:
            goto DoHelp;

        case IDOK:

            GetDlgItemText (hWnd, ID_SCHEMEFILENAME,  szSchemeName, MAX_SCHEME_NAME_LEN);
            StripBlanks (szSchemeName);

            if (*szSchemeName=='/0')
            {
                MessageBeep(0);
                break;
            }

            lstrcpy (gszSchemeName,  szSchemeName);

        // fall through...

        case IDCANCEL:

            EndDialog (hWnd, LOWORD (wParam) == IDOK);
            return (TRUE);
        }

    default:

        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hWnd);
            return TRUE;
        }
        else
            return FALSE;
    }
    return (FALSE);  // Didn't process a message
}

// returns Filename with a default path in system directory
// if no path is already specified
LPSTR MakeFilename(LPSTR sz)
{
    if (sz[0]=='\\' || sz[1]==':')
    {
        return sz;
    }
    else
    {
        GetSystemDirectory(gszFileName2,sizeof(gszFileName2));
        lstrcat(gszFileName2,"\\");
        lstrcat(gszFileName2,sz);

        return gszFileName2;
    }
}


/* StripBlanks()
   Strips leading and trailing blanks from a string.
   Alters the memory where the string sits.

   ErikGav- Stolen from \control\main\util.c
*/

void StripBlanks (LPSTR pszString)
{
    LPSTR pszPosn;

    /* strip leading blanks */
    pszPosn = pszString;
    while(*pszPosn == ' ')
    {
        pszPosn++;
    }

    if (pszPosn != pszString);
        strcpy (pszString, pszPosn);

    /* strip trailing blanks */
    if ((pszPosn=pszString + strlen (pszString)) != pszString)
    {
       pszPosn = CharPrev (pszString, pszPosn);
       while(*pszPosn == ' ')
           pszPosn = CharPrev (pszString, pszPosn);
       pszPosn = CharNext (pszPosn);
       *pszPosn = '\0';
    }
}



