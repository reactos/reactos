/*----------------------------------------------------------------------------*\
|   qa.c - A template for a Windows application 			       |
|                                                                              |
|   Usage:                                                                     |
|	To make a quick windows application, modify the following files        |
|	    QA.C	    - App source code				       |
|	    MAKEFILE	    - App makefile (unix make, none of this dos crap)  |
|	    QA.H	    - App constants				       |
|	    QA.DEF	    - App exported functions and module name	       |
|	    QA.RC	    - App resources, DLG boxes, ...		       |
|           QA.ICO          - App icon  (2.x format)                           |
|           QA.IC3          - App icon  (3.x format)                           |
|                                                                              |
|   History:                                                                   |
|	01/01/88 toddla     Created					       |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>       // for timeGetTime
#include <commdlg.h>
#include <vfw.h>
#include "ddtest.h"
#include "dib.h"
#include "ddsucks.h"
#ifdef WIN32
#include <string.h>
#endif
#include <stdarg.h>

#define BI_CRAM     mmioFOURCC('C','R','A','M')

#define NFRAMES 100
#define XFRAMES 30

/* Macro to swap two values */
#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g e n e r a l   c o n s t a n t s                                          |
|                                                                              |
\*----------------------------------------------------------------------------*/

#define MAXSTR   80

#ifndef WIN32
#define EXPORT	FAR  PASCAL _export	// exported function
#endif
#define PRIVATE NEAR PASCAL		// function local to this segment

#define PUBLIC	FAR  PASCAL		// function external to this segment

typedef LONG (FAR PASCAL *LPWNDPROC)(HWND, UINT, UINT, LONG); // pointer to a window procedure

#define DibPtr(lpbi) ((LPBYTE)(lpbi) + \
            (int)((LPBITMAPINFOHEADER)(lpbi))->biSize + \
            (int)((LPBITMAPINFOHEADER)(lpbi))->biClrUsed * sizeof(RGBQUAD) )

#define FPFIT(time,n) time/1000,time%1000,            \
            time ? (1000l * n / time) : 0,            \
            time ? (1000000l * n / time) % 1000: 0

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
static  TCHAR    szAppName[]=TEXT("DDTest");

static  HANDLE  hInstApp;
static  HWND    hwndApp;

HANDLE             hdibCurrent;
LPBITMAPINFOHEADER lpbiCurrent;
BITMAPINFOHEADER   biCurrent;
RECT               rcCurrent = {0,0,0,0};
RECT               rcDraw;
RECT               rcSource;

HDRAWDIB           hdd = NULL;
UINT               wDrawDibFlags = 0;

TCHAR		   achFileName[128] = TEXT("TestDib8");

#ifdef WIN32
#define ProfBegin()
#define ProfEnd()
#else
#define ProfBegin() ProfClear(); ProfSampRate(5,1); ProfStart();
#define ProfEnd()   ProfStop(); ProfFlush();
#endif

static HCURSOR hcurSave;
#define StartWait() hcurSave = SetCursor(LoadCursor(NULL,IDC_WAIT))
#define EndWait()   SetCursor(hcurSave)

static UINT (FAR *displayFPS)[3][2];

/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

LONG _export AppWndProc (HWND hwnd, unsigned uiMessage, UINT wParam, LONG lParam);
int  ErrMsg (LPTSTR sz,...);
BOOL fDialog(int id,HWND hwnd,FARPROC fpfn);

void RandRect(PRECT prc, int dx, int dy);

HANDLE CopyHandle(HANDLE h);
HANDLE MakeDib(UINT bits);

LONG NEAR PASCAL AppCommand(HWND hwnd, unsigned msg, UINT wParam, LONG lParam);

void DrawDibTest(void);
void TimeDrawDib(void);

#ifndef QUERYDIBSUPPORT
    #define QUERYDIBSUPPORT     3073
    #define QDI_SETDIBITS       0x0001
    #define QDI_GETDIBITS       0x0002
    #define QDI_DIBTOSCREEN     0x0004
    #define QDI_STRETCHDIB      0x0008
#endif

