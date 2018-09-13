/*

SSBEZIER.C

Bezier screensaver.

  History:
       10/14/91        kentd  Wrote for Windows NT.  Hacked from WinBez.

*/

#include <windows.h>
#include <commctrl.h>
#include <scrnsave.h>
#include "bezdlg.h"
#include "strings.h"
#include "uniconv.h"
#include "stdlib.h"

#undef OVERFLOW
#undef UNDERFLOW

#include "math.h"


#define INT int

#if !defined(_ALPHA_)
// floating point always initialized on ALPHA and ALPHA64
VOID _fltused(VOID) {}
#endif

// routines from bez.c

VOID vInitPoints();
VOID vRedraw();
VOID vNextBez();
LONG GetDlgItemLong(HWND hDlg, WORD wID, BOOL *pfTranslated, BOOL fSigned);
VOID GetIniEntries(VOID);
VOID vInitPalette(HDC);
VOID vNewColor(VOID);
LONG GetPrivateProfileLong(LPTSTR pszApp, LPTSTR pszKey, LONG lDefault);

typedef struct _STR
{
    PSZ     psz;
    SIZE    sz;
    SHORT   c;
    BYTE    f;
} STR;

typedef struct _inst
{
    ULONG i;
    POINT pt;
    POINT ptVel;
    LONG  c;
} INST, *PINST;


typedef struct _WINDOW {
    HWND hwnd;
    HDC hdc;
    HANDLE hWait;
    int xsize;
    int ysize;
    HPALETTE hpalette;

    // frame data

    HBITMAP hbitmap;
    HDC hdcBitmap;
    PBYTE pdata;
    RECT rcBlt;
    RECT rcDraw;
    int xDelta;
    int yDelta;

    // text data

    HBITMAP hbitmapText;
    HDC hdcText;
    PBYTE pdataText;
} WINDOW, *PWINDOW;

PWINDOW gpwindow;
BOOL fRepaint = TRUE;

//
// Length is the number of beziers in each loop
// Width is the number of times each bezier loop is drawn
//

#define MINLENGTH     1
#define MAXLENGTH     10
#define DEF_LENGTH    4
#define MINWIDTH      1
#define MAXWIDTH      100
#define DEF_WIDTH     30
#define MINVEL        2
#define DEFVEL        10
#define MAXVEL        20
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define NEWVEL (ulRandom() % (MAXVEL / 2 - MINVEL) + MINVEL)


HDC     ghdc;
HPEN    ghpenBez;
HPEN    ghpenErase;
HBRUSH  ghbrushBack;
DWORD   glSeed;
LONG    gcxScreen;
LONG    gcyScreen;
LONG    gczScreen;
LONG    gcPoints;
HDC     hdcBM;
HBITMAP hbm;
PINST   ainst;
BOOL    gbPointsDrawn;
LONG    giVelMax  =  DEFVEL;
LONG    gcBez     =  DEF_WIDTH;
LONG    gcRingLen =  DEF_LENGTH;
STR    *astr      = NULL;
BOOL    bInit     = FALSE;
int     cstr      = 0;
ULONG   ic        = 0;
BYTE    gf        = 0xff;
BOOL    gbCopy    = TRUE;
BOOL    gbPalette = FALSE;
HPALETTE ghpal    = 0;
HPALETTE ghpalOld = 0;

#define NUM_PALETTE_ENTRIES 10
#define FADE_RESOLUTION     24
#define MAX_TICKS_WIMPY     1000
#define MAX_TICKS_COOL      100

PALETTEENTRY gapal[NUM_PALETTE_ENTRIES * FADE_RESOLUTION];
PALETTEENTRY gapalDefault[NUM_PALETTE_ENTRIES + 1] =
                              { {255, 0,   0},   {128, 0,   0},
                                {0,   128, 0},   {128, 128, 0},
                                {0,   0,   128}, {128, 0,   128},
                                {0,   128, 128}, {128, 128, 128},
                                {192, 192, 192}, {255, 0,   0},
                                {0,   0,   0} };

LONG gipal;
LONG gcpal;
LONG gcTicker;
LONG gcMaxTicks;

// Structures:

typedef struct _BAND {
    POINT apt[2];
} BAND;

typedef struct _BEZ {
    BAND band[MAXLENGTH];
    BOOL bDrawn;
} BEZ, *PBEZ;

BEZ bezbuf[MAXWIDTH];
PBEZ gpBez;

POINT aPts[MAXLENGTH * 3 + 1];
POINT aVel[MAXLENGTH][2];

TCHAR  szLineSpeed [] = TEXT("LineSpeed");  // .INI Line Speed key

TCHAR  szNumBez [] = TEXT("Width");         // .INI Width key

TCHAR  szNumRings [] = TEXT("Length");      // .INI Length key

void Init(HWND);

BYTE mask = 0;

