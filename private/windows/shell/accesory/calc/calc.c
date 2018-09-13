/**************************************************************************\
 *** SCICALC Scientific Calculator for Windows 3.00.12
 *** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989
 *** (c)1989 Microsoft Corporation.  All Rights Reserved.
 ***
 *** scimain.c
 ***
 *** Definitions of all globals, WinMain procedure
 ***
 *** Last modification
 ***    Fri  22-Nov-1996
 ***
 *** -by- Jonathan Parati. [jonpa]   22-Nov-1996
 *** Converted Calc from floating point to infinite precision.
 *** The new math engine is in ..\ratpak
 ***
 ***
 *** -by- Amit Chatterjee. [amitc]  05-Jan-1990.
 *** Calc did not have a floating point exception signal handler. This
 *** would cause CALC to be forced to exit on a FP exception as that's
 *** the default.
 *** The signal handler is defined in SCIFUNC.C, in WinMain we hook the
 *** the signal.
\**************************************************************************/

#include "scicalc.h"
#include "calchelp.h"
#include "signal.h"
#include "unifunc.h"
#include "input.h"


/**************************************************************************/
/*** Global variable declarations and initializations                   ***/
/**************************************************************************/

int        nCalc=0;        /* 0=Scientific, 1=Simple.                      */
BOOL       gbUseSep=FALSE; /* display the number with a separator          */
ANGLE_TYPE nDecMode=ANGLE_DEG;   /* Holder for last used Deg/Rad/Grad mode.      */
int        nHexMode=0;     /* Holder for last used Dword/Word/Byte mode.   */

int        nTempCom=0,     /* Holding place for the last command.          */
           nParNum=0,      /* Number of parenthases.                       */
           nOpCode=0,      /* ID value of operation.                       */
           nOp[25],        /* Holding array for parenthasis operations.    */
           nPrecOp[25],    /* Holding array for precedence  operations.    */
           nPrecNum=0,     /* Current number of precedence ops in holding. */
           gcIntDigits;    /* Number of digits allowed in the current base */

eNUMOBJ_FMT nFE = FMT_FLOAT; /* Scientific notation conversion flag.       */

HWND       g_hwndDlg=0,     /* Global handle to main window.               */
           hEdit=0,         /* Handle to Clibboard I/O edit control        */
           hStatBox=0,      /* Global handle to statistics box.            */
           hListBox=0;      /* Global handle for statistics list box.      */
            

HMENU      g_hHexMenu=NULL;     // Global handle for hex menu 
HMENU      g_hDecMenu=NULL;     // Global handle for dec menu 

HANDLE     hAccel;              // Accelerator handle.
HINSTANCE  hInst;               // Global instance.

BOOL       bHyp=FALSE,          // Hyperbolic on/off flag.
           bInv=FALSE,          // Inverse on/off flag.
           bError=FALSE,        // Error flag.
           bColor=TRUE;         // Flag indicating if color is available.

HNUMOBJ    ghnoNum=NULL,        // Currently displayed number used everywhere.
           ghnoParNum[25],      // Holding array for parenthasis values.
           ghnoPrecNum[25],     // Holding array for precedence  values.
           ghnoMem=NULL,        // Current memory value.
           ghnoLastNum = NULL;  // Number before operation (left operand).

LONG       nPrecision = 32,         // number of digits to use in decimal mode
           nDecimalPlaces = 10,     // number of decimal places to show
           nRadix=10,               // the current base (2, 8, 10, or 16)
           dwWordBitWidth = 64;     // # of bits in currently selected word size

HNUMOBJ g_ahnoChopNumbers[4];   // word size inforcement

BOOL    bFarEast;       // true if we need to use Far East localization

#ifdef USE_MIRRORING
BOOL    g_fLayoutRTL = FALSE;
#endif

extern CALCINPUTOBJ gcio;
extern BOOL         gbRecord;

/* DO NOT LOCALIZE THESE STRINGS.                                           */

