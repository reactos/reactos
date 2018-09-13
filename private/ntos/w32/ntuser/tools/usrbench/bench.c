
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   bench.c

Abstract:

   Measure time for a number of API calls or small application modules

Author:

   Dan Almosnino (danalm) July 96
   Based on code by (MarkE) and DarrinM

Enviornment:

   User Mode

Revision History:

    Dan Almosnino (danalm) 20-Sept-195

1. Timer call modified to run on both NT and WIN95.

    Dan Almosnino (danalm) 20-Nov-1995

1.  Modified Timer call to measure Pentium cycle counts when applicable
2.  Modified default numbers for test iterations to accomodate the statistical module add-on.
    (Typically 10 test samples are taken, doing 10-1000 iterations each).

    Dan Almosnino (danalm) 25-July-96

1.  Adapted to the same format as that of GDIbench, including the features above.
2.  Cleanup some tests.

--*/

#include "precomp.h"
#include "bench.h"
#include "usrbench.h"
#include "resource.h"

#ifndef WIN32
#define APIENTRY FAR PASCAL
typedef int INT;
typedef char CHAR;
#endif

#define SETWINDOWLONGVAL  99999L
/*
 * Function prototypes
 */
INT_PTR APIENTRY FileOpenDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY ClearDlg(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY ClearDlgNoState(HWND, UINT, WPARAM, LPARAM);
LRESULT APIENTRY CreateDestroyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY CreateDestroyWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(expr)    if (!(expr)) { char szBuf[256];                   \
sprintf(szBuf,"Assertion Failure %s %ld:"#expr"\n", __FILE__, __LINE__);  \
MessageBox(NULL, szBuf, "Assert Failure", MB_OK); }


static void DispErrorMsg(const char* title)
{
    LPVOID msgbuf;

    if (title == NULL) {
        title = "Error";
    }
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &msgbuf,
        0,
        NULL);
    MessageBox(NULL, msgbuf, title, MB_OK | MB_ICONEXCLAMATION);
    LocalFree(msgbuf);
}

/*
 * Global variables
 */
CHAR *aszTypical[] = {
    "change", "alteration", "amendment", "mutation", "permutation",
    "variation", "substitution", "modification", "transposition",
    "transformation"
};
INT NStrings = sizeof(aszTypical)/sizeof(aszTypical[0]);

BOOL gfSetFocus = TRUE;
#define PROPCLASSNAME TEXT("PropWindow")

/*
 * External global variables.
 */

extern HWND ghwndFrame, ghwndMDIClient;
extern HANDLE ghinst;

/*++

Routine Description:

    Measure start count

Arguments



Return Value - Performance Count


--*/
extern BOOL gfPentium;
_int64
BeginTimeMeasurement()
{
    _int64 PerformanceCount;
    extern BOOL gfUseCycleCount;

#ifdef _X86_
                SYSTEM_INFO SystemInfo;
                GetSystemInfo(&SystemInfo);
                if(gfUseCycleCount&&(PROCESSOR_INTEL_PENTIUM==SystemInfo.dwProcessorType))
                        gfPentium = TRUE;
                else
#endif
                        gfPentium = FALSE;
#ifdef _X86_
                if(gfPentium)
                    PerformanceCount = GetCycleCount();
                else
#endif
                QueryPerformanceCounter((LARGE_INTEGER *)&PerformanceCount);

    return(PerformanceCount);
}

/*++

Routine Description:

    Measure stop count and return the calculated time difference

Arguments

    StartTime   = Start Time Count
    Iter        = No. of Test Iterations

Return Value - Test Time per Iteration, in 100 nano-second units


--*/

ULONGLONG
EndTimeMeasurement(
    _int64  StartTime,
    ULONG      Iter)
{

   _int64 PerformanceCount;
   extern  _int64 PerformanceFreq;
   extern  BOOL gfPentium;

#ifdef _X86_
                if(gfPentium)
                {
                    PerformanceCount = GetCycleCount();
                    PerformanceCount -= CCNT_OVERHEAD;
                }
                else
#endif
                    QueryPerformanceCounter((LARGE_INTEGER *)&PerformanceCount);

   PerformanceCount -= StartTime ;

#ifdef _X86_
                if(gfPentium)
                    PerformanceCount /= Iter;
                else
#endif
                    PerformanceCount /= (PerformanceFreq * Iter); // Result is in 100ns units
                                                                  // because PerformanceFreq
                                                                  // was set to Counts/(100ns)
   return((ULONGLONG)PerformanceCount);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*++

Routine Description:

   Measure APIs

Arguments

   hdc   - dc
   iter  - number of times to call

Return Value

   time for calls

--*/

/***************************************************************************\
* RegisterClass
*
*
* History:
\***************************************************************************/

ULONGLONG
msProfRegisterClass(
    HDC hdc,
    ULONG Iter)
{
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = PROPCLASSNAME;

    START_TIMER;

    while (ix--)
    {
        RegisterClass(&wc);
        UnregisterClass(PROPCLASSNAME, ghinst);
    }

    END_TIMER;

}


/***************************************************************************\
* Class Query APIs
*
*
* History:
\***************************************************************************/

ULONGLONG msProfClassGroup(HDC hdc, ULONG Iter)
{

    char szBuf[50];
    HICON hIcon;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = hIcon = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = PROPCLASSNAME;
    RegisterClass(&wc);

    START_TIMER;

    while (ix--)
    {
        GetClassInfo(ghinst, PROPCLASSNAME, &wc);
        GetClassName(ghwndFrame, szBuf, sizeof(szBuf)/sizeof(szBuf[0]));
        GetClassLongPtr(ghwndFrame, GCLP_HBRBACKGROUND);
        SetClassLongPtr(ghwndFrame, GCLP_HICON, (LONG_PTR)hIcon);
    }

    END_TIMER_NO_RETURN;

    UnregisterClass(PROPCLASSNAME, ghinst);

    RETURN_STOP_TIME;
}


/***************************************************************************\
* Clipboard APIs tests
*
*
* History:
\***************************************************************************/

ULONGLONG msProfClipboardGroup(HDC hdc, ULONG Iter)
{

    INIT_TIMER;
    HGLOBAL h;
    int i;

    START_TIMER;

    while (ix--)
    {
        START_OVERHEAD;
        h = GlobalAlloc(GPTR | GMEM_DDESHARE, 80);
        strcpy( h, "hello");
        END_OVERHEAD;

        OpenClipboard(ghwndFrame);
        EmptyClipboard();
        SetClipboardData(CF_TEXT, h);
        GetClipboardData(CF_TEXT);
        CloseClipboard();
    }

    END_TIMER_NO_RETURN;
    RETURN_STOP_TIME;
}


/***************************************************************************\
* ProfAvgDlgDraw
*
*
* History:
\***************************************************************************/

ULONGLONG msProfAvgDlgDraw(HDC hdc, ULONG Iter)
{

    CHAR szFile[128];
    INIT_TIMER;

    HWND hwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(CLEARBOX), ghwndFrame,
            ClearDlg, MAKELONG(szFile,0));

    START_TIMER;

    while (ix--)
    {
        ShowWindow(hwnd, TRUE);
        UpdateWindow(hwnd);
        ShowWindow(hwnd, FALSE);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);

    RETURN_STOP_TIME;

}


ULONGLONG msProfAvgDlgCreate(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    CHAR szFile[128];
    INIT_TIMER;

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(CLEARBOX), ghwndFrame,
                ClearDlg, MAKELONG(szFile,0));
        ShowWindow(hwnd, TRUE);
        UpdateWindow(hwnd);
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    RETURN_STOP_TIME;
}


