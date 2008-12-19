/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* GLOBALS *******************************************************************/



/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
BOOL
WINAPI
DragDetect(
  HWND hWnd,
  POINT pt)
{
#if 0
  return NtUserDragDetect(hWnd, pt);
#else
  MSG msg;
  RECT rect;
  POINT tmp;
  ULONG dx = GetSystemMetrics(SM_CXDRAG);
  ULONG dy = GetSystemMetrics(SM_CYDRAG);

  rect.left = pt.x - dx;
  rect.right = pt.x + dx;
  rect.top = pt.y - dy;
  rect.bottom = pt.y + dy;

  SetCapture(hWnd);

  for (;;)
  {
    while (
    PeekMessageW(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) ||
    PeekMessageW(&msg, 0, WM_KEYFIRST,   WM_KEYLAST,   PM_REMOVE)
    )
    {
      if (msg.message == WM_LBUTTONUP)
      {
        ReleaseCapture();
        return FALSE;
      }
      if (msg.message == WM_MOUSEMOVE)
      {
        tmp.x = LOWORD(msg.lParam);
        tmp.y = HIWORD(msg.lParam);
        if (!PtInRect(&rect, tmp))
        {
          ReleaseCapture();
          return TRUE;
        }
      }
      if (msg.message == WM_KEYDOWN)
      {
         if (msg.wParam == VK_ESCAPE)
         {
             ReleaseCapture();
             return TRUE;
         }
      }
    }
    WaitMessage();
  }
  return 0;
#endif
}


/*
 * @implemented
 */
BOOL WINAPI
BlockInput(BOOL fBlockIt)
{
  return NtUserBlockInput(fBlockIt);
}


/*
 * @implemented
 */
BOOL WINAPI
EnableWindow(HWND hWnd,
	     BOOL bEnable)
{
    LONG Style = GetWindowLongW(hWnd, GWL_STYLE);
    /* check if updating is needed */
    UINT bIsDisabled = (Style & WS_DISABLED);
    if ( (bIsDisabled && bEnable) || (!bIsDisabled && !bEnable) )
    {
        if (bEnable)
        {
            Style &= ~WS_DISABLED;
        }
        else
        {
            Style |= WS_DISABLED;
            /* Remove keyboard focus from that window if it had focus */
            if (hWnd == GetFocus())
            {
               SetFocus(NULL);
            }
        }
        NtUserSetWindowLong(hWnd, GWL_STYLE, Style, FALSE);

        SendMessageA(hWnd, WM_ENABLE, (LPARAM) IsWindowEnabled(hWnd), 0);
    }
    // Return nonzero if it was disabled, or zero if it wasn't:
    return IsWindowEnabled(hWnd);
}


/*
 * @implemented
 */
SHORT WINAPI
GetAsyncKeyState(int vKey)
{
 if (vKey < 0 || vKey > 256)
    return 0;
 return (SHORT) NtUserGetAsyncKeyState((DWORD) vKey);
}


/*
 * @implemented
 */
UINT
WINAPI
GetDoubleClickTime(VOID)
{
  return NtUserGetDoubleClickTime();
}


/*
 * @implemented
 */
HKL WINAPI
GetKeyboardLayout(DWORD idThread)
{
  return (HKL)NtUserCallOneParam((DWORD) idThread,  ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT);
}


/*
 * @implemented
 */
UINT WINAPI
GetKBCodePage(VOID)
{
  return GetOEMCP();
}


/*
 * @implemented
 */
int WINAPI
GetKeyNameTextA(LONG lParam,
		LPSTR lpString,
		int nSize)
{
  LPWSTR intermediateString =
    HeapAlloc(GetProcessHeap(),0,nSize * sizeof(WCHAR));
  int ret = 0;
  UINT wstrLen = 0;
  BOOL defChar = FALSE;

  if( !intermediateString ) return 0;
  ret = GetKeyNameTextW(lParam,intermediateString,nSize);
  if( ret == 0 ) { lpString[0] = 0; return 0; }

  wstrLen = wcslen( intermediateString );
  ret = WideCharToMultiByte(CP_ACP, 0,
			    intermediateString, wstrLen,
			    lpString, nSize, ".", &defChar );
  lpString[ret] = 0;
  HeapFree(GetProcessHeap(),0,intermediateString);

  return ret;
}

/*
 * @implemented
 */
