/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/vgafontedit/main.c
 * PURPOSE:     Main entry point of the application
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "precomp.h"

const WCHAR szAppName[] = L"ReactOS VGA Font Editor";

HINSTANCE hInstance;
HANDLE hProcessHeap;

INT WINAPI
wWinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HACCEL hAccel;
    INT nRet = 1;
    MSG msg;
    PMAIN_WND_INFO Info = 0;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInstance = hInst;
    hProcessHeap = GetProcessHeap();

    hAccel = LoadAcceleratorsW( hInstance, MAKEINTRESOURCEW(IDA_MAINACCELERATORS) );

    if( InitMainWndClass() && InitFontWndClass() && InitFontBoxesWndClass() && InitEditGlyphWndClasses() )
    {
        if( CreateMainWindow(nCmdShow, &Info) )
        {
            while( GetMessageW(&msg, NULL, 0, 0) )
            {
                if( !TranslateMDISysAccel(Info->hMdiClient, &msg) &&
                    !TranslateAccelerator(Info->hMainWnd, hAccel, &msg) )
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            nRet = 0;
        }
    }

    // Just unregister our window classes, don't care whether they were created or not
    UnInitEditGlyphWndClasses();
    UnInitFontBoxesWndClass();
    UnInitFontWndClass();
    UnInitMainWndClass();

    return nRet;
}
