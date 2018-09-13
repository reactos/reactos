#include "sol.h"
#include <shellapi.h>  // To pick up ShellAbout()
#include <htmlhelp.h>

VSZASSERT


#define rgbGreen RGB(0x00,0x80,0x00)
#define rgbWhite RGB(0xff,0xff,0xff)

PT   ptNil = {0x7fff, 0x7fff};
TCHAR szAppName[10];      // name of this app: 'solitaire'
TCHAR szScore[50];        // 'score:' for internationalization

/* Instance info */
static HANDLE  hAccel; // accelerators handle

HWND    hwndApp;       // window handle to this app
HANDLE  hinstApp;      // instance handle to this app
BOOL    fBW=FALSE;     // true if on true monochrome video! (never true on NT)
HBRUSH  hbrTable;      // brush for background of table top
LONG    rgbTable;      // RGB value of table top

BOOL fIconic = fFalse; // true if app is 'iconic'

INT  dyChar;           // tmHeight of font in hdc
INT  dxChar;           // tmMaxCharWidth of font in hdc


#define modeNil -1
INT modeFaceDown = modeNil;  // back of cards ID


GM *pgmCur = NULL;           // current game

/* card extent info */
DEL delCrd;
DEL delScreen;

RC rcClient;                 // client rectangle

INT igmCur;   /* the current game #, srand seeded with this */
#ifdef DEBUG
BOOL fScreenShots = fFalse;
#endif

/* window messages for external app drawing */
static UINT wmCardDraw;


HDC hdcCur = NULL;   // current hdc to draw on
INT usehdcCur = 0;   // hdcCur use count
X xOrgCur = 0;
Y yOrgCur = 0;

static TCHAR szClass[] = TEXT("Solitaire");

TCHAR szOOM[50];

// BUG: some of these should go in gm struct
//
BOOL fStatusBar   = fTrue;
BOOL fTimedGame   = fTrue;
BOOL fKeepScore   = fFalse;
SMD  smd          = smdStandard;  /* Score MoDe */
INT  ccrdDeal     = 3;
BOOL fOutlineDrag = fFalse;

BOOL fHalfCards = fFalse;


INT  xCardMargin;
#define MIN_MARGIN  (dxCrd / 8 + 3)


/********************  Internal Functions ****************/
BOOL FSolInit( HANDLE, HANDLE, LPTSTR, INT );
VOID GetIniFlags( BOOL * );
VOID APIENTRY cdtTerm( VOID );
VOID DoHelp( INT );

LRESULT APIENTRY SolWndProc(HWND, UINT, WPARAM, LPARAM);

// International stuff
//
INT  iCurrency;
TCHAR szCurrency[5];


/******************************************************************************
 * WINMAIN/ENTRY POINT
 *   This is the main entry-point for the application.  It uses the porting
 *   macro MMain() since it was ported from 16bit Windows.
 *
 *   The accelerator-table was added from demo-purposes.
 *
 *
 *****************************************************************************/
MMain( hinst, hinstPrev, lpstrCmdLine, sw )

    MSG msg;
    LPTSTR  lpszCmdLine = GetCommandLine();


    // Initialize the application.
    //
    if (!FSolInit(hinst, hinstPrev, lpszCmdLine, sw))
            return(0);


    // Message-Polling loop.
    //
    msg.wParam = 1;
    while (GetMessage((LPMSG)&msg, NULL, 0, 0))
    {
        if( !TranslateAccelerator( hwndApp, hAccel, &msg ))
        {
            TranslateMessage((LPMSG)&msg);
            DispatchMessage((LPMSG)&msg);
        }
    }

    return ((int)(msg.wParam ? 1 : 0));

    // Eliminate unreferenced-variable warnings from
    // porting macro.
    //
    (void)_argv;
    (void)_argc;
}