//
// Help IDs
//
DWORD aBezDlgHelpIds[] = {
    65535,                  ((DWORD) -1),
    ID_LENGTH_LABEL,        IDH_DISPLAY_SCREENSAVER_BEZIERS_LENGTH,
    ID_LENGTH,              IDH_DISPLAY_SCREENSAVER_BEZIERS_LENGTH,
    ID_LENGTHARROW,         IDH_DISPLAY_SCREENSAVER_BEZIERS_LENGTH,
    ID_WIDTH_LABEL,         IDH_DISPLAY_SCREENSAVER_BEZIERS_WIDTH,
    ID_WIDTH,               IDH_DISPLAY_SCREENSAVER_BEZIERS_WIDTH,
    ID_WIDTHARROW,          IDH_DISPLAY_SCREENSAVER_BEZIERS_WIDTH,
    ID_VELOCITY,            IDH_DISPLAY_SCREENSAVER_BEZIERS_SPEED,
    ID_VELOCITY_SLOW,       IDH_DISPLAY_SCREENSAVER_BEZIERS_SPEED,
    ID_VELOCITY_FAST,       IDH_DISPLAY_SCREENSAVER_BEZIERS_SPEED,
    0,0
};

/* This is the main window procedure to be used when the screen saver is
    activated in a screen saver mode ( as opposed to configure mode ).  This
    function must be declared as an EXPORT in the EXPORTS section of the
    DEFinition file... */

LRESULT ScreenSaverProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static UINT_PTR  wTimer;
    TCHAR   szTemp[20];                     // Temporary string buffer
    int i;
    BYTE bit;

    switch (message)
    {
    case WM_CREATE:
        GetIniEntries ();
        glSeed = GetCurrentTime ();     // random number generator

        ghdc = GetDC(hWnd);

        gcxScreen =  ((LPCREATESTRUCT)lParam)->cx;
        gcyScreen =  ((LPCREATESTRUCT)lParam)->cy;

        vInitPoints();

        if (gczScreen & (1 << 14)) {
            Init(hWnd);
        } else {
            ghbrushBack = GetStockObject(BLACK_BRUSH);
            ghpenBez    = CreatePen(PS_SOLID, 0, 0xff);
            ghpenErase  = CreatePen(PS_SOLID, 0, 0);
            SelectObject(ghdc,ghpenBez);
            SelectObject(ghdc,ghbrushBack);

            vInitPalette(ghdc);
            wTimer = SetTimer (hWnd, 1, 1, NULL);
        }
        break;

    case WM_SIZE:
        gcxScreen = LOWORD(lParam);
        gcyScreen = HIWORD(lParam);
        break;

    case WM_PALETTECHANGED:
        RealizePalette(ghdc);
        break;

    case WM_QUERYNEWPALETTE:
        if (ghpal != 0)
        {
            SelectPalette(ghdc, ghpal, FALSE);
            RealizePalette(ghdc);
            InvalidateRect(hWnd, NULL, TRUE);

            return(TRUE);
        }
        else
            return(FALSE);

    case WM_PAINT:
        if (gczScreen & (1 << 14)) {
            PAINTSTRUCT paint;
            BeginPaint(hWnd, &paint);
            EndPaint(hWnd, &paint);
            fRepaint = TRUE;
        } else {
            vRedraw();
        }
        break;

    case WM_TIMER:
        if (gczScreen & (1 << 14)) {
            SetEvent(gpwindow->hWait);
        } else {
            vNextBez();
        }
        break;

    case WM_DESTROY:
        if (wTimer)
            KillTimer (hWnd, wTimer);

        if (ghpal != 0)
        {
            SelectPalette(ghdc, ghpalOld, FALSE);
            DeleteObject(ghpal);
        }
        ReleaseDC(hWnd, ghdc);
        break;
    }
    return (DefScreenSaverProc (hWnd, message, wParam, lParam));
}


//***************************************************************************

