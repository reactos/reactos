/*
 * WineCalc (winecalc.c)
 *
 * Copyright 2003 James Briggs
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

#include <stdio.h> // sprintf
#include <math.h>
#include <stdlib.h>

#include <windows.h>
#include <tchar.h>
// #include <commctrl.h>

#include "winecalc.h"
#include "dialog.h"
#include "stats.h"
#include "resource.h"

// lame M$ math library doesn't support hyp functions
//#ifdef _MSC_VER
#define atanh(X) atan(X)
#define asinh(X) asin(X)
#define acosh(X) acos(X)
//#endif

// How winecalc works
//
// 1. In the original calculator, numbers are "extended precision".
//    We emulate with calcfloat (calcfloat floating point) for now.
//    Note that recent M$ compilers 5.0 and later do not implement double float,
//    and do not support 80-bit numbers. Max width is 64-bits.
//
// 2. 4 temp areas:
//
//    i) current edit buffer value for keystrokes
//   ii) computed value area for last computation
//  iii) memory area for MR, MC, MS, M+
//   iv) formatted display buffer
//
// 3. Memory Store works off current buffer, not value
//
// 4. display limit is 32 numbers plus 10 commas plus one period = 43 characters
//
// 5. original calc is better called SciCalc and saves settings in win.ini:
//
// [SciCalc]
// layout=1 (0 = scientific mode, 1 = basic mode)
// UseSep=1 (0 = no digits separator, 1 = enable digits separator)
//
// 6. Window Menus
//
//    The menus know their own states, so we don't need to track them
//    When switching to Standard Calculator Mode, number base is changed to Decimal and Trig Mode to Degrees,
//    but Word Size Mode is unchanged
//
// 7. It would be nice to add command line parsing for batch files
//

// various display error messages

static TCHAR err_invalid        [CALC_BUF_SIZE];
static TCHAR err_divide_by_zero [CALC_BUF_SIZE];
static TCHAR err_undefined      [CALC_BUF_SIZE];

// calculator state is kept here

CALC calc;

static RECT rFiller;

static int keys[CALC_NS_COUNT]['f'+1]; // note: sparse array

// map from button press to calc[] index
//  0,        ...                      9,  A,  B,  C,  D,  E,  F,FE,DMS,SIN,COS,TAN,EXP, PI
static const int h[] = {
    0, 0, 33, 34, 21, 22, 23, 10, 11, 12, 54, 55, 56 ,57, 58 ,59, 6, 17, 28, 39, 50, 18, 53 };

// enable status of various buttons on sci mode depending on number base
static const int btn_toggle[CALC_NS_COUNT][TOGGLE_COUNT] =
//    0, ...                     9, A, B, C, D, E, F,FE,DMS,SIN,COS,TAN,EXP,PI
{
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 }
};

static int hms[] = { 64, 65, 66 };
static int hws[] = { 69, 70, 71, 72 };

static int sta[] = { 16, 27, 38, 49 };

static HMENU menus[3];

static TCHAR appname[40];

static int debug;

int parse(int wParam, int lParam);

HWND hWndDlgStats;

extern HWND hWndListBox;

HINSTANCE hInstance;

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow )
{
    MSG msg;
    WNDCLASS wc;
    HWND hWnd;
    HACCEL haccel;
    TCHAR s[CALC_BUF_SIZE];
    int r;

    hInstance = hInst;

    r = GetProfileString(TEXT("SciCalc"),
                         TEXT("layout"),
                         TEXT("0"),
                         s,
                         CALC_BUF_SIZE
    );

    calc.sciMode  = _ttoi(s);

    if (calc.sciMode != 0 &&
        calc.sciMode != 1)
        calc.sciMode = 1; // Standard Mode

    r = GetProfileString(TEXT("SciCalc"),
                         TEXT("UseSep"),
                         TEXT("0"),
                         s,
                         CALC_BUF_SIZE
        );

    calc.digitGrouping  = _ttoi(s);

    if (calc.digitGrouping != 0 &&
        calc.digitGrouping != 1)
        calc.digitGrouping = 1;

    calc.new      = 1; // initialize struct values

    if (!LoadString( hInst, IDS_APPNAME,            appname,            sizeof(appname) / sizeof(appname[0])))
        exit(1);
    if (!LoadString( hInst, IDS_ERR_INVALID_INPUT,  err_invalid,        sizeof(err_invalid) / sizeof(err_invalid[0])))
        exit(1);
    if (!LoadString( hInst, IDS_ERR_DIVIDE_BY_ZERO, err_divide_by_zero, sizeof(err_divide_by_zero) / sizeof(err_divide_by_zero[0])))
        exit(1);
    if (!LoadString( hInst, IDS_ERR_UNDEFINED,      err_undefined,      sizeof(err_undefined) / sizeof(err_undefined[0])))
        exit(1);

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CALCICON));
    wc.hCursor       = LoadCursor( NULL, IDI_APPLICATION );
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = appname;

    if (!RegisterClass(&wc)) exit(1);

    hWnd = CreateWindow( appname,
        appname,
        WS_CLIPSIBLINGS | (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX),
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        calc.sciMode ? CALC_STANDARD_WIDTH :  CALC_SCIENTIFIC_WIDTH,
        calc.sciMode ? CALC_STANDARD_HEIGHT : CALC_SCIENTIFIC_HEIGHT,
        NULL,
        NULL,
        hInst,
        NULL );

    if (!hWnd)
        exit(1);

    ShowWindow( hWnd, cmdshow );
    UpdateWindow( hWnd );

    if (!(haccel = LoadAccelerators(hInst, TEXT("MAIN_MENU"))))
        exit(1);

    while( GetMessage(&msg, NULL, 0, 0) ) {
        if (hWndDlgStats == 0 || !IsDialogMessage(hWndDlgStats, &msg)) {
           if (!TranslateAccelerator( hWnd, haccel, &msg )) {
              TranslateMessage( &msg );
              DispatchMessage( &msg );
            }
        }
    }

    return msg.wParam;
}

LRESULT WINAPI MainProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (msg) {

    case WM_CREATE:
        calc.hInst = ((LPCREATESTRUCT) lParam)->hInstance;
        calc.hWnd = hWnd;

        InitLuts();
        InitCalc( &calc );
        InitMenus(calc.hInst);

        if (calc.sciMode)
            SetMenu(hWnd, menus[MENU_STD]);
        else
            SetMenu(hWnd, menus[MENU_SCIMS]);

        calc_buffer_display(&calc);

        return 0;

    case WM_PAINT:
        {
            HDC hMemDC;

            hdc = BeginPaint( hWnd, &ps );
            hMemDC = CreateCompatibleDC( hdc );

            DrawCalc( hdc, hMemDC, &ps, &calc );

            DeleteDC( hMemDC );
            EndPaint( hWnd, &ps );

            return 0;
        }

    case WM_MOVE:
        calc.pos.x = (unsigned) LOWORD(lParam);
        calc.pos.y = (unsigned) HIWORD(lParam);
        return 0;

    case WM_DESTROY:
        {
            int r;
            TCHAR s[CALC_BUF_SIZE];

            _stprintf(s, TEXT("%d"), calc.sciMode);
            r = WriteProfileString(TEXT("SciCalc"), TEXT("layout"), s);

            _stprintf(s, TEXT("%d"), calc.digitGrouping);
            r = WriteProfileString(TEXT("SciCalc"), TEXT("UseSep"), s);
        }

        DestroyCalc( &calc );
		DestroyMenus();
        PostQuitMessage( 0 );
        return 0;

    case WM_KEYDOWN:
        switch (wParam) {

        case VK_F1:
            calc.next = 1;
            MessageBox(hWnd, TEXT("No Help Available"), TEXT("Windows Help"), MB_OK);
            return 0;

        case VK_F2: // DWORD

            calc.next = 1;
            if (!calc.sciMode) {
                if (calc.numBase == NBASE_DECIMAL)
                    SendMessage(hWnd, WM_COMMAND, ID_CALC_MS_DEGREES, lParam);
                else
                    SendMessage(hWnd, WM_COMMAND, ID_CALC_WS_DWORD, lParam);
            }
            return 0;

        case VK_F3: // WORD

            calc.next = 1;
            if (!calc.sciMode) {
                if (calc.numBase == NBASE_DECIMAL)
                    SendMessage(hWnd, WM_COMMAND, ID_CALC_MS_RADIANS, lParam);
                else
                    SendMessage(hWnd, WM_COMMAND, ID_CALC_WS_WORD, lParam);
            }
            return 0;

        case VK_F4: // Byte

            // Grad
            calc.next = 1;
            if (!calc.sciMode) {
                if (calc.numBase == NBASE_DECIMAL)
                    SendMessage(hWnd, WM_COMMAND, ID_CALC_MS_GRADS, lParam);
                else
                    SendMessage(hWnd, WM_COMMAND, ID_CALC_WS_BYTE, lParam);
            }
            return 0;

        case VK_F5: // Hex

            calc.next = 1;
            if (!calc.sciMode)
                SendMessage(hWnd, WM_COMMAND, ID_CALC_NS_HEX, lParam);
            return 0;

        case VK_F6: // Decimal

            calc.next = 1;
            if (!calc.sciMode)
                SendMessage(hWnd, WM_COMMAND, ID_CALC_NS_DEC, lParam);
            return 0;

        case VK_F7: // Octal

            calc.next = 1;
            if (!calc.sciMode)
                SendMessage(hWnd, WM_COMMAND, ID_CALC_NS_OCT, lParam);
            return 0;

        case VK_F8: // Binary

            calc.next = 1;
            if (!calc.sciMode)
                SendMessage(hWnd, WM_COMMAND, ID_CALC_NS_BIN, lParam);
            return 0;

        case VK_F12: // QWORD

            calc.next = 1;
            if (!calc.sciMode)
                SendMessage(hWnd, WM_COMMAND, ID_CALC_WS_QWORD, lParam);
            return 0;

        case VK_F9: // +/-

            if (!calc.sciMode)
                SendMessage(hWnd, WM_CHAR, 'Z', lParam);
            return 0;

        case VK_INSERT: // Dat
            calc.next = 1;
            SendMessage(hWndListBox, LB_INSERTSTRING, -1, (LPARAM)calc.buffer);
            InvalidateRect(hWndDlgStats, NULL, TRUE); // update list count at bottom edge of dialog
            UpdateWindow(hWndDlgStats);
            return 0;

        case VK_DELETE:

            calc.next = 1;
            calc.buffer[0] = TEXT('\0');
            calc_buffer_display(&calc);
            return 0;

        default:
            break;
        }

        return 0;

    case WM_CHAR:
        if (debug)
            show_debug(&calc, TEXT("Start WM_CHAR"), wParam, lParam);

        switch (wParam) {

        case TEXT('\x13'): // Ctrl+S Sta Statistics

            if (hWndDlgStats) { // Statistics Box already displayed, focus but don't create another one
                // SetFocus(hWndDlgStats);
                // SendMessage(hWndDlgStats, WM_SETFOCUS, 0, 0);
            }
            else {
                int i;

                if (lParam == 1) {
                   for (i=0;i<4;i++)
                       EnableWindow((HWND)calc.cb[sta[i]].hBtn, FALSE);
                }
                else {
                   for (i=0;i<4;i++)
                       EnableWindow((HWND)calc.cb[sta[i]].hBtn, TRUE);

                   hWndDlgStats = CreateDialog( // modeless dialog
                      calc.hInst,  	            // handle to application instance
                      TEXT("DLG_STATS"),	            // identifies dialog box template name
                      hWnd,	                    // handle to owner window
                      StatsDlgProc);            // pointer to dialog box procedure

                   if (!hWndDlgStats)
                      exit(1);

                   SendMessage(hWndDlgStats, WM_SETFOCUS, 0, 0);
                }
            }

            break;

        case TEXT('\x01'): // Ctrl+A Ave Statistics
        case TEXT('\x04'): // Ctrl+D s Standard Deviation Statistics
        case TEXT('\x14'): // Ctrl+T Sum Statistics
            {
               int i;
               int n;

               TCHAR s[CALC_BUF_SIZE];
               double val = 0L;
               double avg = 0L;
               double total = 0L;
               double x[1024];

               // This is all lame. Here we are querying the list control for items and calculating the total.
               // We should have an array of intermediate results to work with to avoid roundoff errors, etc.

               n = SendMessage(hWndListBox, LB_GETCOUNT, 0, 0);

               for (i=0;i<n;i++) {
                   SendMessage(hWndListBox, LB_GETTEXT,   i, (LPARAM)s);
                   val = calc_atof(s,calc.numBase);
                   total += val;
                   x[i] = val;
               }

               if (LOWORD(wParam) != TEXT('\x14')) // not sum only
                  avg = total / n;

               if (LOWORD(wParam == TEXT('\x04'))) {   // standard deviation is sqrt(sum( xbar - x )^2 / (n-1))
                   val = 0L;

                   for (i=0;i<n;i++)
                       val += pow(x[i] - avg, 2);

                   if (calc.invMode)
                      total = sqrt(val / n);       // complete population
                   else
                      total = sqrt(val / (n - 1)); // sample of population
               }

               if (LOWORD(wParam) == TEXT('\x01')) // average or mean
                   total = avg;

               calc_ftoa(&calc, total, s);
               _tcscpy(calc.buffer, s);
               calc_buffer_display(&calc);
            }

            break;

        case TEXT('\x03'): // Ctrl+C Copy
            {
                int i;
                int len;
                TCHAR *s;
                HGLOBAL hGlobalMemory;
                LPTSTR pGlobalMemory;

                if (!(len = _tcslen(calc.display)))
                    return 0;

                if (!(s = calc.display))
                    return 0;

                if (s[len - 1] == TEXT('.') || s[len - 1] == TEXT(','))
                    len--;

                if (!(hGlobalMemory = GlobalAlloc(GHND, (len + 1) * sizeof(TCHAR))))
                    return 0;

                if (!(pGlobalMemory = GlobalLock(hGlobalMemory)))
                    return 0;

                for (i = 0; i < len; i++)
                    *pGlobalMemory++ = *s++;

                pGlobalMemory[len - 1] = 0;

                GlobalUnlock(hGlobalMemory); // call GetLastError() for exception handling

                if (!OpenClipboard(hWnd))
                    return 0;

                if (!EmptyClipboard())
                    return 0;

                if (!SetClipboardData(CF_TTEXT, hGlobalMemory))
                    return 0;

                if (!CloseClipboard())
                    return 0;
            }
            break;

        case TEXT('\x16'): // Ctrl+V Paste
            {
                TCHAR *s;
                TCHAR c;
                int cmd = 0;
                size_t size = 0;
                size_t i = 0;
                HGLOBAL hGlobalMemory;
                LPTSTR pGlobalMemory;

                if (IsClipboardFormatAvailable(CF_TTEXT)) {
                    if (!OpenClipboard(hWnd))
                        return 0;

                    if (!(hGlobalMemory = GetClipboardData(CF_TTEXT)))
                        return 0;

                    if (!(size = GlobalSize(hGlobalMemory)))
                        return 0;

                    if (!(s = (TCHAR *)malloc(size)))
                        return 0;

                    if (!(pGlobalMemory = GlobalLock(hGlobalMemory)))
                        return 0;

                    _tcscpy(s, pGlobalMemory);

                    GlobalUnlock(hGlobalMemory);

                    if (!CloseClipboard())
                        return 0;

                    // calc paste protocol
                    //
                    // :c clear memory                WM_CHAR, 0x0c
                    // :e enable scientific notation  WM_CHAR, 'v'
                    // :m store display in memory     WM_CHAR, 0x0d
                    // :p add display to memory       WM_CHAR, 0x10
                    // :q clear current calculation   WM_CHAR, '\x1b'
                    // :r read memory into display    WM_CHAR, 0x12
                    // \  Dat                         WM_CHAR, VK_INSERT
                    //
                    // parse the pasted data, validate and SendMessage it one character at a time.
                    // it would appear wincalc does it this way (slow), although very slow appearing on Wine.

                    while ((c = *s++) && (i++ < size / sizeof(TCHAR))) {
                        if (c == TEXT(':')) {
                            cmd = 1;
                        }
                        else if (c == TEXT('\\')) {
                            SendMessage(hWnd, WM_KEYDOWN, VK_INSERT, lParam);
                        }
                        else {
                            if (cmd) {
                                cmd = 0;

                                switch(c) {
                                case TEXT('c'): // clear memory

                                case TEXT('C'):
                                    SendMessage(hWnd, WM_CHAR, 0x0c, lParam);
                                    break;
                                case TEXT('e'): // enable scientific notation

                                case TEXT('E'):
                                    SendMessage(hWnd, WM_CHAR, 'v', lParam);
                                    break;
                                case TEXT('m'): // store display in memory

                                case TEXT('M'):
                                    SendMessage(hWnd, WM_CHAR, 0x0d, NUMBER_OF_THE_BEAST);
                                    break;
                                case TEXT('p'): // add display to memory

                                case TEXT('P'):
                                    SendMessage(hWnd, WM_CHAR, 0x10, lParam);
                                    break;
                                case TEXT('q'): // clear current calculation

                                case TEXT('Q'):
                                    SendMessage(hWnd, WM_CHAR, TEXT('\x1b'), lParam);
                                    break;
                                case TEXT('r'): // read memory into display

                                case TEXT('R'):
                                    SendMessage(hWnd, WM_CHAR, 0x12, lParam);
                                    break;
                                default: // just eat it but complain

                                    MessageBeep(0);
                                    break;
                                }
                            }
                            else {
                                if ((calc.numBase == NBASE_HEX) &&
                                        ((c >= TEXT('0') && c <= TEXT('9')) ||
                                        (c >= TEXT('a') && c <= TEXT('f')) ||
                                        (c >= TEXT('A') && c <= TEXT('F')))) {

                                    SendMessage(hWnd, WM_CHAR, c, lParam);
                                }
                                else if ((calc.numBase == NBASE_DECIMAL) &&
                                        (c >= TEXT('0') && c <= TEXT('9'))) {
                                    SendMessage(hWnd, WM_CHAR, c, lParam);
                                }
                                else if ((calc.numBase == NBASE_OCTAL) &&
                                        (c >= TEXT('0') && c <= TEXT('7'))) {
                                    SendMessage(hWnd, WM_CHAR, c, lParam);
                                }
                                else if ((calc.numBase == NBASE_BINARY) &&
                                        (c == TEXT('0') || c == TEXT('1'))) {
                                    SendMessage(hWnd, WM_CHAR, c, lParam);
                                }
                                else if (c == TEXT('.') || c == TEXT(',') ||
                                        c == TEXT('e') || c == TEXT('E') ||
                                        c == TEXT('+') || c == TEXT('-')) {
                                    SendMessage(hWnd, WM_CHAR, c, lParam);
                                }
                                else if (c == TEXT(' ') ||  // eat harmless trash here
                                    c == TEXT(';') ||
                                        c == TEXT(':')) {
                                    ;                  // noop
                                }
                                else {                // extra spicy trash gets noticed

                                    MessageBeep(0);    // uh, beeps can get annoying. maybe rate limit.
                                }
                            }
                        }
                    }
                }
            }

            break;
        default:
            {
                parse(wParam, lParam);
            }
        } // switch WM_CHAR

        calc_buffer_display(&calc);
        return 0;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {

        case IDM_COPY:
            SendMessage(hWnd, WM_CHAR, TEXT('\x03'), lParam);
            return 0;

        case IDM_PASTE:
            SendMessage(hWnd, WM_CHAR, TEXT('\x16'), lParam);
            return 0;

        case IDM_MODE_STANDARD:
            if (!calc.sciMode) {
                int i;
                RECT lpRect;

                calc.sciMode = 1;
                calc.trigMode = TRIGMODE_DEGREES;
                calc.numBase  = NBASE_DECIMAL;

                EnableWindow(hWnd, FALSE);


                for (i=1;i<COUNT_MENUS;i++) {
                    if (calc.numBase != ID_CALC_NS_DEC) {
                        CheckMenuItem(menus[i], ID_CALC_NS_HEX, MF_UNCHECKED);
                        CheckMenuItem(menus[i], ID_CALC_NS_DEC, MF_CHECKED);
                        CheckMenuItem(menus[i], ID_CALC_NS_OCT, MF_UNCHECKED);
                        CheckMenuItem(menus[i], ID_CALC_NS_BIN, MF_UNCHECKED);
                    }
                }

                for (i=1;i<COUNT_MENUS;i++) {
                    CheckMenuItem(menus[i], ID_CALC_MS_DEGREES, MF_CHECKED);
                    CheckMenuItem(menus[i], ID_CALC_MS_RADIANS, MF_UNCHECKED);
                    CheckMenuItem(menus[i], ID_CALC_MS_GRADS,   MF_UNCHECKED);
                }

                SetMenu(hWnd, menus[MENU_STD]);

                // SendMessage(hWnd, WM_SIZE, SIZE_RESTORED, CALC_STANDARD_WIDTH | (CALC_STANDARD_HEIGHT << 8));
                GetWindowRect(hWnd, &lpRect);
                MoveWindow(hWnd, lpRect.left, lpRect.top, CALC_STANDARD_WIDTH, CALC_STANDARD_HEIGHT, TRUE);
                DestroyCalc(&calc);
                InitCalc(&calc);

                EnableWindow(hWnd, TRUE);

            }
            return 0;

        case IDM_MODE_SCIENTIFIC:
            if (calc.sciMode) {
                RECT lpRect;
                calc.sciMode = 0;

                EnableWindow(hWnd, FALSE);

                SetMenu(hWnd, menus[MENU_SCIMS]);

                GetWindowRect(hWnd, &lpRect);
                MoveWindow(hWnd, lpRect.left, lpRect.top, CALC_SCIENTIFIC_WIDTH, CALC_SCIENTIFIC_HEIGHT, TRUE);
                DestroyCalc(&calc);
                InitCalc(&calc);

                if (calc.invMode)
                    SendMessage(calc.cb[67].hBtn, BM_SETCHECK, TRUE, 0);

                if (calc.hypMode)
                    SendMessage(calc.cb[68].hBtn, BM_SETCHECK, TRUE, 0);

                EnableWindow(hWnd, TRUE);

            }
            return 0;

        case IDM_DIGIT_GROUPING:
            {
                int i;
                int n;

                calc.digitGrouping = !calc.digitGrouping;

                n = (calc.digitGrouping ? MF_CHECKED : MF_UNCHECKED);

                for (i=0;i<COUNT_MENUS;i++)
                    CheckMenuItem(menus[i], IDM_DIGIT_GROUPING, n);

                calc_buffer_display(&calc);
            }
            return 0;

        case IDM_HELP_TOPICS:
            MessageBox(hWnd, TEXT("No Help Available"), TEXT("Windows Help"), MB_OK);
            return 0;

        case IDM_ABOUT:
            DialogBox( calc.hInst, TEXT("DLG_ABOUT"), hWnd, AboutDlgProc );
            return 0;

        case ID_CALC_NS_HEX:
        case ID_CALC_NS_DEC:
        case ID_CALC_NS_OCT:
        case ID_CALC_NS_BIN:
            {
                int i;
                int w = LOWORD(wParam);

                if (w == ID_CALC_NS_HEX) {
                    if (calc.numBase == NBASE_HEX)
                        return 0;
                    else
                        calc.numBase = NBASE_HEX;
                }
                else if (w == ID_CALC_NS_DEC) {
                    if (calc.numBase == NBASE_DECIMAL)
                        return 0;
                    else
                        calc.numBase = NBASE_DECIMAL;
                }
                else if (w == ID_CALC_NS_OCT) {
                    if (calc.numBase == NBASE_OCTAL)
                        return 0;
                    else
                        calc.numBase = NBASE_OCTAL;
                }
                else if (w == ID_CALC_NS_BIN) {
                    if (calc.numBase == NBASE_BINARY)
                        return 0;
                    else
                        calc.numBase = NBASE_BINARY;
                }

                for (i=0;i<CALC_NS_COUNT;i++)
                    SendMessage(calc.cb[60+i].hBtn, BM_SETCHECK, w == (ID_CALC_NS_HEX + i) ? 1 : 0, 0);

                for (i=2;i<TOGGLE_COUNT;i++) { // skip 0 and 1, always valid
                    if (btn_toggle[w - ID_CALC_NS_HEX ][i]) {
                        if (!IsWindowEnabled((HWND)calc.cb[h[i]].hBtn))
                            EnableWindow((HWND)calc.cb[h[i]].hBtn, TRUE);

                    }
                    else {
                        if (IsWindowEnabled((HWND)calc.cb[h[i]].hBtn))
                            EnableWindow((HWND)calc.cb[h[i]].hBtn, FALSE);
                    }
                }

                if (w == ID_CALC_NS_DEC) {
                    for (i=0;i<CALC_WS_COUNT;i++) {
                        if (IsWindowEnabled((HWND)calc.cb[hws[i]].hBtn)) {
                            ShowWindow((HWND)calc.cb[hws[i]].hBtn, SW_HIDE);
                            EnableWindow((HWND)calc.cb[hws[i]].hBtn, FALSE);
                        }
                    }
                    for (i=0;i<CALC_MS_COUNT;i++) {
                        if (!IsWindowEnabled((HWND)calc.cb[hms[i]].hBtn)) {
                            ShowWindow((HWND)calc.cb[hms[i]].hBtn, SW_SHOWNORMAL);
                            EnableWindow((HWND)calc.cb[hms[i]].hBtn, TRUE);
                        }
                    }
                }
                else {
                    for (i=0;i<CALC_MS_COUNT;i++) {
                        if (IsWindowEnabled((HWND)calc.cb[hms[i]].hBtn)) {
                            ShowWindow((HWND)calc.cb[hms[i]].hBtn, SW_HIDE);
                            EnableWindow((HWND)calc.cb[hms[i]].hBtn, FALSE);
                        }
                    }
                    for (i=0;i<CALC_WS_COUNT;i++) {
                        if (!IsWindowEnabled((HWND)calc.cb[hws[i]].hBtn)) {
                            ShowWindow((HWND)calc.cb[hws[i]].hBtn, SW_SHOWNORMAL);
                            EnableWindow((HWND)calc.cb[hws[i]].hBtn, TRUE);
                        }
                    }
                }

                CheckMenuItem(menus[i], ID_CALC_MS_DEGREES,  MF_CHECKED);

                for (i=1;i<COUNT_MENUS;i++) { // skip the simple Standard calculator mode
                    CheckMenuItem(menus[i], ID_CALC_NS_HEX,     (w == ID_CALC_NS_HEX ? MF_CHECKED : MF_UNCHECKED) );
                    CheckMenuItem(menus[i], ID_CALC_NS_DEC,     (w == ID_CALC_NS_DEC ? MF_CHECKED : MF_UNCHECKED) );
                    CheckMenuItem(menus[i], ID_CALC_NS_OCT,     (w == ID_CALC_NS_OCT ? MF_CHECKED : MF_UNCHECKED) );
                    CheckMenuItem(menus[i], ID_CALC_NS_BIN,     (w == ID_CALC_NS_BIN ? MF_CHECKED : MF_UNCHECKED) );
                }

                if (wParam == ID_CALC_NS_DEC) {
                    SetMenu(hWnd, menus[MENU_SCIMS]);

                }
                else {
                    calc.displayMode = 0;
                    SetMenu(hWnd, menus[MENU_SCIWS]);
                }
            }
            calc_buffer_display(&calc);

            break;

        case ID_CALC_MS_DEGREES:
        case ID_CALC_MS_RADIANS:
        case ID_CALC_MS_GRADS:
            {
                int i;
                int w = LOWORD(wParam);

                if (w == ID_CALC_MS_DEGREES)
                    calc.trigMode = TRIGMODE_DEGREES;
                else if (w == ID_CALC_MS_RADIANS)
                    calc.trigMode = TRIGMODE_RADIANS;
                else if (w == ID_CALC_MS_GRADS)
                    calc.trigMode = TRIGMODE_GRADS;
                else
                    return 0;

                for (i=0;i<CALC_MS_COUNT;i++)
                    SendMessage(calc.cb[64+i].hBtn, BM_SETCHECK, w == (ID_CALC_MS_DEGREES  + i) ? 1 : 0, 0);

                for (i=1;i<COUNT_MENUS;i++) { // skip the simple Standard calculator mode
                    CheckMenuItem(menus[i], ID_CALC_MS_DEGREES, (wParam == ID_CALC_MS_DEGREES) ? MF_CHECKED : MF_UNCHECKED);
                    CheckMenuItem(menus[i], ID_CALC_MS_RADIANS, (wParam == ID_CALC_MS_RADIANS) ? MF_CHECKED : MF_UNCHECKED);
                    CheckMenuItem(menus[i], ID_CALC_MS_GRADS,   (wParam == ID_CALC_MS_GRADS)   ? MF_CHECKED : MF_UNCHECKED);
                }
            }

            SetFocus(hWnd);
            return 0;

        case ID_CALC_WS_QWORD:
        case ID_CALC_WS_DWORD:
        case ID_CALC_WS_WORD:
        case ID_CALC_WS_BYTE:
            {
                int i;
                int w = LOWORD(wParam);

                calc.wordSize = w;

                for (i=0;i<CALC_WS_COUNT;i++)
                    SendMessage(calc.cb[69+i].hBtn, BM_SETCHECK, LOWORD(wParam) == (ID_CALC_WS_QWORD  + i) ? 1 : 0, 0);

                for (i=1; i<COUNT_MENUS; i++) { // skip the simple Standard calculator mode
                    CheckMenuItem(menus[i], ID_CALC_WS_QWORD, (w == ID_CALC_WS_QWORD ? MF_CHECKED : MF_UNCHECKED) );
                    CheckMenuItem(menus[i], ID_CALC_WS_DWORD, (w == ID_CALC_WS_DWORD ? MF_CHECKED : MF_UNCHECKED) );
                    CheckMenuItem(menus[i], ID_CALC_WS_WORD,  (w == ID_CALC_WS_WORD  ? MF_CHECKED : MF_UNCHECKED) );
                    CheckMenuItem(menus[i], ID_CALC_WS_BYTE,  (w == ID_CALC_WS_BYTE  ? MF_CHECKED : MF_UNCHECKED) );
                }
            }

            SetFocus(hWnd);
            return 0;

        case ID_CALC_CB_INV:
            if (calc.invMode)
                calc.invMode = 0;
            else
                calc.invMode = 1;

            SendMessage(calc.cb[67].hBtn, BM_SETCHECK, calc.invMode ? TRUE : FALSE, 0);
            SetFocus(hWnd);
            return 0;

        case ID_CALC_CB_HYP:
            if (calc.hypMode)
                calc.hypMode = 0;
            else
                calc.hypMode = 1;

            SendMessage(calc.cb[68].hBtn, BM_SETCHECK, calc.hypMode ? TRUE : FALSE, 0);
            SetFocus(hWnd);
            return 0;

        default:
            if (HIWORD(wParam) == BN_CLICKED) {

                if (calc.err &&
                    LOWORD(wParam) != ID_CALC_CLEAR_ENTRY &&
                    LOWORD(wParam) != ID_CALC_CLEAR_ALL) {

                    MessageBeep(0);
                    return 0;
                }
                else {
                    calc.err = 0;
                }

                switch (LOWORD(wParam)) {

                case ID_CALC_ZERO:
                case ID_CALC_ONE:
                case ID_CALC_TWO:
                case ID_CALC_THREE:
                case ID_CALC_FOUR:
                case ID_CALC_FIVE:
                case ID_CALC_SIX:
                case ID_CALC_SEVEN:
                case ID_CALC_EIGHT:
                case ID_CALC_NINE:
                    SendMessage(hWnd, WM_CHAR, LOWORD(wParam)+TEXT('0') , lParam);
                    break;

                case ID_CALC_A:
                    SendMessage(hWnd, WM_CHAR, TEXT('a'), lParam);
                    break;

                case ID_CALC_B:
                    SendMessage(hWnd, WM_CHAR, TEXT('b'), lParam);
                    break;

                case ID_CALC_C:
                    SendMessage(hWnd, WM_CHAR, TEXT('c'), lParam);
                    break;

                case ID_CALC_D:
                    SendMessage(hWnd, WM_CHAR, TEXT('d'), lParam);
                    break;

                case ID_CALC_E:
                    SendMessage(hWnd, WM_CHAR, TEXT('e'), lParam);
                    break;

                case ID_CALC_F:
                    SendMessage(hWnd, WM_CHAR, TEXT('f'), lParam);
                    break;

                case ID_CALC_DECIMAL:
                    SendMessage(hWnd, WM_CHAR, TEXT('.'), lParam);
                    break;

                case ID_CALC_BACKSPACE:
                    SendMessage(hWnd, WM_CHAR, TEXT('\b'), lParam);
                    break;

                case ID_CALC_CLEAR_ENTRY:
                    SendMessage(hWnd, WM_KEYDOWN, VK_DELETE , lParam);
                    break;

                case ID_CALC_CLEAR_ALL:
                    SendMessage(hWnd, WM_CHAR, TEXT('\x1b'), lParam);
                    break;

                case ID_CALC_MEM_CLEAR:
                    SendMessage(hWnd, WM_CHAR, 0x0c, lParam);
                    break;

                case ID_CALC_MEM_RECALL:
                    SendMessage(hWnd, WM_CHAR, 0x12, lParam);
                    break;

                case ID_CALC_MEM_STORE:
                    SendMessage(hWnd, WM_CHAR, 0x0d, NUMBER_OF_THE_BEAST); // trying to tell between Return and Ctrl+M
                    break;

                case ID_CALC_MEM_PLUS:
                    SendMessage(hWnd, WM_CHAR, 0x10, lParam);
                    break;

                case ID_CALC_SQRT:
                    SendMessage(hWnd, WM_CHAR, TEXT('?'), lParam); // this is not a wincalc keystroke
                    break;

                case ID_CALC_SQUARE:
                    SendMessage(hWnd, WM_CHAR, TEXT('@'), lParam);
                    break;

                case ID_CALC_PI:
                    SendMessage(hWnd, WM_CHAR, TEXT('p'), lParam);
                    break;

                case ID_CALC_LN:
                    SendMessage(hWnd, WM_CHAR, TEXT('n'), lParam);
                    break;

                case ID_CALC_LOG10:
                    SendMessage(hWnd, WM_CHAR, TEXT('l'), lParam);
                    break;

                case ID_CALC_CUBE:
                    SendMessage(hWnd, WM_CHAR, TEXT('#'), lParam);
                    break;

                case ID_CALC_POWER:
                    SendMessage(hWnd, WM_CHAR, TEXT('y'), lParam);
                    break;

                case ID_CALC_SIN:
                    SendMessage(hWnd, WM_CHAR, TEXT('s'), lParam);
                    break;

                case ID_CALC_COS:
                    SendMessage(hWnd, WM_CHAR, TEXT('o'), lParam);
                    break;

                case ID_CALC_TAN:
                    SendMessage(hWnd, WM_CHAR, TEXT('t'), lParam);
                    break;

                case ID_CALC_LSH:
                    SendMessage(hWnd, WM_CHAR, TEXT('<'), lParam);
                    break;

                case ID_CALC_NOT:
                    SendMessage(hWnd, WM_CHAR, TEXT('~'), lParam);
                    break;

                case ID_CALC_AND:
                    SendMessage(hWnd, WM_CHAR, TEXT('&'), lParam);
                    break;

                case ID_CALC_OR:
                    SendMessage(hWnd, WM_CHAR, TEXT('|'), lParam);
                    break;

                case ID_CALC_XOR:
                    SendMessage(hWnd, WM_CHAR, TEXT('^'), lParam);
                    break;

                case ID_CALC_INT:
                    SendMessage(hWnd, WM_CHAR, TEXT(';'), lParam);
                    break;

                case ID_CALC_FACTORIAL:
                    SendMessage(hWnd, WM_CHAR, TEXT('!'), lParam);
                    break;

                case ID_CALC_RECIPROCAL:
                    SendMessage(hWnd, WM_CHAR, TEXT('r'), lParam);
                    break;

                case ID_CALC_SIGN:
                    SendMessage(hWnd, WM_KEYDOWN, VK_F9, lParam);
                    break;

                case ID_CALC_PLUS:
                    SendMessage(hWnd, WM_CHAR, TEXT('+'), lParam);
                    break;

                case ID_CALC_MINUS:
                    SendMessage(hWnd, WM_CHAR, TEXT('-'), lParam);
                    break;

                case ID_CALC_MULTIPLY:
                    SendMessage(hWnd, WM_CHAR, TEXT('*'), lParam);
                    break;

                case ID_CALC_DIVIDE:
                    SendMessage(hWnd, WM_CHAR, TEXT('/'), lParam);
                    break;

                case ID_CALC_EQUALS:
                    SendMessage(hWnd, WM_CHAR, TEXT('='), lParam);
                    break;

                case ID_CALC_PERCENT:
                    SendMessage(hWnd, WM_CHAR, TEXT('%'), lParam);
                    break;

                case ID_CALC_EXP:
                    SendMessage(hWnd, WM_CHAR, TEXT('x'), lParam);
                    break;

                case ID_CALC_FE:
                    SendMessage(hWnd, WM_CHAR, TEXT('v'), lParam);
                    break;

                case ID_CALC_LEFTPAREN:
                    SendMessage(hWnd, WM_CHAR, TEXT('('), lParam);
                    break;

                case ID_CALC_RIGHTPAREN:
                    SendMessage(hWnd, WM_CHAR, TEXT(')'), lParam);
                    break;

                case ID_CALC_MOD:
                    SendMessage(hWnd, WM_CHAR, TEXT('%'), lParam);
                    break;

                case ID_CALC_DAT:
                    SendMessage(hWnd, WM_KEYDOWN, VK_INSERT, lParam);
                    break;

                case ID_CALC_AVE:
                    SendMessage(hWnd, WM_CHAR, TEXT('\x01'), lParam); // Ctrl+A
                    break;

                case ID_CALC_S:
                    SendMessage(hWnd, WM_CHAR, TEXT('\x04'), lParam); // Ctrl+D
                    break;

                case ID_CALC_STA:
                    SendMessage(hWnd, WM_CHAR, TEXT('\x13'), lParam); // Ctrl+S
                    break;

                case ID_CALC_SUM:
                    SendMessage(hWnd, WM_CHAR, TEXT('\x14'), lParam); // Ctrl+T
                    break;

                case ID_CALC_DMS:
                    SendMessage(hWnd, WM_CHAR, TEXT('m'), lParam);
                    break;

                default:
                    break;

                } // button message switch


                SetFocus(hWnd);

                if (debug)
                   show_debug(&calc, TEXT("After WM_CHAR"), wParam, lParam);

                return 0;

            }   // if BN_CLICKED
        }       // WM_COMMAND switch
    }           // Main Message switch
    return( DefWindowProc( hWnd, msg, wParam, lParam ));
}               // MainProc

void InitLuts(void)
{
    int i;

    // initialize keys lut for validating keystrokes in various number bases

    for (i=TEXT('0');i<=TEXT('9');i++) {
        keys[NBASE_HEX][i]       = 1;
        keys[NBASE_DECIMAL][i]   = 1;

        if (i <= TEXT('7'))
            keys[NBASE_OCTAL][i]  = 1;

        if (i <= TEXT('1'))
            keys[NBASE_BINARY][i] = 1;
    }

    for (i=TEXT('a');i<=TEXT('f');i++)
        keys[NBASE_HEX][i] = 1;

    for (i=TEXT('A');i<=TEXT('F');i++)
        keys[NBASE_HEX][i] = 1;
}

void InitMenus(HINSTANCE hInst)
{
    if (!(menus[MENU_STD]   = LoadMenu(hInst,TEXT("MAIN_MENU"))))
        exit(1);

    if (!(menus[MENU_SCIMS] = LoadMenu(hInst,TEXT("SCIMS_MENU"))))
        exit(1);

    if (!(menus[MENU_SCIWS] = LoadMenu(hInst,TEXT("SCIWS_MENU"))))
        exit(1);

    CheckMenuItem(menus[MENU_STD], IDM_MODE_STANDARD, MF_CHECKED);
    if (calc.digitGrouping) {
       CheckMenuItem(menus[MENU_STD],   IDM_DIGIT_GROUPING, MF_CHECKED);
       CheckMenuItem(menus[MENU_SCIMS], IDM_DIGIT_GROUPING, MF_CHECKED);
       CheckMenuItem(menus[MENU_SCIWS], IDM_DIGIT_GROUPING, MF_CHECKED);
    }

    calc_setmenuitem_radio(menus[MENU_STD], IDM_MODE_STANDARD);
    calc_setmenuitem_radio(menus[MENU_STD], IDM_MODE_SCIENTIFIC);


    calc_setmenuitem_radio(menus[MENU_SCIMS], IDM_MODE_STANDARD);
    calc_setmenuitem_radio(menus[MENU_SCIMS], IDM_MODE_SCIENTIFIC);

    calc_setmenuitem_radio(menus[MENU_SCIMS], ID_CALC_NS_HEX);
    calc_setmenuitem_radio(menus[MENU_SCIMS], ID_CALC_NS_DEC);
    calc_setmenuitem_radio(menus[MENU_SCIMS], ID_CALC_NS_OCT);
    calc_setmenuitem_radio(menus[MENU_SCIMS], ID_CALC_NS_BIN);

    calc_setmenuitem_radio(menus[MENU_SCIMS], ID_CALC_MS_DEGREES);
    calc_setmenuitem_radio(menus[MENU_SCIMS], ID_CALC_MS_RADIANS);
    calc_setmenuitem_radio(menus[MENU_SCIMS], ID_CALC_MS_GRADS);
    CheckMenuItem(menus[MENU_SCIMS], IDM_MODE_SCIENTIFIC, MF_CHECKED);
    CheckMenuItem(menus[MENU_SCIMS], ID_CALC_NS_DEC,      MF_CHECKED);
    CheckMenuItem(menus[MENU_SCIMS], ID_CALC_MS_DEGREES,  MF_CHECKED);


    calc_setmenuitem_radio(menus[MENU_SCIWS], IDM_MODE_STANDARD);
    calc_setmenuitem_radio(menus[MENU_SCIWS], IDM_MODE_SCIENTIFIC);

    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_NS_HEX);
    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_NS_DEC);
    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_NS_OCT);
    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_NS_BIN);

    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_WS_QWORD);
    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_WS_DWORD);
    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_WS_WORD);
    calc_setmenuitem_radio(menus[MENU_SCIWS], ID_CALC_WS_BYTE);
    CheckMenuItem(menus[MENU_SCIWS], IDM_MODE_SCIENTIFIC, MF_CHECKED);
    CheckMenuItem(menus[MENU_SCIWS], ID_CALC_NS_DEC,      MF_CHECKED);
    CheckMenuItem(menus[MENU_SCIWS], ID_CALC_WS_QWORD,    MF_CHECKED);
}

void InitCalc (CALC *calc)
{
    int n;
    int skipx;
    int skipy;
    int top_button;

    HINSTANCE hInst = calc->hInst;
    HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);

    calc->trigMode = TRIGMODE_DEGREES;
    calc->numBase  = NBASE_DECIMAL;
    calc->init     = 1;

    if (calc->new) {
        calc->new           = 0;
        calc->value         = 0;
        calc->memory        = 0;
        calc->displayMode   = 0;
        calc->buffer[0]     = TEXT('\0');
        _tcscpy(calc->display, TEXT("0."));

        calc->err           = 0;
        calc->next          = TEXT('\0');

        calc->wordSize      = WORDSIZE_QWORD;
        calc->invMode       = 0;
        calc->hypMode       = 0;
    }

    if (calc->sciMode) {
        calc->numButtons = CALC_BUTTONS_STANDARD;

        // Row 1 (top)

        skipx = MARGIN_LEFT;
        skipy = CALC_STANDARD_MARGIN_TOP + CALC_EDIT_HEIGHT;

        calc->cb[0].id       = 0;
        _tcscpy(calc->cb[0].label,TEXT("FILLER"));
        calc->cb[0].color    = CALC_COLOR_BLUE;
        calc->cb[0].r.left   = skipx + 4;
        calc->cb[0].r.top    = skipy + 2;
        calc->cb[0].r.right  = skipx + SZ_FILLER_X - 2;
        calc->cb[0].r.bottom = skipy + SZ_FILLER_Y - 2;
        calc->cb[0].enable   = 1;

        skipx = SZ_FILLER_X + MARGIN_STANDARD_BIG_X + 11;

        calc->cb[1].id       = ID_CALC_BACKSPACE;
        LoadString( hInst, IDS_BTN_BACKSPACE, calc->cb[1].label, sizeof(calc->cb[1].label) / sizeof(calc->cb[1].label[0]));
        calc->cb[1].color    = CALC_COLOR_RED;
        calc->cb[1].r.left   = skipx;
        calc->cb[1].r.top    = skipy;
        calc->cb[1].r.right  = SZ_BIGBTN_X;
        calc->cb[1].r.bottom = SZ_BIGBTN_Y;
        calc->cb[1].enable   = 1;

        skipx += SZ_BIGBTN_X + MARGIN_SMALL_X;

        calc->cb[2].id       = ID_CALC_CLEAR_ENTRY;
        LoadString( hInst, IDS_BTN_CLEAR_ENTRY, calc->cb[2].label, sizeof(calc->cb[2].label) / sizeof(calc->cb[2].label[0]));
        calc->cb[2].color    = CALC_COLOR_RED;
        calc->cb[2].r.left   = skipx;
        calc->cb[2].r.top    = skipy;
        calc->cb[2].r.right  = SZ_BIGBTN_X;
        calc->cb[2].r.bottom = SZ_BIGBTN_Y;
        calc->cb[2].enable   = 1;

        skipx += SZ_BIGBTN_X + MARGIN_SMALL_X;

        calc->cb[3].id       = ID_CALC_CLEAR_ALL;
        LoadString( hInst, IDS_BTN_CLEAR_ALL, calc->cb[3].label, sizeof(calc->cb[3].label) / sizeof(calc->cb[3].label[0]));
        calc->cb[3].color    = CALC_COLOR_RED;
        calc->cb[3].r.left   = skipx;
        calc->cb[3].r.top    = skipy;
        calc->cb[3].r.right  = SZ_BIGBTN_X;
        calc->cb[3].r.bottom = SZ_BIGBTN_Y;
        calc->cb[3].enable   = 1;

        // Row 2

        skipx = MARGIN_LEFT;
        skipy += SZ_BIGBTN_Y + MARGIN_BIG_Y;

        calc->cb[4].id       = ID_CALC_MEM_CLEAR;
        LoadString( hInst, IDS_BTN_MEM_CLEAR, calc->cb[4].label, sizeof(calc->cb[4].label) / sizeof(calc->cb[4].label[0]));
        calc->cb[4].color    = CALC_COLOR_RED;
        calc->cb[4].r.left   = skipx;
        calc->cb[4].r.top    = skipy;
        calc->cb[4].r.right  = SZ_MEDBTN_X;
        calc->cb[4].r.bottom = SZ_MEDBTN_Y;
        calc->cb[4].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_STANDARD_BIG_X;

        calc->cb[5].id       = ID_CALC_SEVEN;
        _tcscpy(calc->cb[5].label,TEXT("7"));
        calc->cb[5].color    = CALC_COLOR_BLUE;
        calc->cb[5].r.left   = skipx;
        calc->cb[5].r.top    = skipy;
        calc->cb[5].r.right  = SZ_MEDBTN_X;
        calc->cb[5].r.bottom = SZ_MEDBTN_Y;
        calc->cb[5].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[6].id       = ID_CALC_EIGHT;
        _tcscpy(calc->cb[6].label,TEXT("8"));
        calc->cb[6].color    = CALC_COLOR_BLUE;
        calc->cb[6].r.left   = skipx;
        calc->cb[6].r.top    = skipy;
        calc->cb[6].r.right  = SZ_MEDBTN_X;
        calc->cb[6].r.bottom = SZ_MEDBTN_Y;
        calc->cb[6].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[7].id       = ID_CALC_NINE;
        _tcscpy(calc->cb[7].label,TEXT("9"));
        calc->cb[7].color    = CALC_COLOR_BLUE;
        calc->cb[7].r.left   = skipx;
        calc->cb[7].r.top    = skipy;
        calc->cb[7].r.right  = SZ_MEDBTN_X;
        calc->cb[7].r.bottom = SZ_MEDBTN_Y;
        calc->cb[7].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[8].id       = ID_CALC_DIVIDE;
        _tcscpy(calc->cb[8].label,TEXT("/"));
        calc->cb[8].color    = CALC_COLOR_RED;
        calc->cb[8].r.left   = skipx;
        calc->cb[8].r.top    = skipy;
        calc->cb[8].r.right  = SZ_MEDBTN_X;
        calc->cb[8].r.bottom = SZ_MEDBTN_Y;
        calc->cb[8].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[9].id       = ID_CALC_SQRT;
        LoadString( hInst, IDS_BTN_SQRT, calc->cb[9].label, sizeof(calc->cb[9].label) / sizeof(calc->cb[9].label[0]));
        calc->cb[9].color    = CALC_COLOR_BLUE;
        calc->cb[9].r.left   = skipx;
        calc->cb[9].r.top    = skipy;
        calc->cb[9].r.right  = SZ_MEDBTN_X;
        calc->cb[9].r.bottom = SZ_MEDBTN_Y;
        calc->cb[9].enable   = 1;

        // Row 3

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[10].id       = ID_CALC_MEM_RECALL;
        LoadString( hInst, IDS_BTN_MEM_RECALL, calc->cb[10].label, sizeof(calc->cb[10].label) / sizeof(calc->cb[10].label[0]));
        calc->cb[10].color    = CALC_COLOR_RED;
        calc->cb[10].r.left   = skipx;
        calc->cb[10].r.top    = skipy;
        calc->cb[10].r.right  = SZ_MEDBTN_X;
        calc->cb[10].r.bottom = SZ_MEDBTN_Y;
        calc->cb[10].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_STANDARD_BIG_X;

        calc->cb[11].id       = ID_CALC_FOUR;
        _tcscpy(calc->cb[11].label,TEXT("4"));
        calc->cb[11].color    = CALC_COLOR_BLUE;
        calc->cb[11].r.left   = skipx;
        calc->cb[11].r.top    = skipy;
        calc->cb[11].r.right  = SZ_MEDBTN_X;
        calc->cb[11].r.bottom = SZ_MEDBTN_Y;
        calc->cb[11].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[12].id       = ID_CALC_FIVE;
        _tcscpy(calc->cb[12].label,TEXT("5"));
        calc->cb[12].color    = CALC_COLOR_BLUE;
        calc->cb[12].r.left   = skipx;
        calc->cb[12].r.top    = skipy;
        calc->cb[12].r.right  = SZ_MEDBTN_X;
        calc->cb[12].r.bottom = SZ_MEDBTN_Y;
        calc->cb[12].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[13].id       = ID_CALC_SIX;
        _tcscpy(calc->cb[13].label,TEXT("6"));
        calc->cb[13].color    = CALC_COLOR_BLUE;
        calc->cb[13].r.left   = skipx;
        calc->cb[13].r.top    = skipy;
        calc->cb[13].r.right  = SZ_MEDBTN_X;
        calc->cb[13].r.bottom = SZ_MEDBTN_Y;
        calc->cb[13].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[14].id       = ID_CALC_MULTIPLY;
        _tcscpy(calc->cb[14].label,TEXT("*"));
        calc->cb[14].color    = CALC_COLOR_RED;
        calc->cb[14].r.left   = skipx;
        calc->cb[14].r.top    = skipy;
        calc->cb[14].r.right  = SZ_MEDBTN_X;
        calc->cb[14].r.bottom = SZ_MEDBTN_Y;
        calc->cb[14].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[15].id       = ID_CALC_PERCENT;
        _tcscpy(calc->cb[15].label,TEXT("%"));
        calc->cb[15].color    = CALC_COLOR_BLUE;
        calc->cb[15].r.left   = skipx;
        calc->cb[15].r.top    = skipy;
        calc->cb[15].r.right  = SZ_MEDBTN_X;
        calc->cb[15].r.bottom = SZ_MEDBTN_Y;
        calc->cb[15].enable   = 1;

        // Row 4

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[16].id       = ID_CALC_MEM_STORE;
        LoadString( hInst, IDS_BTN_MEM_STORE, calc->cb[16].label, sizeof(calc->cb[16].label) / sizeof(calc->cb[16].label[0]));
        calc->cb[16].color    = CALC_COLOR_RED;
        calc->cb[16].r.left   = skipx;
        calc->cb[16].r.top    = skipy;
        calc->cb[16].r.right  = SZ_MEDBTN_X;
        calc->cb[16].r.bottom = SZ_MEDBTN_Y;
        calc->cb[16].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_STANDARD_BIG_X;

        calc->cb[17].id       = ID_CALC_ONE;
        _tcscpy(calc->cb[17].label,TEXT("1"));
        calc->cb[17].color    = CALC_COLOR_BLUE;
        calc->cb[17].r.left   = skipx;
        calc->cb[17].r.top    = skipy;
        calc->cb[17].r.right  = SZ_MEDBTN_X;
        calc->cb[17].r.bottom = SZ_MEDBTN_Y;
        calc->cb[17].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[18].id       = ID_CALC_TWO;
        _tcscpy(calc->cb[18].label,TEXT("2"));
        calc->cb[18].color    = CALC_COLOR_BLUE;
        calc->cb[18].r.left   = skipx;
        calc->cb[18].r.top    = skipy;
        calc->cb[18].r.right  = SZ_MEDBTN_X;
        calc->cb[18].r.bottom = SZ_MEDBTN_Y;
        calc->cb[18].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[19].id       = ID_CALC_THREE;
        _tcscpy(calc->cb[19].label,TEXT("3"));
        calc->cb[19].color    = CALC_COLOR_BLUE;
        calc->cb[19].r.left   = skipx;
        calc->cb[19].r.top    = skipy;
        calc->cb[19].r.right  = SZ_MEDBTN_X;
        calc->cb[19].r.bottom = SZ_MEDBTN_Y;
        calc->cb[19].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[20].id       = ID_CALC_MINUS;
        _tcscpy(calc->cb[20].label,TEXT("-"));
        calc->cb[20].color    = CALC_COLOR_RED;
        calc->cb[20].r.left   = skipx;
        calc->cb[20].r.top    = skipy;
        calc->cb[20].r.right  = SZ_MEDBTN_X;
        calc->cb[20].r.bottom = SZ_MEDBTN_Y;
        calc->cb[20].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[21].id       = ID_CALC_RECIPROCAL;
        _tcscpy(calc->cb[21].label,TEXT("1/x"));
        calc->cb[21].color    = CALC_COLOR_RED;
        calc->cb[21].r.left   = skipx;
        calc->cb[21].r.top    = skipy;
        calc->cb[21].r.right  = SZ_MEDBTN_X;
        calc->cb[21].r.bottom = SZ_MEDBTN_Y;
        calc->cb[21].enable   = 1;

        // Row 5 (bottom)

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[22].id       = ID_CALC_MEM_PLUS;
        LoadString( hInst, IDS_BTN_MEM_PLUS, calc->cb[22].label, sizeof(calc->cb[22].label) / sizeof(calc->cb[22].label[0]));
        calc->cb[22].color    = CALC_COLOR_RED;
        calc->cb[22].r.left   = skipx;
        calc->cb[22].r.top    = skipy;
        calc->cb[22].r.right  = SZ_MEDBTN_X;
        calc->cb[22].r.bottom = SZ_MEDBTN_Y;
        calc->cb[22].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_STANDARD_BIG_X;

        calc->cb[23].id       = ID_CALC_ZERO;
        _tcscpy(calc->cb[23].label,TEXT("0"));
        calc->cb[23].color    = CALC_COLOR_BLUE;
        calc->cb[23].r.left   = skipx;
        calc->cb[23].r.top    = skipy;
        calc->cb[23].r.right  = SZ_MEDBTN_X;
        calc->cb[23].r.bottom = SZ_MEDBTN_Y;
        calc->cb[23].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[24].id       = ID_CALC_SIGN;
        _tcscpy(calc->cb[24].label,TEXT("+/-"));
        calc->cb[24].color    = CALC_COLOR_RED;
        calc->cb[24].r.left   = skipx;
        calc->cb[24].r.top    = skipy;
        calc->cb[24].r.right  = SZ_MEDBTN_X;
        calc->cb[24].r.bottom = SZ_MEDBTN_Y;
        calc->cb[24].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[25].id       = ID_CALC_DECIMAL;
        _tcscpy(calc->cb[25].label,TEXT("."));
        calc->cb[25].color    = CALC_COLOR_BLUE;
        calc->cb[25].r.left   = skipx;
        calc->cb[25].r.top    = skipy;
        calc->cb[25].r.right  = SZ_MEDBTN_X;
        calc->cb[25].r.bottom = SZ_MEDBTN_Y;
        calc->cb[25].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[26].id       = ID_CALC_PLUS;
        _tcscpy(calc->cb[26].label,TEXT("+"));
        calc->cb[26].color    = CALC_COLOR_RED;
        calc->cb[26].r.left   = skipx;
        calc->cb[26].r.top    = skipy;
        calc->cb[26].r.right  = SZ_MEDBTN_X;
        calc->cb[26].r.bottom = SZ_MEDBTN_Y;
        calc->cb[26].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[27].id       = ID_CALC_EQUALS;
        _tcscpy(calc->cb[27].label,TEXT("="));
        calc->cb[27].color    = CALC_COLOR_RED;
        calc->cb[27].r.left   = skipx;
        calc->cb[27].r.top    = skipy;
        calc->cb[27].r.right  = SZ_MEDBTN_X;
        calc->cb[27].r.bottom = SZ_MEDBTN_Y;
        calc->cb[27].enable   = 1;
    }
    else {
        calc->numButtons = CALC_BUTTONS_SCIENTIFIC;

        // Row 1 (top)

        skipx = MARGIN_LEFT;
        skipy = CALC_SCIENTIFIC_MARGIN_TOP + CALC_EDIT_HEIGHT - 1;

        calc->cb[0].id       = 0;
        _tcscpy(calc->cb[0].label,TEXT("FILLER"));
        calc->cb[0].color    = CALC_COLOR_BLUE;
        calc->cb[0].r.left   = skipx + 4 * SZ_MEDBTN_X + 2 * SZ_SPACER_X + 2 * MARGIN_SMALL_X + 12;
        calc->cb[0].r.top    = skipy;
        calc->cb[0].r.right  = skipx + 4 * SZ_MEDBTN_X + 2 * SZ_SPACER_X + 2 * MARGIN_SMALL_X + SZ_FILLER_X + 4;
        calc->cb[0].r.bottom = skipy + SZ_FILLER_Y - 6;
        calc->cb[0].enable   = 1;

        calc->cb[1].id       = 0;
        _tcscpy(calc->cb[1].label,TEXT("FILLER"));
        calc->cb[1].color    = CALC_COLOR_BLUE;
        calc->cb[1].r.left   = skipx + 3 * SZ_MEDBTN_X +     SZ_SPACER_X + 2 * MARGIN_SMALL_X + 8;
        calc->cb[1].r.top    = skipy;
        calc->cb[1].r.right  = skipx + 3 * SZ_MEDBTN_X +     SZ_SPACER_X + 2 * MARGIN_SMALL_X + SZ_FILLER_X + 0;
        calc->cb[1].r.bottom = skipy + SZ_FILLER_Y - 6;
        calc->cb[1].enable   = 1;

        skipx += SZ_FILLER_X + MARGIN_SMALL_X;

        skipx = MARGIN_BIG_X;

        calc->cb[2].id       = ID_CALC_BACKSPACE;
        LoadString( hInst, IDS_BTN_BACKSPACE, calc->cb[2].label, sizeof(calc->cb[2].label) / sizeof(calc->cb[2].label[0]));
        calc->cb[2].color    = CALC_COLOR_RED;
        calc->cb[2].r.left   = skipx;
        calc->cb[2].r.top    = skipy;
        calc->cb[2].r.right  = SZ_BIGBTN_X;
        calc->cb[2].r.bottom = SZ_BIGBTN_Y;
        calc->cb[2].enable   = 1;

        skipx += SZ_BIGBTN_X + MARGIN_SMALL_X;

        calc->cb[3].id       = ID_CALC_CLEAR_ENTRY;
        LoadString( hInst, IDS_BTN_CLEAR_ENTRY, calc->cb[3].label, sizeof(calc->cb[3].label) / sizeof(calc->cb[3].label[0]));
        calc->cb[3].color    = CALC_COLOR_RED;
        calc->cb[3].r.left   = skipx;
        calc->cb[3].r.top    = skipy;
        calc->cb[3].r.right  = SZ_BIGBTN_X;
        calc->cb[3].r.bottom = SZ_BIGBTN_Y;
        calc->cb[3].enable   = 1;

        skipx += SZ_BIGBTN_X + MARGIN_SMALL_X;

        calc->cb[4].id       = ID_CALC_CLEAR_ALL;
        LoadString( hInst, IDS_BTN_CLEAR_ALL, calc->cb[4].label, sizeof(calc->cb[4].label) / sizeof(calc->cb[4].label[0]));
        calc->cb[4].color    = CALC_COLOR_RED;
        calc->cb[4].r.left   = skipx;
        calc->cb[4].r.top    = skipy;
        calc->cb[4].r.right  = SZ_BIGBTN_X;
        calc->cb[4].r.bottom = SZ_BIGBTN_Y;
        calc->cb[4].enable   = 1;

        // Row 2

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[5].id       = ID_CALC_STA;
        _tcscpy(calc->cb[5].label,TEXT("Sta"));
        calc->cb[5].color    = CALC_COLOR_GRAY;
        calc->cb[5].r.left   = skipx;
        calc->cb[5].r.top    = skipy;
        calc->cb[5].r.right  = SZ_MEDBTN_X;
        calc->cb[5].r.bottom = SZ_MEDBTN_Y;
        calc->cb[5].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[6].id       = ID_CALC_FE;
        _tcscpy(calc->cb[6].label,TEXT("F-E"));
        calc->cb[6].color    = CALC_COLOR_MAGENTA;
        calc->cb[6].r.left   = skipx;
        calc->cb[6].r.top    = skipy;
        calc->cb[6].r.right  = SZ_MEDBTN_X;
        calc->cb[6].r.bottom = SZ_MEDBTN_Y;
        calc->cb[6].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[7].id       = ID_CALC_LEFTPAREN;
        _tcscpy(calc->cb[7].label,TEXT("("));
        calc->cb[7].color    = CALC_COLOR_MAGENTA;
        calc->cb[7].r.left   = skipx;
        calc->cb[7].r.top    = skipy;
        calc->cb[7].r.right  = SZ_MEDBTN_X;
        calc->cb[7].r.bottom = SZ_MEDBTN_Y;
        calc->cb[7].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[8].id       = ID_CALC_RIGHTPAREN;
        _tcscpy(calc->cb[8].label,TEXT(")"));
        calc->cb[8].color    = CALC_COLOR_MAGENTA;
        calc->cb[8].r.left   = skipx;
        calc->cb[8].r.top    = skipy;
        calc->cb[8].r.right  = SZ_MEDBTN_X;
        calc->cb[8].r.bottom = SZ_MEDBTN_Y;
        calc->cb[8].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[9].id       = ID_CALC_MEM_CLEAR;
        LoadString( hInst, IDS_BTN_MEM_CLEAR, calc->cb[9].label, sizeof(calc->cb[9].label) / sizeof(calc->cb[9].label[0]));
        calc->cb[9].color    = CALC_COLOR_RED;
        calc->cb[9].r.left   = skipx;
        calc->cb[9].r.top    = skipy;
        calc->cb[9].r.right  = SZ_MEDBTN_X;
        calc->cb[9].r.bottom = SZ_MEDBTN_Y;
        calc->cb[9].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[10].id       = ID_CALC_SEVEN;
        _tcscpy(calc->cb[10].label,TEXT("7"));
        calc->cb[10].color    = CALC_COLOR_BLUE;
        calc->cb[10].r.left   = skipx;
        calc->cb[10].r.top    = skipy;
        calc->cb[10].r.right  = SZ_MEDBTN_X;
        calc->cb[10].r.bottom = SZ_MEDBTN_Y;
        calc->cb[10].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[11].id       = ID_CALC_EIGHT;
        _tcscpy(calc->cb[11].label,TEXT("8"));
        calc->cb[11].color    = CALC_COLOR_BLUE;
        calc->cb[11].r.left   = skipx;
        calc->cb[11].r.top    = skipy;
        calc->cb[11].r.right  = SZ_MEDBTN_X;
        calc->cb[11].r.bottom = SZ_MEDBTN_Y;
        calc->cb[11].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[12].id       = ID_CALC_NINE;
        _tcscpy(calc->cb[12].label,TEXT("9"));
        calc->cb[12].color    = CALC_COLOR_BLUE;
        calc->cb[12].r.left   = skipx;
        calc->cb[12].r.top    = skipy;
        calc->cb[12].r.right  = SZ_MEDBTN_X;
        calc->cb[12].r.bottom = SZ_MEDBTN_Y;
        calc->cb[12].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[13].id       = ID_CALC_DIVIDE;
        _tcscpy(calc->cb[13].label,TEXT("/"));
        calc->cb[13].color    = CALC_COLOR_RED;
        calc->cb[13].r.left   = skipx;
        calc->cb[13].r.top    = skipy;
        calc->cb[13].r.right  = SZ_MEDBTN_X;
        calc->cb[13].r.bottom = SZ_MEDBTN_Y;
        calc->cb[13].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[14].id       = ID_CALC_MOD;
        _tcscpy(calc->cb[14].label,TEXT("Mod"));
        calc->cb[14].color    = CALC_COLOR_RED;
        calc->cb[14].r.left   = skipx;
        calc->cb[14].r.top    = skipy;
        calc->cb[14].r.right  = SZ_MEDBTN_X;
        calc->cb[14].r.bottom = SZ_MEDBTN_Y;
        calc->cb[14].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[15].id       = ID_CALC_AND;
        _tcscpy(calc->cb[15].label,TEXT("And"));
        calc->cb[15].color    = CALC_COLOR_RED;
        calc->cb[15].r.left   = skipx;
        calc->cb[15].r.top    = skipy;
        calc->cb[15].r.right  = SZ_MEDBTN_X;
        calc->cb[15].r.bottom = SZ_MEDBTN_Y;
        calc->cb[15].enable   = 1;

        // Row 3

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[16].id       = ID_CALC_AVE;
        _tcscpy(calc->cb[16].label,TEXT("Ave"));
        calc->cb[16].color    = CALC_COLOR_GRAY;
        calc->cb[16].r.left   = skipx;
        calc->cb[16].r.top    = skipy;
        calc->cb[16].r.right  = SZ_MEDBTN_X;
        calc->cb[16].r.bottom = SZ_MEDBTN_Y;
        calc->cb[16].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[17].id       = ID_CALC_DMS;
        _tcscpy(calc->cb[17].label,TEXT("dms"));
        calc->cb[17].color    = CALC_COLOR_MAGENTA;
        calc->cb[17].r.left   = skipx;
        calc->cb[17].r.top    = skipy;
        calc->cb[17].r.right  = SZ_MEDBTN_X;
        calc->cb[17].r.bottom = SZ_MEDBTN_Y;
        calc->cb[17].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[18].id       = ID_CALC_EXP;
        _tcscpy(calc->cb[18].label,TEXT("Exp"));
        calc->cb[18].color    = CALC_COLOR_MAGENTA;
        calc->cb[18].r.left   = skipx;
        calc->cb[18].r.top    = skipy;
        calc->cb[18].r.right  = SZ_MEDBTN_X;
        calc->cb[18].r.bottom = SZ_MEDBTN_Y;
        calc->cb[18].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[19].id       = ID_CALC_LN;
        _tcscpy(calc->cb[19].label,TEXT("ln"));
        calc->cb[19].color    = CALC_COLOR_MAGENTA;
        calc->cb[19].r.left   = skipx;
        calc->cb[19].r.top    = skipy;
        calc->cb[19].r.right  = SZ_MEDBTN_X;
        calc->cb[19].r.bottom = SZ_MEDBTN_Y;
        calc->cb[19].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[20].id       = ID_CALC_MEM_RECALL;
        _tcscpy(calc->cb[20].label,TEXT("MR"));
        calc->cb[20].color    = CALC_COLOR_RED;
        calc->cb[20].r.left   = skipx;
        calc->cb[20].r.top    = skipy;
        calc->cb[20].r.right  = SZ_MEDBTN_X;
        calc->cb[20].r.bottom = SZ_MEDBTN_Y;
        calc->cb[20].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[21].id       = ID_CALC_FOUR;
        _tcscpy(calc->cb[21].label,TEXT("4"));
        calc->cb[21].color    = CALC_COLOR_BLUE;
        calc->cb[21].r.left   = skipx;
        calc->cb[21].r.top    = skipy;
        calc->cb[21].r.right  = SZ_MEDBTN_X;
        calc->cb[21].r.bottom = SZ_MEDBTN_Y;
        calc->cb[21].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[22].id       = ID_CALC_FIVE;
        _tcscpy(calc->cb[22].label,TEXT("5"));
        calc->cb[22].color    = CALC_COLOR_BLUE;
        calc->cb[22].r.left   = skipx;
        calc->cb[22].r.top    = skipy;
        calc->cb[22].r.right  = SZ_MEDBTN_X;
        calc->cb[22].r.bottom = SZ_MEDBTN_Y;
        calc->cb[22].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[23].id       = ID_CALC_SIX;
        _tcscpy(calc->cb[23].label,TEXT("6"));
        calc->cb[23].color    = CALC_COLOR_BLUE;
        calc->cb[23].r.left   = skipx;
        calc->cb[23].r.top    = skipy;
        calc->cb[23].r.right  = SZ_MEDBTN_X;
        calc->cb[23].r.bottom = SZ_MEDBTN_Y;
        calc->cb[23].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[24].id       = ID_CALC_MULTIPLY;
        _tcscpy(calc->cb[24].label,TEXT("*"));
        calc->cb[24].color    = CALC_COLOR_RED;
        calc->cb[24].r.left   = skipx;
        calc->cb[24].r.top    = skipy;
        calc->cb[24].r.right  = SZ_MEDBTN_X;
        calc->cb[24].r.bottom = SZ_MEDBTN_Y;
        calc->cb[24].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[25].id       = ID_CALC_OR;
        _tcscpy(calc->cb[25].label,TEXT("Or"));
        calc->cb[25].color    = CALC_COLOR_RED;
        calc->cb[25].r.left   = skipx;
        calc->cb[25].r.top    = skipy;
        calc->cb[25].r.right  = SZ_MEDBTN_X;
        calc->cb[25].r.bottom = SZ_MEDBTN_Y;
        calc->cb[25].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[26].id       = ID_CALC_XOR;
        _tcscpy(calc->cb[26].label,TEXT("Xor"));
        calc->cb[26].color    = CALC_COLOR_RED;
        calc->cb[26].r.left   = skipx;
        calc->cb[26].r.top    = skipy;
        calc->cb[26].r.right  = SZ_MEDBTN_X;
        calc->cb[26].r.bottom = SZ_MEDBTN_Y;
        calc->cb[26].enable   = 1;

        // Row 4

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[27].id       = ID_CALC_SUM;
        _tcscpy(calc->cb[27].label,TEXT("Sum"));
        calc->cb[27].color    = CALC_COLOR_GRAY;
        calc->cb[27].r.left   = skipx ;
        calc->cb[27].r.top    = skipy;
        calc->cb[27].r.right  = SZ_MEDBTN_X;
        calc->cb[27].r.bottom = SZ_MEDBTN_Y;
        calc->cb[27].enable   = 0;

        skipx += MARGIN_SMALL_X + SZ_MEDBTN_X + SZ_SPACER_X;

        calc->cb[28].id       = ID_CALC_SIN;
        _tcscpy(calc->cb[28].label,TEXT("sin"));
        calc->cb[28].color    = CALC_COLOR_MAGENTA;
        calc->cb[28].r.left   = skipx;
        calc->cb[28].r.top    = skipy;
        calc->cb[28].r.right  = SZ_MEDBTN_X;
        calc->cb[28].r.bottom = SZ_MEDBTN_Y;
        calc->cb[28].enable   = 1;

        skipx += MARGIN_SMALL_X + SZ_MEDBTN_X;

        calc->cb[29].id       = ID_CALC_POWER	;
        _tcscpy(calc->cb[29].label,TEXT("x^y"));
        calc->cb[29].color    = CALC_COLOR_MAGENTA;
        calc->cb[29].r.left   = skipx;
        calc->cb[29].r.top    = skipy;
        calc->cb[29].r.right  = SZ_MEDBTN_X;
        calc->cb[29].r.bottom = SZ_MEDBTN_Y;
        calc->cb[29].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[30].id       = ID_CALC_LOG10;
        _tcscpy(calc->cb[30].label,TEXT("log"));
        calc->cb[30].color    = CALC_COLOR_MAGENTA;
        calc->cb[30].r.left   = skipx;
        calc->cb[30].r.top    = skipy;
        calc->cb[30].r.right  = SZ_MEDBTN_X;
        calc->cb[30].r.bottom = SZ_MEDBTN_Y;
        calc->cb[30].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[31].id       = ID_CALC_MEM_STORE;
        LoadString( hInst, IDS_BTN_MEM_STORE, calc->cb[31].label, sizeof(calc->cb[31].label) / sizeof(calc->cb[31].label[0]));
        calc->cb[31].color    = CALC_COLOR_RED;
        calc->cb[31].r.left   = skipx;
        calc->cb[31].r.top    = skipy;
        calc->cb[31].r.right  = SZ_MEDBTN_X;
        calc->cb[31].r.bottom = SZ_MEDBTN_Y;
        calc->cb[31].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[32].id       = ID_CALC_ONE;
        _tcscpy(calc->cb[32].label,TEXT("1"));
        calc->cb[32].color    = CALC_COLOR_BLUE;
        calc->cb[32].r.left   = skipx;
        calc->cb[32].r.top    = skipy;
        calc->cb[32].r.right  = SZ_MEDBTN_X;
        calc->cb[32].r.bottom = SZ_MEDBTN_Y;
        calc->cb[32].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[33].id       = ID_CALC_TWO;
        _tcscpy(calc->cb[33].label,TEXT("2"));
        calc->cb[33].color    = CALC_COLOR_BLUE;
        calc->cb[33].r.left   = skipx;
        calc->cb[33].r.top    = skipy;
        calc->cb[33].r.right  = SZ_MEDBTN_X;
        calc->cb[33].r.bottom = SZ_MEDBTN_Y;
        calc->cb[33].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[34].id       = ID_CALC_THREE;
        _tcscpy(calc->cb[34].label,TEXT("3"));
        calc->cb[34].color    = CALC_COLOR_BLUE;
        calc->cb[34].r.left   = skipx;
        calc->cb[34].r.top    = skipy;
        calc->cb[34].r.right  = SZ_MEDBTN_X;
        calc->cb[34].r.bottom = SZ_MEDBTN_Y;
        calc->cb[34].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[35].id       = ID_CALC_MINUS;
        _tcscpy(calc->cb[35].label,TEXT("-"));
        calc->cb[35].color    = CALC_COLOR_RED;
        calc->cb[35].r.left   = skipx;
        calc->cb[35].r.top    = skipy;
        calc->cb[35].r.right  = SZ_MEDBTN_X;
        calc->cb[35].r.bottom = SZ_MEDBTN_Y;
        calc->cb[35].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[36].id       = ID_CALC_LSH;
        _tcscpy(calc->cb[36].label,TEXT("Lsh"));
        calc->cb[36].color    = CALC_COLOR_RED;
        calc->cb[36].r.left   = skipx;
        calc->cb[36].r.top    = skipy;
        calc->cb[36].r.right  = SZ_MEDBTN_X;
        calc->cb[36].r.bottom = SZ_MEDBTN_Y;
        calc->cb[36].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[37].id       = ID_CALC_NOT;
        _tcscpy(calc->cb[37].label,TEXT("Not"));
        calc->cb[37].color    = CALC_COLOR_RED;
        calc->cb[37].r.left   = skipx;
        calc->cb[37].r.top    = skipy;
        calc->cb[37].r.right  = SZ_MEDBTN_X;
        calc->cb[37].r.bottom = SZ_MEDBTN_Y;
        calc->cb[37].enable   = 1;

        // Row 5 (bottom)

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[38].id       = ID_CALC_S;
        _tcscpy(calc->cb[38].label,TEXT("s"));
        calc->cb[38].color    = CALC_COLOR_GRAY;
        calc->cb[38].r.left   = skipx;
        calc->cb[38].r.top    = skipy;
        calc->cb[38].r.right  = SZ_MEDBTN_X;
        calc->cb[38].r.bottom = SZ_MEDBTN_Y;
        calc->cb[38].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[39].id       = ID_CALC_COS;
        _tcscpy(calc->cb[39].label,TEXT("cos"));
        calc->cb[39].color    = CALC_COLOR_MAGENTA;
        calc->cb[39].r.left   = skipx;
        calc->cb[39].r.top    = skipy;
        calc->cb[39].r.right  = SZ_MEDBTN_X;
        calc->cb[39].r.bottom = SZ_MEDBTN_Y;
        calc->cb[39].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[40].id       = ID_CALC_CUBE;
        _tcscpy(calc->cb[40].label,TEXT("x^3"));
        calc->cb[40].color    = CALC_COLOR_MAGENTA;
        calc->cb[40].r.left   = skipx;
        calc->cb[40].r.top    = skipy;
        calc->cb[40].r.right  = SZ_MEDBTN_X;
        calc->cb[40].r.bottom = SZ_MEDBTN_Y;
        calc->cb[40].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[41].id       = ID_CALC_FACTORIAL;
        _tcscpy(calc->cb[41].label,TEXT("n!"));
        calc->cb[41].color    = CALC_COLOR_MAGENTA;
        calc->cb[41].r.left   = skipx;
        calc->cb[41].r.top    = skipy;
        calc->cb[41].r.right  = SZ_MEDBTN_X;
        calc->cb[41].r.bottom = SZ_MEDBTN_Y;
        calc->cb[41].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[42].id       = ID_CALC_MEM_PLUS;
        LoadString( hInst, IDS_BTN_MEM_PLUS, calc->cb[42].label, sizeof(calc->cb[42].label) / sizeof(calc->cb[42].label[0]));
        calc->cb[42].color    = CALC_COLOR_RED;
        calc->cb[42].r.left   = skipx;
        calc->cb[42].r.top    = skipy;
        calc->cb[42].r.right  = SZ_MEDBTN_X;
        calc->cb[42].r.bottom = SZ_MEDBTN_Y;
        calc->cb[42].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[43].id       = ID_CALC_ZERO;
        _tcscpy(calc->cb[43].label,TEXT("0"));
        calc->cb[43].color    = CALC_COLOR_BLUE;
        calc->cb[43].r.left   = skipx;
        calc->cb[43].r.top    = skipy;
        calc->cb[43].r.right  = SZ_MEDBTN_X;
        calc->cb[43].r.bottom = SZ_MEDBTN_Y;
        calc->cb[43].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[44].id       = ID_CALC_SIGN;
        _tcscpy(calc->cb[44].label,TEXT("+/-"));
        calc->cb[44].color    = CALC_COLOR_RED;
        calc->cb[44].r.left   = skipx;
        calc->cb[44].r.top    = skipy;
        calc->cb[44].r.right  = SZ_MEDBTN_X;
        calc->cb[44].r.bottom = SZ_MEDBTN_Y;
        calc->cb[44].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[45].id       = ID_CALC_DECIMAL;
        _tcscpy(calc->cb[45].label,TEXT("."));
        calc->cb[45].color    = CALC_COLOR_BLUE;
        calc->cb[45].r.left   = skipx;
        calc->cb[45].r.top    = skipy;
        calc->cb[45].r.right  = SZ_MEDBTN_X;
        calc->cb[45].r.bottom = SZ_MEDBTN_Y;
        calc->cb[45].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[46].id       = ID_CALC_PLUS;
        _tcscpy(calc->cb[46].label,TEXT("+"));
        calc->cb[46].color    = CALC_COLOR_RED;
        calc->cb[46].r.left   = skipx;
        calc->cb[46].r.top    = skipy;
        calc->cb[46].r.right  = SZ_MEDBTN_X;
        calc->cb[46].r.bottom = SZ_MEDBTN_Y;
        calc->cb[46].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[47].id       = ID_CALC_EQUALS;
        _tcscpy(calc->cb[47].label,TEXT("="));
        calc->cb[47].color    = CALC_COLOR_RED;
        calc->cb[47].r.left   = skipx;
        calc->cb[47].r.top    = skipy;
        calc->cb[47].r.right  = SZ_MEDBTN_X;
        calc->cb[47].r.bottom = SZ_MEDBTN_Y;
        calc->cb[47].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[48].id       = ID_CALC_INT;
        _tcscpy(calc->cb[48].label,TEXT("Int"));
        calc->cb[48].color    = CALC_COLOR_RED;
        calc->cb[48].r.left   = skipx;
        calc->cb[48].r.top    = skipy;
        calc->cb[48].r.right  = SZ_MEDBTN_X;
        calc->cb[48].r.bottom = SZ_MEDBTN_Y;
        calc->cb[48].enable   = 1;


        // Row 6

        skipx = MARGIN_LEFT;
        skipy += SZ_MEDBTN_Y + MARGIN_SMALL_Y;

        calc->cb[49].id       = ID_CALC_DAT;
        _tcscpy(calc->cb[49].label,TEXT("Dat"));
        calc->cb[49].color    = CALC_COLOR_GRAY;
        calc->cb[49].r.left   = skipx;
        calc->cb[49].r.top    = skipy;
        calc->cb[49].r.right  = SZ_MEDBTN_X;
        calc->cb[49].r.bottom = SZ_MEDBTN_Y;
        calc->cb[49].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[50].id       = ID_CALC_TAN;
        _tcscpy(calc->cb[50].label,TEXT("tan"));
        calc->cb[50].color    = CALC_COLOR_MAGENTA;
        calc->cb[50].r.left   = skipx;
        calc->cb[50].r.top    = skipy;
        calc->cb[50].r.right  = SZ_MEDBTN_X;
        calc->cb[50].r.bottom = SZ_MEDBTN_Y;
        calc->cb[50].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[51].id       = ID_CALC_SQUARE;
        _tcscpy(calc->cb[51].label,TEXT("x^2"));
        calc->cb[51].color    = CALC_COLOR_MAGENTA;
        calc->cb[51].r.left   = skipx;
        calc->cb[51].r.top    = skipy;
        calc->cb[51].r.right  = SZ_MEDBTN_X;
        calc->cb[51].r.bottom = SZ_MEDBTN_Y;
        calc->cb[51].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[52].id       = ID_CALC_RECIPROCAL;
        _tcscpy(calc->cb[52].label,TEXT("1/x"));
        calc->cb[52].color    = CALC_COLOR_MAGENTA;
        calc->cb[52].r.left   = skipx;
        calc->cb[52].r.top    = skipy;
        calc->cb[52].r.right  = SZ_MEDBTN_X;
        calc->cb[52].r.bottom = SZ_MEDBTN_Y;
        calc->cb[52].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[53].id       = ID_CALC_PI;
        _tcscpy(calc->cb[53].label,TEXT("pi"));
        calc->cb[53].color    = CALC_COLOR_BLUE;
        calc->cb[53].r.left   = skipx;
        calc->cb[53].r.top    = skipy;
        calc->cb[53].r.right  = SZ_MEDBTN_X;
        calc->cb[53].r.bottom = SZ_MEDBTN_Y;
        calc->cb[53].enable   = 1;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X + SZ_SPACER_X;

        calc->cb[54].id       = ID_CALC_A;
        _tcscpy(calc->cb[54].label,TEXT("A"));
        calc->cb[54].color    = CALC_COLOR_GRAY;
        calc->cb[54].r.left   = skipx;
        calc->cb[54].r.top    = skipy;
        calc->cb[54].r.right  = SZ_MEDBTN_X;
        calc->cb[54].r.bottom = SZ_MEDBTN_Y;
        calc->cb[54].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[55].id       = ID_CALC_B;
        _tcscpy(calc->cb[55].label,TEXT("B"));
        calc->cb[55].color    = CALC_COLOR_GRAY;
        calc->cb[55].r.left   = skipx;
        calc->cb[55].r.top    = skipy;
        calc->cb[55].r.right  = SZ_MEDBTN_X;
        calc->cb[55].r.bottom = SZ_MEDBTN_Y;
        calc->cb[55].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[56].id       = ID_CALC_C;
        _tcscpy(calc->cb[56].label,TEXT("C"));
        calc->cb[56].color    = CALC_COLOR_GRAY;
        calc->cb[56].r.left   = skipx;
        calc->cb[56].r.top    = skipy;
        calc->cb[56].r.right  = SZ_MEDBTN_X;
        calc->cb[56].r.bottom = SZ_MEDBTN_Y;
        calc->cb[56].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[57].id       = ID_CALC_D;
        _tcscpy(calc->cb[57].label,TEXT("D"));
        calc->cb[57].color    = CALC_COLOR_GRAY;
        calc->cb[57].r.left   = skipx;
        calc->cb[57].r.top    = skipy;
        calc->cb[57].r.right  = SZ_MEDBTN_X;
        calc->cb[57].r.bottom = SZ_MEDBTN_Y;
        calc->cb[57].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[58].id       = ID_CALC_E;
        _tcscpy(calc->cb[58].label,TEXT("E"));
        calc->cb[58].color    = CALC_COLOR_GRAY;
        calc->cb[58].r.left   = skipx;
        calc->cb[58].r.top    = skipy;
        calc->cb[58].r.right  = SZ_MEDBTN_X;
        calc->cb[58].r.bottom = SZ_MEDBTN_Y;
        calc->cb[58].enable   = 0;

        skipx += SZ_MEDBTN_X + MARGIN_SMALL_X;

        calc->cb[59].id       = ID_CALC_F;
        _tcscpy(calc->cb[59].label,TEXT("F"));
        calc->cb[59].color    = CALC_COLOR_GRAY;
        calc->cb[59].r.left   = skipx;
        calc->cb[59].r.top    = skipy;
        calc->cb[59].r.right  = SZ_MEDBTN_X;
        calc->cb[59].r.bottom = SZ_MEDBTN_Y;
        calc->cb[59].enable   = 0;

        // Buttons

        calc->cb[60].id       = ID_CALC_NS_HEX;
        LoadString( hInst, IDS_BTN_SHRT_HEX, calc->cb[60].label, sizeof(calc->cb[60].label) / sizeof(calc->cb[60].label[0]));
        calc->cb[60].color    = CALC_COLOR_GRAY;
        calc->cb[60].r.left   = CALC_NS_HEX_LEFT;
        calc->cb[60].r.top    = CALC_NS_HEX_TOP;
        calc->cb[60].r.right  = SZ_RADIO_NS_X;
        calc->cb[60].r.bottom = SZ_RADIO_NS_Y;
        calc->cb[60].enable   = 1;

        calc->cb[61].id       = ID_CALC_NS_DEC;
        LoadString( hInst, IDS_BTN_SHRT_DEC, calc->cb[61].label, sizeof(calc->cb[61].label) / sizeof(calc->cb[61].label[0]));
        calc->cb[61].color    = CALC_COLOR_GRAY;
        calc->cb[61].r.left   = CALC_NS_DEC_LEFT;
        calc->cb[61].r.top    = CALC_NS_DEC_TOP;
        calc->cb[61].r.right  = SZ_RADIO_NS_X;
        calc->cb[61].r.bottom = SZ_RADIO_NS_Y;
        calc->cb[61].enable   = 1;

        calc->cb[62].id       = ID_CALC_NS_OCT;
        LoadString( hInst, IDS_BTN_SHRT_OCT, calc->cb[62].label, sizeof(calc->cb[62].label) / sizeof(calc->cb[62].label[0]));
        calc->cb[62].color    = CALC_COLOR_GRAY;
        calc->cb[62].r.left   = CALC_NS_OCT_LEFT;
        calc->cb[62].r.top    = CALC_NS_OCT_TOP;
        calc->cb[62].r.right  = SZ_RADIO_NS_X;
        calc->cb[62].r.bottom = SZ_RADIO_NS_Y;
        calc->cb[62].enable   = 1;

        calc->cb[63].id       = ID_CALC_NS_BIN;
        LoadString( hInst, IDS_BTN_SHRT_BIN, calc->cb[63].label, sizeof(calc->cb[63].label) / sizeof(calc->cb[63].label[0]));
        calc->cb[63].color    = CALC_COLOR_GRAY;
        calc->cb[63].r.left   = CALC_NS_BIN_LEFT;
        calc->cb[63].r.top    = CALC_NS_BIN_TOP;
        calc->cb[63].r.right  = SZ_RADIO_NS_X;
        calc->cb[63].r.bottom = SZ_RADIO_NS_Y;
        calc->cb[63].enable   = 1;

        calc->cb[64].id       = ID_CALC_MS_DEGREES;
        LoadString( hInst, IDS_BTN_DEGREES, calc->cb[64].label, sizeof(calc->cb[64].label) / sizeof(calc->cb[64].label[0]));
        calc->cb[64].color    = CALC_COLOR_GRAY;
        calc->cb[64].r.left   = CALC_MS_DEGREES_LEFT;
        calc->cb[64].r.top    = CALC_MS_DEGREES_TOP;
        calc->cb[64].r.right  = SZ_RADIO_MS_X;
        calc->cb[64].r.bottom = SZ_RADIO_MS_Y;
        calc->cb[64].enable   = 1;

        calc->cb[65].id       = ID_CALC_MS_RADIANS;
        LoadString( hInst, IDS_BTN_RADIANS, calc->cb[65].label, sizeof(calc->cb[65].label) / sizeof(calc->cb[65].label[0]));
        calc->cb[65].color    = CALC_COLOR_GRAY;
        calc->cb[65].r.left   = CALC_MS_RADIANS_LEFT;
        calc->cb[65].r.top    = CALC_MS_RADIANS_TOP;
        calc->cb[65].r.right  = SZ_RADIO_MS_X;
        calc->cb[65].r.bottom = SZ_RADIO_MS_Y;
        calc->cb[65].enable   = 1;

        calc->cb[66].id       = ID_CALC_MS_GRADS;
        LoadString( hInst, IDS_BTN_GRADS, calc->cb[66].label, sizeof(calc->cb[66].label) / sizeof(calc->cb[66].label[0]));
        calc->cb[66].color    = CALC_COLOR_GRAY;
        calc->cb[66].r.left   = CALC_MS_GRADS_LEFT;
        calc->cb[66].r.top    = CALC_MS_GRADS_TOP;
        calc->cb[66].r.right  = SZ_RADIO_MS_X;
        calc->cb[66].r.bottom = SZ_RADIO_MS_Y;
        calc->cb[66].enable   = 1;

        calc->cb[67].id       = ID_CALC_CB_INV;
        _tcscpy(calc->cb[67].label,TEXT("Inv"));
        calc->cb[67].color    = CALC_COLOR_GRAY;
        calc->cb[67].r.left   = CALC_CB_INV_LEFT;
        calc->cb[67].r.top    = CALC_CB_INV_TOP;
        calc->cb[67].r.right  = SZ_RADIO_CB_X;
        calc->cb[67].r.bottom = SZ_RADIO_CB_Y;
        calc->cb[67].enable   = 1;

        calc->cb[68].id       = ID_CALC_CB_HYP;
        _tcscpy(calc->cb[68].label,TEXT("Hyp"));
        calc->cb[68].color    = CALC_COLOR_GRAY;
        calc->cb[68].r.left   = CALC_CB_HYP_LEFT;
        calc->cb[68].r.top    = CALC_CB_HYP_TOP;
        calc->cb[68].r.right  = SZ_RADIO_CB_X;
        calc->cb[68].r.bottom = SZ_RADIO_CB_Y;
        calc->cb[68].enable   = 1;

        calc->cb[69].id       = ID_CALC_WS_QWORD;
        _tcscpy(calc->cb[69].label,TEXT("Qword"));
        calc->cb[69].color    = CALC_COLOR_GRAY;
        calc->cb[69].r.left   = CALC_WS_QWORD_LEFT;
        calc->cb[69].r.top    = CALC_WS_QWORD_TOP;
        calc->cb[69].r.right  = SZ_RADIO_WS_X;
        calc->cb[69].r.bottom = SZ_RADIO_WS_Y;
        calc->cb[69].enable   = 1;

        calc->cb[70].id       = ID_CALC_WS_DWORD;
        _tcscpy(calc->cb[70].label,TEXT("Dword"));
        calc->cb[70].color    = CALC_COLOR_GRAY;
        calc->cb[70].r.left   = CALC_WS_DWORD_LEFT;
        calc->cb[70].r.top    = CALC_WS_DWORD_TOP;
        calc->cb[70].r.right  = SZ_RADIO_WS_X;
        calc->cb[70].r.bottom = SZ_RADIO_WS_Y;
        calc->cb[70].enable   = 1;

        calc->cb[71].id       = ID_CALC_WS_WORD;
        _tcscpy(calc->cb[71].label,TEXT("Word"));
        calc->cb[71].color    = CALC_COLOR_GRAY;
        calc->cb[71].r.left   = CALC_WS_WORD_LEFT;
        calc->cb[71].r.top    = CALC_WS_WORD_TOP;
        calc->cb[71].r.right  = SZ_RADIO_WS_X;
        calc->cb[71].r.bottom = SZ_RADIO_WS_Y;
        calc->cb[71].enable   = 1;

        calc->cb[72].id       = ID_CALC_WS_BYTE;
        _tcscpy(calc->cb[72].label,TEXT("Byte"));
        calc->cb[72].color    = CALC_COLOR_GRAY;
        calc->cb[72].r.left   = CALC_WS_BYTE_LEFT;
        calc->cb[72].r.top    = CALC_WS_BYTE_TOP;
        calc->cb[72].r.right  = SZ_RADIO_WS_X;
        calc->cb[72].r.bottom = SZ_RADIO_WS_Y;
        calc->cb[72].enable   = 1;
    }

    // preload clip region for filler squares

    if (calc->sciMode) {
        n = 1;

        rFiller.left = calc->cb[0].r.left;
        rFiller.top  = calc->cb[0].r.top;
        rFiller.right  = calc->cb[0].r.right;
        rFiller.bottom = calc->cb[0].r.bottom;
    }
    else {
        n = 2;

        rFiller.left   = calc->cb[1].r.left;
        rFiller.top    = calc->cb[1].r.top;
        rFiller.right  = calc->cb[0].r.right;
        rFiller.bottom = calc->cb[0].r.bottom;
    }

    top_button = calc->numButtons - 1;
    if (!calc->sciMode)
        top_button -= CALC_NS_COUNT + CALC_MS_COUNT + CALC_CB_COUNT + CALC_WS_COUNT;

    for (; n <= top_button; n++) {
    ;

        calc->cb[n].hBtn = CreateWindow(
            TEXT("BUTTON"),
            calc->cb[n].label,
            WS_VISIBLE | WS_CHILD | WS_BORDER | BS_CENTER | BS_VCENTER | BS_TEXT |
                (calc->cb[n].enable ? 0 : WS_DISABLED), // BS_FLAT
            calc->cb[n].r.left,
            calc->cb[n].r.top,
            calc->cb[n].r.right,
            calc->cb[n].r.bottom,
            calc->hWnd,
            (HMENU)calc->cb[n].id,
            calc->hInst,
            NULL
        );

        if (!calc->cb[n].hBtn)
            exit(1);

        SendMessage(calc->cb[n].hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    if (!calc->sciMode) {
        top_button += CALC_NS_COUNT;
        for (; n<=top_button; n++) {
            int j = ID_CALC_NS_HEX + n - top_button + CALC_NS_COUNT - 1;
            calc->cb[n].hBtn = CreateWindow(
                TEXT("BUTTON"),
                calc->cb[n].label,
                WS_VISIBLE | WS_CHILD | BS_LEFT | BS_VCENTER | BS_TEXT | BS_RADIOBUTTON,
                CALC_NS_OFFSET_X + calc->cb[n].r.left,
                CALC_NS_OFFSET_Y + calc->cb[n].r.top,
                calc->cb[n].r.right,
                calc->cb[n].r.bottom,
                calc->hWnd,
                (HMENU)(j),
                calc->hInst,
                NULL
            );

            if (!calc->cb[n].hBtn)
               exit(1);

            SendMessage(calc->cb[n].hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        top_button += CALC_MS_COUNT;

        for (; n<=top_button; n++) {
            calc->cb[n].hBtn = CreateWindow(
                TEXT("BUTTON"),
                calc->cb[n].label,
                WS_VISIBLE | WS_CHILD | BS_LEFT | BS_VCENTER | BS_TEXT | BS_RADIOBUTTON,
                CALC_MS_OFFSET_X + calc->cb[n].r.left,
                CALC_MS_OFFSET_Y + calc->cb[n].r.top,
                calc->cb[n].r.right,
                calc->cb[n].r.bottom,
                calc->hWnd,
                (HMENU)(ID_CALC_MS_DEGREES + n - top_button + CALC_MS_COUNT - 1),
                calc->hInst,
                NULL
            );

            if (!calc->cb[n].hBtn)
               exit(1);

            SendMessage(calc->cb[n].hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        top_button += CALC_CB_COUNT;

        for (; n<=top_button; n++) {
            calc->cb[n].hBtn = CreateWindow(
                TEXT("BUTTON"),
                calc->cb[n].label,
                WS_VISIBLE | WS_CHILD | BS_LEFT | BS_VCENTER | BS_TEXT | BS_CHECKBOX,
                CALC_CB_OFFSET_X + calc->cb[n].r.left,
                CALC_CB_OFFSET_Y + calc->cb[n].r.top,
                calc->cb[n].r.right,
                calc->cb[n].r.bottom,
                calc->hWnd,
                (HMENU)(ID_CALC_CB_INV + n - top_button + CALC_CB_COUNT - 1),
                calc->hInst,
                NULL
            );

            if (!calc->cb[n].hBtn)
               exit(1);

            SendMessage(calc->cb[n].hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        top_button += CALC_WS_COUNT;

        for (; n<=top_button; n++) {
            calc->cb[n].hBtn = CreateWindow(
                TEXT("BUTTON"),
                calc->cb[n].label,
                WS_CHILD | BS_LEFT | BS_VCENTER | BS_TEXT | BS_RADIOBUTTON,
                CALC_WS_OFFSET_X + calc->cb[n].r.left,
                CALC_WS_OFFSET_Y + calc->cb[n].r.top,
                calc->cb[n].r.right,
                calc->cb[n].r.bottom,
                calc->hWnd,
                (HMENU)(ID_CALC_WS_QWORD + n - top_button + CALC_WS_COUNT - 1),
                calc->hInst,
                NULL
            );

            if (!calc->cb[n].hBtn)
               exit(1);

            EnableWindow(calc->cb[n].hBtn, FALSE);
            SendMessage(calc->cb[n].hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // set sci defaults

        SendMessage(calc->cb[61].hBtn, BM_SETCHECK, 1, 0); // decimal
        SendMessage(calc->cb[64].hBtn, BM_SETCHECK, 1, 0); // degrees
        SendMessage(calc->cb[69].hBtn, BM_SETCHECK, 1, 0); // qword
    }
}

void DrawCalcRect (HDC hdc, HDC hMemDC, PAINTSTRUCT  *ps, CALC *calc, int object)
{
    POINT pt;
    HPEN hPenGray;
    HPEN hPenBlack;
    HPEN hPenWhite;
    HPEN hPenOrg;

    int WDISPLAY_LEFT;
    int WDISPLAY_TOP;
    int WDISPLAY_RIGHT;
    int WDISPLAY_BOTTOM;

    if (!calc->sciMode) {
        WDISPLAY_LEFT   = WDISPLAY_SCIENTIFIC_LEFT;
        WDISPLAY_TOP    = WDISPLAY_SCIENTIFIC_TOP;
        WDISPLAY_RIGHT  = WDISPLAY_SCIENTIFIC_RIGHT;
        WDISPLAY_BOTTOM = WDISPLAY_SCIENTIFIC_BOTTOM;
    }
    else {
        WDISPLAY_LEFT   = WDISPLAY_STANDARD_LEFT;
        WDISPLAY_TOP    = WDISPLAY_STANDARD_TOP;
        WDISPLAY_RIGHT  = WDISPLAY_STANDARD_RIGHT;
        WDISPLAY_BOTTOM = WDISPLAY_STANDARD_BOTTOM;
    }

    // never delete the active pen!

    // Gray Pen

    hPenGray  = CreatePen(PS_SOLID, 1, RGB(CALC_GRAY,CALC_GRAY,CALC_GRAY));
    hPenBlack = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    hPenWhite = CreatePen(PS_SOLID, 1, RGB(255,255,255));

    hPenOrg = SelectObject(hdc, hPenGray);

    MoveToEx(hdc,
        calc->cb[object].r.left,
        calc->cb[object].r.bottom,
        &pt);

    LineTo(hdc, calc->cb[object].r.left,  calc->cb[object].r.top);
    LineTo(hdc, calc->cb[object].r.right, calc->cb[object].r.top);

    // Black Pen

    SelectObject(hdc, hPenBlack);

    MoveToEx(hdc,
        calc->cb[object].r.right-2,
        calc->cb[object].r.top+1,
        &pt);

    LineTo(hdc, calc->cb[object].r.left+1, calc->cb[object].r.top+1);
    LineTo(hdc, calc->cb[object].r.left+1, calc->cb[object].r.bottom-1);

    SelectObject(hdc, hPenBlack);

    MoveToEx(hdc,
        WDISPLAY_LEFT - 1,
        WDISPLAY_BOTTOM,
        &pt);

    LineTo(hdc, WDISPLAY_LEFT - 1,  WDISPLAY_TOP - 1);
    LineTo(hdc, WDISPLAY_RIGHT + 1, WDISPLAY_TOP - 1);

    SelectObject(hdc, hPenGray);

    MoveToEx(hdc,
        WDISPLAY_RIGHT + 1,
        WDISPLAY_TOP - 2,
        &pt);

    LineTo(hdc, WDISPLAY_LEFT - 2, WDISPLAY_TOP - 2);
    LineTo(hdc, WDISPLAY_LEFT - 2, WDISPLAY_BOTTOM + 1);

    // White Pen

    SelectObject(hdc, hPenWhite);

    MoveToEx(hdc,
        calc->cb[object].r.left,
        calc->cb[object].r.bottom,
        &pt);

    LineTo(hdc, calc->cb[object].r.right, calc->cb[object].r.bottom);
    LineTo(hdc, calc->cb[object].r.right, calc->cb[object].r.top);

    SelectObject(hdc, hPenOrg);
    DeleteObject(hPenGray);
    DeleteObject(hPenBlack);
    DeleteObject(hPenWhite);
}

void DrawCalcRectSci(HDC hdc, HDC hMemDC, PAINTSTRUCT  *ps, CALC *calc, RECT *r)
{
    POINT pt;
    HPEN hPen;
    HPEN hPenOrg;

    // never delete the active pen!

    // White Pen

    hPen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
    hPenOrg = SelectObject(hdc, hPen);

    MoveToEx(hdc,
        r->left + 1,
        r->bottom + 1,
        &pt);

    LineTo(hdc, r->left  + 1, r->top    + 1);
    LineTo(hdc, r->right + 1, r->top    + 1);
    LineTo(hdc, r->right + 1, r->bottom + 1);
    LineTo(hdc, r->left  + 1, r->bottom + 1);

    SelectObject(hdc, hPenOrg);
    DeleteObject(hPen);

    // Black Pen

    hPen = CreatePen(PS_SOLID, 1, RGB(CALC_GRAY,CALC_GRAY,CALC_GRAY));
    hPenOrg = SelectObject(hdc, hPen);

    MoveToEx(hdc,
        r->left,
        r->bottom,
        &pt);

    LineTo(hdc, r->left,  r->top);
    LineTo(hdc, r->right, r->top);
    LineTo(hdc, r->right, r->bottom);
    LineTo(hdc, r->left,  r->bottom);

    SelectObject(hdc, hPenOrg);
    DeleteObject(hPen);
}

static RECT scirect1 = {
    WDISPLAY_SCIENTIFIC_LEFT - 2,
    WDISPLAY_SCIENTIFIC_TOP  + 25,
    WDISPLAY_SCIENTIFIC_LEFT + 202,
    WDISPLAY_SCIENTIFIC_TOP  + 51
};

static RECT scirect2 = {
    WDISPLAY_SCIENTIFIC_LEFT  + 205,
    WDISPLAY_SCIENTIFIC_TOP   + 25,
    WDISPLAY_SCIENTIFIC_RIGHT,
    WDISPLAY_SCIENTIFIC_TOP   + 51
};

static RECT scirect3 = {
    WDISPLAY_SCIENTIFIC_LEFT - 2,
    WDISPLAY_SCIENTIFIC_TOP  + 57,
    WDISPLAY_SCIENTIFIC_LEFT + 102,
    WDISPLAY_SCIENTIFIC_TOP  + 81
};

void DrawCalc (HDC hdc, HDC hMemDC, PAINTSTRUCT  *ps, CALC *calc)
{
    TCHAR s[CALC_BUF_SIZE];

    scirect1.right = calc->cb[0].r.right + 2;
    scirect2.left  = calc->cb[0].r.right + 5;
    scirect3.right = calc->cb[1].r.left  - 10;

    DrawCalcRect (hdc, hMemDC, ps, calc, 0);

    if (!calc->sciMode) {
        DrawCalcRect (hdc, hMemDC, ps, calc, 1);
        DrawCalcRectSci(hdc, hMemDC, ps, calc, &scirect1);
        DrawCalcRectSci(hdc, hMemDC, ps, calc, &scirect2);
        DrawCalcRectSci(hdc, hMemDC, ps, calc, &scirect3);
    }

    LoadString(calc->hInst, IDS_BTN_MEM_STATUS_M, s, sizeof(s) / sizeof(s[0]));

    DrawCalcText(hdc, hMemDC, ps, calc, 0, s);
}

void DrawCalcText (HDC hdc, HDC hMemDC, PAINTSTRUCT  *ps, CALC *calc, int object, TCHAR *s)
{
    POINT pt;
    HFONT hFont;
    HFONT hFontOrg;
    HPEN hPen;
    HPEN hPenOrg;
    HBRUSH hBrush;
    HBRUSH hBrushOrg;

    TCHAR s2[CALC_BUF_SIZE];

    int WDISPLAY_LEFT;
    int WDISPLAY_TOP;
    int WDISPLAY_RIGHT;
    int WDISPLAY_BOTTOM;

    if (!calc->sciMode) {
        WDISPLAY_LEFT   = WDISPLAY_SCIENTIFIC_LEFT;
        WDISPLAY_TOP    = WDISPLAY_SCIENTIFIC_TOP;
        WDISPLAY_RIGHT  = WDISPLAY_SCIENTIFIC_RIGHT;
        WDISPLAY_BOTTOM = WDISPLAY_SCIENTIFIC_BOTTOM;
    }
    else {
        WDISPLAY_LEFT   = WDISPLAY_STANDARD_LEFT;
        WDISPLAY_TOP    = WDISPLAY_STANDARD_TOP;
        WDISPLAY_RIGHT  = WDISPLAY_STANDARD_RIGHT;
        WDISPLAY_BOTTOM = WDISPLAY_STANDARD_BOTTOM;
    }

    // DEFAULT_GUI_FONT is Tahoma on 2000 and XP?
    // SYSTEM_FONT is MS Sans Serif?

    hFont = GetStockObject(DEFAULT_GUI_FONT);

    hFontOrg = SelectObject(hdc, hFont);

    if (calc->memory) {
        SetBkMode(hdc, TRANSPARENT);

        TextOut(hdc,
            calc->cb[object].r.left + 9,
            calc->cb[object].r.top  + 7,
            s,
            _tcslen(s)
        );

        SetBkMode(hdc, OPAQUE);
    }

    if (calc->paren) {
        _stprintf(s2, TEXT("(=%d"),calc->paren);

        SetBkMode(hdc, TRANSPARENT);

        SetTextAlign(hdc, TA_CENTER);

        TextOut(hdc,
            calc->cb[object+1].r.left + 13,
            calc->cb[object+1].r.top  + 6,
            s2,
            _tcslen(s2)
        );

        SetBkMode(hdc, OPAQUE);
    }

    hPen = CreatePen(PS_SOLID, 1, RGB(255,255,255));
    hPenOrg = SelectObject(hdc, hPen);
    hBrush = GetSysColorBrush(COLOR_WINDOW);
    hBrushOrg = SelectObject(hdc, hBrush);

    MoveToEx(hdc,
        WDISPLAY_LEFT - 1,
        WDISPLAY_BOTTOM + 1,
        &pt
    );

    LineTo(hdc, WDISPLAY_RIGHT + 1, WDISPLAY_BOTTOM + 1);
    LineTo(hdc, WDISPLAY_RIGHT + 1, WDISPLAY_TOP - 2);

    Rectangle(hdc, WDISPLAY_LEFT, WDISPLAY_TOP, WDISPLAY_RIGHT, WDISPLAY_BOTTOM);

    SelectObject(hdc, hPenOrg);
    SelectObject(hdc, hBrushOrg);
    DeleteObject(hPen);

    SetBkMode(hdc, TRANSPARENT);

    SetTextAlign(hdc, TA_RIGHT);

    TextOut(hdc,
        WDISPLAY_RIGHT - 4,
        WDISPLAY_TOP   + 1,
        calc->display,
        _tcslen(calc->display)
    );

    SelectObject(hdc, hFontOrg);
}

void DestroyCalc (CALC *calc)
{
    int i;

    for (i=0;i<calc->numButtons;i++)
        DestroyWindow(calc->cb[i].hBtn);
}

void DestroyMenus()
{
    if (menus[MENU_STD] != 0)
        DestroyMenu(menus[MENU_STD]);

    if (menus[MENU_SCIMS] != 0)
        DestroyMenu(menus[MENU_SCIMS]);

    if (menus[MENU_SCIWS] != 0)
        DestroyMenu(menus[MENU_SCIWS]);
}

void calc_buffer_format(CALC *calc) {
    TCHAR *p;
    int n;
    int flag = 0;
    int point = 0;

    // calc_buffer_format: enforce buffer content rules
    //
    // disallow more than one point (beep)
    // remove leading zeros
    // remove trailing zeros after point (sprintf can cause this)
    // remove trailing points
    // chop at 32 digits. Though there could also be a point, and 10 separator characters.

    p = calc->buffer;
    while (*p) {
        if (*p++ == TEXT('.'))
            point++;
    }

    if (point > 1) {
        calc->buffer[_tcslen(calc->buffer)-1] = TEXT('\0');
        MessageBeep(0);
    }

    //if (point) {
    //    p = calc->buffer;
    //    n = _tcslen(p) - 1;
    //    while (*(p+n) &&
    //            *(p+n) != TEXT('.') &&
    //           *(p+n) == TEXT('0')) {
    //        calc->buffer[n] = TEXT('\0');
    //        n--;
    //    }
    //}

    // remove leading zeros

    p = calc->buffer;
    while (*p) {
        if (*p != TEXT('0'))
            break;
        p++;
    }

    // remove trailing points

    n = _tcslen(p);

    while (n) {
        if (*(p+n-1) == TEXT('.')) {
            if (flag) {
                *(p + n) = TEXT('\0');
            }
            else {
                flag = 1;
            }
            n--;
        }
        else {
            break;
        }
    }

    //   if (!*p)
    //      _tcscpy(p, TEXT("0"));

    // chop at 32 digits
    if (flag)
        *(p+33) = TEXT('\0');
    else
        *(p+32) = TEXT('\0');

    n = 0;
    while (*p)
        *(calc->buffer + n++) = *(p++);
    *(calc->buffer + n) = TEXT('\0');
}

void calc_buffer_display(CALC *calc) {
    TCHAR *p;
    TCHAR s[CALC_BUF_SIZE];
    int point=0;
    calcfloat real;
    static int old_base = NBASE_DECIMAL;


    switch (calc->numBase) {
    case NBASE_HEX:
        real = calc_atof(calc->buffer, old_base);
        _stprintf(calc->display, _T("%lx"), (long)real);
        _stprintf(calc->buffer, _T("%lx"), (long)real);
        old_base = NBASE_HEX;
        break;

    case NBASE_OCTAL:
         real = calc_atof(calc->buffer, old_base);
        _stprintf(calc->display, TEXT("%lo"), (long)real);
        _stprintf(calc->buffer, TEXT("%lo"), (long)real);
        old_base = NBASE_OCTAL;
        break;

    case NBASE_BINARY:
        {
          int buf=0;
          int t;

          if (calc->buffer[0]==_T('\0'))
          {
            real=0;
          }
          else
          {
            real = calc_atof(calc->buffer, old_base);
          }

          calc->display[buf]=_T('0');
          calc->buffer[buf]=_T('0');
          for (t=31;t>=0;t--)
          {
            if (((((long)real)>>t) & ~0xFFFFFFFE)==0)
            {
                calc->display[buf]=_T('0');
                calc->buffer[buf]=_T('0');
                buf++;
            }
            else
            {
                calc->display[buf]=_T('1');
                calc->buffer[buf]=_T('1');
                buf++;
            }

          }

          if (buf==0)
          {
              buf++;
          }

          calc->buffer[buf]=_T('\0');
          calc->display[buf]=_T('\0');
          old_base = NBASE_BINARY;
        }
        break;

    case NBASE_DECIMAL:

       calc_buffer_format(calc);

        if (calc->displayMode) {
            if (!_tcscmp(calc->buffer, TEXT("0")) || !calc->buffer[0]) {
                _tcscpy(calc->display, TEXT("0.e+0"));
            }
            else {
                int i = 0;
                int lz = 0;
                int exp = 0;



                real = calc_atof(calc->buffer,old_base);
                _stprintf(s, FMT_DESC_EXP, real);
                // remove leading zeros in exponent
                p = s;
                while (*p) {
                    if (*p == TEXT('e')) { // starting exponent parsing

                        exp = 1;
                    }
                    else if (exp) { // inside exponent, and haven't seen a digit, so could be a leading zero

                        if (*p == TEXT('0'))
                            lz = 1;
                    }

                    if (exp && (*p != TEXT('e')) && (*p != TEXT('0')) && (*p != TEXT('+')) && (*p != TEXT('-'))) {
                        exp = 0;
                        lz = 0;
                    }

                    if (!lz)
                        calc->display[i++] = *p;

                    p++;
                }

                if (calc->display[i-1] == TEXT('+')) // all trailing zeros

                    calc->display[i++] = TEXT('0');

                calc->display[i] = 0;
            }
        }
        else {
            // calc_buffer_display: display buffer after formatting
            //
            // if digitGrouping, embed separators
            // add point if missing
            // display

            if (old_base != calc->numBase)
            {
                if (calc->buffer[0]==_T('\0'))
                {
                    real = 0;
                }
                else
                {
                    real = calc_atof(calc->buffer, old_base);
                }
                _stprintf(calc->display, _T("%.f"), real);
                _stprintf(calc->buffer, _T("%.f"), real);
            }

            _tcscpy(s,calc->buffer);
            p = s;

            while (*p) {
                if (*p++ == TEXT('.'))
                    point = 1;
            }

            if (!*s)
                _tcscpy(s, TEXT("0"));

            if (calc->digitGrouping)
                calc_sep(s);

            if (!point && calc->numBase == NBASE_DECIMAL)
                _tcscat(s, TEXT("."));

            if (*s == TEXT('.'))
                _tcscpy(calc->display, TEXT("0"));
            else
                calc->display[0] = 0;
            _tcscat(calc->display, s);
        }
        old_base = NBASE_DECIMAL;
    }

    InvalidateRect(calc->hWnd, NULL, FALSE);
    UpdateWindow(calc->hWnd);
}

TCHAR *calc_sep(TCHAR *s)
{
    TCHAR c;
    TCHAR *p;
    int n;
    int x = 1;
    int i = 0;
    int point = 0;
    TCHAR r[CALC_BUF_SIZE];

    n = _tcslen(s);

    if (!*s)
        return s;

    p = s;

    // need to handle leading minus sign!

    // see if there is a point character

    while (*p) {
        if (*p++ == TEXT('.')) {
            point = p - s;
            break;
        }
    }

    // if there is a point character, skip over decimal places

    if (point) {
        i = n - point + 1;
        n = point - 1;
        _tcscpy(r, s);
        _tcsrev(r);
    }

    // commify the integer part now

    while ((c = *(s + --n))) {
        r[i++] = c;
        if (x++ % 3 == 0)
            r[i++] = TEXT(',');
        if (n == 0)
            break;
    }

    if (r[i-1] == TEXT(','))
        r[--i] = TEXT('\0');
    else
        r[i] = TEXT('\0');

    _tcscpy(s, _tcsrev(r));

    return s;
}

long factorial(long n)
{
    if (n <= 1L)
        return 1L;

    return n * factorial(n - 1);
}

void calc_setmenuitem_radio(HMENU hMenu, UINT id)
{
    MENUITEMINFO menuItem;

    menuItem.fMask         = MIIM_FTYPE;
    menuItem.fType         = MFT_STRING | MFT_RADIOCHECK;
    //   menuItem.fState        = MFS_ENABLED;
    //   menuItem.wID           = id;
    //   menuItem.hSubMenu      = NULL;
    //   menuItem.hbmpChecked   = NULL;
    //   menuItem.hbmpUnchecked = NULL;
    //   menuItem.dwItemData    = 0;
    //   menuItem.dwTypeData    = "Hex\tF5";
    //   menuItem.cch           = sizeof("Hex\tF5");
    menuItem.cbSize        = sizeof(MENUITEMINFO);

    SetMenuItemInfo(hMenu, id, FALSE, &menuItem);
}

calcfloat calc_convert_to_radians(CALC *calc)
{
    calcfloat r = calc_atof(calc->buffer, calc->numBase);

    if (calc->trigMode == TRIGMODE_RADIANS)
        return r;
    if (calc->trigMode == TRIGMODE_DEGREES)
        return r * CONST_PI / 180;
    else if (calc->trigMode == TRIGMODE_GRADS)
        return r * CONST_PI / 200;           // 90 degrees == 100 grads

    return 0L;
}

calcfloat calc_convert_from_radians(CALC *calc)
{
    calcfloat r = calc_atof(calc->buffer, calc->numBase);

    if (calc->trigMode == TRIGMODE_RADIANS)
        return r;
    if (calc->trigMode == TRIGMODE_DEGREES)
        return r * 180 / CONST_PI;
    else if (calc->trigMode == TRIGMODE_GRADS)
        return r * 200 / CONST_PI;           // 90 degrees == 100 grads

    return 0L;
}

void show_debug(CALC *calc, TCHAR *title, long w, long l)
{
    TCHAR s[1024];

    _stprintf(s,

        TEXT("wParam	= (%C) %d:%d, %x:%xh\n \
lParam	= %d:%d, %x:%x\n \
value	= %.32g\n \
memory	= %.32g\n \
buffer	= \"%s\"\n \
display	= \"%s\"\n \
numBase	= %d\n \
trigMode	= %d\n \
wordSize	= %d\n \
invMode	= %d\n \
hypMode	= %d\n \
oper	= (%C)\n"),

        LOWORD(w),
        LOWORD(w),
        HIWORD(w),
        LOWORD(w),
        HIWORD(w),
        LOWORD(l),
        HIWORD(l),
        LOWORD(l),
        HIWORD(l),
        calc->value,
        calc->memory,
        calc->buffer,
        calc->display,
        calc->numBase,
        calc->trigMode,
        calc->wordSize,
        calc->invMode,
        calc->hypMode,
        calc->oper
            );

    MessageBox(calc->hWnd, s, title, MB_OK);
}

calcfloat calc_atof(const TCHAR *s, int base)
{
#ifdef UNICODE
    char s_ansi[128];
#endif

    // converts from another base to decimal calcfloat
    switch (base) {
    case NBASE_DECIMAL:
        wcstombs(s_ansi, s, sizeof(s_ansi));
        return CALC_ATOF(s_ansi);
    case NBASE_HEX:
        return (calcfloat)_tcstol(s, NULL, 16);
    case NBASE_OCTAL:
        return (calcfloat)_tcstol(s, NULL, 8);
    case NBASE_BINARY:
        return (calcfloat)_tcstol(s, NULL, 2);
    default:
        break;
    }

    return 0L;
}

void calc_ftoa(CALC *calc, calcfloat r, TCHAR *buf)
{
    // converts from decimal calcfloat to another base

    switch (calc->numBase) {
    case NBASE_DECIMAL:
        _stprintf(buf, FMT_DESC_FLOAT, r);
        break;
    case NBASE_HEX:
        _stprintf(buf, TEXT("%lX"), (long)r);
        break;
    case NBASE_OCTAL:
        _stprintf(buf, TEXT("%lo"), (long)r);
        break;
    case NBASE_BINARY: // 911 - need routine here

        break;
    default:
        break;
    }
}

int parse(int wParam, int lParam)
{
    switch (wParam) {
    case TEXT('\b'): // backspace

        if (calc.buffer[0])
            calc.buffer[_tcslen(calc.buffer)-1] = TEXT('\0');
        break;

    case TEXT('\x1b'): // ESC

        calc.next = 1;
        calc.buffer[0] = TEXT('\0');
        calc.value = 0;
        calc.init = 1;
        break;

    case TEXT('0'):
    case TEXT('1'):
    case TEXT('2'):
    case TEXT('3'):
    case TEXT('4'):
    case TEXT('5'):
    case TEXT('6'):
    case TEXT('7'):
    case TEXT('8'):
    case TEXT('9'):
    case TEXT('a'):
    case TEXT('b'):
    case TEXT('c'):
    case TEXT('d'):
    case TEXT('e'):
    case TEXT('f'):
    case TEXT('A'):
    case TEXT('B'):
    case TEXT('C'):
    case TEXT('D'):
    case TEXT('E'):
    case TEXT('F'):
        {
            TCHAR s22[CALC_BUF_SIZE];
            TCHAR w = (TCHAR)LOWORD(wParam);

            if (!keys[calc.numBase][(WORD)w]) {
               MessageBeep(0);
               return 0;
            }

            if (calc.next) { // user first digit indicates new buffer needed after previous UI event
               calc.next      = 0;
               calc.buffer[0] = TEXT('\0');
            }
            calc.newenter = 1;

            _stprintf(s22,TEXT("%C"), w);
            _tcscat(calc.buffer, s22);
            //MessageBox(NULL, s22, NULL, 0);
        }
        break;

    case TEXT('.'):
    case TEXT(','): // 911 - need to handle this, i18n

        if (calc.numBase == NBASE_DECIMAL) {
            if (calc.next) { // first digit indicates new buffer needed after previous UI event
               calc.next      = 0;
               calc.buffer[0] = TEXT('\0');
            }

            _tcscat(calc.buffer, TEXT("."));
        }
        else {
            MessageBeep(0);
            return 0;
        }
        break;

    case TEXT('x'):
    case TEXT('X'): // exp, e(1)=2.718

        if (calc.numBase == NBASE_DECIMAL) {
            calc.next = 1;
            calc.value = exp(calc_atof(calc.buffer, calc.numBase));
            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value);
        }
        else {
            MessageBeep(0);
            return 0;
        }
        break;

    case TEXT('l'):
    case TEXT('L'):
        calc.next = 1;
        if (calc_atof(calc.buffer, calc.numBase) == 0.0L) {
            _tcscpy(calc.buffer, err_invalid);
        }
        else {
            calc.value = log10(calc_atof(calc.buffer, calc.numBase));
            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value);
        }
        break;

    case TEXT('N'):
    case TEXT('n'): // ln is natural logarithm

        calc.next = 1;
        if (calc_atof(calc.buffer, calc.numBase) == 0.0L) {
            _tcscpy(calc.buffer, err_invalid);
        }
        else {
            calc.value = log(calc_atof(calc.buffer, calc.numBase));
            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value);
        }
        break;

    case TEXT('p'):
    case TEXT('P'):
        if (calc.numBase == NBASE_DECIMAL) {
            calc.next = 1;
            _stprintf(calc.buffer, FMT_DESC_FLOAT, CONST_PI);
        }
        else {
            MessageBeep(0);
            return 0;
        }
        break;

    case TEXT('\x0c'): // Ctrl+L MC (only need to invalid rectangle if wasn't already zero, no need to update display)

        calc.next = 1;
        if (calc.memory)  {
            calc.memory = 0.0;
            InvalidateRect(calc.hWnd, &rFiller, TRUE);
            UpdateWindow(calc.hWnd);
        }

        return 0;
        break;

    case TEXT('\x12'): // Ctrl+R MR (value doesn't change, so no invalid rectangle. but display is updated.)

        calc.next = 1;
        if (calc.memory != calc_atof(calc.buffer, calc.numBase))
            _stprintf(calc.buffer,FMT_DESC_FLOAT,calc.memory);
        else
            return 0;
        break;

    case TEXT('\x10'): // Ctrl+P M+ (need to invalidate rectangle in many cases but not display)

        calc.next = 1;
        InvalidateRect(calc.hWnd, &rFiller, TRUE);
        calc.memory += calc_atof(calc.buffer, calc.numBase);
        return 0;
        break;

    case TEXT('\x0d'): // Ctrl+M  MS (only need to invalid rectangle if was zero and now not,

        // or was not zero and now is, not display)
        calc.next = 1;
        {
            int x = GetKeyState(VK_CONTROL);

            if (x < 0 || lParam == NUMBER_OF_THE_BEAST) {
                if ((!calc.memory && calc_atof(calc.buffer, calc.numBase)) ||
                        (calc.memory && !calc_atof(calc.buffer, calc.numBase))) {

                    calc.memory = calc_atof(calc.buffer, calc.numBase);
                    InvalidateRect(calc.hWnd, &rFiller, FALSE);
                    UpdateWindow(calc.hWnd);
                }
                else {
                    calc.memory = calc_atof(calc.buffer, calc.numBase);
                }
                return 0;
            }
        }
        // fall through for Enter processing

    case TEXT('='):
        {
            calcfloat r = calc.operand;

            if (calc.newenter) {
                calc.newenter = 0;
                calc.operand = calc_atof(calc.buffer, calc.numBase);
                r = calc_atof(calc.buffer, calc.numBase); // convert buffer from whatever base to decimal real
            }

            if (calc.oper == TEXT('+')) {
                calc.value += r;
            }
            else if (calc.oper == TEXT('-')) {
                calc.value -= r;
            }
            else if (calc.oper == TEXT('*')) {
                calc.value *= r;
            }
            else if (calc.oper == TEXT('%')) {
                calc.value = (long)calc.value % (long)r;
            }
            else if (calc.oper == TEXT('/')) {
                if (!calc_atof(calc.buffer, calc.numBase)) {
                    _tcscpy(calc.buffer, err_undefined);
                    calc.err = 1;
                }
                else {
                    calc.value /= r;
                }
            }
            else if (calc.oper == TEXT('&')) {
                calc.value = (calcfloat)((long)calc.value & (long)r);
            }
            else if (calc.oper == TEXT('|')) {
                calc.value = (calcfloat)((long)calc.value | (long)r);
            }
            else if (calc.oper == TEXT('^')) {
                calc.value = (calcfloat)((long)calc.value ^ (long)r);
            }
            else if (calc.oper == TEXT('y')) {
                calc.value = (calcfloat)pow(calc.value, r);
            }
            else if (calc.oper == TEXT('<')) {
                if (calc.invMode)
                   calc.value = (calcfloat)((long)calc.value >> (long)calc_atof(calc.buffer, calc.numBase));
                else
                   calc.value = (calcfloat)((long)calc.value << (long)calc_atof(calc.buffer, calc.numBase));
            }
            else { // must have been a gratuitous user =

                calc.value = calc_atof(calc.buffer, calc.numBase);
            }

            calc_ftoa(&calc, calc.value, calc.buffer);

            if (!calc.next)
                // calc.value = r;
                calc.next = 1;
        }
        break;

    case TEXT('R'):
    case TEXT('r'): // 1/x

        calc.next = 1;
        if (calc_atof(calc.buffer, calc.numBase) == 0) {
            calc.err = 1;
            _tcscpy(calc.buffer, err_divide_by_zero);
        }
        else {
            calc.value = 1/calc_atof(calc.buffer, calc.numBase);
            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        }
        break;

    case TEXT('@'): // ^2 - sqrt in standard mode, squared in sci mode

        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);

        if (!calc.sciMode)
           calc.value *= calc.value;
        else
           calc.value = sqrt(calc.value);

        _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        break;

    case TEXT('#'): // ^3

        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);
        calc.value *= calc.value * calc.value;
        _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        break;

    case TEXT('Y'):
    case TEXT('y'): // x^y

        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);
        calc.oper = TEXT('y');
        calc.next = 1;
        break;

    case TEXT('<'): // Lsh

        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);
        calc.oper = TEXT('<');
        calc.next = 1;
        break;

    case TEXT(';'): // INT

        calc.next = 1;
        calc.value = (long)calc_atof(calc.buffer, calc.numBase);
        _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        break;

    case TEXT('!'): // factorial, need to use gamma function for reals t^(z-1)*e^t dt

        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);
        calc.value = (calcfloat)factorial((long)calc.value);
        _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        break;

    case TEXT('&'): // bitwise and

        calc.oper = '&';
        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);
        break;

    case TEXT('|'): // bitwise or

        calc.oper = TEXT('|');
        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);
        break;

    case TEXT('^'): // bitwise xor

        calc.oper = TEXT('^');
        calc.next = 1;
        calc.value = calc_atof(calc.buffer, calc.numBase);
        break;

    case TEXT('~'): // bitwise not

        calc.next = 1;
        calc.value = ~ (long) calc_atof(calc.buffer, calc.numBase);
        _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        break;

    case TEXT('+'):
        calc.operand = calc_atof(calc.buffer, calc.numBase);
        calc.value   += calc.operand;
        calc_ftoa(&calc, calc.value, calc.buffer);
        calc.oper    = TEXT('+');
        calc.newenter = 1;
        calc.next = 1;
        break;

    case TEXT('-'):
        if (calc.init) {
            calc.init = 0;
            calc.operand = calc_atof(calc.buffer, calc.numBase);
            calc.value = calc.operand;
        }
        else {
            calc.value   -= calc.operand;
            calc_ftoa(&calc, calc.value, calc.buffer);
        }

        calc.oper    = TEXT('-');
        calc.newenter = 1;
        calc.next = 1;
        break;

    case TEXT('*'):
        if (calc.init) {
            calc.init = 0;
            calc.operand = calc_atof(calc.buffer, calc.numBase);
            calc.value = calc.operand;
        }
        else {
            calc.value   *= calc.operand;
            calc_ftoa(&calc, calc.value, calc.buffer);
        }
        calc.oper    = TEXT('*');
        calc.newenter = 1;
        calc.next = 1;
        break;

    case TEXT('/'):
        if (calc.init) {
            calc.init = 0;
            calc.operand = calc_atof(calc.buffer, calc.numBase);
            calc.value = calc.operand;
        }
        else {
           calc.value   /= calc.operand;
           calc_ftoa(&calc, calc.value, calc.buffer);
        }
        calc.oper    = TEXT('/');
        calc.newenter = 1;
        calc.next = 1;
        break;

    case TEXT('%'):
        if (!calc.sciMode) {
            if (calc.init) {
               calc.init = 0;
               calc.operand = calc_atof(calc.buffer, calc.numBase);
               calc.value = calc.operand;
            }
            else {
               calc.value = (long)calc_atof(calc.buffer, calc.numBase) % (long)calc.operand;
               calc_ftoa(&calc, calc.value, calc.buffer);
            }
        }
        else {
            calcfloat r;
            r = calc_atof(calc.buffer, calc.numBase);
            calc.next = 1;
            _stprintf(calc.buffer, FMT_DESC_FLOAT, r * calc.value / (calcfloat)100.0);
        }

        calc.oper    = TEXT('%');
        calc.newenter = 1;
        calc.next = 1;
        break;

    case TEXT('O'): // cos

    case TEXT('o'):
        if (calc.numBase == NBASE_DECIMAL) {

            calcfloat r;
            calc.next = 1;
            r = calc_convert_to_radians(&calc);
            if (calc.hypMode && calc.invMode)
                calc.value = acosh(r);
            else if (calc.invMode)
                calc.value = acos(r);
            else if (calc.hypMode)
                calc.value = cosh(r);
            else
                calc.value = cos(calc_atof(calc.buffer, calc.numBase));
            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        }
        else {
            MessageBeep(0);
            return 0;
        }
        break;

    case TEXT('S'): // sin

    case TEXT('s'):
        if (calc.numBase == NBASE_DECIMAL) {

            calcfloat r = calc_convert_to_radians(&calc);
            calc.next = 1;
            if (calc.hypMode && calc.invMode)
                calc.value = asinh(r);
            else if (calc.invMode)
                calc.value = asin(r);
            else if (calc.hypMode)
                calc.value = sinh(r);
            else
                calc.value = sin(r);

            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        }
        else {
            MessageBeep(0);
            return 0;
        }
        break;

    case TEXT('T'): // tan

    case TEXT('t'):
        if (calc.numBase == NBASE_DECIMAL) {
            calcfloat r = calc_convert_to_radians(&calc);
            calc.next = 1;
            if (calc.hypMode && calc.invMode)
                calc.value = atanh(r);
            else if (calc.invMode)
                calc.value = atan(r);
            else if (calc.hypMode)
                calc.value = tanh(r);
            else
                calc.value = tan(r);

            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        }
        else {
            MessageBeep(0);
            return 0;
        }
        break;

    case TEXT('M'): // dms = Degrees Minutes Seconds

    case TEXT('m'):
        if (calc.numBase == NBASE_DECIMAL) {
            calcfloat r2;
            calcfloat r  = calc_atof(calc.buffer, calc.numBase);
            calc.next = 1;
            r2 = r - (long)r;

            r = (long)r + r2 * 0.6; // multiply by 60 and divide by 100
            calc.value = r;
            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        }
        else {
            MessageBeep(0);
            return 0;
        }
        break;

    case TEXT('V'): // toggle scientic notation like 1.e+2 or 100

    case TEXT('v'):
        calc.displayMode = !calc.displayMode;
        break;

// non-standard keystrokes ...

    case TEXT('?'):

        calc.next = 1;
        if (calc_atof(calc.buffer, calc.numBase) < 0) {
            calc.err = 1;
            _tcscpy(calc.buffer, err_invalid);
        }
        else {
            calc.value = sqrt(calc_atof(calc.buffer, calc.numBase));
            _stprintf(calc.buffer, FMT_DESC_FLOAT, calc.value );
        }
        break;

    case TEXT('Z'): // +/-

        {
            TCHAR s[CALC_BUF_SIZE] = TEXT("-");

            if (!_tcscmp(calc.buffer, TEXT("0")))
                return 0;

            if (calc.buffer[0] == TEXT('-'))
                _tcscpy(s, calc.buffer+1);
            else
                _tcscpy(s+1, calc.buffer);

            _tcscpy(calc.buffer, s);
        }
        break;

    case TEXT('G'): // debug mode

    case TEXT('g'):
        calc.next = 1;
        debug = !debug;
        break;

    default:
        MessageBeep(0);
        return 0;
        break;
    }

    return 0;
}

/*
    case WM_CONTEXTMENU: // need to subclass control and just call WinHelp!
        WinHelp((HWND) wParam, "c:/windows/help/calc.hlp", HELP_CONTEXTMENU, (DWORD)(LPVOID)2);
        return 0;
        }
*/