/******************************************************************************
 *      FSolInit
 *
 *      Main program initialization.
 *
 *      Arguments:
 *              hinst - instance of this task
 *              hinstPrev - previous instance, or NULL if this is the
 *                      first instance
 *              lpszCmdLine - command line argument string
 *              sw - show window command
 *
 *      Returns:
 *              fFalse on failure.
 *
 *****************************************************************************/
BOOL FSolInit(HANDLE hinst, HANDLE hinstPrev, LPTSTR lpszCmdLine, INT sw)
{
    WNDCLASSEX cls;
    HDC        hdc;
    TEXTMETRIC tm;
    HANDLE     hcrsArrow;
    BOOL       fStartIconic;
    TCHAR FAR  *lpch;
    BOOL       fOutline;
    TCHAR      szT[20];
    RECT       rect;
    WORD APIENTRY TimerProc(HWND, UINT, UINT_PTR, DWORD);

    hinstApp = hinst;

    /* create stock objects */

    CchString(szOOM, idsOOM);
    if(!cdtInit((INT FAR *)&dxCrd, (INT FAR *)&dyCrd))
    {
        goto OOMError;
    }
    hcrsArrow = LoadCursor(NULL, IDC_ARROW);
    hdc = GetDC(NULL);
    if(hdc == NULL)
    {
        OOMError:
        OOM();
        return fFalse;
    }

    GetTextMetrics(hdc, (LPTEXTMETRIC)&tm);
    dyChar = tm.tmHeight;
    dxChar = tm.tmMaxCharWidth;
    if (GetDeviceCaps(hdc, NUMCOLORS) == 2)
        fBW = fTrue;

/* BUG:  if HORZRES not big enough, have to call cdtDrawExt & shrink dxCrd */
/* BUG:  Need to check VERTRES and divide dxCrd by 2 (esp w/ lores ega) */
    dxScreen = GetDeviceCaps(hdc, HORZRES);
    dyScreen = GetDeviceCaps(hdc, VERTRES);
    if(fHalfCards = dyScreen < 300)
        dyCrd /= 2;
    ReleaseDC(NULL, hdc);
    rgbTable = fBW ? rgbWhite : rgbGreen;
    hbrTable = CreateSolidBrush(rgbTable);

    srand((WORD) time(NULL));

    /* load strings */
    CchString(szAppName, idsAppName);
    CchString(szScore, idsScore);

    CchString(szT, idsCardDraw);
    wmCardDraw = RegisterWindowMessage(szT);

    /* scan cmd line to see if should come up iconic */
    /* this may be unnecessary with win3.0 (function may be provided to */
    /* do it automatically */

    fStartIconic = fFalse;
    for(lpch = lpszCmdLine; *lpch != TEXT('\000'); lpch++)
    {
        if(*lpch == TEXT('/') && *(lpch+1) == TEXT('I'))
        {
            fStartIconic = fTrue;
            break;
        }
    }


    /* Load the solitaire icon */

    hIconMain = LoadIcon(hinstApp, MAKEINTRESOURCE(ID_ICON_MAIN));

    /* Load the solitaire icon image */

    hImageMain = LoadImage(hinstApp, MAKEINTRESOURCE(ID_ICON_MAIN),
                         IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);


    /* register window classes */

    if (hinstPrev == NULL)
    {
        ZeroMemory( &cls, sizeof(cls) );
        cls.cbSize= sizeof(cls);
        cls.style = CS_BYTEALIGNWINDOW | CS_DBLCLKS,
        cls.lpfnWndProc = SolWndProc;
        cls.hInstance = hinstApp;
        cls.hIcon =  hIconMain;
        cls.hIconSm= hImageMain;
        cls.hCursor = hcrsArrow;
        cls.hbrBackground = hbrTable;
        cls.lpszMenuName = MAKEINTRESOURCE(idmSol);
        cls.lpszClassName = (LPTSTR)szClass;
        if (!RegisterClassEx(&cls))
        {
            goto OOMError;
        }
     }

	/* Determine the proper starting size for the window */

	/* Card margin is just a little bigger than 1/8 of a card */
	xCardMargin = MIN_MARGIN;
	
	/* We need 7 card widths and 8 margins */
	rect.right = dxCrd * 7 + 8 * xCardMargin;

	/* Compute the window size we need for a client area this big */
	rect.bottom = dyCrd * 4;
	rect.left = rect.top = 0;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
	rect.right -= rect.left;
	rect.bottom -= rect.top;

	/* Make sure it's not too big */
	if (rect.bottom > dyScreen)
	    rect.bottom = dyScreen;

    /* create our windows */
    if (!
    (hwndApp = CreateWindow( (LPTSTR)szClass, (LPTSTR)szAppName,
                    fStartIconic ? WS_OVERLAPPEDWINDOW | WS_MINIMIZE | WS_CLIPCHILDREN:
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                    CW_USEDEFAULT, 0,
			rect.right, rect.bottom,
                        (HWND)NULL, (HMENU)NULL, hinstApp, (LPTSTR)NULL)))
        {
        goto OOMError;
        }

    GetIniFlags(&fOutline);


    if(SetTimer(hwndApp, 666, 250, TimerProc) == 0)
    {
        goto OOMError;
    }

    FInitGm();
    FSetDrag(fOutline);

    ShowWindow(hwndApp, sw);
    UpdateWindow(hwndApp);

    hAccel = LoadAccelerators( hinst, TEXT("HiddenAccel") );

    FRegisterStat(hinstPrev == NULL);
    if(fStatusBar)
        FCreateStat();

    Assert(pgmCur != NULL);
    if(sw != SW_SHOWMINNOACTIVE && sw != SW_MINIMIZE)
        PostMessage(hwndApp, WM_COMMAND, idsInitiate, 0L);

    return(fTrue);
}