BOOL ScreenSaverConfigureDialog (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL    fError;                         // Error flag

    UINT    wTemp;
    TCHAR   szTemp[20];                     // Temporary string buffer
    char    szTemp2[20];                    // Temporary string buffer

    WORD    nCtlId;
    int     nVal, nOldVal;
    LONG   *plState = (LONG *) szTemp2;     // State buffer

    static HWND hVelocity,         // window handle of Speed scrollbar
                hIDOK,             // window handle of OK button
                hSetPassword,      // window handle of SetPassword button
                hNumBeziers,       // window handle of NumBezier EditControl
                hNumRings;         // window handle of NumBezier EditControl


    switch (message)
    {
    case WM_INITDIALOG:
        GetIniEntries ();        // Get initial values

        hVelocity = GetDlgItem (hDlg, ID_VELOCITY);
        hIDOK = GetDlgItem (hDlg, IDOK);
        hNumBeziers = GetDlgItem (hDlg, ID_WIDTH);
        hNumRings = GetDlgItem (hDlg, ID_LENGTH);

        SendMessage (hNumBeziers, EM_LIMITTEXT, 3, 0);
        SendMessage (hNumRings, EM_LIMITTEXT, 3, 0);
        SetScrollRange (hVelocity, SB_CTL, MINVEL, MAXVEL, FALSE);
        SetScrollPos (hVelocity, SB_CTL, giVelMax, TRUE);

        SetDlgItemInt (hDlg, ID_WIDTH, gcBez, FALSE);
        SetDlgItemInt (hDlg, ID_LENGTH, gcRingLen, FALSE);

        SendDlgItemMessage( hDlg, ID_LENGTHARROW, UDM_SETRANGE, 0, MAKELONG(MAXLENGTH, MINLENGTH));
        SendDlgItemMessage( hDlg, ID_WIDTHARROW, UDM_SETRANGE, 0, MAKELONG(MAXWIDTH, MINWIDTH));

        wsprintf (szTemp, TEXT("%d"), gcBez);
        WritePrivateProfileString (szAppName, szNumBez, szTemp, szIniFile);
        return TRUE;


    case WM_HSCROLL:
        switch (LOWORD(wParam))
        {
        case SB_PAGEUP:
            --giVelMax;
            break;

        case SB_LINEUP:
            --giVelMax;
            break;

        case SB_PAGEDOWN:
            ++giVelMax;
            break;

        case SB_LINEDOWN:
            ++giVelMax;
            break;

        case SB_THUMBPOSITION:
            giVelMax = HIWORD (wParam);
            break;

        case SB_BOTTOM:
            giVelMax = MINVEL;
            break;

        case SB_TOP:
            giVelMax = MAXVEL;
            break;

        case SB_THUMBTRACK:
        case SB_ENDSCROLL:
            return TRUE;
            break;
        }
        if ((int)giVelMax <= MINVEL)
            giVelMax = MINVEL;
        if ((int)giVelMax >= MAXVEL)
            giVelMax = MAXVEL;

        SetScrollPos ((HWND) lParam, SB_CTL, giVelMax, TRUE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_LENGTH:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                wTemp = GetDlgItemInt (hDlg, ID_LENGTH, &fError, FALSE);
                fError = ((wTemp <= MAXLENGTH) && (wTemp >= MINLENGTH));
                EnableWindow (GetDlgItem (hDlg, ID_LENGTHARROW), fError);
                EnableWindow (GetDlgItem (hDlg, IDOK), fError);
            }
            break;

        case ID_WIDTH:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                wTemp = GetDlgItemInt (hDlg, ID_WIDTH, &fError, FALSE);
                fError = ((wTemp <= MAXWIDTH) && (wTemp >= MINWIDTH));
                EnableWindow (GetDlgItem (hDlg, ID_WIDTHARROW), fError);
                EnableWindow (GetDlgItem (hDlg, IDOK), fError);
            }
            break;


        case IDOK:
            wTemp = GetDlgItemInt (hDlg, ID_WIDTH, &fError, FALSE);
            wTemp |= GetPrivateProfileInt (szAppName, szNumBez, DEF_WIDTH, szIniFile) & (1 << 14);
            wsprintf (szTemp, TEXT("%d"), wTemp);

            WritePrivateProfileString (szAppName, szNumBez, szTemp, szIniFile);

            wTemp = GetDlgItemInt (hDlg, ID_LENGTH, &fError, FALSE);
            wsprintf (szTemp, TEXT("%d"), wTemp);
            WritePrivateProfileString (szAppName, szNumRings, szTemp, szIniFile);

            wsprintf (szTemp, TEXT("%d"), giVelMax);
            WritePrivateProfileString (szAppName, szLineSpeed, szTemp, szIniFile);

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
            (ULONG_PTR) (LPSTR) aBezDlgHelpIds
        );
        break;

    case WM_CONTEXTMENU:  // right mouse click
        WinHelp(
            (HWND) wParam,
            szHelpFile,
            HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aBezDlgHelpIds
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
    TCHAR   szTemp[20];
    LPTSTR  pszTemp;

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
    LoadString (hMainInstance, idsName, szName, TITLEBARNAMELEN);
    LoadString (hMainInstance, idsAppName, szAppName, APPNAMEBUFFERLEN);

    //Load Common Strings from stringtable...
    LoadString (hMainInstance, idsIniFile, szIniFile, MAXFILELEN);
    LoadString (hMainInstance, idsScreenSaver, szScreenSaver, 22);
    LoadString (hMainInstance, idsHelpFile, szHelpFile, MAXFILELEN);
    LoadString (hMainInstance, idsNoHelpMemory, szNoHelpMemory, BUFFLEN);

    giVelMax = GetPrivateProfileInt (szAppName, szLineSpeed, DEFVEL, szIniFile);
    if (giVelMax > MAXVEL || giVelMax < MINVEL)
        giVelMax = DEFVEL;

    gcBez = GetPrivateProfileInt (szAppName, szNumBez, DEF_WIDTH, szIniFile);
    gcBez = (gczScreen = gcBez) & ~(1 << 14);

    if (gcBez > MAXWIDTH)
        gcBez = MAXWIDTH;
    if (gcBez < MINWIDTH)
        gcBez = MINWIDTH;

    gcRingLen = GetPrivateProfileInt (szAppName, szNumRings, DEF_LENGTH, szIniFile);
    if (gcRingLen > MAXLENGTH)
        gcRingLen = MAXLENGTH;
    if (gcRingLen < MINLENGTH)
        gcRingLen = MINLENGTH;
}