TCHAR      szAppName[10]=TEXT("SciCalc"), /* Application name.              */
           szDec[5]=TEXT("."),            /* Default decimal character      */
           gszSep[5]=TEXT(","),           /* Default thousand seperator     */
           szBlank[6]=TEXT("");           /* Blank space.                   */

LPTSTR     gpszNum = NULL;
static TCHAR szInitNum[] = TEXT("0");      // text to init gpszNum with

/* END WARNING */


/* rgpsz[] is an array of pointers to strings in a locally allocated      */
/* memory block.  This block is fixed such that LocalLock does not need   */
/* to be called to use a string.                                          */

TCHAR     *rgpsz[CSTRINGS];
RECT      rcDeg[6];


void ParseCmdLine( LPSTR pszCmdA );
BOOL InitializeWindowClass( HINSTANCE hPrevInstance );
void InitialOneTimeOnlySetup();
void EverythingResettingNumberSetup();

/**************************************************************************/
/*** Main Window Procedure.                                             ***/
/***                                                                    ***/
/*** Important functions:                                               ***/
/***     1)  Gets text dimensions and sets conversion units correctly.  ***/
/***                                                                    ***/
/***     2)  Checks the display device driver for color capability.     ***/
/***         If only 2 colors are available (mono, cga), bColor is      ***/
/***         set to FALSE, and the background brush is gray.  If        ***/
/***         color is available, the background brush colors are read   ***/
/***         from WIN.INI and the brush is created.                     ***/
/***                                                                    ***/
/***     3)  Window and hidden edit control are created.                ***/
/***                                                                    ***/
/***     4)  Contains message loop and deletes the brushes used.        ***/
/***                                                                    ***/
/**************************************************************************/

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    MSG        msg;
    INT        nx;
    TCHAR      *psz;
    WORD       cch = 0, cchTotal = 0;
    HANDLE     hMem;
    TCHAR      szTempString[100];
#ifdef USE_MIRRORING
    DWORD      dwLayout;
#endif

    // A bunch of sanity checks to ensure nobody is violating any of the
    // bazillion
    // assumptions calc makes about the order of controls.  Of course these 
    // asserts
    // wouldn't prevent a really dedicated idiot from messing things up but they
    // should help guide a rational person who might not be aware of calc's 
    // idiosyncrasies.
    // Anyone who modifies the resource file should hit these asserts which
    // will then
    // alert them to the consequences of their actions.

    // IDC_0 to IDC_F must be in sequential increasing order
    ASSERT( 15 == (IDC_F - IDC_0) );
    // Binary operators IDC_AND through IDC_PWR must be in order
    ASSERT( (95-86) == (IDC_PWR - IDC_AND) );
    // Unary operators IDC_CHOP through IDC_EQU must be in order
    ASSERT( (112-96) == (IDC_EQU - IDC_CHOP) );
    // menu item id's must be in order
    ASSERT( 5 == (IDM_LASTMENU - IDM_FIRSTMENU) );

#ifdef USE_MIRRORING
    GetProcessDefaultLayout(&dwLayout);
    if (dwLayout & LAYOUT_RTL)
    {
        SetProcessDefaultLayout(dwLayout & ~LAYOUT_RTL);
        g_fLayoutRTL = TRUE;
    }
