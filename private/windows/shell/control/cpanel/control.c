/** FILE: control.c ******** Module Header ********************************
 *
 *  Control Panel main window functions and procedures.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  21 Sept 1993  -by-  Steve Cathcart   [stevecat]
 *        Added Caching of Control Panel modules
 *
 *
 *  Copyright (C) 1990-1993 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                           Include files
//==========================================================================
// Windows SDK
#include <windows.h>
#include <shellapi.h>

// Application specific
#include "cpl.h"
#include "lb.h"
#include "cpl_defs.h"
#include "uniconv.h"

//==========================================================================
//                          Local Definitions
//==========================================================================
#define ItemHeight (iIconH + ifontH + (ifontH>>1))

#define MAX_APPLET_NAME_LEN 32

//  Private Control Panel message.  This works exactly the same way as the
//  WM_CPL_LAUNCH message except the wParam is ProcessId instead of an HWND.

#define WM_CPL_LAUNCHEX   (WM_CPL_LAUNCH-1)

typedef INT (APIENTRY *SHELL_ABOUT)(HWND hWnd, LPTSTR szApp,
                                    LPTSTR szOtherStuff, HICON hIcon);

//==========================================================================
//                            External Declarations
//==========================================================================
// External functions

extern BOOL  bAppletActive;
extern BOOL  bRebuildCache;

//==========================================================================
//                            Local Data Declarations
//==========================================================================
TCHAR    aszControlIni[] = TEXT("control.ini");
TCHAR    aszControlIniPath[MAX_PATH];

TCHAR    szMAINCPL[]        = TEXT("MAIN.CPL");
TCHAR    szClass[]          = TEXT("CtlPanelClass");    // some apps depend on this!
TCHAR    szMMCPL[]          = TEXT("MMCPL");
TCHAR    szNULL[]           = TEXT("");
TCHAR    szCPL[]            = TEXT("*.CPL");
TCHAR    szDots[]           = TEXT("...");
TCHAR    szStatic[]         = TEXT("static");
TCHAR    szHelpFile[]       = TEXT("control.hlp");
TCHAR    szText[]           = TEXT("Text");
TCHAR    cBackslash         = TEXT('\\');
TCHAR    szlistbox[]        = TEXT("lb");
TCHAR    szMaxWidth[]       = TEXT("MaxWidth");

TCHAR    szErrorText[1024];                 //  Used in ASSERT and Debug macros

BYTE     szCPlApplet[]      = "CPlApplet";  // procedure name

LPTSTR   pStrings[NUMCPLSTRINGS];

TCHAR    szUsername[MAX_PATH] = TEXT("GetUserName failed::");

HINSTANCE  hCPlInst;                // MMCPL instance handle
HWND       hCPlLB, hCPlTB;          // list box and text box handles
HFONT      hCPlFont;                // handle to MMCPl font for applet names
HBRUSH     hCPlBk;                  // handle to MMCPl background brush
PCPLMODULE CPlMods[CPL_MAXMODS];    // array of ptrs to CPLMODULE struct
HWND       hCPlWnd = 0;             // MMCPL main window handle
RECT       CPlRect;                 // last valid window x,y,w,h
WORD       numApps = 0;             // number of lData's in lCPlApps[]
int        ifontH;                  // height of font
int        iIconX;                  // X offset to draw icons
int        iIconY;                  // Y offset to draw icons
int        iIconW;                  // icon width in pixels
int        iIconH;                  // icon height " "
int        iWidth = 62;             // width of the owner-draw columns
TCHAR     *szWndDim[4] = { TEXT("X"), TEXT("Y"), TEXT("W"), TEXT("H") };
TCHAR      szNumApps[]  = TEXT("NumApps");
UINT       wHelpMessage;            // stuff for help
WORD       wMenuID = 0;
DWORD      dwMenuBits = 0L;
HHOOK      hhkMsgFilter = NULL;
HWND       hSetup = 0;              // means we are running under setup
BOOL       bSetup = FALSE;          // means we were run by Setup


//==========================================================================
//                            Local Function Prototypes
//==========================================================================
LRESULT APIENTRY fnText (HWND hWnd, UINT message, WPARAM wParam, LONG lParam);
LRESULT APIENTRY CPlWndProc (HWND hWnd, UINT message, WPARAM wParam, LONG lParam);


BOOL AlreadyLoaded    (HANDLE, int);
void AskApps          (PCPLMODULE, int);
int  ChngSelApp       (int, TCHAR);
void CPHelp           (HWND, LPTSTR, DWORD);
BOOL CreatChildWindows (void);
int  FindApplet       (LPTSTR);
BOOL GetCPLStrings    (void);
void GetCPlWndPos     (int *);
HWND GetRealParent    (HWND);
BOOL LoadApplets      (void);
void MakeWinPath      (LPTSTR, LPTSTR);
int  MessageFilter    (int, DWORD, LPMSG);
BOOL RunCommandLineApplet (int, LPTSTR, LPTSTR, HANDLE);
PCPLAPPLET SendAppMsg (int, DWORD);
void SetAppItems      (PCPLMODULE, LPCPLINFO, LPNEWCPLINFO, PCPLAPPLET, int);
void SetCPlWndPos     (HWND);
void ConvertCplInfoA  (LPNEWCPLINFO);

//==========================================================================
//                                Functions
//==========================================================================

void ConvertCplInfoA (LPNEWCPLINFO lpCPlInfoW)
{
   NEWCPLINFOA   CplInfoA;

   memcpy ((LPBYTE) &CplInfoA, lpCPlInfoW, sizeof(NEWCPLINFOA));
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, CplInfoA.szName, 32,
                        (LPWSTR) lpCPlInfoW->szName, 32);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, CplInfoA.szInfo, 64,
                        (LPWSTR) lpCPlInfoW->szInfo, 64);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, CplInfoA.szHelpFile, 128,
                        (LPWSTR) lpCPlInfoW->szHelpFile, 128);
}

//////////////////////////////////////////////////////////////////////////////
//
// -added- 4/13/91 to strip the "&" because DrawText with CALCRECT
// doesn't seem to work -jyg-
//
//////////////////////////////////////////////////////////////////////////////

int GetNameExtent(HDC hDC, LPTSTR szName)
{
    TCHAR   szBuffer[128];
    LPTSTR  pstr;
    SIZE    Size;

    pstr = szBuffer;

#ifdef JAPAN    // Apr-23-1993 MSKK [ShigeO]
    do
    {
        if (*szName == TEXT('(') && *(szName+1) == TEXT('&') && *(szName+3) == TEXT(')'))
            szName += 4;
    }
#else
    do
    {
        if (*szName == TEXT('&'))
            szName++;
    }
#endif
    while (*pstr++ = *szName++);

    GetTextExtentPoint(hDC, szBuffer, lstrlen(szBuffer), &Size);

    return (int)Size.cx;
}

//////////////////////////////////////////////////////////////////////////////
//
// -added- 4/16/91 If someone has an applet without an accelerator we
// add it to the first character -jyg-
//
//////////////////////////////////////////////////////////////////////////////

void ScanName(LPTSTR szName)
{
    TCHAR  szBuffer[128];
    LPTSTR pstr1 = szName, pstr2 = szBuffer;

    while (*pstr1)
    {
        if (*pstr1 == TEXT('&'))
        {
            if (*(pstr1+1) != TEXT('&'))
                return;
            if (*(pstr1+1) == TEXT('&'))
                pstr1++;
        }
        pstr1++;
    }

    // No ampersand! Tack one on the first char

    szBuffer[0] = TEXT('&');
    szBuffer[1] = TEXT('\0');
    lstrcat(szBuffer, szName);
    lstrcpy(szName, szBuffer);
}


//////////////////////////////////////////////////////////////////////////////
//
// Init the icon handle, and ptrs to the strings.
//
//////////////////////////////////////////////////////////////////////////////

void SetAppItems(PCPLMODULE pCPlMod, LPCPLINFO pCPlInfo, LPNEWCPLINFO pNewCPlInfo, PCPLAPPLET pCPlApp, int iIndex)
{
    HANDLE  hLib;
    HMENU   hMenu;
    HDC     hDC;
    HFONT   hFont;
    int     numChars;
    TCHAR   szBuffer[128];
    TCHAR   szTemp[10];
    LPNTCPL lpNtCPl;

    hLib = pCPlMod->hLibrary;

    // now the help, info and icon stuff

    if (pNewCPlInfo)
    {
        pCPlApp->lData = pNewCPlInfo->lData;
        pCPlApp->dwContext = pNewCPlInfo->dwHelpContext;
        if (pNewCPlInfo->szHelpFile)
        {
            pCPlApp->pszHelpFile = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(lstrlen(pNewCPlInfo->szHelpFile)+1));
            if (pCPlApp->pszHelpFile)
                lstrcpy(pCPlApp->pszHelpFile, pNewCPlInfo->szHelpFile);
        }
        pCPlApp->hIcon = pNewCPlInfo->hIcon;
        lstrcpy(szBuffer, pNewCPlInfo->szInfo);
        numChars = lstrlen(szBuffer);
    }
    else
    {
        pCPlApp->lData = pCPlInfo->lData;
        pCPlApp->dwContext = 0L;
        pCPlApp->pszHelpFile = NULL;

        /* get the text for the applet's description */
        pCPlApp->hIcon = LoadIcon (hLib, (LPTSTR) MAKEINTRESOURCE(pCPlInfo->idIcon));
        numChars = LoadString (hLib, pCPlInfo->idInfo, szBuffer, CharSizeOf(szBuffer));
    }

    if (pCPlApp->pszInfo = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(numChars+1)))
        lstrcpy(pCPlApp->pszInfo, szBuffer);

    /* get the text for the applet's name */
    if (pNewCPlInfo)
    {
        lstrcpy(szBuffer, pNewCPlInfo->szName);
        numChars = lstrlen(szBuffer);
    }
    else
    {
        numChars = LoadString (hLib, pCPlInfo->idName, szBuffer, CharSizeOf(szBuffer) - 5);
    }

    if (pCPlApp->pszName = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(numChars+1)))
    {
        LPTSTR pIn, pOut;

        //  Save the complete name of the applet which has the '&' char
        if (pCPlApp->pszFullName = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(numChars+1)))
            lstrcpy (pCPlApp->pszFullName, szBuffer);


        pIn = szBuffer;
        pOut = pCPlApp->pszName;