VOID DoPaint(HWND hwnd)
{
    PAINTSTRUCT paint;

    BeginPaint(hwnd, (LPPAINTSTRUCT) &paint);
    if(pgmCur)
        SendGmMsg(pgmCur, msggPaint, (INT_PTR) &paint, 0);
    EndPaint(hwnd, (LPPAINTSTRUCT) &paint);
}


/*      SolWndProc
 *
 *      Window procedure for main Sol window.
 *
 *      Arguments:
 *              hwnd - window handle receiving the message - should
 *                      be hwndSol
 *              wm - window message
 *              wParam, lParam - more info as required by wm
 *
 *      Returns:
 *              depends on the message
 */
LRESULT APIENTRY SolWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    HMENU hmenu;
    PT pt;
    INT msgg;
    VOID NewGame();
    VOID StatString();

    switch (wm)
    {
        default:
            if(wm == wmCardDraw)
            {
                switch(wParam)
                {
                    case drwInit:
                        return MAKELONG(dxCrd, dyCrd);

                    case drwDrawCard:
                        #define lpcddr ((CDDR FAR *)lParam)
                        return cdtDraw(lpcddr->hdc, lpcddr->x, lpcddr->y, lpcddr->cd, lpcddr->mode, lpcddr->rgbBgnd);
                        #undef lpcddr

                    case drwClose:
                        PostMessage(hwndApp, WM_SYSCOMMAND, SC_CLOSE, 0L);
                        return fTrue;
                }
            }
            break;

    case WM_HELP:
        DoHelp( idsHelpIndex );
        break;

    case WM_DESTROY:
        KillTimer(hwndApp, 666);
        SendGmMsg(pgmCur, msggEnd, 0, 0);
        FSetDrag(fTrue);    /* Free up screen bitmaps if we made em */
        cdtTerm();
        DeleteObject(hbrTable);
        PostQuitMessage(0);
        break;

    case WM_ACTIVATE:
        if( GET_WM_ACTIVATE_STATE(wParam, lParam) &&
		   !GET_WM_ACTIVATE_FMINIMIZED(wParam, lParam) )
            DoPaint(hwnd);
        break;

    case WM_KILLFOCUS:
        if(pgmCur->fButtonDown)
            SendGmMsg(pgmCur, msggMouseUp, 0, fTrue);
        /* Fall through. */
    case WM_SETFOCUS:
        ShowCursor(wm == WM_SETFOCUS);
        break;


    case WM_SIZE:
    {
	    int nNewMargin;
	    int nMinMargin;

	    fIconic = IsIconic(hwnd);
	    GetClientRect(hwnd, (LPRECT) &rcClient);

	    /* Compute the new margin size if any and if necessary, redraw */
	    nNewMargin = ((short)lParam - 7 * (short)dxCrd) / 8;
	    nMinMargin = MIN_MARGIN;
	    if (nNewMargin < nMinMargin && xCardMargin != nMinMargin)
		nNewMargin = nMinMargin;
	    if (nNewMargin >= nMinMargin)
	    {
            xCardMargin = nNewMargin;
            PositionCols();
            InvalidateRect(hwnd, NULL, TRUE);
	    }

	    /* Code always falls through here */
    }


    case WM_MOVE:
        StatMove();
        break;


    case WM_MENUSELECT:
	    // Don't send in garbage if not a menu item
	    if( GET_WM_MENUSELECT_FLAGS( wParam, lParam ) & MF_POPUP     ||
		    GET_WM_MENUSELECT_FLAGS( wParam, lParam ) & MF_SYSMENU   ||
		    GET_WM_MENUSELECT_FLAGS( wParam, lParam ) & MF_SEPARATOR ) {

		    StatString(idsNil);
		}
		else {
		    StatString( GET_WM_MENUSELECT_CMD( wParam, lParam ));
		}
        break;

    case WM_KEYDOWN:
        Assert(pgmCur);
        SendGmMsg(pgmCur, msggKeyHit, wParam, 0);
        break;

    case WM_LBUTTONDOWN:
        /*              ProfStart(); */
        SetCapture(hwnd);
        if(pgmCur->fButtonDown)
            break;
        msgg = msggMouseDown;
        goto DoMouse;

    case WM_LBUTTONDBLCLK:
        msgg = msggMouseDblClk;
        if(pgmCur->fButtonDown)
            break;
        goto DoMouse;

    case WM_RBUTTONDOWN:
        // If the left mousebutton is down, ignore the right click.
        if (GetCapture())
            break;
        msgg = msggMouseRightClk;
        goto DoMouse;

    case WM_LBUTTONUP:
        /*              ProfStop(); */
        ReleaseCapture();
        msgg = msggMouseUp;
        if(!pgmCur->fButtonDown)
            break;
        goto DoMouse;

    case WM_MOUSEMOVE:
        msgg = msggMouseMove;
        if(!pgmCur->fButtonDown)
            break;
DoMouse:
            Assert(pgmCur != NULL);
            LONG2POINT( lParam, pt );
            Assert(pgmCur);
            SendGmMsg(pgmCur, msgg, (INT_PTR) &pt, 0);
            break;


    case WM_COMMAND:
        switch( GET_WM_COMMAND_ID( wParam, lParam ))
        {
            /* Game menu */
            case idsInitiate:
                NewGame(fTrue, fFalse);
                break;
            case idsUndo:
                Assert(pgmCur);
                SendGmMsg(pgmCur, msggUndo, 0, 0);
                break;
            case idsBacks:
                DoBacks();
                break;
            case idsOptions:
                DoOptions();
                break;
            case idsExit:
                PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                break;
            /* Help Menu */
            case (WORD)idsHelpIndex:
            case (WORD)idsHelpSearch:
            case (WORD)idsHelpUsing:
                DoHelp( (INT)(SHORT)GET_WM_COMMAND_ID( wParam, lParam ));
                break;
            case idsAbout:
            {
                TCHAR szExtraInfo[100];
                CchString( szExtraInfo, idsExtraInfo );
#ifndef _GAMBIT_
                ShellAbout(hwnd, szAppName, szExtraInfo, hIconMain);
#endif
                break;
            }
            case idsForceWin:
                SendGmMsg(pgmCur, msggForceWin, 0, 0);
                break;
#ifdef DEBUG
            case idsGameNo:
                if(FSetGameNo())
                    NewGame(fFalse, fFalse);
                break;

            case idsCardMacs:
                PrintCardMacs(pgmCur);
                break;
            case idsAssertFail:
                Assert(fFalse);
                break;
            case idsMarquee:
                break;

            case idsScreenShots:
                fScreenShots ^= 1;
                CheckMenuItem(GetMenu(hwnd), idsScreenShots, fScreenShots ? MF_CHECKED|MF_BYCOMMAND : MF_UNCHECKED|MF_BYCOMMAND);
                InvalidateRect(hwndStat, NULL, fTrue);
                if(fScreenShots)
                    InvalidateRect(hwnd, NULL, fTrue);
                break;
#endif
            default:
                break;
            }
            break;

    case WM_INITMENU:
            hmenu = GetMenu(hwnd);
            Assert(pgmCur);
            EnableMenuItem(hmenu, idsUndo,
                    pgmCur->udr.fAvail && !FSelOfGm(pgmCur) ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hmenu, idsInitiate, FSelOfGm(pgmCur) ? MF_DISABLED|MF_GRAYED : MF_ENABLED);
            EnableMenuItem(hmenu, idsBacks,    FSelOfGm(pgmCur) ? MF_DISABLED|MF_GRAYED : MF_ENABLED);
            EnableMenuItem(hmenu, idsAbout,    FSelOfGm(pgmCur) ? MF_DISABLED|MF_GRAYED : MF_ENABLED);
            break;

    case WM_PAINT:
        if(!fIconic)
        {
            DoPaint(hwnd);
            return(0L);
        }
        break;    
    }

    return(DefWindowProc(hwnd, wm, wParam, lParam));
}