#endif

    ParseCmdLine( lpCmdLine );

    hInst = hInstance;

    if ( !InitializeWindowClass( hPrevInstance ) )
        return FALSE;

    // Read strings for keys, errors, trig types, etc.
    // These will be copied from the resources to local memory.  A larger
    // than needed block is allocated first and then reallocated once we
    // know how much is actually used.

    try
    {
        hMem = LocalAlloc(LPTR, ByteCountOf(CCHSTRINGSMAX));
        if (!hMem)
            throw;

        psz = (TCHAR *)hMem;

        for (nx = 0; nx <= CSTRINGS; nx++)
        {
            ASSERT( (CCHSTRINGSMAX-cchTotal) > LoadString(hInstance, (WORD) IDS_FIRSTKEY + nx, NULL, 0) );
            cch = (WORD)(1 + LoadString(hInstance, (WORD) IDS_FIRSTKEY + nx, psz, (int) (CCHSTRINGSMAX-cchTotal)));
            cchTotal += cch;
            rgpsz[nx] = psz;
            psz += cch;
        }

        if (!LocalReAlloc(hMem, ByteCountOf(cchTotal), LMEM_FIXED))
            throw;
    }
    catch ( ... )
    {
        if (!LoadString(hInst, IDS_NOMEM, szTempString, CharSizeOf(szTempString)))
        {
            // only do this if LoadString Fails, means system is really hosed!
            lstrcpy(szTempString, TEXT("<Main> Not enough memory."));
        }
        MessageBox((HWND) NULL, szTempString, NULL, MB_OK | MB_ICONHAND);
        return FALSE;
    }

    // The display in calc isn't really an edit control so we use this edit 
    // control to simplify cutting to the clipboard

    hEdit = CreateWindow( TEXT("EDIT"), TEXT("CalcMsgPumpWnd"), 
                          WS_OVERLAPPED | WS_VISIBLE, 
                          CW_USEDEFAULT,0,CW_USEDEFAULT,0, 
                          NULL, NULL, hInst, NULL );

    // This initializes things that only need to be set up once, including a 
    // call to ratpak so that ratpak can create any constants it needs

    InitialOneTimeOnlySetup();

    // we store in the win.ini file our desired display mode, Scientific 
    //  or Standard

    nCalc = (INT)GetProfileInt(szAppName, TEXT("layout"), 1);

    gbUseSep = (INT)GetProfileInt(szAppName, TEXT("UseSep"), 0);

    // InitSciCalc creates a dialog based on what the value of nCalc is. 
    // A handle to the window that is created is stored in g_hwndDlg

    InitSciCalc(TRUE);

    hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDA_ACCELTABLE));


    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!hStatBox || !IsDialogMessage(hStatBox, &msg))
        {
            if ( ((msg.hwnd == g_hwndDlg)||IsChild(g_hwndDlg, msg.hwnd)) && TranslateAccelerator (g_hwndDlg, (HACCEL)hAccel, &msg))
                continue;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    LocalFree(hMem);
    return (DWORD)msg.wParam;
}


/**************************************************************************\
*
*   Command Line processing routines
*
*   History
*       22-Nov-1996 JonPa       Wrote it
*
\**************************************************************************/

#define IsWhiteSpace( ch )  ((ch) == TEXT(' ') || (ch) == TEXT('\t'))
#define IsDigit( ch )       ((ch) >= TEXT('0') && (ch) <= TEXT('9'))

LPTSTR TtoL( LPTSTR psz, LONG *pl ) {
    LONG l = 0;

    while( IsDigit( *psz ) ) {
        l = l * 10 + (*psz - TEXT('0'));
        psz = CharNext( psz );
    }

    *pl = l;
    return psz;
}

