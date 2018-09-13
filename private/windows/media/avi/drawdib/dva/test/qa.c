/*----------------------------------------------------------------------------*\
|   qa.c - A template for a Windows application                                |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include "qa.h"
#include "dva.h"
#include "lockbm.h"
#include "dibeng.inc"


#define DI_TYPE     0x4944  // 'DI'
#define DE_TYPE     0x4544  // 'DE'
#define RP_TYPE     0x5250  // 'RP'
#define M4_TYPE     0x344D  // 'M4'

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g e n e r a l   c o n s t a n t s                                          |
|                                                                              |
\*----------------------------------------------------------------------------*/

#define MAXSTR   80

#define EXPORT  FAR  PASCAL _export     // exported function
#define PRIVATE NEAR PASCAL             // function local to this segment
#define PUBLIC  FAR  PASCAL             // function external to this segment

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
static  char    szAppName[]="Quick App";

static  HANDLE  hInstApp;
static  HWND    hwndApp;

LPVOID lpScreen;
BITMAPINFOHEADER biScreen;

/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

LONG EXPORT AppWndProc (HWND hwnd, unsigned uiMessage, WORD wParam, LONG lParam);
int  ErrMsg (LPSTR sz,...);
BOOL fDialog(int id,HWND hwnd,FARPROC fpfn);

LONG NEAR PASCAL AppCommand(HWND hwnd, unsigned msg, WORD wParam, LONG lParam);

/*----------------------------------------------------------------------------*\
|   AppAbout( hDlg, uiMessage, wParam, lParam )                                |
|                                                                              |
|   Description:                                                               |
|       This function handles messages belonging to the "About" dialog box.    |
|       The only message that it looks for is WM_COMMAND, indicating the use   |
|       has pressed the "OK" button.  When this happens, it takes down         |
|       the dialog box.                                                        |
|                                                                              |
|   Arguments:                                                                 |
|       hDlg            window handle of about dialog window                   |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if message has been processed, else FALSE                         |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL EXPORT AppAbout( hDlg, uiMessage, wParam, lParam )
    HWND     hDlg;
    unsigned uiMessage;
    WORD     wParam;
    long     lParam;
{
    switch (uiMessage) {
        case WM_COMMAND:
            if (wParam == IDOK)
            {
                EndDialog(hDlg,TRUE);
            }
            break;

        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   AppInit( hInst, hPrev)                                                     |
|                                                                              |
|   Description:                                                               |
|       This is called when the application is first loaded into               |
|       memory.  It performs all initialization that doesn't need to be done   |
|       once per instance.                                                     |
|                                                                              |
|   Arguments:                                                                 |
|       hInstance       instance handle of current instance                    |
|       hPrev           instance handle of previous instance                   |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if successful, FALSE if not                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL AppInit(HANDLE hInst,HANDLE hPrev,WORD sw,LPSTR szCmdLine)
{
    WNDCLASS cls;
    int      dx,dy;

    /* Save instance handle for DialogBoxs */
    hInstApp = hInst;

    if (!hPrev) {
        /*
         *  Register a class for the main application window
         */
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = LoadIcon(hInst,"AppIcon");
        cls.lpszMenuName   = "AppMenu";
        cls.lpszClassName  = szAppName;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        cls.lpfnWndProc    = (WNDPROC)AppWndProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        if (!RegisterClass(&cls))
            return FALSE;
    }

    dx = GetSystemMetrics (SM_CXSCREEN);
    dy = GetSystemMetrics (SM_CYSCREEN);

    hwndApp = CreateWindow (szAppName,    // Class name
                            szAppName,              // Caption
                            WS_OVERLAPPEDWINDOW,    // Style bits
                            CW_USEDEFAULT, 0,       // Position
                            dx/2,dy/2,              // Size
                            (HWND)NULL,             // Parent window (no parent)
                            (HMENU)NULL,            // use class menu
                            (HANDLE)hInst,          // handle to window instance
                            (LPSTR)NULL             // no params to pass on
                           );
    ShowWindow(hwndApp,sw);

    return TRUE;
}

