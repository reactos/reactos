#define OEMRESOURCE
#include <windows.h>

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL   RunOnceFill(HWND hWnd);
LRESULT CALLBACK dlgProcRunOnce(
                HWND hWnd,         // window handle
                UINT message,      // type of message
                WPARAM uParam,     // additional information
                LPARAM lParam);     // additional information
void WashCreate(HWND hwndParent);

BOOL   CreateGlobals(HWND hwndCtl);
BOOL CenterWindow (HWND hwndChild, HWND hwndParent);
BOOL TopLeftWindow(HWND hwndChild, HWND hwndParent);

extern SIZE g_SizeTextExt;
extern int g_cxIcon;
extern int g_cyIcon;
extern int g_cxSmIcon;
extern int g_cySmIcon;
extern HFONT g_hfont;
extern HBRUSH g_hbrBkGnd;
extern HFONT g_hBoldFont;
extern HINSTANCE hInst;
extern HWND g_hWash;

#define MAX_TITLE 32
#define MAX_TEXT 128

typedef struct tagTASK
{
     TCHAR Text[MAX_TEXT+1];
     WORD wHeight;
     HICON hIcon;
     TCHAR Cmd[MAX_PATH+1];
} TASK,  * PTASK;

// Bit fields for command line switches.
#define CMD_DO_CHRIS 1
#define CMD_DO_REBOOT 2
#define CMD_DO_RESTART 4




