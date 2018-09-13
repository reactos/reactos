/*
marquee.c
This is a screen saver that can easily be added onto...

  History:
       6/17/91        stevecat    ported to NT Windows
       2/10/92        stevecat    snapped to latest Win3.1 sources
*/

#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>
#include <scrnsave.h>
#include "marquee.h"
#include "strings.h"
#include "uniconv.h"


#define MulDiv(a, b, c)  ((int)(a) * (int)(b) / (int)(c))

typedef struct
{
    HWND        hDlg;
    WORD        wID;
    HDC         hDC;
} INFOSTRUCT;
typedef INFOSTRUCT far * LPINFOSTRUCT;

UINT PWM_NEWSPEED;
UINT PWM_NEWPOSITION;
#define BUFFER_LEN        255
#define COUNT             2
#define MAX_SPEED         10
#define DEF_SPEED         10
#define DIV_SPEED         3

#define NATTRIBUTES       5
#define UNDERLINE         0
#define STRIKEOUT         1
#define ITALIC            2
#define MODE              3
#define BOLD              4

#define DEFAULT_TEXT_COLOR      RGB(255,0,255)
#define DEFAULT_SCREEN_COLOR    RGB(0,0,0)

TCHAR szDefaultText[BUFFER_LEN];         // Buffer for default Marquee text
TCHAR szFormatText[TITLEBARNAMELEN];     // Name in font formatting dlg.

TCHAR szFontName[]=TEXT("Font");               // CONTROL.INI key values
TCHAR szSizeName[]=TEXT("Size");
TCHAR szTextName[]=TEXT("Text");
TCHAR szTColorName[]=TEXT("TextColor");
TCHAR szBColorName[]=TEXT("BackgroundColor");
TCHAR szAttributes[]=TEXT("Attributes");
TCHAR szSpeedName[]=TEXT("Speed");
TCHAR szCharSetName[]=TEXT("CharSet");
TCHAR szShowTextName[]=TEXT("showtext");

TCHAR szBuffer[BUFFER_LEN];              // Text to display in Marquee
TCHAR szFaceName[LF_FACESIZE];           // Font face name to use...
TCHAR szDefFontName[LF_FACESIZE];
BOOL fMode=FALSE;                       // Mode of ScreenSaver
TCHAR fUnderline=TEXT('0');
TCHAR fStrikeOut=TEXT('0');
TCHAR fItalic=TEXT('0');
TCHAR fBold=TEXT('0');
HFONT hfontMessage = NULL;
DWORD dwTColor;                         // Global text color
DWORD dwBColor;                         // Global background color
BYTE bCharSet;
DWORD dwRand = 1L;

#define RAND(x)   ((rand() % ((x == 0) ? 1 : x)) + 1)
#define ZRAND(x)  (rand() % ((x == 0) ? 1 : x))

// Function prototypes...

void  srand (DWORD);
WORD  rand (void);
LRESULT APIENTRY ShowTextProc (HWND, UINT, WPARAM, LPARAM);
int   GetHeightFromPointSize (int);
void  FillR (HDC, LPRECT, DWORD);
void  FrameR (HDC, LPRECT, DWORD, int);
void  PatB (HDC, int, int, int, int, DWORD);
void  GetAttributes (void);
DWORD GetProfileRgb (LPTSTR, LPTSTR, DWORD);
WORD  AtoI (LPTSTR);
BOOL  APIENTRY ChooseFontHookProc (HWND, UINT, DWORD, LONG);

//
// Help IDs
//
DWORD aMarqueeDlgHelpIds[] = {
    ((DWORD) -1), ((DWORD) -1),
    ID_FORMATTEXT,          IDH_DISPLAY_SCREENSAVER_MARQUEE_FORMAT_TEXT,
    ID_CENTERED,            IDH_DISPLAY_SCREENSAVER_MARQUEE_POSITION_CENTERED,
    ID_RANDOM,              IDH_DISPLAY_SCREENSAVER_MARQUEE_POSITION_RANDOM,
    ID_BGROUNDCOLOR_LABEL,  IDH_DISPLAY_SCREENSAVER_MARQUEE_BACKGROUND_COLOR,
    ID_BGROUNDCOLOR,        IDH_DISPLAY_SCREENSAVER_MARQUEE_BACKGROUND_COLOR,
    ID_SPEED_SLOW,          IDH_DISPLAY_SCREENSAVER_MARQUEE_SPEED,
    ID_SPEED_FAST,          IDH_DISPLAY_SCREENSAVER_MARQUEE_SPEED,
    ID_SPEED,               IDH_DISPLAY_SCREENSAVER_MARQUEE_SPEED,
    ID_MARQUEETEXT_LABEL,   IDH_DISPLAY_SCREENSAVER_MARQUEE_TEXT,
    ID_MARQUEETEXT,         IDH_DISPLAY_SCREENSAVER_MARQUEE_TEXT,
    ID_TEXTWINDOW,          IDH_DISPLAY_SCREENSAVER_MARQUEE_TEXT_EXAMPLE,
    0,0
};

//***************************************************************************