void AppExit()
{
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, cmdShow )                              |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|       hInst           instance handle of this instance of the app            |
|       hPrev           instance handle of previous instance, NULL if first    |
|       szCmdLine       ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
int PASCAL WinMain(HANDLE hInst, HANDLE hPrev, LPSTR szCmdLine, WORD sw)
{
    MSG     msg;

    /* Call initialization procedure */
    if (!AppInit(hInst,hPrev,sw,szCmdLine))
        return FALSE;

    /*
     * Polling messages from event queue
     */

    while (GetMessage(&msg,NULL,0,0))  {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    AppExit();
    return msg.wParam;
}

UINT BiosMode()
{
    UINT Mode;

    _asm
    {
        mov     ax, 6F04h               ;Get V7 mode
        int     10h
        cmp     al,04h
        jne     short v7
	mov	ax,0F00h		;Call BIOS to get mode back.
        int     10h                     ;al = mode we are in.
v7:     xor     ah,ah
        mov     Mode,ax
    }

    return Mode;
}

#if 0
LONG GetSelBase(short sel)
{
    _asm
    {
    }
}
#endif

/*----------------------------------------------------------------------------*\
|   AppPaint(hwnd, hdc)                                                        |
|                                                                              |
|   Description:                                                               |
|       The paint function.  Right now this does nothing.                      |
|                                                                              |
|   Arguments:                                                                 |
|       hwnd             window painting into                                  |
|       hdc              display context to paint to                           |
|                                                                              |
|   Returns:                                                                   |
|       nothing                                                                |
|                                                                              |
\*----------------------------------------------------------------------------*/

AppPaint (HWND hwnd, HDC hdc)
{
    RECT    rc;
    int     x,y;
    char    ach[128];
    char    achDisplay[80];
    UINT    wType;
    TEXTMETRIC tm;
    IBITMAP FAR *pbm;
    DWORD off;
    WORD sel;

    SetPixel(hdc, 0, 0, RGB(0,0,0));

#define TEXT(sz) \
    (TextOut(hdc, x, y, sz, lstrlen(sz)), y += tm.tmHeight+1)

#define PF1(sz, a)          PF3(sz, a, 0, 0)
#define PF2(sz, a, b)       PF3(sz, a, b, 0)
#define PF3(sz, a, b, c)    (wsprintf(ach, sz, a, b, c), TEXT(ach))
#define FLAG(w, f, s)       if ((w) & (f)) TEXT(s)

    SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));

    GetTextMetrics(hdc, &tm);

    GetClientRect(hwnd,&rc);

    x = 4; y = 4;

    SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc,GetSysColor(COLOR_WINDOW));

    GetPrivateProfileString("boot", "display.drv","",achDisplay,sizeof(achDisplay),"system.ini");

    wsprintf(ach, "Device: %dx%dx%d (%s)",
        GetDeviceCaps(hdc, HORZRES),
        GetDeviceCaps(hdc, VERTRES),
        GetDeviceCaps(hdc, BITSPIXEL) *
        GetDeviceCaps(hdc, PLANES),
        (LPSTR)achDisplay);

    TEXT(ach);
    PF1("Bios Mode = %04X",BiosMode());

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_DEVBITS)
        TEXT("  Device bitmap support");

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        TEXT("  Palette device");
    else
        TEXT("  RGB device (non-palette)");

    if (lpScreen != NULL)
    {
        TEXT("Direct Screen access");
    }

    if (pbm = (IBITMAP FAR *)GetPDevice(hdc))
    {
        PF1("lpPDevice = %08lX",pbm);

        if (pbm->bmType == DI_TYPE ||
            pbm->bmType == DE_TYPE ||
            pbm->bmType == RP_TYPE ||
            pbm->bmType == M4_TYPE)
        {
            DIBENGINE FAR *pde = (DIBENGINE FAR *)pbm;

            PF2("   deType       = %c%c",  LOBYTE(pde->deType), HIBYTE(pde->deType));
            PF1("   deWidth      = %d",    pde->deWidth);
            PF1("   deHeight     = %d",    pde->deHeight);
            PF1("   deWidthBytes = %d",    pde->deWidthBytes);
            PF1("   dePlanes     = %d",    pde->dePlanes);
            PF1("   deBitsPixel  = %d",    pde->deBitsPixel);
            PF1("   deReserved1  = %08lX", pde->deReserved1);
            PF1("   deDeltaScan  = %ld",   pde->deDeltaScan);
            PF1("   delpPDevice  = %08lX", pde->delpPDevice);
            PF2("   deBits       = %04X:%08lX",pde->deBitsSelector,pde->deBitsOffset);
            PF1("         base   = %08lX",GetSelectorBase(pde->deBitsSelector));
            PF1("         limit  = %08lX",GetSelectorLimit(pde->deBitsSelector));
            PF1("   deFlags      = %04X",  pde->deFlags);
            FLAG(pde->deFlags, MINIDRIVER      , "    MINIDRIVER   ");
            FLAG(pde->deFlags, PALETTIZED      , "    PALETTIZED   ");
            FLAG(pde->deFlags, SELECTEDDIB     , "    SELECTEDDIB  ");
            FLAG(pde->deFlags, CURSOREXCLUDE   , "    CURSOREXCLUDE");
            FLAG(pde->deFlags, DISABLED        , "    DISABLED     ");
            FLAG(pde->deFlags, VRAM            , "    VRAM         ");
            FLAG(pde->deFlags, BANKEDVRAM      , "    BANKEDVRAM   ");
            FLAG(pde->deFlags, BANKEDSCAN      , "    BANKEDSCAN   ");
            PF1("   deVersion    = %04X",  pde->deVersion);
            PF1("   deBitmapInfo = %08lX", pde->deBitmapInfo);
        }
        else
        {
            PF1("   bmType         = %04X",pbm->bmType);
            PF1("   bmWidth        = %d",pbm->bmWidth);
            PF1("   bmHeight       = %d",pbm->bmHeight);
            PF1("   bmWidthBytes   = %d",pbm->bmWidthBytes);
            PF1("   bmPlanes       = %d",pbm->bmPlanes);
            PF1("   bmBitsPixel    = %d",pbm->bmBitsPixel);

            sel = ((WORD FAR *)&pbm->bmBits)[1];
            off = ((WORD FAR *)&pbm->bmBits)[0];

            PF2("   bmBits         = %04X:%04X",sel,(WORD)off);
            PF1("           base   = %08lX",GetSelectorBase(sel));
            PF1("           limit  = %08lX",GetSelectorLimit(sel));

            sel = ((WORD FAR  *)&pbm->bmBits)[2];
            off = ((DWORD FAR *)&pbm->bmBits)[0];
            PF2("   bmBits48       = %04X:%08lX",sel,off);
            PF1("           base   = %08lX",GetSelectorBase(sel));
            PF1("           limit  = %08lX",GetSelectorLimit(sel));

            PF1("   bmWidthPlanes  = %d",pbm->bmWidthPlanes);
            PF1("   bmlpPDevice    = %08lX",pbm->bmlpPDevice);
            PF1("   bmSegmentIndex = %04X",pbm->bmSegmentIndex);
            PF1("   bmScanSegment  = %d",pbm->bmScanSegment);
            PF1("   bmFillBytes    = %d",pbm->bmFillBytes);
            PF1("   reserved1      = %d",pbm->reserved1);
            PF1("   reserved2      = %d",pbm->reserved2);
        }
    }

    if (CanLockBitmaps() && GetBitmapType())
    {
        wType = GetBitmapType();

        TEXT("Can Lock Bitmaps");

	if (wType & BM_HUGE)
	    TEXT("  Huge Bitmaps");
	else
	    TEXT("  Flat Bitmaps");

	if (wType & BM_BOTTOMTOTOP)
	    TEXT("  Bottom to top Bitmaps");
	else
	    TEXT("  Top to bottom Bitmaps");

        switch (wType & BM_TYPE)
        {
            case BM_VGA:    TEXT("  Bitmap type: VGA"); break;
            case BM_1BIT:   TEXT("  Bitmap type: 1 BIT"); break;
            case BM_4BIT:   TEXT("  Bitmap type: 4 BIT"); break;
            case BM_8BIT:   TEXT("  Bitmap type: 8 BIT"); break;
            case BM_16555:  TEXT("  Bitmap type: 16 BIT RGB 555"); break;
            case BM_16565:  TEXT("  Bitmap type: 16 BIT RGB 565"); break;
            case BM_24BGR:  TEXT("  Bitmap type: 24 BIT BGR"); break;
            case BM_24RGB:  TEXT("  Bitmap type: 24 BIT RGB"); break;
            case BM_32BGR:  TEXT("  Bitmap type: 32 BIT BGR"); break;
            case BM_32RGB:  TEXT("  Bitmap type: 32 BIT RGB"); break;
	    default:	    TEXT("  Bitmap type: Unknown"); break;
        }
    }

    return TRUE;
}

