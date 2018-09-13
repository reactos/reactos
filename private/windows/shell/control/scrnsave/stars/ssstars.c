/*

STARS.C

Starfield simulator screensaver.

  History:
       6/17/91        stevecat    ported to NT Windows
       2/10/92        stevecat    snapped to latest ported to NT Windows
*/

#include <windows.h>
#include <scrnsave.h>
#include <commctrl.h>
#include "stars.dlg"
#include "strings.h"
#include "uniconv.h"


#define SCOPE       256
#define MAXWARP     10              // Maximum warp speed
#define MINWARP     0               // Minimum warp speed
#define CLICKRANGE (MAXWARP-MINWARP)// Range for WarpSpeed scroll bar
#define MINSTARS    10              // Minimum number of stars in field
#define MAXSTARS    200             // Maximum number of stars in field
#define WARPFACTOR  10              // Warp Factor 10 Mr. Sulu!
#define SIZE        64
#define DEF_DENSITY 25              // Default number of stars in field
#define RAND(x)     ((rand() % (x))+1)
#define ZRAND(x)    (rand() % (x))
#define MINTIMERSPEED 50

VOID CreateStar            (WORD wIndex);
LONG GetDlgItemLong        (HWND hDlg, WORD wID, BOOL *pfTranslated, BOOL fSigned);
VOID GetIniEntries         (VOID);
LONG GetPrivateProfileLong (LPTSTR pszApp, LPTSTR pszKey, LONG lDefault);
WORD rand                  (VOID);
VOID srand                 (DWORD dwSeed);

DWORD dwRand;                           // Current random seed

TCHAR  szWarpSpeed [] = TEXT("WarpSpeed");     // .INI WarpSpeed key

TCHAR  szDensity [] = TEXT("Density");         // .INI Density key

LONG  nX[MAXSTARS],
      nY[MAXSTARS],
      nZ[MAXSTARS];
WORD  wXScreen,
      wYScreen,
      wX2Screen,
      wY2Screen;
WORD  wWarpSpeed,                       // Global WarpSpeed value
      wDensity;                         // Global starfield density value

//
// Help IDs
//
DWORD aStarsDlgHelpIds[] = {
    ((DWORD) -1), ((DWORD) -1),
    ID_SPEED_SLOW,              IDH_DISPLAY_SCREENSAVER_STARFIELD_WARP,
    ID_SPEED_FAST,              IDH_DISPLAY_SCREENSAVER_STARFIELD_WARP,
    ID_SPEED,                   IDH_DISPLAY_SCREENSAVER_STARFIELD_WARP,
    ID_DENSITY_LABEL,           IDH_DISPLAY_SCREENSAVER_STARFIELD_DENSITY,
    ID_DENSITY,                 IDH_DISPLAY_SCREENSAVER_STARFIELD_DENSITY,
    ID_DENSITYARROW,            IDH_DISPLAY_SCREENSAVER_STARFIELD_DENSITY,
    0,0
};

/* This is the main window procedure to be used when the screen saver is
    activated in a screen saver mode ( as opposed to configure mode ).  This
    function must be declared as an EXPORT in the EXPORTS section of the
    DEFinition file... */

