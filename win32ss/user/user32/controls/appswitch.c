/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/user32/controls/appswitch.c
 * PURPOSE:         app switching functionality
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  David Quintana (gigaherz@gmail.com)
 */

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

// limit the number of windows shown in the alt-tab window
// 120 windows results in (12*40) by (10*40) pixels worth of icons.
#define MAX_WINDOWS 120

// Global variables
HWND switchdialog = NULL;
HFONT dialogFont;
int selectedWindow = 0;
BOOL isOpen = FALSE;

int fontHeight=0;

WCHAR windowText[1024];

HWND windowList[MAX_WINDOWS];
HICON iconList[MAX_WINDOWS];
int windowCount = 0;

int cxBorder, cyBorder;
int nItems, nCols, nRows;
int itemsW, itemsH;
int totalW, totalH;
int xOffset, yOffset;
POINT pt;

void ResizeAndCenter(HWND hwnd, int width, int height)
{
   int screenwidth = GetSystemMetrics(SM_CXSCREEN);
   int screenheight = GetSystemMetrics(SM_CYSCREEN);

   pt.x = (screenwidth - width) / 2;
   pt.y = (screenheight - height) / 2;

   MoveWindow(hwnd, pt.x, pt.y, width, height, FALSE);
}

void MakeWindowActive(HWND hwnd)
{
   WINDOWPLACEMENT wpl;

   wpl.length = sizeof(WINDOWPLACEMENT);
   GetWindowPlacement(hwnd, &wpl);
  
   TRACE("GetWindowPlacement wpl.showCmd %d\n",wpl.showCmd);
   if (wpl.showCmd == SW_SHOWMINIMIZED)
      ShowWindowAsync(hwnd, SW_RESTORE);

   BringWindowToTop(hwnd);  // same as: SetWindowPos(hwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE); ?
   SetForegroundWindow(hwnd);
}

void CompleteSwitch(BOOL doSwitch)
{
   if (!isOpen)
      return;

   isOpen = FALSE;

   TRACE("[ATbot] CompleteSwitch Hiding Window.\n");
   ShowWindow(switchdialog, SW_HIDE);

   if(doSwitch)
   {
      if(selectedWindow >= windowCount)
         return;

      // FIXME: workaround because reactos fails to activate the previous window correctly.
      //if(selectedWindow != 0)
      {
         HWND hwnd = windowList[selectedWindow];
                  
         GetWindowTextW(hwnd, windowText, _countof(windowText));

         TRACE("[ATbot] CompleteSwitch Switching to 0x%08x (%ls)\n", hwnd, windowText);

         MakeWindowActive(hwnd);
      }
   }

   windowCount = 0;
}

BOOL CALLBACK EnumerateCallback(HWND window, LPARAM lParam)
{
   HICON hIcon;

   UNREFERENCED_PARAMETER(lParam);

   if (!IsWindowVisible(window))
            return TRUE;

   GetClassNameW(window, windowText, _countof(windowText));
   if ((wcscmp(L"Shell_TrayWnd", windowText)==0) ||
       (wcscmp(L"Progman", windowText)==0) )
            return TRUE;
      
   // First try to get the big icon assigned to the window
   hIcon = (HICON)SendMessageW(window, WM_GETICON, ICON_BIG, 0);
   if (!hIcon)
   {
      // If no icon is assigned, try to get the icon assigned to the windows' class
      hIcon = (HICON)GetClassLongPtrW(window, GCL_HICON);
      if (!hIcon)
      {
         // If we still don't have an icon, see if we can do with the small icon,
         // or a default application icon
         hIcon = (HICON)SendMessageW(window, WM_GETICON, ICON_SMALL2, 0);
         if (!hIcon)
         {
            // If all fails, give up and continue with the next window
            return TRUE;
         }
      }
   }

   windowList[windowCount] = window;
   iconList[windowCount] = CopyIcon(hIcon);

   windowCount++;

   // If we got to the max number of windows,
   // we won't be able to add any more
   if(windowCount == MAX_WINDOWS)
      return FALSE;

   return TRUE;
}

