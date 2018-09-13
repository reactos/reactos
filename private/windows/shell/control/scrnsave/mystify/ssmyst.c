/*  MYSTIFY.C
**
**  Copyright (C) Microsoft, 1991, All Rights Reserved.
**
**  Screensaver Control Panel Applet.  This type creates one or two polygons
**  which bounce around the screen.
**
**  History:
**       6/17/91        stevecat    ported to NT Windows
**       2/10/92        stevecat    snapped to latest ported to NT Windows
*/

#define  OEMRESOURCE
#include <windows.h>
#include <commctrl.h>
#include <scrnsave.h>
#include "mystify.dlg"
#include "strings.h"
#include "uniconv.h"


// void  SetFields (HWND, WORD);

DWORD AdjustColor      (DWORD dwSrc, DWORD dwDest, int nInc, int nCnt);
LONG  AppOwnerDraw     (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
WORD  AtoI             (LPTSTR lpszConvert);
BOOL  DrawBitmap       (HDC hdc, int x, int y, HBITMAP hbm, DWORD rop);
VOID  DrawPolygon      (HDC hDC, HPEN hPen, WORD wPolygon, WORD wLine);
VOID  FillR            (HDC hdc, LPRECT prc, DWORD rgb);
VOID  FrameR           (HDC hdc, LPRECT prc, DWORD rgb, int iFrame);
DWORD GenerateColor    (VOID);
WORD  GenerateVelocity (VOID);
VOID  GetFields        (VOID);
DWORD GetProfileRgb    (LPTSTR szApp, LPTSTR szItem, DWORD rgb);
VOID  PatB             (HDC hdc, int x, int y, int dx, int dy, DWORD rgb);
WORD  rand             (VOID);
VOID  ShadeWindows     (HWND hDlg, WORD wPoly, WORD wPolygon);
VOID  srand            (DWORD dwSeed);

#define RAND(x)  ((rand () % (x))+1)
#define ZRAND(x) (rand () % (x))

#define rgbBlack        RGB (0,0,0)
#define rgbWhite        RGB (255,255,255)
#define rgbGrey         RGB (128,128,128)
#define rgbMenu         GetSysColor (COLOR_MENU)
#define rgbMenuText     GetSysColor (COLOR_MENUTEXT)

#define BUFFER_SIZE     20
#define BUFFER2_SIZE    20

#define NUMBER_POLYGONS  2
#define MAXXVEL         12
#define MAXYVEL         12
#define MAXLINES        15
#define NUMLINES         8

TCHAR  szClearName[] = TEXT("Clear Screen");   // ClearScreen .INI key

DWORD dwRand = 1L;                      // current Random seed

TCHAR  szBuffer[BUFFER_SIZE];            // temp buffer

TCHAR  szBuffer2[BUFFER2_SIZE];          // temp buffer

BOOL  fOn[NUMBER_POLYGONS];             // flag for Active status of polygon

BOOL  fWalk[NUMBER_POLYGONS];           // color usage for each polygon

WORD  wLines[NUMBER_POLYGONS];          // number of lines for each polygon

WORD  wNumDisplay[2];
WORD  wFreeEntry[NUMBER_POLYGONS];      // colors for each polygon

DWORD dwStartColor[NUMBER_POLYGONS];
DWORD dwEndColor[NUMBER_POLYGONS];
DWORD dwCurrentColor[NUMBER_POLYGONS];
DWORD dwDestColor[NUMBER_POLYGONS];
DWORD dwSrcColor[NUMBER_POLYGONS];
WORD  wIncColor[NUMBER_POLYGONS];
WORD  wCurInc[NUMBER_POLYGONS];
TCHAR cblogpalPal[(MAXLINES*NUMBER_POLYGONS+1)
                           *sizeof (PALETTEENTRY)+sizeof (LOGPALETTE)];
POINT ptBox[MAXLINES*NUMBER_POLYGONS][4]; // array for points used in polygons

LPLOGPALETTE   lplogpalPal;
LPPALETTEENTRY lppePal;
HPALETTE       hPalette;
BOOL fClearScreen;                      // Global flag for ClearScreen state

//
// Help IDs
//
DWORD aMystDlgHelpIds[] = {
    ((DWORD) -1),((DWORD) -1),
    ID_SHAPE_LABEL,             IDH_DISPLAY_SCREENSAVER_MYSTIFY_SHAPE,
    ID_SHAPE,                   IDH_DISPLAY_SCREENSAVER_MYSTIFY_SHAPE,
    ID_ACTIVE,                  IDH_DISPLAY_SCREENSAVER_MYSTIFY_ACTIVE,
    ID_LINES_LABEL,             IDH_DISPLAY_SCREENSAVER_MYSTIFY_LINES,
    ID_LINES,                   IDH_DISPLAY_SCREENSAVER_MYSTIFY_LINES,
    ID_LINESARROW,              IDH_DISPLAY_SCREENSAVER_MYSTIFY_LINES,
    ID_COLORGROUP,              ((DWORD) -1),
    ID_2COLORS,                 IDH_DISPLAY_SCREENSAVER_MYSTIFY_TWO_COLORS,
    ID_COLOR1,                  IDH_DISPLAY_SCREENSAVER_MYSTIFY_TWO_COLORS,
    ID_COLOR2,                  IDH_DISPLAY_SCREENSAVER_MYSTIFY_TWO_COLORS,
    ID_RANDOMCOLORS,            IDH_DISPLAY_SCREENSAVER_MYSTIFY_RANDOM_COLORS,
    ID_CLEARSCREEN,             IDH_DISPLAY_SCREENSAVER_MYSTIFY_CLEAR_SCREEN,
    0,0
};

LRESULT ScreenSaverProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static POINT ptChange[MAXLINES*NUMBER_POLYGONS][4];
    static UINT_PTR  wTimer;
    WORD         wLoop1;
    WORD         wLoop2;
    WORD         wMainLoop;
    static WORD  wScreenX;
    static WORD  wScreenY;
    HPEN         hPen;
    static HPEN  hErasePen;
    HDC          hDC;
    HPALETTE     hOldPal;

    switch (message)
    {
    // Things to do while setting up the window...
    case WM_CREATE:
        GetFields ();
        wTimer = SetTimer (hWnd, 1, 10, NULL);

        // Make sure we use the entire virtual desktop size for multiple
        // displays
        wScreenX = (WORD) ((LPCREATESTRUCT)lParam)->cx;
        wScreenY = (WORD) ((LPCREATESTRUCT)lParam)->cy;

        srand (GetCurrentTime ());
        for (wMainLoop = 0; wMainLoop < NUMBER_POLYGONS; wMainLoop++)
        {
            if (fOn[wMainLoop])
            {
                for (wLoop1 = 0; wLoop1 < 4; wLoop1++)
                {
                    ptBox[wMainLoop*MAXLINES][wLoop1].x = RAND (wScreenX) - 1;
                    ptBox[wMainLoop*MAXLINES][wLoop1].y = RAND (wScreenY) - 1;
                    if ((ptChange[wMainLoop*MAXLINES][wLoop1].x =
                        RAND (MAXXVEL * 2)) > MAXXVEL)
                        ptChange[wMainLoop*MAXLINES][wLoop1].x =
                            -(ptChange[wMainLoop*MAXLINES][wLoop1].x
                            -MAXXVEL);
                    if ((ptChange[wMainLoop*MAXLINES][wLoop1].y =
                        RAND (MAXYVEL * 2)) > MAXYVEL)
                        ptChange[wMainLoop*MAXLINES][wLoop1].y =
                            -(ptChange[wMainLoop*MAXLINES][wLoop1].y
                            -MAXYVEL);
                }
                wNumDisplay[wMainLoop] = 1;
                wFreeEntry[wMainLoop] = 0;
                wCurInc[wMainLoop] = 0;
                wIncColor[wMainLoop] = 0;
                if (fWalk[wMainLoop])
                    dwDestColor[wMainLoop] = GenerateColor ();
                else
                    dwDestColor[wMainLoop] = dwStartColor[wMainLoop];
            }
        }
        lppePal = (LPPALETTEENTRY)(cblogpalPal + 4);
        lplogpalPal = (LPLOGPALETTE)cblogpalPal;
        lplogpalPal->palVersion = 0x300;
        lplogpalPal->palNumEntries = MAXLINES * NUMBER_POLYGONS + 1;
        for (wLoop1 = 0; wLoop1 <= MAXLINES * NUMBER_POLYGONS; wLoop1++)
        {
            lplogpalPal->palPalEntry[wLoop1].peRed = 0;
            lplogpalPal->palPalEntry[wLoop1].peGreen = 0;
            lplogpalPal->palPalEntry[wLoop1].peBlue = 0;
            lplogpalPal->palPalEntry[wLoop1].peFlags = PC_RESERVED;
        }
        hErasePen = CreatePen (PS_SOLID, 1,
            PALETTEINDEX (MAXLINES * NUMBER_POLYGONS));
        hPalette = CreatePalette (lplogpalPal);
        break;

    case WM_SIZE:
        wScreenX = LOWORD(lParam);
        wScreenY = HIWORD(lParam);
        break;



    case WM_ERASEBKGND:
        if (fClearScreen)
            break;
        return 0l;

    case WM_TIMER:
        // Get the display context...
        hDC = GetDC (hWnd);
        if (hDC != NULL)
        {
            // Now that we have changed the palette, make sure that it
            // gets updated by first unrealizing, and then realizing...
            hOldPal = SelectPalette (hDC, hPalette, 0);
            RealizePalette (hDC);

            for (wMainLoop = 0; wMainLoop < NUMBER_POLYGONS; wMainLoop++)
            {
                // Check to see if the current loop is on...
                if (fOn[wMainLoop])
                {
                    // If our current count is the same as the final count,
                    // generate a new count...
                    if (wCurInc[wMainLoop] == wIncColor[wMainLoop])
                    {
                        // Set the count to zero...
                        wCurInc[wMainLoop] = 0;

                        // Set an new variant...
                        wIncColor[wMainLoop] = GenerateVelocity ();

                        // Set up the cycling colors...
                        dwSrcColor[wMainLoop] = dwDestColor[wMainLoop];

                        if (fWalk[wMainLoop])
                            dwDestColor[wMainLoop] = GenerateColor ();
                        else if (dwSrcColor[wMainLoop] == dwEndColor[wMainLoop])
                            dwDestColor[wMainLoop] = dwStartColor[wMainLoop];
                        else
                            dwDestColor[wMainLoop] = dwEndColor[wMainLoop];
                    }
                    else
                        wCurInc[wMainLoop]++;

                    // Now adjust the color between the starting and the
                    // ending values...
                    dwCurrentColor[wMainLoop] = AdjustColor (dwSrcColor
                        [wMainLoop], dwDestColor[wMainLoop], wIncColor
                        [wMainLoop], wCurInc[wMainLoop]);
                    wLoop2 = wFreeEntry[wMainLoop] + wMainLoop * MAXLINES;

                    lplogpalPal->palPalEntry[wLoop2].peRed =
                                        GetRValue (dwCurrentColor[wMainLoop]);
                    lplogpalPal->palPalEntry[wLoop2].peGreen =
                                        GetGValue (dwCurrentColor[wMainLoop]);
                    lplogpalPal->palPalEntry[wLoop2].peBlue =
                                        GetBValue (dwCurrentColor[wMainLoop]);
                    lplogpalPal->palPalEntry[wLoop2].peFlags = PC_RESERVED;

                    // Adjust the palette...
                    AnimatePalette (hPalette, wLoop2, 1,
                                        &lplogpalPal->palPalEntry[wLoop2]);
                }
            }

            // Now cycle through again...
            for (wMainLoop = 0; wMainLoop < NUMBER_POLYGONS; wMainLoop++)
            {
                if (fOn[wMainLoop])
                {
                    /* If we are currently displaying all of the lines, then
                       delete the last line... */
                    if (wNumDisplay[wMainLoop] == wLines[wMainLoop])
                        /* Erase the last line... */
                        DrawPolygon (hDC, hErasePen, wMainLoop,
                                     (WORD) (wNumDisplay[wMainLoop] - 1));

                    /* Starting with the last entry, make it equal to the
                       entry before it... until we reach the first
                       entry... */
                    for (wLoop1 = (wNumDisplay[wMainLoop] - 1); wLoop1; wLoop1--)
                    {
                        /* Copy the points in the polygon over... */
                        for (wLoop2 = 0; wLoop2 < 4; wLoop2++)
                        {
                            ptBox[wLoop1+wMainLoop*MAXLINES][wLoop2].x =
                                ptBox[wLoop1-1+wMainLoop*MAXLINES][wLoop2].x;
                            ptBox[wLoop1+wMainLoop*MAXLINES][wLoop2].y =
                                ptBox[wLoop1-1+wMainLoop*MAXLINES][wLoop2].y;
                            ptChange[wLoop1+wMainLoop*MAXLINES][wLoop2].x =
                                ptChange[wLoop1-1+wMainLoop*MAXLINES]
                                [wLoop2].x;
                            ptChange[wLoop1+wMainLoop*MAXLINES][wLoop2].y =
                                ptChange[wLoop1-1+wMainLoop*MAXLINES]
                                [wLoop2].y;
                        }
                    }

                    /* Seeing as we now have entry 0 the same as entry 1,
                       generate a new entry 0... */
                    for (wLoop1 = 0; wLoop1 < 4; wLoop1++)
                    {
                        ptBox[wMainLoop*MAXLINES][wLoop1].x +=
                            ptChange[wMainLoop*MAXLINES][wLoop1].x;
                        ptBox[wMainLoop*MAXLINES][wLoop1].y +=
                            ptChange[wMainLoop*MAXLINES][wLoop1].y;
                        if (ptBox[wMainLoop*MAXLINES][wLoop1].x >=
                            (int)wScreenX)
                        {
                            ptBox[wMainLoop*MAXLINES][wLoop1].x =
                                ptBox[wMainLoop*MAXLINES][wLoop1].x
                                -2 * (ptBox[wMainLoop*MAXLINES][wLoop1].x
                                -wScreenX + 1);
                            ptChange[wMainLoop*MAXLINES][wLoop1].x =
                                -RAND (MAXXVEL);
                        }
                        if ((int)ptBox[wMainLoop*MAXLINES][wLoop1].x < 0)
                        {
                            ptBox[wMainLoop*MAXLINES][wLoop1].x =
                                -ptBox[wMainLoop*MAXLINES][wLoop1].x;
                            ptChange[wMainLoop*MAXLINES][wLoop1].x =
                                RAND (MAXXVEL);
                        }
                        if (ptBox[wMainLoop*MAXLINES][wLoop1].y >=
                            (int)wScreenY)
                        {
                            ptBox[wMainLoop*MAXLINES][wLoop1].y =
                                ptBox[wMainLoop*MAXLINES][wLoop1].y - 2 *
                                (ptBox[wMainLoop*MAXLINES][wLoop1].y
                                -wScreenY + 1);
                            ptChange[wMainLoop*MAXLINES][wLoop1].y =
                                -RAND (MAXYVEL);
                        }
                        if ((int)ptBox[wMainLoop*MAXLINES][wLoop1].y < 0)
                        {
                            ptBox[wMainLoop*MAXLINES][wLoop1].y =
                                -ptBox[wMainLoop*MAXLINES][wLoop1].y;
                            ptChange[wMainLoop*MAXLINES][wLoop1].y =
                                RAND (MAXYVEL);
                        }
                    }

                    /* Now redraw the new line... */
                    wLoop2 = wFreeEntry[wMainLoop] + wMainLoop * MAXLINES;
                    hPen = CreatePen (PS_SOLID, 1, PALETTEINDEX (wLoop2));
                    DrawPolygon (hDC, hPen, wMainLoop, 0);
                    if (hPen)
                        DeleteObject (hPen);

                    /* Now, as we are finished with the entry in the
                       palette, increment it such that the next time
                       around, it points at the next position... */
                    if ((++wFreeEntry[wMainLoop]) == wLines[wMainLoop])
                        wFreeEntry[wMainLoop] = 0;

                    /* Now, if we are not at the maximum number of lines,
                       then increment towards there... */
                    if (wNumDisplay[wMainLoop] < wLines[wMainLoop])
                        wNumDisplay[wMainLoop]++;
                }
            }

            /* Reselect the old palette... */
            if (hOldPal)
                SelectPalette (hDC, hOldPal, FALSE);

            /* Release the display context... */
            ReleaseDC (hWnd, hDC);
        }
        break;

    case WM_DESTROY:
        if (wTimer)
            KillTimer (hWnd, 1);
        if (hPalette)
            DeleteObject (hPalette);
        if (hErasePen)
            DeleteObject (hErasePen);
        break;
    }
    return (DefScreenSaverProc (hWnd, message, wParam, lParam));
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

BOOL RegisterDialogClasses (HANDLE hInst)
{
    /* Register the custom controls.. */
    InitCommonControls();
    return TRUE;
}


//***************************************************************************

BOOL ScreenSaverConfigureDialog (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WORD         nPal;
    DWORD        dwTemp;
    HPALETTE     hPal;
    WORD         wLoop, wTemp;
    BOOL         fError;
    BYTE         byR, byG, byB;
    RECT         rDlgBox;
    TCHAR        szTemp[80];
    static HWND  hIDOK;
    static WORD  wPolygon;

    switch (message)
    {
    case WM_INITDIALOG:
        GetFields ();                // Read fields from CONTROL.INI

        GetWindowRect (hDlg, (LPRECT) & rDlgBox);
        hIDOK = GetDlgItem (hDlg, IDOK);

        // Set the global clear state...
        CheckDlgButton (hDlg, ID_CLEARSCREEN, fClearScreen);

        // Fill the boxes...
        for (wLoop = 0; wLoop < NUMBER_POLYGONS; wLoop++)
        {
            TCHAR   szBuffer[20];
            WORD    wTemp;

            LoadString (hMainInstance, idsPolygon, szTemp, CharSizeOf(szTemp));
            wsprintf (szBuffer, szTemp, wLoop + 1);
            wTemp = (WORD)SendDlgItemMessage (hDlg, ID_SHAPE, CB_ADDSTRING, 0,
                                              (LPARAM)szBuffer);
            SendDlgItemMessage (hDlg, ID_SHAPE, CB_SETITEMDATA, wTemp, wLoop);
        }

        hPal = GetStockObject (DEFAULT_PALETTE);
        GetObject (hPal, sizeof (WORD), (LPTSTR) &nPal);
        for (wTemp = 0; wTemp < nPal; wTemp++)
        {
            SendDlgItemMessage (hDlg, ID_COLOR1, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"a");
            SendDlgItemMessage (hDlg, ID_COLOR2, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"a");
        }

        // Start at the first polygon and let'r rip...
        SendDlgItemMessage (hDlg, ID_LINES, EM_LIMITTEXT, 2, 0l);
        SendDlgItemMessage( hDlg, ID_LINESARROW, UDM_SETRANGE, 0, MAKELONG(MAXLINES, 1));
        SendDlgItemMessage (hDlg, ID_SHAPE, CB_SETCURSEL, (wPolygon = 0), 0l);
        SendMessage (hDlg, WM_COMMAND, MAKELONG (ID_SHAPE, CBN_SELCHANGE), 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        // If we switch polygons, then update all of the info...
        case ID_SHAPE:
            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                WORD    wTemp;

                wTemp = (WORD)SendDlgItemMessage (hDlg, ID_SHAPE,
                                                    CB_GETCURSEL, 0, 0l);
                wPolygon = (WORD)SendDlgItemMessage (hDlg, ID_SHAPE,
                                                    CB_GETITEMDATA, wTemp, 0l);
                CheckDlgButton (hDlg, ID_ACTIVE, fOn[wPolygon]);
                SetDlgItemInt (hDlg, ID_LINES, wLines[wPolygon], FALSE);
                hPal = GetStockObject (DEFAULT_PALETTE);
                GetObject (hPal, sizeof (WORD), (LPTSTR) &nPal);
                if (SendDlgItemMessage (hDlg, ID_COLOR1, CB_SETCURSEL,
                    GetNearestPaletteIndex (hPal, dwStartColor[wPolygon]),
                    0l) == CB_ERR)
                    SendDlgItemMessage (hDlg, ID_COLOR1, CB_SETCURSEL, 0, 0l);
                if (SendDlgItemMessage (hDlg, ID_COLOR2, CB_SETCURSEL,
                    GetNearestPaletteIndex (hPal, dwEndColor[wPolygon]),
                    0l) == CB_ERR)
                    SendDlgItemMessage (hDlg, ID_COLOR2, CB_SETCURSEL, 0, 0l);

                // Set the walk state...
                CheckRadioButton (hDlg, ID_2COLORS, ID_RANDOMCOLORS, ID_2COLORS +
                    fWalk[wPolygon]);

                // Enable/disbale windows...
                ShadeWindows (hDlg, wPolygon, wPolygon);
            }
            break;

        // Toggle the actiavtion state...
        case ID_ACTIVE:
            fOn[wPolygon] ^= 1;
            CheckDlgButton (hDlg, LOWORD(wParam), fOn[wPolygon]);
            ShadeWindows (hDlg, wPolygon, wPolygon);
            break;

        case ID_CLEARSCREEN:
            fClearScreen ^= 1;
            CheckDlgButton (hDlg, LOWORD(wParam), fClearScreen);
            break;

        case ID_COLOR1:
        case ID_COLOR2:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                wTemp = (WORD)SendDlgItemMessage (hDlg, LOWORD(wParam), CB_GETCURSEL, 0, 0l);
                hPal = GetStockObject (DEFAULT_PALETTE);
                GetPaletteEntries (hPal, wTemp, 1, (LPPALETTEENTRY)(LPDWORD) & dwTemp);
                if ( LOWORD(wParam) == ID_COLOR1 )
                    dwStartColor[wPolygon] = dwTemp & 0xffffffL;
                else
                    dwEndColor[wPolygon] = dwTemp & 0xffffffL;
            }
            break;

        // Toggle the walk state...
        case ID_2COLORS:
        case ID_RANDOMCOLORS:
            fWalk[wPolygon] = LOWORD(wParam) - ID_2COLORS;
            CheckRadioButton (hDlg, ID_2COLORS, ID_RANDOMCOLORS, LOWORD(wParam));
            EnableWindow (GetDlgItem (hDlg, ID_COLOR1), !fWalk[wPolygon]);
            InvalidateRect (GetDlgItem (hDlg, ID_COLOR1), NULL, TRUE);
            EnableWindow (GetDlgItem (hDlg, ID_COLOR2), !fWalk[wPolygon]);
            InvalidateRect (GetDlgItem (hDlg, ID_COLOR2), NULL, TRUE);
            break;

        // Check to see if the edit texts have lost their focus. If so
        // update...
        case ID_LINES:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                wLoop = (WORD) GetDlgItemInt (hDlg, LOWORD(wParam), &fError, FALSE);
                fError = fError && (wLoop >= 1 && wLoop <= MAXLINES);
                EnableWindow (GetDlgItem (hDlg, ID_LINESARROW), fError);
                EnableWindow (GetDlgItem (hDlg, IDOK), fError);
                if (fError)
                    wLines[wPolygon] = wLoop;
            }
            break;

        // Save the current parameters...
        case IDOK:
            wLines[wPolygon] = (WORD) GetDlgItemInt (hDlg, ID_LINES, &fError, FALSE);

            // Write the activation state of clearing the screen...
            wsprintf (szBuffer, TEXT("%d"), fClearScreen);
            WritePrivateProfileString (szAppName, szClearName, szBuffer, szIniFile);

            /* Write the updated versions of everything here... */
            for (wLoop = 0; wLoop < NUMBER_POLYGONS; wLoop++)
            {
                /* Set the activation state... */
                wsprintf (szBuffer, TEXT("Active%d"), wLoop + 1);
                wsprintf (szBuffer2, TEXT("%d"), fOn[wLoop]);
                WritePrivateProfileString (szAppName, szBuffer, szBuffer2, szIniFile);

                /* Set the walk state... */
                wsprintf (szBuffer, TEXT("WalkRandom%d"), wLoop + 1);
                wsprintf (szBuffer2, TEXT("%d"), fWalk[wLoop]);
                WritePrivateProfileString (szAppName, szBuffer, szBuffer2, szIniFile);

                /* Get the number of lines for the current polygon... */
                wsprintf (szBuffer, TEXT("Lines%d"), wLoop + 1);
                wsprintf (szBuffer2, TEXT("%d"), wLines[wLoop]);
                WritePrivateProfileString (szAppName, szBuffer, szBuffer2, szIniFile);

                /* Set the start color... */
                wsprintf (szBuffer, TEXT("StartColor%d"), wLoop + 1);
                byR = GetRValue (dwStartColor[wLoop]);
                byG = GetGValue (dwStartColor[wLoop]);
                byB = GetBValue (dwStartColor[wLoop]);
                wsprintf (szBuffer2, TEXT("%d %d %d"), byR, byG, byB);
                WritePrivateProfileString (szAppName, szBuffer, szBuffer2, szIniFile);

                /* Set the end color... */
                wsprintf (szBuffer, TEXT("EndColor%d"), wLoop + 1);
                byR = GetRValue (dwEndColor[wLoop]);
                byG = GetGValue (dwEndColor[wLoop]);
                byB = GetBValue (dwEndColor[wLoop]);
                wsprintf (szBuffer2, TEXT("%d %d %d"), byR, byG, byB);
                WritePrivateProfileString (szAppName, szBuffer, szBuffer2, szIniFile);
            }

        /* Bail out... */

        case IDCANCEL:
            EndDialog (hDlg, LOWORD(wParam) == IDOK);
            return TRUE;

        }
        break;

    case WM_DRAWITEM:
        return (BOOL)AppOwnerDraw (hDlg, message, wParam, lParam);

    case WM_MEASUREITEM:
        return (BOOL)AppOwnerDraw (hDlg, message, wParam, lParam);

    case WM_DELETEITEM:
        return (BOOL)AppOwnerDraw (hDlg, message, wParam, lParam);

    case WM_HELP: // F1
        WinHelp(
            (HWND) ((LPHELPINFO) lParam)->hItemHandle,
            szHelpFile,
            HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aMystDlgHelpIds
        );
        break;

    case WM_CONTEXTMENU:  // right mouse click
        WinHelp(
            (HWND) wParam,
            szHelpFile,
            HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aMystDlgHelpIds
        );
        break;

    default:
        break;
    }
    return FALSE;
}