int WINAPI
GetKeyNameTextW(LONG lParam,
		LPWSTR lpString,
		int nSize)
{
  return NtUserGetKeyNameText( lParam, lpString, nSize );
}


/*
 * @implemented
 */
SHORT WINAPI
GetKeyState(int nVirtKey)
{
 return (SHORT) NtUserGetKeyState((DWORD) nVirtKey);
}


/*
 * @implemented
 */
BOOL WINAPI
GetKeyboardLayoutNameA(LPSTR pwszKLID)
{
  WCHAR buf[KL_NAMELENGTH];

  if (GetKeyboardLayoutNameW(buf))
    return WideCharToMultiByte( CP_ACP, 0, buf, -1, pwszKLID, KL_NAMELENGTH, NULL, NULL ) != 0;
  return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
  return NtUserGetKeyboardLayoutName( pwszKLID );
}


/*
 * @implemented
 */
BOOL WINAPI
GetKeyboardState(PBYTE lpKeyState)
{

  return (BOOL) NtUserGetKeyboardState((LPBYTE) lpKeyState);
}


/*
 * @implemented
 */
int WINAPI
GetKeyboardType(int nTypeFlag)
{
return (int)NtUserCallOneParam((DWORD) nTypeFlag,  ONEPARAM_ROUTINE_GETKEYBOARDTYPE);
}


/*
 * @implemented
 */
BOOL WINAPI
GetLastInputInfo(PLASTINPUTINFO plii)
{
  return NtUserGetLastInputInfo(plii);
}


/*
 * @implemented
 */
HKL WINAPI
LoadKeyboardLayoutA(LPCSTR pwszKLID,
		    UINT Flags)
{
  return NtUserLoadKeyboardLayoutEx( NULL, 0, NULL, NULL, NULL,
               strtoul(pwszKLID, NULL, 16),
               Flags);
}


/*
 * @implemented
 */
HKL WINAPI
LoadKeyboardLayoutW(LPCWSTR pwszKLID,
		    UINT Flags)
{
  // Look at revision 25596 to see how it's done in windows.
  // We will do things our own way. Also be compatible too!
  return NtUserLoadKeyboardLayoutEx( NULL, 0, NULL, NULL, NULL,
               wcstoul(pwszKLID, NULL, 16),
               Flags);
}


/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyA(UINT uCode,
	       UINT uMapType)
{
  return MapVirtualKeyExA( uCode, uMapType, GetKeyboardLayout( 0 ) );
}


/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyExA(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return MapVirtualKeyExW( uCode, uMapType, dwhkl );
}


/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyExW(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return NtUserMapVirtualKeyEx( uCode, uMapType, 0, dwhkl );
}


/*
 * @implemented
 */
UINT WINAPI
MapVirtualKeyW(UINT uCode,
	       UINT uMapType)
{
  return MapVirtualKeyExW( uCode, uMapType, GetKeyboardLayout( 0 ) );
}


/*
 * @implemented
 */
DWORD WINAPI
OemKeyScan(WORD wOemChar)
{
  WCHAR p;
  SHORT Vk;
  UINT Scan;

  MultiByteToWideChar(CP_OEMCP, 0, (PCSTR)&wOemChar, 1, &p, 1);
  Vk = VkKeyScanW(p);
  Scan = MapVirtualKeyW((Vk & 0x00ff), 0);
  if(!Scan) return -1;
  /*
     Page 450-1, MS W2k SuperBible by SAMS. Return, low word has the
     scan code and high word has the shift state.
   */
  return ((Vk & 0xff00) << 8) | Scan;
}


/*
 * @implemented
 */
BOOL WINAPI
RegisterHotKey(HWND hWnd,
	       int id,
	       UINT fsModifiers,
	       UINT vk)
{
  return (BOOL)NtUserRegisterHotKey(hWnd,
                                       id,
                                       fsModifiers,
                                       vk);
}


/*
 * @implemented
 */
BOOL WINAPI
SetDoubleClickTime(UINT uInterval)
{
  return (BOOL)NtUserSystemParametersInfo(SPI_SETDOUBLECLICKTIME,
                                             uInterval,
                                             NULL,
                                             0);
}


/*
 * @implemented
 */
HWND WINAPI
SetFocus(HWND hWnd)
{
  return NtUserSetFocus(hWnd);
}