/************************************************************************
* Bezier code
*
* Created: 19-Oct-1990 10:18:45
* Author: Paul Butzi
*
* Copyright (c) 1990 Microsoft Corporation
*
* Generates random lines
*    Hacked from arcs.c
\**************************************************************************/


DWORD ulRandom()
{
    glSeed *= 69069;
    glSeed++;
    return(glSeed);
}


VOID vCLS()
{
    PatBlt(ghdc, 0, 0, gcxScreen, gcyScreen, PATCOPY);
}

int iNewVel(INT i)
{

    if ((gcRingLen != 1) || (i == 1) || (i == 2))
        return(ulRandom() % (giVelMax + 1 / 3) + MINVEL);
    else
        return(ulRandom() % giVelMax + MINVEL);
}



VOID vInitPoints()
{
    INT ii;

    for (ii = 0; ii < MAXLENGTH; ii++)
    {
        bezbuf[0].band[ii].apt[0].x = gcxScreen ? ulRandom() % gcxScreen : 0;
        bezbuf[0].band[ii].apt[0].y = gcyScreen ? ulRandom() % gcyScreen : 0;
        bezbuf[0].band[ii].apt[1].x = gcxScreen ? ulRandom() % gcxScreen : 0;
        bezbuf[0].band[ii].apt[1].y = gcyScreen ? ulRandom() % gcyScreen : 0;

        aVel[ii][0].x = iNewVel(ii) * ((ulRandom() & 0x10) ? 1 : -1);
        aVel[ii][0].y = iNewVel(ii) * ((ulRandom() & 0x10) ? 1 : -1);
        aVel[ii][1].x = iNewVel(ii) * ((ulRandom() & 0x11) ? 1 : -1);
        aVel[ii][1].y = iNewVel(ii) * ((ulRandom() & 0x10) ? 1 : -1);
    }

    gpBez = bezbuf;
}


VOID vRedraw()
{
    INT j;

    for ( j = 0; j < gcBez; j += 1 )
    {
        bezbuf[j].bDrawn = FALSE;
    }

    vCLS();
    gpBez = bezbuf;
    gbPointsDrawn = FALSE;
}


/******************************Public*Routine******************************\
* VOID vDrawBand(pbez)
*
* History:
*  14-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID vDrawBand(PBEZ pbez)
{
    INT    ii;
    INT    iNext;
    PPOINT ppt;

// If only drawing one Bezier, special case it:

    if (gcRingLen == 1)
    {
        aPts[0] = pbez->band[0].apt[0];
        aPts[1] = pbez->band[0].apt[1];
        aPts[2] = pbez->band[1].apt[0];
        aPts[3] = pbez->band[1].apt[1];
    }
    else
    {

    // Do the elastic band effect, with 2nd order continuity:

        aPts[0].x = (pbez->band[0].apt[0].x + pbez->band[0].apt[1].x) >> 1;
        aPts[0].y = (pbez->band[0].apt[0].y + pbez->band[0].apt[1].y) >> 1;

        ppt = &aPts[1];

        for (ii = 0; ii < gcRingLen; ii++)
        {
            iNext = (ii + 1) % gcRingLen;

            *ppt++ = pbez->band[ii].apt[1];
            *ppt++ = pbez->band[iNext].apt[0];

            ppt->x = (pbez->band[iNext].apt[0].x + pbez->band[iNext].apt[1].x) >> 1;
            ppt->y = (pbez->band[iNext].apt[0].y + pbez->band[iNext].apt[1].y) >> 1;
            ppt++;
        }
    }

    PolyBezier(ghdc, aPts, gcRingLen * 3 + 1);
}


/******************************Public*Routine******************************\
* VOID vNextBez()
*
\**************************************************************************/

