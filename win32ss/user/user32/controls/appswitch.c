/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            win32ss/user/user32/controls/appswitch.c
 * PURPOSE:         app switching functionality
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  David Quintana (gigaherz@gmail.com)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

//
// TODO:
//       Move to Win32k.
//       Add registry support.
//
//

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

#define DIALOG_MARGIN   8       // margin of dialog contents

#define CX_ICON         32      // width of icon
#define CY_ICON         32      // height of icon
#define ICON_MARGIN     4       // margin width around an icon

#define CX_ITEM         (CX_ICON + 2 * ICON_MARGIN)
#define CY_ITEM         (CY_ICON + 2 * ICON_MARGIN)
#define ITEM_MARGIN     4       // margin width around an item

#define CX_ITEM_SPACE   (CX_ITEM + 2 * ITEM_MARGIN)
#define CY_ITEM_SPACE   (CY_ITEM + 2 * ITEM_MARGIN)

#define CY_TEXT_MARGIN  4       // margin height around text

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
POINT ptStart;

int nShift = 0;

BOOL Esc = FALSE;

BOOL CoolSwitch = TRUE;
int CoolSwitchRows = 3;
int CoolSwitchColumns = 7;

// window style
const DWORD Style = WS_POPUP | WS_BORDER | WS_DISABLED;
const DWORD ExStyle = WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW;

BOOL LoadCoolSwitchSettings(void)
{
   CoolSwitch = TRUE;
   CoolSwitchRows = 3;
   CoolSwitchColumns = 7;

   // FIXME: load the settings from registry

   TRACE("CoolSwitch: %d\n", CoolSwitch);
   TRACE("CoolSwitchRows: %d\n", CoolSwitchRows);
   TRACE("CoolSwitchColumns: %d\n", CoolSwitchColumns);

   return TRUE;
}

void ResizeAndCenter(HWND hwnd, int width, int height)
{
   int x, y;
   RECT Rect;

   int screenwidth = GetSystemMetrics(SM_CXSCREEN);
   int screenheight = GetSystemMetrics(SM_CYSCREEN);

   x = (screenwidth - width) / 2;
   y = (screenheight - height) / 2;

   SetRect(&Rect, x, y, x + width, y + height);
   AdjustWindowRectEx(&Rect, Style, FALSE, ExStyle);

   x = Rect.left;
   y = Rect.top;
   width = Rect.right - Rect.left;
   height = Rect.bottom - Rect.top;
   MoveWindow(hwnd, x, y, width, height, FALSE);

   ptStart.x = x;
   ptStart.y = y;
}

void CompleteSwitch(BOOL doSwitch)
{
   if (!isOpen)
      return;

   isOpen = FALSE;

   TRACE("[ATbot] CompleteSwitch Hiding Window.\n");
   ShowWindowAsync(switchdialog, SW_HIDE);

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

         SwitchToThisWindow(hwnd, TRUE);
      }
   }

   windowCount = 0;
}

BOOL CALLBACK EnumerateCallback(HWND window, LPARAM lParam)
{
    HICON hIcon = NULL;
    LRESULT bAlive;

    UNREFERENCED_PARAMETER(lParam);

    // First try to get the big icon assigned to the window
#define ICON_TIMEOUT 100 // in milliseconds
    bAlive = SendMessageTimeoutW(window, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK,
                                 ICON_TIMEOUT, (PDWORD_PTR)&hIcon);
    if (!hIcon)
    {
        // If no icon is assigned, try to get the icon assigned to the windows' class
        hIcon = (HICON)GetClassLongPtrW(window, GCL_HICON);
        if (!hIcon)
        {
            // If we still don't have an icon, see if we can do with the small icon,
            // or a default application icon
            if (bAlive)
            {
                SendMessageTimeoutW(window, WM_GETICON, ICON_SMALL2, 0,
                                    SMTO_ABORTIFHUNG | SMTO_BLOCK, ICON_TIMEOUT,
                                    (PDWORD_PTR)&hIcon);
            }
#undef ICON_TIMEOUT
            if (!hIcon)
            {
                // using windows logo icon as default
                hIcon = gpsi->hIconWindows;
                if (!hIcon)
                {
                    //if all attempts to get icon fails go to the next window
                    return TRUE;
                }
            }
        }
    }

    windowList[windowCount] = window;
    iconList[windowCount] = CopyIcon(hIcon);
    windowCount++;

    // If we got to the max number of windows, we won't be able to add any more
    return (windowCount < MAX_WINDOWS);
}

