/*
 *   Control
 *   Copyright (C) 1998 by Marcel Baur <mbaur@g26.ethz.ch>
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
/*
 * Portions copyright Robert Dickenson <robd@reactos.org>, August 15, 2002.
 */
 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include "params.h"

//typedef   void (WINAPI *CONTROL_RUNDLL)(HWND hWnd, HINSTANCE hInst, LPCTSTR cmd, DWORD nCmdShow);
typedef void (*CONTROL_RUNDLL)(HWND hWnd, HINSTANCE hInst, LPCTSTR cmd, DWORD nCmdShow);

const TCHAR szLibName[] = _T("ROSHEL32.DLL");
#ifdef UNICODE
const char  szProcName[] = "Control_RunDLLW";
#else
const char  szProcName[] = "Control_RunDLLA";
#endif

#ifndef __GNUC__
#ifdef UNICODE
extern void __declspec(dllimport) Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);
#else
extern void __declspec(dllimport) Control_RunDLLA(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow);
#endif
#endif

#ifdef UNICODE
#define Control_RunDLL Control_RunDLLW
#else
#define Control_RunDLL Control_RunDLLA
#endif


void launch(LPTSTR lpCmdLine)
{
#if 0
    HMODULE         hShell32;
    CONTROL_RUNDLL  pControl_RunDLL;

    hShell32 = LoadLibrary(szLibName);
    if (hShell32) {
        pControl_RunDLL = (CONTROL_RUNDLL)(FARPROC)GetProcAddress(hShell32, szProcName);
        if (pControl_RunDLL) {
            pControl_RunDLL(GetDesktopWindow(), 0, lpCmdLine, SW_SHOW);
        } else {
            _tprintf(_T("ERROR: failed to Get Procedure Address for %s in Library %s\n"), szLibName, szProcName);
        }
        FreeLibrary(hShell32);
    } else {
        _tprintf(_T("ERROR: failed to Load Library %s\n"), szLibName);
    }
#else    
    Control_RunDLL(GetDesktopWindow(), 0, lpCmdLine, SW_SHOW);
#endif
    exit(0);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, CHAR *szParam, INT argc)
{
    TCHAR szParams[255];
    LPTSTR pParams = GetCommandLine();
    lstrcpy(szParams, pParams);
    CharUpper(szParams);

    _tprintf(_T("Control Panel Launcher...\n"));

    switch (--argc) {
    case 0:  /* no parameters - pop up whole "Control Panel" by default */
         launch(_T(""));
         break;
    case 1:  /* check for optional parameter */
         if (!lstrcmp(szParams,szP_DESKTOP))
             launch(szC_DESKTOP);
         if (!lstrcmp(szParams,szP_COLOR))
             launch(szC_COLOR);
         if (!lstrcmp(szParams,szP_DATETIME))
             launch(szC_DATETIME);
         if (!lstrcmp(szParams,szP_DESKTOP))
             launch(szC_DESKTOP);
         if (!lstrcmp(szParams,szP_INTERNATIONAL))
             launch(szC_INTERNATIONAL);
         if (!lstrcmp(szParams,szP_KEYBOARD))
             launch(szC_KEYBOARD);
         if (!lstrcmp(szParams,szP_MOUSE))
             launch(szC_MOUSE);
         if (!lstrcmp(szParams,szP_PORTS))
             launch(szC_PORTS);
         if (!lstrcmp(szParams,szP_PRINTERS))
             launch(szC_PRINTERS);
         /* try to launch if a .cpl file is given directly */
         launch(szParams);
         break;
    default: _tprintf(_T("Syntax error."));
    }
    return 0;
}
