/*
 *  ReactOS shell32 - Control Panel
 *
 *  control.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <memory.h>
//#include <process.h>
    
#include "control.h"
#include "framewnd.h"
#include "settings.h"

#include "shell32.h"
#include "trace.h"

//#define _USE_WINE_WND_

//WINE_DEFAULT_DEBUG_CHANNEL(shlctrl);

////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

HINSTANCE hInst;
HWND hFrameWnd;
HWND hStatusBar;
HMENU hMenuFrame;

TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];


////////////////////////////////////////////////////////////////////////////////

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT rect;
    WNDCLASSEX wcFrame = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW/*style*/,
        FrameWndProc,
        0/*cbClsExtra*/,
        0/*cbWndExtra*/,
        hInstance,
        LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_CONTROL)),
        LoadCursor(0, IDC_ARROW),
        0/*hbrBackground*/,
        0/*lpszMenuName*/,
        szWindowClass,
        (HICON)LoadImage(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_CONTROL), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
    };
    ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class

	hMenuFrame = LoadMenu(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDR_CONTROL_MENU));

    // Initialize the Windows Common Controls DLL
    InitCommonControls();

    if (LoadSettings(&rect)) {
        hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
                    WS_OVERLAPPEDWINDOW | WS_EX_CLIENTEDGE,
                    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                    NULL, hMenuFrame, hInstance, NULL/*lpParam*/);
    } else {
        hFrameWnd = CreateWindowEx(0, (LPCTSTR)(int)hFrameWndClass, szTitle,
                    WS_OVERLAPPEDWINDOW | WS_EX_CLIENTEDGE,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, hMenuFrame, hInstance, NULL/*lpParam*/);
    }


    if (!hFrameWnd) {
        return FALSE;
    }

    // Create the status bar
    hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, 
                                    _T(""), hFrameWnd, STATUS_WINDOW);
    if (hStatusBar) {
        // Create the status bar panes
        SetupStatusBar(hFrameWnd, FALSE);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);
    }
    ShowWindow(hFrameWnd, nCmdShow);
    UpdateWindow(hFrameWnd);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

void ExitInstance(void)
{
    DestroyMenu(hMenuFrame);
}


int APIENTRY ControlMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPCTSTR   pCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_CONTROL, szWindowClass, MAX_LOADSTRING);
    
    // Store instance handle in our global variable
    hInst = hInstance;

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_CONTROL);

    // Main message loop:
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    ExitInstance();
    return msg.wParam;
}

////////////////////////////////////////////////////////////////////////////////

CPlApplet* Control_UnloadApplet(CPlApplet* applet)
{
    unsigned    i;
    CPlApplet*  next;

    for (i = 0; i < applet->count; i++) {
        if (!applet->info[i].dwSize) continue;
        applet->proc(applet->hWnd, CPL_STOP, i, applet->info[i].lData);
    }
    if (applet->proc) applet->proc(applet->hWnd, CPL_EXIT, 0L, 0L);
    FreeLibrary(applet->hModule);
    next = applet->next;
    HeapFree(GetProcessHeap(), 0, applet);
    return next;
}

CPlApplet* Control_LoadApplet(HWND hWnd, LPCTSTR cmd, CPlApplet** pListHead)
{
    CPlApplet*  applet;
    unsigned    i;
    CPLINFO     info;

    if (!(applet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*applet))))
       return applet;
    applet->hWnd = hWnd;
    if (!(applet->hModule = LoadLibrary(cmd))) {
        TRACE(_T("Cannot load control panel applet %s\n"), cmd);
        goto theError;
    }
    if (!(applet->proc = (APPLET_PROC)GetProcAddress(applet->hModule, "CPlApplet"))) {
//    if (!(applet->proc = (APPLET_PROC)GetProcAddress(applet->hModule, "_CPlApplet@16"))) {
        TRACE(_T("Not a valid control panel applet %s\n"), cmd);
        goto theError;
    }
    if (!applet->proc(hWnd, CPL_INIT, 0L, 0L)) {
        TRACE(_T("Init of applet has failed\n"));
        goto theError;
    }
    if ((applet->count = applet->proc(hWnd, CPL_GETCOUNT, 0L, 0L)) == 0) {
        TRACE(_T("No subprogram in applet\n"));
        goto theError;
    }
    applet = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, applet,
