/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  Layout.cpp
 *
 *   created on: 08/03/2000
 *   created by: Eric R. Mader
 */

#include <windows.h>
#include <stdio.h>

#include "paragraph.h"

#include "GDIGUISupport.h"
#include "GDIFontMap.h"
#include "UnicodeReader.h"
#include "ScriptCompositeFontInstance.h"

#include "resource.h"

#define ARRAY_LENGTH(array) (sizeof array / sizeof array[0])

struct Context
{
    le_int32 width;
    le_int32 height;
    Paragraph *paragraph;
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

#define APP_NAME "LayoutSample"

TCHAR szAppName[] = TEXT(APP_NAME);

void PrettyTitle(HWND hwnd, char *fileName)
{
    char title[MAX_PATH + 64];

    sprintf(title, "%s - %s", APP_NAME, fileName);

    SetWindowTextA(hwnd, title);
}

void InitParagraph(HWND hwnd, Context *context)
{
    SCROLLINFO si;

    if (context->paragraph != NULL) {
        // FIXME: does it matter what we put in the ScrollInfo
        // if the window's been minimized?
        if (context->width > 0 && context->height > 0) {
            context->paragraph->breakLines(context->width, context->height);
        }

        si.cbSize = sizeof si;
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
        si.nMin = 0;
        si.nMax = context->paragraph->getLineCount() - 1;
        si.nPage = context->height / context->paragraph->getLineHeight();
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    HWND hwnd;
    HACCEL hAccel;
    MSG msg;
    WNDCLASS wndclass;
    LEErrorCode status = LE_NO_ERROR;

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(LONG);
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName  = szAppName;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass)) {
        MessageBox(NULL, TEXT("This demo only runs on Windows 2000!"), szAppName, MB_ICONERROR);

        return 0;
    }

    hAccel = LoadAccelerators(hInstance, szAppName);

    hwnd = CreateWindow(szAppName, NULL,
                        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        600, 400,
                        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnregisterClass(szAppName, hInstance);
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    Context *context;
    static le_int32 windowCount = 0;
    static GDIFontMap *fontMap = NULL;
    static GDISurface *surface = NULL;
    static GDIGUISupport *guiSupport = new GDIGUISupport();
    static ScriptCompositeFontInstance *font = NULL;

    switch (message) {
    case WM_CREATE:
    {
        LEErrorCode fontStatus = LE_NO_ERROR;

        hdc = GetDC(hwnd);
        surface = new GDISurface(hdc);

        fontMap = new GDIFontMap(surface, "FontMap.GDI", 24, guiSupport, fontStatus);
        font    = new ScriptCompositeFontInstance(fontMap);

        if (LE_FAILURE(fontStatus)) {
            ReleaseDC(hwnd, hdc);
            return -1;
        }

        context = new Context();

        context->width  = 600;
        context->height = 400;

        context->paragraph = Paragraph::paragraphFactory("Sample.txt", font, guiSupport);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) context);

        windowCount += 1;
        ReleaseDC(hwnd, hdc);

        PrettyTitle(hwnd, "Sample.txt");
        return 0;
    }

    case WM_SIZE:
    {
        context = (Context *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
        context->width  = LOWORD(lParam);
        context->height = HIWORD(lParam);

        InitParagraph(hwnd, context);
        return 0;
    }

    case WM_VSCROLL:
    {
        SCROLLINFO si;
        le_int32 vertPos;

        si.cbSize = sizeof si;
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        vertPos = si.nPos;

        switch (LOWORD(wParam))
        {
        case SB_TOP:
            si.nPos = si.nMin;
            break;

        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;

        case SB_LINEUP:
            si.nPos -= 1;
            break;

        case SB_LINEDOWN:
            si.nPos += 1;
            break;

        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;

        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;

        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;

        default:
            break;
        }

        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hwnd, SB_VERT, &si);

        context = (Context *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (context->paragraph != NULL && si.nPos != vertPos) {
            ScrollWindow(hwnd, 0, context->paragraph->getLineHeight() * (vertPos - si.nPos), NULL, NULL);
            UpdateWindow(hwnd);
        }

        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        SCROLLINFO si;
        le_int32 firstLine, lastLine;

        hdc = BeginPaint(hwnd, &ps);
        SetBkMode(hdc, TRANSPARENT);

        si.cbSize = sizeof si;
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        firstLine = si.nPos;

        context = (Context *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (context->paragraph != NULL) {
            surface->setHDC(hdc);

            // NOTE: si.nPos + si.nPage may include a partial line at the bottom
            // of the window. We need this because scrolling assumes that the
            // partial line has been painted.
            lastLine  = min (si.nPos + (le_int32) si.nPage, context->paragraph->getLineCount() - 1);

            context->paragraph->draw(surface, firstLine, lastLine);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_OPEN:
        {
            OPENFILENAMEA ofn;
            char szFileName[MAX_PATH], szTitleName[MAX_PATH];
            static char szFilter[] = "Text Files (.txt)\0*.txt\0"
                                     "All Files (*.*)\0*.*\0\0";

            ofn.lStructSize       = sizeof (OPENFILENAMEA);
            ofn.hwndOwner         = hwnd;
            ofn.hInstance         = NULL;
            ofn.lpstrFilter       = szFilter;
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter    = 0;
            ofn.nFilterIndex      = 0;
            ofn.lpstrFile         = szFileName;
            ofn.nMaxFile          = MAX_PATH;
            ofn.lpstrFileTitle    = szTitleName;
            ofn.nMaxFileTitle     = MAX_PATH;
            ofn.lpstrInitialDir   = NULL;
            ofn.lpstrTitle        = NULL;
            ofn.Flags             = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
            ofn.nFileOffset       = 0;
            ofn.nFileExtension    = 0;
            ofn.lpstrDefExt       = "txt";
            ofn.lCustData         = 0L;
            ofn.lpfnHook          = NULL;
            ofn.lpTemplateName    = NULL;

            szFileName[0] = '\0';

            if (GetOpenFileNameA(&ofn)) {
                hdc = GetDC(hwnd);
                surface->setHDC(hdc);

                Paragraph *newParagraph = Paragraph::paragraphFactory(szFileName, font, guiSupport);

                if (newParagraph != NULL) {
                    context = (Context *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

                    if (context->paragraph != NULL) {
                        delete context->paragraph;
                    }

                    context->paragraph = newParagraph;
                    InitParagraph(hwnd, context);
                    PrettyTitle(hwnd, szTitleName);
                    InvalidateRect(hwnd, NULL, TRUE);

                }
            }

            //ReleaseDC(hwnd, hdc);

            return 0;
        }

        case IDM_FILE_EXIT:
        case IDM_FILE_CLOSE:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;

        case IDM_HELP_ABOUTLAYOUTSAMPLE:
            MessageBox(hwnd, TEXT("Windows Layout Sample 0.1\n")
                             TEXT("Copyright (C) 1998-2005 By International Business Machines Corporation and others.\n")
                             TEXT("Author: Eric Mader"),
                       szAppName, MB_ICONINFORMATION | MB_OK);
            return 0;

        }
        break;


    case WM_DESTROY:
    {
        context = (Context *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (context != NULL && context->paragraph != NULL) {
            delete context->paragraph;
        }

        delete context;

        if (--windowCount <= 0) {
            delete font;
            delete surface;

            PostQuitMessage(0);
        }

        return 0;
    }

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