VOID vNextBez()
{
    INT ii;
    INT jj;

    PBEZ obp = gpBez++;

    if ( gpBez >= &bezbuf[gcBez] )
        gpBez = bezbuf;

// If bezier on screen, erase by redrawing:

    if (gpBez->bDrawn)
    {
        if (gbCopy)
            SelectObject(ghdc, ghpenErase);

        vDrawBand(gpBez);
    }

// Adjust points:

    for (ii = 0; ii < MAX(gcRingLen, 2); ii++)
    {
        for (jj = 0; jj < 2; jj++)
        {
            register INT x, y;

            x = obp->band[ii].apt[jj].x;
            y = obp->band[ii].apt[jj].y;

            x += aVel[ii][jj].x;
            y += aVel[ii][jj].y;

            if ( x >= gcxScreen )
            {
                x = gcxScreen - ((x - gcxScreen) + 1);
                aVel[ii][jj].x = - iNewVel(ii);
            }
            if ( x < 0 )
            {
                x = - x;
                aVel[ii][jj].x = iNewVel(ii);
            }
            if ( y >= gcyScreen )
            {
                y = gcyScreen - ((y - gcyScreen) + 1);
                aVel[ii][jj].y = - iNewVel(ii);
            }
            if ( y < 0 )
            {
                y = - y;
                aVel[ii][jj].y = iNewVel(ii);
            }

            gpBez->band[ii].apt[jj].x = x;
            gpBez->band[ii].apt[jj].y = y;
        }
    }

    vNewColor();

    if (gbCopy)
        SelectObject(ghdc, ghpenBez);

    vDrawBand(gpBez);
    gpBez->bDrawn = TRUE;
}


ULONG iGet()
{
    static int i = 0;
           int j;
    if (++i >= cstr)
        i = 1;
    j = i;
    while (astr[i].f == gf) {i = (i % (cstr - 1)) + 1; if (i == j) gf = ~gf;}
    astr[i].f = ~astr[i].f;
    return(i);
}

VOID vInitPalette(HDC hdc)
{
    LOGPALETTE    lp;
    HPALETTE      hpalOld = 0;
    PALETTEENTRY *ppal;
    LONG          cBitsPerPel;
    LONG          i;
    LONG          j;

    cBitsPerPel = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

    gbPalette = (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) > 0;

    if (gbPalette)
    {
    // Try and realize a palette with one palette entry reserved:

        lp.palVersion             = 0x300;
        lp.palNumEntries          = 1;
        lp.palPalEntry[0].peFlags = PC_RESERVED;

        ghpal = CreatePalette(&lp);
        if (ghpal != 0)
        {
            ghpalOld = SelectPalette(hdc, ghpal, FALSE);
            RealizePalette(ghdc);
        }
    }

    if (!gbPalette && cBitsPerPel <= 4)
    {
    // If on a wimpy device, simply cycle through the 'dark' colors of
    // its palette, skipping black because it's boring:

        gcpal = GetSystemPaletteEntries(hdc, 1, NUM_PALETTE_ENTRIES, &gapal[0]);
        if (gcpal == 0)
        {
        // Worst comes to worst, always use a white pen:

            gcpal = 1;
            gapal[0].peRed   = 255;
            gapal[0].peGreen = 255;
            gapal[0].peBlue  = 255;
            gapal[0].peFlags = 0;
        }

        gipal      = 8 % gcpal;     // Start with red
        gcMaxTicks = MAX_TICKS_WIMPY;
        gcTicker   = 0;

        return;
    }

// At this point, we either have a palette managed or high color device.

    ppal = &gapal[0];
    for (i = 0; i < NUM_PALETTE_ENTRIES; i++)
    {
        for (j = 0; j < FADE_RESOLUTION; j++)
        {
            ppal->peRed   = (BYTE)(gapalDefault[i].peRed +
                (j * (gapalDefault[i + 1].peRed   - gapalDefault[i].peRed))
                / FADE_RESOLUTION);
            ppal->peGreen = (BYTE)(gapalDefault[i].peGreen +
                (j * (gapalDefault[i + 1].peGreen - gapalDefault[i].peGreen))
                / FADE_RESOLUTION);
            ppal->peBlue = (BYTE)(gapalDefault[i].peBlue +
                (j * (gapalDefault[i + 1].peBlue  - gapalDefault[i].peBlue))
                / FADE_RESOLUTION);
            ppal->peFlags = PC_RESERVED;
            ppal++;
        }
    }

    gcpal      = (NUM_PALETTE_ENTRIES - 1) * FADE_RESOLUTION;
    gipal      = 0;
    gcMaxTicks = MAX_TICKS_COOL;
    gcTicker   = 0;

    if (gbPalette)
    {
    // Create a pen that maps to logical palette index zero:

        SelectObject(hdc, GetStockObject(BLACK_PEN));
        DeleteObject(ghpenBez);
        ghpenBez = CreatePen(0, 0, PALETTEINDEX(0));
        SelectObject(hdc, ghpenBez);
    }

    return;
}

VOID vNewColor(VOID)
{
    HPEN hpen;

    if (--gcTicker <= 0)
    {
        if (gbPalette)
        {
            AnimatePalette(ghpal, 0, 1, &gapal[gipal]);
        }
        else
        {
            if (gbCopy)
            {
                hpen = CreatePen(0, 0, RGB(gapal[gipal].peRed,
                                           gapal[gipal].peGreen,
                                           gapal[gipal].peBlue));

                SelectObject(ghdc, hpen);
                DeleteObject(ghpenBez);
                ghpenBez = hpen;
            }
        }

        gcTicker = gcMaxTicks;
        if (--gipal < 0)
            gipal = gcpal - 1;
    }

    return;
}

