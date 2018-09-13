#include <windows.h>
#include <drawdib.h>
#include <mmsystem.h>
#include "ddt.h"

static  char    szAppName[]="Display Test";
static  HANDLE  hInstApp;
static  int     fhLog;

static UINT (FAR *displayFPS)[3][2];

void LogMsg (LPSTR sz,...);
void ErrMsg (LPSTR sz,...);

DWORD VideoForWindowsVersion()
{
    HINSTANCE h;
    DWORD (FAR PASCAL *Version)(void);
    DWORD ver=0;

    SetErrorMode(SEM_NOOPENFILEERRORBOX);
    h = LoadLibrary("MSVIDEO");
    SetErrorMode(0);

    if (h <= HINSTANCE_ERROR)
        return ver;

    (FARPROC)Version = GetProcAddress(h, "VideoForWindowsVersion");

    if (Version != NULL)
        ver = Version();

    FreeLibrary(h);
    return ver;
}

#define BI_CRAM     mmioFOURCC('C','R','A','M')

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

    hdc = GetDC(NULL);

    //
    // send the Escape to see if they support this DIB
    //
    if (!Escape(hdc, QUERYDIBSUPPORT, (int)lpbi->biSize, (LPVOID)lpbi, (LPVOID)&dw) > 0)
        dw = -1;

    ReleaseDC(NULL, hdc);
    return dw;
}

static void TestDisplay(void)
{
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
            "   8 Bit DIBs     \tStretchDI    \tSetDI+BitBlt\n"
            "       Stretch x1 \t%03d.%01d fps\t%03d.%01d fps %s\n"
            "       Stretch x2 \t%03d.%01d fps\t%03d.%01d fps %s\n"
            "       Stretch xN \t%03d.%01d fps\t%03d.%01d fps %s\n\n"

            "   16 Bit DIBs\n"
            "       Stetch  x1 \t%03d.%01d fps\t%03d.%01d fps %s\n"
            "       Stretch x2 \t%03d.%01d fps\t%03d.%01d fps %s\n"
            "       Stretch xN \t%03d.%01d fps\t%03d.%01d fps %s\n\n"

            "   24 Bit DIBs\n"
            "       Stetch  x1 \t%03d.%01d fps\t%03d.%01d fps %s\n"
            "       Stretch x2 \t%03d.%01d fps\t%03d.%01d fps %s\n"
            "       Stretch xN \t%03d.%01d fps\t%03d.%01d fps %s",
            SUCKS(8),
            SUCKS(16),
            SUCKS(24)
            );
}

void Sleep(UINT uSleep)
{
    MSG msg;
    int id;

    id = SetTimer(NULL, 42, uSleep, NULL);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_TIMER && msg.wParam == (WORD)id)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(NULL, id);
}

#pragma optimize("", off)
void GetNetName(char *pBuffer)
{
    _asm
    {
        mov     bx, pBuffer     ; ds:bx -> pBuffer

        xor     ax,ax           ; zero the buffer
        mov     [bx],ax
        mov     [bx+2],ax
        mov     [bx+4],ax
        mov     [bx+8],ax
        mov     [bx+10],ax
        mov     [bx+12],ax
        mov     [bx+14],ax

        mov     dx, bx          ; ds:dx -> pBuffer
        mov     ax, 5E00h       ; get machine name
        int     21h
        cmc
        sbb     ax,ax
    }
}
#pragma optimize("", on)

void NetName(char *achNetName)
{
    int i;

    GetNetName(achNetName);

    for (i=15; i>=0 && (achNetName[i] == ' ' || achNetName[i] == 0); i--)
        achNetName[i] = 0;

    if (achNetName[0] == 0)
        lstrcpy(achNetName, "(Unknown)");
}

void DisplayName(char *szDisplay)
{
    char achDriver[128];
    char achDriverName[128];
    HDC hdc;

    GetPrivateProfileString("boot",            "display.drv","",achDriver, sizeof(achDriver),"system.ini");
    GetPrivateProfileString("boot.description","display.drv","",achDriverName, sizeof(achDriverName),"system.ini");

    hdc = GetDC(NULL);
    wsprintf(szDisplay, "%dx%dx%d, %s, %s",
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL),
        (LPSTR)achDriver, (LPSTR)achDriverName);
    ReleaseDC(NULL, hdc);
}

