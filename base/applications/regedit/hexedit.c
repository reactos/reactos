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

#include <regedit.h>
typedef struct
{
  HWND hWndSelf;
  HWND hWndParent;
  HLOCAL hBuffer;
  DWORD style;
  DWORD MaxBuffer;
  INT ColumnsPerLine;
  INT nLines;
  INT nVisibleLinesComplete;
  INT nVisibleLines;
  INT Position;
  INT LineHeight;
  INT CharWidth;
  HFONT hFont;
  BOOL SbVisible;

  INT LeftMargin;
  INT AddressSpacing;
  INT SplitSpacing;

  BOOL EditingField;
  INT CaretCol;
  INT CaretLine;
} HEXEDIT_DATA, *PHEXEDIT_DATA;

/* hit test codes */
#define HEHT_LEFTMARGIN	(0x1)
#define HEHT_ADDRESS	(0x2)
#define HEHT_ADDRESSSPACING	(0x3)
#define HEHT_HEXDUMP	(0x4)
#define HEHT_HEXDUMPSPACING	(0x5)
#define HEHT_ASCIIDUMP	(0x6)
#define HEHT_RIGHTMARGIN	(0x7)

INT_PTR CALLBACK HexEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

ATOM
WINAPI
RegisterHexEditorClass(HINSTANCE hInstance)
{
  WNDCLASSEX WndClass;

  ZeroMemory(&WndClass, sizeof(WNDCLASSEX));
  WndClass.cbSize = sizeof(WNDCLASSEX);
  WndClass.style = CS_DBLCLKS;
  WndClass.lpfnWndProc = (WNDPROC)HexEditWndProc;
  WndClass.cbWndExtra = sizeof(PHEXEDIT_DATA);
  WndClass.hInstance = hInstance;
  WndClass.hCursor = LoadCursor(0, IDC_IBEAM);
  WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  WndClass.lpszClassName = HEX_EDIT_CLASS_NAME;

  return RegisterClassEx(&WndClass);
}

BOOL
WINAPI
UnregisterHexEditorClass(HINSTANCE hInstance)
{
  return UnregisterClass(HEX_EDIT_CLASS_NAME, hInstance);
}

/*** Helper functions *********************************************************/

static VOID
HEXEDIT_MoveCaret(PHEXEDIT_DATA hed, BOOL Scroll)
{
  SCROLLINFO si;

  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS;
  GetScrollInfo(hed->hWndSelf, SB_VERT, &si);

  if(Scroll)
  {
    if(si.nPos > hed->CaretLine)
    {
      si.nPos = hed->CaretLine;
      SetScrollInfo(hed->hWndSelf, SB_VERT, &si, TRUE);
      GetScrollInfo(hed->hWndSelf, SB_VERT, &si);
      InvalidateRect(hed->hWndSelf, NULL, TRUE);
    }
    else if(hed->CaretLine >= (hed->nVisibleLinesComplete + si.nPos))
    {
      si.nPos = hed->CaretLine - hed->nVisibleLinesComplete + 1;
      SetScrollInfo(hed->hWndSelf, SB_VERT, &si, TRUE);
      GetScrollInfo(hed->hWndSelf, SB_VERT, &si);
      InvalidateRect(hed->hWndSelf, NULL, TRUE);
    }
  }

  if(hed->EditingField)
    SetCaretPos(hed->LeftMargin + ((4 + hed->AddressSpacing + (3 * hed->CaretCol)) * hed->CharWidth) - 1, (hed->CaretLine - si.nPos) * hed->LineHeight);
  else
    SetCaretPos(hed->LeftMargin + ((4 + hed->AddressSpacing + hed->SplitSpacing + (3 * hed->ColumnsPerLine) + hed->CaretCol) * hed->CharWidth) - 2, (hed->CaretLine - si.nPos) * hed->LineHeight);
}