/*----------------------------------------------------------------------------*\
|                                                                              |
|   w i n d o w   p r o c s                                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|   AppWndProc( hwnd, uiMessage, wParam, lParam )                              |
|                                                                              |
|   Description:                                                               |
|       The window proc for the app's main (tiled) window.  This processes all |
|       of the parent window's messages.                                       |
|                                                                              |
|   Arguments:                                                                 |
|       hwnd            window handle for the window                           |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       0 if processed, nonzero if ignored                                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
LONG EXPORT AppWndProc(hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    WORD     wParam;
    long     lParam;
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg) {
        case WM_CREATE:
            hdc = GetDC(hwnd);
            lpScreen = DVAGetSurface(hdc, &biScreen);
            ReleaseDC(hwnd, hdc);
            break;

        case WM_SYSCOLORCHANGE:
            break;

        case WM_TIMER:
            break;

        case WM_ERASEBKGND:
            break;

        case WM_INITMENU:
            EnableMenuItem((HMENU)wParam, MENU_TEST,  lpScreen == NULL ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem((HMENU)wParam, MENU_TEST1, lpScreen == NULL ? MF_GRAYED : MF_ENABLED);
            break;

        case WM_COMMAND:
            return AppCommand(hwnd,msg,wParam,lParam);

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            break;

        case WM_PAINT:
            BeginPaint(hwnd,&ps);
            AppPaint (hwnd,ps.hdc);
            EndPaint(hwnd,&ps);
            return 0L;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

void NEAR PASCAL AlignPlaybackWindow(HWND hwnd, BOOL fAlign)
{
    DWORD dw;
    int x,y;
    RECT rc;
    HDC hdc;

    #define X_ALIGN 4
    #define Y_ALIGN 4

    hdc = GetDC(hwnd);
    dw = GetDCOrg(hdc);
    x = LOWORD(dw);
    y = HIWORD(dw);
    ReleaseDC(hwnd, hdc);

    if (!fAlign)
    {
        x++;
        y++;
    }

    if ((x & (X_ALIGN-1)) || (y & (Y_ALIGN-1)))
    {
        //
        // dont move the window if it does not want to be moved.
        //
        if (IsWindowVisible(hwnd) &&
           !IsZoomed(hwnd) &&
           !IsIconic(hwnd) &&
            IsWindowEnabled(hwnd))
        {
            GetWindowRect(hwnd, &rc);
            OffsetRect(&rc, -(x & (X_ALIGN-1)), -(y & (Y_ALIGN-1)));

            if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
                ScreenToClient(GetParent(hwnd), (LPPOINT)&rc);

            SetWindowPos(hwnd,NULL,rc.left,rc.top,0,0,
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
        }
    }
}

LONG NEAR PASCAL AppCommand (hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    WORD     wParam;
    long     lParam;
{
    HDC hdc;
    LPVOID lpScreen;
    BITMAPINFOHEADER bi;

    switch(wParam)
    {
        case MENU_ABOUT:
            fDialog(ABOUTBOX,hwnd,(FARPROC)AppAbout);
            break;

        case MENU_OPEN:
            break;

        case MENU_EXIT:
            PostMessage(hwnd,WM_CLOSE,0,0L);
            break;

        case MENU_XALIGN:
            AlignPlaybackWindow(hwnd, FALSE);
            break;

        case MENU_ALIGN:
            AlignPlaybackWindow(hwnd, TRUE);
            break;

        case MENU_TEST1:
            hdc = GetDC(hwnd);

            lpScreen = DVAGetSurface(hdc, &bi);

            if (lpScreen)
            {
                UINT u;
                RECT rc;
                DWORD dw;
                int x,y,dx,dy,w;
                DWORD time;

                extern void FAR PASCAL DIBRect(LPVOID, LONG, int, int, int, int, UINT);

                GetClientRect(hwnd, &rc);
                dw = GetDCOrg(hdc);

                x = LOWORD(dw) * (int)bi.biBitCount / 8;
                y = HIWORD(dw);
                dx = rc.right * (int)bi.biBitCount / 8;
                dy = rc.bottom;
                w  = bi.biWidth * (int)bi.biBitCount / 8;

                time = timeGetTime();

                for (u=0; u<256; u++)
                {
                    DVABeginAccess(hdc,x,y,dx,dy);
                    DIBRect(lpScreen,w,x,y,dx,dy,u);
                    DVAEndAccess(hdc);
                }

                time = timeGetTime() - time;

                ErrMsg("DVA %d.%03d fps",
                    (UINT)(1000l * 256 / time),
                    (UINT)((1000000l * 256 / time) % 1000));
            }

            ReleaseDC(hwnd, hdc);
            break;

        case MENU_TEST:
            hdc = GetDC(hwnd);

            lpScreen = DVAGetSurface(hdc, &bi);

            if (lpScreen)
            {
                UINT u;
                RECT rc;
                DWORD dw;
                int x,y,dx,dy,w;
                DWORD time;

                extern void FAR PASCAL DIBRect(LPVOID, LONG, int, int, int, int, UINT);

                GetClientRect(hwnd, &rc);
                dw = GetDCOrg(hdc);

                x = LOWORD(dw) * (int)bi.biBitCount / 8;
                y = HIWORD(dw);
                dx = rc.right * (int)bi.biBitCount / 8;
                dy = rc.bottom;
                w  = bi.biWidth * (int)bi.biBitCount / 8;

                DVABeginAccess(hdc, x, y, dx, dy);

                time = timeGetTime();

                for (u=0; u<256; u++)
                {
                    DIBRect(lpScreen,w,x,y,dx,dy,u);
                }

                time = timeGetTime() - time;

                DVAEndAccess(hdc);

                ErrMsg("DVA %d.%03d fps",
                    (UINT)(1000l * 256 / time),
                    (UINT)((1000000l * 256 / time) % 1000));
            }

            ReleaseDC(hwnd, hdc);
            break;
    }
    return 0L;
}

/*----------------------------------------------------------------------------*\
|   ErrMsg - Opens a Message box with a error message in it.  The user can     |
|            select the OK button to continue                                  |
\*----------------------------------------------------------------------------*/
int ErrMsg (LPSTR sz,...)
{
    char ach[128];

    wvsprintf (ach,sz,(LPSTR)(&sz+1));   /* Format the string */
    MessageBox(NULL,ach,NULL,MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   fDialog(id,hwnd,fpfn)                                                      |
|                                                                              |
|   Description:                                                               |
|       This function displays a dialog box and returns the exit code.         |
|       the function passed will have a proc instance made for it.             |
|                                                                              |
|   Arguments:                                                                 |
|       id              resource id of dialog to display                       |
|       hwnd            parent window of dialog                                |
|       fpfn            dialog message function                                |
|                                                                              |
|   Returns:                                                                   |
|       exit code of dialog (what was passed to EndDialog)                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL fDialog(int id,HWND hwnd,FARPROC fpfn)
{
    BOOL        f;
    HANDLE      hInst;

    hInst = GetWindowWord(hwnd,GWW_HINSTANCE);
    fpfn  = MakeProcInstance(fpfn,hInst);
    f = DialogBox(hInst,MAKEINTRESOURCE(id),hwnd,fpfn);
    FreeProcInstance (fpfn);
    return f;
}