//
// This function returns TRUE, if szBuffer includes DBCS, otherwise FALSE.
// #425:12/21/92:fixing DBCS dispatch automatically
//
// ToddB: all DBCS and CodePage issues are handled by first calling this function.
// To do the FE single binary merge I'm simply calling this function always (instead
// of only in Far East builds).  If this function returns FALSE then the remaining
// code path is identical to the old US version.
BOOL FAR PASCAL IsTextIncludeDBCSChar(void)
{
    static BOOL bDBCS = -1;
    int   i, len = lstrlen(szBuffer) ;
    CHAR  c;
    CHAR  szb[BUFFER_LEN*sizeof(TCHAR)];

    // Use lazy initialization since I have multiple the entry points which vary
    // depending on what message handlers are processed in the WndProc
    if ( -1 == bDBCS )
        bDBCS = GetSystemMetrics( SM_DBCSENABLED );

    // if we are not using a DBCS version of user.exe then nothing should
    // be treated as a DBCS character.
    if (!bDBCS)
        return FALSE;

    if (sizeof(TCHAR) == sizeof(CHAR))
    {
        // same size, just copy.  The cast is valid due to the above check and
        // it keeps the compiler happy
        lstrcpy( (TCHAR *)szb, szBuffer );
    }
    else
    {
        // szBuffer is UNICODE, we convert it to DBCS before checking for lead bytes.
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK,
		szBuffer, len+1,
		szb, BUFFER_LEN*sizeof(TCHAR),
		NULL, NULL );
    }

    for (i = 0;i < len;i++) {
        c = szb[i] ;

        if (IsDBCSLeadByte(c)) {
            return TRUE ;
        }
/* hankaku katakana JAPAN only */
        else if (GetACP() == 932 && c >= 0xa0 && c <  0xe0) {
            return TRUE ;
        }
    }
    return FALSE ;
}

static CHARSETINFO csi;

void LoadStrings(void)
{
    TCHAR szTmp[BUFFER_LEN];
    OSVERSIONINFO osi;

    // This simply fills a CHARSETINFO structure with data about the code page
    DWORD dw = GetACP();
    if (!TranslateCharsetInfo((DWORD*)dw, &csi, TCI_SRCCODEPAGE))
        csi.ciCharset = ANSI_CHARSET;

    LoadString (hMainInstance, idsName, szName, CharSizeOf(szName));
    LoadString (hMainInstance, idsAppName, szAppName, CharSizeOf(szAppName));

    // Get OS Version
    LoadString (hMainInstance, idsDefaultText, szTmp, CharSizeOf(szTmp));
    osi.dwOSVersionInfoSize = sizeof(osi);
    if (!GetVersionEx(&osi)) {
        osi.dwMajorVersion = 4;
        osi.dwMinorVersion = 0;
    }
    wsprintf( szDefaultText, szTmp, osi.dwMajorVersion, osi.dwMinorVersion );

    LoadString (hMainInstance, idsIniFile, szIniFile, CharSizeOf(szIniFile));
    LoadString (hMainInstance, idsScreenSaver, szScreenSaver, CharSizeOf(szScreenSaver));
    LoadString (hMainInstance, idsHelpFile, szHelpFile, CharSizeOf(szHelpFile));
    LoadString (hMainInstance, idsNoHelpMemory, szNoHelpMemory, CharSizeOf(szNoHelpMemory));
    LoadString (hMainInstance, idsFormatText, szFormatText, CharSizeOf(szFormatText));
    LoadString (hMainInstance, idsDefFontName, szDefFontName, CharSizeOf(szDefFontName));
}

//***************************************************************************

/* This is the main window procedure to be used when the screen saver is
    activated in a screen saver mode ( as opposed to configure mode ).  This
    function must be declared as an EXPORT in the EXPORTS section of the
    DEFinition file... */

