/*

 * MIDI.C
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * The MIDI control panel applet.
 *
 * History:
 *
 *  t-mikemc    10-Apr-90 Created.
 */
/* Revision history:
   March 92 Ported to 16/32 common code by Laurie Griffiths (LaurieGr)
*/

/*-=-=-=-=- Include Files       -=-=-=-=-*/

#include        "preclude.h"
#include        <windows.h>
#include        <mmsystem.h>
#if defined(WIN32)
#include        <port1632.h>
#endif
#include "hack.h"
#include        "midimap.h"
#include        <cpl.h>
#include        "cphelp.h"
#include        "cparrow.h"
#include        "midi.h"

/*-=- poop -=-*/

#define MM_MAXRESLEN    32      // arbitrary maximum resource string length
/*-=-=-=-=- Prototypes          -=-=-=-=-*/

BOOL FAR PASCAL _loadds MainBox (HWND, UINT, WPARAM, LPARAM);

BOOL NEAR PASCAL LibMain(HINSTANCE,UINT,LPSTR);
#if defined(WIN16)
#pragma alloc_text(_INIT, LibMain)
#endif //WIN16

LRESULT FAR PASCAL _loadds CPlApplet(HWND,UINT,WPARAM,LPARAM);
#if defined(WIN16)
#pragma alloc_text(_INIT, CPlApplet)
#endif //WIN16

void NEAR PASCAL InitCPL(void);
#if defined(WIN16)
#pragma alloc_text(_INIT, InitCPL)
#endif //WIN16


/*-=-=-=-=- Global Variables    -=-=-=-=-*/

/* This many global variables is a sure sign of sickness.  LaurieGr */

HINSTANCE       hLibInst;               // Library instance handle.
HGLOBAL hKeyMap;                        // Midikeymap handle.
HFONT   hFont;                          // Dialog box font handle.
HWND    hWnd,                           // Current window handle.
        hCombo,                         // Combo box handle.
        hEdit,                          // Edit control handle.
        hArrow;                         // Arrow control handle.
RECT    rcBox;                          // Clipping/scroll rectangle.
int     rgxPos[8],                      // horizontal line positions.
        yBox,                           // rows of data y extent
        xClient,                        // Window client area x pixels.
        yClient,                        // Window client area y pixels.
        iCurPos,                        // Current position on screen.
        iVertPos,                       // Current vertical scroll position.
        iVertMax,                       // Maximum veritcal scroll position.
        nLines,                         // Number of lines of data.
        yChar,                          // Height of character in font.
        xChar,                          // Width of average character in font.
        iMap;                           // Current map type being edited.
char    szCurrent[MMAP_MAXNAME],        // Current map name.
        szCurDesc[MMAP_MAXDESC],        // Current map description.
        szCurSetup[MMAP_MAXNAME],       // Current setup
        szNone[16],                     // Generic global string.
        szMidiCtl[64],                  // Window caption for midicpl.
        aszSourceKey[32],
        aszSourceKeyName[40],
        aszPatchNumber[40],
        aszSourcePatch[32],
        aszSourcePatchName[40],
        aszSourceMnumonic[5],
        aszSourceChannel[32],
        aszActive[32],
        szMidiHlp[24];
BOOL    fModified,                      // Flag; Has the map been modified?
        fChanged,                       // Flag; Has anything ever been changed?
        fNew,                           // Flag; Is this a new map?
        fHidden;                        // Flag; Is the acitve line hidden?
BOOL    fReadOnly;
char    aszClose[16];

static  char aszCaptionFormat[24];
static  SZCODE szHelpMessage[] = "ShellHelp";
static  SZCODE aszTempPrefix[] = "mmr";         // limited to 3 characters
static  SZCODE aszFontName[] = "MS Sans Serif";
static  BOOL fAppletEntered;            // Disallow multiple applet instances
UINT near uHelpMessage;
DWORD near dwContext;

typedef struct {
        int idIcon;
        int idName;
        int idInfo;
        BOOL bEnabled;
        DWORD dwContext;
        PSTR pszHelp;
} APPLET_INFO;

#define NUM_APPLETS 1
APPLET_INFO near applets[NUM_APPLETS];


/* Move the window, if necessary, to keep it on the desktop, as far as possible */
VOID PlaceWindow(HWND hwnd)
{
    RECT rcWind;  /* window rectangle */
    HDC hdc;      /* so we can get device capabilities -i.e. screen size */
    int up;       /* amount to move window up */
    int left;     /* amount to move window left */
    int HorzRes;  /* horizontal screen resolution in pixels */
    int VertRes;  /* vertical screen resolution in lines */

     /* GetWindowRect(HWND_DESKTOP, &rcDesk) doesn't work! */
    hdc = GetDC(hwnd);
    HorzRes = GetDeviceCaps(hdc,HORZRES);
    VertRes = GetDeviceCaps(hdc,VERTRES);
    GetWindowRect(hwnd, &rcWind);
    up = rcWind.bottom - VertRes;       /* how much to move up to get onto screen... */
    if (up<0) up = 0;			/* don't bother to go down to get to the bottom */
    if (up>rcWind.top) up = rcWind.top;	/* don't ever go off the top */

    left = rcWind.right - HorzRes;       /* how much to move left to get all on screen... */
    if (left<0) left = 0;		 /* don't bother to go right */
    if (left>rcWind.left) left = rcWind.left; /* but don't ever go off the left */

    SetWindowPos( hwnd, HWND_TOP, rcWind.left-left, rcWind.top-up, 0,0,SWP_NOSIZE);

} /* PlaceWindow */


VOID NEAR PASCAL CancelToClose(HWND hDlg)
{
    if (!fReadOnly) {
        char    aszText[16];

        GetDlgItemText(hDlg, IDOK, aszText, sizeof(aszText));
        if (lstrcmp(aszText, aszClose))
            SetDlgItemText(hDlg, IDOK, aszClose);
    }
}

//      -       -       -       -       -       -       -       -       -