void ParseCmdLine( LPSTR pszCmdA ) {
    BOOL fInQuote;
    LPTSTR pszCmdT = GetCommandLine();

    // parse cmd line
    // usage: -p:## -r:## -w:## -e -x -i
    // -e, -x, and -i currently do nothing.

    // Skip app name
    while( *pszCmdT && IsWhiteSpace( *pszCmdT )) {
        pszCmdT = CharNext( pszCmdT );
    }

    fInQuote = FALSE;
    while( *pszCmdT && (fInQuote || !IsWhiteSpace(*pszCmdT)) ) {
        if (*pszCmdT == TEXT('\"'))
            fInQuote = !fInQuote;
        pszCmdT = CharNext( pszCmdT );
    }

    while( *pszCmdT )
    {
        switch( *pszCmdT )
        {
        case TEXT('p'):
        case TEXT('P'):
            // -p:## precision
            pszCmdT = CharNext(pszCmdT);

            // Skip ':' and white space
            while( *pszCmdT && (*pszCmdT == TEXT(':') || IsWhiteSpace(*pszCmdT)) ) {
                pszCmdT = CharNext(pszCmdT);
            }

            pszCmdT = TtoL( pszCmdT, &nPrecision );

            // a percision > C_NUM_MAX_DIGITS will allow a string too long for it's buffer
            if ( nPrecision > C_NUM_MAX_DIGITS)
            {
                ASSERT( nPrecision <= C_NUM_MAX_DIGITS );
                nPrecision = C_NUM_MAX_DIGITS;
            }

            // NOTE: this code assumes there MUST be a space after the number
            break;

        case TEXT('r'):
        case TEXT('R'):
            // -r:## Radix
            pszCmdT = CharNext(pszCmdT);

            // Skip ':' and white space
            while( *pszCmdT && (*pszCmdT == TEXT(':') || IsWhiteSpace(*pszCmdT)) ) {
                pszCmdT = CharNext(pszCmdT);
            }

            pszCmdT = TtoL( pszCmdT, &nRadix );

            // since the UI only has 16 keys for digit input, we only allow upto base 16
            if (nRadix > 16)
            {
                ASSERT( nRadix <= 16 );
                nRadix = 16;
            }
            else if (nRadix < 2)    // you know some fool would try for base zero if you let them
            {
                ASSERT( nRadix >= 2 );
                nRadix = 2;
            }

           // NOTE: this code assumes there MUST be a space after the number
            break;

        case TEXT('e'):
        case TEXT('E'):
            // -e extended mode
            break;

        case TEXT('w'):
        case TEXT('W'):
            // -w:## Word size in bits
            pszCmdT = CharNext(pszCmdT);

            // Skip ':' and white space
            while( *pszCmdT && (*pszCmdT == TEXT(':') || IsWhiteSpace(*pszCmdT)) ) {
                pszCmdT = CharNext(pszCmdT);
            }

            // Set bit count
            pszCmdT = TtoL( pszCmdT, &dwWordBitWidth );

            // NOTE: this code assumes there MUST be a space after the number
            break;
        }

        pszCmdT = CharNext( pszCmdT );
    }
}