//                         sizeof(*applet) + (applet->count - 1) * sizeof(NEWCPLINFOA));
                         sizeof(*applet) + (applet->count - 0) * sizeof(NEWCPLINFO));
    for (i = 0; i < applet->count; i++) {
//       applet->info[i].dwSize = sizeof(NEWCPLINFOA);
       applet->info[i].dwSize = sizeof(NEWCPLINFO);
       /* proc is supposed to return a null value upon success for
        * CPL_INQUIRE and CPL_NEWINQUIRE
        * However, real drivers don't seem to behave like this
        * So, use introspection rather than return value
        */
       applet->info[i].hIcon = 0;
       applet->proc(hWnd, CPL_NEWINQUIRE, i, (LPARAM)&applet->info[i]);
       if (applet->info[i].hIcon == 0) {
           applet->proc(hWnd, CPL_INQUIRE, i, (LPARAM)&info);
           if (info.idIcon == 0 || info.idName == 0) {
               TRACE(_T("Couldn't get info from sp %u\n"), i);
               applet->info[i].dwSize = 0;
           } else {
               /* convert the old data into the new structure */
               applet->info[i].dwFlags = 0;
               applet->info[i].dwHelpContext = 0;
               applet->info[i].lData = info.lData;
//               applet->info[i].hIcon = LoadIcon(applet->hModule, (LPCTSTR)MAKEINTRESOURCEA(info.idIcon));
//               applet->info[i].hIcon = LoadIcon(applet->hModule, (LPCTSTR)MAKEINTRESOURCE(info.idIcon));
               applet->info[i].hIcon = LoadImage(applet->hModule, (LPCTSTR)info.idIcon, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

               LoadString(applet->hModule, info.idName, applet->info[i].szName, sizeof(applet->info[i].szName)/sizeof(TCHAR));
               //LoadString(applet->hModule, info.idInfo, applet->info[i].szInfo, sizeof(applet->info[i].szInfo)/sizeof(TCHAR));
               //applet->info[i].szHelpFile[0] = '\0';
               LoadString(applet->hModule, info.idInfo, applet->info[i].szInfo, 192);
           }
       } else {
           TRACE(_T("Using CPL_NEWINQUIRE data\n"));
       }
    }
    applet->next = *pListHead;
    *pListHead = applet;
    return applet;
theError:
    Control_UnloadApplet(applet);
    return NULL;
}

void Control_DoLaunch(CPlApplet** pListHead, HWND hWnd, LPCTSTR cmd)
   /* forms to parse:
    *   foo.cpl,@sp,str
    *   foo.cpl,@sp
    *   foo.cpl,,str
    *   foo.cpl @sp
    *   foo.cpl str
    */
{
    TCHAR*      buffer;
    TCHAR*      beg = NULL;
    TCHAR*      end;
    TCHAR       ch;
    unsigned    sp = 0;
    TCHAR*      extraPmts = NULL;

    buffer = HeapAlloc(GetProcessHeap(), 0, _tcslen(cmd) + 1);
    if (!buffer) return;

    end = _tcscpy(buffer, cmd);

    for (;;) {
        ch = *end;
        if (ch == _T(' ') || ch == _T(',') || ch == _T('\0')) {
            *end = _T('\0');
            if (beg) {
                if (*beg == _T('@')) {
                    sp = _ttoi(beg + 1);
                } else if (*beg == _T('\0')) {
                    sp = 0;
                } else {
                    extraPmts = beg;
                }
            }
            if (ch == _T('\0')) break;
            beg = end + 1;
            if (ch == _T(' ')) while (end[1] == _T(' ')) end++;
        }
        end++;
    }
#if 1
    Control_LoadApplet(hWnd, buffer, pListHead);
    if (*pListHead) {
       CPlApplet* applet = *pListHead;
       assert(applet && applet->next == NULL);
       if (sp >= applet->count) {
          TRACE(_T("Out of bounds (%u >= %u), setting to 0\n"), sp, applet->count);
          sp = 0;
       }
       if (applet->info[sp].dwSize) {
          if (!applet->proc(applet->hWnd, CPL_STARTWPARMS, sp, (LPARAM)extraPmts))
             applet->proc(applet->hWnd, CPL_DBLCLK, sp, applet->info[sp].lData);
       }
       Control_UnloadApplet(applet);
    }
#else

//static void Control_LaunchApplet(HWND hWnd, CPlEntry* pCPlEntry)

#endif
    HeapFree(GetProcessHeap(), 0, buffer);
}