static HWND GetNiceRootOwner(HWND hwnd)
{
    HWND hwndOwner;
    DWORD ExStyle, OwnerExStyle;

    for (;;)
    {
        // A window with WS_EX_APPWINDOW is treated as if it has no owner
        ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (ExStyle & WS_EX_APPWINDOW)
            break;

        // Is the owner visible?
        // An window with WS_EX_TOOLWINDOW is treated as if it weren't visible
        hwndOwner = GetWindow(hwnd, GW_OWNER);
        OwnerExStyle = GetWindowLong(hwndOwner, GWL_EXSTYLE);
        if (!IsWindowVisible(hwndOwner) || (OwnerExStyle & WS_EX_TOOLWINDOW))
            break;

        hwnd = hwndOwner;
    }

    return hwnd;
}

// c.f. https://devblogs.microsoft.com/oldnewthing/20071008-00/?p=24863
BOOL IsAltTabWindow(HWND hwnd)
{
    DWORD ExStyle;
    RECT rc;
    HWND hwndTry, hwndWalk;
    WCHAR szClass[64];

    // must be visible
    if (!IsWindowVisible(hwnd))
        return FALSE;

    // must not be WS_EX_TOOLWINDOW nor WS_EX_NOACTIVATE
    ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (ExStyle & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE))
        return FALSE;

    // must be not empty rect
    GetWindowRect(hwnd, &rc);
    if (IsRectEmpty(&rc))
        return FALSE;

    // check special windows
    if (!GetClassNameW(hwnd, szClass, _countof(szClass)) ||
        wcscmp(szClass, L"Shell_TrayWnd") == 0 ||
        wcscmp(szClass, L"Progman") == 0)
    {
        return TRUE;
    }

    // get 'nice' root owner
    hwndWalk = GetNiceRootOwner(hwnd);

    // walk back from hwndWalk toward hwnd
    for (;;)
    {
        hwndTry = GetLastActivePopup(hwndWalk);
        if (hwndTry == hwndWalk)
            break;

        ExStyle = GetWindowLong(hwndTry, GWL_EXSTYLE);
        if (IsWindowVisible(hwndTry) && !(ExStyle & WS_EX_TOOLWINDOW))
            break;

        hwndWalk = hwndTry;
    }

    return hwnd == hwndTry;     // Reached?
}

static BOOL CALLBACK
EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (IsAltTabWindow(hwnd))
    {
        if (!EnumerateCallback(hwnd, lParam))
            return FALSE;
    }
    return TRUE;
}