VOID GetFields (VOID)
{
    WORD wLoop;
    //Load Global Strings from stringtable
    LoadString (hMainInstance, idsName, szName, CharSizeOf(szName));
    LoadString (hMainInstance, idsAppName, szAppName, CharSizeOf(szAppName));

    //Load Common Strings from stringtable...
    LoadString (hMainInstance, idsIniFile, szIniFile, CharSizeOf(szIniFile));
    LoadString (hMainInstance, idsScreenSaver, szScreenSaver, CharSizeOf(szScreenSaver));
    LoadString (hMainInstance, idsHelpFile, szHelpFile, CharSizeOf(szHelpFile));
    LoadString (hMainInstance, idsNoHelpMemory, szNoHelpMemory, CharSizeOf(szNoHelpMemory));

    /* Do we clear the screen when we start... */
    if ((fClearScreen = GetPrivateProfileInt (szAppName, szClearName, 1, szIniFile)) != 0)
        fClearScreen = 1;

    /* Loop through and get all of the field information... */
    for (wLoop = 0; wLoop < NUMBER_POLYGONS; wLoop++)
    {
        /* Get the activation state... */
        wsprintf (szBuffer, TEXT("Active%d"), wLoop + 1);
        if ((fOn[wLoop] = GetPrivateProfileInt (szAppName, szBuffer, 1, szIniFile)) != 0)
            fOn[wLoop] = 1;

        /* Get the walk state... */
        wsprintf (szBuffer, TEXT("WalkRandom%d"), wLoop + 1);
        if ((fWalk[wLoop] = GetPrivateProfileInt (szAppName, szBuffer, 1, szIniFile)) != 0)
            fWalk[wLoop] = 1;

        /* Get the number of lines for the current polygon... */
        wsprintf (szBuffer, TEXT("Lines%d"), wLoop + 1);
        wLines[wLoop] = (WORD) GetPrivateProfileInt (szAppName, szBuffer, 5, szIniFile);
        if ((int)wLines[wLoop] < 1)
            wLines[wLoop] = 1;
        if (wLines[wLoop] > MAXLINES)
            wLines[wLoop] = MAXLINES;

        /* Get the starting and ending colors (stored in DWORD format)... */
        wsprintf (szBuffer, TEXT("StartColor%d"), wLoop + 1);
        dwStartColor[wLoop] = GetProfileRgb (szAppName, szBuffer, RGB (0, 0, 0));
        wsprintf (szBuffer, TEXT("EndColor%d"), wLoop + 1);
        dwEndColor[wLoop] = GetProfileRgb (szAppName, szBuffer, RGB (255, 255, 255));
    }

    return;
}


