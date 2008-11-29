
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/appswitch.c
 * PURPOSE:         app switching functionality
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "w32csr.h"
//#define NDEBUG
#include <debug.h>

typedef struct APPSWITCH_ITEM
{
    HWND hwndDlg;
    DWORD zPos;
    HICON hIcon;
    BOOL bFocus;
    struct APPSWITCH_ITEM * Next;
    WCHAR szText[1];
}APPSWITCH_ITEM, *PAPPSWITCH_ITEM;

static PAPPSWITCH_ITEM pRoot = NULL;
static DWORD NumOfWindows = 0;
static HWND hAppWindowDlg = NULL;
static HHOOK hhk = NULL;

BOOL
CALLBACK 
EnumWindowEnumProc(
    HWND hwnd,
    LPARAM lParam
)
{
    PAPPSWITCH_ITEM pItem;
    UINT Length;
    HICON hIcon;
    PAPPSWITCH_ITEM pCurItem;
    DWORD dwPid;
    HANDLE hProcess;
    WCHAR szFileName[MAX_PATH] = {0};

    /* check if the enumerated window is visible */
    if (!IsWindowVisible(hwnd))
        return TRUE;
    /* get window icon */
    hIcon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0);
    if (!hIcon)
    {
       GetWindowThreadProcessId(hwnd, &dwPid);
       hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, dwPid);
       if (hProcess)
       {
           if (GetModuleFileNameExW(hProcess, NULL, szFileName, MAX_PATH))
           {
               szFileName[MAX_PATH-1] = L'\0';
               ExtractIconExW(szFileName, 0, &hIcon, NULL, 1);
           }
       }
    }
    else
    {
       /* icons from WM_GETICON need to be copied */
       hIcon = CopyIcon(hIcon);
    }
    /* get the text length */
    Length = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
    /* allocate item structure for it */
    pItem = (PAPPSWITCH_ITEM)HeapAlloc(Win32CsrApiHeap, HEAP_ZERO_MEMORY, sizeof(APPSWITCH_ITEM) + Length * sizeof(WCHAR));
    if (!pItem)
        return TRUE;
    if (Length)
    {
        /* retrieve the window text when available */
        SendMessageW(hwnd, WM_GETTEXT, Length+1, (LPARAM)pItem->szText);
    }
    /* copy the window icon */
    pItem->hIcon = hIcon;
    /* store window handle */
    pItem->hwndDlg = hwnd;
    /* is the window the active window */
    if (GetActiveWindow() == hwnd)
        pItem->bFocus = TRUE;

    if (!pRoot)
    {
       /* first item */
       pRoot = pItem;
       return TRUE;
    }

    /* enumerate the last item */
    pCurItem = pRoot;
    while(pCurItem->Next)
        pCurItem = pCurItem->Next;

    /* insert it into the list */
    pCurItem->Next = pItem;
    NumOfWindows++;
    return TRUE;
}

VOID
EnumerateAppWindows(HDESK hDesk, HWND hwndDlg)
{
   /* initialize defaults */
   pRoot = NULL;
   NumOfWindows = 0;
   hAppWindowDlg = hwndDlg;
   /* enumerate all windows */
   EnumDesktopWindows(hDesk, EnumWindowEnumProc, (LPARAM)NULL);
   if (NumOfWindows > 7)
   {
       /* FIXME resize window */
   }
}

VOID
MarkNextEntryAsActive()
{
    PAPPSWITCH_ITEM pItem;

    pItem = pRoot;
    if (!pRoot)
       return;

    while(pItem)
    {
        if (pItem->bFocus)
        {
            pItem->bFocus = FALSE;
            if (pItem->Next)
                pItem->Next->bFocus = TRUE;
            else
                pRoot->bFocus = TRUE;
        }
        pItem = pItem->Next;
    }

    InvalidateRgn(hAppWindowDlg, NULL, TRUE);
}


LRESULT
CALLBACK
KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
)
{
   PKBDLLHOOKSTRUCT hk = (PKBDLLHOOKSTRUCT) lParam;

   if (wParam == WM_SYSKEYUP)
   {
       /* is tab key pressed */
       if (hk->vkCode == VK_TAB)
       {
          if (hAppWindowDlg == NULL)
          {
              /* FIXME 
               * launch window
               */
             DPRINT1("launch alt-tab window\n");
          }
          else
          {
              MarkNextEntryAsActive();
          }
       }
   }
   return CallNextHookEx(hhk, nCode, wParam, lParam);
}

VOID
PaintAppWindows(HWND hwndDlg, HDC hDc)
{
   DWORD dwIndex, X, Y;
   PAPPSWITCH_ITEM pCurItem;
   RECT Rect;
   DWORD XSize, YSize, XMax;
   HBRUSH hBrush;

   X = 10;
   Y = 10;
   XSize = GetSystemMetrics(SM_CXICON);
   YSize = GetSystemMetrics(SM_CYICON);
   XMax = (XSize+(XSize/2)) * 7 + X;
   pCurItem = pRoot;

   for (dwIndex = 0; dwIndex < NumOfWindows; dwIndex++)
   {
       if (X >= XMax)
       {
           X = 10;
           Y += YSize + (YSize/2);
       }
       if (pCurItem->bFocus)
       {
            hBrush = CreateSolidBrush(RGB(30, 30, 255));
            SetRect(&Rect, X-5, Y-5, X + XSize + 5, Y + YSize + 5);
            FillRect(hDc, &Rect, hBrush);
            DeleteObject((HGDIOBJ)hBrush);
            SendDlgItemMessageW(hwndDlg, IDC_STATIC_CUR_APP, WM_SETTEXT, 0, (LPARAM)pCurItem->szText);
       }

       DrawIcon(hDc, X, Y, pCurItem->hIcon);
       pCurItem = pCurItem->Next;
       X += XSize +(XSize/2);
   }
}
VOID
DestroyAppWindows()
{
   PAPPSWITCH_ITEM pCurItem, pNextItem;

   pCurItem = pRoot;
   while(pCurItem)
   {
       pNextItem = pCurItem->Next;
       DestroyIcon(pCurItem->hIcon);
       HeapFree(Win32CsrApiHeap, 0, pCurItem);
       pCurItem = pNextItem;
   }
   pRoot = NULL;
   hAppWindowDlg = NULL;
   NumOfWindows = 0;
}

INT_PTR
CALLBACK
SwitchWindowDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT Paint;
    HDESK hInput;

    switch (message)
    {
    case WM_INITDIALOG:
        hInput =  OpenInputDesktop(0,0, GENERIC_ALL);
        if (hInput)
        {
            EnumerateAppWindows(hInput, hwndDlg);
            CloseDesktop(hInput);
        }
        return TRUE;
    case WM_PAINT:
        BeginPaint(hwndDlg, &Paint);
        PaintAppWindows(hwndDlg, Paint.hdc);
        EndPaint(hwndDlg, &Paint);
        break;
    case WM_DESTROY:
        DestroyAppWindows();
        break;
    }
    return FALSE;
}

VOID
WINAPI
InitializeAppSwitchHook()
{
    hhk = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
    DPRINT("InitializeAppSwitchHook hhk %p\n", hhk);
}