LRESULT ScreenSaverProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT         rRect;
    WORD         wLoop;
    static UINT_PTR wTimer;
    static WORD  wWarp;
    static WORD  wTimerSet=MINTIMERSPEED;
    static WORD  wCurrentWarp;
    static int   nPassCount=0;
    int          nXTemp, nYTemp, nTemp;
    BOOL         fHyperSpace = TRUE;
    HDC          hDC;

    switch (message)
    {
    case WM_CREATE:
        /* Do anything that you need to do when you initialize the window
           here... */
        GetIniEntries ();
        srand (GetCurrentTime ());

        /* Make sure we use the entire virtual desktop size for multiple
           displays... */

        wXScreen = (WORD) ((LPCREATESTRUCT)lParam)->cx;
        wYScreen = (WORD) ((LPCREATESTRUCT)lParam)->cy;


        wX2Screen = wXScreen / 2;
        wY2Screen = wYScreen / 2;
        for (wLoop = 0; wLoop < wDensity; wLoop++)
            CreateStar (wLoop);
        wWarp = wWarpSpeed * WARPFACTOR + WARPFACTOR; // ZRAND (((wWarpSpeed)*WARPFACTOR)+1)+1;

        wTimer = SetTimer (hWnd, 1, wTimerSet, NULL);
        break;

    case WM_SIZE:
        wXScreen = LOWORD(lParam);
        wYScreen = HIWORD(lParam);
        break;


    case WM_TIMER:
    {
        MSG msg;

        hDC = GetDC (hWnd);
        /* Begin to loop through each star, accelerating so it seems that
           we are traversing the starfield... */
        for (wLoop = 0; wLoop < wDensity; wLoop++)
        {
            nXTemp = (int)((nX[wLoop] * (LONG)(SCOPE * WARPFACTOR))
                                                / nZ[wLoop]) + wX2Screen;
            nYTemp = (int)((nY[wLoop] * SCOPE * WARPFACTOR) / nZ[wLoop])
                                                     + wY2Screen;
            nTemp = (int)((SCOPE * WARPFACTOR - nZ[wLoop]) /
                                                    (SIZE * WARPFACTOR)) + 1;
            PatBlt (hDC, nXTemp, nYTemp, nTemp, nTemp, BLACKNESS);

            if (wCurrentWarp < wWarp)
                wCurrentWarp++;
            else if (wCurrentWarp > wWarp)
                wCurrentWarp--;

            nZ[wLoop] = max (0, (int)(nZ[wLoop] - wCurrentWarp));
            if (!nZ[wLoop])
                CreateStar (wLoop);

            nXTemp = (int)((nX[wLoop] * (LONG)(SCOPE * WARPFACTOR))
                                                    / nZ[wLoop]) + wX2Screen;
            nYTemp = (int)((nY[wLoop] * SCOPE * WARPFACTOR)
                                                    / nZ[wLoop]) + wY2Screen;
            if ((nXTemp < 0 || nYTemp < 0) ||
                (nXTemp > (int) wXScreen || nYTemp > (int) wYScreen))
            {
                CreateStar (wLoop);
                nXTemp = (int)((nX[wLoop] * (LONG)(SCOPE * WARPFACTOR))
                                                 / nZ[wLoop]) + wX2Screen;
                nYTemp = (int)((nY[wLoop] * SCOPE * WARPFACTOR)
                                                 / nZ[wLoop]) + wY2Screen;
            }
            nTemp = (int)((SCOPE * WARPFACTOR - nZ[wLoop]) /
                                                (SIZE * WARPFACTOR)) + 1;
            PatBlt (hDC, nXTemp, nYTemp, nTemp, nTemp, WHITENESS);
        }
        ReleaseDC (hWnd, hDC);

        if (PeekMessage(&msg, hWnd, WM_TIMER, WM_TIMER, PM_REMOVE))
        {
            // There is another WM_TIMER message in the queue.  We have
            // removed it, but now we want to adjust the timer a bit so
            // hopefully we won't get another WM_TIMER message before we
            // finish the screen update. (bug #8423)  TG:11/25/91

            wTimerSet += 10;
            SetTimer(hWnd, 1, wTimerSet, NULL);
            nPassCount = 0;
        }
        else
            ++nPassCount;

        if (nPassCount >= 100)
        {
            nPassCount = 0;
            wTimerSet -= 100;
            if ((short)wTimerSet < MINTIMERSPEED)
                wTimerSet = MINTIMERSPEED;
            SetTimer(hWnd, 1, wTimerSet, NULL);
        }
        break;
    }

    case WM_ERASEBKGND:
            /* If you want something put on the background, do it right here
                using wParam as a handle to a device context.  Remember to
                unrealize a brush if it is not a solid color.  If you do
                something here, you want to use the line:
                    return 0l;
                So the program knows not to take the default action. Otherwise
                just use:
                    break;
                */
        break;
        GetClientRect (hWnd, &rRect);
        FillRect ((HDC) wParam, &rRect, GetStockObject (GRAY_BRUSH));
        return 0l;

    case WM_DESTROY:
        /* Anything that needs to be deleted when the window is closed
                goes here... */
        if (wTimer)
            KillTimer (hWnd, wTimer);
        break;
    }
    /* Unless it is told otherwise, the program will take default actions... */
    return (DefScreenSaverProc (hWnd, message, wParam, lParam));
}