//      Windows entry point.

BOOL NEAR PASCAL LibMain(
        HINSTANCE       hInstance,
        UINT    uHeapSize,
        LPSTR   lpCmdLine)
{
        hLibInst = hInstance;
        return TRUE;
}


void NEAR PASCAL InitCPL(void)
{
        if ( applets[0].idIcon == 0 )
        {
                LoadString(hLibInst, IDS_CLOSE,  aszClose, sizeof(aszClose));
                applets[0].idIcon = ID_ICON;
                applets[0].idName = IDS_NAME;
                applets[0].idInfo = IDS_INFO;
                applets[0].bEnabled = TRUE;
                applets[0].dwContext = IDH_CHILD_MIDI;
                applets[0].pszHelp = szMidiHlp;
                LoadString(hLibInst, IDS_NONE, szNone, sizeof(szNone));
                LoadString(hLibInst, IDS_HELPFILE, szMidiHlp, sizeof(szMidiHlp));
                LoadString(hLibInst, IDS_SOURCEKEY, aszSourceKey, sizeof(aszSourceKey));
                LoadString(hLibInst, IDS_SOURCEKEYNAME, aszSourceKeyName, sizeof(aszSourceKeyName));
                LoadString(hLibInst, IDS_PATCHNUMBER, aszPatchNumber, sizeof(aszPatchNumber));
                LoadString(hLibInst, IDS_SOURCEPATCH, aszSourcePatch, sizeof(aszSourcePatch));
                LoadString(hLibInst, IDS_SOURCEPATCHNAME, aszSourcePatchName, sizeof(aszSourcePatchName));
                LoadString(hLibInst, IDS_SOURCEMNUMONIC, aszSourceMnumonic, sizeof(aszSourceMnumonic));
                LoadString(hLibInst, IDS_SOURCECHANNEL, aszSourceChannel, sizeof(aszSourceChannel));
                LoadString(hLibInst, IDS_ACTIVETITLE, aszActive, sizeof(aszActive));
        }
}

#ifdef STUPID
/*
 * ComboBox String Lookup, written because we were stupid and
 * let an intern design this applet.
 */

int FAR PASCAL ComboLookup(
        HWND hCombo,
        LPSTR szLookup)
{
        int iEntries;
        static char szBuf[29];
        PSTR pstrBuf;

        iEntries = (int)(LONG)SendMessage(hCombo,CB_GETCOUNT,(WPARAM)0,(LPARAM)0);

        if (iEntries > CB_ERR)
        {
                do
                {
                        iEntries--;
                        pstrBuf = szBuf;
                        if ((int)(LONG)SendMessage(hCombo,CB_GETLBTEXT,(WPARAM)iEntries,(LPARAM)(LPSTR)szBuf) == CB_ERR)
                                return CB_ERR;

                        if (lstrcmpi(szLookup,pstrBuf) == 0)
                                return iEntries;
                }
                while(iEntries);
        }
        return CB_ERR;
}
#endif

//      Given an error code, display an error message.  If the error code
//      was invalid, displays a default bogus English-language text string.

VOID FAR PASCAL VShowError(
        HWND    hwnd,           // Window to tie the message box to.
        MMAPERR mmaperr)        // Error code to display text for.
{
        char    asz[256];

        LoadString(hLibInst, IDS_MMAPERR_BASE + mmaperr, asz, sizeof(asz));
        MessageBox(hwnd, asz, NULL, MB_ICONEXCLAMATION | MB_OK);
}

//      -       -       -       -       -       -       -       -       -

//      Returns TRUE if the file exists and is correct, or was initialized
//      properly.  Returns FALSE if the user didn't want it initialized, or
//      if the initialization failed.
//
//      This function deals with English-language specific stuff.  There is
//      no good reason for this that I can see.

static  BOOL FAR PASCAL FGetMapFile(
        HWND    hwnd)
{
        char    aszBuf[256];
        DWORD   dwVersion;
        MMAPERR mmaperr;

        //
        //      An error return from "mapFileVersion" indicates a problem that
        //      I don't want to go into too specifically at this time.  The
        //      most likely event is that the file doesn't exist.  The code in
        //      in this function assumes that this is what happened.  If it
        //      was another problem, hopefully initializing the file will
        //      solve it.  This can be looked into later.
        //
        dwVersion = mapFileVersion();
        if ((HIWORD(dwVersion) == MMAPERR_SUCCESS) &&
                (LOWORD(dwVersion) == 1))
                return TRUE;    // File was fine (opened + was right version).
        //
        //      This code is executing if there was a problem opening the
        //      file.  Hopefully this problem involved non-existence.
        //
        LoadString(hLibInst, IDS_CREATE_QUESTION, aszBuf, sizeof(aszBuf));
        if (MessageBox(hwnd, aszBuf, szMidiCtl,
                MB_YESNO | MB_ICONHAND) == IDYES)
                if ((mmaperr = mapInitMapFile()) == MMAPERR_SUCCESS)
                        return TRUE;
                else
                        VShowError(hwnd, mmaperr);

        return FALSE;
}

//      -       -       -       -       -       -       -       -       -

//      Control panel entry point.