// Function mostly compatible with the normal EnumWindows,
// except it lists in Z-Order and it doesn't ensure consistency
// if a window is removed while enumerating
void EnumWindowsZOrder(WNDENUMPROC callback, LPARAM lParam)
{
    HWND next = GetTopWindow(NULL);
    while (next != NULL)
    {
        if(!callback(next, lParam))
         break;
        next = GetWindow(next, GW_HWNDNEXT);
    }
}

void ProcessMouseMessage(UINT message, LPARAM lParam)
{
   int xPos = LOWORD(lParam); 
   int yPos = HIWORD(lParam); 

   int xIndex = (xPos - xOffset)/40;
   int xOff   = (xPos - xOffset)%40;

   int yIndex = (yPos - yOffset)/40;
   int yOff   = (yPos - yOffset)%40;

   if(xOff > 32 || xIndex > nItems)
      return;

   if(yOff > 32 || yIndex > nRows)
      return;

   selectedWindow = (yIndex*nCols) + xIndex;
   if (message == WM_MOUSEMOVE)
   {
      InvalidateRect(switchdialog, NULL, TRUE);
      //RedrawWindow(switchdialog, NULL, NULL, 0);
   }
   else
   {
      selectedWindow = (yIndex*nCols) + xIndex;
      CompleteSwitch(TRUE);
   }
}

void OnPaint(HWND hWnd)
{
   HDC dialogDC;
   PAINTSTRUCT paint;
   RECT cRC, textRC;
   int i;
   HBRUSH hBrush;
   HPEN hPen;
   HFONT dcFont;
   COLORREF cr;
   int nch = GetWindowTextW(windowList[selectedWindow], windowText, _countof(windowText));

   dialogDC = BeginPaint(hWnd, &paint);
   {
      GetClientRect(hWnd, &cRC);
      FillRect(dialogDC, &cRC, GetSysColorBrush(COLOR_MENU));

      for(i=0; i< windowCount; i++)
      {
         HICON hIcon = iconList[i];
         
         int xpos = xOffset + 40 * (i % nCols);
         int ypos = yOffset + 40 * (i / nCols);

         if (selectedWindow == i)
         {
            hBrush = GetSysColorBrush(COLOR_HIGHLIGHT);
         }
         else
         {
            hBrush = GetSysColorBrush(COLOR_MENU);
         }
#if TRUE
         cr = GetSysColor(COLOR_BTNTEXT); // doesn't look right! >_<
         hPen = CreatePen(PS_DOT, 1, cr);
         SelectObject(dialogDC, hPen);
         SelectObject(dialogDC, hBrush);
         Rectangle(dialogDC, xpos-2, ypos-2, xpos+32+2, ypos+32+2);
         DeleteObject(hPen);
         // Must NOT destroy the system brush!
#else
         RECT rc = { xpos-2, ypos-2, xpos+32+2, ypos+32+2 };
         FillRect(dialogDC, &rc, hBrush);
#endif
         DrawIcon(dialogDC, xpos, ypos, hIcon);
      }

      dcFont = SelectObject(dialogDC, dialogFont);
      SetTextColor(dialogDC, GetSysColor(COLOR_BTNTEXT));
      SetBkColor(dialogDC, GetSysColor(COLOR_BTNFACE));

      textRC.top = itemsH;
      textRC.left = 8;
      textRC.right = totalW - 8;
      textRC.bottom = totalH - 8;
      DrawTextW(dialogDC, windowText, nch, &textRC, DT_CENTER|DT_END_ELLIPSIS);
      SelectObject(dialogDC, dcFont);
   }
   EndPaint(hWnd, &paint);
}

DWORD CreateSwitcherWindow(HINSTANCE hInstance)
{
    switchdialog = CreateWindowExW( WS_EX_TOPMOST|WS_EX_DLGMODALFRAME|WS_EX_TOOLWINDOW,
                                    WC_SWITCH,
                                    L"",
                                    WS_POPUP|WS_BORDER|WS_DISABLED,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    400, 150,
                                    NULL, NULL,
                                    hInstance, NULL);
    if (!switchdialog)
    {
       TRACE("[ATbot] Task Switcher Window failed to create.\n");
       return 0;
    }
                                
    isOpen = FALSE;
    return 1;
}
                                        