ULONGLONG msProfAvgDlgCreateDestroy(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    CHAR szFile[128];
    INIT_TIMER;

    gfSetFocus = FALSE;     // Trying to minimize GDI's impact.

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(CLEARBOX), ghwndFrame,
                ClearDlg, MAKELONG(szFile,0));
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    gfSetFocus = TRUE;

    RETURN_STOP_TIME;

}


ULONGLONG msProfAvgDlgCreateDestroyNoMenu(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    CHAR szFile[128];
    INIT_TIMER;

    gfSetFocus = FALSE;     // Trying to minimize GDI's impact.

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(CLEARBOXNOMENU), ghwndFrame,
                ClearDlg, MAKELONG(szFile,0));
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    gfSetFocus = TRUE;

    RETURN_STOP_TIME;
}


ULONGLONG msProfAvgDlgCreateDestroyNoFont(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    CHAR szFile[128];
    INIT_TIMER;

    gfSetFocus = FALSE;     // Trying to minimize GDI's impact.

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(CLEARBOXNOFONT), ghwndFrame,
                ClearDlg, MAKELONG(szFile,0));
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    gfSetFocus = TRUE;

    RETURN_STOP_TIME;

}


ULONGLONG msProfAvgDlgCreateDestroyEmpty(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    CHAR szFile[128];
    INIT_TIMER;

    gfSetFocus = FALSE;     // Trying to minimize GDI's impact.

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateDialogParam(ghinst, MAKEINTRESOURCE(EMPTY), ghwndFrame,
                ClearDlgNoState, MAKELONG(szFile,0));
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    gfSetFocus = TRUE;

    RETURN_STOP_TIME;

}