//int APIENTRY ControlMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPCTSTR lpCmdLine, int nCmdShow);

VOID Control_RunDLL(HWND hWnd, HINSTANCE hInst_unused, LPCTSTR lpCmdLine, DWORD nCmdShow)
{
    CPanel      panel;
//    TRACE("(0x%08x, 0x%08lx, %s, 0x%08lx)\n", hWnd, (DWORD)hInst, debugstr_a(lpCmdLine), nCmdShow);

    memset(&panel, 0, sizeof(panel));

    if (!lpCmdLine || !*lpCmdLine) {
#ifdef _USE_WINE_WND_
        Control_DoWindow(&panel, hWnd, hInst);
#else
        ControlMain(hInst, NULL, lpCmdLine, nCmdShow);
#endif
    } else {
        Control_DoLaunch(&panel.first, hWnd, lpCmdLine);
    }
}


////////////////////////////////////////////////////////////////////////////////
#ifdef _USE_WINE_WND_

static void      Control_WndProc_Create(HWND hWnd, const CREATESTRUCTA* cs)
{
   CPanel*      panel = (CPanel*)cs->lpCreateParams;

   SetWindowLong(hWnd, 0, (LPARAM)panel);
   panel->status = 0;
   panel->hWnd = hWnd;
}

#define XICON   32
#define XSTEP   128
#define YICON   32
#define YSTEP   64

static BOOL Control_Localize(const CPanel* panel, unsigned cx, unsigned cy,
                             CPlApplet** papplet, unsigned* psp)
{
    unsigned    i, x = (XSTEP-XICON)/2, y = 0;
    CPlApplet*  applet;
    RECT        rc;

    GetClientRect(panel->hWnd, &rc);
    for (applet = panel->first; applet; applet = applet = applet->next) {
        for (i = 0; i < applet->count; i++) {
            if (!applet->info[i].dwSize) continue;
            if (x + XSTEP >= rc.right - rc.left) {
                x = (XSTEP-XICON)/2;
                y += YSTEP;
            }
            if (cx >= x && cx < x + XICON && cy >= y && cy < y + YSTEP) {
                *papplet = applet;
                *psp = i;
                return TRUE;
            }
            x += XSTEP;
        }
    }
    return FALSE;
}

static LRESULT Control_WndProc_Paint(const CPanel* panel, WPARAM wParam)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc, txtRect;
    unsigned    i, x = 0, y = 0;
    CPlApplet*  applet;
    HGDIOBJ     hOldFont;

    hdc = (wParam) ? (HDC)wParam : BeginPaint(panel->hWnd, &ps);
    hOldFont = SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
    GetClientRect(panel->hWnd, &rc);
    for (applet = panel->first; applet; applet = applet = applet->next) {
        for (i = 0; i < applet->count; i++) {
            if (x + XSTEP >= rc.right - rc.left) {
                x = 0;
                y += YSTEP;
            }
            if (!applet->info[i].dwSize) continue;
            DrawIcon(hdc, x + (XSTEP-XICON)/2, y, applet->info[i].hIcon);
            txtRect.left = x;
            txtRect.right = x + XSTEP;
            txtRect.top = y + YICON;
            txtRect.bottom = y + YSTEP;
            DrawText(hdc, applet->info[i].szName, -1, &txtRect,
                      DT_CENTER | DT_VCENTER);
            x += XSTEP;
        }
    }
    SelectObject(hdc, hOldFont);
    if (!wParam) EndPaint(panel->hWnd, &ps);
    return 0;
}

static LRESULT Control_WndProc_LButton(CPanel* panel, LPARAM lParam, BOOL up)
{
    unsigned    i;
    CPlApplet*  applet;

    if (Control_Localize(panel, LOWORD(lParam), HIWORD(lParam), &applet, &i)) {
       if (up) {
           if (panel->clkApplet == applet && panel->clkSP == i) {
               applet->proc(applet->hWnd, CPL_DBLCLK, i, applet->info[i].lData);
           }
       } else {
           panel->clkApplet = applet;
           panel->clkSP = i;
       }
    }
    return 0;
}

