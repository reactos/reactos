/* Control Panel management
 *
 * Copyright 2001 Eric Pouech
 * Copyright 2008 Owen Rudge
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <windef.h>
#include <winbase.h>
#define NO_SHLWAPI_REG
#include <shlwapi.h>
#include <wine/debug.h>

#include "cpanel.h"

WINE_DEFAULT_DEBUG_CHANNEL(shlctrl);

CPlApplet*    Control_UnloadApplet(CPlApplet* applet)
{
    unsigned    i;
    CPlApplet*    next;

    for (i = 0; i < applet->count; i++)
    {
        if (!applet->info[i].dwSize)
            continue;
        applet->proc(applet->hWnd, CPL_STOP, i, applet->info[i].lData);
    }
    if (applet->proc)
        applet->proc(applet->hWnd, CPL_EXIT, 0L, 0L);
    
    FreeLibrary(applet->hModule);
    next = applet->next;
    HeapFree(GetProcessHeap(), 0, applet);
    return next;
}

CPlApplet*    Control_LoadApplet(HWND hWnd, LPCWSTR cmd, CPanel* panel)
{
    CPlApplet*    applet;
    unsigned     i;
    CPLINFO    info;
    NEWCPLINFOW newinfo;

    if (!(applet = (CPlApplet *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*applet))))
       return applet;

    applet->hWnd = hWnd;

    if (!(applet->hModule = LoadLibraryW(cmd)))
    {
        WARN("Cannot load control panel applet %s\n", debugstr_w(cmd));
        goto theError;
    }
    if (!(applet->proc = (APPLET_PROC)GetProcAddress(applet->hModule, "CPlApplet")))
    {
        WARN("Not a valid control panel applet %s\n", debugstr_w(cmd));
        goto theError;
    }
    if (!applet->proc(hWnd, CPL_INIT, 0L, 0L))
    {
        WARN("Init of applet has failed\n");
        goto theError;
    }
    if ((applet->count = applet->proc(hWnd, CPL_GETCOUNT, 0L, 0L)) == 0)
    {
        WARN("No subprogram in applet\n");
        goto theError;
    }

    applet = (CPlApplet *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, applet,
             sizeof(*applet) + (applet->count - 1) * sizeof(NEWCPLINFOW));

    for (i = 0; i < applet->count; i++)
    {
        ZeroMemory(&newinfo, sizeof(newinfo));
        newinfo.dwSize = sizeof(NEWCPLINFOW);
        applet->info[i].dwSize = sizeof(NEWCPLINFOW);
       /* proc is supposed to return a null value upon success for
    * CPL_INQUIRE and CPL_NEWINQUIRE
    * However, real drivers don't seem to behave like this
    * So, use introspection rather than return value
    */
        applet->proc(hWnd, CPL_NEWINQUIRE, i, (LPARAM)&newinfo);
        if (newinfo.hIcon == 0)
        {
            applet->proc(hWnd, CPL_INQUIRE, i, (LPARAM)&info);
            if (info.idIcon == 0 || info.idName == 0)
            {
                WARN("Couldn't get info from sp %u\n", i);
                applet->info[i].dwSize = 0;
            }
            else
            {
                /* convert the old data into the new structure */
                applet->info[i].dwFlags = 0;
                applet->info[i].dwHelpContext = 0;
                applet->info[i].lData = info.lData;
                applet->info[i].hIcon = LoadIconW(applet->hModule,
                                MAKEINTRESOURCEW(info.idIcon));
                LoadStringW(applet->hModule, info.idName,
                    applet->info[i].szName, sizeof(applet->info[i].szName) / sizeof(WCHAR));
                LoadStringW(applet->hModule, info.idInfo,
                    applet->info[i].szInfo, sizeof(applet->info[i].szInfo) / sizeof(WCHAR));
                applet->info[i].szHelpFile[0] = '\0';
            }
        }
        else
        {
            CopyMemory(&applet->info[i], &newinfo, newinfo.dwSize);
            if (newinfo.dwSize != sizeof(NEWCPLINFOW))
            {
                applet->info[i].dwSize = sizeof(NEWCPLINFOW);
                lstrcpyW(applet->info[i].szName, newinfo.szName);
                lstrcpyW(applet->info[i].szInfo, newinfo.szInfo);
                lstrcpyW(applet->info[i].szHelpFile, newinfo.szHelpFile);
            }
        }
    }

    applet->next = panel->first;
    panel->first = applet;

    return applet;

 theError:
    Control_UnloadApplet(applet);
    return NULL;
}