ULONGLONG msProfCreateDestroyWindow(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    INIT_TIMER;
    WNDCLASS wc;

    if(!ghinst)MessageBox(GetParent(ghwndMDIClient), "1ghinst==0","ERROR!", MB_OK);


    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";


    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "1RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }


    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("CreateDestroyWindow", "Create/DestroyWindow test",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                NULL, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            break;
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCreateDestroyChildWindow(HDC hdc, ULONG Iter)
{

    HWND hwnd, hwndParent;
    INIT_TIMER;

    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "2RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("CreateDestroyWindow", "Create/DestroyWindow test",
                WS_CHILD,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                hwndParent, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            break;
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;
}


LRESULT APIENTRY CreateDestroyWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg) {
    case WM_SETTEXT:
    case WM_GETTEXTLENGTH:
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);

    }

    return 0;
}

LRESULT APIENTRY CreateDestroyWndProcW(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg) {
    case WM_SETTEXT:
    case WM_GETTEXTLENGTH:
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}


ULONGLONG msProfLocalAllocFree(HDC hdc, ULONG Iter)
{

    HANDLE h1, h2, h3, h4, h5;
    INIT_TIMER;

    START_TIMER;

    // Try to stress the heap a little bit more than just doing a
    // series of Alloc/Frees.

    while (ix--)                         // originally ix<Iter/5 as well
    {
        h1 = LocalAlloc(0, 500);
        h2 = LocalAlloc(0, 600);
        h3 = LocalAlloc(0, 700);
        LocalFree(h2);
        h4 = LocalAlloc(0, 1000);
        h5 = LocalAlloc(0, 100);
        LocalFree(h1);
        LocalFree(h3);
        LocalFree(h4);
        LocalFree(h5);
    }

    END_TIMER;
}