//////////////////////////////////////////////////
//
// InitalizeWindowClass
//
//////////////////////////////////////////////////
BOOL InitializeWindowClass( HINSTANCE hPrevInstance )
{
    WNDCLASSEX wndclass;

    if (!hPrevInstance)
    {
        wndclass.cbSize         = sizeof(wndclass);
        wndclass.style          = 0;
        wndclass.lpfnWndProc    = CalcWndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = DLGWINDOWEXTRA;
        wndclass.hInstance      = hInst;
        wndclass.hIcon          = LoadIcon(hInst, TEXT("SC"));
        wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW);
        wndclass.hbrBackground  = GetSysColorBrush(COLOR_3DFACE);
        wndclass.lpszMenuName   = MAKEINTRESOURCE(IDM_CALCMENU);
        wndclass.lpszClassName  = szAppName;
        wndclass.hIconSm        = NULL;

        if (!RegisterClassEx(&wndclass))
            return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////////////////
//
// InitialOneTimeOnlyNumberSetup
//
//////////////////////////////////////////////////
void InitialOneTimeOnlySetup()
{
    // Initialize the decimal input code.  This ends up getting called twice
    // but it's quick so that shouldn't be a problem.  Needs to be done before
    // SetRadix is called.

    CIO_vClear( &gcio );
    gbRecord = TRUE;

    // we must now setup all the ratpak constants and our arrayed pointers 
    // to these constants.
    BaseOrPrecisionChanged();

    // these rat numbers are set only once and then never change regardless of 
    // base or precision changes
    g_ahnoChopNumbers[0] = rat_qword;
    g_ahnoChopNumbers[1] = rat_dword;
    g_ahnoChopNumbers[2] = rat_word;
    g_ahnoChopNumbers[3] = rat_byte;

    // we can't call this until after we have set the radix (and thus called 
    // ChangeConstants) so we do it last.

    EverythingResettingNumberSetup();

    NumObjAssign( &ghnoMem, HNO_ZERO );
}

//////////////////////////////////////////////////
//
// EverythingResettingNumberSetup
//
//////////////////////////////////////////////////
void EverythingResettingNumberSetup()
{
    int i;

    // Initialize the decimal input code.
    CIO_vClear( &gcio );
    gbRecord = TRUE;

    NumObjAssign( &ghnoNum, HNO_ZERO );
    NumObjAssign( &ghnoLastNum, HNO_ZERO );

    // REVIEW: is it just me, or do we speew major memory wheneven this method
    // executes?

    // array used to handle ( and )
    for( i = 0; i < ARRAYSIZE(ghnoParNum); i++ )
        ghnoParNum[i] = NULL;

    // array used to handle order of operations
    for( i = 0; i < ARRAYSIZE(ghnoPrecNum); i++ )
        ghnoPrecNum[i] = NULL;

    gpszNum = (LPTSTR)NumObjAllocMem( sizeof(szInitNum) );
    lstrcpy( gpszNum, szInitNum );
}

//////////////////////////////////////////////////
//
// InitSciCalc
//
//////////////////////////////////////////////////
VOID  APIENTRY InitSciCalc(BOOL bViewChange)
{
    TCHAR   chLastDec;
    TCHAR   chLastSep;
    int     nLastSepLen;
    HMENU   hMenu;
    BOOL    bRepaint=FALSE;
    RECT    rect = {0,0,0,0};

    EverythingResettingNumberSetup();

    // when we switch modes, we need to remind the ui that we are no longer 
    // inputing the number we were inputting before we switched modes.

    gbRecord = FALSE;    // REVIEW: This should not be needed with the new initialization

    chLastDec = szDec[0];
    chLastSep = gszSep[0];

    GetProfileString(TEXT("intl"), TEXT("sDecimal"), TEXT("."), 
                     szDec, CharSizeOf(szDec));
    GetProfileString(TEXT("intl"), TEXT("sThousand"), TEXT(","), 
                     gszSep, CharSizeOf(gszSep));
    
    // if the thousands symbol has changed we always do the following things

    if ( gszSep[0] != chLastSep )
    {
        chLastSep = gszSep[0];

        bRepaint = TRUE;
    }
    
    // if the decimal symbol has changed we always do the following things
    if ( szDec[0] != chLastDec )
    {
        chLastDec = szDec[0];

        // Re-initialize input string's decimal point.
        CIO_vUpdateDecimalSymbol(&gcio, chLastDec);

        // put the new decimal symbol into the table used to draw the decimal
        // key

        *(rgpsz[IDS_DECIMAL]) = chLastDec;

        // we need to redraw to update the decimal point button
        bRepaint = TRUE;
    }

    if ( bViewChange )
    {
        BOOL    bUseOldPos = FALSE;

        // if we are changing views we destory the old window and create 
        // a new window

        if ( g_hwndDlg )
        {
            SetMenu(g_hwndDlg, g_hDecMenu);
            bUseOldPos = TRUE;
            GetWindowRect( g_hwndDlg, &rect );
            DestroyWindow( g_hwndDlg );
            DestroyMenu(g_hHexMenu);
            g_hHexMenu=NULL;
        }

        // create the correct window for the mode we're currently in
        if ( nCalc )
        {
            // switch to standard mode
            g_hwndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_STANDARD), 0, 
                                     NULL);
            g_hDecMenu=GetMenu(g_hwndDlg);

#ifdef USE_MIRRORING
            if (g_fLayoutRTL)
            {
                SetWindowLong(g_hwndDlg,
                              GWL_EXSTYLE,
                              GetWindowLong(g_hwndDlg,GWL_EXSTYLE) | \
                              WS_EX_LAYOUTRTL |  WS_EX_NOINHERITLAYOUT);
            }
#endif
        }
        else
        {
            // switch to scientific mode
            g_hwndDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SCIENTIFIC), 
                                     0, NULL);
            g_hDecMenu=GetMenu(g_hwndDlg);
            g_hHexMenu=LoadMenu(hInst, MAKEINTRESOURCE(IDM_HEXCALCMENU));