/****************************************************************************/

typedef struct _DOT {
    int xm, ym, zm;
    int xn, yn, zn;
    int color;
} DOT, *PDOT;

typedef struct _LIST *PLIST;
typedef struct _LIST {
    PLIST pnext;
    PLIST plistComplete;
    PSZ psz;
} LIST;

#define MAXFIXED (65536)

#define AXISSIZE 150
#define XSIZE (gcxScreen)
#define YSIZE (gcyScreen)
#define XSIZE2 (XSIZE / 2)
#define YSIZE2 (YSIZE / 2)
#define SCANSIZE ((8 * XSIZE + 31) & ~31) / 8
#define PI (3.141529)

#define MAXANGLE (360 * 10)

/****************************************************************************/

int *icos;
int *isin;

/****************************************************************************/

void ClearRect(
    PBYTE pstart,
    PRECT prc)
{
    PBYTE pdst;
    int length, y;

    pdst = pstart + SCANSIZE * prc->top + prc->left;
    length = prc->right - prc->left;

    for (y = prc->top; y < prc->bottom; y++) {
         memset(pdst, 0, length);
         pdst += SCANSIZE;
    }
}

/****************************************************************************/

void UnionRects(
    PRECT prcDest,
    PRECT prc)
{
    if (prc->left   < prcDest->left)   prcDest->left = prc->left;
    if (prc->right  > prcDest->right)  prcDest->right = prc->right;
    if (prc->top    < prcDest->top)    prcDest->top = prc->top;
    if (prc->bottom > prcDest->bottom) prcDest->bottom = prc->bottom;
}

/****************************************************************************/

__inline int WrapPlus(
    int deg,
    int range)
{
    return  deg >= range
      ? deg - range
      : deg;
}

__inline int WrapMinus(
    int deg,
    int range)
{
    return   deg < 0
        ? deg + range
        : deg;
}

__inline int Bound(
    int deg,
    int range)
{
    return WrapMinus(WrapPlus(deg, range), range);
}

/****************************************************************************/

int RandomInt(
    int min,
    int max)
{
    int dx = max - min;
    int mask = 1;
    int value;

    while (mask < dx) {
        mask = (mask << 1) + 1;
    }

    while ((value = (rand() & mask) + min) > max) ;
    return value;
}

/****************************************************************************/

#define NUMDOTS 1500

PDOT adot;

void InitDrawShaded(
    PWINDOW pwindow)
{
    int d0, d1;
    int c0, c1, s0, s1;
    int i;
    int x, y, z;

    pwindow->rcDraw.left = 0;
    pwindow->rcDraw.right = 0;
    pwindow->rcDraw.top = 0;
    pwindow->rcDraw.bottom = 0;

    for (i = 0; i < NUMDOTS; i++) {
        PDOT pdot = adot + i;

        pdot->xm = 1 * AXISSIZE / 4;
        pdot->ym = 0;
        pdot->zm = 0;

        d0 = RandomInt(0, MAXANGLE / 2);
        d1 = RandomInt(0, MAXANGLE - 1);

        c0 = icos[d0];
        s0 = isin[d0];
        c1 = icos[d1];
        s1 = isin[d1];

        x = (pdot->zm * s0 + pdot->xm * c0) / MAXFIXED;
        z = (pdot->zm * c0 - pdot->xm * s0) / MAXFIXED;

        y = (z * s1 + pdot->ym * c1) / MAXFIXED;
        z = (z * c1 - pdot->ym * s1) / MAXFIXED;

        pdot->xm = x;
        pdot->ym = y;
        pdot->zm = z;

        pdot->xn = 0;
        pdot->yn = 0;
        pdot->zn = 0;
    }
}


/****************************************************************************/

#define DELTA0 47
#define DELTA1 30
#define DELTA2 40