VOID DrawPolygon (HDC hDC, HPEN hPen, WORD wPolygon, WORD wLine)
{
    HANDLE          hOldPen;
    WORD            wLoop1;

    hOldPen = SelectObject (hDC, hPen);
    MoveToEx (hDC, ptBox[wPolygon*MAXLINES+wLine][0].x,
                                ptBox[wPolygon*MAXLINES+wLine][0].y, NULL);
    for (wLoop1 = 0; wLoop1 < 4; wLoop1++)
        LineTo (hDC, ptBox[wPolygon*MAXLINES+wLine][(wLoop1+1)%4].x,
            ptBox[wPolygon*MAXLINES+wLine][(wLoop1+1)%4].y);
    if (hOldPen)
        SelectObject (hDC, hOldPen);
    return;
}

/* Adjust each of the rgb components according to the four input variables...*/

DWORD AdjustColor (DWORD dwSrc, DWORD dwDest, int nInc, int nCnt)
{
    DWORD dwTemp;
    WORD  wLoop;
    int      nSrc, nDst, nTmp;
    int      n1, n2, n3, n4, n5;

    /* Nullify the end value... */
    dwTemp = 0;

    /* Cycle through and compute the difference on each byte... */
    for (wLoop = 0; wLoop < 3; wLoop++)
    {
        nSrc = (int)((dwSrc >> (wLoop * 8)) % 256);
        nDst = (int)((dwDest >> (wLoop * 8)) % 256);
        n1 = nDst - nSrc;
        n2 = n1 * 10;
        n3 = n2 / nInc;
        n4 = n3 * nCnt;
        n5 = n4 / 10;
        nTmp = nSrc + n5;
        dwTemp += ((DWORD)nTmp) << (wLoop * 8);
    }
    return dwTemp;
}