LRESULT FAR PASCAL _loadds CPlApplet(
        HWND    hCPlWnd,
        UINT    uMessage,
        WPARAM  lParam1,
        LPARAM  lParam2)
{
        LPNEWCPLINFO    lpCPlInfo;
        BOOL    fSuccess = TRUE;
        BOOL    fBackup;
        LPSTR   lpstrBakPath;
        LPSTR   lpstrCfgPath;
        int     iApplet;

        switch (uMessage) {
        case CPL_INIT :
                InitCPL();
                uHelpMessage = RegisterWindowMessage(szHelpMessage);
                hWnd = hCPlWnd;
                return (LRESULT)TRUE;
        case CPL_GETCOUNT :
                return (LRESULT)NUM_APPLETS;
        case CPL_NEWINQUIRE :
                lpCPlInfo = (LPNEWCPLINFO)lParam2;
                iApplet = (int)(LONG)lParam1;
                lpCPlInfo->hIcon = LoadIcon(hLibInst,
                                          MAKEINTRESOURCE(applets[iApplet].idIcon));
                LoadString(hLibInst, applets[iApplet].idName, lpCPlInfo->szName, sizeof(lpCPlInfo->szName));
                LoadString(hLibInst, applets[iApplet].idInfo, lpCPlInfo->szInfo, sizeof(lpCPlInfo->szInfo));
                lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
                lpCPlInfo->lData = (LONG)iApplet;
                lpCPlInfo->dwHelpContext = applets[iApplet].dwContext;
                lstrcpy(lpCPlInfo->szHelpFile, applets[iApplet].pszHelp);
                return (LRESULT)TRUE;
        case CPL_DBLCLK :
                if (fAppletEntered)
                        break;
                //
                // We can enter into lots of bizarre states:

                // We become READ-ONLY IF:
                //      We cannot lock the mapper.
                //      There is a disk or memory exception backing up the
                //              existing MIDIMAP.CFG file or the MIDIMAP.CFG
                //              is READ ONLY.
                // We DO NOT use a backup IF:
                //      The mapper is locked.
                //      No MIDIMAP.CFG file is found.
                //      There is an exception dealing with backing up the
                //              the existing MIDIMAP.CFG file.
                //

                lpstrCfgPath = GlobalLock(GlobalAlloc(GHND,MAXPATHLEN));
                if (lpstrCfgPath == NULL)
                {
                        VShowError(hCPlWnd,IDS_MMAPERR_MEMORY);
                        break;
                }

                lpstrBakPath = GlobalLock(GlobalAlloc(GHND,MAXPATHLEN));
                if (lpstrBakPath == NULL)
                {
                        VShowError(hCPlWnd,IDS_MMAPERR_MEMORY);
                        goto exitfree;
                }

                fAppletEntered = TRUE;
                if (!mapLock()) // Attempt to lock the mapper
                {
                        char szApp[32],
                             szMessage[256];

                        LoadString(hLibInst, IDS_READONLYMODE, szMessage, sizeof(szMessage));
                        LoadString(hLibInst, IDS_TITLE, szApp, sizeof(szApp));

                        MessageBox(hCPlWnd, szMessage, szApp,
                                MB_ICONINFORMATION | MB_OK);

                        fBackup = FALSE;        // No backups
                        fReadOnly = TRUE;       // Read Only Mode
                }
                else
                {
                        fBackup = TRUE;         // Try to do a backup
                        fReadOnly = FALSE;      // Not in READ ONLY mode
                }

                if (fBackup)
                {
                        char    szMapCfg[MMAP_MAXCFGNAME];

                        LoadString(hLibInst,IDS_MIDIMAPCFG,szMapCfg,MMAP_MAXCFGNAME);
                        GetSystemDirectory(lpstrCfgPath,MAXPATHLEN - sizeof(szMapCfg));
                        lstrcat(lpstrCfgPath,szMapCfg);

                        MGetTempFileName(0, aszTempPrefix,0,lpstrBakPath);

                        if (DupMapCfg(lpstrCfgPath,lpstrBakPath)) {
                                mapConnect(lpstrBakPath);

                        }
                        else
                        {
                                // No Backups.
                                fBackup = FALSE;
                                // if fReadOnly == TRUE, the exception
                                //    has forced us into READ-ONLY mode
                                // else
                                //    we didn't have a MIDIMAP.CFG file
                                //    and we'll read/write directly.
                                if (fReadOnly)
                                        mapUnlock();

                                DosDelete(lpstrBakPath);  // GetTempFileName Creates a File of size 0
                        }
                }

                if (!FGetMapFile(hCPlWnd))
                {
                        fSuccess = FALSE;
                        goto appexit;
                }
                // !!! Never unregisters the class, so this will come back
                //  FALSE on subsequent startups.  This is of course bogus.
                (VOID)RegisterArrowClass(hLibInst);
#if 0
                if ((hFont = CreateFont(8, NULL, NULL, NULL,
                        FW_NORMAL, NULL, NULL, NULL,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                        VARIABLE_PITCH | FF_DONTCARE, aszFontName)) == NULL)
                        goto appexit;
#else
                {
                        LOGFONT lf;

                        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), (LPVOID)&lf, 0);
                        if (!(hFont = CreateFontIndirect(&lf)))
                                goto appexit;
                }
#endif

                fSuccess = DialogBox(hLibInst, MAKEINTRESOURCE(ID_MAINBOX),
                        hCPlWnd, (DLGPROC)MainBox);
                UnregisterArrowClass(hLibInst);
                DeleteObject(hFont);

appexit:
                if (fBackup)
                {
                        MDOUT("Disconnecting from mapper");
                        mapDisconnect();
                        if (fSuccess)
                        {
                                MDOUT("Updating Backup");
                                UpdateMapCfg(lpstrCfgPath,lpstrBakPath);
                        }
                        mapUnlock();
                        DosDelete(lpstrBakPath);
                }
                else
                if (!fReadOnly)
                {
                        mapUnlock();
                        if (!fSuccess)
                                DosDelete(lpstrCfgPath);
                }

                fAppletEntered = FALSE;
                GlobalFree((HGLOBAL)lpstrBakPath);
exitfree:       GlobalFree((HGLOBAL)lpstrCfgPath);

                break;
        }
        return (LRESULT)0;
}

//      -       -       -       -       -       -       -       -       -

//      This will return either "MMAPERR_SUCCESS" if everything is fine,
//      or will return "MMAPERR_INVALIDPORT" if the setup references a
//      bogus port.  These are both expected conditions.
//
//      Unexpected conditions can also happen, which are returned.