void ProcessMouseMessage(UINT message, LPARAM lParam)
{
   int xPos = LOWORD(lParam);
   int yPos = HIWORD(lParam);

   int xIndex = (xPos - DIALOG_MARGIN) / CX_ITEM_SPACE;
   int yIndex = (yPos - DIALOG_MARGIN) / CY_ITEM_SPACE;

   if (xIndex < 0 || nCols <= xIndex ||
       yIndex < 0 || nRows <= yIndex)
   {
        return;
   }

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
    int i, xPos, yPos, CharCount;
    HFONT dcFont;
    HICON hIcon;
    HPEN hPen;
    COLORREF Color;

    // check
    if (nCols == 0 || nItems == 0)
        return;

    // begin painting
    dialogDC = BeginPaint(hWnd, &paint);
    if (dialogDC == NULL)
        return;

    // fill the client area
    GetClientRect(hWnd, &cRC);
    FillRect(dialogDC, &cRC, (HBRUSH)(COLOR_3DFACE + 1));

    // if the selection index exceeded the display items, then
    // do display item shifting
    if (selectedWindow >= nItems)
        nShift = selectedWindow - nItems + 1;
    else
        nShift = 0;

    for (i = 0; i < nItems; ++i)
    {
        // get the icon to display
        hIcon = iconList[i + nShift];

        // calculate the position where we start drawing
        xPos = DIALOG_MARGIN + CX_ITEM_SPACE * (i % nCols) + ITEM_MARGIN;
        yPos = DIALOG_MARGIN + CY_ITEM_SPACE * (i / nCols) + ITEM_MARGIN;

        // centering
        if (nItems < CoolSwitchColumns)
        {
            xPos += (itemsW - nItems * CX_ITEM_SPACE) / 2;
        }

        // if this position is selected,
        if (selectedWindow == i + nShift)
        {
            // create a solid pen
            Color = GetSysColor(COLOR_HIGHLIGHT);
            hPen = CreatePen(PS_SOLID, 1, Color);

            // draw a rectangle with using the pen
            SelectObject(dialogDC, hPen);
            SelectObject(dialogDC, GetStockObject(NULL_BRUSH));
            Rectangle(dialogDC, xPos, yPos, xPos + CX_ITEM, yPos + CY_ITEM);
            Rectangle(dialogDC, xPos + 1, yPos + 1,
                                xPos + CX_ITEM - 1, yPos + CY_ITEM - 1);

            // delete the pen
            DeleteObject(hPen);
        }

        // draw icon
        DrawIconEx(dialogDC, xPos + ICON_MARGIN, yPos + ICON_MARGIN,
                   hIcon, CX_ICON, CY_ICON, 0, NULL, DI_NORMAL);
    }

    // set the text rectangle
    SetRect(&textRC, DIALOG_MARGIN, DIALOG_MARGIN + itemsH,
            totalW - DIALOG_MARGIN, totalH - DIALOG_MARGIN);

    // draw the sunken button around text
    DrawFrameControl(dialogDC, &textRC, DFC_BUTTON,
                     DFCS_BUTTONPUSH | DFCS_PUSHED);

    // get text
    CharCount = GetWindowTextW(windowList[selectedWindow], windowText,
                               _countof(windowText));

    // draw text
    dcFont = SelectObject(dialogDC, dialogFont);
    SetTextColor(dialogDC, GetSysColor(COLOR_BTNTEXT));
    SetBkMode(dialogDC, TRANSPARENT);
    DrawTextW(dialogDC, windowText, CharCount, &textRC,
              DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
    SelectObject(dialogDC, dcFont);

    // end painting
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

DWORD GetDialogFont(VOID)
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

void PrepareWindow(VOID)
{
   nItems = windowCount;

   nCols = CoolSwitchColumns;
   nRows = (nItems + CoolSwitchColumns - 1) / CoolSwitchColumns;
   if (nRows > CoolSwitchRows)
   {
      nRows = CoolSwitchRows;
      nItems = nRows * nCols;
   }

   itemsW = nCols * CX_ITEM_SPACE;
   itemsH = nRows * CY_ITEM_SPACE;

   totalW = itemsW + 2 * DIALOG_MARGIN;
   totalH = itemsH + 2 * DIALOG_MARGIN;
   totalH += fontHeight + 2 * CY_TEXT_MARGIN;

   ResizeAndCenter(switchdialog, totalW, totalH);
}

BOOL ProcessHotKey(VOID)
{
   if (!isOpen)
   {
      windowCount = 0;
      EnumWindows(EnumWindowsProc, 0);

      if (windowCount == 0)
         return FALSE;

      if (windowCount == 1)
      {
         SwitchToThisWindow(windowList[0], TRUE);
         return FALSE;
      }

      if (!CreateSwitcherWindow(User32Instance))
         return FALSE;

      selectedWindow = 1;

      TRACE("[ATbot] HotKey Received. Opening window.\n");
      ShowWindowAsync(switchdialog, SW_SHOWNORMAL);
      SwitchToThisWindow(switchdialog, TRUE);
      isOpen = TRUE;
   }
   else
   {
      TRACE("[ATbot] HotKey Received  Rotating.\n");
      selectedWindow = (selectedWindow + 1)%windowCount;
      InvalidateRect(switchdialog, NULL, TRUE);
   }
   return TRUE;
}

void RotateTasks(BOOL bShift)
{
    HWND hwndFirst, hwndLast;
    DWORD Size;

    if (windowCount < 2 || !Esc)
        return;

    hwndFirst = windowList[0];
    hwndLast = windowList[windowCount - 1];

    if (bShift)
    {
        SetWindowPos(hwndLast, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                     SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_ASYNCWINDOWPOS);

        SwitchToThisWindow(hwndLast, TRUE);

        Size = (windowCount - 1) * sizeof(HWND);
        MoveMemory(&windowList[1], &windowList[0], Size);
        windowList[0] = hwndLast;
    }
    else
    {
        SetWindowPos(hwndFirst, hwndLast, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                     SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_ASYNCWINDOWPOS);

        SwitchToThisWindow(windowList[1], TRUE);

        Size = (windowCount - 1) * sizeof(HWND);
        MoveMemory(&windowList[0], &windowList[1], Size);
        windowList[windowCount - 1] = hwndFirst;
    }
}

static void MoveLeft(void)
{
    selectedWindow = selectedWindow - 1;
    if (selectedWindow < 0)
        selectedWindow = windowCount - 1;
    InvalidateRect(switchdialog, NULL, TRUE);
}

static void MoveRight(void)
{
    selectedWindow = (selectedWindow + 1) % windowCount;
    InvalidateRect(switchdialog, NULL, TRUE);
}

static void MoveUp(void)
{
    INT iRow = selectedWindow / nCols;
    INT iCol = selectedWindow % nCols;

    --iRow;
    if (iRow < 0)
        iRow = nRows - 1;

    selectedWindow = iRow * nCols + iCol;
    if (selectedWindow >= windowCount)
        selectedWindow = windowCount - 1;
    InvalidateRect(switchdialog, NULL, TRUE);
}

static void MoveDown(void)
{
    INT iRow = selectedWindow / nCols;
    INT iCol = selectedWindow % nCols;

    ++iRow;
    if (iRow >= nRows)
        iRow = 0;

    selectedWindow = iRow * nCols + iCol;
    if (selectedWindow >= windowCount)
        selectedWindow = windowCount - 1;
    InvalidateRect(switchdialog, NULL, TRUE);
}

VOID
DestroyAppWindows(VOID)
{
    // for every item of the icon list:
    INT i;
    for (i = 0; i < windowCount; ++i)
    {
        // destroy the icon
        DestroyIcon(iconList[i]);
        iconList[i] = NULL;
    }
}

LRESULT WINAPI DoAppSwitch( WPARAM wParam, LPARAM lParam )
{
   HWND hwndActive;
   MSG msg;

   // FIXME: Is loading timing OK?
   LoadCoolSwitchSettings();

   if (!CoolSwitch)
      return 0;

   // Already in the loop.
   if (switchdialog || Esc) return 0;

   if (lParam == VK_ESCAPE)
   {
      Esc = TRUE;

      windowCount = 0;
      EnumWindows(EnumWindowsProc, 0);

      if (windowCount < 2)
          return 0;

      RotateTasks(GetAsyncKeyState(VK_SHIFT) < 0);

      hwndActive = GetActiveWindow();

      if (hwndActive == NULL)
      {
          Esc = FALSE;
          return 0;
      }
   }

   // Capture current active window.
   hwndActive = GetActiveWindow();
   if (hwndActive)
       SetCapture(hwndActive);

   switch (lParam)
   {
      case VK_TAB:
         if (!GetDialogFont() || !ProcessHotKey())
             goto Exit;
         break;

      case VK_ESCAPE:
         break;

      default:
         goto Exit;
   }

   if (!hwndActive)
       goto Exit;

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
             if ( msg.wParam == VK_TAB )
             {
                if (Esc) break;
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    MoveLeft();
                }
                else
                {
                    MoveRight();
                }
             }
             else if ( msg.wParam == VK_ESCAPE )
             {
                if (!Esc) break;
                RotateTasks(GetKeyState(VK_SHIFT) < 0);
             }
             else if ( msg.wParam == VK_LEFT )
             {
                MoveLeft();
             }
             else if ( msg.wParam == VK_RIGHT )
             {
                MoveRight();
             }
             else if ( msg.wParam == VK_UP )
             {
                MoveUp();
             }
             else if ( msg.wParam == VK_DOWN )
             {
                MoveDown();
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
   if (Esc) DestroyAppWindows();
   switchdialog = NULL;
   selectedWindow = 0;
   windowCount = 0;
   Esc = FALSE;
   return 0;
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
            ati->cbSize = sizeof(ALTTABINFO);
            ati->cItems = nItems;
            ati->cColumns = nCols;
            ati->cRows = nRows;
            if (nCols)
            {
               ati->iColFocus = (selectedWindow - nShift) % nCols;
               ati->iRowFocus = (selectedWindow - nShift) / nCols;
            }
            else
            {
               ati->iColFocus = 0;
               ati->iRowFocus = 0;
            }
            ati->cxItem = CX_ITEM_SPACE;
            ati->cyItem = CY_ITEM_SPACE;
            ati->ptStart = ptStart;
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