LRESULT APIENTRY ScreenSaverProc(hWnd, message, wParam, lParam)
HWND   hWnd;
UINT   message;
WPARAM wParam;
LPARAM lParam;
{
RECT                rRect;
static int          wSize;
static WORD         wHeight;
static UINT_PTR     wTimer;
static WORD         wX;
static WORD         wY;
static WORD         wCount;
static SIZE         sizeExtent;
static int          wLength;
static WORD         wSpeed;
static WORD         wVelocity;
static HBRUSH       hbrTemp;
static TEXTMETRIC   tm;
static BOOL         bMELocale;
HBRUSH              hbrOld;
HFONT               hfontOld;
HDC                 hDC;
DWORD               dwLocale;
UINT                uiETOFlags;

    switch(message)
    {
        case WM_CREATE:
            LoadStrings ();
            GetAttributes();
            /* Get the info necessary to create the font... */
            GetPrivateProfileString (szAppName, szFontName, szDefFontName, szFaceName,
                                     CharSizeOf(szFaceName), szIniFile);
            bCharSet = (BYTE)GetPrivateProfileInt (szAppName,szCharSetName,
                                                    (WORD)ANSI_CHARSET, szIniFile);
	    if( IsTextIncludeDBCSChar() )
            {
                bCharSet = (BYTE)csi.ciCharset;
            }

            hDC = GetDC (NULL);

            //  See if the user locale id is Arabic or Hebrew.
            dwLocale    = GetUserDefaultLCID();
            bMELocale = ((PRIMARYLANGID(LANGIDFROMLCID(dwLocale)) == LANG_ARABIC) ||
                (PRIMARYLANGID(LANGIDFROMLCID(dwLocale)) == LANG_HEBREW));

            /* Get the dimensions of the entire virtual screen... */
            wX = (WORD)((LPCREATESTRUCT)lParam)->cx;
            wY = (WORD)((LPCREATESTRUCT)lParam)->cy;

            wSize = GetPrivateProfileInt (szAppName, szSizeName, 0, szIniFile);
            // wSize is in POINTS, we need to convert it to LogicalUnits...
            wSize = GetHeightFromPointSize (wSize);

            if (fChildPreview) {
                // Scale font down to fit in preview window
                wSize = (wSize * wY) / GetDeviceCaps(hDC, VERTRES);
            }

            hfontMessage = CreateFont (wSize, 0, 0, 0,
                                       (fBold == TEXT('0')) ? FW_NORMAL : FW_BOLD,
                                       (fItalic == TEXT('0')) ? 0 : 1,
                                       (fUnderline == TEXT('0')) ? 0 : 1,
                                       (fStrikeOut == TEXT('0')) ? 0 : 1,
                                       bCharSet,
                                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                       DEFAULT_PITCH|FF_DONTCARE, szFaceName);

            /* Get the text to display and figure out how long it is... */
            GetPrivateProfileString (szAppName, szTextName, szDefaultText, szBuffer,
                                     CharSizeOf(szBuffer), szIniFile);
            lstrcat (szDefaultText, TEXT("     "));
            wLength = lstrlen (szBuffer);
            hfontOld = SelectObject (hDC,hfontMessage);
            GetTextExtentPoint32 (hDC, szBuffer, wLength, &sizeExtent);

            if (fChildPreview)
                sizeExtent.cx *= 2;

            GetTextMetrics (hDC,&tm);
            if (hfontOld)
                SelectObject (hDC,hfontOld);

            ReleaseDC (NULL,hDC);
            if (bMELocale) {
                wCount = (WORD)(0 - sizeExtent.cy + 1);
            } else {
                wCount = wX;
            }
            srand(GetCurrentTime());

            /* set everything up... */
            if(fMode)
                wHeight = (WORD) ZRAND(wY - sizeExtent.cy);
            else
                wHeight = (WORD) (wY - sizeExtent.cy)/2;

            if ((int)(wSpeed = (WORD) GetPrivateProfileInt (szAppName, szSpeedName, DEF_SPEED, szIniFile))
                < 1)
                wSpeed = 1;
            if (wSpeed > (MAX_SPEED * DIV_SPEED))
                wSpeed = MAX_SPEED * DIV_SPEED;

            dwTColor = GetProfileRgb(szAppName,szTColorName,DEFAULT_TEXT_COLOR);
            dwBColor = GetProfileRgb(szAppName,szBColorName,DEFAULT_SCREEN_COLOR);
            hbrTemp = CreateSolidBrush(dwBColor);

            /* Set the timer... */
            wTimer = SetTimer(hWnd,9,1,NULL);
            break;

        case WM_SIZE:
            wX = LOWORD(lParam);
            wY = HIWORD(lParam);
            break;

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
            GetClientRect (hWnd, &rRect);
            FillRect ((HDC)wParam, &rRect, hbrTemp);
            return 0l;

        case WM_TIMER:
        {
            RECT rc;

            // NOTE:  For Win32 the casting of these quantities is extremely
            //        important.  The original code was very sloppy and just
            //        made everything WORD (even for signed quantities).  We
            //        must use proper casting here to get around these coding
            //        ERRORS!!
            //           [stevecat]

            rc.top    = (int)(short) wHeight;
            rc.left   = (int)(short) wCount - tm.tmMaxCharWidth;
            rc.bottom = (int)(short) wHeight + sizeExtent.cy + (sizeExtent.cy >> 3); //Some fonts leave a trail
            rc.right  = (int)(short) wCount + sizeExtent.cx + (wVelocity / DIV_SPEED) +
                                  1 + tm.tmMaxCharWidth * 2;

            /* Add the new increment to the timer count, if we have not reached
                the integral part of the count, wait until we do... */
            wVelocity += wSpeed;
            if(wVelocity < DIV_SPEED)
                break;
            hDC = GetDC(hWnd);
            hfontOld = SelectObject(hDC,hfontMessage);
            SetTextColor(hDC,dwTColor);
            SetBkColor(hDC,dwBColor);

            uiETOFlags = ETO_OPAQUE;
            
            if (bMELocale) {
              uiETOFlags |= ETO_RTLREADING;
            }

            ExtTextOut(hDC, (int)(short)wCount, wHeight, uiETOFlags,
                                            &rc, szBuffer, wLength, NULL);
                                            
            if (hfontOld)
                SelectObject(hDC,hfontOld);

            if (bMELocale) { // Arabic/Hebrew Locale
                if((short)wCount < (short) wX)
                   wCount += (wVelocity/DIV_SPEED)+1;
                else
                {
                    wCount = (WORD)(0 - sizeExtent.cx + 1);
                    if(fMode)
                       wHeight = (WORD) ZRAND(wY - sizeExtent.cy);
                 }

            } else {

                /* Increment so it is ready for the next pass... */
                if((short)wCount >= (short)(0-sizeExtent.cx))
                   wCount -= (wVelocity/DIV_SPEED)+1;
                else
                {
                    hbrOld = SelectObject(hDC,hbrTemp);
                    //  The wSize variable is some bogus value left over during WM_CREATE and
                    //  doesn't seem to have any connection to where the PatBlt should start
                    //  in the X direction.  Replacing this value with 0 fixes bug #5415
                    //                PatBlt(hDC,(int)(short)wSize, (int)(short)wHeight,
                    PatBlt(hDC, 0, (int)(short)wHeight,
                             ((wVelocity/DIV_SPEED)+1)*1+tm.tmMaxCharWidth*2,
                               sizeExtent.cy, PATCOPY);
                    if (hbrOld)
                       SelectObject(hDC,hbrOld);
                    wCount = wX;
                    if(fMode)
                       wHeight = (WORD) ZRAND(wY - sizeExtent.cy);
                 }
            }
            ReleaseDC(hWnd,hDC);

            wVelocity = wVelocity % DIV_SPEED;
            break;
        }
        case WM_DESTROY:
            /* Anything that needs to be deleted when the window is closed
                goes here... */
            if(wTimer)
                KillTimer(hWnd,wTimer);
            if(hfontMessage)
                DeleteObject(hfontMessage);
            DeleteObject(hbrTemp);
            break;
    }
    /* Unless it is told otherwise, the program will take default actions... */
    return (DefScreenSaverProc(hWnd,message,wParam,lParam));
}