DWORD QueryDibSupport(LPBITMAPINFOHEADER lpbi)
{
    HDC hdc;
    DWORD dw = 0;

    DPF(("QueryDibSupport"));
    hdc = GetDC(NULL);

    //
    // send the Escape to see if they support this DIB
    //
    if (!Escape(hdc, QUERYDIBSUPPORT, (int)lpbi->biSize, (LPVOID)lpbi, (LPVOID)&dw) > 0)
        dw = (DWORD)-1;

    ReleaseDC(NULL, hdc);
    return dw;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

static BOOL gfResource;

static int iCompressor = -1;
static int nCompressors;
static HIC ahic[100];

void FreeDib()
{
    DPF(("FreeDib"));
    if (gfResource)
    {
        UnlockResource(hdibCurrent);
        FreeResource(hdibCurrent);
    }
    else if (hdibCurrent)
        GlobalFree(hdibCurrent);

    hdibCurrent = NULL;
    gfResource = FALSE;
}

void InitDIB(HWND hwnd, HANDLE hdib, LPTSTR szFile)
{
    HANDLE h;
    TCHAR ach[80];
    int i;
    DWORD dw;

    if (hdib == NULL) {
	DPF(("InitDib; File %ls", szFile));
        hdib = OpenDIB(szFile, 0);
    } else {
	DPF(("InitDib; hDib 0x%08X", hdib));
    }

    if (hdib == NULL)
    {
        if (h = FindResource(hInstApp, szFile, RT_BITMAP))
        {
            FreeDib();
            hdib = LoadResource(hInstApp, h);

            hdibCurrent = hdib;
            lpbiCurrent = (LPVOID)LockResource(hdibCurrent);
            gfResource = TRUE;
        }

        if (hdib == NULL)
        {
            ErrMsg(TEXT("Unable to open %ls"),(LPTSTR)szFile);
            return;
        }
    }
    else
    {
        FreeDib();
        hdibCurrent = hdib;
        lpbiCurrent = (LPVOID)GlobalLock(hdibCurrent);
    }

    if (lpbiCurrent->biClrUsed == 0 && lpbiCurrent->biBitCount <= 8)
        lpbiCurrent->biClrUsed = (1 << (int)lpbiCurrent->biBitCount);

    DibInfo(lpbiCurrent, &biCurrent);
    SetRect(&rcCurrent, 0, 0, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
    SetRect(&rcSource,  0, 0, (int)biCurrent.biWidth, (int)biCurrent.biHeight);

    switch (biCurrent.biCompression)
    {

        case BI_RGB:    dw= mmioFOURCC('N','o','n','e'); break;
        case BI_RLE4:   dw= mmioFOURCC('R','l','e','4'); break;
        case BI_RLE8:   dw= mmioFOURCC('R','l','e','8'); break;
        default:	dw= biCurrent.biCompression;

    }

    for (i=0; i<nCompressors; i++)
    {
        if (ICDecompressQuery(ahic[i], lpbiCurrent, NULL) == ICERR_OK)
            iCompressor = i;
    }

    wsprintf(ach, TEXT("%ls (%dx%dx%d '%.4hs' %dk)"),
            (LPTSTR)szAppName,
            (int)biCurrent.biWidth,
            (int)biCurrent.biHeight,
            (int)biCurrent.biBitCount,
            (LPSTR) &dw,
            (int)(biCurrent.biSizeImage/1024));
    DPF(("Setting title: %ls", ach));

    SetWindowText(hwnd, ach);

    InvalidateRect(hwnd,NULL,TRUE);
}

void InitCompress(HWND hwnd)
{
    ICINFO icinfo;
    HIC    hic;
    int    i;
    HMENU  hmenu;

    DPF(("InitCompress"));
    hmenu = GetSubMenu(GetMenu(hwnd), 2);
    DeleteMenu(hmenu, MENU_COMPRESS, MF_BYCOMMAND);

    for (nCompressors=i=0; ICInfo(ICTYPE_VIDEO, i, &icinfo); i++)
    {
        hic = ICOpen(ICTYPE_VIDEO, icinfo.fccHandler, ICMODE_COMPRESS|ICMODE_DECOMPRESS);

	if (hic) {
            ICGetInfo(hic, &icinfo, sizeof(icinfo));

            if (ICQueryConfigure(hic))
                lstrcat(icinfo.szDescription, TEXT("..."));

            InsertMenu(hmenu, 0, MF_STRING|MF_BYPOSITION, MENU_COMPRESS+nCompressors, icinfo.szDescription);
            ahic[nCompressors++] = hic;
	}
    }
}

void TermCompress()
{
    int i;

    for (i=0; i<nCompressors; i++)
    {
        ICClose(ahic[i]);
    }
}

void TestCompress(int nFrames)
{
    HIC hic = ahic[iCompressor];
    LPBITMAPINFOHEADER lpbi;
    int   i;
    DWORD dw;
    DWORD dwFlags;
    DWORD dwQuality;
    DWORD ckid;
    DWORD time;

    DPF(("TestCompress"));
    if (hdibCurrent == NULL || ICCompressQuery(hic, lpbiCurrent, NULL) != ICERR_OK)
        return;

    StartWait();

    lpbi = (LPVOID)GlobalAllocPtr(GHND, sizeof(*lpbi) + 256*sizeof(RGBQUAD));

    ICCompressGetFormat(hic, lpbiCurrent, lpbi);

    dw = ICCompressGetSize(hic, lpbiCurrent, lpbi);

    lpbi = (LPVOID)GlobalReAllocPtr(lpbi, dw + sizeof(*lpbi) + 256*sizeof(RGBQUAD), 0);

    ICCompressBegin(hic, lpbiCurrent, lpbi);

    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
        lpbi->biClrUsed = (1 << (int)lpbi->biBitCount);

    dwFlags = 0;
    ckid = 0;
    dwQuality = ICGetDefaultQuality(hic);

    ProfBegin();
    time = timeGetTime();

    for (i=0; i<nFrames; i++)
        dw = ICCompress(hic,
                0,                  // flags
                lpbi,               // output format
                DibPtr(lpbi),       // output data
                lpbiCurrent,        // format of frame to compress
                DibPtr(lpbiCurrent),// frame data to compress
                &ckid,              // ckid for data in AVI file
                &dwFlags,           // flags in the AVI index.
                0,                  // frame number of seq.
                0,                  // reqested size in bytes. (if non zero)
                dwQuality,          // quality
                NULL,               // format of previous frame
                NULL);              // previous frame

    time = timeGetTime() - time;
    ProfEnd();
    EndWait();

    ICCompressEnd(hic);

#ifdef WIN32
    InitDIB(hwndApp, (HANDLE)lpbi, NULL);
#else
    InitDIB(hwndApp, (HANDLE)SELECTOROF(lpbi), NULL);
#endif

    if (nFrames > 1)
        ErrMsg(TEXT("CompressTime = %ld.%03ld sec %ld.%03ld fps"),
            FPFIT(time, nFrames)
            );
}

DWORD TimeDecompress(HIC hic, int bits)
{
    LPBITMAPINFOHEADER lpbi;
    int   i;
    HANDLE hdib;
    DWORD time = 0;

    DPF(("TimeDecompress"));
    hdib = CreateDib(bits, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
    lpbi = (LPVOID)GlobalLock(hdib);

    if (ICDecompressQuery(hic, lpbiCurrent, lpbi) == ICERR_OK)
    {
	ICDecompressBegin(hic, lpbiCurrent, lpbi);

	ProfBegin();
	time = timeGetTime();

	for (i=0; i<NFRAMES; i++)
	    ICDecompress(hic, 0, lpbiCurrent, DibPtr(lpbiCurrent),
                lpbi, DibPtr(lpbi));

        time = timeGetTime() - time;

	ProfEnd();

	ICDecompressEnd(hic);
    }

    GlobalFree(hdib);

    return time;
}

void TestDecompress()
{
    HIC hic = ahic[iCompressor];
    DWORD time8;
    DWORD time16;
    DWORD time24;

    DPF(("TestDecompress"));
    if (hdibCurrent == NULL || ICDecompressQuery(hic, lpbiCurrent, NULL) != ICERR_OK)
        return;

    StartWait();
    time8  = TimeDecompress(hic, 8);
    time16 = TimeDecompress(hic, 16);
    time24 = TimeDecompress(hic, 24);
    EndWait();

    ErrMsg(TEXT("DecompressTime:\n\n")
           TEXT("8bit = \t%ld.%03ld sec %ld.%03ld fps\n")
           TEXT("16bit =\t%ld.%03ld sec %ld.%03ld fps\n")
           TEXT("24bit =\t%ld.%03ld sec %ld.%03ld fps\n"),
            FPFIT(time8, NFRAMES),
            FPFIT(time16, NFRAMES),
            FPFIT(time24, NFRAMES)
            );
}

/*----------------------------------------------------------------------------*\
|   AppAbout( hDlg, uiMessage, wParam, lParam ) 			       |
|									       |
|   Description:							       |
|	This function handles messages belonging to the "About" dialog box.    |
|	The only message that it looks for is WM_COMMAND, indicating the use   |
|	has pressed the "OK" button.  When this happens, it takes down	       |
|	the dialog box. 						       |
|									       |
|   Arguments:								       |
|	hDlg		window handle of about dialog window		       |
|	uiMessage	message number					       |
|	wParam		message-dependent				       |
|	lParam		message-dependent				       |
|									       |
|   Returns:								       |
|	TRUE if message has been processed, else FALSE			       |
|									       |
\*----------------------------------------------------------------------------*/
BOOL _export AppAbout( hDlg, uiMessage, wParam, lParam )
    HWND     hDlg;
    unsigned uiMessage;
    UINT     wParam;
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
|   AppInit( hInst, hPrev)						       |
|									       |
|   Description:							       |
|	This is called when the application is first loaded into	       |
|	memory.  It performs all initialization that doesn't need to be done   |
|	once per instance.						       |
|									       |
|   Arguments:								       |
|	hInstance	instance handle of current instance		       |
|	hPrev		instance handle of previous instance		       |
|									       |
|   Returns:								       |
|	TRUE if successful, FALSE if not				       |
|									       |
\*----------------------------------------------------------------------------*/
BOOL AppInit(HANDLE hInst,HANDLE hPrev,UINT sw,LPSTR szCmdLine)
{
    WNDCLASS cls;
    int      dx,dy;

    DPF(("AppInit"));
    /* Save instance handle for DialogBoxs */
    hInstApp = hInst;

    if (szCmdLine && szCmdLine[0]) {
#ifdef UNICODE
        wsprintf(achFileName, TEXT("%hs"), szCmdLine);
#else
        lstrcpy(achFileName, szCmdLine);
#endif
    }

    if (!hPrev) {
	/*
	 *  Register a class for the main application window
	 */
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = LoadIcon(hInst,MAKEINTATOM(ID_APP));
        cls.lpszMenuName   = MAKEINTATOM(ID_APP);
        cls.lpszClassName  = MAKEINTATOM(ID_APP);
        cls.hbrBackground  = (HBRUSH)COLOR_WINDOW + 1;
        cls.hInstance      = hInst;
        cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        cls.lpfnWndProc    = (LPWNDPROC)AppWndProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        if (!RegisterClass(&cls))
	    return FALSE;
    }

    dx = GetSystemMetrics (SM_CXSCREEN);
    dy = GetSystemMetrics (SM_CYSCREEN);

    hwndApp = CreateWindow (MAKEINTATOM(ID_APP),    // Class name
                            szAppName,              // Caption
                            WS_OVERLAPPEDWINDOW,    // Style bits
                            CW_USEDEFAULT, 0,       // Position
                            dx*3/4,dy*3/4,          // Size
                            (HWND)NULL,             // Parent window (no parent)
                            (HMENU)NULL,            // use class menu
                            (HANDLE)hInst,          // handle to window instance
                            (LPTSTR)NULL             // no params to pass on
                           );
    ShowWindow(hwndApp,sw);

    return TRUE;
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, cmdShow )			       |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|	hInst		instance handle of this instance of the app	       |
|	hPrev		instance handle of previous instance, NULL if first    |
|       szCmdLine       ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
#else
int PASCAL WinMain(HANDLE hInst, HANDLE hPrev, LPSTR szCmdLine, int sw)
#endif
{
    MSG     msg;

    /* Call initialization procedure */
    if (!AppInit(hInst,hPrev,sw,szCmdLine))
        return FALSE;

    DPF(("WinMain... init done"));
    hdd = DrawDibOpen();

    /*
     * Polling messages from event queue
     */
    for (;;)
    {
        if (PeekMessage(&msg, NULL, 0, 0,PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            if (rcCurrent.right == -1)
                SendMessage(hwndApp, WM_TIMER, 0, 0);
            else
                WaitMessage();
        }
    }

    DrawDibClose(hdd);

    return msg.wParam;
}

/*----------------------------------------------------------------------------*\
|   AppWndProc( hwnd, uiMessage, wParam, lParam )			       |
|                                                                              |
|   Description:                                                               |
|       The window proc for the app's main (tiled) window.  This processes all |
|       of the parent window's messages.                                       |
|                                                                              |
|   Arguments:                                                                 |
|	hwnd		window handle for the window			       |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       0 if processed, nonzero if ignored                                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
LONG _export AppWndProc(hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    UINT     wParam;
    long     lParam;
{
    PAINTSTRUCT ps;
    BOOL        f;
    HDC 	hdc;
    int         i;

    //DPF(("AppWndProc:  hwnd %8x msg %8x   wP=%8x  lP=%8x\r\n", hwnd, msg, wParam, lParam));
    switch (msg) {
        case WM_CREATE:
	    InitDIB(hwnd, NULL, achFileName); // SD:  these 2 lines reversed in Win 16
	    InitCompress(hwnd);
	    break;

        case WM_LBUTTONDBLCLK:
            break;

        case WM_COMMAND:
            return AppCommand(hwnd,msg,wParam,lParam);

        case WM_INITMENU:
			DPF(("WM_INITMENU"));
            EnableMenuItem((HMENU)wParam, MENU_PASTE, IsClipboardFormatAvailable(CF_DIB) ? MF_ENABLED : MF_GRAYED);

            EnableMenuItem((HMENU)wParam, MENU_COPY, hdibCurrent ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_SAVE, hdibCurrent ? MF_ENABLED : MF_GRAYED);

            CheckMenuItem((HMENU)wParam, MENU_DIB_4,  biCurrent.biBitCount == 4  ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, MENU_DIB_8,  biCurrent.biBitCount == 8  ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, MENU_DIB_16, biCurrent.biBitCount == 16 ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, MENU_DIB_24, biCurrent.biBitCount == 24 ? MF_CHECKED : MF_UNCHECKED);

            CheckMenuItem((HMENU)wParam, MENU_STRETCH_1, rcCurrent.right == (int)biCurrent.biWidth ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, MENU_STRETCH_2, rcCurrent.right == (int)biCurrent.biWidth*2 ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, MENU_STRETCH_15, rcCurrent.right == (int)biCurrent.biWidth*3/2 ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, MENU_STRETCH_WIN, rcCurrent.right == 0 ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, MENU_STRETCH_HUH, rcCurrent.right == -1 ? MF_CHECKED : MF_UNCHECKED);

            CheckMenuItem((HMENU)wParam, MENU_JUST_DRAW_IT, (wDrawDibFlags & DDF_JUSTDRAWIT) ? MF_CHECKED : MF_UNCHECKED);

            CheckMenuItem((HMENU)wParam, MENU_FULLSCREEN, (wDrawDibFlags & DDF_FULLSCREEN) ? MF_CHECKED : MF_UNCHECKED);

            for (i=0; i<nCompressors; i++)
                CheckMenuItem((HMENU)wParam, MENU_COMPRESS+i, i==iCompressor ? MF_CHECKED : MF_UNCHECKED);

            EnableMenuItem((HMENU)wParam, MENU_TIME_DRAWDIB,   TRUE  ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_TEST_DRAWDIB,   TRUE  ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_TEST_DISPLAY,   TRUE  ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_TEST_ESCAPE,    TRUE  ? MF_ENABLED : MF_GRAYED);

            f = (iCompressor != -1) &&
                (ahic[iCompressor] != NULL) &&
                ICCompressQuery(ahic[iCompressor], lpbiCurrent, NULL) == ICERR_OK;

            EnableMenuItem((HMENU)wParam, MENU_TEST_COMPRESS, f ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_COMPRESS_IT,   f ? MF_ENABLED : MF_GRAYED);

            f = (iCompressor != -1) &&
                (ahic[iCompressor] != NULL) &&
                ICDecompressQuery(ahic[iCompressor], lpbiCurrent, NULL) == ICERR_OK;

            EnableMenuItem((HMENU)wParam, MENU_TEST_DECOMPRESS,f ? MF_ENABLED : MF_GRAYED);
            break;

        case WM_DESTROY:
            TermCompress();
            DrawDibClose(hdd);
            FreeDib();
	    PostQuitMessage(0);
	    break;

        case WM_CLOSE:
	    break;

        case WM_PALETTECHANGED:
            if ((HANDLE)wParam != hwnd) {
                hdc = GetDC(hwnd);

                if (DrawDibRealize(hdd, hdc, FALSE))
                    InvalidateRect(hwnd,NULL,TRUE);

                ReleaseDC(hwnd,hdc);

            }
            break;

	case WM_QUERYNEWPALETTE:
            hdc = GetDC(hwnd);

            if (f = DrawDibRealize(hdd, hdc, FALSE))
                InvalidateRect(hwnd,NULL,TRUE);

            ReleaseDC(hwnd,hdc);

            return f;

	case WM_ERASEBKGND:
	    hdc = (HDC) wParam;
            SaveDC(hdc);

            GetClientRect(hwnd, &rcDraw);

            if (!IsRectEmpty(&rcCurrent) && rcCurrent.right != -1)
            {
                SetRect(&rcDraw,(rcDraw.right  - rcCurrent.right)/2,
                                (rcDraw.bottom - rcCurrent.bottom)/2,
                                (rcDraw.right  - rcCurrent.right)/2  + rcCurrent.right,
                                (rcDraw.bottom - rcCurrent.bottom)/2 + rcCurrent.bottom);

                ExcludeClipRect(hdc,
                    rcDraw.left, rcDraw.top,
                    rcDraw.right, rcDraw.bottom);
            }

	    DefWindowProc(hwnd, msg, wParam, lParam);

            RestoreDC(hdc, -1);
            return 0L;

        case WM_TIMER:
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd,&ps);

            GetClientRect(hwnd, &rcDraw);

            if (rcCurrent.right == -1)
            {
                RandRect(&rcDraw, rcDraw.right, rcDraw.bottom);
                RandRect(&rcSource, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
            }
            else if (!IsRectEmpty(&rcCurrent))
            {
                SetRect(&rcDraw,(rcDraw.right  - rcCurrent.right)/2,
                                (rcDraw.bottom - rcCurrent.bottom)/2,
                                (rcDraw.right  - rcCurrent.right)/2  + rcCurrent.right,
                                (rcDraw.bottom - rcCurrent.bottom)/2 + rcCurrent.bottom);
            }

            if (hdibCurrent)
            {
                DrawDibDraw(hdd, hdc,
                    rcDraw.left,
                    rcDraw.top,
                    rcDraw.right-rcDraw.left,
                    rcDraw.bottom-rcDraw.top,
                    lpbiCurrent, NULL,
                    rcSource.left,
                    rcSource.top,
                    rcSource.right-rcSource.left,
                    rcSource.bottom-rcSource.top,
                    wDrawDibFlags);
            }

            EndPaint(hwnd,&ps);
            break;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

//static TCHAR achFileName[80];

LONG NEAR PASCAL AppCommand (hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    UINT     wParam;
    long     lParam;
{
    BITMAPINFOHEADER bi;
    DWORD flags16, flagsCram16, flagsCram8;
    OPENFILENAME ofn;
    HANDLE h;

    switch(wParam)
    {
	case MENU_ABOUT:
	    fDialog(ABOUTBOX,hwnd,(FARPROC)AppAbout);
            break;

	case MENU_EXIT:
	    PostMessage(hwnd,WM_CLOSE,0,0L);
            break;

        case MENU_COPY:
            if (!OpenClipboard(hwnd))
                break;

            EmptyClipboard();
            SetClipboardData(CF_DIB,CopyHandle(hdibCurrent));
            CloseClipboard();
            break;

        case MENU_PASTE:
            if (!OpenClipboard(hwnd))
                break;

            if (h = GetClipboardData(CF_DIB))
                InitDIB(hwnd, CopyHandle(h), NULL);

            CloseClipboard();
            break;

        case MENU_DIB_4:
            InitDIB(hwnd, NULL, TEXT("TestDib4"));
            break;

        case MENU_DIB_8:
            InitDIB(hwnd, NULL, TEXT("TestDib8"));
            break;

        case MENU_DIB_16:
        case MENU_DIB_24:
#ifdef WIN32
            // MakeDib will allocate and return a handle to the memory bitmap.
            // We cannot use MAKEINTATOM as that creates a 16 bit value - fine
            // when dealing with resources.  Not so good for 32 bit handles.
            InitDIB(hwnd, (LPTSTR)(MakeDib(wParam-MENU_DIB)), NULL);
#else
            InitDIB(hwnd, MakeDib(wParam-MENU_DIB), NULL);
#endif
            break;

        case MENU_STRETCH_1:
            SetRect(&rcSource, 0, 0, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
            SetRect(&rcCurrent, 0, 0, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;

        case MENU_STRETCH_2:
            SetRect(&rcSource, 0, 0, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
            SetRect(&rcCurrent, 0, 0, (int)biCurrent.biWidth*2, (int)biCurrent.biHeight*2);
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;

        case MENU_STRETCH_15:
            SetRect(&rcSource, 0, 0, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
            SetRect(&rcCurrent, 0, 0, (int)biCurrent.biWidth*3/2, (int)biCurrent.biHeight*3/2);
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;

        case MENU_STRETCH_WIN:
            SetRect(&rcSource, 0, 0, (int)biCurrent.biWidth, (int)biCurrent.biHeight);
            SetRectEmpty(&rcCurrent);
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;

        case MENU_STRETCH_HUH:
            SetRectEmpty(&rcSource);
            SetRectEmpty(&rcCurrent);
            rcCurrent.right = -1;
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MENU_JUST_DRAW_IT:
            wDrawDibFlags ^= DDF_JUSTDRAWIT;
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case MENU_COMPRESS+0:
        case MENU_COMPRESS+1:
        case MENU_COMPRESS+2:
        case MENU_COMPRESS+3:
        case MENU_COMPRESS+4:
        case MENU_COMPRESS+5:
        case MENU_COMPRESS+6:
        case MENU_COMPRESS+7:
        case MENU_COMPRESS+8:
        case MENU_COMPRESS+9:
            if (ahic[(int)wParam - MENU_COMPRESS])
            {
                iCompressor = (int)wParam - MENU_COMPRESS;
                ICConfigure(ahic[iCompressor], hwnd);
            }
            break;

        case MENU_COMPRESS_IT:
            TestCompress(1);
            break;

        case MENU_TEST_COMPRESS:
            TestCompress(NFRAMES/4);
            break;

        case MENU_TEST_DECOMPRESS:
            TestDecompress();
            break;

        case MENU_TEST_DRAWDIB:
            DrawDibTest();
            break;

        case MENU_TIME_DRAWDIB:
            TimeDrawDib();
            break;

        case MENU_TEST_DISPLAY:

            (LPVOID)displayFPS = (LPVOID)DrawDibProfileDisplay(NULL);

#define SUCK(bpp,n) \
            displayFPS[bpp/8][n][0]/10, displayFPS[bpp/8][n][0]%10, \
            displayFPS[bpp/8][n][1]/10, displayFPS[bpp/8][n][1]%10, \
            (LPSTR)(displayFPS[bpp/8][n][0] < displayFPS[bpp/8][n][1] ? "** POOR **" : "")

#define SUCKS(bpp) \
            SUCK(bpp,0), \
            SUCK(bpp,1), \
            SUCK(bpp,2)  \

            ErrMsg(
#if 0
                    TEXT("4 Bit DIBs     \tDibBlt  \tSet+Blt\n")
                    TEXT("    Stetch  x1 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch x2 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch xN \t%03d.%d \t%03d.%d %s\n\n")
#endif

                    TEXT("8 Bit DIBs     \tDibBlt  \tSet+Blt\n")
                    TEXT("    Stetch  x1 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch x2 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch xN \t%03d.%d \t%03d.%d %s\n\n")

                    TEXT("16 Bit DIBs\n")
                    TEXT("    Stetch  x1 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch x2 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch xN \t%03d.%d \t%03d.%d %s\n\n")

                    TEXT("24 Bit DIBs\n")
                    TEXT("    Stetch  x1 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch x2 \t%03d.%d \t%03d.%d %s\n")
                    TEXT("    Stretch xN \t%03d.%d \t%03d.%d %s\n"),

      //              SUCKS(4),
                    SUCKS(8),
                    SUCKS(16),
                    SUCKS(24)
                    );
            break;

        case MENU_TEST_ESCAPE:

            bi = *lpbiCurrent;

            bi.biBitCount = 16;
            flags16 = QueryDibSupport(&bi);

            bi.biBitCount = 16;
            bi.biCompression = BI_CRAM;
            flagsCram16 = QueryDibSupport(&bi);

            bi.biBitCount = 8;
            bi.biCompression = BI_CRAM;
            flagsCram8 = QueryDibSupport(&bi);

#define SDIB(f) \
	   (LPTSTR)((f != -1) ? ((f & QDI_SETDIBITS   ) ? TEXT("Yes") : TEXT("No")) : TEXT("Unknown") ), \
	   (LPTSTR)((f != -1) ? ((f & QDI_GETDIBITS   ) ? TEXT("Yes") : TEXT("No")) : TEXT("Unknown") ), \
	   (LPTSTR)((f != -1) ? ((f & QDI_DIBTOSCREEN ) ? TEXT("Yes") : TEXT("No")) : TEXT("Unknown") ), \
           (LPTSTR)((f != -1) ? ((f & QDI_STRETCHDIB  ) ? TEXT("Yes") : TEXT("No")) : TEXT("Unknown") )

            ErrMsg(
		    TEXT("16 Bit RGB 555\n")
		    TEXT("    SetDIBits  \t%s\n")
		    TEXT("    GetDIBits  \t%s\n")
                    TEXT("    ToScreen   \t%s\n")
		    TEXT("    Stretch    \t%s\n\n")

		    TEXT("8 Bit Cram\n")
		    TEXT("    SetDIBits  \t%s\n")
		    TEXT("    GetDIBits  \t%s\n")
                    TEXT("    ToScreen   \t%s\n")
                    TEXT("    Stretch    \t%s\n\n")

		    TEXT("16 Bit Cram\n")
		    TEXT("    SetDIBits  \t%s\n")
		    TEXT("    GetDIBits  \t%s\n")
                    TEXT("    ToScreen   \t%s\n")
                    TEXT("    Stretch    \t%s\n"),

                    SDIB(flags16),
                    SDIB(flagsCram8),
                    SDIB(flagsCram16)
                    );
            break;

        case MENU_TEST_ALL:
            ErrMsg(TEXT("You are dreaming!"));
            break;

        case MENU_SAVE:
            achFileName[0] = 0;

            /* prompt user for file to open */
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.hInstance = NULL;
            ofn.lpstrFilter = TEXT("Bitmaps\0*.dib;*.bmp\0All\0*.*\0");
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter = 0;
            ofn.nFilterIndex = 0;
            ofn.lpstrFile = achFileName;
            ofn.nMaxFile = sizeof(achFileName)/sizeof(TCHAR);
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = TEXT("Open Dib");
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.nFileOffset = 0;
            ofn.nFileExtension = 0;
            ofn.lpstrDefExt = NULL;
            ofn.lCustData = 0;
            ofn.lpfnHook = NULL;
            ofn.lpTemplateName = NULL;

            if (GetSaveFileName(&ofn))
            {
                if (iCompressor != -1)
                    TestCompress(1);

                WriteDIB(achFileName, 0, hdibCurrent);
            }
	    break;


        case MENU_OPEN:
            achFileName[0] = 0;

            /* prompt user for file to open */
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.hInstance = NULL;
            ofn.lpstrFilter = TEXT("Bitmaps\0*.dib;*.bmp\0All\0*.*\0");
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter = 0;
            ofn.nFilterIndex = 0;
            ofn.lpstrFile = achFileName;
            ofn.nMaxFile = sizeof(achFileName)/sizeof(TCHAR);
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = TEXT("Open Dib");
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.nFileOffset = 0;
            ofn.nFileExtension = 0;
            ofn.lpstrDefExt = NULL;
            ofn.lCustData = 0;
            ofn.lpfnHook = NULL;
            ofn.lpTemplateName = NULL;

            if (GetOpenFileName(&ofn))
            {
                InitDIB(hwnd, NULL, achFileName);
            }
	    break;
    }
    return 0L;
}

/*----------------------------------------------------------------------------*\
|   ErrMsg - Opens a Message box with a error message in it.  The user can     |
|            select the OK button to continue                                  |
\*----------------------------------------------------------------------------*/
static TCHAR ach[2000];

int ErrMsg (LPTSTR sz,...)
{
    va_list va;
    DPF(("Errmsg"));

    va_start(va, sz);
    wvsprintf(ach, sz, va);
    va_end(va);
    MessageBox(NULL,ach,NULL,MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   fDialog(id,hwnd,fpfn)						       |
|									       |
|   Description:                                                               |
|	This function displays a dialog box and returns the exit code.	       |
|	the function passed will have a proc instance made for it.	       |
|									       |
|   Arguments:                                                                 |
|	id		resource id of dialog to display		       |
|	hwnd		parent window of dialog 			       |
|	fpfn		dialog message function 			       |
|                                                                              |
|   Returns:                                                                   |
|	exit code of dialog (what was passed to EndDialog)		       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL fDialog(int id,HWND hwnd,FARPROC fpfn)
{
    BOOL	f;
    HANDLE	hInst;

#ifdef WIN32
    hInst = (HANDLE) GetWindowLong(hwnd, GWL_HINSTANCE);
#else
    hInst = GetWindowWord(hwnd,GWW_HINSTANCE);
#endif
    fpfn  = MakeProcInstance(fpfn,hInst);
    f = DialogBox(hInst,MAKEINTRESOURCE(id),hwnd, (DLGPROC)fpfn);
    FreeProcInstance (fpfn);
    return f;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
void TimeDrawDib()
{
    DWORD time;
    int i;
    HDC hdc;
    DRAWDIBTIME ddtime;
    BOOL f;

    StartWait();

    hdc = GetDC(hwndApp);

    DrawDibTime(hdd, NULL);

    f = (BOOL)DrawDibBegin(hdd, hdc,
                rcDraw.right-rcDraw.left,rcDraw.bottom-rcDraw.top,
                lpbiCurrent, -1, -1, wDrawDibFlags);

    if (!f)
    {
        ErrMsg(TEXT("Unable to draw"));
        return;
    }

    DrawDibRealize(hdd, hdc, FALSE);

    ProfBegin();
    time = timeGetTime();

    for (i=0; i<NFRAMES; i++)
        DrawDibDraw(hdd, hdc,
            rcDraw.left,
            rcDraw.top,
            rcDraw.right-rcDraw.left,
            rcDraw.bottom-rcDraw.top,
            lpbiCurrent, NULL,
            rcSource.left,
            rcSource.top,
            rcSource.right-rcSource.left,
            rcSource.bottom-rcSource.top,
            wDrawDibFlags | DDF_SAME_HDC | DDF_SAME_DRAW);

    time = timeGetTime() - time;
    ProfEnd();

    DrawDibEnd(hdd);

    ReleaseDC(hwndApp, hdc);

    EndWait();

    if (DrawDibTime(hdd, &ddtime))
    {
        ErrMsg(TEXT("DrawDibTime = %ld.%03ld sec %ld.%03ld fps\n\n")
               TEXT("timeDraw:      \t%ldms\t (%ld)\n")
               TEXT("timeDecompress:\t%ldms\t (%ld)\n")
               TEXT("timeDither:    \t%ldms\t (%ld)\n")
               TEXT("timeStretch:   \t%ldms\t (%ld)\n")
               TEXT("timeSetDIBits: \t%ldms\t (%ld)\n")
               TEXT("timeBlt:       \t%ldms\t (%ld)"),
            FPFIT(time, NFRAMES),
            ddtime.timeDraw,       ddtime.timeDraw       / ddtime.timeCount,
            ddtime.timeDecompress, ddtime.timeDecompress / ddtime.timeCount,
            ddtime.timeDither,     ddtime.timeDither     / ddtime.timeCount,
            ddtime.timeStretch,    ddtime.timeStretch    / ddtime.timeCount,
            ddtime.timeSetDIBits,  ddtime.timeSetDIBits  / ddtime.timeCount,
            ddtime.timeBlt,        ddtime.timeBlt        / ddtime.timeCount);
    }
    else
    {
        ErrMsg(TEXT("DrawDibTime = %ld.%03ld sec %ld.%03ld fps"),
           FPFIT(time, NFRAMES));
    }
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
DWORD DrawDibTimeIt()
{
    DWORD time;
    int i;
    HDC hdc;

    DPF(("DrawDibTime"));
    hdc = GetDC(hwndApp);

    DrawDibBegin(hdd, hdc,
                rcDraw.right-rcDraw.left,rcDraw.bottom-rcDraw.top,
                lpbiCurrent, -1, -1, wDrawDibFlags);

    DrawDibRealize(hdd, hdc, FALSE);

    ProfBegin();
    time = timeGetTime();

    for (i=0; i<XFRAMES; i++)
        DrawDibDraw(hdd, hdc,
            rcDraw.left,
            rcDraw.top,
            rcDraw.right-rcDraw.left,
            rcDraw.bottom-rcDraw.top,
            lpbiCurrent, NULL,
            rcSource.left,
            rcSource.top,
            rcSource.right-rcSource.left,
            rcSource.bottom-rcSource.top,
            wDrawDibFlags | DDF_SAME_HDC | DDF_SAME_DRAW);

    time = timeGetTime() - time;
    ProfEnd();

    DrawDibEnd(hdd);

    ReleaseDC(hwndApp, hdc);

    return time;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
void DrawDibTest()
{
    DWORD time[4][4];
    int size_cmd;

    DPF(("DrawDibTest"));
    StartWait();

#define DDTIME(bit_cmd, bit_n) \
    SendMessage(hwndApp, WM_COMMAND, bit_cmd, 0);                       \
    for (size_cmd=MENU_STRETCH_1; size_cmd<=MENU_STRETCH_2; size_cmd++) \
    {                                                                   \
        SendMessage(hwndApp, WM_COMMAND, size_cmd, 0);                  \
        time[bit_n][size_cmd-MENU_STRETCH_1] = DrawDibTimeIt();         \
    }

    DDTIME(MENU_DIB_4,  0)
    DDTIME(MENU_DIB_8,  1)
    DDTIME(MENU_DIB_16, 2)
    DDTIME(MENU_DIB_24, 3)

    SendMessage(hwndApp, WM_COMMAND, MENU_DIB_8, 0);
    SendMessage(hwndApp, WM_COMMAND, MENU_STRETCH_1, 0);

    EndWait();

    ErrMsg(
            TEXT("4 Bit Draw\n")
            TEXT("    x1  \t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x1.5\t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x2  \t%02ld.%ld sec \t%03ld.%ld fps\n\n")

            TEXT("8 Bit Draw\n")
            TEXT("    x1  \t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x1.5\t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x2  \t%02ld.%ld sec \t%03ld.%ld fps\n\n")

            TEXT("16 Bit Draw\n")
            TEXT("    x1  \t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x1.5\t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x2  \t%02ld.%ld sec \t%03ld.%ld fps\n\n")

            TEXT("24 Bit Draw\n")
            TEXT("    x1  \t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x1.5\t%02ld.%ld sec \t%03ld.%ld fps\n")
            TEXT("    x2  \t%02ld.%ld sec \t%03ld.%ld fps\n"),

            FPFIT(time[0][0], XFRAMES),
            FPFIT(time[0][1], XFRAMES),
            FPFIT(time[0][2], XFRAMES),

            FPFIT(time[1][0], XFRAMES),
            FPFIT(time[1][1], XFRAMES),
            FPFIT(time[1][2], XFRAMES),

            FPFIT(time[2][0], XFRAMES),
            FPFIT(time[2][1], XFRAMES),
            FPFIT(time[2][2], XFRAMES),

            FPFIT(time[3][0], XFRAMES),
            FPFIT(time[3][1], XFRAMES),
            FPFIT(time[3][2], XFRAMES)
            );
}

#define RAND(x)  (rand() % (x))
static DWORD dwRand = 0L;


WORD PASCAL rand(void)
{
    if (dwRand == 0)
        dwRand = GetCurrentTime();

    dwRand = dwRand * 214013L + 2531011L;
    return ((WORD)(dwRand >> 16) & 0xffffu);
}

void  NormalizeRect (PRECT prc)
{
    if (prc->right < prc->left)
        SWAP(prc->right,prc->left);
    if (prc->bottom < prc->top)
        SWAP(prc->bottom,prc->top);
}

void RandRect(PRECT prc, int dx, int dy)
{
    prc->left   = rand() % dx;
    prc->right  = rand() % dx;
    prc->top    = rand() % dy;
    prc->bottom = rand() % dy;
    NormalizeRect(prc);
}

HANDLE CopyHandle(HANDLE h)
{
    HANDLE hCopy;

    DPF(("Copying handle"));
    if (hCopy = GlobalAlloc(GHND,GlobalSize(h)))
        hmemcpy(GlobalLock(hCopy), GlobalLock(h), GlobalSize(h));

    return hCopy;
}

#define BITMAP_X    160
#define BITMAP_Y    120

HANDLE MakeDib(UINT bits)
{
    HANDLE hdib;
    LPBITMAPINFOHEADER lpbi;
    LPBYTE pb;
    int x,y;
    LONG biSizeImage = BITMAP_Y*(DWORD)((BITMAP_X*(bits/8)+3)&~3);

    DPF(("MakeDib routine"));
    hdib =GlobalAlloc(GHND,sizeof(BITMAPINFOHEADER)+biSizeImage);
    lpbi = (LPVOID)GlobalLock(hdib) ;
    pb   = (LPVOID)(lpbi+1);

    if (bits == 16)
    {
        for (y=0; y<BITMAP_Y; y++)
            for (x=0; x<BITMAP_X; x++)
            {
                *pb++ = (BYTE) ((UINT)y * 31 / (BITMAP_Y-1));
                *pb++ = (BYTE)(((UINT)x * 31 / (BITMAP_X-1)) << 2);
            }
    }
    else if (bits == 24)
    {
        for (y=0; y<BITMAP_Y; y++)
            for (x=0; x<BITMAP_X; x++)
            {
                *pb++ = (BYTE) ((UINT)y * 255u / (BITMAP_Y-1));
                *pb++ = (BYTE)~((UINT)x * 255u / (BITMAP_X-1));
                *pb++ = (BYTE) ((UINT)x * 255u / (BITMAP_X-1));
            }
    }

    lpbi->biSize            = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth           = BITMAP_X;
    lpbi->biHeight          = BITMAP_Y;
    lpbi->biPlanes          = 1;
    lpbi->biBitCount        = bits;
    lpbi->biCompression     = BI_RGB;
    lpbi->biSizeImage       = biSizeImage;
    lpbi->biXPelsPerMeter   = 0;
    lpbi->biYPelsPerMeter   = 0;
    lpbi->biClrUsed         = 0;
    lpbi->biClrImportant    = 0;

    return hdib;
}

/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * DDTEST=1
 *
 ****************************************************************************/

#ifdef DEBUG

#define MODNAME "DDTEST"

static BOOL fDebug = 1;

void FAR CDECL dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;

    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug", MODNAME, FALSE);

    if (!fDebug)
        return;

    lstrcpyA(ach, MODNAME ": ");
    va_start(va, szFormat);
    wvsprintfA(ach+lstrlenA(ach),szFormat, va);
    va_end(va);
    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
}

#endif
