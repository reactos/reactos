/* Control Panel management
 *
 * Copyright 2001 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "wine/debug.h"
#include "cpl.h"
#include "wine/unicode.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "cpanel.h"

WINE_DEFAULT_DEBUG_CHANNEL(shlctrl);

CPlApplet*	Control_UnloadApplet(CPlApplet* applet)
{
    unsigned	i;
    CPlApplet*	next;

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

CPlApplet*	Control_LoadApplet(HWND hWnd, LPCWSTR cmd, CPanel* panel)
{
    CPlApplet*	applet;
    unsigned 	i;
    CPLINFO	info;
    NEWCPLINFOW newinfo;

    if (!(applet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*applet))))
       return applet;

    applet->hWnd = hWnd;

    if (!(applet->hModule = LoadLibraryW(cmd))) {
        WARN("Cannot load control panel applet %s\n", debugstr_w(cmd));
	goto theError;
    }
    if (!(applet->proc = (APPLET_PROC)GetProcAddress(applet->hModule, "CPlApplet"))) {
        WARN("Not a valid control panel applet %s\n", debugstr_w(cmd));
	goto theError;
    }
    if (!applet->proc(hWnd, CPL_INIT, 0L, 0L)) {
        WARN("Init of applet has failed\n");
	goto theError;
    }
    if ((applet->count = applet->proc(hWnd, CPL_GETCOUNT, 0L, 0L)) == 0) {
        WARN("No subprogram in applet\n");
	goto theError;
    }

    applet = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, applet,
			 sizeof(*applet) + (applet->count - 1) * sizeof(NEWCPLINFOW));

    for (i = 0; i < applet->count; i++) {
       ZeroMemory(&newinfo, sizeof(newinfo));
       newinfo.dwSize = sizeof(NEWCPLINFOA);
       applet->info[i].dwSize = sizeof(NEWCPLINFOW);
       /* proc is supposed to return a null value upon success for
	* CPL_INQUIRE and CPL_NEWINQUIRE
	* However, real drivers don't seem to behave like this
	* So, use introspection rather than return value
	*/
       applet->proc(hWnd, CPL_NEWINQUIRE, i, (LPARAM)&newinfo);
       if (newinfo.hIcon == 0) {
	   applet->proc(hWnd, CPL_INQUIRE, i, (LPARAM)&info);
	   if (info.idIcon == 0 || info.idName == 0) {
	       WARN("Couldn't get info from sp %u\n", i);
	       applet->info[i].dwSize = 0;
	   } else {
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
               MultiByteToWideChar(CP_ACP, 0, ((LPNEWCPLINFOA)&newinfo)->szName,
	                           sizeof(((LPNEWCPLINFOA)&newinfo)->szName) / sizeof(CHAR),
			           applet->info[i].szName,
			           sizeof(applet->info[i].szName) / sizeof(WCHAR));
               MultiByteToWideChar(CP_ACP, 0, ((LPNEWCPLINFOA)&newinfo)->szInfo,
	                           sizeof(((LPNEWCPLINFOA)&newinfo)->szInfo) / sizeof(CHAR),
			           applet->info[i].szInfo,
			           sizeof(applet->info[i].szInfo) / sizeof(WCHAR));
               MultiByteToWideChar(CP_ACP, 0, ((LPNEWCPLINFOA)&newinfo)->szHelpFile,
	                           sizeof(((LPNEWCPLINFOA)&newinfo)->szHelpFile) / sizeof(CHAR),
			           applet->info[i].szHelpFile,
			           sizeof(applet->info[i].szHelpFile) / sizeof(WCHAR));
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

static void 	 Control_WndProc_Create(HWND hWnd, const CREATESTRUCTA* cs)
{
   CPanel*	panel = (CPanel*)cs->lpCreateParams;

   SetWindowLongA(hWnd, 0, (LPARAM)panel);
   panel->status = 0;
   panel->hWnd = hWnd;
}

#define	XICON	32
#define XSTEP	128
#define	YICON	32
#define YSTEP	64

static BOOL	Control_Localize(const CPanel* panel, unsigned cx, unsigned cy,
				 CPlApplet** papplet, unsigned* psp)
{
    unsigned	i, x = (XSTEP-XICON)/2, y = 0;
    CPlApplet*	applet;
    RECT	rc;

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
    HDC		hdc;
    PAINTSTRUCT	ps;
    RECT	rc, txtRect;
    unsigned	i, x = 0, y = 0;
    CPlApplet*	applet;
    HGDIOBJ	hOldFont;

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
	    DrawTextW(hdc, applet->info[i].szName, -1, &txtRect,
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
    unsigned	i;
    CPlApplet*	applet;

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

static LRESULT WINAPI	Control_WndProc(HWND hWnd, UINT wMsg,
					WPARAM lParam1, LPARAM lParam2)
{
   CPanel*	panel = (CPanel*)GetWindowLongA(hWnd, 0);

   if (panel || wMsg == WM_CREATE) {
      switch (wMsg) {
      case WM_CREATE:
	 Control_WndProc_Create(hWnd, (CREATESTRUCTA*)lParam2);
	 return 0;
      case WM_DESTROY:
         {
	    CPlApplet*	applet = panel->first;
	    while (applet)
	       applet = Control_UnloadApplet(applet);
         }
         PostQuitMessage(0);
	 break;
      case WM_PAINT:
	 return Control_WndProc_Paint(panel, lParam1);
      case WM_LBUTTONUP:
	 return Control_WndProc_LButton(panel, lParam2, TRUE);
      case WM_LBUTTONDOWN:
	 return Control_WndProc_LButton(panel, lParam2, FALSE);
/* EPP       case WM_COMMAND: */
/* EPP 	 return Control_WndProc_Command(mwi, lParam1, lParam2); */
      }
   }

   return DefWindowProcA(hWnd, wMsg, lParam1, lParam2);
}

static void    Control_DoInterface(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    WNDCLASSA	wc;
    MSG		msg;
    const CHAR* appName = "Wine Control Panel";
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

    if (!RegisterClassA(&wc)) return;

    CreateWindowExA(0, wc.lpszClassName, appName,
		    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		    CW_USEDEFAULT, CW_USEDEFAULT,
		    CW_USEDEFAULT, CW_USEDEFAULT,
		    hWnd, NULL, hInst, panel);
    if (!panel->hWnd) return;

    if (!panel->first) {
	/* FIXME appName & message should be localized  */
	MessageBoxA(panel->hWnd, "Cannot load any applets", appName, MB_OK);
	return;
    }

    while (GetMessageA(&msg, panel->hWnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static	void	Control_DoWindow(CPanel* panel, HWND hWnd, HINSTANCE hInst)
{
    HANDLE		h;
    WIN32_FIND_DATAW	fd;
    WCHAR		buffer[MAX_PATH];
    static const WCHAR wszAllCpl[] = {'*','.','c','p','l',0};
    WCHAR *p;

    GetSystemDirectoryW( buffer, MAX_PATH );
    p = buffer + strlenW(buffer);
    *p++ = '\\';
    lstrcpyW(p, wszAllCpl);

    if ((h = FindFirstFileW(buffer, &fd)) != INVALID_HANDLE_VALUE) {
        do {
	   lstrcpyW(p, fd.cFileName);
	   Control_LoadApplet(hWnd, buffer, panel);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
    }

    Control_DoInterface(panel, hWnd, hInst);
}

static	void	Control_DoLaunch(CPanel* panel, HWND hWnd, LPCWSTR wszCmd)
   /* forms to parse:
    *	foo.cpl,@sp,str
    *	foo.cpl,@sp
    *	foo.cpl,,str
    *	foo.cpl @sp
    *	foo.cpl str
    *   "a path\foo.cpl"
    */
{
    LPWSTR	buffer;
    LPWSTR	beg = NULL;
    LPWSTR	end;
    WCHAR	ch;
    LPWSTR       ptr;
    unsigned 	sp = 0;
    LPWSTR	extraPmts = NULL;
    int        quoted = 0;

    buffer = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(wszCmd) + 1) * sizeof(*wszCmd));
    if (!buffer) return;

    end = lstrcpyW(buffer, wszCmd);

    for (;;) {
	ch = *end;
        if (ch == '"') quoted = !quoted;
	if (!quoted && (ch == ' ' || ch == ',' || ch == '\0')) {
	    *end = '\0';
	    if (beg) {
	        if (*beg == '@') {
		    sp = atoiW(beg + 1);
		} else if (*beg == '\0') {
		    sp = 0;
		} else {
		    extraPmts = beg;
		}
	    }
	    if (ch == '\0') break;
	    beg = end + 1;
	    if (ch == ' ') while (end[1] == ' ') end++;
	}
	end++;
    }
    while ((ptr = StrChrW(buffer, '"')))
	memmove(ptr, ptr+1, lstrlenW(ptr)*sizeof(WCHAR));

    TRACE("cmd %s, extra %s, sp %d\n", debugstr_w(buffer), debugstr_w(extraPmts), sp);

    Control_LoadApplet(hWnd, buffer, panel);

    if (panel->first) {
       CPlApplet* applet = panel->first;

       assert(applet && applet->next == NULL);
       if (sp >= applet->count) {
	  WARN("Out of bounds (%u >= %u), setting to 0\n", sp, applet->count);
	  sp = 0;
       }
       if (applet->info[sp].dwSize) {
	  if (!applet->proc(applet->hWnd, CPL_STARTWPARMSA, sp, (LPARAM)extraPmts))
	     applet->proc(applet->hWnd, CPL_DBLCLK, sp, applet->info[sp].lData);
       }
       Control_UnloadApplet(applet);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
}

/*************************************************************************
 * Control_RunDLLW			[SHELL32.@]
 *
 */
void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow)
{
    CPanel	panel;

    TRACE("(%p, %p, %s, 0x%08lx)\n",
	  hWnd, hInst, debugstr_w(cmd), nCmdShow);

    memset(&panel, 0, sizeof(panel));

    if (!cmd || !*cmd) {
        Control_DoWindow(&panel, hWnd, hInst);
    } else {
        Control_DoLaunch(&panel, hWnd, cmd);
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
 * Control_FillCache_RunDLLA			[SHELL32.@]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLLA(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    FIXME("%p %p 0x%04lx 0x%04lx stub\n", hWnd, hModule, w, x);
    return 0;
}

/*************************************************************************
 * Control_FillCache_RunDLLW			[SHELL32.@]
 *
 */
HRESULT WINAPI Control_FillCache_RunDLLW(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
    return Control_FillCache_RunDLLA(hWnd, hModule, w, x);
}


/*************************************************************************
 * RunDLL_CallEntry16				[SHELL32.122]
 * the name is probably wrong
 */
void WINAPI RunDLL_CallEntry16( DWORD proc, HWND hwnd, HINSTANCE inst,
                                LPCSTR cmdline, INT cmdshow )
{
#if !defined(__CYGWIN__) && !defined (__MINGW32__) && !defined(_MSC_VER)
    WORD args[5];
    SEGPTR cmdline_seg;

    TRACE( "proc %lx hwnd %p inst %p cmdline %s cmdshow %d\n",
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
 * CallCPLEntry16				[SHELL32.166]
 *
 * called by desk.cpl on "Advanced" with:
 * hMod("DeskCp16.Dll"), pFunc("CplApplet"), 0, 1, 0xc, 0
 *
 */
DWORD WINAPI CallCPLEntry16(HMODULE hMod, FARPROC pFunc, DWORD dw3, DWORD dw4, DWORD dw5, DWORD dw6)
{
    FIXME("(%p, %p, %08lx, %08lx, %08lx, %08lx): stub.\n", hMod, pFunc, dw3, dw4, dw5, dw6);
    return 0x0deadbee;
}