static  MMAPERR NEAR PASCAL MmaperrInvalidPortCheck(
        LPSTR   lszName)                // Name of current setup.
{
        LPMIDIMAP lpMap;
        MMAPERR mmaperr;
        DWORD   dwSize;
        HGLOBAL hMidiMap;

        if ((dwSize = mapGetSize(MMAP_SETUP, lszName)) < MMAPERR_MAXERROR)
                return (MMAPERR)dwSize;
        if ((hMidiMap = GlobalAlloc(GHND, dwSize)) == NULL)
                return MMAPERR_MEMORY;
        lpMap = (LPMIDIMAP)GlobalLock(hMidiMap);
        mmaperr = mapRead(MMAP_SETUP, lszName, (LPVOID)lpMap);
        GlobalUnlock(hMidiMap);
        GlobalFree(hMidiMap);
        return mmaperr;
}

//      -       -       -       -       -       -       -       -       -

//      Converts a "MMAP_" into a "IDS_", i.e. converts "MMAP_SETUP" into
//      "IDS_SETUP", which can be used to get the word "Setup" out of the
//      "midi.rc" file.

static  int PASCAL IdsGetMapNameId( int iMap)
{
        if (iMap == MMAP_SETUP)
                return IDS_SETUP;
        if (iMap == MMAP_PATCH)
                return IDS_PATCH;
        return IDS_KEY;
}

//      -       -       -       -       -       -       -       -       -

/*
 * DELETEMAP
 *
 * This function returns TRUE if it is OK to delete whatever map
 * exists in szCurrent.
 */

static  BOOL NEAR PASCAL FConfirmDeleteMap(
        HWND    hwnd)
{
        char    szSetup[MMAP_MAXNAME],
                szSrc[MM_MAXRESLEN],
                szUsedBy[MM_MAXRESLEN],
                szDel[50],
                szCap[50],
                szRsrc[256],
                szBuf[256];
        DWORD   dwRet;
        UINT    uSrcID;
        UINT    uUsage;
        MMAPERR mmaperr;


        uSrcID = IdsGetMapNameId(iMap);
        LoadString(hLibInst, uSrcID, szSrc, sizeof(szSrc));
        LoadString(hLibInst, IDS_DELETE, szDel, sizeof(szDel));
        wsprintf(szCap, szDel, (LPSTR)szSrc);
//      AnsiUpperBuff(szSrc, 1);        // Upper-case first character.
        switch (iMap) {
        case MMAP_SETUP:
                if ((mmaperr = mapGetCurrentSetup(szSetup,
                        MMAP_MAXNAME)) != MMAPERR_SUCCESS) {
                        VShowError(hwnd, mmaperr);
                        return FALSE;
                }
                if (lstrcmpi(szSetup, szCurrent))
                        break;
                LoadString(hLibInst, IDS_NODELISCURRENT, szRsrc, sizeof(szRsrc));
                wsprintf(szBuf, szRsrc, (LPSTR)szCurrent);
                MessageBox(hwnd, szBuf, szCap,
                        MB_ICONINFORMATION | MB_OK);
                return FALSE;
        case MMAP_PATCH:
        case MMAP_KEY:
                dwRet = mapGetUsageCount(iMap, szCurrent);
                if (LOWORD(dwRet) != MMAPERR_SUCCESS) {
                        VShowError(hwnd, LOWORD(dwRet));
                        return FALSE;
                }
                if (!(uUsage = HIWORD(dwRet)))
                        break;
                uSrcID--;               // Patch->Setup, Key->Patch
                if (uUsage > 1)         // Make a singular a plural.  This
                        uSrcID += 3;    //  "3" is commented in "midi.h".
                LoadString(hLibInst, uSrcID, szUsedBy, sizeof(szUsedBy));
//              AnsiLowerBuff(szUsedBy, 1);     // Lower-case first character.
                LoadString(hLibInst, IDS_NODELISREFERENCED, szRsrc, sizeof(szRsrc));
                wsprintf(szBuf, szRsrc, (LPSTR)szSrc, (LPSTR)szCurrent,
                        uUsage, (LPSTR)szUsedBy);
                MessageBox (hwnd, szBuf, szCap,
                        MB_ICONINFORMATION | MB_OK);
                return FALSE;
        }
        LoadString(hLibInst, IDS_VERIFYDELETE, szRsrc, sizeof(szRsrc));
        wsprintf(szBuf, szRsrc, (LPSTR)szSrc, (LPSTR)szCurrent);
        return (IDYES == MessageBox (hwnd, szBuf, szCap,
                MB_ICONEXCLAMATION | MB_YESNO));
}

static VOID NEAR PASCAL VFreeItemData(
        HWND    hdlg,
        int     idCtrl)
{
        UINT    uCount;

        uCount = (UINT)SendDlgItemMessage(hdlg, idCtrl, CB_GETCOUNT, (WPARAM)NULL, (LPARAM)0);
        for (; uCount--; ) {
                HGLOBAL hDescription;

                if ((hDescription = (HGLOBAL)(DWORD)SendDlgItemMessage(hdlg, idCtrl,
                        CB_GETITEMDATA, (WPARAM)uCount, (LPARAM)0)) != NULL)
                        GlobalFree(hDescription);
        }
}

static  VOID NEAR PASCAL GetMBData(
        UINT    uFlag,
        LPMBDATA        lpmbData)
{
        switch (uFlag) {
        case MMAP_SETUP:
                lpmbData->lpfnBox = (DLGPROC)SetupBox;
                lpmbData->idBox = DLG_SETUPEDIT;
                break;
        case MMAP_PATCH:
                lpmbData->lpfnBox = (DLGPROC)PatchBox;
                lpmbData->idBox = DLG_PATCHEDIT;
                break;
        case MMAP_KEY:
                lpmbData->lpfnBox = (DLGPROC)KeyBox;
                lpmbData->idBox = DLG_KEYEDIT;
                break;
        default:
                lpmbData->lpfnBox = 0L;
                lpmbData->idBox = 0;
                break;
        }
} /* GetMBData */