#ifdef USE_MIRRORING
            if (g_fLayoutRTL)
            {
                SetWindowLong(g_hwndDlg,
                              GWL_EXSTYLE,
                              GetWindowLong(g_hwndDlg,GWL_EXSTYLE) | WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT);
            }
#endif

            // Stat box is initially off, disable stat buttons.
            for ( int iID = IDC_AVE; iID <= IDC_DATA; iID++ )
                EnableWindow( GetDlgItem( g_hwndDlg, iID ), FALSE );

            SwitchModes(10, nDecMode, nHexMode);

            // If precision won't fit in display, then resize it
            if (nPrecision > 32)
            {
                HWND hwndDisplay;
                RECT rc, rcMain;

                hwndDisplay=GetDlgItem( g_hwndDlg, IDC_DISPLAY );
                GetWindowRect( hwndDisplay, &rc );
                GetClientRect( g_hwndDlg, &rcMain );
                MapWindowPoints( g_hwndDlg, NULL, (LPPOINT)&rcMain, 2);

                rc.left    = rcMain.left + (rcMain.right - rc.right);
                OffsetRect( &rc, -(rcMain.left), -(rcMain.top) );

                SetWindowPos(hwndDisplay, NULL, 
                             rc.left, rc.top, 
                             rc.right - rc.left, rc.bottom - rc.top,
                             SWP_NOACTIVATE | SWP_NOZORDER );
            }
        }

        // keep calc in the same place it was before
        if ( bUseOldPos )
        {
            SetWindowPos( g_hwndDlg, NULL, rect.left, rect.top, 0,0, 
                          SWP_NOZORDER | SWP_NOSIZE );
        }

        // ensure the menu items for Scientific and Standard are set correctly

        CheckMenuRadioItem(g_hDecMenu, IDM_SC, IDM_SSC, 
                           (nCalc == 0 ? IDM_SC : IDM_SSC), MF_BYCOMMAND); 

        CheckMenuItem(g_hDecMenu, IDM_USE_SEPARATOR, 
                      MF_BYCOMMAND | (gbUseSep ? MF_CHECKED : MF_UNCHECKED));  

        if (g_hHexMenu)
        {
            CheckMenuRadioItem(g_hHexMenu, IDM_SC, IDM_SSC, 
                               (nCalc == 0 ? IDM_SC : IDM_SSC), MF_BYCOMMAND); 

            CheckMenuItem(g_hHexMenu, IDM_USE_SEPARATOR, 
                          MF_BYCOMMAND | (gbUseSep ? MF_CHECKED:MF_UNCHECKED)); 
        }

        // To ensure that the call to SetRadix correctly update the active 
        // state of the buttons on
        // SciCalc we must tell it to forget the previous Radix
        {
            extern long oldRadix;
            oldRadix = (unsigned)-1;
        }

        // this will set the correct buttons on the UI
        SetRadix(10);

        SetDlgItemText(g_hwndDlg, IDC_MEMTEXT, 
                       NumObjIsZero(ghnoMem) ? (szBlank) : (TEXT(" M")) );

        ShowWindow( g_hwndDlg, SW_SHOW );
        UpdateWindow(g_hwndDlg);

    } // END if ( bViewChanged )
    else if ( bRepaint )
    {
        // no need to repaint if we just changed views
        InvalidateRect( g_hwndDlg, NULL, TRUE );
    }
}