DWORD GetDialogFont()
{
   HDC tDC;
   TEXTMETRIC tm;

   dialogFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

   tDC = GetDC(0);
   GetTextMetrics(tDC, &tm);
   fontHeight = tm.tmHeight;
   ReleaseDC(0, tDC);

   return 1;
}

void PrepareWindow()
{
   cxBorder = GetSystemMetrics(SM_CXBORDER);
   cyBorder = GetSystemMetrics(SM_CYBORDER);
   
   nItems = windowCount;
   nCols = min(max(nItems,8),12);
   nRows = (nItems+nCols-1)/nCols;

   itemsW = nCols*32 + (nCols+1)*8;
   itemsH = nRows*32 + (nRows+1)*8;

   totalW = itemsW + 2*cxBorder + 4;
   totalH = itemsH + 2*cyBorder + fontHeight + 8; // give extra pixels for the window title

   xOffset = 8;
   yOffset = 8;

   if (nItems < nCols)
   {
      int w2 = nItems*32 + (nItems-1)*8;
      xOffset = (itemsW-w2)/2;
   }
   ResizeAndCenter(switchdialog, totalW, totalH);
}

void ProcessHotKey()
{
   if (!isOpen)
   {
      windowCount=0;
      EnumWindowsZOrder(EnumerateCallback, 0);

      if (windowCount < 2)
         return;

      selectedWindow = 1;

      TRACE("[ATbot] HotKey Received. Opening window.\n");
      ShowWindow(switchdialog, SW_SHOWNORMAL);
      MakeWindowActive(switchdialog);
      isOpen = TRUE;
   }
   else
   {
      TRACE("[ATbot] HotKey Received  Rotating.\n");
      selectedWindow = (selectedWindow + 1)%windowCount;
      InvalidateRect(switchdialog, NULL, TRUE);
   }
}

LRESULT WINAPI DoAppSwitch( WPARAM wParam, LPARAM lParam )
{
   HWND hwnd, hwndActive;
   MSG msg;
   BOOL Esc = FALSE;
   INT Count = 0;
   WCHAR Text[1024];

   // Already in the loop.
   if (switchdialog) return 0;

   hwndActive = GetActiveWindow();
   // Nothing is active so exit.
   if (!hwndActive) return 0;
   // Capture current active window.
   SetCapture( hwndActive );

   switch (lParam)
   {
      case VK_TAB:
         if( !CreateSwitcherWindow(User32Instance) ) goto Exit;
         if( !GetDialogFont() ) goto Exit;
         ProcessHotKey();
         break;

      case VK_ESCAPE:
         windowCount = 0;
         Count = 0;
         EnumWindowsZOrder(EnumerateCallback, 0);
         if (windowCount < 2) goto Exit;
         if (wParam == SC_NEXTWINDOW)
            Count = 1;
         else
         {
            if (windowCount == 2)
               Count = 0;
            else
               Count = windowCount - 1;
         }
         TRACE("DoAppSwitch VK_ESCAPE 1 Count %d windowCount %d\n",Count,windowCount);
         hwnd = windowList[Count];
         GetWindowTextW(hwnd, Text, _countof(Text));
         TRACE("[ATbot] Switching to 0x%08x (%ls)\n", hwnd, Text);
         MakeWindowActive(hwnd);
         Esc = TRUE;
         break;

      default:
         goto Exit;
   }
   // Main message loop:
   while (1)
   {
      for (;;)
      {
         if (PeekMessageW( &msg, 0, 0, 0, PM_NOREMOVE ))
         {
             if (!CallMsgFilterW( &msg, MSGF_NEXTWINDOW )) break;
             /* remove the message from the queue */
             PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
         }
         else
             WaitMessage();
      }

      switch (msg.message)
      {
        case WM_KEYUP:
        {
          PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
          if (msg.wParam == VK_MENU)
          {
             CompleteSwitch(TRUE);
          }
          else if (msg.wParam == VK_RETURN)
          {
             CompleteSwitch(TRUE);
          }
          else if (msg.wParam == VK_ESCAPE)
          {
             TRACE("DoAppSwitch VK_ESCAPE 2\n");
             CompleteSwitch(FALSE);
          }
          goto Exit; //break;
        }

        case WM_SYSKEYDOWN:
        {
          PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
          if (HIWORD(msg.lParam) & KF_ALTDOWN)
          {
             INT Shift;
             if ( msg.wParam == VK_TAB )
             {
                if (Esc) break;
                Shift = GetKeyState(VK_SHIFT) & 0x8000 ? SC_PREVWINDOW : SC_NEXTWINDOW;
                if (Shift == SC_NEXTWINDOW)
                {
                   selectedWindow = (selectedWindow + 1)%windowCount;
                }
                else
                {
                   selectedWindow = selectedWindow - 1;
                   if (selectedWindow < 0)
                      selectedWindow = windowCount - 1;
                }
                InvalidateRect(switchdialog, NULL, TRUE);
             }
             else if ( msg.wParam == VK_ESCAPE )
             {
                if (!Esc) break;
                if (windowCount < 2)
                   goto Exit;
                if (wParam == SC_NEXTWINDOW)
                {
                   Count = (Count + 1)%windowCount;
                }
                else
                {
                   Count--;
                   if (Count < 0)
                      Count = windowCount - 1;
                }
                hwnd = windowList[Count];
                GetWindowTextW(hwnd, Text, _countof(Text));
                MakeWindowActive(hwnd);
             }
          }
          break;
        }

        case WM_LBUTTONUP:
          PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
          ProcessMouseMessage(msg.message, msg.lParam);
          goto Exit;

        default:
          if (PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE ))
          {
             TranslateMessage(&msg);
             DispatchMessageW(&msg);
          }
          break;
      }
   }