/*
 * @implemented
 */
BOOL WINAPI
SetKeyboardState(LPBYTE lpKeyState)
{
 return (BOOL) NtUserSetKeyboardState((LPBYTE)lpKeyState);
}


/*
 * @implemented
 */
BOOL
WINAPI
SwapMouseButton(
  BOOL fSwap)
{
  return NtUserSwapMouseButton(fSwap);
}


/*
 * @implemented
 */
int WINAPI
ToAscii(UINT uVirtKey,
	UINT uScanCode,
	CONST PBYTE lpKeyState,
	LPWORD lpChar,
	UINT uFlags)
{
  return ToAsciiEx(uVirtKey, uScanCode, lpKeyState, lpChar, uFlags, 0);
}


/*
 * @implemented
 */
int WINAPI
ToAsciiEx(UINT uVirtKey,
	  UINT uScanCode,
	  CONST PBYTE lpKeyState,
	  LPWORD lpChar,
	  UINT uFlags,
	  HKL dwhkl)
{
  WCHAR UniChars[2];
  int Ret, CharCount;

  Ret = ToUnicodeEx(uVirtKey, uScanCode, lpKeyState, UniChars, 2, uFlags, dwhkl);
  CharCount = (Ret < 0 ? 1 : Ret);
  WideCharToMultiByte(CP_ACP, 0, UniChars, CharCount, (LPSTR) lpChar, 2, NULL, NULL);

  return Ret;
}


/*
 * @implemented
 */
int WINAPI
ToUnicode(UINT wVirtKey,
	  UINT wScanCode,
	  CONST PBYTE lpKeyState,
	  LPWSTR pwszBuff,
	  int cchBuff,
	  UINT wFlags)
{
  return ToUnicodeEx( wVirtKey, wScanCode, lpKeyState, pwszBuff, cchBuff,
		      wFlags, 0 );
}


/*
 * @implemented
 */
int WINAPI
ToUnicodeEx(UINT wVirtKey,
	    UINT wScanCode,
	    CONST PBYTE lpKeyState,
	    LPWSTR pwszBuff,
	    int cchBuff,
	    UINT wFlags,
	    HKL dwhkl)
{
  return NtUserToUnicodeEx( wVirtKey, wScanCode, lpKeyState, pwszBuff, cchBuff,
			    wFlags, dwhkl );
}



/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanA(CHAR ch)
{
  WCHAR wChar;

  if (IsDBCSLeadByte(ch)) return -1;

  MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wChar, 1);
  return VkKeyScanW(wChar);
}


/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanExA(CHAR ch,
	     HKL dwhkl)
{
  WCHAR wChar;

  if (IsDBCSLeadByte(ch)) return -1;

  MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wChar, 1);
  return VkKeyScanExW(wChar, dwhkl);
}


/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanExW(WCHAR ch,
	     HKL dwhkl)
{
  return (SHORT) NtUserVkKeyScanEx(ch, dwhkl, 0);
}


/*
 * @implemented
 */
SHORT WINAPI
VkKeyScanW(WCHAR ch)
{
  return VkKeyScanExW(ch, GetKeyboardLayout(0));
}


/*
 * @implemented
 */
UINT
WINAPI
SendInput(
  UINT nInputs,
  LPINPUT pInputs,
  int cbSize)
{
  return NtUserSendInput(nInputs, pInputs, cbSize);
}


/*
 * @implemented
 */
VOID
WINAPI
keybd_event(
	    BYTE bVk,
	    BYTE bScan,
	    DWORD dwFlags,
	    ULONG_PTR dwExtraInfo)


{
  INPUT Input;

  Input.type = INPUT_KEYBOARD;
  Input.ki.wVk = bVk;
  Input.ki.wScan = bScan;
  Input.ki.dwFlags = dwFlags;
  Input.ki.time = 0;
  Input.ki.dwExtraInfo = dwExtraInfo;

  NtUserSendInput(1, &Input, sizeof(INPUT));
}


/*
 * @implemented
 */
VOID
WINAPI
mouse_event(
	    DWORD dwFlags,
	    DWORD dx,
	    DWORD dy,
	    DWORD dwData,
	    ULONG_PTR dwExtraInfo)
{
  INPUT Input;

  Input.type = INPUT_MOUSE;
  Input.mi.dx = dx;
  Input.mi.dy = dy;
  Input.mi.mouseData = dwData;
  Input.mi.dwFlags = dwFlags;
  Input.mi.time = 0;
  Input.mi.dwExtraInfo = dwExtraInfo;

  NtUserSendInput(1, &Input, sizeof(INPUT));
}