ULONGLONG msProfGetWindowLong(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    INIT_TIMER;
    LONG l;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 4;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "3RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwnd = CreateWindow("CreateDestroyWindow", "Create/DestroyWindow test",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, ghinst, NULL);

    if (hwnd == NULL) {
        MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    START_TIMER;

    while (ix--)
        l = GetWindowLong(hwnd, 0);

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfSetWindowLong(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    INIT_TIMER;
    LONG l;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 4;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "4RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwnd = CreateWindow("CreateDestroyWindow", "Create/DestroyWindow test",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, ghinst, NULL);

    if (hwnd == NULL) {
        MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    START_TIMER;

    while (ix--)
        l = SetWindowLong(hwnd, 0, SETWINDOWLONGVAL);

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCreateDestroyListbox(HDC hdc, ULONG Iter)
{

    HWND hwnd, hwndParent;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "5RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("ListBox", NULL, WS_CHILD | LBS_STANDARD | WS_VISIBLE,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCreateDestroyButton(HDC hdc, ULONG Iter)
{

    HWND hwnd, hwndParent;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "6RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("Button", "OK", WS_CHILD | BS_PUSHBUTTON,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCreateDestroyCombobox(HDC hdc, ULONG Iter)
{

    HWND hwnd, hwndParent;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "7RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("Combobox", NULL, WS_CHILD | CBS_SIMPLE | CBS_HASSTRINGS | WS_VISIBLE,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCreateDestroyEdit(HDC hdc, ULONG Iter)
{

    HWND hwnd, hwndParent;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "8RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("Edit", NULL, WS_CHILD | WS_VISIBLE,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCreateDestroyStatic(HDC hdc, ULONG Iter)
{

    HWND hwnd, hwndParent;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "9RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("Static", "Hello", WS_CHILD | SS_SIMPLE,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCreateDestroyScrollbar(HDC hdc, ULONG Iter)
{

    HWND hwnd, hwndParent;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "10RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        hwnd = CreateWindow("Scrollbar", "Hello", WS_CHILD | SBS_HORZ | SBS_TOPALIGN,
                50, 50, 100, 100, hwndParent, NULL, ghinst, NULL);
        if (hwnd == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
        DestroyWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfListboxInsert1(HDC hdc, ULONG Iter)
{

    ULONG i;
    HWND hwndParent;
    INIT_TIMER;
    WNDCLASS wc;
    HWND *ahwnd = LocalAlloc(LPTR,Iter*sizeof(HANDLE));
    if(!ahwnd)
       MessageBox(GetParent(ghwndMDIClient),
        "Could not Allocate Memory for Handle Array",
        "ERROR!", MB_OK);

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "11RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    for (i=0; i < Iter; i++)
    {
        ahwnd[i] = CreateWindow("ListBox", NULL, WS_CHILD | LBS_STANDARD,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (ahwnd[i] == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
    }

    START_TIMER;

    while (ix--)
        for (i = 0; i < 200; i++)
            SendMessage(ahwnd[ix], LB_ADDSTRING, 0, (LPARAM)aszTypical[i % NStrings]);

    END_TIMER_NO_RETURN;

    for (i = 0; i < Iter; i++)
        DestroyWindow(ahwnd[i]);

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);
    LocalFree(ahwnd);

    RETURN_STOP_TIME;

}


ULONGLONG msProfListboxInsert2(HDC hdc, ULONG Iter)
{
    ULONG i;
    HWND hwndParent;
    INIT_TIMER;
    WNDCLASS wc;
    HWND *ahwnd = LocalAlloc(LPTR,Iter*sizeof(HANDLE));
    if(!ahwnd)
       MessageBox(GetParent(ghwndMDIClient),
        "Could not Allocate Memory for Handle Array",
        "ERROR!", MB_OK);

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "12RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    for (i = 0; i < Iter; i++) {
        ahwnd[i] = CreateWindow("ListBox", NULL,
                WS_CHILD | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (ahwnd[i] == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
    }

    START_TIMER;

    while (ix--)
        for (i = 0; i < 200; i++)
            SendMessage(ahwnd[ix], LB_ADDSTRING, 0, (LPARAM)aszTypical[i % NStrings]);

    END_TIMER_NO_RETURN;

    for (i = 0; i < Iter; i++)
        DestroyWindow(ahwnd[i]);

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);
    LocalFree(ahwnd);

    RETURN_STOP_TIME;

}


ULONGLONG msProfListboxInsert3(HDC hdc, ULONG Iter)
{
    ULONG i;
    HWND hwndParent;
    INIT_TIMER;
    WNDCLASS wc;
    HWND *ahwnd = LocalAlloc(LPTR,Iter*sizeof(HANDLE));
    if(!ahwnd)
       MessageBox(GetParent(ghwndMDIClient),
        "Could not Allocate Memory for Handle Array",
        "ERROR!", MB_OK);

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "13RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwndParent = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    for (i = 0; i < Iter; i++) {
        ahwnd[i] = CreateWindow("ListBox", NULL,
                WS_CHILD | LBS_SORT | LBS_OWNERDRAWFIXED,
                50, 50, 200, 250, hwndParent, NULL, ghinst, NULL);
        if (ahwnd[i] == NULL) {
            MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
                    "ERROR!", MB_OK);
            return (ULONGLONG)(0);
        }
    }

    START_TIMER;

    while (ix--)
        for (i = 0; i < 200; i++)
            SendMessage(ahwnd[ix], LB_ADDSTRING, 0, (LPARAM)aszTypical[i % NStrings]);

    END_TIMER_NO_RETURN;

    for (i = 0; i < Iter; i++)
        DestroyWindow(ahwnd[i]);

    DestroyWindow(hwndParent);
    UnregisterClass("CreateDestroyWindow", ghinst);
    LocalFree(ahwnd);

    RETURN_STOP_TIME;

}


/*
 * Mostly stolen from pbrush.
 */
INT_PTR APIENTRY ClearDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static BOOL bChangeOk = TRUE;

    HDC dc;
    INT wid, hgt, numcolors;

    switch (message) {
    case WM_INITDIALOG:
        /*
         * standard init stuff for pbrush
         */
        dc = GetDC(NULL);
        numcolors = GetDeviceCaps(dc, NUMCOLORS);
        ReleaseDC(NULL, dc);

        dc = GetDC(NULL);
        wid = GetDeviceCaps(dc, HORZRES);
        hgt = GetDeviceCaps(dc, VERTRES);
        ReleaseDC(NULL, dc);

        CheckRadioButton (hDlg, ID2, ID256, ID2);

        EnableWindow(GetDlgItem(hDlg, ID256), FALSE);
        CheckRadioButton(hDlg, ID2, ID256, ID256);

        CheckRadioButton(hDlg, ID2, ID256, ID2);
        EnableWindow(GetDlgItem(hDlg, ID256), FALSE);
        CheckRadioButton(hDlg, ID2, ID256, ID256);

        SetDlgItemInt(hDlg, IDWIDTH, 0, FALSE);
        SetDlgItemInt(hDlg, IDHEIGHT, 0, FALSE);
        CheckRadioButton(hDlg, IDIN, IDPELS, TRUE);

        if (!gfSetFocus)
            return FALSE;

        break;

    default:
        return FALSE;
    }

    return TRUE;
}


INT_PTR APIENTRY ClearDlgNoState(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static BOOL bChangeOk = TRUE;

    HDC dc;
    INT wid, hgt, numcolors;

    switch (message) {
    case WM_INITDIALOG:
        if (!gfSetFocus)
            return FALSE;

        break;

    default:
        return FALSE;
    }

    return TRUE;
}


ULONGLONG msProfSize(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    RECT rc;
    INIT_TIMER;

    GetClientRect(ghwndMDIClient, (LPRECT)&rc);
    InflateRect((LPRECT)&rc, -10, -10);

    hwnd = GetWindow(ghwndMDIClient, GW_CHILD);
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd, FALSE);
    UpdateWindow(hwnd);

    /* time start */

    START_TIMER;

    while (ix--)
    {
        SetWindowPos(hwnd, NULL, rc.left, rc.top,
                rc.right - rc.left, rc.bottom - rc.top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hwnd, NULL, rc.left, rc.top,
                (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2,
                SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /* time end */

    END_TIMER_NO_RETURN;

    ShowWindow(hwnd, SW_RESTORE);

    RETURN_STOP_TIME;

}


ULONGLONG msProfMove(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    RECT rc;
    INIT_TIMER;

    GetClientRect(ghwndMDIClient, (LPRECT)&rc);
    InflateRect((LPRECT)&rc, -(rc.right - rc.left) / 4,
            -(rc.bottom - rc.top) / 4);

    hwnd = GetWindow(ghwndMDIClient, GW_CHILD);
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd, FALSE);
    UpdateWindow(hwnd);

    /* time start */

    START_TIMER;

    while (ix--)
    {
        SetWindowPos(hwnd, NULL, rc.left, rc.top,
                rc.right - rc.left, rc.bottom - rc.top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
    }

    /* time end */

    END_TIMER_NO_RETURN;

    ShowWindow(hwnd, SW_RESTORE);

    RETURN_STOP_TIME;

}


#define WM_SYSTIMER 0x0118

ULONGLONG msProfMenu(HDC hdc, ULONG Iter)
{

    MSG msg;
    INIT_TIMER;

    HWND hwnd = GetParent(ghwndMDIClient);

    ShowWindow(hwnd, FALSE);

    /*
     * Multipad's edit menu is a great choice. Multipad does a goodly lot
     * of WM_INITMENU time initialization. The WM_SYSTIMER message is
     * to circumvent menu type ahead so the menu actually shows.
     */

    START_TIMER;

    while (ix--)
    {
        PostMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0L);
        PostMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0L);
        PostMessage(hwnd, WM_SYSTIMER, 0, 0L);
        SendMessage(hwnd, WM_SYSCOMMAND, SC_KEYMENU, (DWORD)(WORD)'e');
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);
    }

    END_TIMER_NO_RETURN;

    ShowWindow(hwnd, SW_RESTORE);
    UpdateWindow(hwnd);

    RETURN_STOP_TIME;

}



ULONGLONG msProfGetClientRect(HDC hdc, ULONG Iter)
{

    RECT rc;
    INIT_TIMER;

    START_TIMER;

    while (ix--)
    {
        GetClientRect(ghwndMDIClient, &rc);
    }

    END_TIMER;
}


ULONGLONG msProfScreenToClient(HDC hdc, ULONG Iter)
{

    POINT pt;
    INIT_TIMER;

    pt.x = 100;
    pt.y = 200;

    START_TIMER;

    while (ix--)
    {
        ScreenToClient(ghwndMDIClient, &pt);
    }

    END_TIMER;
}


ULONGLONG msProfGetInputState(HDC hdc, ULONG Iter)
{

    INIT_TIMER;

    START_TIMER;

    while (ix--)
    {
        GetInputState();
    }

    END_TIMER;
}


ULONGLONG msProfGetKeyState(HDC hdc, ULONG Iter)
{

    INIT_TIMER;

    START_TIMER;

    while (ix--)
    {
        GetKeyState(VK_ESCAPE);
    }

    END_TIMER;
}


ULONGLONG msProfGetAsyncKeyState(HDC hdc, ULONG Iter)
{

    INIT_TIMER;

    START_TIMER;

    while (ix--)
    {
        GetAsyncKeyState(VK_ESCAPE);
    }

    END_TIMER;
}


ULONGLONG msProfDispatchMessage(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    MSG msg;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "14RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwnd = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    msg.hwnd = hwnd;
    msg.message = WM_MOUSEMOVE;
    msg.wParam = 1;
    msg.lParam = 2;
    msg.time = 3;
    msg.pt.x = 4;
    msg.pt.y = 5;

    START_TIMER;

    while (ix--)
    {
        DispatchMessage(&msg);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfCallback(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    INIT_TIMER;
    WNDCLASSW wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProcW;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = L"CreateDestroyWindow";

    if (!RegisterClassW(&wc)) {
// Fails On Chicago
//        MessageBox(GetParent(ghwndMDIClient), "15RegisterClass call failed.",
//                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwnd = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}


ULONGLONG msProfSendMessage(HDC hdc, ULONG Iter)
{

    HWND hwnd;
    INIT_TIMER;
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "CreateDestroyWindow";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "16RegisterClass call failed.",
                "ERROR!", MB_OK);
        return (ULONGLONG)(0);
    }

    hwnd = CreateWindow("CreateDestroyWindow", NULL, WS_CHILD,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ghwndMDIClient, NULL, ghinst, NULL);

    START_TIMER;

    while (ix--)
    {
        SendMessage(hwnd, WM_MOUSEMOVE, 1, 2);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    UnregisterClass("CreateDestroyWindow", ghinst);

    RETURN_STOP_TIME;

}

HWND hwndShare;

DWORD SendMessageDiffThreadFunc(PVOID pdwData)
{
    WNDCLASS wc;
    MSG msg;
    BOOL b;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "SendMessageDiffThread";

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "19RegisterClass call failed.",
                "ERROR!", MB_OK);
        return FALSE;
    }

    hwndShare = CreateWindow("SendMessageDiffThread", NULL, 0,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            GetDesktopWindow(), NULL, ghinst, NULL);


    ASSERT(hwndShare);

    SetEvent(*((PHANDLE)pdwData));

    while (GetMessage(&msg, NULL, 0, 0)) {
        DispatchMessage(&msg);
    }

    b = DestroyWindow(hwndShare);

    ASSERT(b);

    b = UnregisterClass("SendMessageDiffThread", ghinst);

    ASSERT(b);

    return TRUE;
}

ULONGLONG msProfSendMessageDiffThread(HDC hdc, ULONG Iter)
{

    DWORD dwData;
    DWORD id;
    HANDLE hEvent;
    HANDLE hThread;
    INIT_TIMER;

    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    hwndShare = (HWND)0;

    hThread = CreateThread( NULL, 0, SendMessageDiffThreadFunc, &hEvent, 0, &id);

    WaitForSingleObject( hEvent, 20*1000);

    Sleep(10*1000);

    ASSERT(hwndShare);

    START_TIMER;

    while (ix--)
    {
        SendMessage(hwndShare, WM_MOUSEMOVE, 1, 2);
    }

    END_TIMER_NO_RETURN;

    PostThreadMessage(id, WM_QUIT, 0, 0);
    WaitForSingleObject(hThread, 2*1000); // Wait for Cleanup before Starting Next Cycle
    CloseHandle(hThread);

    RETURN_STOP_TIME;

}


ULONGLONG msProfUpdateWindow(HDC hDC, ULONG Iter)
{
    WNDCLASS wc;
    HWND hwnd;
    RECT rect;
    static LPCSTR className = "UpdateWindowTest";
    INIT_TIMER;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = className;

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "RegisterClass call failed.",
                "ERROR!", MB_OK);
        return FALSE;
    }

    hwnd = CreateWindow(className, NULL,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, ghinst, NULL);
    if (hwnd == NULL) {
        MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
            "ERROR!", MB_OK);
            return 0;
    }
    GetClientRect(hwnd, &rect);

    START_TIMER;
    while (ix--) {
        START_OVERHEAD;
        InvalidateRect(hwnd, &rect, FALSE);
        END_OVERHEAD;
        UpdateWindow(hwnd);
    }

    END_TIMER_NO_RETURN;

    DestroyWindow(hwnd);
    UnregisterClass(className, ghinst);

    RETURN_STOP_TIME;
}

ULONGLONG msProfTranslateMessage(HDC hDC, ULONG Iter)
{
    WNDCLASS wc;
    HWND hwnd;
    RECT rect;
    static LPCSTR className = "TranslateMessageTest";
    static MSG msgTemplate = {
        (HWND)NULL, WM_KEYDOWN,
        (WPARAM)0,
        (LPARAM)0,
        0,  // time
        {0, 0}, // mouse pointer
    };
    INIT_TIMER;

    wc.style            = 0;
    wc.lpfnWndProc      = CreateDestroyWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghinst;
    wc.hIcon            = LoadIcon(ghinst, (LPSTR)IDUSERBENCH);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = className;

    if (!RegisterClass(&wc)) {
        MessageBox(GetParent(ghwndMDIClient), "RegisterClass call failed.",
                "ERROR!", MB_OK);
        return FALSE;
    }

    hwnd = CreateWindow(className, NULL,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, ghinst, NULL);
    if (hwnd == NULL) {
        MessageBox(GetParent(ghwndMDIClient), "CreateWindow call failed.",
            "ERROR!", MB_OK);
            return 0;
    }
    msgTemplate.hwnd = hwnd;


    START_TIMER;
    while (ix--) {
        MSG msg = msgTemplate;
        TranslateMessage(&msg);
    }

    DestroyWindow(hwnd);
    UnregisterClass(className, ghinst);

    END_TIMER;
}

#define WIDE    1
#include "awtest.inc"
#undef WIDE
#include "awtest.inc"

#define A1  A
#define A2  A
#include "abtest.inc"
#define A1  A
#define A2  W
#include "abtest.inc"
#define A1  W
#define A2  A
#include "abtest.inc"
#define A1  W
#define A2  W
#include "abtest.inc"


/*********************************************************************/

#define TEST_DEFAULT 300

TEST_ENTRY  gTestEntry[] = {
    (PUCHAR)"AvgDialog draw",(PFN_MS)msProfAvgDlgDraw, 10, 0 ,
    (PUCHAR)"AvgDialog create/draw/destroy",(PFN_MS)msProfAvgDlgCreate, 10, 0 ,
    (PUCHAR)"AvgDialog create/destroy",(PFN_MS)msProfAvgDlgCreateDestroy, 100, 0 ,
    (PUCHAR)"AvgDialog(no font) create/destroy",(PFN_MS)msProfAvgDlgCreateDestroyNoFont, 100, 0 ,
    (PUCHAR)"AvgDialog(no menu) create/destroy",(PFN_MS)msProfAvgDlgCreateDestroyNoMenu, 100, 0 ,
    (PUCHAR)"AvgDialog(empty) create/destroy",(PFN_MS)msProfAvgDlgCreateDestroyEmpty, 200, 0 ,
    (PUCHAR)"SizeWindow",(PFN_MS)msProfSize, 200, 0 ,
    (PUCHAR)"MoveWindow",(PFN_MS)msProfMove, 2000, 0 ,
    (PUCHAR)"Create/DestroyWindow (top)",(PFN_MS)msProfCreateDestroyWindow, 1000, 0 ,
    (PUCHAR)"Create/DestroyWindow (child)",(PFN_MS)msProfCreateDestroyChildWindow, 1000, 0 ,

    (PUCHAR)"Create/Destroy Listbox",(PFN_MS)msProfCreateDestroyListbox, 1000, 0 ,
    (PUCHAR)"Create/Destroy Button",(PFN_MS)msProfCreateDestroyButton, 1000, 0 ,
    (PUCHAR)"Create/Destroy Combobox",(PFN_MS)msProfCreateDestroyCombobox, 1000, 0 ,
    (PUCHAR)"Create/Destroy Edit",(PFN_MS)msProfCreateDestroyEdit, 1000, 0 ,
    (PUCHAR)"Create/Destroy Static",(PFN_MS)msProfCreateDestroyStatic, 1000, 0 ,
    (PUCHAR)"Create/Destroy Scrollbar",(PFN_MS)msProfCreateDestroyScrollbar, 1000, 0 ,
    (PUCHAR)"SendMessage w/callback",(PFN_MS)msProfCallback, 4000, 0 ,
    (PUCHAR)"SendMessage",(PFN_MS)msProfSendMessage, 4000, 0 ,
    (PUCHAR)"SendMessage Ansi->Ansi text",(PFN_MS)msProfSendMessageAA, 4000, 0 ,
    (PUCHAR)"SendMessage Ansi->Unicode text",(PFN_MS)msProfSendMessageAW, 4000, 0 ,
    (PUCHAR)"SendMessage Unicode->Ansi text", (PFN_MS)msProfSendMessageWA, 4000, 0,
    (PUCHAR)"SendMessage Unicode->Unicode text", (PFN_MS)msProfSendMessageWW, 4000, 0,

    (PUCHAR)"SendMessage - DiffThread",(PFN_MS)msProfSendMessageDiffThread, 1000, 0 ,
    (PUCHAR)"SetWindowLong",(PFN_MS)msProfSetWindowLong, 400, 0 ,
    (PUCHAR)"GetWindowLong",(PFN_MS)msProfGetWindowLong, 2000, 0 ,

    (PUCHAR)"PeekMessageA",(PFN_MS)msProfPeekMessageA, 1000, 0 ,
    (PUCHAR)"PeekMessageW",(PFN_MS)msProfPeekMessageW, 1000, 0 ,
    (PUCHAR)"DispatchMessageA",(PFN_MS)msProfDispatchMessageA, 4000, 0 ,
    (PUCHAR)"DispatchMessageW",(PFN_MS)msProfDispatchMessageW, 4000, 0 ,

    (PUCHAR)"LocalAlloc/Free",(PFN_MS)msProfLocalAllocFree, 2000, 0 ,

    (PUCHAR)"200 Listbox Insert",(PFN_MS)msProfListboxInsert1, 20, 0 ,
    (PUCHAR)"200 Listbox Insert (ownerdraw)",(PFN_MS)msProfListboxInsert2, 20, 0 ,
    (PUCHAR)"200 Listbox Insert (ownerdraw/sorted)",(PFN_MS)msProfListboxInsert3, 20, 0 ,

    (PUCHAR)"GetClientRect",(PFN_MS)msProfGetClientRect, 8000, 0 ,
    (PUCHAR)"ScreenToClient",(PFN_MS)msProfScreenToClient, 8000, 0 ,
    (PUCHAR)"GetInputState",(PFN_MS)msProfGetInputState, 2000, 0 ,
    (PUCHAR)"GetKeyState",(PFN_MS)msProfGetKeyState, 2000, 0 ,
    (PUCHAR)"GetAsyncKeyState",(PFN_MS)msProfGetAsyncKeyState, 8000, 0 ,

    (PUCHAR)"Register|UnregisterClass",(PFN_MS)msProfRegisterClass, 500, 0 ,
    (PUCHAR)"GetClassInfo|Name|Long|SetClassLong",(PFN_MS)msProfClassGroup, 500, 0 ,

    (PUCHAR)"Menu pulldown",(PFN_MS)msProfMenu, 2000, 0 ,

    (PUCHAR)"Open|Empty|Set|Get|CloseClipboard",(PFN_MS)msProfClipboardGroup, 1000, 0 ,

    (PUCHAR)"GetWindowTextLengthA",(PFN_MS)msProfGetWindowTextLengthA, 10000, 0,
    (PUCHAR)"GetWindowTextLengthW",(PFN_MS)msProfGetWindowTextLengthW, 10000, 0,

    (PUCHAR)"UdpateWindow",(PFN_MS)msProfUpdateWindow, 10000, 0,
    (PUCHAR)"TranslateMessage", (PFN_MS)msProfTranslateMessage, 10000, 0,
    (PUCHAR)"IsCharUpperA", (PFN_MS)msProfCharUpperA, 2000, 0,
    (PUCHAR)"IsCharLowerA", (PFN_MS)msProfCharLowerA, 2000, 0,
    (PUCHAR)"IsCharUpperW", (PFN_MS)msProfCharUpperW, 2000, 0,
    (PUCHAR)"IsCharLowerW", (PFN_MS)msProfCharLowerW, 2000, 0,
    (PUCHAR)"CharNextA", (PFN_MS)msProfCharNextA, 2000, 0,
    (PUCHAR)"CharNextW", (PFN_MS)msProfCharNextW, 2000, 0,
    (PUCHAR)"GetMessageW", (PFN_MS)msProfGetMessageW, 5000, 0,
    (PUCHAR)"GetMessageA", (PFN_MS)msProfGetMessageA, 5000, 0,
// Add New Tests Here
};


ULONG gNumTests = sizeof gTestEntry / sizeof gTestEntry[0]; //Total No. of Tests
ULONG gNumQTests = 10;// No. of tests in Group 1 (starting from first in the list)