BOOL WriteLog()
{
    char     ach[128];
    OFSTRUCT of;
    int i;
    BITMAPINFOHEADER bi;
    DWORD f;
    DWORD VfW;
    WORD  Win;

    // Open log file, keep trying for a while.

    GetModuleFileName(hInstApp, ach, sizeof(ach));
    lstrcpy(ach+lstrlen(ach)-4,".log");

    SetErrorMode(SEM_FAILCRITICALERRORS);
    for (i=0; i<100; i++)
    {
        fhLog = OpenFile(ach, &of, OF_READWRITE|OF_SHARE_DENY_WRITE);

        if (fhLog == -1)    //!!! should we do this?
            fhLog = OpenFile(ach, &of, OF_READWRITE|OF_CREATE|OF_SHARE_DENY_WRITE);

        if (fhLog != -1)
            break;

        Sleep(2000);        // sleep for a while
    }
    SetErrorMode(0);

    if (fhLog == -1)
        return FALSE;

    _llseek(fhLog, 0, SEEK_END);

    LogMsg("Display Test Results 1.0 *******************************************\r\n");

    NetName(ach);
    LogMsg("User:   \t%s\r\n",(LPSTR)ach);

    VfW = VideoForWindowsVersion();
    Win = (WORD)GetVersion();

    LogMsg("Windows:\t%d.%02d %s\r\n",LOBYTE(Win), HIBYTE(Win), (LPSTR)(GetSystemMetrics(SM_DEBUG) ? "(Debug)" : ""));
    LogMsg("VfW:    \t%d.%02d.%02d.%02d\r\n",HIBYTE(HIWORD(VfW)),LOBYTE(HIWORD(VfW)),HIBYTE(VfW),LOBYTE(VfW));

    DisplayName(ach);
    LogMsg("Display:\t%s\r\n",(LPSTR)ach);

#define SDIB(f) \
    (LPSTR)((f != -1) ? ((f & QDI_SETDIBITS   ) ? "Yes" : "No") : "Not Supported" )

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = 100;
    bi.biHeight = 100;
    bi.biPlanes = 1;
    bi.biBitCount = 0;
    bi.biCompression = 0;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    bi.biBitCount = 16;
    bi.biCompression = 0;
    f = QueryDibSupport(&bi);
    LogMsg("RGB555:   \t%s\r\n",SDIB(f));

    bi.biBitCount = 32;
    bi.biCompression = 0;
    f = QueryDibSupport(&bi);
    LogMsg("RGB32:    \t%s\r\n",SDIB(f));

    bi.biBitCount = 16;
    bi.biCompression = BI_CRAM;
    f = QueryDibSupport(&bi);
    LogMsg("Cram16:   \t%s\r\n",SDIB(f));

    bi.biBitCount = 8;
    bi.biCompression = BI_CRAM;
    f = QueryDibSupport(&bi);
    LogMsg("Cram8:    \t%s\r\n",SDIB(f));

    LogMsg("\r\n");

    LogMsg(
            "\t8 Bit DIBs \tStretchDI\tSetDI+BitBlt\r\n"
            "\tStretch x1 \t%03d.%01d   \t%03d.%01d\t%s\r\n"
            "\tStretch x2 \t%03d.%01d   \t%03d.%01d\t%s\r\n"
            "\tStretch xN \t%03d.%01d   \t%03d.%01d\t%s\r\n\r\n"

            "\t16 Bit DIBs\r\n"
            "\tStetch  x1 \t%03d.%01d   \t%03d.%01d\t%s\r\n"
            "\tStretch x2 \t%03d.%01d   \t%03d.%01d\t%s\r\n"
            "\tStretch xN \t%03d.%01d   \t%03d.%01d\t%s\r\n\r\n"

            "\t24 Bit DIBs\r\n"
            "\tStetch  x1 \t%03d.%01d   \t%03d.%01d\t%s\r\n"
            "\tStretch x2 \t%03d.%01d   \t%03d.%01d\t%s\r\n"
            "\tStretch xN \t%03d.%01d   \t%03d.%01d\t%s\r\n\r\n",
            SUCKS(8),
            SUCKS(16),
            SUCKS(24)
            );

    LogMsg("\r\n");
    _lclose(fhLog);

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
int PASCAL WinMain(HANDLE hInst, HANDLE hPrev, LPSTR szCmdLine, WORD sw)
{
    hInstApp = hInst;

    TestDisplay();

    if (hPrev == NULL)
        WriteLog();

    return 0;
}

void ErrMsg (LPSTR sz,...)
{
    static char ach[2000];

    wvsprintf (ach,sz,(LPSTR)(&sz+1));	 /* Format the string */
    MessageBox(NULL,ach,szAppName,
#ifdef BIDI
		MB_RTL_READING |
#endif

MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
}

void LogMsg (LPSTR sz,...)
{
    static char ach[2000];
    int len;

    len = wvsprintf (ach,sz,(LPSTR)(&sz+1));   /* Format the string */

    if (fhLog != -1)
        _lwrite(fhLog, ach, len);
}