HDC HdcSet(HDC hdc, X xOrg, Y yOrg)
{
    HDC hdcT = hdcCur;
    hdcCur = hdc;
    xOrgCur = xOrg;
    yOrgCur = yOrg;
    return hdcT;
}



BOOL FGetHdc()
{
    HDC hdc;

    Assert(hwndApp);
    if(hdcCur != NULL)
    {
        usehdcCur++;
        return fTrue;
    }

    hdc = GetDC(hwndApp);
    if(hdc == NULL)
        return fFalse;
    HdcSet(hdc, 0, 0);
    usehdcCur = 1;
    return fTrue;
}


VOID ReleaseHdc()
{
    if(hdcCur == NULL)
        return;
    if(--usehdcCur == 0)
    {
        ReleaseDC(hwndApp, hdcCur);
        hdcCur = NULL;
    }
}


WORD APIENTRY TimerProc(HWND hwnd, UINT wm, UINT_PTR id, DWORD dwTime)
{

    if(pgmCur != NULL)
		SendGmMsg(pgmCur, msggTimer, 0, 0);
    return fTrue;
}





VOID ChangeBack(INT mode)
{

    if(mode == modeFaceDown)
        return;
    modeFaceDown = mode;
    InvalidateRect(hwndApp, NULL, fTrue);
}