//***************************************************************************

/*  This is where the code for the configure dialog box goes. It is a typical
    dialog box. The corresponding resource that is loaded is called
    'ScreenSaverConfigure' and is located in the ResourceCompiler file.
    Minimally (as in this case), this functions as an about box.  In this
    case, we also get the applications icon which must be defined as
    ID_APP... */

BOOL APIENTRY ScreenSaverConfigureDialog(hDlg, message, wParam, lParam)
HWND   hDlg;
UINT   message;
WPARAM wParam;
LPARAM lParam;
{
UINT            wTemp,wPal =0;
static int      wSize;              // current font size selected.
HPALETTE        hPal;
RECT            rc;
static HWND     hIDOK, hSetPassword;
HDC             hDC;
static LOGFONT  lfFont;
CHOOSEFONT      chfChooseFont;
FARPROC         lpfp;

static HFONT hfontPrev, hFontSave;
static LOGFONT lfFontPrev;

    switch(message)
    {
        case WM_INITDIALOG:
            PWM_NEWSPEED = RegisterWindowMessage(TEXT("PWM_NEWSPEED"));
            PWM_NEWPOSITION = RegisterWindowMessage(TEXT("PWM_NEWPOSITION"));

            LoadStrings ();
            GetAttributes ();
            hIDOK = GetDlgItem (hDlg, IDOK);

            /* Fill up both of the color combo boxes and select the right
                entries... */
            hPal = GetStockObject (DEFAULT_PALETTE);
            GetObject (hPal, sizeof(int), (LPTSTR)&wPal);
            for (wTemp = 0; wTemp < wPal; wTemp++)
                SendDlgItemMessage (hDlg, ID_BGROUNDCOLOR, CB_ADDSTRING, 0,
                                    (LPARAM)TEXT("a"));

            dwBColor = GetProfileRgb (szAppName, szBColorName, DEFAULT_SCREEN_COLOR);
            wTemp = GetNearestPaletteIndex (hPal,dwBColor);
            SendDlgItemMessage (hDlg, ID_BGROUNDCOLOR, CB_SETCURSEL, wTemp, 0l);
            GetPaletteEntries (hPal, wTemp, 1, (LPPALETTEENTRY)(LPDWORD)&dwBColor);

            /* Get the mode of the marquee... */
            CheckRadioButton (hDlg,ID_CENTERED,ID_RANDOM,
                              fMode ? ID_RANDOM : ID_CENTERED);
            SendDlgItemMessage (hDlg, ID_TEXTWINDOW, PWM_NEWPOSITION, fMode, 0l);

            /* Set up the scroll bar to take care of speed... */
            SetScrollRange (GetDlgItem (hDlg,ID_SPEED), SB_CTL, 1, MAX_SPEED * DIV_SPEED,
                            FALSE);
            if ((wTemp = GetPrivateProfileInt (szAppName, szSpeedName, DEF_SPEED, szIniFile))
                < 1)
                wTemp = 1;
            if (wTemp > (MAX_SPEED * DIV_SPEED))
                wTemp = MAX_SPEED * DIV_SPEED;
            SetScrollPos (GetDlgItem (hDlg,ID_SPEED), SB_CTL, wTemp, TRUE);
            SendDlgItemMessage (hDlg, ID_TEXTWINDOW, PWM_NEWSPEED, wTemp, 0l);

            /* Get the text from the .INI file entry and set up the edit box
                where the user enters the text to display... */
            SendDlgItemMessage (hDlg, ID_MARQUEETEXT, EM_LIMITTEXT, CharSizeOf(szBuffer) - 1, 0l);
            GetPrivateProfileString (szAppName, szTextName, szDefaultText, szBuffer,
                                     CharSizeOf(szBuffer), szIniFile);
            SetWindowText (GetDlgItem (hDlg, ID_MARQUEETEXT), szBuffer);

            /* Get the info necessary to create the font... */
            GetPrivateProfileString (szAppName, szFontName, szDefFontName, szFaceName,
                                     CharSizeOf(szFaceName), szIniFile);
            bCharSet = (BYTE)GetPrivateProfileInt (szAppName, szCharSetName,
                                                 (WORD)ANSI_CHARSET, szIniFile);

            if( IsTextIncludeDBCSChar() )
            {
                bCharSet = (BYTE) csi.ciCharset;
            }

            wSize = GetPrivateProfileInt (szAppName, szSizeName, 10, szIniFile);
            // wSize is in POINTS, we need to convert it to LogicalUnits...
            wSize = GetHeightFromPointSize (wSize);

            hfontMessage = CreateFont(wSize,0,0,0,
                (fBold     ==TEXT('0'))?FW_NORMAL:FW_BOLD,
                (TCHAR)((fItalic   ==TEXT('0'))?0:1),
                (TCHAR)((fUnderline==TEXT('0'))?0:1),
                (TCHAR)((fStrikeOut==TEXT('0'))?0:1),
                bCharSet,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                DEFAULT_PITCH|FF_DONTCARE,szFaceName);
            dwTColor = GetProfileRgb(szAppName,szTColorName,DEFAULT_TEXT_COLOR);

            lfFont.lfWeight   =(fBold     ==TEXT('0'))?FW_NORMAL:FW_BOLD,
            lfFont.lfItalic   =(fItalic   ==TEXT('0'))?(TCHAR)0:(TCHAR)1,
            lfFont.lfUnderline=(fUnderline==TEXT('0'))?(TCHAR)0:(TCHAR)1,
            lfFont.lfStrikeOut=(fStrikeOut==TEXT('0'))?(TCHAR)0:(TCHAR)1,
            lfFont.lfHeight=(LONG)wSize;
            lstrcpy(lfFont.lfFaceName, szFaceName);

            return TRUE;

        case WM_HSCROLL:
            wTemp = GetScrollPos(GetDlgItem(hDlg,ID_SPEED),SB_CTL);
            switch(LOWORD(wParam))
            {
                case SB_PAGEDOWN:
                    wTemp += (DIV_SPEED-1);
                case SB_LINEDOWN:
                    wTemp += 1;
                    wTemp = min(MAX_SPEED*DIV_SPEED,wTemp);
                    break;
                case SB_PAGEUP:
                    wTemp -= (DIV_SPEED-1);
                case SB_LINEUP:
                    wTemp -= 1;
                    wTemp = max(1,(int)wTemp);
                    break;
                case SB_THUMBPOSITION:
                    wTemp = HIWORD(wParam);
                    break;
            }
            SetScrollPos(GetDlgItem(hDlg,ID_SPEED),SB_CTL,wTemp,TRUE);
            SendDlgItemMessage(hDlg,ID_TEXTWINDOW,PWM_NEWSPEED,wTemp,0l);
            break;

        case WM_MEASUREITEM:
            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = 12;
            return TRUE;

        case WM_DRAWITEM:
            rc    = ((LPDRAWITEMSTRUCT)lParam)->rcItem;

            if (((LPDRAWITEMSTRUCT)lParam)->itemState & ODS_SELECTED)
            {
                FrameR(((LPDRAWITEMSTRUCT)lParam)->hDC,&rc,RGB(0,0,0),2);
                InflateRect(&rc,-1,-1);
                FrameR(((LPDRAWITEMSTRUCT)lParam)->hDC,&rc,RGB(255,255,255),2);
                InflateRect(&rc,-1,-1);
            }
            FillR(((LPDRAWITEMSTRUCT)lParam)->hDC,&rc,PALETTEINDEX
                (((LPDRAWITEMSTRUCT)lParam)->itemID));
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case ID_MARQUEETEXT:
                    if(HIWORD(wParam) == EN_UPDATE)
                    {
                        GetDlgItemText (hDlg, ID_MARQUEETEXT, szBuffer, CharSizeOf(szBuffer));

                        if (IsTextIncludeDBCSChar()) {
                            if (lfFont.lfCharSet != csi.ciCharset) {
                                if (hfontPrev) {
                                    if (hfontMessage)
                                        DeleteObject(hfontMessage);
                                    // Restore old font imformation
                                    hfontMessage = hfontPrev;
                                    lfFont = lfFontPrev;
                                    hfontPrev = NULL;
                                }
                                else {
                                    // Save old font imformation
                                    hfontPrev = hfontMessage;
                                    lfFontPrev = lfFont;
                                    lfFont.lfCharSet = (BYTE) csi.ciCharset;
                                    hfontMessage = CreateFontIndirect((LPLOGFONT)&lfFont);
                                }
                                SendDlgItemMessage(hDlg, ID_TEXTWINDOW, PWM_NEWPOSITION, fMode, 0l);
                                InvalidateRect(GetDlgItem(hDlg, ID_TEXTWINDOW), NULL, TRUE);
                            }
                        }
                        else {
                            if (lfFont.lfCharSet == csi.ciCharset) {
                                if (hfontPrev) {
                                    if (hfontMessage)
                                        DeleteObject(hfontMessage);
                                    // Restore old font imformation
                                    hfontMessage = hfontPrev;
                                    lfFont = lfFontPrev;
                                    hfontPrev = NULL;
                                    SendDlgItemMessage(hDlg, ID_TEXTWINDOW, PWM_NEWPOSITION, fMode, 0l);
                                    InvalidateRect(GetDlgItem(hDlg, ID_TEXTWINDOW), NULL, TRUE);
                                }
                            }
                        }
                        bCharSet = lfFont.lfCharSet;
                        SetDlgItemText (hDlg, ID_TEXTWINDOW, szBuffer);
                    }
                    break;

                case ID_CENTERED:
                case ID_RANDOM:
                    fMode=(wParam!=ID_CENTERED);
                    SendDlgItemMessage(hDlg,ID_TEXTWINDOW,PWM_NEWPOSITION,fMode,0l);
                    CheckRadioButton(hDlg,ID_CENTERED,ID_RANDOM,LOWORD(wParam));
                    break;

                case ID_FORMATTEXT:
                    hDC = GetDC(hDlg);

                    hFontSave = SelectObject( hDC, hfontMessage );
                    GetTextFace( hDC, LF_FACESIZE, lfFont.lfFaceName );
                    SelectObject( hDC, hFontSave );

                    chfChooseFont.lStructSize = sizeof (CHOOSEFONT);
                    chfChooseFont.hwndOwner = hDlg;
                    chfChooseFont.hDC = hDC;
                    chfChooseFont.lpLogFont = &lfFont;
                    chfChooseFont.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
                                          CF_LIMITSIZE | CF_EFFECTS | CF_ENABLEHOOK;
                    chfChooseFont.rgbColors = dwTColor;
                    chfChooseFont.lCustData = 0L;
                    chfChooseFont.lpfnHook = (LPCFHOOKPROC)ChooseFontHookProc;
                    chfChooseFont.lpTemplateName = (LPTSTR)NULL;
                    chfChooseFont.hInstance = (HANDLE) NULL;
                    chfChooseFont.lpszStyle = (LPTSTR) NULL;
                    chfChooseFont.nFontType = SCREEN_FONTTYPE;
                    chfChooseFont.nSizeMin = 8;
                    chfChooseFont.nSizeMax = 200;
                    if (ChooseFont(&chfChooseFont))
                    {
                        lstrcpy(szFaceName, lfFont.lfFaceName);
                        wSize     =lfFont.lfHeight;
                        dwTColor  =chfChooseFont.rgbColors;
                        fStrikeOut=(lfFont.lfStrikeOut)?(TCHAR)TEXT('1'):(TCHAR)TEXT('0');
                        fUnderline=(lfFont.lfUnderline)?(TCHAR)TEXT('1'):(TCHAR)TEXT('0');
                        fItalic   =(lfFont.lfItalic)   ?(TCHAR)TEXT('1'):(TCHAR)TEXT('0');
                        fBold     =(lfFont.lfWeight==FW_NORMAL)?(TCHAR)TEXT('0'):(TCHAR)TEXT('1');
                        bCharSet  =lfFont.lfCharSet;

                        if (hfontPrev)
                        {
                            DeleteObject(hfontPrev);
                            hfontPrev = NULL;
                        }

                        if (hfontMessage)
                           DeleteObject(hfontMessage);

                        hfontMessage = CreateFontIndirect((LPLOGFONT)&lfFont);

                        if (IsTextIncludeDBCSChar()) {
                            if (lfFont.lfCharSet != csi.ciCharset) {
                                hfontPrev = hfontMessage ;
                                lfFontPrev = lfFont ;
                                lfFont.lfCharSet = (BYTE) csi.ciCharset ;
                                hfontMessage = CreateFontIndirect( &lfFont );
                            }
                        }

                        SendDlgItemMessage(hDlg,ID_TEXTWINDOW,PWM_NEWPOSITION,fMode,0l);
                        InvalidateRect(GetDlgItem(hDlg,ID_TEXTWINDOW),NULL,TRUE);
                    }
                    ReleaseDC(hDlg, hDC);
                    break;

                case ID_BGROUNDCOLOR:
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        wTemp = (WORD)SendDlgItemMessage(hDlg,LOWORD(wParam),
                            CB_GETCURSEL,0,0l);
                        hPal = GetStockObject(DEFAULT_PALETTE);
                        GetPaletteEntries(hPal,wTemp,1,
                            (LPPALETTEENTRY)(LPDWORD)&dwBColor);
                        InvalidateRect(GetDlgItem(hDlg,ID_TEXTWINDOW),NULL,TRUE);
                    }
                    break;

                case IDOK:
                    GetWindowText(GetDlgItem(hDlg,ID_MARQUEETEXT),szBuffer,BUFFER_LEN);
                    WritePrivateProfileString(szAppName,szTextName,szBuffer, szIniFile);

                    WritePrivateProfileString(szAppName,szFontName,szFaceName, szIniFile);

                    // wSize is in logical units... we want to save as point size.
                    hDC = GetDC(hDlg);
                    wSize = MulDiv(-wSize, 72, GetDeviceCaps(hDC, LOGPIXELSY));
                    wsprintf(szBuffer, TEXT("%d"), wSize);
                    WritePrivateProfileString(szAppName,szSizeName, szBuffer, szIniFile);
                    ReleaseDC(hDlg, hDC);

                    hPal = GetStockObject(DEFAULT_PALETTE);
                    wTemp = (WORD)SendDlgItemMessage(hDlg,ID_BGROUNDCOLOR,CB_GETCURSEL,
                        0,0l);
                    GetPaletteEntries(hPal,wTemp,1,
                        (LPPALETTEENTRY)(LPDWORD)&dwBColor);
                    wsprintf(szBuffer,TEXT("%d %d %d"),GetRValue(dwBColor),
                        GetGValue(dwBColor),GetBValue(dwBColor));
                    WritePrivateProfileString(szAppName,szBColorName,szBuffer, szIniFile);

                    wsprintf(szBuffer,TEXT("%d %d %d"),GetRValue(dwTColor),
                        GetGValue(dwTColor),GetBValue(dwTColor));
                    WritePrivateProfileString(szAppName,szTColorName,szBuffer, szIniFile);

                    wTemp = GetScrollPos(GetDlgItem(hDlg,ID_SPEED),SB_CTL);
                    wsprintf(szBuffer,TEXT("%d"),wTemp);
                    WritePrivateProfileString(szAppName,szSpeedName,szBuffer, szIniFile);

                    szBuffer[UNDERLINE]=fUnderline;
                    szBuffer[STRIKEOUT]=fStrikeOut;
                    szBuffer[ITALIC]=fItalic;
                    szBuffer[MODE]=(fMode?(TCHAR)TEXT('1'):(TCHAR)TEXT('0'));
                    szBuffer[BOLD]=fBold;
                    szBuffer[NATTRIBUTES]=TEXT('\0');
                    WritePrivateProfileString(szAppName,szAttributes,szBuffer,szIniFile);

                    wsprintf(szBuffer, TEXT("%i"), (int)bCharSet);
                    WritePrivateProfileString(szAppName,szCharSetName,szBuffer,szIniFile);

                case IDCANCEL:
                    if (hfontMessage)
                        DeleteObject(hfontMessage);
                    EndDialog(hDlg,LOWORD(wParam) == IDOK);
                    return TRUE;

            }
            break;

    case WM_HELP: // F1
        WinHelp(
            (HWND) ((LPHELPINFO) lParam)->hItemHandle,
            szHelpFile,
            HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aMarqueeDlgHelpIds
        );
        break;

    case WM_CONTEXTMENU:  // right mouse click
        WinHelp(
            (HWND) wParam,
            szHelpFile,
            HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aMarqueeDlgHelpIds
        );
        break;

        default:
            break;
    }
    return FALSE;
}