#ifdef JAPAN    /* V-KeijiY  July.7.1992 */
        do
        {
            if (*pIn == TEXT('(') && *(pIn+1) == TEXT('&') && *(pIn+3) == TEXT(')'))
                pIn += 3;
            else if (*pIn != TEXT('&'))
                *pOut++ = *pIn;
        } while (*pIn++) ;
#else
        do
        {
            if (*pIn != TEXT('&'))
                *pOut++ = *pIn;
        } while (*pIn++) ;
#endif

        // check to see if this is in the [don't load] section
        // after stripping & junk

         GetPrivateProfileString(TEXT("don't load"), pCPlApp->pszName, szNULL,
                                 szTemp, 10, aszControlIniPath);
         if (szTemp[0])    // yep, don't put this in the list
            return;
    }

    if (!hCPlWnd)           // don't add the LB stuff if we aren't showing
        return;             // the main window

    hDC = GetDC (NULL);
    hFont = SelectObject (hDC, hCPlFont);

    // longest name determines column width

    /* Get the name length without '&' and adjust the longest name
       Also add & before the first char if there is none  -jyg- */

    if ( (pCPlApp->iNameW = GetNameExtent(hDC, szBuffer)) > iWidth)
        iWidth = pCPlApp->iNameW ;

    SelectObject (hDC, hFont);
    ReleaseDC (NULL, hDC);

    ScanName (szBuffer);    // adds '&'

    /* append a new menu item to the Settings pop-up menu */
    lstrcat (szBuffer, szDots);
    hMenu = GetMenu (hCPlWnd);
    hMenu = GetSubMenu (hMenu, 0);
    InsertMenu (hMenu, numApps,
                (!numApps || (numApps & 0x0F)) ?
                MF_STRING|MF_BYPOSITION : MF_STRING|MF_MENUBARBREAK|MF_BYPOSITION,
                numApps+MENU_SETTINGS, szBuffer);

    numApps++;

    lpNtCPl = (LPNTCPL) LocalAlloc (LPTR, sizeof(NTCPL));

    if (lpNtCPl)
    {
        lpNtCPl->pCPlMod = pCPlMod;
        lpNtCPl->iApplet = iIndex;
        SendMessage( hCPlLB, LB_ADDSTRING, 0, (LONG)lpNtCPl);
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// Send the inquire messages for each app and initialize necessary
// structs for them.
//
//////////////////////////////////////////////////////////////////////////////

void AskApps(PCPLMODULE pCPlMod, int numApplets)
{
    CPLINFO     CPlInfo;
    NEWCPLINFO  NewCPlInfo;
    PCPLAPPLET  pCPlApp;
    int         i;

    for (i = 0, pCPlApp = pCPlMod->pCPlApps; i < numApplets; i++, pCPlApp++)
    {
        // try the new method first

        NewCPlInfo.dwSize = 0L;
        NewCPlInfo.dwFlags = 0L;
        (*pCPlMod->lpfnCPlApplet)(hCPlWnd,
                                  CPL_NEWINQUIRE,
                                  (LONG)i,
                                  (LONG)&NewCPlInfo);

        if (NewCPlInfo.dwSize == sizeof(NEWCPLINFOA))
        {
            ConvertCplInfoA (&NewCPlInfo);
            SetAppItems(pCPlMod, NULL, &NewCPlInfo, pCPlApp, i);
        }
        else if (NewCPlInfo.dwSize == sizeof(NEWCPLINFO))
        {
            SetAppItems(pCPlMod, NULL, &NewCPlInfo, pCPlApp, i);
        }
        else
        {
            // old method
            (*pCPlMod->lpfnCPlApplet)(hCPlWnd, CPL_INQUIRE, (LONG)i,
                                                (LONG)&CPlInfo);
            SetAppItems(pCPlMod, &CPlInfo, NULL, pCPlApp, i);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// Returns TRUE if lpszStr and lpszName match
//
//////////////////////////////////////////////////////////////////////////////

BOOL NameCmp (LPTSTR lpszStr, LPTSTR lpszName)
{
    while (*lpszStr)
    {
        if (*lpszName == TEXT('&'))
            lpszName++;

        // Compare the uppercase of the first character

        else if ((TCHAR)(ULONG)CharUpper((LPTSTR)(ULONG)(TUCHAR)*(lpszStr++)) !=
                 (TCHAR)(ULONG)CharUpper((LPTSTR)(ULONG)(TUCHAR)*(lpszName++)))
            return FALSE;
    }
    return !*lpszName;
}

int FindApplet (LPTSTR szName)
{
    PCPLMODULE  pCPlMod;
    PCPLAPPLET  pCPlApp;
    int         iApplet;
    int         i, n;
    LONG        lData;

    n = (int)SendMessage (hCPlLB, LB_GETCOUNT, 0, 0L);

    for (i = 0; i < n; i++)
    {
        lData = SendMessage (hCPlLB, LB_GETITEMDATA, i, 0L );

        iApplet = ((LPNTCPL)lData)->iApplet;
        pCPlMod = ((LPNTCPL)lData)->pCPlMod;
        pCPlApp = pCPlMod ? &pCPlMod->pCPlApps[iApplet] : NULL;

        if (pCPlApp && NameCmp (szName, pCPlApp->pszName))
            return i;
    }

    return(-1);
}

//////////////////////////////////////////////////////////////////////////////
//
// Return whether we already loaded this module.
//
//////////////////////////////////////////////////////////////////////////////

BOOL AlreadyLoaded(HANDLE hLib, int numMods)
{
    int  i;

    for (i = numMods - 1; i >= 0; i--)
    {
        if (CPlMods[i]->hLibrary == hLib)
            return TRUE;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Pass initial messages to the indicated module, and fill in the
//  passed CPLMODULE structure.
//
//////////////////////////////////////////////////////////////////////////////

BOOL AskModule (LPTSTR name, int numMods)
{
    TCHAR       szLoadingName[64+ CharSizeOf(szLoading)];
    int         numApplets;
    BOOL        bLoaded=FALSE;
    PCPLMODULE  pCPlMod;
    UINT        uError;


    if (pCPlMod = (PCPLMODULE)LocalAlloc (LPTR, sizeof(CPLMODULE)))
    {
        //  Module pathname
        lstrcpy(pCPlMod->szPathname, name);

        if (hCPlWnd)
        {
            /* update feedback on what applet library is currently loading */
#ifdef JAPAN    /* V-KeijiY  July.6.7 */
            lstrcpy(szLoadingName, name);
            lstrcat(szLoadingName, szLoading);
#else
            lstrcpy(szLoadingName, szLoading);
            lstrcat(szLoadingName, name);
#endif

            SetWindowText(hCPlTB, szLoadingName);
            UpdateWindow(hCPlWnd);
        }

        uError = SetErrorMode (SEM_FAILCRITICALERRORS);

        if (pCPlMod->hLibrary = LoadLibrary(name))
        {
            if (!AlreadyLoaded(pCPlMod->hLibrary, numMods)  &&
                (pCPlMod->lpfnCPlApplet =
                 (APPLET_PROC)GetProcAddress(pCPlMod->hLibrary, szCPlApplet))
                 && (*pCPlMod->lpfnCPlApplet)(hCPlWnd, CPL_INIT, 0L, 0L))
            {

                numApplets = (int)(*pCPlMod->lpfnCPlApplet)(hCPlWnd,
                                                        CPL_GETCOUNT, 0L, 0L);
                pCPlMod->numApplets = numApplets;

                if (pCPlMod->pCPlApps = (PCPLAPPLET)LocalAlloc(LPTR,
                                                numApplets*sizeof(CPLAPPLET)))
                {
                    AskApps (pCPlMod, numApplets);
                    pCPlMod->bLoaded = bLoaded = TRUE;
                    CPlMods[numMods] = pCPlMod;
                }
            }
            else
                FreeLibrary(pCPlMod->hLibrary);
        }
        else
            FreeLibrary(pCPlMod->hLibrary);

        SetErrorMode (uError);
    }

    if (!bLoaded && pCPlMod)
        LocalFree((HANDLE)pCPlMod);

    return bLoaded;
}


//////////////////////////////////////////////////////////////////////////////
//
// Copy pstrFile on top of the filename already attached pstrPath.
// Length is the length of pstrPath.
//
//////////////////////////////////////////////////////////////////////////////

void CatPath(LPTSTR pstrPath, LPTSTR pstrFile, int length )
{
    LPTSTR    pstr;

    for ( pstr = pstrPath + length;
          ( pstr >= pstrPath ) && ( *pstr != cBackslash );
          pstr = CharPrev(pstrPath,pstr)
        )
        ;
    lstrcpy( pstr+1, pstrFile );
}


//////////////////////////////////////////////////////////////////////////////
//
// Return the WIN3 SYSTEM dir concatenated with the past filename.
// Assumes array size of pstrPath = 256, and pstrFile begins with a '\'.
//
//////////////////////////////////////////////////////////////////////////////

void MakeSysPath (LPTSTR pstrPath, LPTSTR pstrFile)
{
    int     length;
    LPTSTR  pstr;

    length = GetSystemDirectory (pstrPath, 256);

    pstr = CharPrev (pstrPath, pstrPath+length);

    if (*pstr != cBackslash )
        *(++pstr) = cBackslash;  // add the trailing '\'

    lstrcpy (pstr+1, pstrFile);
}


//////////////////////////////////////////////////////////////////////////////
//
// Return the WIN3 dir concatenated with the past filename.
// Assumes array size of pstrPath = 256, and pstrFile begins with a '\'.
//
//////////////////////////////////////////////////////////////////////////////

void MakeWinPath (LPTSTR pstrPath, LPTSTR pstrFile)
{
    int     length;
    LPTSTR  pstr;

    length = GetWindowsDirectory (pstrPath, 256);

    pstr = CharPrev (pstrPath, pstrPath+length);

    if (*pstr != cBackslash )
        *(++pstr) = cBackslash;  // add the trailing '\'

    lstrcpy (pstr+1, pstrFile);
}


//////////////////////////////////////////////////////////////////////////////
//
// Get the keynames under [MMCPL] in CONTROL.INI and cycle
// through all such keys to load their applets into our
// list box.  Also allocate the array of CPLMODULE structs.
// Returns early if can't load old WIN3 applets.
//
//////////////////////////////////////////////////////////////////////////////

int LoadApplets()
{
    LPTSTR      pstr;
    int         i, numMods = 0;
    TCHAR       szKeys[256];    // array size = 256 is assumed by GetSysDir()!
    TCHAR       szName[64];
    HANDLE      fFile;
    BOOL        b;
    WIN32_FIND_DATA   FindFileData;

    // Load in MAIN.CPL first, then search for and load any other applets

    /* try the SYSTEM directory */
    MakeSysPath (szKeys, szMAINCPL);

    if (AskModule (szKeys, numMods))
       numMods += 1;

    // load the modules specified in CONTROL.INI under [MMCPL]

    GetPrivateProfileString(szMMCPL, NULL, szNULL, szKeys, CharSizeOf(szKeys), aszControlIniPath);

    for (pstr = szKeys; *pstr && (numMods < CPL_MAXMODS-1); pstr += lstrlen(pstr) + 1)
    {
        GetPrivateProfileString(szMMCPL, pstr, szNULL, szName, 64, aszControlIniPath);

        if (_tcsicmp(pstr, szNumApps) && _tcsicmp(pstr, szMaxWidth) &&
           !((*(pstr+1) == (TCHAR) 0)  &&           // skip over Wnd size keynames
               ((*pstr == *szWndDim[0]) || (*pstr == *szWndDim[1]) ||
                (*pstr == *szWndDim[2]) || (*pstr == *szWndDim[3])) ) &&
            AskModule(szName, numMods))
            numMods += 1;       // Increment module count if it loaded ok
    }

    // load applets from the system directory

    MakeSysPath (szKeys, szCPL);

    if ((fFile = FindFirstFile(szKeys, &FindFileData)) != INVALID_HANDLE_VALUE)
    {
        b = TRUE;

        while (b && (numMods < CPL_MAXMODS - 1))
        {
            //  Since we force load of MAIN.CPL first, skip over this
            //  file when we find it during the FINDFILE operation.

            if (_tcsicmp (FindFileData.cFileName, szMAINCPL))
            {
                CatPath(szKeys, FindFileData.cFileName, lstrlen(szKeys));
                if (AskModule(szKeys, numMods))
                   numMods++;
            }
            b = FindNextFile(fFile, &FindFileData);
        }
        FindClose(fFile);
    }

    CPlMods[numMods] = NULL;        // NULL terminate CPlMods[]

    /////////////////////////////////////////////////////////////////////
    //  Now free all .CPL modules until the User asks for one
    /////////////////////////////////////////////////////////////////////

    for (i = 0; i < numMods; )
        FreeCachedModule (CPlMods[i++]);

    return numMods;
}


//////////////////////////////////////////////////////////////////////////////
//
// Allocate memory and load the strings that CPL must always have around
// Assumes hCPlInst has already been set.
//
//////////////////////////////////////////////////////////////////////////////

BOOL GetCPLStrings(void)
{
    WORD  wString;
    TCHAR szTemp[256];

    for (wString = 0; wString < NUMCPLSTRINGS; ++wString)
    {
        /* No need to deallocate memory, since it all goes away if we
         * return FALSE
         */
        if (!LoadString (hCPlInst, INITS+wString, szTemp, CharSizeOf(szTemp)))
            return (FALSE);
        if (!(pStrings[wString] = (LPTSTR) LocalAlloc (LPTR, ByteCountOf(lstrlen(szTemp)+1))))
            return (FALSE);
        lstrcpy (pStrings[wString], szTemp);
    }

    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
// Creates the owner-draw list box for displaying the applet icons,
// the static text control for the description, and calls routines
// to load and intialize the applets.
// Returns successful if was at least able to initialize the old
// WIN3 applets.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CreatChildWindows(void)
{
    TEXTMETRIC tm;
    HDC        hDC;
    HFONT      hFont;
    LOGFONT    lf;   // logical font for Applet name area

    /* init the height of the Info text box */

    // this font and brush init stuff should go somewhere else!

    SystemParametersInfo (SPI_GETICONTITLELOGFONT, sizeof(lf), (PVOID)&lf, FALSE);
    hCPlFont = CreateFontIndirect (&lf);

    hDC = GetDC (NULL);
    if (hCPlFont)
        hFont = SelectObject (hDC, hCPlFont);
    else
        hFont = NULL;
    GetTextMetrics (hDC, &tm);

    if (hFont)
        SelectObject(hDC, hFont);
    ReleaseDC (NULL, hDC);
    ifontH = tm.tmHeight + tm.tmExternalLeading;

    /* create a brush to be used for painting the list box background */
    hCPlBk = CreateSolidBrush (GetSysColor (COLOR_APPWORKSPACE));

    /* create the list box to hold all the applets */
    hCPlLB = CreateWindow (szlistbox, NULL,
                           WS_CHILD | WS_VISIBLE | WS_BORDER |
                           WS_HSCROLL| LBS_NOTIFY | LBS_MULTICOLUMN     |
                           LBS_NOINTEGRALHEIGHT   | LBS_OWNERDRAWFIXED  |
                           LBS_WANTKEYBOARDINPUT,
                           0, 0, 0, 0, hCPlWnd, (HMENU)ID_LISTBOX, hCPlInst, NULL);

    if (!hCPlLB)
        return FALSE;

    /* create the box at bottom where applet description text is displayed */
    hCPlTB = CreateWindow (szText, szLoading,
                           WS_BORDER | SS_LEFT | WS_CHILD | WS_VISIBLE,
                           0, 0, 0, 0, hCPlWnd, (HMENU)ID_INFOBOX, hCPlInst, NULL);

    if (!hCPlTB)
        return FALSE;

    return TRUE;
}


int LoadAndSizeApplets(void)
{
    int     iLoaded;
    int     nNumApps;
    HCURSOR hCur;
    int     iOldWidth;
    TCHAR   szTemp[20];
    BOOL    bCache = FALSE;

    hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    ///////////////////////////////////////////////////////////////////////
    //  Old way
    //    SendMessage (hCPlLB, WM_SETREDRAW, FALSE, 0L);
    ///////////////////////////////////////////////////////////////////////

    //  Preliminary setup of values to allow drawing of ICONs in LB
    //  as .CPL modules are read in by Control
    //
    //  Get stored "MaxWidth" value as a good starting point
    //
    // NOTE: Assumes that iWidth (global) is initialized to a reasonable value

    iOldWidth = GetPrivateProfileInt(szMMCPL, szMaxWidth, iWidth, aszControlIniPath);
    SendMessage (hCPlLB, LB_SETCOLUMNWIDTH, iOldWidth, 0L);

    iIconX = (iOldWidth - iIconW) / 2;
    iIconY = ifontH >> 1;

    InvalidateRect (hCPlTB, NULL, TRUE);
    UpdateWindow (hCPlTB);

    //  Init iWidth global to 1, in order to force a recalc of text widths
    iWidth = 1;

    //////////////////////////////////////////////////////////////////////////////
    //  Check cache first
    //////////////////////////////////////////////////////////////////////////////

    if (CheckCache())
    {
        bCache = TRUE;
        SendMessage (hCPlLB, WM_SETREDRAW, FALSE, 0L);

        if (!LoadFromCache())
        {
FullLoad:
            bCache = FALSE;
            SendMessage (hCPlLB, WM_SETREDRAW, TRUE, 0L);

            //  LoadApplets will recalculate "iWidth" value

            if ((iLoaded = LoadApplets()) != 0)
            {
CheckColumnWidth:
                if (iOldWidth != iWidth)
                {
                    // Save new MaxWidth value
                    wsprintf (szTemp, TEXT("%d"), iWidth);
                    WritePrivateProfileString(szMMCPL, szMaxWidth, szTemp, aszControlIniPath);

                    /* Set the column width from the widest name */
                    SendMessage (hCPlLB, LB_SETCOLUMNWIDTH, iWidth, 0L);
                    iIconX = (iWidth - iIconW) / 2;
                    iIconY = ifontH >> 1;
                }

//              SendMessage (hCPlLB, LB_SETCURSEL, 0, 0L);
            }
        }
        else
        {
            iLoaded = 1;

            goto CheckColumnWidth;
        }
    }
    else
        goto FullLoad;


    //////////////////////////////////////////////////////////////////////////////
    //  Check and Update cache as necessary
    //////////////////////////////////////////////////////////////////////////////

    if (!IsCacheValid())
        UpdateCache();


    if (!IsIconic(hCPlWnd) &&
        (nNumApps = (int)SendMessage(hCPlLB, LB_GETCOUNT, 0, 0L)) !=
        (int)GetPrivateProfileInt(szMMCPL, szNumApps, 0, aszControlIniPath))
    {
        int nx, nWidth, nMaxWidth;
        int ny, nHeight, nMaxHeight;
        RECT rLB, rCPl;

        wsprintf(szTemp, TEXT("%d"), nNumApps);
        WritePrivateProfileString(szMMCPL, szNumApps, szTemp,aszControlIniPath);

        GetClientRect(hCPlLB, &rLB);
        nx = rLB.right / iWidth;
        if (!nx)
            nx = 1;
        ny = (nNumApps + nx - 1) / nx;
        nHeight = ny * ItemHeight;
        if (nHeight > rLB.bottom)
        {
            /* At this point, there must be a scroll bar, which makes
             * out calculation of nWidth 16 pixels too large, but I like
             * the effect, so I'm going to leave it.
             */
            GetWindowRect(hCPlWnd, &rCPl);
            nWidth = 7 * iWidth;
            nWidth += rCPl.right-rCPl.left - rLB.right;

            nMaxWidth = GetSystemMetrics( SM_CXSCREEN );
            if (nWidth > nMaxWidth)
                nWidth = nMaxWidth;
            if (rCPl.left > nMaxWidth-nWidth)
                rCPl.left = nMaxWidth-nWidth;

            nx = nWidth / iWidth;
            if (!nx)
                nx = 1;
            ny = (nNumApps + nx - 1) / nx;
            nHeight = ny * ItemHeight;
            nHeight += rCPl.bottom-rCPl.top - rLB.bottom;

            nMaxHeight = GetSystemMetrics( SM_CYSCREEN );
            if (nHeight > nMaxHeight)
                nHeight = nMaxHeight;
            if (rCPl.top > nMaxHeight-nHeight)
                rCPl.top = nMaxHeight-nHeight;

            SetWindowPos(hCPlWnd, NULL, rCPl.left, rCPl.top,
                                                nWidth, nHeight, SWP_NOZORDER);
        }
    }

    ///////////////////////////////////////////////////////////////////////
    //  Old way
    //    SendMessage (hCPlLB, WM_SETREDRAW, TRUE, 0L);
    //    InvalidateRect (hCPlWnd, NULL, TRUE);
    ///////////////////////////////////////////////////////////////////////

    // New way - don't force a redraw unless necessary

    if (iOldWidth != iWidth)
        InvalidateRect (hCPlWnd, NULL, TRUE);
    else
        // Force a re-size of ListBox to draw scroll bars, as necessary
        SendMessage (hCPlLB, WM_SIZE, 0, 0L);

    if (bCache)
    {
        DWORD tid;
        HANDLE hThread;

        SendMessage (hCPlLB, LB_SETCURSEL, 0, 0L);
        SendMessage (hCPlLB, WM_SETREDRAW, TRUE, 0L);
        InvalidateRect (hCPlWnd, NULL, TRUE);

        ////////////////////////////////////////////////////////////////////
        //  If we loaded from the Cache perform an Absolute Validation of
        //  of applets in the background, just to be sure something did not
        //  enable or change one of the applets that we could not detect
        //  during CheckCache().
        ////////////////////////////////////////////////////////////////////

        UpdateWindow (hCPlWnd);

        bValidationDone = FALSE;
        hThread = CreateThread (NULL, 0,
                                (LPTHREAD_START_ROUTINE) AbsoluteValidation,
                                NULL, 0, &tid);

        CloseHandle(hThread);
    }
    else
    {
        //  Force selection of first Lbox item (also force redraw of Info text)

        SendMessage (hCPlLB, LB_SETCURSEL, 1, 0L);
        SendMessage (hCPlLB, LB_SETCURSEL, 0, 0L);
    }

    SetCursor (hCur);
    return (iLoaded);
}


PCPLAPPLET SendAppMsg (int Msg, DWORD i)
{
    HCURSOR     hCur;
    PCPLMODULE  pCPlMod;
    PCPLAPPLET  pCPlApp;
    int         iApplet;
    LONG        lData;
    TCHAR       szMutex[MAX_PATH];
    HANDLE      hmutex = NULL;

    //
    //  Check and set global flag indicating an applet is active
    //

    if (bAppletActive)
        return NULL;

    bAppletActive = TRUE;

    lData = SendMessage (hCPlLB, LB_GETITEMDATA, i, 0L);

    if (Msg == CPL_DBLCLK)
        hCur = SetCursor (LoadCursor(NULL, IDC_WAIT));

    iApplet = ((LPNTCPL)lData)->iApplet;
    pCPlMod = ((LPNTCPL)lData)->pCPlMod;
    pCPlApp = pCPlMod ? pCPlMod->pCPlApps + iApplet : NULL;

    ///////////////////////////////////////////////////////////////////////
    //
    //  Before loading this module, make sure that it is not in use
    //  by the AbsoluteValidation thread.
    //
    ///////////////////////////////////////////////////////////////////////

    if (!bValidationDone)
    {
        CreateMutexNameFromPath (pCPlMod->szPathname, szMutex);
        hmutex = CreateMutex (NULL, FALSE, szMutex);

        WaitForSingleObject (hmutex, INFINITE);
    }

    if ((Msg == CPL_DBLCLK) && (pCPlMod->bLoaded == FALSE))
    {
        //  Load module into memory and init it
        pCPlMod->bLoaded = LoadCachedModule (pCPlMod);
    }

    if (pCPlMod->bLoaded && pCPlApp)
    {
//        ASSERT(*pCPlMod->lpfnCPlApplet != NULL);

        (*pCPlMod->lpfnCPlApplet)(hCPlWnd, Msg, (LONG)iApplet, pCPlApp->lData);

        if (Msg == CPL_DBLCLK)
        {
            (*pCPlMod->lpfnCPlApplet)(hCPlWnd, CPL_STOP, (LONG)iApplet, pCPlApp->lData);
            FreeCachedModule (pCPlMod);
            SetCursor (hCur);
        }
    }

    if (hmutex)
    {
        ReleaseMutex (hmutex);
        CloseHandle (hmutex);
    }

    bAppletActive = FALSE;

    //
    //  Check global flag to see if we should rebuild applet cache
    //

    if (bRebuildCache)
    {
        //
        //  IMPORTANT NOTE:  Global BOOL value must be set FALSE here
        //  before calling RebuildCache() in order to avoid infinite
        //  recursion, and eventual stack overflow.  SendAppMsg() is
        //  called many times during the cache rebuild processing.
        //
        //  ALSO:  RebuildCache() must be called only after the
        //         bAppletActive flag is reset, or else the cache
        //         will never get rebuilt.
        //

        bRebuildCache = FALSE;
        RebuildCache ();
    }

    return (pCPlApp);
}


//////////////////////////////////////////////////////////////////////////////
//
//  Run the applet specified on the Command line
//
//  NOTE:  This routine must ReleaseMutex on the hmutex if it is going to
//         run a control panel applet.  It must NOT release the mutex if
//         it is going to return FALSE.
//
//////////////////////////////////////////////////////////////////////////////

BOOL RunCommandLineApplet (int argc, LPTSTR szCplFile, LPTSTR szApplet, HANDLE hmutex)
{
  HCURSOR     hCur;
  PCPLMODULE  pCPlMod;
  PCPLAPPLET  pCPlApp;
  int         iApplet;

    hCur = SetCursor (LoadCursor (NULL, IDC_WAIT));

    //  See if this is a valid Control Panel applet file

    if (!AskModule (szCplFile, 0))
    {
        SetCursor (hCur);
        return (FALSE);
    }

    pCPlMod = CPlMods[0];

    //  Now search for the specified applet within the CPL file

    if (argc > 2)
    {
       for (iApplet = 0; iApplet < pCPlMod->numApplets; iApplet++)
       {
          if (NameCmp (szApplet, pCPlMod->pCPlApps[iApplet].pszName))
             break;
       }
    }
    else
       iApplet = 0;

    //  If search not successful, launch the first applet in CPL file

    if (iApplet >= pCPlMod->numApplets)
        iApplet = 0;

    pCPlApp = &pCPlMod->pCPlApps[iApplet];

    //  If this is a valid applet, release Initialization mutex to let
    //  other instances of the Control Panel run

    if (hmutex)
    {
        ReleaseMutex (hmutex);
        CloseHandle (hmutex);
    }

    // If running under "Setup" let applet know first
    if (pCPlApp && bSetup)
        (*pCPlMod->lpfnCPlApplet)(NULL, CPL_SETUP, 0L, 0L);

    if (pCPlApp)
        (*pCPlMod->lpfnCPlApplet)(NULL, CPL_DBLCLK, (LONG)iApplet, pCPlApp->lData);

    FreeLibrary (pCPlMod->hLibrary);

    SetCursor (hCur);

    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  Set the initial position and size of CPl app window, and return
//  whether the window was last closed as an icon.
//
//////////////////////////////////////////////////////////////////////////////

void GetCPlWndPos(int *CPlWndPos)
{
    int     i, iX, iY;

    // Set the default position and size.

    // Default sizing is normally determined by the profile.  In the
    // event that the profile is munged, we default to fixed sizes.
    // We should add a resize message based on the icon metrics to be
    // nice to developers adding their own CPL applets. -jyg 4/9/91

    // System position defaults (better than fixed).

    CPlWndPos[0] = CW_USEDEFAULT;
    CPlWndPos[1] = 0;

    // Fixed size defaults.  The spec says we should be big enough to hold
    // all of our own (1.0) icons.

    CPlWndPos[2] = CPL_DEFDIMW;
    CPlWndPos[3] = CPL_DEFDIMH;

    // Let CONTROL.INI defines initial position and size
    for (i = 0; i < 4; i++ )
        CPlWndPos[i] = GetPrivateProfileInt (szMMCPL,
                                  szWndDim[i], CPlWndPos[i], aszControlIniPath);

    // Make sure CONTROL.INI position didn't put us off the screen

    iX = GetSystemMetrics (SM_CXSCREEN);
    iY = GetSystemMetrics (SM_CYSCREEN);

    // Remember, CW_USEDEFAULT is ((int)0x8000) (-1)

    //  Also make sure at least 25% of Window is visible to User

    if (CPlWndPos[0] != CW_USEDEFAULT)
        while (CPlWndPos[0] >= (iX - CPlWndPos[2] / 4))
            CPlWndPos[0] /= 2 ;

    while (CPlWndPos[1] >= (iY - CPlWndPos[3] / 4))
        CPlWndPos[1] /= 2 ;
}


//////////////////////////////////////////////////////////////////////////////
//
//  Record the last valid size of the CPL window.
//
//////////////////////////////////////////////////////////////////////////////

void SetCPlWndPos (HWND hWnd)
{
    int     i, dim[4];
    TCHAR   szBuffer[64];
    WINDOWPLACEMENT wp;

    wp.length = sizeof(wp);
    GetWindowPlacement (hWnd, &wp);

    dim[0] = wp.rcNormalPosition.left;
    dim[1] = wp.rcNormalPosition.top;
    dim[2] = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    dim[3] = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;

    for (i = 0; i < 4; i++ )
    {
        wsprintf (szBuffer, TEXT("%d"), dim[i]);
        WritePrivateProfileString (szMMCPL,
                                   szWndDim[i], szBuffer, aszControlIniPath);
    }
}


//////////////////////////////////////////////////////////////////////////////
//
//  For each CPLMODULE free its CPLAPPLET array, and library handle
//  and then free the CPLMODULE array.
//
//////////////////////////////////////////////////////////////////////////////

void FreeApps(void)
{
    PCPLMODULE  pCPlMod;
    int         i;

    // free lib handles and CPLMod array here
    for (i = 0; pCPlMod = CPlMods[i]; i++)
    {
        LocalFree ((HANDLE)pCPlMod->pCPlApps);

        // no longer in need of your services
        if (*pCPlMod->lpfnCPlApplet)
            (*pCPlMod->lpfnCPlApplet) (hCPlWnd, CPL_EXIT, 0L, 0L);

        if (pCPlMod->hLibrary)
            FreeLibrary (pCPlMod->hLibrary);
        LocalFree ((HANDLE)pCPlMod);
        CPlMods[i] = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////
//
//  Taken from old WIN3 UTILTEXT.C.
//
//////////////////////////////////////////////////////////////////////////////

void ErrMemDlg (HWND hParent)
{
   MessageBox (hParent, szErrMem, szAppName, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
}


int ChngSelApp (int iApplet,TCHAR chKey)
{
    LPTSTR pstr;
    LONG lItemData;
    PCPLAPPLET pCPlApp;
    int i;
    //
    // change selected applet to the next one whose accelerator
    // matches the character just pressed
    //
    //

    for (i = iApplet + 1; i != iApplet; i++)
    {
        if (i == (int) numApps)
            //
            // wrap search around to the first applet
            //
            if (iApplet)
                i = 0;
            else
                //
                // started with the first applet so outta here
                //
                return -1;

        //
        // compare the accelerator letter of this applet with user key
        //
        if ((lItemData = SendMessage (hCPlLB, LB_GETITEMDATA, i, 0L)) != LB_ERR)
        {
            int iTmpApplet;

            iTmpApplet = ((LPNTCPL)lItemData)->iApplet;
            pCPlApp = (((LPNTCPL)lItemData)->pCPlMod)->pCPlApps + iTmpApplet;

            pstr = pCPlApp->pszName;

            // look for accelerator part "&x"

            while ((*pstr) != TEXT('\0'))
            {
                if (*pstr == TEXT('&'))
                {
                    if (*(pstr+1) == TEXT('&'))
                        pstr++;
                    else if ((TCHAR)(ULONG)CharUpper((LPTSTR)(ULONG)(TUCHAR)*(pstr + 1)) == chKey)
                        //
                        // found a match, make it the current selection
                        //
                        return( i );
                }
                pstr++;
            }

            // now look for first char only

            if ((TCHAR)(ULONG)CharUpper((LPTSTR)(ULONG)(TUCHAR)*(pCPlApp->pszName)) == chKey)
                return i;
        }
    }
    return -1;
}


//////////////////////////////////////////////////////////////////////////////
//
//  GetAppletName
//
//  Read the name of the applet from another processes memory space and
//  copy it to local buffer.  If the calling app is ANSI, assume that
//  we are reading an ANSI string and vice versa for UNICODE.
//
//////////////////////////////////////////////////////////////////////////////

BOOL GetAppletName (DWORD dwProcessId, LPTSTR lpszAppletName, DWORD dwAddress, BOOL fUnicode)
{
    HANDLE hProcess;
    DWORD  dwBytesRead;
    TCHAR  szReadBufferW[2];
    CHAR   szReadBufferA[2];
    CHAR   szNameBufferA[MAX_APPLET_NAME_LEN];
    int    i;


    hProcess = OpenProcess (PROCESS_VM_READ, FALSE, dwProcessId);

    //  Read other processes memory 1 or 2 bytes at a time until we reach
    //  end of their string or our data buffer (assumed to be 32 bytes)

    if (hProcess)
    {
        i = 0;

        if (fUnicode)
        {
            while (ReadProcessMemory (hProcess,
                                      (LPVOID) dwAddress,
                                      szReadBufferW,
                                      2,
                                      &dwBytesRead))
            {
                if ((dwBytesRead == 2) && (i < MAX_APPLET_NAME_LEN-1))
                {
                    lpszAppletName[i++] = szReadBufferW[0];

                    //  Reached end of string?
                    if (szReadBufferW[0] == TEXT('\0'))
                        break;
                    else
                        dwAddress += 2;
                }
                else
                {
                    //  Error - Terminate string
                    lpszAppletName[i] = TEXT('\0');
#if DBG
                    GetLastError();
#endif  // DBG
                    break;
                }
            }
        }
        else
        {
            while (ReadProcessMemory (hProcess,
                                      (LPVOID) dwAddress,
                                      szReadBufferA,
                                      1,
                                      &dwBytesRead))
            {
                if ((dwBytesRead == 1) && (i < MAX_APPLET_NAME_LEN-1))
                {
                    szNameBufferA[i++] = szReadBufferA[0];

                    //  Reached end of string?
                    if (szReadBufferA[0] == '\0')
                    {
ConvertAppletName:
                        //  Convert to Unicode
                        MultiByteToWideChar (CP_ACP,
                                             MB_PRECOMPOSED,
                                             szNameBufferA,
                                             -1,
                                             lpszAppletName,
                                             MAX_APPLET_NAME_LEN);
                        break;
                    }
                    else
                        dwAddress++;
                }
                else
                {
                    //  Error - Terminate string, then goto common exit
                    //  for Unicode conversion
                    szNameBufferA[i] = '\0';
#if DBG
                    GetLastError();
#endif  // DBG
                    goto ConvertAppletName;
                    // break;
                }
            }
        }
        return TRUE;
    }
    else
    {
#if DBG
        GetLastError ();
#endif  // DBG
        return FALSE;
    }
}



LRESULT APIENTRY CPlWndProc (HWND hWnd, UINT Message, WPARAM wParam, LONG lParam)
{
    LPDRAWITEMSTRUCT    drwI;
    LPMEASUREITEMSTRUCT measI;
    PCPLAPPLET          pCPlApp;
    PCPLMODULE          pCPlMod;
    HBRUSH              hBrush;
    HFONT               hFont;
    RECT                Rect;
    LONG                rgbBG, rgbFG;
    BOOL                bLaunchedOK;
    UINT                Msg;
    int                 i, dx, dy, iBkMode;
    DWORD               dwContext;
    LPTSTR              pHelpFile;
    TCHAR               szTitle[40];
    DWORD               dwProcessId;
    TCHAR               szAppletName[MAX_APPLET_NAME_LEN];
    BOOL                fUnicode;

    switch (Message)
    {

        //  NOTE: for WM_CPL_LAUNCH and WM_CPL_LAUNCHEX messages
        //
        //  For Windows NT, we may have to try to read the name of the
        //  applet from another processes memory space.  The NT way is
        //  to open that process and do a ReadMemory from its data space.

    case WM_CPL_LAUNCH:
        //  wParam is HWND of calling app
        //  lParam points to name of applet to run.
        //  when done call back the caller, unless it was us

        bLaunchedOK = FALSE;

        if (lParam)
        {
            if (wParam && ((HWND) wParam != hWnd))
            {
                GetWindowThreadProcessId ((HWND) wParam, &dwProcessId);

                fUnicode = IsWindowUnicode ((HWND) wParam);

                if (GetAppletName (dwProcessId, szAppletName, lParam, fUnicode))
                {
                    i = FindApplet(szAppletName);
                }
                else
                {
#if DBG
                    GetLastError ();
#endif // DBG
                    return (LRESULT)bLaunchedOK;
                }
            }
            else
                i = FindApplet ((LPTSTR) lParam);

            if (i >= 0)
            {
                if (wParam)
                    EnableWindow ((HWND)wParam, FALSE);

                SendMessage(hWnd, LB_SETCURSEL, i, 0L);
                bLaunchedOK = SendAppMsg(CPL_DBLCLK, i) != NULL;

                if (wParam)
                    EnableWindow((HWND)wParam, TRUE);
            }
        }

        // if called from another app let 'em know it completed

        if (wParam)
        {
            PostMessage((HWND)wParam, WM_CPL_LAUNCHED, bLaunchedOK, 0L);

            // if the message is from us then it must have been from
            // the command line, so exit

            if (((HWND)wParam == hWnd) && bLaunchedOK)
                    PostMessage(hWnd, WM_CLOSE, 0, 0L);
        }
        return (LRESULT)bLaunchedOK;


    case WM_CPL_LAUNCHEX:
        //  wParam is PROCESS ID of calling app
        //  lParam points to name of applet to run.

        bLaunchedOK = FALSE;

        if (lParam)
        {
            //  For Windows NT, we may have to try to read the name of the
            //  applet from another processes memory space.  The NT way is
            //  to open that process and do a ReadMemory from its data space.

            //  IMPORTANT NOTE: Since this is currently a private message, we
            //  can always assume that it is called from another copy of the
            //  Control Panel and that wParam is the ProcessId. No calls are
            //  made back to calling app.  Also, we can assume that the calling
            //  Control Panel app is UNICODE.

            if (wParam)
            {
                i = 0;

                //  Assume Unicode name string at *lParam
                if (GetAppletName (wParam, szAppletName, lParam, TRUE))
                {
                    i = FindApplet(szAppletName);
                }
                else
                {
#if DBG
                    GetLastError ();
#endif // DBG
                    return (LRESULT)bLaunchedOK;
                }
            }
            else
            {
                //  Called locally, assume lParam ptr is in our address space

                i = FindApplet((LPTSTR)lParam);
            }

            if (i >= 0)
            {
                SendMessage(hWnd, LB_SETCURSEL, i, 0L);
                bLaunchedOK = SendAppMsg(CPL_DBLCLK, i) != NULL;
            }
        }
        return (LRESULT)bLaunchedOK;


    case WM_COMMAND:
        if (lParam && (LOWORD(wParam) == ID_LISTBOX))
        {
           //  it's a list box message
           //
           // Use local signed variable to get around USER bug
           // where they return a negative number for LB_GETCURSEL
           // if someone is holding down Left Mouse Button, moves
           // mouse outside left-side of window and then releases
           // mouse buttom. (See WM_DRAWITEM, below)
           //

           if (HIWORD(wParam) == LBN_DBLCLK)
               Msg = CPL_DBLCLK;
           else if (HIWORD(wParam) == LBN_SELCHANGE)
               Msg = CPL_SELECT;
           else
               break;

           i = SendMessage(hCPlLB, LB_GETCURSEL, 0, 0L);

           if (i >= 0)
               SendAppMsg(Msg, i);
        }
        else if ((LOWORD(wParam) >= MENU_SETTINGS) &&
                 (LOWORD(wParam) < (WORD) (MENU_SETTINGS + (DWORD)numApps)))
        {
            /* it's a menu selection to launch an applet */
            i = wParam - MENU_SETTINGS;
            SendMessage(hCPlLB, LB_SETCURSEL, i, 0L);
            SendAppMsg(CPL_DBLCLK, i);
            return (LRESULT) TRUE;
        }
        else
        {
            /* it's some other menu message */

            switch (LOWORD(wParam))
            {
              /* help stuff taken from old WIN3 source: CONTROL.C */
            case MENU_CACHE:
                RebuildCache ();
                break;

            case MENU_EXIT:
                PostMessage (hWnd, WM_CLOSE, 0, 0L);
                break;

            case MENU_F1:
                i = (int) SendMessage (hCPlLB, LB_GETCURSEL, 0, 0L);
                if (i >= 0)
                {
                    pCPlApp = SendAppMsg(CPL_SELECT, i);
                    dwContext = pCPlApp->dwContext;
                    pHelpFile = pCPlApp->pszHelpFile;
                    CPHelp(hWnd, pHelpFile, dwContext);
                }
                break;

            case MENU_USEHELP:
                if (!WinHelp(hWnd, NULL, HELP_HELPONHELP, 0L))
                    ErrMemDlg(hWnd);
                break;

            case MENU_INDHELP:
                if (!WinHelp(hWnd, szHelpFile, HELP_INDEX, 0L))
                    ErrMemDlg(hWnd);
                break;

            case MENU_SCHHELP:
                if (!WinHelp(hWnd, szHelpFile, HELP_PARTIALKEY, (DWORD)(LPSTR)""))
                    ErrMemDlg(hWnd);
                break;

            case MENU_ABOUT:
              {
                HANDLE hLib;
                SHELL_ABOUT pfn;

                LoadString(hCPlInst, CONTROLABOUT, szTitle, CharSizeOf(szTitle));

                hLib = LoadLibrary (TEXT("Shell32.dll"));

                pfn = (SHELL_ABOUT) GetProcAddress (hLib, "ShellAboutW");

                if (pfn)
                    (*pfn) (hWnd, szTitle, NULL, LoadIcon (hCPlInst,
                                             (LPTSTR) MAKEINTRESOURCE(MMCPL)));
                FreeLibrary (hLib);

                break;
              }
            }
        }
        break;

    case WM_CHARTOITEM:
        //
        // Select the applet according to the accelerator -jyg-
        //
        return(LRESULT)(ChngSelApp((int)HIWORD(wParam),
                    (TCHAR)(ULONG)CharUpper((LPTSTR)(ULONG)(TUCHAR)LOWORD(wParam))));

    case WM_CREATE:
        hCPlWnd = hWnd;
        if (!CreatChildWindows ())
            return (LRESULT) -1L;
        break;

    case WM_DELETEITEM:
        // free memory for applets
        if (pCPlApp = SendAppMsg (CPL_STOP,
                                  ((LPDELETEITEMSTRUCT)lParam)->itemID))
        {
            LocalFree ((HANDLE)pCPlApp->pszFullName);
            LocalFree ((HANDLE)pCPlApp->pszName);
            LocalFree ((HANDLE)pCPlApp->pszInfo);
            if (pCPlApp->pszHelpFile)
                LocalFree ((HANDLE)pCPlApp->pszHelpFile);
        }

        if (--numApps == 0)
           FreeApps();
        break;

    case WM_CLOSE:
    case WM_ENDSESSION:
        SetCPlWndPos (hWnd);
        break;

    case WM_DESTROY:
        if (hCPlFont)
            DeleteObject (hCPlFont);
        if (hCPlBk)
            DeleteObject (hCPlBk);

        WinHelp (hWnd, szHelpFile, HELP_QUIT, 0L);
        PostQuitMessage (0);
        break;

    case WM_CTLCOLORLISTBOX:
        rgbBG = GetSysColor (COLOR_APPWORKSPACE);
        if ((WORD)GetRValue(rgbBG) + (WORD)GetGValue (rgbBG)
             + (WORD)GetBValue(rgbBG) > 383)
            rgbFG = RGB(0,0,0);
        else
            rgbFG = RGB(255,255,255);

        SetBkColor ((HDC)wParam, rgbBG);
        SetTextColor ((HDC)wParam, rgbFG);
        return ((LRESULT)hCPlBk);
        break;

    case WM_DRAWITEM:
        drwI = (LPDRAWITEMSTRUCT)lParam;

        //
        // Make sure itemData field is not equal to -1 or 0, this
        // is to work around USER problem where they will send us
        // an DRAWITEMSTRUCT.itemID value of -1, -2, etc. if someone
        // is holding the LEFTMOUSEBUTTON down, moves outside of the
        // window and then releases the button. (See WM_COMMAND, above)
        //
        // The itemData value is a pointer to our local NTCPL data struct
        //

        if (((int)drwI->itemID >= 0)                       &&
            (pCPlMod = ((LPNTCPL)drwI->itemData)->pCPlMod)  &&
            (pCPlApp = pCPlMod->pCPlApps + ((LPNTCPL)drwI->itemData)->iApplet))
        {
            // Draw the icon if we must
            if (drwI->itemAction & ODA_DRAWENTIRE)
                DrawIcon (drwI->hDC, (drwI->rcItem).left + iIconX,
                          (drwI->rcItem).top + iIconY, pCPlApp->hIcon);

            // Draw the Name text under the icon
            // dx = (iWidth - pCPlApp->iNameW) / 2;

            if (drwI->itemState & ODS_SELECTED)
            {
                rgbBG = SetBkColor (drwI->hDC, GetSysColor(COLOR_ACTIVECAPTION));
                rgbFG = SetTextColor (drwI->hDC, GetSysColor(COLOR_CAPTIONTEXT));

            }
            else    // erase underneath text with background color
            {
                CopyRect (&Rect, &drwI->rcItem);
                Rect.top += iIconH + iIconY;
                hBrush = SelectObject (drwI->hDC, hCPlBk);
                FillRect (drwI->hDC, &Rect, hCPlBk);
                SelectObject (drwI->hDC, hBrush);
                iBkMode = SetBkMode (drwI->hDC, TRANSPARENT);
            }

            hFont = SelectObject (drwI->hDC, hCPlFont);
            CopyRect (&Rect, &drwI->rcItem);
            Rect.top += iIconH + iIconY;
            DrawText (drwI->hDC,pCPlApp->pszName,-1, (LPRECT)&Rect, DT_CENTER|DT_NOCLIP|DT_SINGLELINE);
            SelectObject (drwI->hDC, hFont);

            // Draw the Info text at the bottom of the window
            if (drwI->itemState & ODS_SELECTED)
            {
                SetBkColor (drwI->hDC, rgbBG);
                SetTextColor (drwI->hDC, rgbFG);
                SetWindowText (hCPlTB,pCPlApp->pszInfo); //!!!
            }
            else
                SetBkMode(drwI->hDC, iBkMode);
        }

        return((LRESULT)TRUE);

    case WM_MEASUREITEM:
        measI = ((LPMEASUREITEMSTRUCT)lParam);
        measI->itemHeight = ItemHeight;
        return((LRESULT)TRUE);

    case WM_MOVE:
        if (!IsIconic(hWnd) && !IsZoomed(hWnd))
             GetWindowRect (hWnd, &CPlRect);
        break;

    case WM_SETFOCUS:
        SetFocus (hCPlLB);
        break;

    case WM_SIZE:
        if (wParam != SIZEICONIC)
        {
            int cx, cy;
            cx = GetSystemMetrics (SM_CXBORDER);
            cy = GetSystemMetrics (SM_CYBORDER);

            dx = LOWORD(lParam);
            dy = HIWORD(lParam);

            //    This code assumes that the text window has a border
            //    and the listbox window does too, and offsets the two
            //    windows accordingly.  The listbox is given a border,
            //    then inflated so that the border is hidden by the
            //    surrounding window parts; this avoids the common but
            //    ugly double-border-around-scrollbar syndrome.
            //
            //    edh, 24-Sep-91.

            GetClientRect (hCPlTB, &Rect);
            MoveWindow (hCPlLB, -cx, -cy, dx+cx+cx, dy-Rect.bottom+cy, TRUE);
            MoveWindow (hCPlTB, -cx, dy-Rect.bottom-cy, dx+cx+cx, Rect.bottom+cy+cy, TRUE);

            if (wParam != SIZEFULLSCREEN)
                 GetWindowRect (hWnd, &CPlRect);
        }
        break;


    case WM_SYSCOLORCHANGE:
        if (hCPlBk)
            DeleteObject (hCPlBk);

        hCPlBk = CreateSolidBrush (GetSysColor (COLOR_APPWORKSPACE));
        break;

    case WM_MENUSELECT:
        if (lParam)
        {
            // Save the menu the user selected
            wMenuID = LOWORD(wParam);
            dwMenuBits = (DWORD) HIWORD(wParam);
        }
        break;

    default:
        if (Message == wHelpMessage)
        {
            if (hSetup)
            {
                // trap F1 for setup and pass it on

                PostMessage(hSetup, WM_COMMAND, 3, 0L);

            }
            else if (wParam == MSGF_MENU)
            {
                // Get outta menu mode if help for a menu item

                if (wMenuID && HIWORD(dwMenuBits))
                {
                    WORD m = wMenuID;    // save
                    DWORD mb = dwMenuBits;

                    SendMessage(hWnd, WM_CANCELMODE, 0, 0L);

                    wMenuID   = m;        // restore
                    dwMenuBits = mb;
                }

                if (!(LOWORD(dwMenuBits) & MF_POPUP))
                {
                    dwContext = 0L;
                    pHelpFile = NULL;

                    if (LOWORD(dwMenuBits) & MF_SYSMENU)
                    {
                        dwContext = IDH_SYSMENU;
                    }
                    else if ((wMenuID >= MENU_SETTINGS) &&
                             (wMenuID < (WORD)(MENU_SETTINGS + numApps)))
                    {
                        pCPlApp = SendAppMsg(CPL_SELECT, wMenuID - MENU_SETTINGS);
                        dwContext = pCPlApp->dwContext;
                        pHelpFile = pCPlApp->pszHelpFile;
                    }
                    else
                    {
                        dwContext = wMenuID + IDH_HELPFIRST;
                    }

                    CPHelp(hWnd, pHelpFile, dwContext);
                  }

            }
            else if (wParam == MSGF_DIALOGBOX)
            {
                // let dialog box deal with it

                PostMessage(GetRealParent((HWND)lParam), wHelpMessage, 0, 0L);
            }
        }
        else
            return (LRESULT)DefWindowProc(hWnd, Message, wParam, lParam);
        break;
    }

    return DefWindowProc(hWnd, Message, wParam, lParam);
}

LRESULT APIENTRY fnText (HWND hwnd, UINT message, WPARAM wParam, LONG lParam)
{
    static HFONT hStatFont = NULL;

    TCHAR ach[128];

    switch (message)
    {
        case WM_CREATE:
        {
          HDC        hDC;
          HFONT      hOldFont;
          TEXTMETRIC tm;

            hDC = GetDC (hwnd);

#ifdef JAPAN    /* V-KeijiY  July.16.1992 */
            hStatFont = CreateFont(-10 * GetDeviceCaps(hDC, LOGPIXELSY) / 72,
                                   0, 0, 0, 400, 0, 0, 0, ( ANSI_CHARSET | SHIFTJIS_CHARSET ),
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                   VARIABLE_PITCH | FF_SWISS, TEXT("System"));
#else
#ifdef UNICODE
          /* initialize the Unicode font */
            hStatFont = CreateFont (-10 * GetDeviceCaps(hDC, LOGPIXELSY) / 72,
                                    0, 0, 0, 400, 0, 0, 0, ANSI_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                                    TEXT("MS Shell Dlg"));
#else
            hStatFont = CreateFont (-10 * GetDeviceCaps(hDC, LOGPIXELSY) / 72,
                                    0, 0, 0, 400, 0, 0, 0, ANSI_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
                                    TEXT("MS Shell Dlg"));
#endif
#endif

            if (hStatFont)
                hOldFont = SelectObject (hDC, hStatFont);
            else
                hOldFont = NULL;

            GetTextMetrics (hDC, &tm);
            SetWindowPos (hwnd, NULL, 0, 0, 0, tm.tmHeight+6*GetSystemMetrics(SM_CYBORDER)+2,
                          SWP_NOZORDER | SWP_NOMOVE);

            SelectObject (hDC, hOldFont);

            ReleaseDC (hwnd, hDC);
            break;
        }

        case WM_DESTROY:
            if (hStatFont)
               DeleteObject (hStatFont);
            break;

        case WM_SETTEXT:
            /* Do nothing if the text does not change
             * This prevents flicker on PAINT messages
             */
            GetWindowText (hwnd, ach, CharSizeOf(ach));
            if (lstrcmp (ach, (LPTSTR)lParam))
            {
                DefWindowProc (hwnd, message, wParam, lParam);
                InvalidateRect (hwnd,NULL,FALSE);
                UpdateWindow (hwnd);
            }
            return 0L;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT   rc;
            int    len, nxBorder, nyBorder;
            HFONT  hOldFont = NULL;
            HBRUSH hBrush, hOldBrush;
            int    nOldMode;
            DWORD  dwOldColor;

            BeginPaint (hwnd, &ps);

            GetClientRect (hwnd,&rc);

            nxBorder = GetSystemMetrics (SM_CXBORDER);
            rc.left += 9 * nxBorder;
            rc.right -= 9 * nxBorder;

            nyBorder = GetSystemMetrics (SM_CYBORDER);
            rc.top += 3 * nyBorder;
            rc.bottom -= 3 * nyBorder;

            if (hBrush=CreateSolidBrush (GetSysColor (COLOR_BTNHIGHLIGHT)))
            {
                if (hOldBrush = SelectObject (ps.hdc, hBrush))
                {
                    PatBlt (ps.hdc, rc.left-nxBorder, rc.bottom,
                            rc.right-rc.left+2*nxBorder, nyBorder, PATCOPY);
                    PatBlt (ps.hdc, rc.right, rc.top-nyBorder,
                            nxBorder, rc.bottom-rc.top+2*nxBorder, PATCOPY);
                    SelectObject (ps.hdc, hOldBrush);
                }
                DeleteObject (hBrush);
            }

            if (hBrush = CreateSolidBrush (GetSysColor(COLOR_BTNSHADOW)))
            {
                if (hOldBrush = SelectObject (ps.hdc, hBrush))
                {
                    PatBlt (ps.hdc, rc.left-nxBorder, rc.top-nyBorder,
                            rc.right-rc.left+nxBorder, nyBorder, PATCOPY);
                    PatBlt (ps.hdc, rc.left-nxBorder, rc.top-nyBorder,
                            nxBorder, rc.bottom-rc.top+nxBorder, PATCOPY);
                    SelectObject (ps.hdc, hOldBrush);
                }
                DeleteObject (hBrush);
            }

            if (hBrush = CreateSolidBrush (GetSysColor(COLOR_BTNFACE)))
            {
                if (hOldBrush = SelectObject (ps.hdc, hBrush))
                {
                    PatBlt (ps.hdc, rc.left, rc.top, rc.right-rc.left,
                            rc.bottom-rc.top, PATCOPY);
                    SelectObject (ps.hdc, hOldBrush);
                }
                DeleteObject (hBrush);
            }

            len = GetWindowText (hwnd, ach, CharSizeOf(ach));
            nOldMode = SetBkMode (ps.hdc, TRANSPARENT);
            dwOldColor = SetTextColor (ps.hdc, GetSysColor(COLOR_BTNTEXT));

            if (hStatFont)
                hOldFont = SelectObject (ps.hdc, hStatFont);
            ExtTextOut (ps.hdc, rc.left + 2 * nxBorder, rc.top, ETO_CLIPPED,
                        &rc, ach, len, NULL);
            if (hOldFont)
                SelectObject (ps.hdc, hOldFont);

            SetBkMode (ps.hdc, nOldMode);
            SetTextColor (ps.hdc, dwOldColor);

            EndPaint (hwnd, &ps);
            return 0L;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
//
// CONTROL.EXE [FILE.CPL] [Applet Name] [Setup]
// CONTROL.EXE /Cache
//
// if given a CPL file to load and an optional applet name we
// will load that CPL (and not create the main window) and then
// run the appropriate applet.  If the applet name is not found
// (or not specified) we will run the first applet in the .CPL file
//
// =============== May 4, 1993 [stevecat] ===============================
//
// A third possible argument was added to satisfy a last minute NT
// Setup request. If "Setup" is exactly the third argument after
// Control.exe on command line, I will send a new message "CPL_SETUP"
// to the applet before trying to run the command line applet.  In
// this case, the second argument must be present, and must be a
// valid applet name within the CPL file.
//
// ======================================================================
//
// if no parameters are given we will run normally, if another instance
// is already running (minimized or not) we will bring it to the
// foreground.  If given an applet name we will activate that applet
//
// =============== Sept. 21, 1993 [stevecat] ============================
//
// Added new command line arg to force Control Panel module cache to
// be rebuilt.
//
//
/////////////////////////////////////////////////////////////////////////////

__cdecl main (int argc, char *argv[])
{
    WNDCLASS   rClass;
    MSG        rMsg;
    int        CPlWndPos[4];
    HACCEL     hAccel;
    HANDLE     hPrevInstance;
    HANDLE     hmutexInit = NULL;
    int        len;
    TCHAR    **v_U;
    STARTUPINFO si;

    extern lbInit(HANDLE);


    ///////////////////////////////////////////////////////////////////////
    //  Immediately create a simple blocking MUTEX to stop multiple
    //  instances of the Control Panel being launched before we can
    //  detect it thru the USER FindWindow api.
    ///////////////////////////////////////////////////////////////////////

    hmutexInit = CreateMutex (NULL, FALSE, TEXT("Control Panel Init"));

    WaitForSingleObject (hmutexInit, INFINITE);

    //
    //  Get User's logon name to use in MUTEX object naming for uniqueness
    //

    len = MAX_PATH;

    GetUserName (szUsername, &len);

    //
    //  Get UNICODE command line arguments
    //

    v_U = CommandLineToArgvW (GetCommandLine(), &argc);

    hCPlInst = GetModuleHandle (NULL);

    if (!GetCPLStrings())
        return(FALSE);

    MakeWinPath(aszControlIniPath, aszControlIni);

    // Load in all key accelerators and setup help messages and
    // hooks so they are available for when only one applet is
    // running alone.

    hAccel = LoadAccelerators (hCPlInst, (LPTSTR) MAKEINTRESOURCE(MMCPL));
    wHelpMessage = RegisterWindowMessage (TEXT("ShellHelp"));
    hhkMsgFilter = SetWindowsHook (WH_MSGFILTER, (HOOKPROC) MessageFilter);

    //
    // Check Command line for Cache argument
    //

    if ((argc > 1) && (!lstrcmpi (TEXT("/Cache"), v_U[1])))
    {
        //  Force Cache to be rebuilt
        ClearCacheValid();

        //  No other arguments accepted with this one
        argc = 1;
    }

    ////////////////////////////////////////////////////////////////////////
    //  This is a sneaky thing I am doing.  If someone calls the control
    //  panel with any argument, I check the cache and build it if needed.
    //  This will get invoked during setup (timezone) and will create the
    //  cached entries at setup time.  Then, the first time the User starts
    //  the Control Panel after they logon, it will just seem very fast.
    ////////////////////////////////////////////////////////////////////////

    if (argc > 1)
    {
        BuildCache ();

        if (!lstrcmpi (TEXT("/Build"), v_U[1]))
            return (TRUE);
    }

    // Check Command line for Setup argument
    if ((argc >= 4) && (!lstrcmpi (TEXT("Setup"), v_U[3])))
        bSetup = TRUE;

    //
    //  RunCommandLineApplet releases MUTEX as necessary
    //

    if (argc > 1 && RunCommandLineApplet (argc, v_U[1], v_U[2], hmutexInit))
        return (TRUE);

    hPrevInstance = FindWindow (szClass, szAppName);

    if (hPrevInstance != NULL)
    {
        //  Free MUTEX here since we will exit along this code path
        if (hmutexInit)
        {
            ReleaseMutex (hmutexInit);
            CloseHandle (hmutexInit);
        }

        //  If there was a previous instance of the Control Panel active,
        //  just bring it to the foreground (or its' last active child window).

        if (IsIconic(hPrevInstance))
        {
            ShowWindow (hPrevInstance, SW_RESTORE);
            SetForegroundWindow (hPrevInstance);
        }
        else
            SetForegroundWindow (GetLastActivePopup(hPrevInstance));

        if (IsWindowEnabled (hPrevInstance))
        {
            if (argc > 1)
                SendMessage (hPrevInstance, WM_CPL_LAUNCHEX,
                             GetCurrentProcessId(), (LONG) v_U[1]);
            return (TRUE);
        }

        return (FALSE);
    }
    else
    {
        //
        //  We haven't registered our window classes yet
        //

// DoRegisterStuff:

        lbInit(hCPlInst);

// TODO: [stevecat] TEST for FULL DRAG redraw - remove Class styles
        rClass.style            = 0;     //  CS_HREDRAW | CS_VREDRAW;
        rClass.lpfnWndProc      = CPlWndProc;
        rClass.cbClsExtra       = 0;
        rClass.cbWndExtra       = 0;
        rClass.hInstance        = hCPlInst;
        rClass.hIcon            = LoadIcon (hCPlInst, (LPTSTR) MAKEINTRESOURCE(MMCPL));
        rClass.hCursor          = LoadCursor (NULL, IDC_ARROW);
        rClass.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE+1);
        rClass.lpszMenuName     = (LPTSTR) MAKEINTRESOURCE(MMCPL);
        rClass.lpszClassName    = szClass;

        if (!RegisterClass(&rClass))
            return (FALSE);

        rClass.style            = CS_HREDRAW | CS_VREDRAW;
        rClass.lpfnWndProc      = fnText;
        rClass.cbClsExtra       = 0;
        rClass.cbWndExtra       = 0;
        rClass.hInstance        = hCPlInst;
        rClass.hIcon            = NULL;
        rClass.hCursor          = LoadCursor (NULL, IDC_ARROW);
        rClass.hbrBackground    = (HBRUSH)(COLOR_BTNFACE+1);
        rClass.lpszMenuName     = NULL;
        rClass.lpszClassName    = szText;

        if (!RegisterClass (&rClass))
            return FALSE;

        GetCPlWndPos (CPlWndPos);

        //
        //  How big are icons around here anyway?
        //

        iIconH = GetSystemMetrics(SM_CYICON);
        iIconW = GetSystemMetrics(SM_CXICON);

        hCPlWnd = CreateWindow(szClass, szAppName,
                      WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,  // Style bits
                      CPlWndPos[0],         // x, y position
                      CPlWndPos[1],
                      CPlWndPos[2],         // width/height
                      CPlWndPos[3],
                      (HWND)NULL,           // no parent window
                      (HMENU)NULL,          // use class menu
                      (HANDLE)NULL,         // window instance
                      (LPVOID)NULL           // no params to pass on
                      );

        if (!hCPlWnd)
            return FALSE;

        //
        //  ShowWindow based on flags, with default however.
        //

        GetStartupInfo (&si);

        len = (si.dwFlags & STARTF_USESHOWWINDOW) ? (int) si.wShowWindow :
                                                    SW_SHOWNORMAL;

        ShowWindow (hCPlWnd, len);


        if (!LoadAndSizeApplets ())
        {
            MessageBox( hCPlWnd, szErrNoApps, szAppName,
                                MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL );
            rMsg.wParam = 1;
            goto DeregisterPen;
        }
    }

    //
    //  Now we can free MUTEX and let other instances run since
    //  we have created our main window
    //

    if (hmutexInit)
    {
        ReleaseMutex (hmutexInit);
        CloseHandle (hmutexInit);
    }

    //
    // bring up any app on the cmd line
    //

    if (argc > 1)
        PostMessage(hCPlWnd, WM_CPL_LAUNCH, (DWORD)hCPlWnd, (LONG) v_U[1]);

    while (GetMessage(&rMsg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(hCPlWnd, hAccel, &rMsg))
        {
            TranslateMessage(&rMsg);
            DispatchMessage(&rMsg);
        }
    }

DeregisterPen:

    return rMsg.wParam;
}

void CPHelp(HWND hwnd, LPTSTR pszHelpFile, DWORD dwContext)
{
    WORD w;

    // no help file?  use our default help file
    if (!pszHelpFile)
        pszHelpFile = szHelpFile;

    if (dwContext == 0L)
        w = HELP_INDEX;
    else
        w = HELP_CONTEXT;

    WinHelp(hwnd, pszHelpFile, w, dwContext);
}


HWND GetRealParent(HWND hwnd)
{
    //
    // Run up the parent chain until you find a hwnd
    // that doesn't have WS_CHILD set
    //

    while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
        hwnd = (HWND) GetWindowLong (hwnd, GWL_HWNDPARENT);

    return hwnd;
}


int APIENTRY MessageFilter(int nCode, DWORD wParam, LPMSG lpMsg)
{
    if (nCode < 0)
        goto DefHook;

    if (nCode == MSGF_MENU)
    {
        if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1)
        {
            // Window of menu we want help for is in loword of lParam.

            // We don't want to process messages for somebody else's menu
            if (IsWindowEnabled(hCPlWnd))
            {
                PostMessage(hCPlWnd, wHelpMessage, MSGF_MENU, (LONG)lpMsg->hwnd);
                return 1;
            }
        }
    }
    else if (nCode == MSGF_DIALOGBOX)
    {
        if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1)
        {
            // Dialog box we want help for is in loword of lParam

//            PostMessage (GetParent(lpMsg->hwnd), wHelpMessage, MSGF_DIALOGBOX, (LPARAM)lpMsg->hwnd);
            PostMessage (hCPlWnd, wHelpMessage, MSGF_DIALOGBOX, (LPARAM)lpMsg->hwnd);

            return 1;
        }
    }
    else
    {

DefHook:
        return((int)DefHookProc(nCode, wParam, (LONG)lpMsg, &hhkMsgFilter));
    }

  return 0;
}