static  BOOL PASCAL FEditMap(
        HWND    hwnd)
{
        HWND    hTmpWnd;
        MBDATA  mbData;
        DWORD   dwRet;
        int     iRet;
        BOOL    fInSetup;
        char    szSetup[MMAP_MAXNAME];
        MMAPERR mmaperr;

        if ((mmaperr = mapGetCurrentSetup(szSetup,
                MMAP_MAXNAME)) != MMAPERR_SUCCESS) {
                VShowError(hwnd, mmaperr);
                return FALSE;
        }
        switch (iMap) {
        case MMAP_SETUP :
                fInSetup = (BOOL)!lstrcmpi(szCurrent, szSetup);
                break;
        case MMAP_PATCH :
                dwRet = mapPatchMapInSetup(szCurrent, szSetup);
                if (LOWORD(dwRet) != MMAPERR_SUCCESS)
                        return FALSE;
                fInSetup = (BOOL)HIWORD(dwRet);
                break;
        case MMAP_KEY :
                dwRet = mapKeyMapInSetup (szCurrent, szSetup);
                if (LOWORD(dwRet) != MMAPERR_SUCCESS)
                        return FALSE;
                fInSetup = (BOOL)HIWORD(dwRet);
                break;
        }
        hTmpWnd = hWnd;         // "hWnd", not "hwnd".
        GetMBData(iMap, &mbData);
        iRet = DialogBox(hLibInst, MAKEINTRESOURCE(mbData.idBox),
                hWnd, mbData.lpfnBox);
        hWnd = hTmpWnd;
        return iRet;
} /* FEditMap */

static  VOID NEAR PASCAL EnableMain(
        BOOL    fEnable)
{
        char    aszNoEntries[48];

        EnableWindow(hCombo, fEnable);
        if (fReadOnly)
        {
                EnableWindow(GetDlgItem (hWnd, ID_MAINDELETE), FALSE);
                EnableWindow(GetDlgItem (hWnd,ID_MAINNEW),FALSE);
        }
        else
                EnableWindow(GetDlgItem (hWnd, ID_MAINDELETE), fEnable);
        EnableWindow(GetDlgItem (hWnd, ID_MAINEDIT), fEnable);
        EnableWindow(GetDlgItem (hWnd, ID_MAINDESC), fEnable);
        EnableWindow(GetDlgItem (hWnd, ID_MAINNAME), fEnable);
        if (!fEnable) {
                LoadString(hLibInst, IDS_NOENTRIES, aszNoEntries, sizeof(aszNoEntries));
                SetDlgItemText(hWnd, ID_MAINDESC, aszNoEntries);
        }

} /* EnableMain */

static VOID NEAR PASCAL ShowMaps (int nMap)
{
        if (iMap == nMap)
                return;
        iMap = nMap;
        SendMessage(hWnd, WM_MY_INITDIALOG, (WPARAM)NULL, (LPARAM)0);
} /* ShowMaps */