//***************************************************************************

BOOL ScreenSaverConfigureDialog (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL    fError;                         // Error flag

    UINT    wTemp;
    TCHAR   szTemp[20];                     // Temporary string buffer

    static WORD wPause, wScroll;
    static HWND hWarpSpeed,                 // window handle of Speed scrollbar
                hIDOK,                      // window handle of OK button
                hSetPassword,               // window handle of SetPassword button
                hDensity;                   // window handle of Density EditControl

    static WORD wIncScroll = 1;             // density spin button parameters

    static WORD wStartScroll = 1;
    static WORD wStartPause = 1;
    static WORD wMaxScroll = 10;
    static WORD wPauseScroll = 20;
    static LONG lMinScroll = MINSTARS;
    static LONG lMaxScroll = MAXSTARS;


    switch (message)
    {
    case WM_INITDIALOG:
        GetIniEntries ();
        hWarpSpeed = GetDlgItem (hDlg, ID_SPEED);
        hIDOK = GetDlgItem (hDlg, IDOK);
        hDensity = GetDlgItem (hDlg, ID_DENSITY);
        SendMessage (hDensity, EM_LIMITTEXT, 3, 0);

        SendDlgItemMessage( hDlg, ID_DENSITYARROW, UDM_SETBUDDY, (WPARAM)hDensity, 0);
        SendDlgItemMessage( hDlg, ID_DENSITYARROW, UDM_SETRANGE, 0, MAKELONG(lMaxScroll, lMinScroll));

        SetScrollRange (hWarpSpeed, SB_CTL, MINWARP, MAXWARP, FALSE);
        SetScrollPos (hWarpSpeed, SB_CTL, wWarpSpeed, TRUE);

        SetDlgItemInt (hDlg, ID_DENSITY, wDensity, FALSE);
        return TRUE;

    case WM_HSCROLL:
        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
        case SB_PAGEUP:
            --wWarpSpeed;
            break;

        case SB_LINEDOWN:
        case SB_PAGEDOWN:
            ++wWarpSpeed;
            break;

        case SB_THUMBPOSITION:
            wWarpSpeed = HIWORD (wParam);
            break;

        case SB_TOP:
            wWarpSpeed = MINWARP;
            break;

        case SB_BOTTOM:
            wWarpSpeed = MAXWARP;
            break;

        case SB_THUMBTRACK:
        case SB_ENDSCROLL:
            return TRUE;
            break;
        }
        if ((int)((short)wWarpSpeed) <= MINWARP)
            wWarpSpeed = MINWARP;
        if ((int)wWarpSpeed >= MAXWARP)
            wWarpSpeed = MAXWARP;

        SetScrollPos ((HWND) lParam, SB_CTL, wWarpSpeed, TRUE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_DENSITY:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                wTemp = GetDlgItemInt (hDlg, ID_DENSITY, &fError, FALSE);
                fError = ((wTemp <= MAXSTARS) && (wTemp >= MINSTARS));
                EnableWindow (GetDlgItem (hDlg, ID_DENSITYARROW), fError);
                EnableWindow (GetDlgItem (hDlg, IDOK), fError);
            }
            break;

        case IDOK:
            wTemp = GetDlgItemInt (hDlg, ID_DENSITY, &fError, FALSE);
            wsprintf (szTemp, TEXT("%d"), wTemp);
            WritePrivateProfileString (szAppName, szDensity, szTemp, szIniFile);
            wsprintf (szTemp, TEXT("%d"), wWarpSpeed);
            WritePrivateProfileString (szAppName, szWarpSpeed, szTemp, szIniFile);

        case IDCANCEL:
            EndDialog (hDlg, LOWORD(wParam) == IDOK);
            return TRUE;

        }
        break;

    case WM_HELP: // F1
        WinHelp(
            (HWND) ((LPHELPINFO) lParam)->hItemHandle,
            szHelpFile,
            HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aStarsDlgHelpIds
        );
        break;

    case WM_CONTEXTMENU:  // right mouse click
        WinHelp(
            (HWND) wParam,
            szHelpFile,
            HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aStarsDlgHelpIds
        );
        break;

    default:
        break;
    }
    return FALSE;
}


/* This procedure is called right before the dialog box above is created in
   order to register any child windows that are custom controls.  If no
   custom controls need to be registered, then simply return TRUE.
   Otherwise, register the child controls however is convenient... */

