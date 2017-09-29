/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main entry point of the application
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

static const WCHAR szCharacterClipboardFormat[] = L"RosVgaFontChar";

HINSTANCE hInstance;
HANDLE hProcessHeap;
PWSTR szAppName;
UINT uCharacterClipboardFormat;

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

    AllocAndLoadString(&szAppName, IDS_APPTITLE);

    hAccel = LoadAcceleratorsW( hInstance, MAKEINTRESOURCEW(IDA_MAINACCELERATORS) );

    uCharacterClipboardFormat = RegisterClipboardFormatW(szCharacterClipboardFormat);
    if(!uCharacterClipboardFormat)
        return 1;

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

    HeapFree(hProcessHeap, 0, szAppName);

    // Just unregister our window classes, don't care whether they were created or not
    UnInitEditGlyphWndClasses();
    UnInitFontBoxesWndClass();
    UnInitFontWndClass();
    UnInitMainWndClass();

    return nRet;
}