static VOID
HEXEDIT_Update(PHEXEDIT_DATA hed)
{
  SCROLLINFO si;
  RECT rcClient;
  BOOL SbVisible;
  INT bufsize, cvislines;

  GetClientRect(hed->hWndSelf, &rcClient);
  hed->style = GetWindowLong(hed->hWndSelf, GWL_STYLE);

  bufsize = (hed->hBuffer ? (INT) LocalSize(hed->hBuffer) : 0);
  hed->nLines = max(bufsize / hed->ColumnsPerLine, 1);
  if(bufsize > hed->ColumnsPerLine && (bufsize % hed->ColumnsPerLine) > 0)
  {
    hed->nLines++;
  }

  if(hed->LineHeight > 0)
  {
    hed->nVisibleLinesComplete = cvislines = rcClient.bottom / hed->LineHeight;
    hed->nVisibleLines = hed->nVisibleLinesComplete;
    if(rcClient.bottom % hed->LineHeight)
    {
      hed->nVisibleLines++;
    }
  }
  else
  {
    hed->nVisibleLines = cvislines = 0;
  }

  SbVisible = bufsize > 0 && cvislines < hed->nLines;
  ShowScrollBar(hed->hWndSelf, SB_VERT, SbVisible);

  /* update scrollbar */
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE | SIF_PAGE;
  si.nMin = 0;
  si.nMax = ((bufsize > 0) ? hed->nLines - 1 : 0);
  si.nPage = ((hed->LineHeight > 0) ? rcClient.bottom / hed->LineHeight : 0);
  SetScrollInfo(hed->hWndSelf, SB_VERT, &si, TRUE);

  if(IsWindowVisible(hed->hWndSelf) && SbVisible != hed->SbVisible)
  {
    InvalidateRect(hed->hWndSelf, NULL, TRUE);
  }

  hed->SbVisible = SbVisible;
}

static HFONT
HEXEDIT_GetFixedFont(VOID)
{
  LOGFONT lf;
  GetObject(GetStockObject(ANSI_FIXED_FONT), sizeof(LOGFONT), &lf);
  return CreateFontIndirect(&lf);
}

static VOID
HEXEDIT_PaintLines(PHEXEDIT_DATA hed, HDC hDC, DWORD ScrollPos, DWORD First, DWORD Last, RECT *rc)
{
  DWORD dx, dy, linestart;
  INT x;
  PBYTE buf, current, end, line;
  size_t bufsize;
  TCHAR hex[3], addr[17];
  RECT rct;

  FillRect(hDC, rc, (HBRUSH)(COLOR_WINDOW + 1));

  if(hed->hBuffer)
  {
    bufsize = LocalSize(hed->hBuffer);
    buf = LocalLock(hed->hBuffer);
  }
  else
  {
    buf = NULL;
    bufsize = 0;

    if(ScrollPos + First == 0)
    {
      /* draw address */
      _stprintf(addr, _T("%04X"), 0);
      TextOut(hDC, hed->LeftMargin, First * hed->LineHeight, addr, 4);
    }
  }

  if(buf)
  {
    end = buf + bufsize;
    dy = First * hed->LineHeight;
    linestart = (ScrollPos + First) * hed->ColumnsPerLine;
    current = buf + linestart;
    Last = min(hed->nLines - ScrollPos, Last);

    while(First <= Last && current < end)
    {
      DWORD dh;

      dx = hed->LeftMargin;

      /* draw address */
      _stprintf(addr, _T("%04lX"), linestart);
      TextOut(hDC, dx, dy, addr, 4);

      dx += ((4 + hed->AddressSpacing) * hed->CharWidth);
      dh = (3 * hed->CharWidth);

      rct.left = dx;
      rct.top = dy;
      rct.right = rct.left + dh;
      rct.bottom = dy + hed->LineHeight;

      /* draw hex map */
      dx += (hed->CharWidth / 2);
      line = current;
      for(x = 0; x < hed->ColumnsPerLine && current < end; x++)
      {
        rct.left += dh;
        rct.right += dh;

	_stprintf(hex, _T("%02X"), *(current++));
	ExtTextOut(hDC, dx, dy, ETO_OPAQUE, &rct, hex, 2, NULL);
	dx += dh;
      }

      /* draw ascii map */
      dx = ((4 + hed->AddressSpacing + hed->SplitSpacing + (hed->ColumnsPerLine * 3)) * hed->CharWidth);
      current = line;
      for(x = 0; x < hed->ColumnsPerLine && current < end; x++)
      {
	_stprintf(hex, _T("%C"), *(current++));
	hex[0] = ((hex[0] & _T('\x007f')) >= _T(' ') ? hex[0] : _T('.'));
	TextOut(hDC, dx, dy, hex, 1);
	dx += hed->CharWidth;
      }

      dy += hed->LineHeight;
      linestart += hed->ColumnsPerLine;
      First++;
    }
  }

  LocalUnlock(hed->hBuffer);
}