/* Compute a random color that is within the accepted norms... */

DWORD GenerateColor (VOID)
{
    return (((DWORD)ZRAND (256)) + (((DWORD)ZRAND (256)) << 8) +
                    (((DWORD)ZRAND (256)) << 16));
}


/* Compute a random velocity that is within the accepted norms... */

WORD GenerateVelocity (VOID)
{
    return 255;
    return (RAND (30) + 20);
}


LONG AppOwnerDraw (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT        rc;
    DWORD       rgbBg;
    static HBITMAP hbmCheck = NULL;
    LPMEASUREITEMSTRUCT     lpMIS = ((LPMEASUREITEMSTRUCT)lParam);
    LPDRAWITEMSTRUCT        lpDIS = ((LPDRAWITEMSTRUCT)lParam);

    switch (msg)
    {
    case WM_MEASUREITEM:
        lpMIS->itemHeight = 15;
        return TRUE;

    case WM_DRAWITEM:
        rc    = lpDIS->rcItem;
        rgbBg = PALETTEINDEX (lpDIS->itemID);

        if (lpDIS->itemState & ODS_SELECTED)
        {
            FrameR (lpDIS->hDC, &rc, rgbBlack, 2);
            InflateRect (&rc, -1, -1);
            FrameR (lpDIS->hDC, &rc, rgbWhite, 2);
            InflateRect (&rc, -1, -1);
        }
        if (lpDIS->itemState & ODS_DISABLED)
            FillR (lpDIS->hDC, &rc, rgbGrey);
        else
            FillR (lpDIS->hDC, &rc, rgbBg);
        return TRUE;

    case WM_DELETEITEM:
        return TRUE;
    }
    return TRUE;
}