BOOL FAR PASCAL _loadds MainBox(
        HWND    hdlg,
        UINT    uMessage,
        WPARAM  wParam,
        LPARAM  lParam)
{
        static BOOL     fChange,                // has edittext changed?
                        fPatchEnum,             // have patches been enum'd?
                        fKeyEnum,               // have keys been enum'd?
                        fEnabled;
        static UINT     uDeleted;               // which maps types deleted
        static char     aszInitialSetup[MMAP_MAXNAME]; // initial setup name
        MMAPERR mmaperr;
        HGLOBAL hDesc;
        LPSTR   lpDesc;
        UINT    uIdx;
        int     idCurCombo;
        BOOL    fEnum;
//      char    szBuf[128];
        char    szTmpDesc[MMAP_MAXDESC];

        switch (uMessage) {
        case WM_INITDIALOG:
                // get the current setup name and store in
                // static aszInitialSetup
                fChanged = FALSE;
                if ((mmaperr = mapGetCurrentSetup(aszInitialSetup,
                        MMAP_MAXNAME)) != MMAPERR_SUCCESS) {
exit00:                 VShowError(hdlg, mmaperr);
/*exit01:*/             EndDialog (hdlg, FALSE);
                        return TRUE;
                }
                lstrcpy(szCurSetup, aszInitialSetup);
                // Load caption string and set the window text
                LoadString(hLibInst, IDS_TITLE, szMidiCtl, sizeof(szMidiCtl));
                hWnd = hdlg;
                SetWindowText(hdlg, szMidiCtl);
/*!!
                // check for invalid devices
                if ((mmaperr = MmaperrInvalidPortCheck(
                        aszInitialSetup)) == MMAPERR_INVALIDPORT) {
                        if (!InvalidPortMsgBox(hdlg))
                                goto exit01;
                }
                else if (mmaperr != MMAPERR_SUCCESS)
                        goto exit00;
!!*/
                // hide the patch and key comboboxes
                ShowWindow(GetDlgItem(hdlg, ID_MAINPATCHCOMBO), SW_HIDE);
                ShowWindow(GetDlgItem(hdlg, ID_MAINKEYCOMBO), SW_HIDE);
                // enumerate setups into setup combo box
                mmaperr = mapEnumerate( MMAP_SETUP
                                      , EnumFunc
                                      , MMENUM_INTOCOMBO
                                      , GetDlgItem( hdlg, ID_MAINSETUPCOMBO)
                                      , NULL
                                      );
                if ( mmaperr != MMAPERR_SUCCESS )
                        goto exit00;
                // set up the auto-radiobuttons
                CheckRadioButton(hdlg, ID_MAINFIRSTRADIO,
                        ID_MAINLASTRADIO, ID_MAINSETUP);
                // intialize some variables
                *szCurrent = 0;
                hCombo = NULL;
                iMap = MMAP_SETUP;
                fEnabled = FALSE;       // Fix for bug #2039.  13-Feb-90, BLM.
                                        //  It used to set the variable TRUE.
                fChange = FALSE;
                fPatchEnum = FALSE;
                fKeyEnum = FALSE;
                uDeleted = 0;
                // fall through
        case WM_MY_INITDIALOG:
                fEnum = FALSE;
                switch (iMap) {
                case MMAP_SETUP:
                        idCurCombo = ID_MAINSETUPCOMBO;
                        uIdx = ComboLookup(GetDlgItem(hdlg,idCurCombo),(LPSTR)szCurSetup);//-jyg
                        break;
                case MMAP_PATCH:
                        idCurCombo = ID_MAINPATCHCOMBO;
                        if (!fPatchEnum) {
                                fEnum = TRUE;
                                fPatchEnum = TRUE;
                        }
                        uIdx = 0;
                        break;
                case MMAP_KEY:
                        idCurCombo = ID_MAINKEYCOMBO;
                        if (!fKeyEnum) {
                                fEnum = TRUE;
                                fKeyEnum = TRUE;
                        }
                        uIdx = 0;
                        break;
                default:uIdx = 0;
                        idCurCombo = 0; /* kill compiler warning about use before set */
                }
                // hide the old combobox, if any
                if (hCombo != NULL)
                        ShowWindow (hCombo, SW_HIDE);
                // get the new combobox handle
                hCombo = GetDlgItem (hdlg, idCurCombo);
                // show the new combobox
                ShowWindow(hCombo, SW_SHOW);
                // if not done already, enumerate maps into the box
                if (fEnum)
                {       mmaperr = mapEnumerate(iMap, EnumFunc, MMENUM_INTOCOMBO, hCombo, NULL);
                        if (mmaperr != MMAPERR_SUCCESS)
                                goto exit00;
                }
                // set the current selection
                uIdx = (UINT)SendMessage(hCombo, CB_SETCURSEL, (WPARAM)uIdx, (LPARAM)0);
                // if no maps of that type, disable all necessary controls
                if (uIdx == CB_ERR) {
                        *szCurrent = 0;
                        EnableMain(fEnabled = FALSE);
                        break;
                }
                // if we were disabled, enable us
                if (!fEnabled)
                        EnableMain(fEnabled = TRUE);
                // fill the edit control with description
#if defined(WIN16)
                SendMessage(hdlg, WM_COMMAND, (WPARAM)ID_MAINCOMBO,
                        MAKELPARAM(hCombo, CBN_SELCHANGE));
#else
                SendMessage( hdlg
                           , WM_COMMAND
                           , (WPARAM)MAKELONG(ID_MAINCOMBO, CBN_SELCHANGE)
                           , (LPARAM)hCombo
                           );

#endif  // WIN16
                break;
        case WM_COMMAND:
            {   WORD wNotifCode;
#if defined(WIN16)
                wNotifCode = HIWORD(lParam);
#else
                wNotifCode = HIWORD(wParam);
#endif //WIN16

                switch (LOWORD(wParam)) {

                case  IDH_CHILD_MIDI:
                        goto DoHelp;

                case IDOK:
                        if (fReadOnly || !fChanged)
                        {
                                PostMessage(hdlg,WM_COMMAND,(WPARAM)IDCANCEL,(LPARAM)0);
                                break;
                        }
                        // check for invalid ports
                        if ((mmaperr = MmaperrInvalidPortCheck(
                                szCurSetup)) == MMAPERR_INVALIDPORT) {
                                if (!InvalidPortMsgBox(hdlg))
                                        break;
                        } else if (mmaperr != MMAPERR_SUCCESS)
                                goto exit00;
                        if (lstrcmpi(szCurSetup, aszInitialSetup))
                                {
                                        mmaperr = mapSetCurrentSetup(
                                                szCurSetup);
                                        if (mmaperr !=
                                                MMAPERR_SUCCESS)
                                                goto exit00;
                                }


                        // this is where any deleted maps actually get the axe
                        //
                        //      I'm going to leave this as is until I figure
                        //      out how to deal with any errors that happen.
                        //      brucemo
                        //
#if 0
                        if (uDeleted & MMAP_SETUP)
                                mapEnumerate( iMap = MMAP_SETUP
                                            , EnumFunc
                                            , MMENUM_DELETE
                                            , GetDlgItem(hdlg,ID_MAINSETUPCOMBO)
                                            , NULL
                                            );
                        if (uDeleted & MMAP_PATCH)
                                mapEnumerate( iMap = MMAP_PATCH
                                            , EnumFunc
                                            , MMENUM_DELETE
                                            , GetDlgItem(hdlg,ID_MAINPATCHCOMBO)
                                            , NULL
                                            );
                        if (uDeleted & MMAP_KEY)
                                mapEnumerate( iMap = MMAP_KEY
                                            , EnumFunc,
                                            , MMENUM_DELETE
                                            , GetDlgItem (hdlg,ID_MAINKEYCOMBO)
                                            , NULL
                                            );
#endif

                case IDCANCEL:
                        // clean up and go home
                        VFreeItemData(hdlg, ID_MAINSETUPCOMBO);
                        if (fPatchEnum)
                                VFreeItemData(hdlg, ID_MAINPATCHCOMBO);
                        if (fKeyEnum)
                                VFreeItemData(hdlg, ID_MAINKEYCOMBO);
                        if (LOWORD(wParam) == IDOK)                    // eh?
                                EndDialog(hdlg,TRUE);
                        else
                                EndDialog(hdlg,FALSE);
                        break;
                case ID_MAINDELETE:
                        if (!FConfirmDeleteMap(hdlg))
                                break;
                        CancelToClose(hdlg);
                        // set a bit in the deleted word
                        uDeleted |= iMap;
                        // get the current selections index
                        uIdx = (UINT)SendMessage(hCombo,
                                CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                        // get the handle to selections description
                        if ((hDesc = (HGLOBAL)(DWORD)SendMessage(hCombo,
                                CB_GETITEMDATA, (WPARAM)uIdx, (LPARAM)0)) != NULL)
                                GlobalFree(hDesc);
                        // delete the entry from the combobox
                        SendMessage(hCombo, CB_DELETESTRING, (WPARAM)uIdx, (LPARAM)0);

                        // -jyg-

                        mapEnumerate(iMap, EnumFunc, MMENUM_DELETE, hCombo, NULL);

                        // reset to initial setup or first entry
                        if (iMap == MMAP_SETUP)
                        {
                                uIdx = ComboLookup(hCombo,(LPSTR)aszInitialSetup); //-jyg
                                lstrcpy(szCurSetup,aszInitialSetup); // reset current string to initial setup
                        }
                        else
                                uIdx = 0;

                        uIdx = (UINT)SendMessage(hCombo, CB_SETCURSEL, (WPARAM)uIdx, (LPARAM)0);
                        // if deleted last one then disable window,
                        // otherwise update the edit control
                        if (uIdx == CB_ERR) {
                                EnableMain(fEnabled = FALSE);
                                SendMessage(hdlg, DM_SETDEFID, (WPARAM)ID_MAINNEW, (LPARAM)0);
                                SetFocus(GetDlgItem(hdlg, ID_MAINNEW));

                        } else
#if defined(WIN16)
                        SendMessage(hdlg, WM_COMMAND,
                                (WPARAM)ID_MAINCOMBO, MAKELPARAM(hCombo,
                                CBN_SELCHANGE));
#else
                        SendMessage( hdlg
                                   , WM_COMMAND
                                   , (WPARAM)MAKELONG(ID_MAINCOMBO, CBN_SELCHANGE)
                                   , (LPARAM)hCombo
                                   );
#endif //WIN16
                        break;
                case ID_MAINNEW:
                        // if they don't specify a new map get outta here
                        fNew = TRUE;
                        if (!DialogBox(hLibInst, MAKEINTRESOURCE(ID_PROPBOX),
                                hdlg, (DLGPROC)PropBox)) {
                                fNew = FALSE;
                                break;
                        }
                        // if they don't want to save new map, restore
                        // name and description and get outta here
                        if (!FEditMap(hdlg)) {
                                fNew = FALSE;
                                GetWindowText(hCombo, (LPSTR)szCurrent,
                                        MMAP_MAXNAME);
                                GetDlgItemText(hdlg, ID_MAINDESC, szCurDesc,
                                        MMAP_MAXDESC);
                                break;
                        }
                        uIdx = ComboLookup(hCombo,(LPSTR)szCurrent);//-jyg
                        if (uIdx != CB_ERR) {
                                char    aszName[MMAP_MAXNAME];
                                SendMessage(hCombo, CB_GETLBTEXT, (WPARAM)uIdx, (LPARAM)(LPSTR)aszName);
                                if (!lstrcmpi(aszName, szCurrent)) {
                                        hDesc = (HGLOBAL)(DWORD)SendMessage(hCombo, CB_GETITEMDATA, (WPARAM)uIdx, (LPARAM)0);
                                        if (hDesc != NULL)
                                                GlobalFree(hDesc);
                                        //break;
                                } else
                                        uIdx = (UINT)CB_ERR;
                        }
                        if (uIdx == CB_ERR)
                                uIdx = (UINT)SendMessage(hCombo, CB_ADDSTRING,
                                        (WPARAM)0, (LPARAM)(LPCSTR)szCurrent);
                        // make the new map the current one
                        SendMessage(hCombo, CB_SETCURSEL, (WPARAM)uIdx, (LPARAM)0);
                        if (IsDlgButtonChecked(hdlg, ID_MAINSETUP))
                                lstrcpy(szCurSetup, szCurrent);
                        // allocate buffer for map description data
                        if ((hDesc = GlobalAlloc(GHND, (DWORD)(sizeof(char) *
                                (lstrlen (szCurDesc) + 1)))) != NULL) {
                                lpDesc = (LPSTR)GlobalLock(hDesc);
                                lstrcpy(lpDesc, szCurDesc);
                                GlobalUnlock(hDesc);
                        }
                        SetDlgItemText (hdlg, ID_MAINDESC, szCurDesc);
                        // put the handle in the entrys item data
                        SendMessage(hCombo, CB_SETITEMDATA,
                                (WPARAM)uIdx, (LPARAM)(DWORD)(UINT)hDesc);
                        // enable windows if it is the first map of type
                        if (!IsWindowEnabled(hCombo))
                                EnableMain(TRUE);
                        CancelToClose(hdlg);
                        break;
                case ID_MAINEDIT:
                        if (!FEditMap(hdlg))
                                break;
                        CancelToClose(hdlg);
                        if (iMap != MMAP_SETUP)
                                break;
                        // reset to current setup
                        uIdx = ComboLookup(hCombo,(LPSTR)szCurSetup); //-jyg
                        SendMessage(hCombo, CB_SETCURSEL, (WPARAM)uIdx, (LPARAM)0);

#if defined(WIN16)
                        SendMessage(hdlg, WM_COMMAND, (WPARAM)ID_MAINCOMBO,
                                MAKELPARAM(hCombo, CBN_SELCHANGE));
#else
                        SendMessage( hdlg
                                   , WM_COMMAND
                                   , (WPARAM)MAKELONG(ID_MAINCOMBO, CBN_SELCHANGE)
                                   , (LPARAM)hCombo
                                   );
#endif //WIN16
                        break;
                case ID_MAINSETUP:
                        ShowMaps(MMAP_SETUP);
                        break;
                case ID_MAINPATCH:
                        ShowMaps(MMAP_PATCH);
                        break;
                case ID_MAINKEY:
                        ShowMaps(MMAP_KEY);
                        break;
                case ID_MAINSETUPCOMBO:
                case ID_MAINPATCHCOMBO:
                case ID_MAINKEYCOMBO:
                case ID_MAINCOMBO:
                        if (wNotifCode == CBN_SELENDOK) {
                                CancelToClose(hdlg);
                                fChanged = TRUE;
                        }
                        if (wNotifCode != CBN_SELCHANGE)
                                return FALSE;
                        uIdx = (UINT)SendMessage(hCombo, CB_GETCURSEL,
                                (WPARAM)0, (LPARAM)0);
                        if (LOWORD(wParam) == ID_MAINSETUPCOMBO)
                                SendMessage(hCombo, CB_GETLBTEXT, (WPARAM)uIdx,
                                        (LPARAM) (LPSTR) szCurSetup);
                        hDesc = (HGLOBAL)(DWORD)SendMessage(hCombo, CB_GETITEMDATA,
                                (WPARAM)uIdx, (LPARAM)0);
                        lpDesc = GlobalLock(hDesc);
                        lstrcpy(szCurDesc, lpDesc);
                        GlobalUnlock(hDesc);
                        SetDlgItemText (hdlg, ID_MAINDESC, szCurDesc);
                        SendMessage(hCombo, CB_GETLBTEXT, (WPARAM)uIdx,
                                (LPARAM)(LPSTR)szCurrent);
                        break;
                case ID_MAINDESC:
                        // NOTE:  this is not implemented.
                        if (wNotifCode == EN_CHANGE)
                                fChange = TRUE;
                        else if ((wNotifCode == EN_KILLFOCUS) && fChange) {
                                GetDlgItemText (hdlg, ID_MAINDESC, szTmpDesc,
                                        MMAP_MAXDESC);
                                if (!lstrcmpi(szCurDesc, szTmpDesc)) {
                                        // description change logic here.
                                        // There's no API to changedesc!!
                                }
                        }
                        break;
                default:
                        return FALSE;
                }
            }  /* end of WM_COMMAND */
                break;
        default:
                if (uMessage == uHelpMessage) {
DoHelp:
                        WinHelp(hWnd, szMidiHlp, HELP_CONTEXT,
                                                        IDH_CHILD_MIDI);
                        return TRUE;
                }
                else
                        return FALSE;
        break;
        }
        return TRUE;
} /* MainBox */

BOOL    FAR PASCAL InvalidPortMsgBox (
        HWND    hwnd)
{
        int     iRet;
        char    szBuf[256];

        LoadString(hLibInst, IDS_INVALIDPORT, szBuf, sizeof(szBuf));
        iRet = MessageBox (hwnd, szBuf, szMidiCtl,
                MB_ICONSTOP | MB_YESNO);
        return iRet == IDYES;
} /* InvalidPortMsgBox */

VOID FAR PASCAL Modify(
        BOOL    fSet)
{
        fModified = fSet;
        if (fModified)
                fChanged = TRUE;
} /* Modify */

/*
 * ENUMFUNC
 *
 * Enumerate setup, patchmap or keymap names.
 */
BOOL FAR PASCAL _loadds EnumFunc(
        LPSTR   lpName,
        LPSTR   lpDesc,
        UINT    uCase,
        HWND    hCombo,
        LPSTR   unused
        )
{
        HGLOBAL hMem;
        LPSTR   lpStr;
        UINT    uIdx;
        MMAPERR mmaperr;

        // see if we're dealing with 'delete' enumeration.
        if (uCase == MMENUM_DELETE) {
                uIdx = ComboLookup(hCombo, (LPSTR)lpName);//-jyg

                if (uIdx != CB_ERR)
                        return TRUE;
                mmaperr = mapDelete(iMap, lpName);
                if (mmaperr == MMAPERR_SUCCESS)
                        return TRUE;
                VShowError(hWnd, mmaperr);
                return FALSE;
        }
        uIdx = (UINT)SendMessage(hCombo, CB_ADDSTRING, (WPARAM)NULL, (LPARAM)lpName);
        // see if we're dealing with enumeration from the main dialog
        if (uCase == MMENUM_INTOCOMBO) {
                hMem = GlobalAlloc(GHND, (DWORD)(lstrlen(lpDesc) + 1));
                if (hMem != NULL) {
                        lpStr = (LPSTR)GlobalLock(hMem);
                        lstrcpy(lpStr, lpDesc);
                        GlobalUnlock(hMem);
                }
                SendMessage( hCombo,
                        CB_SETITEMDATA, (WPARAM)uIdx, (LPARAM)(DWORD)(UINT)hMem);
        }
        return TRUE;
 } /* EnumFunc */

#if 0
/*
 * SETWINDOWCAPTION
 *
 * Set the caption of a 'window', such as 'MIDI Setup: "foo"', even though
 * these are actually dialog boxes.
 */
VOID FAR PASCAL SetWindowCaption(
        VOID)
{
        char    szCaption[80],
                szName[MM_MAXRESLEN];

        LoadString(hLibInst, IdsGetMapNameId(iMap), szName, sizeof(szName));
        wsprintf(szCaption, aszCaptionFormat,
                (LPSTR)szName, (LPSTR)szCurrent);
        SetWindowText(hWnd, szCaption);
} /* SetWindowCaption */
#endif //0

int FAR PASCAL QuerySave (VOID)
{
        char    szBuf[256];
        char    aszSave[64];
        char    aszFormat[128];
        char    szFunc[MM_MAXRESLEN];

        LoadString(hLibInst, IdsGetMapNameId(iMap), szFunc, sizeof(szFunc));
        if (fNew) {
//              AnsiUpperBuff(szFunc, 1);       // Upper-case first character.
                LoadString(hLibInst, IDS_NEW_QUESTION, aszFormat, sizeof(aszFormat));
                wsprintf (szBuf, aszFormat, (LPSTR)szCurrent, (LPSTR)szFunc);
        }
    else {
                LoadString(hLibInst, IDS_CHANGE_QUESTION, aszFormat, sizeof(aszFormat));
                wsprintf (szBuf, aszFormat, (LPSTR)szFunc, (LPSTR)szCurrent);
    }
        LoadString(hLibInst, IDS_SAVE_CHANGES, aszSave, sizeof(aszSave));
        return MessageBox(hWnd, szBuf, aszSave, MB_YESNOCANCEL | MB_ICONEXCLAMATION);
} /* QuerySave */