VOID NewGame(BOOL fNewSeed, BOOL fZeroScore)
{

#ifdef DEBUG
    InitDebug();
#endif
    if(fNewSeed)
    {
        static INT lastrnd= -1;     // previous rand() value
        INT rnd1;                   // trial rand() value
        INT Param;

        // It was reported that games never changed.
        // We could not repro it so see if it happens
        // and output a message to the debugger.
        //

        Param= (INT) time(NULL);
        srand( igmCur = ((WORD) Param) & 0x7fff);

#ifdef DEBUG
        rnd1= rand();

        if( lastrnd == rnd1 )
        {
            TCHAR szText[100];
            wsprintf(szText,TEXT("Games repeat: time= %d  GetLastError= %d\n"),
                     Param, GetLastError());
            OutputDebugString(szText);
        }

        lastrnd= rnd1;
#endif

    }

#ifdef DEBUG
    SendGmMsg(pgmCur, msggChangeScore, 0, 0);
#endif
    SendGmMsg(pgmCur, msggDeal, fZeroScore, 0);
}



INT_PTR APIENTRY About(HWND hdlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    if (iMessage == WM_COMMAND)
    {
        EndDialog(hdlg,fTrue);
        return fTrue;
    }
    else if (iMessage == WM_INITDIALOG)
        return fTrue;
    else
        return fFalse;
}


