/*
 * Hex editor control
 *
 * Copyright (C) 2004 Thomas Weidenmueller <w3seek@reactos.com>
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

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#include "hexedit.h"

typedef struct
{
  HWND hWndSelf;
  HWND hWndParent;
  HLOCAL hData;
  DWORD style;
  INT LineHeight;
  INT CharWidth;
  HFONT hFont;
  INT LeftMargin;
  INT TopMargin;
  INT CaretX;
  INT CaretY;
} HEXEDIT_DATA, *PHEXEDIT_DATA;

LRESULT WINAPI HexEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

ATOM
STDCALL
RegisterHexEditorClass(HINSTANCE hInstance)
{
  WNDCLASSEX WndClass;
  
  ZeroMemory(&WndClass, sizeof(WNDCLASSEX));
  WndClass.cbSize = sizeof(WNDCLASSEX);
  WndClass.style = CS_DBLCLKS | CS_PARENTDC;
  WndClass.lpfnWndProc = (WNDPROC)HexEditWndProc;
  WndClass.cbWndExtra = sizeof(PHEXEDIT_DATA);
  WndClass.hInstance = hInstance;
  WndClass.hCursor = LoadCursor(0, IDC_IBEAM);
  WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  WndClass.lpszClassName = HEX_EDIT_CLASS_NAME;
  
  return RegisterClassEx(&WndClass);
}

BOOL
STDCALL
UnregisterHexEditorClass(HINSTANCE hInstance)
{
  return UnregisterClass(HEX_EDIT_CLASS_NAME, hInstance);
}

/*** Helper functions *********************************************************/

static VOID
HEXEDIT_MoveCaret(PHEXEDIT_DATA hed, INT Col, INT Line)
{
  /* FIXME - include the scroll position */
  SetCaretPos(hed->LeftMargin + (Col * hed->CharWidth), hed->TopMargin + (Line * hed->LineHeight));
}

/*** Message Proc *************************************************************/

static LRESULT
HEXEDIT_WM_NCCREATE(HWND hWnd, CREATESTRUCT *cs)
{
  PHEXEDIT_DATA hed;
  
  if(!(hed = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HEXEDIT_DATA))))
  {
    return FALSE;
  }
  
  hed->hWndSelf = hWnd;
  hed->hWndParent = cs->hwndParent;
  hed->style = cs->style;
  
  SetWindowLong(hWnd, 0, (LONG)hed);
  
  return TRUE;
}

static LRESULT
HEXEDIT_WM_NCDESTROY(PHEXEDIT_DATA hed)
{
  if(hed->hData)
  {
    while(LocalUnlock(hed->hData));
    LocalFree(hed->hData);
  }
  
  SetWindowLong(hed->hWndSelf, 0, 0);
  HeapFree(GetProcessHeap(), 0, hed);
  
  return 0;
}

static LRESULT
HEXEDIT_WM_CREATE(PHEXEDIT_DATA hed)
{
  hed->LeftMargin = 2;
  hed->TopMargin = 2;
  return 1;
}

static LRESULT
HEXEDIT_WM_SETFOCUS(PHEXEDIT_DATA hed)
{
  CreateCaret(hed->hWndSelf, 0, 2, hed->LineHeight);
  HEXEDIT_MoveCaret(hed, 0, 0);
  ShowCaret(hed->hWndSelf);
  return 0;
}

static LRESULT
HEXEDIT_WM_KILLFOCUS(PHEXEDIT_DATA hed)
{
  DestroyCaret();
  return 0;
}

static LRESULT
HEXEDIT_WM_SETFONT(PHEXEDIT_DATA hed, HFONT hFont, BOOL bRedraw)
{
  HDC hDC;
  TEXTMETRIC tm;
  HFONT hOldFont = 0;
  
  hed->hFont = hFont;
  hDC = GetDC(hed->hWndSelf);
  if(hFont)
  {
    hOldFont = SelectObject(hDC, hFont);
  }
  GetTextMetrics(hDC, &tm);
  hed->LineHeight = tm.tmHeight;
  hed->CharWidth = tm.tmAveCharWidth;
  if(hOldFont)
  {
    SelectObject(hDC, hOldFont);
  }
  ReleaseDC(hed->hWndSelf, hDC);
  
  if(bRedraw)
  {
    InvalidateRect(hed->hWndSelf, NULL, TRUE);
  }
  
  return 0;
}

LRESULT
WINAPI
HexEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PHEXEDIT_DATA hed;
  
  hed = (PHEXEDIT_DATA)GetWindowLong(hWnd, 0);
  switch(uMsg)
  {
    case WM_SETFOCUS:
      return HEXEDIT_WM_SETFOCUS(hed);
    
    case WM_KILLFOCUS:
      return HEXEDIT_WM_KILLFOCUS(hed);
    
    case WM_SETFONT:
      return HEXEDIT_WM_SETFONT(hed, (HFONT)wParam, (BOOL)LOWORD(lParam));
    
    case WM_CREATE:
      return HEXEDIT_WM_CREATE(hed);
    
    case WM_NCCREATE:
      if(!hed)
      {
        return HEXEDIT_WM_NCCREATE(hWnd, (CREATESTRUCT*)lParam);
      }
      break;
    
    case WM_NCDESTROY:
      if(hed)
      {
        return HEXEDIT_WM_NCDESTROY(hed);
      }
      break;
  }
  
  return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
}