static DWORD
HEXEDIT_HitRegionTest(PHEXEDIT_DATA hed, POINTS pt)
{
  int d;

  if(pt.x <= hed->LeftMargin)
  {
    return HEHT_LEFTMARGIN;
  }

  pt.x -= hed->LeftMargin;
  d = (4 * hed->CharWidth);
  if(pt.x <= d)
  {
    return HEHT_ADDRESS;
  }

  pt.x -= d;
  d = (hed->AddressSpacing * hed->CharWidth);
  if(pt.x <= d)
  {
    return HEHT_ADDRESSSPACING;
  }

  pt.x -= d;
  d = (3 * hed->ColumnsPerLine * hed->CharWidth);
  if(pt.x <= d)
  {
    return HEHT_HEXDUMP;
  }

  pt.x -= d;
  d = (hed->SplitSpacing * hed->CharWidth);
  if(pt.x <= d)
  {
    return HEHT_HEXDUMPSPACING;
  }

  pt.x -= d;
  d = (hed->ColumnsPerLine * hed->CharWidth);
  if(pt.x <= d)
  {
    return HEHT_ASCIIDUMP;
  }

  return HEHT_RIGHTMARGIN;
}

static DWORD
HEXEDIT_PositionFromPoint(PHEXEDIT_DATA hed, POINTS pt, DWORD Hit, POINT *EditPos, BOOL *EditField)
{
  SCROLLINFO si;
  DWORD Pos, bufsize;

  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS;
  GetScrollInfo(hed->hWndSelf, SB_VERT, &si);

  EditPos->x = 0;

  if(hed->LineHeight > 0)
  {
    EditPos->y = min(si.nPos + (pt.y / hed->LineHeight), hed->nLines - 1);
  }
  else
  {
    EditPos->y = si.nPos;
  }

  switch(Hit)
  {
    case HEHT_LEFTMARGIN:
    case HEHT_ADDRESS:
    case HEHT_ADDRESSSPACING:
    case HEHT_HEXDUMP:
      pt.x -= (SHORT) hed->LeftMargin + ((4 + hed->AddressSpacing) * hed->CharWidth);
      *EditField = TRUE;
      break;

    default:
      pt.x -= hed->LeftMargin + ((4 + hed->AddressSpacing + hed->SplitSpacing + (3 * hed->ColumnsPerLine)) * hed->CharWidth);
      *EditField = FALSE;
      break;
  }

  if(pt.x > 0)
  {
    INT BlockWidth = (*EditField ? hed->CharWidth * 3 : hed->CharWidth);
    EditPos->x = min(hed->ColumnsPerLine, pt.x / BlockWidth);
  }

  bufsize = (hed->hBuffer ? (DWORD) LocalSize(hed->hBuffer) : 0);
  Pos = (EditPos->y * hed->ColumnsPerLine) + EditPos->x;
  if(Pos > bufsize)
  {
    INT tmp = bufsize % hed->ColumnsPerLine;
    Pos = bufsize;
    EditPos->x = (tmp == 0 ? hed->ColumnsPerLine : tmp);
  }
  return Pos;
}