/***********************************************************************
 *           get_key_state
 */
static WORD get_key_state(void)
{
    WORD ret = 0;

    if (GetSystemMetrics( SM_SWAPBUTTON ))
    {
        if (GetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (GetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    else
    {
        if (GetAsyncKeyState(VK_LBUTTON) & 0x80) ret |= MK_LBUTTON;
        if (GetAsyncKeyState(VK_RBUTTON) & 0x80) ret |= MK_RBUTTON;
    }
    if (GetAsyncKeyState(VK_MBUTTON) & 0x80)  ret |= MK_MBUTTON;
    if (GetAsyncKeyState(VK_SHIFT) & 0x80)    ret |= MK_SHIFT;
    if (GetAsyncKeyState(VK_CONTROL) & 0x80)  ret |= MK_CONTROL;
    if (GetAsyncKeyState(VK_XBUTTON1) & 0x80) ret |= MK_XBUTTON1;
    if (GetAsyncKeyState(VK_XBUTTON2) & 0x80) ret |= MK_XBUTTON2;
    return ret;
}

static void CALLBACK TrackMouseEventProc(HWND hwndUnused, UINT uMsg, UINT_PTR idEvent,
    DWORD dwTime)
{
    POINT pos;
    POINT posClient;
    HWND hwnd;
    INT hoverwidth = 0, hoverheight = 0;
    RECT client;
    PUSER32_TRACKINGLIST ptracking_info;

    ptracking_info = & User32GetThreadData()->tracking_info;

    GetCursorPos(&pos);
    hwnd = WindowFromPoint(pos);

    SystemParametersInfoW(SPI_GETMOUSEHOVERWIDTH, 0, &hoverwidth, 0);
    SystemParametersInfoW(SPI_GETMOUSEHOVERHEIGHT, 0, &hoverheight, 0);

    /* see if this tracking event is looking for TME_LEAVE and that the */
    /* mouse has left the window */
    if (ptracking_info->tme.dwFlags & TME_LEAVE)
    {
        if (ptracking_info->tme.hwndTrack != hwnd)
        {
            if (ptracking_info->tme.dwFlags & TME_NONCLIENT)
            {
                PostMessageW(ptracking_info->tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
            }
            else
            {
                PostMessageW(ptracking_info->tme.hwndTrack, WM_MOUSELEAVE, 0, 0);
            }

            /* remove the TME_LEAVE flag */
            ptracking_info->tme.dwFlags &= ~TME_LEAVE;
        }
        else
        {
            GetClientRect(hwnd, &client);
            MapWindowPoints(hwnd, NULL, (LPPOINT)&client, 2);
            if (PtInRect(&client, pos))
            {
                if (ptracking_info->tme.dwFlags & TME_NONCLIENT)
                {
                    PostMessageW(ptracking_info->tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
                    /* remove the TME_LEAVE flag */
                    ptracking_info->tme.dwFlags &= ~TME_LEAVE;
                }
            }
            else
            {
                if (!(ptracking_info->tme.dwFlags & TME_NONCLIENT))
                {
                    PostMessageW(ptracking_info->tme.hwndTrack, WM_MOUSELEAVE, 0, 0);
                    /* remove the TME_LEAVE flag */
                    ptracking_info->tme.dwFlags &= ~TME_LEAVE;
                }
            }
        }
    }

    /* see if we are tracking hovering for this hwnd */
    if (ptracking_info->tme.dwFlags & TME_HOVER)
    {
        /* has the cursor moved outside the rectangle centered around pos? */
        if ((abs(pos.x - ptracking_info->pos.x) > (hoverwidth / 2.0)) ||
            (abs(pos.y - ptracking_info->pos.y) > (hoverheight / 2.0)))
        {
            /* record this new position as the current position and reset */
            /* the iHoverTime variable to 0 */
            ptracking_info->pos = pos;
        }
        else
        {
            posClient.x = pos.x;
            posClient.y = pos.y;
            ScreenToClient(hwnd, &posClient);

            if (ptracking_info->tme.dwFlags & TME_NONCLIENT)
            {
                PostMessageW(ptracking_info->tme.hwndTrack, WM_NCMOUSEHOVER,
                            get_key_state(), MAKELPARAM( posClient.x, posClient.y ));
            }
            else
            {
                PostMessageW(ptracking_info->tme.hwndTrack, WM_MOUSEHOVER,
                            get_key_state(), MAKELPARAM( posClient.x, posClient.y ));
            }

            /* stop tracking mouse hover */
            ptracking_info->tme.dwFlags &= ~TME_HOVER;
        }
    }

    /* stop the timer if the tracking list is empty */
    if (!(ptracking_info->tme.dwFlags & (TME_HOVER | TME_LEAVE)))
    {
        KillTimer(0, ptracking_info->timer);
        RtlZeroMemory(ptracking_info,sizeof(USER32_TRACKINGLIST));
    }
}


/***********************************************************************
 * TrackMouseEvent [USER32]
 *
 * Requests notification of mouse events
 *
 * During mouse tracking WM_MOUSEHOVER or WM_MOUSELEAVE events are posted
 * to the hwnd specified in the ptme structure.  After the event message
 * is posted to the hwnd, the entry in the queue is removed.
 *
 * If the current hwnd isn't ptme->hwndTrack the TME_HOVER flag is completely
 * ignored. The TME_LEAVE flag results in a WM_MOUSELEAVE message being posted
 * immediately and the TME_LEAVE flag being ignored.
 *
 * PARAMS
 *     ptme [I,O] pointer to TRACKMOUSEEVENT information structure.
 *
 * RETURNS
 *     Success: non-zero
 *     Failure: zero
 *
 */
/*
 * @implemented
 */
BOOL
WINAPI
TrackMouseEvent(
  LPTRACKMOUSEEVENT ptme)
{
    HWND hwnd;
    POINT pos;
    DWORD hover_time;
    PUSER32_TRACKINGLIST ptracking_info;

    TRACE("%lx, %lx, %p, %lx\n", ptme->cbSize, ptme->dwFlags, ptme->hwndTrack, ptme->dwHoverTime);

    if (ptme->cbSize != sizeof(TRACKMOUSEEVENT)) {
        WARN("wrong TRACKMOUSEEVENT size from app\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ptracking_info = & User32GetThreadData()->tracking_info;

    /* fill the TRACKMOUSEEVENT struct with the current tracking for the given hwnd */
    if (ptme->dwFlags & TME_QUERY )
    {
        *ptme = ptracking_info->tme;
        ptme->cbSize = sizeof(TRACKMOUSEEVENT);

        return TRUE; /* return here, TME_QUERY is retrieving information */
    }

    if (!IsWindow(ptme->hwndTrack))
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }

    hover_time = ptme->dwHoverTime;

    /* if HOVER_DEFAULT was specified replace this with the systems current value */
    if (hover_time == HOVER_DEFAULT || hover_time == 0)
    {
        SystemParametersInfoW(SPI_GETMOUSEHOVERTIME, 0, &hover_time, 0);
    }

    GetCursorPos(&pos);
    hwnd = WindowFromPoint(pos);

    if (ptme->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT))
    {
        FIXME("Unknown flag(s) %08lx\n", ptme->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT));
    }

    if (ptme->dwFlags & TME_CANCEL)
    {
        if (ptracking_info->tme.hwndTrack == ptme->hwndTrack)
        {
            ptracking_info->tme.dwFlags &= ~(ptme->dwFlags & ~TME_CANCEL);

            /* if we aren't tracking on hover or leave remove this entry */
            if (!(ptracking_info->tme.dwFlags & (TME_HOVER | TME_LEAVE)))
            {
                KillTimer(0, ptracking_info->timer);
                RtlZeroMemory(ptracking_info,sizeof(USER32_TRACKINGLIST));
            }
        }
    } else {
        if (ptme->hwndTrack == hwnd)
        {
            /* Adding new mouse event to the tracking list */
            ptracking_info->tme = *ptme;
            ptracking_info->tme.dwHoverTime = hover_time;

            /* Initialize HoverInfo variables even if not hover tracking */
            ptracking_info->pos = pos;

            if (!ptracking_info->timer)
            {
                ptracking_info->timer = SetTimer(0, 0, hover_time, TrackMouseEventProc);
            }
        }
    }

    return TRUE;

}

/* EOF */