Exit:
   ReleaseCapture();
   if (switchdialog) DestroyWindow(switchdialog);
   switchdialog = NULL;
   selectedWindow = 0;
   windowCount = 0;
   return 0;
}

VOID
DestroyAppWindows()
{
   INT i;
   for (i=0; i< windowCount; i++)
   {
      HICON hIcon = iconList[i];
      DestroyIcon(hIcon);
   }
}

//
// Switch System Class Window Proc.
//
LRESULT WINAPI SwitchWndProc_common(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
   PWND pWnd;
   PALTTABINFO ati;
   pWnd = ValidateHwnd(hWnd);
   if (pWnd)
   {
      if (!pWnd->fnid)
      {
         NtUserSetWindowFNID(hWnd, FNID_SWITCH);
      }
   }    

   switch (uMsg)
   {
      case WM_NCCREATE:
         if (!(ati = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ati))))
            return 0;
         SetWindowLongPtrW( hWnd, 0, (LONG_PTR)ati );
         return TRUE;

      case WM_SHOWWINDOW:
         if (wParam)
         {
            PrepareWindow();
            ati = (PALTTABINFO)GetWindowLongPtrW(hWnd, 0);
            ati->cItems = nItems;
            ati->cxItem = ati->cyItem = 43;
            ati->cRows = nRows;
            ati->cColumns = nCols;
         }
         return 0;

      case WM_MOUSEMOVE:
         ProcessMouseMessage(uMsg, lParam);
         return 0;

      case WM_ACTIVATE:
         if (wParam == WA_INACTIVE)
         {
            CompleteSwitch(FALSE);
         }
         return 0;

      case WM_PAINT:
         OnPaint(hWnd);
         return 0;

      case WM_DESTROY:
         isOpen = FALSE;
         ati = (PALTTABINFO)GetWindowLongPtrW(hWnd, 0);
         HeapFree( GetProcessHeap(), 0, ati );
         SetWindowLongPtrW( hWnd, 0, 0 );
         DestroyAppWindows();
         NtUserSetWindowFNID(hWnd, FNID_DESTROY);
         return 0;
   }
   return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT WINAPI SwitchWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return SwitchWndProc_common(hWnd, uMsg, wParam, lParam, FALSE);
}

LRESULT WINAPI SwitchWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   return SwitchWndProc_common(hWnd, uMsg, wParam, lParam, TRUE);
}