/*** Control specific messages ************************************************/

static LRESULT
HEXEDIT_HEM_LOADBUFFER(PHEXEDIT_DATA hed, PVOID Buffer, DWORD Size)
{
  if(Buffer != NULL && Size > 0)
  {
    LPVOID buf;

    if(hed->MaxBuffer > 0 && Size > hed->MaxBuffer)
    {
      Size = hed->MaxBuffer;
    }

    if(hed->hBuffer)
    {
      if(Size > 0)
      {
        if(LocalSize(hed->hBuffer) != Size)
        {
          hed->hBuffer = LocalReAlloc(hed->hBuffer, Size, LMEM_MOVEABLE | LMEM_ZEROINIT);
        }
      }
      else
      {
        hed->hBuffer = LocalFree(hed->hBuffer);
        hed->Position = 0;
        HEXEDIT_Update(hed);

        return 0;
      }
    }
    else if(Size > 0)
    {
      hed->hBuffer = LocalAlloc(LHND, Size);
    }

    if(Size > 0)
    {
      buf = LocalLock(hed->hBuffer);
      if(buf)
      {
        memcpy(buf, Buffer, Size);
      }
      else
        Size = 0;
      LocalUnlock(hed->hBuffer);
    }

    hed->Position = 0;
    HEXEDIT_Update(hed);
    return Size;
  }
  else if(hed->hBuffer)
  {
    hed->Position = 0;
    hed->hBuffer = LocalFree(hed->hBuffer);
    HEXEDIT_Update(hed);
  }

  return 0;
}

static LRESULT
HEXEDIT_HEM_COPYBUFFER(PHEXEDIT_DATA hed, PVOID Buffer, DWORD Size)
{
  size_t nCpy;

  if(!hed->hBuffer)
  {
    return 0;
  }

  if(Buffer != NULL && Size > 0)
  {
    nCpy = min(Size, LocalSize(hed->hBuffer));
    if(nCpy > 0)
    {
      PVOID buf;

      buf = LocalLock(hed->hBuffer);
      if(buf)
      {
        memcpy(Buffer, buf, nCpy);
      }
      else
        nCpy = 0;
      LocalUnlock(hed->hBuffer);
    }
    return nCpy;
  }

  return (LRESULT)LocalSize(hed->hBuffer);
}

static LRESULT
HEXEDIT_HEM_SETMAXBUFFERSIZE(PHEXEDIT_DATA hed, DWORD nMaxSize)
{
  hed->MaxBuffer = nMaxSize;
  if(hed->MaxBuffer > 0 && hed->hBuffer && LocalSize(hed->hBuffer) > hed->MaxBuffer)
  {
    /* truncate the buffer */
    hed->hBuffer = LocalReAlloc(hed->hBuffer, hed->MaxBuffer, LMEM_MOVEABLE);
    HEXEDIT_Update(hed);
  }
  return 0;
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

  hed->ColumnsPerLine = 8;
  hed->LeftMargin = 2;
  hed->AddressSpacing = 2;
  hed->SplitSpacing = 2;
  hed->EditingField = TRUE; /* in hexdump field */

  SetWindowLongPtr(hWnd, 0, (DWORD_PTR)hed);
  HEXEDIT_Update(hed);

  return TRUE;
}

static LRESULT
HEXEDIT_WM_NCDESTROY(PHEXEDIT_DATA hed)
{
  if(hed->hBuffer)
  {
    //while(LocalUnlock(hed->hBuffer));
    LocalFree(hed->hBuffer);
  }

  if(hed->hFont)
  {
    DeleteObject(hed->hFont);
  }

  SetWindowLongPtr(hed->hWndSelf, 0, (DWORD_PTR)0);
  HeapFree(GetProcessHeap(), 0, hed);

  return 0;
}

static LRESULT
HEXEDIT_WM_CREATE(PHEXEDIT_DATA hed)
{
  UNREFERENCED_PARAMETER(hed);
  return 1;
}