BOOL RegisterDialogClasses (HANDLE hInst)
{
    InitCommonControls();

    return TRUE;
}

VOID srand (DWORD dwSeed)
{
    dwRand = dwSeed;
}

WORD rand (VOID)
{
    dwRand = dwRand * 214013L + 2531011L;
    return (WORD)((dwRand >> 16) & 0xffff);
}

VOID CreateStar (WORD wIndex)
{
    nX[wIndex] = wXScreen ? (LONG)((int)(ZRAND (wXScreen)) - (int)wX2Screen) : 0;
    nY[wIndex] = wXScreen ? (LONG)((int)(ZRAND (wYScreen)) - (int)wY2Screen) : 0;
    nZ[wIndex] = SCOPE * WARPFACTOR;
}

LONG GetDlgItemLong (HWND hDlg, WORD wID, BOOL *pfTranslated, BOOL fSigned)
{
    TCHAR szTemp[20];
    LPTSTR pszTemp;
    LONG lTemp = 0l;
    BOOL fNegative;

    if (!GetDlgItemText (hDlg, wID, szTemp, CharSizeOf(szTemp)))
        goto GetDlgItemLongError;

    szTemp[19] = TEXT('\0');
    pszTemp = szTemp;
    while (*pszTemp == TEXT(' ') || *pszTemp == TEXT('\t'))
        pszTemp++;
    if ((!fSigned && *pszTemp == TEXT('-')) || !*pszTemp)
        goto GetDlgItemLongError;
    fNegative = (*pszTemp == TEXT('-')) ? TRUE : FALSE;
    while (*pszTemp >= TEXT('0') && *pszTemp <= TEXT('9'))
        lTemp = lTemp * 10l + (LONG)(*(pszTemp++) - TEXT('0'));
    if (*pszTemp)
        goto GetDlgItemLongError;
    if (fNegative)
        lTemp *= -1;
    *pfTranslated = TRUE;
    return lTemp;

GetDlgItemLongError:
    *pfTranslated = FALSE;
    return 0l;
}


LONG GetPrivateProfileLong (LPTSTR pszApp, LPTSTR pszKey, LONG lDefault)
{
    LONG    lTemp = 0l;
    TCHAR    szTemp[20];
    LPTSTR pszTemp;

    if (!GetPrivateProfileString (pszApp, pszKey, TEXT(""), szTemp, CharSizeOf(szTemp), szIniFile))
        goto GetProfileLongError;

    szTemp[19] = TEXT('\0');
    pszTemp = szTemp;
    while (*pszTemp >= TEXT('0') && *pszTemp <= TEXT('9'))
        lTemp = lTemp * 10l + (LONG)(*(pszTemp++) - TEXT('0'));
    if (*pszTemp)
        goto GetProfileLongError;
    return lTemp;

GetProfileLongError:
    return lDefault;
}


VOID GetIniEntries (VOID)
{
    LoadString (hMainInstance, idsName, szName, CharSizeOf(szName));
    LoadString (hMainInstance, idsAppName, szAppName, CharSizeOf(szAppName));

    //Load Common Strings from stringtable...
    LoadString (hMainInstance, idsIniFile, szIniFile, CharSizeOf(szIniFile));
    LoadString (hMainInstance, idsScreenSaver, szScreenSaver, CharSizeOf(szScreenSaver));
    LoadString (hMainInstance, idsHelpFile, szHelpFile, CharSizeOf(szHelpFile));
    LoadString (hMainInstance, idsNoHelpMemory, szNoHelpMemory, CharSizeOf(szNoHelpMemory));

    wWarpSpeed = (WORD) GetPrivateProfileInt (szAppName, szWarpSpeed, MINWARP + ((MAXWARP - MINWARP) / 2), szIniFile);
    if (wWarpSpeed > MAXWARP)
        wWarpSpeed = MINWARP + ((MAXWARP - MINWARP) / 2);

    wDensity = (WORD) GetPrivateProfileInt (szAppName, szDensity, DEF_DENSITY, szIniFile);
    if (wDensity > MAXSTARS)
        wDensity = MAXSTARS;
    if (wDensity < MINSTARS)
        wDensity = MINSTARS;
}