//***************************************************************************

BOOL APIENTRY ChooseFontHookProc(hDlg, msg, wParam, lParam)
HWND  hDlg;
UINT  msg;
DWORD wParam;
LONG  lParam;
{
    switch(msg)
    {
        case WM_INITDIALOG:
            ShowWindow(hDlg, SW_SHOWNORMAL);    // bug #12820
            SetWindowText(hDlg, szFormatText);
            break;
    }
    return (FALSE);
}

//***************************************************************************

/* This procedure is called right before the dialog box above is created in
    order to register any child windows that are custom controls.  If no
    custom controls need to be registered, then simply return TRUE as in this
    case.  Otherwise, register the child controls however is convenient... */

BOOL     RegisterDialogClasses ( hInst )
HANDLE   hInst;
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ShowTextProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szShowTextName;

    return RegisterClass(&wc);
}

//***************************************************************************

int GetHeightFromPointSize(int szPoints)
{
    HDC hdc;
    int height;

    hdc = GetDC(NULL);
    height = MulDiv(-szPoints, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    return height;
}

void PatB(HDC hdc,int x,int y,int dx,int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

void FillR(HDC hdc, LPRECT prc, DWORD rgb)
{
    SetBkColor(hdc,rgb);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL);
}

void FrameR(HDC hdc, LPRECT prc, DWORD rgb, int iFrame)
{
    int     dx,dy;

    dx = prc->right  - prc->left;
    dy = prc->bottom - prc->top - 2*iFrame;

    PatB(hdc, prc->left, prc->top,          dx,iFrame,   rgb);
    PatB(hdc, prc->left, prc->bottom-iFrame,dx,iFrame,   rgb);

    PatB(hdc, prc->left,          prc->top+iFrame, iFrame,dy, rgb);
    PatB(hdc, prc->right-iFrame,  prc->top+iFrame, iFrame,dy, rgb);
}

void srand ( dwSeed )
DWORD dwSeed;
{
    dwRand = dwSeed;
}

WORD rand ( void )
{
    dwRand = dwRand * 214013L + 2531011L;
    return (WORD)((dwRand >> 16) & 0xffff);
}

LRESULT APIENTRY ShowTextProc ( hWnd, message, wParam, lParam )
HWND  hWnd;
UINT  message;
WPARAM wParam;
LPARAM lParam;
{
    PAINTSTRUCT     ps;
    RECT            rc;
    TCHAR           ach[180];
    int             len;
    static SIZE     sizeExt;
    HFONT           hfontT;
    HBRUSH          hbrTemp;
    static UINT_PTR wTimer;
    static WORD     wCount;
    static WORD     wInc;
    static WORD     wStep;
    static WORD     wHeight;
    static BOOL     bMELocale;
    static BOOL     fNeedToInitHeight;
    HDC             hDC;
    DWORD           dwLocale;
    UINT            uiETOFlags;

    switch (message)
    {
    case WM_CREATE:
        //  See if the user locale id is Arabic or Hebrew.
        dwLocale    = GetUserDefaultLCID();
        bMELocale = ((PRIMARYLANGID(LANGIDFROMLCID(dwLocale)) == LANG_ARABIC) ||
                (PRIMARYLANGID(LANGIDFROMLCID(dwLocale)) == LANG_HEBREW));

        GetClientRect(hWnd,&rc);
        if(wTimer = SetTimer(hWnd,1,1,NULL))
            wCount = (WORD) rc.right;
        else
            wCount = 0;

        wStep = DEF_SPEED;
        hDC = GetDC(NULL);
        GetTextExtentPoint32 (hDC, TEXT("T"), 1, &sizeExt);
        wHeight = (WORD)(((rc.bottom-rc.top)-sizeExt.cy)/2);
        fNeedToInitHeight = TRUE;
        ReleaseDC(NULL, hDC);
        break;

    case WM_TIMER:
        InvalidateRect(hWnd,NULL,FALSE);
        break;

    case WM_DESTROY:
        KillTimer(hWnd,wTimer);
        break;

    case WM_SETTEXT:
        DefWindowProc(hWnd, message, wParam, lParam);
        InvalidateRect(hWnd,NULL,FALSE);
    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        wInc += wStep;
        if (wInc >= DIV_SPEED)
        {
           WORD          wVelocity;
           TEXTMETRIC    tm;

            wVelocity = (wInc / DIV_SPEED) + 1;

            if (bMELocale) {
                wCount += wVelocity;
            } else {
                wCount -= wVelocity;
            }

            wInc = wInc % DIV_SPEED;
            len = GetWindowText (hWnd,ach,180);
            if (hfontMessage)
                hfontT = SelectObject(ps.hdc,hfontMessage);
            else
                hfontT = NULL;
            GetTextExtentPoint32 (ps.hdc, ach, len, &sizeExt);
            GetTextMetrics (ps.hdc, &tm);
            GetClientRect(hWnd,&rc);

            if (bMELocale) {
                 if (((short)wCount >= rc.right) || fNeedToInitHeight)
                 {
                   wCount = (WORD)(0 - sizeExt.cx + 1);
                   if(fMode)
                       wHeight = (WORD)(ZRAND(((rc.bottom-rc.top)-(sizeExt.cy/4))));
                   else
                       wHeight = (WORD)((int)((rc.bottom-rc.top)-(int)sizeExt.cy)/2);
                 }
            } else {
                 if ((((short)wCount + (short)sizeExt.cx) < 0) || fNeedToInitHeight)
                 {
                   wCount = (WORD) rc.right;
                   if(fMode)
                       wHeight = (WORD)(ZRAND(((rc.bottom-rc.top)-(sizeExt.cy/4))));
                   else
                       wHeight = (WORD)((int)((rc.bottom-rc.top)-(int)sizeExt.cy)/2);
                 }
            }

            if(fNeedToInitHeight)
                fNeedToInitHeight = FALSE;
                
            SetBkColor (ps.hdc,dwBColor);
            SetTextColor (ps.hdc,dwTColor);

#ifdef NOT_USED
//////////////////////////////////////////////////////////////////////////////
// This should have never been put in since this winproc only handles the
// "Sample Text Window"  control for the configuration dialog.  It is OK to
// use the whole client area as the opaqueing rect.
//////////////////////////////////////////////////////////////////////////////
            // Compute the opaque rectangle.
            rc.top    = (int)(short) wHeight;
            rc.left   = (int)(short) wCount;
            rc.bottom = (int)(short) wHeight + sizeExt.cy;
            rc.right  = (int)(short) wCount + sizeExt.cx + wVelocity
                                     + tm.tmMaxCharWidth * 2;
#endif  //  NOT_USED

            uiETOFlags = ETO_OPAQUE;
            if (bMELocale) {
              uiETOFlags |= ETO_RTLREADING;
            }

            ExtTextOut (ps.hdc,(int)(short) wCount, (int)(short)wHeight, uiETOFlags, (LPRECT)&rc,
                        ach, len, NULL);
            if (hfontT)
                SelectObject(ps.hdc, hfontT);
        }
        EndPaint (hWnd, &ps);
        return 0L;

    case WM_ERASEBKGND:
        hbrTemp = CreateSolidBrush (dwBColor);
        GetClientRect (hWnd, &rc);
        FillRect ((HDC)wParam, &rc, hbrTemp);
        DeleteObject (hbrTemp);
        return 0l;

    default:
        if (message == PWM_NEWSPEED)
        {
            wStep = (WORD) wParam;
            break;
        }
        if (message == PWM_NEWPOSITION)
        {
            GetClientRect (hWnd,&rc);
            if (fMode)
                wHeight = (WORD)(ZRAND(((rc.bottom-rc.top)-(sizeExt.cy/4))));
            else
                wHeight = (WORD)((int)((rc.bottom-rc.top)-(int)sizeExt.cy)/2);
            fNeedToInitHeight = TRUE;
            InvalidateRect(hWnd,NULL,TRUE);
            break;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

DWORD GetProfileRgb (LPTSTR szApp, LPTSTR szItem, DWORD rgb)
{
  TCHAR    buf[80];
  LPTSTR   pch;
  WORD     r,g,b;

    GetPrivateProfileString (szApp, szItem, TEXT(""), buf, CharSizeOf(buf), szIniFile);

    if (*buf)
    {
        pch = buf;
        r = AtoI (pch);
        while (*pch && *pch != TEXT(' '))
            pch++;
        while (*pch && *pch == TEXT(' '))
            pch++;
        g = AtoI(pch);
        while (*pch && *pch != TEXT(' '))
            pch++;
        while (*pch && *pch == TEXT(' '))
            pch++;
        b = AtoI(pch);

        return RGB(r,g,b);
    }
    else
        return rgb;
}

WORD  AtoI (LPTSTR  lpszConvert)
{
  WORD  wReturn = 0;

    while(*lpszConvert >= TEXT('0') && *lpszConvert <= TEXT('9'))
    {
        wReturn = wReturn*10 + (WORD)(*lpszConvert - TEXT('0'));
        lpszConvert++;
    }
    return wReturn;
}

void GetAttributes(void)
{
    TCHAR szBuffer[NATTRIBUTES+1];

    GetPrivateProfileString (szAppName, szAttributes, TEXT("00000"), szBuffer,
                             CharSizeOf(szBuffer), szIniFile);

    fUnderline = szBuffer[UNDERLINE];
    fStrikeOut = szBuffer[STRIKEOUT];
    fItalic = szBuffer[ITALIC];
    fMode = (szBuffer[MODE] == TEXT('1'));
    fBold = szBuffer[BOLD];
}