static LRESULT
HEXEDIT_WM_SETFOCUS(PHEXEDIT_DATA hed)
{
  CreateCaret(hed->hWndSelf, 0, 1, hed->LineHeight);
  HEXEDIT_MoveCaret(hed, FALSE);
  ShowCaret(hed->hWndSelf);
  return 0;
}

static LRESULT
HEXEDIT_WM_KILLFOCUS(PHEXEDIT_DATA hed)
{
  UNREFERENCED_PARAMETER(hed);
  DestroyCaret();
  return 0;
}

static LRESULT
HEXEDIT_WM_VSCROLL(PHEXEDIT_DATA hed, WORD ThumbPosition, WORD SbCmd)
{
  int ScrollY;
  SCROLLINFO si;

  UNREFERENCED_PARAMETER(ThumbPosition);

  ZeroMemory(&si, sizeof(SCROLLINFO));
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_ALL;
  GetScrollInfo(hed->hWndSelf, SB_VERT, &si);

  ScrollY = si.nPos;
  switch(SbCmd)
  {
    case SB_TOP:
      si.nPos = si.nMin;
      break;

    case SB_BOTTOM:
      si.nPos = si.nMax;
      break;

    case SB_LINEUP:
      si.nPos--;
      break;

    case SB_LINEDOWN:
      si.nPos++;
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
  }

  si.fMask = SIF_POS;
  SetScrollInfo(hed->hWndSelf, SB_VERT, &si, TRUE);
  GetScrollInfo(hed->hWndSelf, SB_VERT, &si);

  if(si.nPos != ScrollY)
  {
    ScrollWindow(hed->hWndSelf, 0, (ScrollY - si.nPos) * hed->LineHeight, NULL, NULL);
    UpdateWindow(hed->hWndSelf);
  }

  return 0;
}