static void Control_WndProc_Create(HWND hWnd, const CREATESTRUCTW* cs)
{
   CPanel*    panel = (CPanel*)cs->lpCreateParams;

   SetWindowLongPtrW(hWnd, 0, (LONG_PTR)panel);
   panel->status = 0;
   panel->hWnd = hWnd;
}

#define    XICON    32
#define XSTEP    128
#define    YICON    32
#define YSTEP    64

static BOOL    Control_Localize(const CPanel* panel, int cx, int cy,
                 CPlApplet** papplet, unsigned* psp)
{
    unsigned int    i;
    int                x = (XSTEP-XICON)/2, y = 0;
    CPlApplet*    applet;
    RECT    rc;

    GetClientRect(panel->hWnd, &rc);
    for (applet = panel->first; applet; applet = applet->next)
    {
        for (i = 0; i < applet->count; i++)
        {
            if (!applet->info[i].dwSize)
                continue;
            if (x + XSTEP >= rc.right - rc.left)
            {
                x = (XSTEP-XICON)/2;
                y += YSTEP;
            }
            if (cx >= x && cx < x + XICON && cy >= y && cy < y + YSTEP)
            {
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
    HDC        hdc;
    PAINTSTRUCT    ps;
    RECT    rc, txtRect;
    unsigned int    i;
    int                x = 0, y = 0;
    CPlApplet*    applet;
    HGDIOBJ    hOldFont;

    hdc = (wParam) ? (HDC)wParam : BeginPaint(panel->hWnd, &ps);
    hOldFont = SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
    GetClientRect(panel->hWnd, &rc);
    
    for (applet = panel->first; applet; applet = applet->next)
    {
        for (i = 0; i < applet->count; i++)
        {
            if (x + XSTEP >= rc.right - rc.left)
            {
                x = 0;
                y += YSTEP;
            }
            if (!applet->info[i].dwSize)
                continue;
            DrawIcon(hdc, x + (XSTEP-XICON)/2, y, applet->info[i].hIcon);
            txtRect.left = x;
            txtRect.right = x + XSTEP;
            txtRect.top = y + YICON;
            txtRect.bottom = y + YSTEP;
            DrawTextW(hdc, applet->info[i].szName, -1, &txtRect,
                  DT_CENTER | DT_VCENTER);
            x += XSTEP;
        }
    }

    SelectObject(hdc, hOldFont);
    if (!wParam)
        EndPaint(panel->hWnd, &ps);

    return 0;
}

static LRESULT Control_WndProc_LButton(CPanel* panel, LPARAM lParam, BOOL up)
{
    unsigned    i;
    CPlApplet*    applet;

    if (Control_Localize(panel, (short)LOWORD(lParam), (short)HIWORD(lParam), &applet, &i))
    {
        if (up)
        {
            if (panel->clkApplet == applet && panel->clkSP == i)
            {
                applet->proc(applet->hWnd, CPL_DBLCLK, i, applet->info[i].lData);
            }
        }
        else
        {
            panel->clkApplet = applet;
            panel->clkSP = i;
        }
    }
    return 0;
}

static LRESULT WINAPI Control_WndProc(HWND hWnd, UINT wMsg,
                    WPARAM lParam1, LPARAM lParam2)
{
    CPanel*    panel = (CPanel*)GetWindowLongPtrW(hWnd, 0);

    if (panel || wMsg == WM_CREATE)
    {
        switch (wMsg)
        {
            case WM_CREATE:
                Control_WndProc_Create(hWnd, (CREATESTRUCTW*)lParam2);
                return 0;
            case WM_DESTROY:
            {
                CPlApplet *applet = panel->first;
                while (applet)
                    applet = Control_UnloadApplet(applet);
                
                PostQuitMessage(0);
                break;
            }
            case WM_PAINT:
                return Control_WndProc_Paint(panel, lParam1);
            case WM_LBUTTONUP:
                return Control_WndProc_LButton(panel, lParam2, TRUE);
            case WM_LBUTTONDOWN:
                return Control_WndProc_LButton(panel, lParam2, FALSE);
            /* EPP       case WM_COMMAND: */
            /* EPP      return Control_WndProc_Command(mwi, lParam1, lParam2); */
        }
    }

   return DefWindowProcW(hWnd, wMsg, lParam1, lParam2);
}

static void Control_DoInterface(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    WNDCLASSW    wc;
    MSG    msg;
    const WCHAR* appName = L"ReactOS Control Panel";
    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = Control_WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(CPlApplet*);
    wc.hInstance = hInst;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"Shell_Control_WndClass";

    if (!RegisterClassW(&wc)) return;

    CreateWindowExW(0, wc.lpszClassName, appName,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hWnd, NULL, hInst, panel);
    
    if (!panel->hWnd)
        return;

    if (!panel->first)
    {
        /* FIXME appName & message should be localized  */
        MessageBoxW(panel->hWnd, L"Cannot load any applets", appName, MB_OK);
        return;
    }

    while (GetMessageW(&msg, panel->hWnd, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static void Control_DoWindow(CPanel *panel, HWND hWnd, HINSTANCE hInst)
{
    HANDLE hFind;
    WIN32_FIND_DATAW wfd;
    WCHAR wszPath[MAX_PATH];
    WCHAR *Ptr = wszPath;

    Ptr += GetSystemDirectoryW(wszPath, MAX_PATH);
    *Ptr++ = '\\';
    wcscpy(Ptr, L"*.cpl");

    hFind = FindFirstFileW(wszPath, &wfd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            wcscpy(Ptr, wfd.cFileName);
            Control_LoadApplet(hWnd, wszPath, panel);
        } while (FindNextFileW(hFind, &wfd));
        FindClose(hFind);
    }

    Control_DoInterface(panel, hWnd, hInst);
}

static void Control_DoLaunch(CPanel *pPanel, HWND hWnd, LPCWSTR pwszCmd)
{
    HANDLE hMutex;

    /* Make a pwszCmd copy so we can modify it */
    LPWSTR pwszCmdCopy = _wcsdup(pwszCmd);

    LPWSTR pwszPath = pwszCmdCopy, pwszArg = NULL, pwszArg2 = NULL;
    if (!pwszCmdCopy)
        return;

    /* Path can be quoted */
    if (pwszPath[0] == L'"')
    {
        ++pwszPath;
        pwszArg = wcschr(pwszPath, L'"');
        if (pwszArg)
            *(pwszArg++) = '\0';
    }
    else
        pwszArg = pwszCmdCopy;

    /* First argument starts after space or ','. Note: we ignore characters between '"' and ',' or ' '. */
    if (pwszArg)
        pwszArg = wcspbrk(pwszArg, L" ,");
    if (pwszArg)
    {
        /* NULL terminate path and find first character of arg */
        *(pwszArg++) = L'\0';
        if (pwszArg[0] == L'"')
        {
            ++pwszArg;
            pwszArg2 = wcschr(pwszArg, L'"');
            if (pwszArg2)
                *(pwszArg2++) = L'\0';
        } else
            pwszArg2 = pwszArg;

        /* Second argument always starts with ','. Note: we ignore characters between '"' and ','. */
        if (pwszArg2)
            pwszArg2 = wcschr(pwszArg2, L',');
    }

    TRACE("Launch %ls, arg %ls, arg2 %ls\n", pwszPath, pwszArg, pwszArg2);

    /* Create a mutex to disallow running multiple instances */
    hMutex = CreateMutexW(NULL, TRUE, PathFindFileNameW(pwszPath));
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        TRACE("Next instance disallowed\n");
        if (hMutex)
            CloseHandle(hMutex);
        return;
    }

    /* Load applet cpl */
    TRACE("Load applet %ls\n", pwszPath);
    Control_LoadApplet(hWnd, pwszPath, pPanel);
    if (pPanel->first)
    {
        INT i = 0;
        /* First pPanel applet is the new one */
        CPlApplet *pApplet = pPanel->first;
        assert(pApplet && pApplet->next == NULL);
        TRACE("pApplet->count %d\n", pApplet->count);

        /* Note: if there is only one applet, first argument is ignored */
        if (pApplet->count > 1 && pwszArg && pwszArg[0])
        {
            /* If arg begins with '@', number specifies applet index */
            if (pwszArg[0] == L'@')
                i = _wtoi(pwszArg + 1);
            else
            {
                /* Otherwise it's applet name */
                for (i = 0; i < (INT)pApplet->count; ++i)
                    if (!wcscmp(pwszArg, pApplet->info[i].szName))
                        break;
            }
        }

        if (i >= 0 && i < (INT)pApplet->count && pApplet->info[i].dwSize)
        {
            /* Start the applet */
            TRACE("Starting applet %d\n", i);
            if (!pApplet->proc(pApplet->hWnd, CPL_STARTWPARMSW, i, (LPARAM)pwszArg))
                pApplet->proc(pApplet->hWnd, CPL_DBLCLK, i, pApplet->info[i].lData);
        } else
            ERR("Applet not found: %ls\n", pwszArg ? pwszArg : L"NULL");

        Control_UnloadApplet(pApplet);
    }
    else
        ERR("Failed to load applet %ls\n", pwszPath);

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    free(pwszCmdCopy);
}

/*************************************************************************
 * Control_RunDLLW            [SHELL32.@]
 *
 */
EXTERN_C void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow)
{
    CPanel Panel;

    TRACE("(%p, %p, %s, 0x%08x)\n",
	  hWnd, hInst, debugstr_w(cmd), nCmdShow);

    memset(&Panel, 0, sizeof(Panel));

    if (!cmd || !*cmd)
    {
        TRACE("[shell32, Control_RunDLLW] Calling Control_DoWindow\n");
        Control_DoWindow(&Panel, hWnd, hInst);
    }
    else
    {
        TRACE("[shell32, Control_RunDLLW] Calling Control_DoLaunch\n");
        Control_DoLaunch(&Panel, hWnd, cmd);
    }
}

/*************************************************************************
 * Control_RunDLLA			[SHELL32.@]
 *
 */
void WINAPI Control_RunDLLA(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, cmd, -1, NULL, 0 );
    LPWSTR wszCmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (wszCmd && MultiByteToWideChar(CP_ACP, 0, cmd, -1, wszCmd, len ))
    {
        Control_RunDLLW(hWnd, hInst, wszCmd, nCmdShow);
    }
    HeapFree(GetProcessHeap(), 0, wszCmd);
}

/*************************************************************************
 * Control_FillCache_RunDLLW			[SHELL32.@]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLLW(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    FIXME("%p %p 0x%08x 0x%08x stub\n", hWnd, hModule, w, x);
    return S_OK;
}

/*************************************************************************
 * Control_FillCache_RunDLLA			[SHELL32.@]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLLA(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    return Control_FillCache_RunDLLW(hWnd, hModule, w, x);
}


/*************************************************************************
 * RunDLL_CallEntry16                [SHELL32.122]
 * the name is probably wrong
 */
EXTERN_C void WINAPI RunDLL_CallEntry16( DWORD proc, HWND hwnd, HINSTANCE inst,
                                LPCSTR cmdline, INT cmdshow )
{
#if !defined(__CYGWIN__) && !defined (__MINGW32__) && !defined(_MSC_VER)
    WORD args[5];
    SEGPTR cmdline_seg;

    TRACE( "proc %x hwnd %p inst %p cmdline %s cmdshow %d\n",
           proc, hwnd, inst, debugstr_a(cmdline), cmdshow );

    cmdline_seg = MapLS( cmdline );
    args[4] = HWND_16(hwnd);
    args[3] = MapHModuleLS(inst);
    args[2] = SELECTOROF(cmdline_seg);
    args[1] = OFFSETOF(cmdline_seg);
    args[0] = cmdshow;
    WOWCallback16Ex( proc, WCB16_PASCAL, sizeof(args), args, NULL );
    UnMapLS( cmdline_seg );
#else
    FIXME( "proc %lx hwnd %p inst %p cmdline %s cmdshow %d\n",
           proc, hwnd, inst, debugstr_a(cmdline), cmdshow );
#endif
}

/*************************************************************************
 * CallCPLEntry16                [SHELL32.166]
 *
 * called by desk.cpl on "Advanced" with:
 * hMod("DeskCp16.Dll"), pFunc("CplApplet"), 0, 1, 0xc, 0
 *
 */
LRESULT WINAPI CallCPLEntry16(HINSTANCE hMod, FARPROC pFunc, HWND dw3, UINT dw4, LPARAM dw5, LPARAM dw6)
{
    FIXME("(%p, %p, %08x, %08x, %08x, %08x): stub.\n", hMod, pFunc, dw3, dw4, dw5, dw6);
    return 0x0deadbee;
}