VOID PatB (HDC hdc, int x, int y, int dx, int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor (hdc, rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut (hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}


VOID FillR (HDC hdc, LPRECT prc, DWORD rgb)
{
    SetBkColor (hdc, rgb);
    ExtTextOut (hdc, 0, 0, ETO_OPAQUE, prc, NULL, 0, NULL);
}


VOID FrameR (HDC hdc, LPRECT prc, DWORD rgb, int iFrame)
{
//    RECT    rc;
    int    dx, dy;

    dx = prc->right  - prc->left;
    dy = prc->bottom - prc->top - 2 * iFrame;

    PatB (hdc, prc->left, prc->top,          dx, iFrame,   rgb);
    PatB (hdc, prc->left, prc->bottom - iFrame, dx, iFrame,   rgb);

    PatB (hdc, prc->left,          prc->top + iFrame, iFrame, dy, rgb);
    PatB (hdc, prc->right - iFrame,  prc->top + iFrame, iFrame, dy, rgb);
}


BOOL DrawBitmap (HDC hdc, int x, int y, HBITMAP hbm, DWORD rop)
{
    HDC hdcBits;
    BITMAP bm;
//    HPALETTE hpalT;
    HBITMAP oldbm;
    BOOL f;

    if (!hdc || !hbm)
        return FALSE;

    hdcBits = CreateCompatibleDC (hdc);
    GetObject (hbm, sizeof (BITMAP), (LPTSTR) & bm);
    oldbm = SelectObject (hdcBits, hbm);
    f = BitBlt (hdc, x, y, bm.bmWidth, bm.bmHeight, hdcBits, 0, 0, rop);
    if (oldbm)
        SelectObject (hdcBits, oldbm);
    DeleteDC (hdcBits);

    return f;
}


DWORD GetProfileRgb (LPTSTR szApp, LPTSTR szItem, DWORD rgb)
{
    TCHAR    buf[80];
    LPTSTR   pch;
    WORD     r, g, b;

    GetPrivateProfileString (szApp, szItem, TEXT(""), buf, CharSizeOf(buf), szIniFile);

    if (*buf)
    {
        pch = buf;
        r = AtoI (pch);
        while (*pch && *pch != TEXT(' '))
            pch++;
        while (*pch && *pch == TEXT(' '))
            pch++;
        g = AtoI (pch);
        while (*pch && *pch != TEXT(' '))
            pch++;
        while (*pch && *pch == TEXT(' '))
            pch++;
        b = AtoI (pch);

        return RGB (r, g, b);
    }
    else
        return rgb;
}


WORD AtoI (LPTSTR lpszConvert)
{
    WORD wReturn = 0;

    while (*lpszConvert >= TEXT('0') && *lpszConvert <= TEXT('9'))
    {
        wReturn = wReturn * 10 + (WORD)(*lpszConvert - TEXT('0'));
        lpszConvert++;
    }
    return wReturn;
}


VOID ShadeWindows (HWND hDlg, WORD wPoly, WORD wPolygon)
{
    EnableWindow (GetDlgItem (hDlg, ID_COLORGROUP), fOn[wPolygon]);
    EnableWindow (GetDlgItem (hDlg, ID_2COLORS), fOn[wPolygon]);
    EnableWindow (GetDlgItem (hDlg, ID_RANDOMCOLORS), fOn[wPolygon]);
    EnableWindow (GetDlgItem (hDlg, ID_LINES), fOn[wPolygon]);
    EnableWindow (GetDlgItem (hDlg, ID_LINESARROW), fOn[wPolygon]);
    EnableWindow (GetDlgItem (hDlg, ID_COLOR1), !fWalk[wPolygon] && fOn[wPolygon]);
    InvalidateRect (GetDlgItem (hDlg, ID_COLOR1), NULL, TRUE);
    EnableWindow (GetDlgItem (hDlg, ID_COLOR2), !fWalk[wPolygon] && fOn[wPolygon]);
    InvalidateRect (GetDlgItem (hDlg, ID_COLOR2), NULL, TRUE);
}