static LRESULT
HEXEDIT_WM_SETFONT(PHEXEDIT_DATA hed, HFONT hFont, BOOL bRedraw)
{
  HDC hDC;
  TEXTMETRIC tm;
  HFONT hOldFont = 0;

  if(hFont == 0)
  {
    hFont = HEXEDIT_GetFixedFont();
  }

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

static LRESULT
HEXEDIT_WM_GETFONT(PHEXEDIT_DATA hed)
{
  return (LRESULT)hed->hFont;
}

static LRESULT
HEXEDIT_WM_PAINT(PHEXEDIT_DATA hed)
{
  PAINTSTRUCT ps;
  SCROLLINFO si;
  RECT rc;
  HBITMAP hbmp, hbmpold;
  INT nLines, nFirst;
  HFONT hOldFont;
  HDC hTempDC;
  DWORD height;

  if(GetUpdateRect(hed->hWndSelf, &rc, FALSE) && (hed->LineHeight > 0))
  {
    ZeroMemory(&si, sizeof(SCROLLINFO));
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;
    GetScrollInfo(hed->hWndSelf, SB_VERT, &si);

    height = (rc.bottom - rc.top);
    nLines = height / hed->LineHeight;
    if((height % hed->LineHeight) > 0)
    {
      nLines++;
    }
    if(nLines > hed->nLines - si.nPos)
    {
      nLines = hed->nLines - si.nPos;
    }
    nFirst = rc.top / hed->LineHeight;

    BeginPaint(hed->hWndSelf, &ps);
    if(!(hTempDC = CreateCompatibleDC(ps.hdc)))
    {
      FillRect(ps.hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
      goto epaint;
    }
    if(!(hbmp = CreateCompatibleBitmap(hTempDC, ps.rcPaint.right, ps.rcPaint.bottom)))
    {
      FillRect(ps.hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
      DeleteDC(hTempDC);
      goto epaint;
    }
    hbmpold = SelectObject(hTempDC, hbmp);
    hOldFont = SelectObject(hTempDC, hed->hFont);
    HEXEDIT_PaintLines(hed, hTempDC, si.nPos, nFirst, nFirst + nLines, &ps.rcPaint);
    BitBlt(ps.hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hTempDC, rc.left, rc.top, SRCCOPY);
    SelectObject(hTempDC, hOldFont);
    SelectObject(hTempDC, hbmpold);

    DeleteObject(hbmp);
    DeleteDC(hTempDC);

    epaint:
    EndPaint(hed->hWndSelf, &ps);
  }

  return 0;
}

static LRESULT
HEXEDIT_WM_MOUSEWHEEL(PHEXEDIT_DATA hed, int cyMoveLines, WORD ButtonsDown, LPPOINTS MousePos)
{
  SCROLLINFO si;
  int ScrollY;

  UNREFERENCED_PARAMETER(ButtonsDown);
  UNREFERENCED_PARAMETER(MousePos);

  SetFocus(hed->hWndSelf);

  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_ALL;
  GetScrollInfo(hed->hWndSelf, SB_VERT, &si);

  ScrollY = si.nPos;

  si.fMask = SIF_POS;
  si.nPos += cyMoveLines;
  SetScrollInfo(hed->hWndSelf, SB_VERT, &si, TRUE);

  GetScrollInfo(hed->hWndSelf, SB_VERT, &si);
  if(si.nPos != ScrollY)
  {
    ScrollWindow(hed->hWndSelf, 0, (ScrollY - si.nPos) * hed->LineHeight, NULL, NULL);
    UpdateWindow(hed->hWndSelf);
  }

  return 0;
}

static LRESULT
HEXEDIT_WM_GETDLGCODE(LPMSG Msg)
{
  UNREFERENCED_PARAMETER(Msg);
  return DLGC_WANTARROWS | DLGC_WANTCHARS;
}

static LRESULT
HEXEDIT_WM_LBUTTONDOWN(PHEXEDIT_DATA hed, INT Buttons, POINTS Pt)
{
  BOOL NewField;
  POINT EditPos;
  DWORD Hit = HEXEDIT_HitRegionTest(hed, Pt);

  UNREFERENCED_PARAMETER(Buttons);
  SetFocus(hed->hWndSelf);

  hed->Position = HEXEDIT_PositionFromPoint(hed, Pt, Hit, &EditPos, &NewField);
  hed->EditingField = NewField;
  hed->CaretCol = EditPos.x;
  hed->CaretLine = EditPos.y;

  HEXEDIT_MoveCaret(hed, TRUE);

  return 0;
}

static BOOL
HEXEDIT_WM_KEYDOWN(PHEXEDIT_DATA hed, INT VkCode)
{
  size_t bufsize;
  BOOL shift, control;

  if(GetKeyState(VK_MENU) & 0x8000)
  {
    return FALSE;
  }

  shift = GetKeyState(VK_SHIFT) & 0x8000;
  control = GetKeyState(VK_CONTROL) & 0x8000;

  bufsize = (hed->hBuffer ? LocalSize(hed->hBuffer) : 0);

  switch(VkCode)
  {
    case VK_LEFT:
      if(hed->Position > 0)
      {
        if(--hed->CaretCol < 0)
	{
	  hed->CaretLine--;
	  hed->CaretCol = hed->ColumnsPerLine;
	}
	else
	  hed->Position--;
      }
      HEXEDIT_MoveCaret(hed, TRUE);
      break;

    case VK_RIGHT:
      if(hed->Position < (INT)bufsize)
      {
        if(++hed->CaretCol > hed->ColumnsPerLine)
	{
	  hed->CaretCol = 0;
	  hed->CaretLine++;
	}
	else
	  hed->Position++;
      }
      HEXEDIT_MoveCaret(hed, TRUE);
      break;

    case VK_UP:
      if(hed->Position > 0)
      {
        if(hed->CaretLine <= 0)
	{
	  hed->CaretCol = 0;
	  hed->Position = 0;
	}
	else
	{
	  hed->CaretLine--;
	  hed->Position -= hed->ColumnsPerLine;
	}
      }
      HEXEDIT_MoveCaret(hed, TRUE);
      break;

    case VK_DOWN:
      if(hed->Position <= (INT)bufsize)
      {
        if(hed->CaretLine < hed->nLines - 1)
	{
	  hed->Position += hed->ColumnsPerLine;
	  hed->CaretLine++;
	  if(hed->Position > (INT)bufsize)
	  {
	    hed->Position = (INT) bufsize;
	    hed->CaretLine = (hed->nLines > 0 ? hed->nLines - 1 : 0);
	    hed->CaretCol = (INT) bufsize % hed->ColumnsPerLine;
	  }
	}
	else
	{
	  INT tmp = (INT) bufsize % hed->ColumnsPerLine;
	  hed->Position = (INT) bufsize;
	  hed->CaretCol = (tmp == 0 ? hed->ColumnsPerLine : tmp);
	}
      }
      HEXEDIT_MoveCaret(hed, TRUE);
      break;
  }

  return FALSE;
}

static LRESULT
HEXEDIT_WM_SIZE(PHEXEDIT_DATA hed, DWORD sType, WORD NewWidth, WORD NewHeight)
{
  UNREFERENCED_PARAMETER(sType);
  UNREFERENCED_PARAMETER(NewHeight);
  UNREFERENCED_PARAMETER(NewWidth);
  HEXEDIT_Update(hed);
  return 0;
}

INT_PTR CALLBACK
HexEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PHEXEDIT_DATA hed;
  POINTS p;

  hed = (PHEXEDIT_DATA)(LONG_PTR)GetWindowLongPtr(hWnd, (DWORD_PTR)0);
  switch(uMsg)
  {
    case WM_ERASEBKGND:
      return TRUE;

    case WM_PAINT:
      return HEXEDIT_WM_PAINT(hed);

    case WM_KEYDOWN:
      return HEXEDIT_WM_KEYDOWN(hed, (INT)wParam);

    case WM_VSCROLL:
      return HEXEDIT_WM_VSCROLL(hed, HIWORD(wParam), LOWORD(wParam));

    case WM_SIZE:
      return HEXEDIT_WM_SIZE(hed, (DWORD)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_LBUTTONDOWN:
    {
      p.x = LOWORD(lParam);
      p.y = HIWORD(lParam);
      return HEXEDIT_WM_LBUTTONDOWN(hed, (INT)wParam, p);
    }

    case WM_MOUSEWHEEL:
    {
      UINT nScrollLines = 3;
      int delta = 0;

      SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &nScrollLines, 0);
      delta -= (SHORT)HIWORD(wParam);
      if(abs(delta) >= WHEEL_DELTA && nScrollLines != 0)
      {
        p.x = LOWORD(lParam);
        p.y = HIWORD(lParam);
        return HEXEDIT_WM_MOUSEWHEEL(hed, nScrollLines * (delta / WHEEL_DELTA), LOWORD(wParam), &p);
      }
      break;
    }

    case HEM_LOADBUFFER:
      return HEXEDIT_HEM_LOADBUFFER(hed, (PVOID)wParam, (DWORD)lParam);

    case HEM_COPYBUFFER:
      return HEXEDIT_HEM_COPYBUFFER(hed, (PVOID)wParam, (DWORD)lParam);

    case HEM_SETMAXBUFFERSIZE:
      return HEXEDIT_HEM_SETMAXBUFFERSIZE(hed, (DWORD)lParam);

    case WM_SETFOCUS:
      return HEXEDIT_WM_SETFOCUS(hed);

    case WM_KILLFOCUS:
      return HEXEDIT_WM_KILLFOCUS(hed);

    case WM_GETDLGCODE:
      return HEXEDIT_WM_GETDLGCODE((LPMSG)lParam);

    case WM_SETFONT:
      return HEXEDIT_WM_SETFONT(hed, (HFONT)wParam, (BOOL)LOWORD(lParam));

    case WM_GETFONT:
      return HEXEDIT_WM_GETFONT(hed);

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