VOID DoHelp(INT idContext)
{
    CHAR sz[100];
    HWND hwndResult;

    LoadStringA(hinstApp, (WORD)idsHelpFile, (LPSTR)sz, 100);

#ifndef _GAMBIT_
    switch(idContext)
    {
        case idsHelpUsing:
            hwndResult = HtmlHelpA(GetDesktopWindow(), "NTHelp.chm", HH_DISPLAY_TOPIC, 0);
            break;
        case idsHelpIndex:
            hwndResult = HtmlHelpA(GetDesktopWindow(), sz, HH_DISPLAY_TOPIC, 0);
            break;
        case idsHelpSearch:
            hwndResult = HtmlHelpA(GetDesktopWindow(), sz, HH_DISPLAY_INDEX, 0);
            break;
    }
    if(!hwndResult)
        ErrorIds(idsNoHelp);
#endif
}


VOID GetIniFlags(BOOL *pfOutline)
{
    INI    ini;
    INT    mode;

    ini.w = 0;
    ini.grbit.fStatusBar = fStatusBar;
    ini.grbit.fTimedGame = fTimedGame;
    ini.grbit.fOutlineDrag = fOutlineDrag;
    ini.grbit.fDrawThree = ccrdDeal == 3;
    ini.grbit.fKeepScore = fKeepScore;
    ini.grbit.fSMD = 0;

    ini.w = GetIniInt(idsAppName, idsOpts, ini.w);

    fStatusBar = ini.grbit.fStatusBar ? 1 : 0;
    fTimedGame = ini.grbit.fTimedGame ? 1 : 0;
    *pfOutline = ini.grbit.fOutlineDrag ? 1 : 0;
    ccrdDeal = ini.grbit.fDrawThree ? 3 : 1;
    fKeepScore = ini.grbit.fKeepScore ? 1 : 0;
    switch(ini.grbit.fSMD)
    {
        default:
            smd = smdStandard;
            break;
        case 1:
            smd = smdVegas;
            break;
        case 2:
            smd = smdNone;
            break;
    }

    mode = GetIniInt(idsAppName, idsBack, rand() % cIDFACEDOWN) + IDFACEDOWNFIRST-1;
    ChangeBack(PegRange(mode, IDFACEDOWNFIRST, IDFACEDOWN12));

    iCurrency = GetIniInt(idsIntl, idsiCurrency, 0);
    FGetIniString(idsIntl, idssCurrency, szCurrency, TEXT("$"), sizeof(szCurrency));
}


VOID WriteIniFlags(INT wif)
{
    INI ini;

    if(wif & wifOpts)
    {
        ini.w = 0;
        ini.grbit.fStatusBar = fStatusBar;
        ini.grbit.fTimedGame = fTimedGame;
        ini.grbit.fOutlineDrag = fOutlineDrag;
        ini.grbit.fDrawThree = ccrdDeal == 3;
        ini.grbit.fKeepScore = fKeepScore;
        switch(smd)
        {
            default:
                Assert(fFalse);
                break;
            case smdStandard:
                ini.grbit.fSMD = 0;
                break;
            case smdVegas:
                ini.grbit.fSMD = 1;
                break;
            case smdNone:
                ini.grbit.fSMD = 2;
                break;
        }

        FWriteIniInt(idsAppName, idsOpts, ini.w);
    }
    if(wif & wifBack)
            FWriteIniInt(idsAppName, idsBack, modeFaceDown-IDFACEDOWNFIRST+1);
}
