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


/* Directory to load key layouts from */
#define SYSTEMROOT_DIR L"\\SystemRoot\\System32\\"


/* GLOBALS *******************************************************************/

typedef struct __TRACKINGLIST {
    TRACKMOUSEEVENT tme;
    POINT pos; /* center of hover rectangle */
} _TRACKINGLIST;

/* FIXME: move tracking stuff into a per thread data */
static _TRACKINGLIST tracking_info;
static UINT_PTR timer;
static const INT iTimerInterval = 50; /* msec for timer interval */


/* LOCALE FUNCTIONS **********************************************************/

/*
 * Utility function to read a value from the registry more easily.
 *
 * IN  PUNICODE_STRING KeyName       -> Name of key to open
 * IN  PUNICODE_STRING ValueName     -> Name of value to open
 * OUT PUNICODE_STRING ReturnedValue -> String contained in registry
 *
 * Returns NTSTATUS
 */

static 
NTSTATUS FASTCALL
ReadRegistryValue( PUNICODE_STRING KeyName,
      PUNICODE_STRING ValueName,
      PUNICODE_STRING ReturnedValue )
{
   NTSTATUS Status;
   HANDLE KeyHandle;
   OBJECT_ATTRIBUTES KeyAttributes;
   PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
   ULONG Length = 0;
   ULONG ResLength = 0;
   UNICODE_STRING Temp;

   InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE,
                              NULL, NULL);
   Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &KeyAttributes);
   if( !NT_SUCCESS(Status) )
   {
      return Status;
   }

   Status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation,
                            0,
                            0,
                            &ResLength);

   if( Status != STATUS_BUFFER_TOO_SMALL )
   {
      NtClose(KeyHandle);
      return Status;
   }

   ResLength += sizeof( *KeyValuePartialInfo );
   KeyValuePartialInfo = LocalAlloc(LMEM_ZEROINIT, ResLength);
   Length = ResLength;

   if( !KeyValuePartialInfo )
   {
      NtClose(KeyHandle);
      return STATUS_NO_MEMORY;
   }

   Status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation,
                            (PVOID)KeyValuePartialInfo,
                            Length,
                            &ResLength);

   if( !NT_SUCCESS(Status) )
   {
      NtClose(KeyHandle);
      LocalFree(KeyValuePartialInfo);
      return Status;
   }

   Temp.Length = Temp.MaximumLength = KeyValuePartialInfo->DataLength;
   Temp.Buffer = (PWCHAR)KeyValuePartialInfo->Data;

   /* At this point, KeyValuePartialInfo->Data contains the key data */
   RtlInitUnicodeString(ReturnedValue,L"");
   RtlAppendUnicodeStringToString(ReturnedValue,&Temp);

   LocalFree(KeyValuePartialInfo);
   NtClose(KeyHandle);

   return Status;
}