void DrawFrameShaded(
    PWINDOW pwindow)
{
    static int deg0 = 0, deg1 = 0, deg2 = 0, deg3 = 0;
    int i, j;
    int c0, c1, sizetext;
    int s0, s1, sizeball;
    int x, y, z;
    int xs, ys, zs;
    PBYTE pdata = pwindow->pdata;
    BYTE color;
    int d0, d1, d2;
    PRECT prc = &(pwindow->rcDraw);

    ClearRect(pwindow->pdata, &(pwindow->rcDraw));

    pwindow->rcBlt = pwindow->rcDraw;

    prc->left = XSIZE;
    prc->right = 0;
    prc->top = YSIZE;
    prc->bottom = 0;

    //
    // draw this frame
    //

    for (j = 0; j < 1; j++) {
        d0 = WrapPlus(deg0 + j * DELTA0, MAXANGLE);
        d0 = Bound((icos[d0] * MAXANGLE / 2) / MAXFIXED, MAXANGLE);
        c0 = icos[d0];
        s0 = isin[d0];

        d1 = WrapPlus(deg1 + j * DELTA1, MAXANGLE);
        c1 = icos[d1];
        s1 = isin[d1];

        d2 = WrapPlus(deg2 + MAXANGLE * 3 / 4, MAXANGLE);

        sizeball = (icos[d2] + MAXFIXED) / 2;
        sizetext = (isin[d2] + MAXFIXED) / 2;

        color = 245;

        /*
         * rotate verticies
         */
        for (i = 0; i < NUMDOTS; i++) {
            PDOT pdot = adot + i;
            PBYTE pbyte;

            xs = pdot->xm;
            ys = pdot->ym;
            zs = pdot->zm;

            x = (zs * s0 + xs * c0) / MAXFIXED;
            z = (zs * c0 - xs * s0) / MAXFIXED;

            y = (z * s1 + ys * c1) / MAXFIXED;
            z = (z * c1 - ys * s1) / MAXFIXED;

            x = (x * sizeball + pdot->xn * sizetext) / MAXFIXED;
            y = (y * sizeball + pdot->yn * sizetext) / MAXFIXED;
            z = (z * sizeball + pdot->zn * sizetext) / MAXFIXED;

            x += XSIZE2;
            y += YSIZE2;

            if (x < 0)         x = 0;
            if (x > XSIZE - 2) x = XSIZE - 2;
            if (y < 0)         y = 0;
            if (y > YSIZE - 2) y = YSIZE - 2;

            if (x < prc->left) prc->left = x;
            if (x+2 > prc->right) prc->right = x+2;

            if (y < prc->top) prc->top = y;
            if (y+2 > prc->bottom) prc->bottom = y+2;

            pbyte = pdata + x + y * SCANSIZE;

            pbyte[0] = color;
            pbyte[1] = color;
            pbyte[SCANSIZE] = color;
            pbyte[SCANSIZE + 1] = color;
        }
    }

    //
    // next frame
    //

    deg0 = WrapPlus(deg0 + DELTA0, MAXANGLE);
    deg1 = WrapPlus(deg1 + DELTA1, MAXANGLE);

    deg2 -= DELTA2;
    if (deg2 < 0) {
        deg2 += MAXANGLE;
    }

    UnionRects(&(pwindow->rcBlt), &(pwindow->rcDraw));
    pwindow->xDelta = pwindow->rcBlt.left;
    pwindow->yDelta = pwindow->rcBlt.top;
}

/****************************************************************************/

void InitDrawingThread(
    PWINDOW pwindow)
{
    int i;

    for (i = 0; i < MAXANGLE; i++) {
        double rad = i * (2.0 * PI / MAXANGLE);
        icos[i] = (int)(cos(rad) * MAXFIXED);
        isin[i] = (int)(sin(rad) * MAXFIXED);
    }
}

/****************************************************************************/