//static LRESULT WINAPI Control_WndProc(HWND hWnd, UINT wMsg,
static LRESULT __stdcall Control_WndProc(HWND hWnd, UINT wMsg,
                                        WPARAM lParam1, LPARAM lParam2)
{
   CPanel* panel = (CPanel*)GetWindowLongA(hWnd, 0);

   if (panel || wMsg == WM_CREATE) {
      switch (wMsg) {
      case WM_CREATE:
         Control_WndProc_Create(hWnd, (CREATESTRUCTA*)lParam2);
         return 0;
      case WM_DESTROY:
         while ((panel->first = Control_UnloadApplet(panel->first)));
         break;
      case WM_PAINT:
         return Control_WndProc_Paint(panel, lParam1);
      case WM_LBUTTONUP:
         return Control_WndProc_LButton(panel, lParam2, TRUE);
      case WM_LBUTTONDOWN:
         return Control_WndProc_LButton(panel, lParam2, FALSE);
/* EPP       case WM_COMMAND: */
/* EPP   return Control_WndProc_Command(mwi, lParam1, lParam2); */
      }
   }

   return DefWindowProcA(hWnd, wMsg, lParam1, lParam2);
}

static void Control_DoInterface(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    WNDCLASS wc;
    MSG         msg;

    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = Control_WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(CPlApplet*);
    wc.hInstance = hInst;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "Shell_Control_WndClass";

    if (!RegisterClass(&wc)) return;

    CreateWindowEx(0, wc.lpszClassName, "Wine Control Panel",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    hWnd, (HMENU)0, hInst, panel);
    if (!panel->hWnd) return;
    while (GetMessage(&msg, panel->hWnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!panel->first) break;
    }
}

static void Control_DoWindow(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    HANDLE              h;
    WIN32_FIND_DATA     fd;
    TCHAR               buffer[MAX_PATH];

    /* TRACE: should grab path somewhere from configuration */
    if ((h = FindFirstFile("c:\\winnt\\system32\\*.cpl", &fd)) != 0) {
        do {
           sprintf(buffer, "c:\\winnt\\system32\\%s", fd.cFileName);
       if (!strstr(fd.cFileName, "powercfg")) {
           Control_LoadApplet(hWnd, buffer, panel);
       }
        } while (FindNextFile(h, &fd));
        FindClose(h);
    }

    if (panel->first) Control_DoInterface(panel, hWnd, hInst);
}

#endif // _USE_WINE_WND_

#if 0
/*************************************************************************
 * Control_RunDLL                       [SHELL32.@]
 *
 */

VOID WINAPI
Control_RunDLL(HWND hWnd, HINSTANCE hInst_unused, LPCSTR lpCmdLine, DWORD nCmdShow)
{
//    TRACE("(0x%08x, 0x%08lx, %s, 0x%08lx)\n", hWnd, (DWORD)hInst, debugstr_a(lpCmdLine), nCmdShow);
}

/*************************************************************************
 * Control_FillCache_RunDLL                     [SHELL32.@]
 *
 */
HRESULT WINAPI
Control_FillCache_RunDLL(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    TRACE(_T("0x%04x 0x%04x 0x%04lx 0x%04lx stub\n"), hWnd, hModule, w, x);
    return 0;
}

/*************************************************************************
 * RunDLL_CallEntry16                           [SHELL32.122]
 * the name is probably wrong
 */
HRESULT WINAPI
RunDLL_CallEntry16(DWORD v, DWORD w, DWORD x, DWORD y, DWORD z)
{
    TRACE(_T("0x%04lx 0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n"),v,w,x,y,z);
    return 0;
}

/*************************************************************************
 * CallCPLEntry16                               [SHELL32.166]
 *
 * called by desk.cpl on "Advanced" with:
 * hMod("DeskCp16.Dll"), pFunc("CplApplet"), 0, 1, 0xc, 0
 *
 */
DWORD WINAPI
CallCPLEntry16(HMODULE hMod, FARPROC pFunc, DWORD dw3, DWORD dw4, DWORD dw5, DWORD dw6)
{
    TRACE(_T("(%04x, %p, %08lx, %08lx, %08lx, %08lx): stub.\n"), hMod, pFunc, dw3, dw4, dw5, dw6);
    return 0x0deadbee;
}
#endif