static
HKL FASTCALL
IntLoadKeyboardLayout( LPCWSTR pwszKLID,
                       UINT Flags)
{
  HANDLE Handle;
  HINSTANCE KBModule = 0;
  FARPROC pAddr = 0;
  DWORD offTable = 0;
  HKL hKL;
  NTSTATUS Status;
  WCHAR LocaleBuffer[16];
  UNICODE_STRING LayoutKeyName;
  UNICODE_STRING LayoutValueName;
  UNICODE_STRING DefaultLocale;
  UNICODE_STRING LayoutFile;
  UNICODE_STRING FullLayoutPath;
  LCID LocaleId;  
  PWCHAR KeyboardLayoutWSTR;
  ULONG_PTR layout;
  LANGID langid;

  layout = (ULONG_PTR) wcstoul(pwszKLID, NULL, 16);

//  LocaleId = GetSystemDefaultLCID();

  LocaleId = (LCID) layout;
  
  /* Create the HKL to be used by NtUserLoadKeyboardLayoutEx*/
  /* 
   * Microsoft Office expects this value to be something specific
   * for Japanese and Korean Windows with an IME the value is 0xe001
   * We should probably check to see if an IME exists and if so then
   * set this word properly.
   */
  langid = PRIMARYLANGID(LANGIDFROMLCID(layout));
  if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
      layout |= 0xe001 << 16; /* FIXME */
  else
      layout |= layout << 16;

  DPRINT("Input  = %S\n", pwszKLID );

  DPRINT("DefaultLocale = %lx\n", LocaleId);

  swprintf(LocaleBuffer, L"%08lx", LocaleId);

  DPRINT("DefaultLocale = %S\n", LocaleBuffer);
  RtlInitUnicodeString(&DefaultLocale, LocaleBuffer);

  RtlInitUnicodeString(&LayoutKeyName,
                       L"\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet"
                       L"\\Control\\KeyboardLayouts\\");

  RtlAppendUnicodeStringToString(&LayoutKeyName,&DefaultLocale);

  RtlInitUnicodeString(&LayoutValueName,L"Layout File");

  Status =  ReadRegistryValue(&LayoutKeyName,&LayoutValueName,&LayoutFile);

  RtlFreeUnicodeString(&LayoutKeyName);
  
  DPRINT("Read registry and got %wZ\n", &LayoutFile);

  RtlInitUnicodeString(&FullLayoutPath,SYSTEMROOT_DIR);
  RtlAppendUnicodeStringToString(&FullLayoutPath,&LayoutFile);

  DPRINT("Loading Keyboard DLL %wZ\n", &FullLayoutPath);

  RtlFreeUnicodeString(&LayoutFile);

  KeyboardLayoutWSTR = LocalAlloc(LMEM_ZEROINIT, 
                                    FullLayoutPath.Length + sizeof(WCHAR));

  if( !KeyboardLayoutWSTR )
  {
     DPRINT1("Couldn't allocate a string for the keyboard layout name.\n");
     RtlFreeUnicodeString(&FullLayoutPath);
     return NULL;
  }
  memcpy(KeyboardLayoutWSTR,FullLayoutPath.Buffer, FullLayoutPath.Length);

  KeyboardLayoutWSTR[FullLayoutPath.Length / sizeof(WCHAR)] = 0;

  KBModule = LoadLibraryW(KeyboardLayoutWSTR);

  DPRINT( "Load Keyboard Layout: %S\n", KeyboardLayoutWSTR );

  if( !KBModule )
     DPRINT1( "Load Keyboard Layout: No %wZ\n", &FullLayoutPath );

  pAddr = GetProcAddress( KBModule, (LPCSTR) 1);
  offTable = (DWORD) pAddr - (DWORD) KBModule; // Weeks to figure this out!

  DPRINT( "Load Keyboard Module Offset: %x\n", offTable );

  FreeLibrary(KBModule);

  Handle = CreateFileW( KeyboardLayoutWSTR,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

  hKL = NtUserLoadKeyboardLayoutEx( Handle,
                                    offTable,
                                   (HKL) layout,
                                   &DefaultLocale,
                                   (UINT) layout,
                                    Flags);

  NtClose(Handle);

  LocalFree(KeyboardLayoutWSTR);
  RtlFreeUnicodeString(&FullLayoutPath);

  return hKL;
}


/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
BOOL
STDCALL
DragDetect(
  HWND hWnd,
  POINT pt)
{
#if 0
  return NtUserDragDetect(hWnd, pt.x, pt.y);
#else
  MSG msg;
  RECT rect;
  POINT tmp;
  ULONG dx = NtUserGetSystemMetrics(SM_CXDRAG);
  ULONG dy = NtUserGetSystemMetrics(SM_CYDRAG);

  rect.left = pt.x - dx;
  rect.right = pt.x + dx;
  rect.top = pt.y - dy;
  rect.bottom = pt.y + dy;

  SetCapture(hWnd);

  for (;;)
  {
    while (PeekMessageW(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
    {
      if (msg.message == WM_LBUTTONUP)
      {
        ReleaseCapture();
        return 0;
      }
      if (msg.message == WM_MOUSEMOVE)
      {
        tmp.x = LOWORD(msg.lParam);
        tmp.y = HIWORD(msg.lParam);
        if (!PtInRect(&rect, tmp))
        {
          ReleaseCapture();
          return 1;
        }
      }
    }
    WaitMessage();
  }
  return 0;
#endif
}


/*
 * @unimplemented
 */
HKL STDCALL
ActivateKeyboardLayout(HKL hkl,
		       UINT Flags)
{
  UNIMPLEMENTED;
  return (HKL)0;
}


/*
 * @implemented
 */
BOOL STDCALL
BlockInput(BOOL fBlockIt)
{
  return NtUserBlockInput(fBlockIt);
}


/*
 * @implemented
 */
BOOL STDCALL
EnableWindow(HWND hWnd,
	     BOOL bEnable)
{
    LONG Style = NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE);
    Style = bEnable ? Style & ~WS_DISABLED : Style | WS_DISABLED;
    NtUserSetWindowLong(hWnd, GWL_STYLE, Style, FALSE);

    SendMessageA(hWnd, WM_ENABLE, (LPARAM) IsWindowEnabled(hWnd), 0);

    // Return nonzero if it was disabled, or zero if it wasn't:
    return IsWindowEnabled(hWnd);
}


/*
 * @implemented
 */
SHORT STDCALL
GetAsyncKeyState(int vKey)
{
 return (SHORT) NtUserGetAsyncKeyState((DWORD) vKey);
}


/*
 * @implemented
 */
UINT
STDCALL
GetDoubleClickTime(VOID)
{
  return NtUserGetDoubleClickTime();
}


/*
 * @implemented
 */
HKL STDCALL
GetKeyboardLayout(DWORD idThread)
{
  return (HKL)NtUserCallOneParam((DWORD) idThread,  ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT);
}


/*
 * @implemented
 */
UINT STDCALL
GetKBCodePage(VOID)
{
  return GetOEMCP();
}


/*
 * @implemented
 */
int STDCALL
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
int STDCALL
GetKeyNameTextW(LONG lParam,
		LPWSTR lpString,
		int nSize)
{
  return NtUserGetKeyNameText( lParam, lpString, nSize );
}


/*
 * @implemented
 */
SHORT STDCALL
GetKeyState(int nVirtKey)
{
 return (SHORT) NtUserGetKeyState((DWORD) nVirtKey);
}


/*
 * @unimplemented
 */
UINT STDCALL
GetKeyboardLayoutList(int nBuff,
		      HKL FAR *lpList)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
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
BOOL STDCALL
GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
  return NtUserGetKeyboardLayoutName( pwszKLID );
}


/*
 * @implemented
 */
BOOL STDCALL
GetKeyboardState(PBYTE lpKeyState)
{

  return (BOOL) NtUserGetKeyboardState((LPBYTE) lpKeyState);
}


/*
 * @implemented
 */
int STDCALL
GetKeyboardType(int nTypeFlag)
{
return (int)NtUserCallOneParam((DWORD) nTypeFlag,  ONEPARAM_ROUTINE_GETKEYBOARDTYPE);
}


/*
 * @implemented
 */
BOOL STDCALL
GetLastInputInfo(PLASTINPUTINFO plii)
{
  return NtUserGetLastInputInfo(plii);
}


/*
 * @implemented
 */
HKL STDCALL
LoadKeyboardLayoutA(LPCSTR pwszKLID,
		    UINT Flags)
{
  HKL ret;
  UNICODE_STRING pwszKLIDW;
        
  if (pwszKLID) RtlCreateUnicodeStringFromAsciiz(&pwszKLIDW, pwszKLID);
  else pwszKLIDW.Buffer = NULL;
                
  ret = LoadKeyboardLayoutW(pwszKLIDW.Buffer, Flags);
  RtlFreeUnicodeString(&pwszKLIDW);
  return ret;
}


/*
 * @implemented
 */
HKL STDCALL
LoadKeyboardLayoutW(LPCWSTR pwszKLID,
		    UINT Flags)
{
  return IntLoadKeyboardLayout( pwszKLID, Flags);
}


/*
 * @implemented
 */
UINT STDCALL
MapVirtualKeyA(UINT uCode,
	       UINT uMapType)
{
  return MapVirtualKeyExA( uCode, uMapType, GetKeyboardLayout( 0 ) );
}


/*
 * @implemented
 */
UINT STDCALL
MapVirtualKeyExA(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return MapVirtualKeyExW( uCode, uMapType, dwhkl );
}


/*
 * @implemented
 */
UINT STDCALL
MapVirtualKeyExW(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return NtUserMapVirtualKeyEx( uCode, uMapType, 0, dwhkl );
}


/*
 * @implemented
 */
UINT STDCALL
MapVirtualKeyW(UINT uCode,
	       UINT uMapType)
{
  return MapVirtualKeyExW( uCode, uMapType, GetKeyboardLayout( 0 ) );
}


/*
 * @implemented
 */ 
DWORD STDCALL
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
BOOL STDCALL
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
BOOL STDCALL
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
HWND STDCALL
SetFocus(HWND hWnd)
{
  return NtUserSetFocus(hWnd);
}


/*
 * @implemented
 */
BOOL STDCALL
SetKeyboardState(LPBYTE lpKeyState)
{
 return (BOOL) NtUserSetKeyboardState((LPBYTE)lpKeyState);
}


/*
 * @implemented
 */
BOOL
STDCALL
SwapMouseButton(
  BOOL fSwap)
{
  return NtUserSwapMouseButton(fSwap);
}


/*
 * @implemented
 */
int STDCALL
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
int STDCALL
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
int STDCALL
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
int STDCALL
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
 * @unimplemented
 */
BOOL STDCALL
UnloadKeyboardLayout(HKL hkl)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
UnregisterHotKey(HWND hWnd,
		 int id)
{
  return (BOOL)NtUserUnregisterHotKey(hWnd, id);
}


/*
 * @implemented
 */
SHORT STDCALL
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
SHORT STDCALL
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
SHORT STDCALL
VkKeyScanExW(WCHAR ch,
	     HKL dwhkl)
{
  return (SHORT) NtUserVkKeyScanEx((DWORD) ch,(DWORD) dwhkl,(DWORD)NULL);
}


/*
 * @implemented
 */
SHORT STDCALL
VkKeyScanW(WCHAR ch)
{
  return VkKeyScanExW(ch, GetKeyboardLayout(0));
}


/*
 * @implemented
 */
UINT
STDCALL
SendInput(
  UINT nInputs,
  LPINPUT pInputs,
  int cbSize)
{
  return NtUserSendInput(nInputs, pInputs, cbSize);
}

/*
 * Private call for CSRSS
 */
VOID
STDCALL
PrivateCsrssRegisterPrimitive(VOID)
{
  NtUserCallNoParam(NOPARAM_ROUTINE_REGISTER_PRIMITIVE);
}

/*
 * Another private call for CSRSS
 */
VOID
STDCALL
PrivateCsrssAcquireOrReleaseInputOwnership(BOOL Release)
{
  NtUserAcquireOrReleaseInputOwnership(Release);
}

/*
 * @implemented
 */
VOID
STDCALL
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
STDCALL
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

    GetCursorPos(&pos);
    hwnd = WindowFromPoint(pos);

//    SystemParametersInfoW(SPI_GETMOUSEHOVERWIDTH, 0, &hoverwidth, 0);
    hoverwidth = 4;
//    SystemParametersInfoW(SPI_GETMOUSEHOVERHEIGHT, 0, &hoverheight, 0);
    hoverheight = 4;

    /* see if this tracking event is looking for TME_LEAVE and that the */
    /* mouse has left the window */
    if (tracking_info.tme.dwFlags & TME_LEAVE)
    {
        if (tracking_info.tme.hwndTrack != hwnd)
        {
            if (tracking_info.tme.dwFlags & TME_NONCLIENT)
                PostMessageW(tracking_info.tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
            else
                PostMessageW(tracking_info.tme.hwndTrack, WM_MOUSELEAVE, 0, 0);

            /* remove the TME_LEAVE flag */
            tracking_info.tme.dwFlags &= ~TME_LEAVE;
        }
        else
        {
            GetClientRect(hwnd, &client);
            MapWindowPoints(hwnd, NULL, (LPPOINT)&client, 2);
            if (PtInRect(&client, pos))
            {
                if (tracking_info.tme.dwFlags & TME_NONCLIENT)
                {
                    PostMessageW(tracking_info.tme.hwndTrack, WM_NCMOUSELEAVE, 0, 0);
                    /* remove the TME_LEAVE flag */
                    tracking_info.tme.dwFlags &= ~TME_LEAVE;
                }
            }
            else
            {
                if (!(tracking_info.tme.dwFlags & TME_NONCLIENT))
                {
                    PostMessageW(tracking_info.tme.hwndTrack, WM_MOUSELEAVE, 0, 0);
                    /* remove the TME_LEAVE flag */
                    tracking_info.tme.dwFlags &= ~TME_LEAVE;
                }
            }
        }
    }

    /* see if we are tracking hovering for this hwnd */
    if (tracking_info.tme.dwFlags & TME_HOVER)
    {
        /* has the cursor moved outside the rectangle centered around pos? */
        if ((abs(pos.x - tracking_info.pos.x) > (hoverwidth / 2.0)) ||
            (abs(pos.y - tracking_info.pos.y) > (hoverheight / 2.0)))
        {
            /* record this new position as the current position and reset */
            /* the iHoverTime variable to 0 */
            tracking_info.pos = pos;
        }
        else
        {
            posClient.x = pos.x;
            posClient.y = pos.y;
            ScreenToClient(hwnd, &posClient);

            if (tracking_info.tme.dwFlags & TME_NONCLIENT)
                PostMessageW(tracking_info.tme.hwndTrack, WM_NCMOUSEHOVER,
                            get_key_state(), MAKELPARAM( posClient.x, posClient.y ));
            else
                PostMessageW(tracking_info.tme.hwndTrack, WM_MOUSEHOVER,
                            get_key_state(), MAKELPARAM( posClient.x, posClient.y ));

            /* stop tracking mouse hover */
            tracking_info.tme.dwFlags &= ~TME_HOVER;
        }
    }

    /* stop the timer if the tracking list is empty */
    if (!(tracking_info.tme.dwFlags & (TME_HOVER | TME_LEAVE)))
    {
        memset(&tracking_info, 0, sizeof(tracking_info));

        KillTimer(0, timer);
        timer = 0;
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
 * @unimplemented
 */
BOOL
STDCALL
TrackMouseEvent(
  LPTRACKMOUSEEVENT ptme)
{
    HWND hwnd;
    POINT pos;
    DWORD hover_time;

    TRACE("%lx, %lx, %p, %lx\n", ptme->cbSize, ptme->dwFlags, ptme->hwndTrack, ptme->dwHoverTime);

    if (ptme->cbSize != sizeof(TRACKMOUSEEVENT)) {
        WARN("wrong TRACKMOUSEEVENT size from app\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* fill the TRACKMOUSEEVENT struct with the current tracking for the given hwnd */
    if (ptme->dwFlags & TME_QUERY )
    {
        *ptme = tracking_info.tme;

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
//        SystemParametersInfoW(SPI_GETMOUSEHOVERTIME, 0, &hover_time, 0);
        hover_time = 400;

    GetCursorPos(&pos);
    hwnd = WindowFromPoint(pos);

    if (ptme->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT))
        FIXME("Unknown flag(s) %08lx\n", ptme->dwFlags & ~(TME_CANCEL | TME_HOVER | TME_LEAVE | TME_NONCLIENT));

    if (ptme->dwFlags & TME_CANCEL)
    {
        if (tracking_info.tme.hwndTrack == ptme->hwndTrack)
        {
            tracking_info.tme.dwFlags &= ~(ptme->dwFlags & ~TME_CANCEL);

            /* if we aren't tracking on hover or leave remove this entry */
            if (!(tracking_info.tme.dwFlags & (TME_HOVER | TME_LEAVE)))
            {
                memset(&tracking_info, 0, sizeof(tracking_info));

                KillTimer(0, timer);
                timer = 0;
            }
        }
    } else {
        if (ptme->hwndTrack == hwnd)
        {
            /* Adding new mouse event to the tracking list */
            tracking_info.tme = *ptme;
            tracking_info.tme.dwHoverTime = hover_time;

            /* Initialize HoverInfo variables even if not hover tracking */
            tracking_info.pos = pos;

            if (!timer)
                timer = SetTimer(0, 0, hover_time, TrackMouseEventProc);
        }
    }

    return TRUE;
 
}

/* EOF */