void BltThread(
    PWINDOW pwindow)
{
    PRECT prc = &(pwindow->rcBlt);

    GdiSetBatchLimit(1);
    InitDrawingThread(pwindow);
    InitDrawShaded(pwindow);

    while (TRUE) {
        if (fRepaint) {
            RECT rc;
            rc.left = 0;
            rc.right = pwindow->xsize;
            rc.top = 0;
            rc.bottom = pwindow->ysize;
            FillRect(pwindow->hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
            fRepaint = FALSE;
        }

        DrawFrameShaded(pwindow);

        BitBlt(
            pwindow->hdc,
            prc->left, prc->top,
            prc->right - prc->left, prc->bottom - prc->top,
            pwindow->hdcBitmap,
            pwindow->xDelta,
            pwindow->yDelta, SRCCOPY);

        WaitForSingleObject(pwindow->hWait, INFINITE);
    }
}

/****************************************************************************/

void vCleanSystemPalette(HDC hdc)
{
    HPALETTE hpal,hpalOld;
    DWORD aTemp[257];
    LPLOGPALETTE lpLogPal;
    UCHAR iTemp;

    lpLogPal = (LPLOGPALETTE) aTemp;
    lpLogPal->palVersion = 0x300;
    lpLogPal->palNumEntries = 256;

    for (iTemp = 0; iTemp < 256; iTemp++)
    {
        lpLogPal->palPalEntry[iTemp].peRed   = 0;
        lpLogPal->palPalEntry[iTemp].peGreen = 0;
        lpLogPal->palPalEntry[iTemp].peBlue  = iTemp;
        lpLogPal->palPalEntry[iTemp].peFlags = PC_RESERVED;
    }

    hpal = CreatePalette(lpLogPal);
    hpalOld = SelectPalette(hdc, hpal, 0);
    RealizePalette(hdc);
    SelectPalette(hdc, hpalOld, 0);
    DeleteObject(hpal);
}

/****************************************************************************/

void InitDibSection(
    PWINDOW pwindow,
    RGBQUAD* ppal,
    BOOL fSystemPalette)
{
    LPLOGPALETTE plp = (LPLOGPALETTE)LocalAlloc(LPTR, sizeof(LOGPALETTE) + 4 * 256);
    LPBITMAPINFO pbmi = (LPBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + 4 * 256);
    int i;
    UCHAR iTemp;
    PUSHORT pw;
    LOGFONT lf;
    HFONT hfont;

    plp->palVersion = 0x300;
    plp->palNumEntries = 256;

    if (fSystemPalette) {
        GetPaletteEntries((HPALETTE)GetStockObject(DEFAULT_PALETTE),
                          0, 1, plp->palPalEntry);
        GetPaletteEntries((HPALETTE)GetStockObject(DEFAULT_PALETTE),
                          255, 1, &plp->palPalEntry[255]);
        for (i = 1; i < 254; i++) {
            plp->palPalEntry[i].peRed   = ppal[i].rgbRed;
            plp->palPalEntry[i].peGreen = ppal[i].rgbGreen;
            plp->palPalEntry[i].peBlue  = ppal[i].rgbBlue;
            plp->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
        }
        SetSystemPaletteUse(pwindow->hdc, SYSPAL_NOSTATIC);
    } else {
        GetPaletteEntries((HPALETTE)GetStockObject(DEFAULT_PALETTE),
                          0, 10, plp->palPalEntry);
        GetPaletteEntries((HPALETTE)GetStockObject(DEFAULT_PALETTE),
                          246, 10, &plp->palPalEntry[255]);

        for (i = 10; i < 246; i++) {
            plp->palPalEntry[i].peRed   = ppal[i].rgbRed;
            plp->palPalEntry[i].peGreen = ppal[i].rgbGreen;
            plp->palPalEntry[i].peBlue  = ppal[i].rgbBlue;
            plp->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
        }
    }

    pwindow->hpalette = CreatePalette(plp);
    vCleanSystemPalette(pwindow->hdc);
    SelectPalette(pwindow->hdc, pwindow->hpalette, FALSE);
    RealizePalette(pwindow->hdc);

    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = XSIZE;
    pbmi->bmiHeader.biHeight        = -YSIZE;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = 8;
    pbmi->bmiHeader.biCompression   = BI_RGB;
    pbmi->bmiHeader.biSizeImage     = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed       = 0;
    pbmi->bmiHeader.biClrImportant  = 0;

    pw = (PUSHORT)(pbmi->bmiColors);
    for (iTemp=0; iTemp<256; iTemp++) {
        pw[iTemp] = iTemp;
    }

    pwindow->hbitmap = CreateDIBSection(
        pwindow->hdc, pbmi, DIB_PAL_COLORS,
        (PVOID*)&(pwindow->pdata), 0, 0);
    pwindow->hdcBitmap = CreateCompatibleDC(pwindow->hdc);
    SelectObject(pwindow->hdcBitmap, pwindow->hbitmap);

    lf.lfHeight = 30;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 400;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
    lstrcpy(lf.lfFaceName, TEXT("Arial"));

    hfont = CreateFontIndirect(&lf);

    pwindow->hbitmapText = CreateDIBSection(
        pwindow->hdc, pbmi, DIB_PAL_COLORS,
        (PVOID*)&(pwindow->pdataText), 0, 0);
    pwindow->hdcText = CreateCompatibleDC(pwindow->hdcText);
    SelectObject(pwindow->hdcText, pwindow->hbitmapText);
    SelectObject(pwindow->hdcText, hfont);
}

/****************************************************************************/

void Init(
    HWND hwnd)
{
    int i;
    RGBQUAD apal[256];
    DWORD tid;

    srand(GetTickCount());

    adot = (PDOT)LocalAlloc(LPTR, sizeof(DOT) * NUMDOTS);
    isin = (int *)LocalAlloc(LPTR, sizeof(int) * MAXANGLE);
    icos = (int *)LocalAlloc(LPTR, sizeof(int) * MAXANGLE);

    gpwindow = (PWINDOW)LocalAlloc(LPTR, sizeof(WINDOW));

    gpwindow->hwnd = hwnd;
    gpwindow->xsize = gcxScreen;
    gpwindow->ysize = gcyScreen;

    gpwindow->hdc = ghdc;
    SetBkColor(gpwindow->hdc, 0);
    SetTextColor(gpwindow->hdc, RGB(0xff, 0xff, 0xff));

    gpwindow->hWait = CreateEvent(NULL, FALSE, FALSE, NULL);
    SetTimer(gpwindow->hwnd, 1, 1000 / 20, NULL);

    for (i = 0; i < 236; i++) {
        apal[i + 10].rgbRed   = (i * 255) / 235;
        apal[i + 10].rgbGreen = (i * 255) / 235;
        apal[i + 10].rgbBlue  = (i * 255) / 235;
    }

    InitDibSection(gpwindow, apal, FALSE);

    CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)BltThread,
        gpwindow,
        0,
        &tid);
}
