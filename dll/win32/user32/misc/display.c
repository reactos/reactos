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
 * FILE:            lib/user32/misc/dde.c
 * PURPOSE:         DDE
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>
#define NDEBUG
#include <debug.h>

#define SIZEOF_DEVMODEA_300 124
#define SIZEOF_DEVMODEA_400 148
#define SIZEOF_DEVMODEA_500 156
#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
EnumDisplayDevicesA(
  LPCSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICEA lpDisplayDevice,
  DWORD dwFlags)
{
  BOOL rc;
  UNICODE_STRING Device;
  DISPLAY_DEVICEW DisplayDeviceW;

  if ( !RtlCreateUnicodeStringFromAsciiz ( &Device, (PCSZ)lpDevice ) )
    {
      SetLastError ( ERROR_OUTOFMEMORY );
      return FALSE;
    }

  DisplayDeviceW.cb = sizeof(DISPLAY_DEVICEW);  
  rc = NtUserEnumDisplayDevices (
    &Device,
    iDevNum,
    &DisplayDeviceW,
    dwFlags );

  /* Copy result from DisplayDeviceW to lpDisplayDevice */
  lpDisplayDevice->StateFlags = DisplayDeviceW.StateFlags;
  WideCharToMultiByte(CP_ACP,0,
     DisplayDeviceW.DeviceName,wcslen(DisplayDeviceW.DeviceName),
     lpDisplayDevice->DeviceName,sizeof(lpDisplayDevice->DeviceName) / sizeof(lpDisplayDevice->DeviceName[0]),
     NULL,NULL);
  WideCharToMultiByte(CP_ACP,0,
     DisplayDeviceW.DeviceString,wcslen(DisplayDeviceW.DeviceString),
     lpDisplayDevice->DeviceString,sizeof(lpDisplayDevice->DeviceString) / sizeof(lpDisplayDevice->DeviceString[0]),
     NULL,NULL);
  WideCharToMultiByte(CP_ACP,0,
     DisplayDeviceW.DeviceID,wcslen(DisplayDeviceW.DeviceID),
     lpDisplayDevice->DeviceID,sizeof(lpDisplayDevice->DeviceID) / sizeof(lpDisplayDevice->DeviceID[0]),
     NULL,NULL);
  WideCharToMultiByte(CP_ACP,0,
     DisplayDeviceW.DeviceKey,wcslen(DisplayDeviceW.DeviceKey),
     lpDisplayDevice->DeviceKey,sizeof(lpDisplayDevice->DeviceKey) / sizeof(lpDisplayDevice->DeviceKey[0]),
     NULL,NULL);

  RtlFreeUnicodeString ( &Device );

  return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDisplayDevicesW(
  LPCWSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICE lpDisplayDevice,
  DWORD dwFlags)
{
  UNICODE_STRING Device;
  BOOL rc;

  RtlInitUnicodeString ( &Device, lpDevice );

  rc = NtUserEnumDisplayDevices (
    &Device,
    iDevNum,
    lpDisplayDevice,
    dwFlags );

  //RtlFreeUnicodeString ( &Device );

  return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDisplayMonitors(
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData)
{
  INT iCount, i;
  HMONITOR *hMonitorList;
  LPRECT pRectList;
  HANDLE hHeap;

  /* get list of monitors/rects */
  iCount = NtUserEnumDisplayMonitors(hdc, lprcClip, NULL, NULL, 0);
  if (iCount < 0)
    {
      /* FIXME: SetLastError() */
      return FALSE;
    }
  if (iCount == 0)
    {
      return TRUE;
    }

  hHeap = GetProcessHeap();
  hMonitorList = HeapAlloc(hHeap, 0, sizeof (HMONITOR) * iCount);
  if (hMonitorList == NULL)
    {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
  pRectList = HeapAlloc(hHeap, 0, sizeof (RECT) * iCount);
  if (pRectList == NULL)
    {
      HeapFree(hHeap, 0, hMonitorList);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  iCount = NtUserEnumDisplayMonitors(hdc, lprcClip, hMonitorList, pRectList, iCount);
  if (iCount <= 0)
    {
      /* FIXME: SetLastError() */
      return FALSE;
    }

  /* enumerate list */
  for (i = 0; i < iCount; i++)
    {
      HMONITOR hMonitor = hMonitorList[i];
      LPRECT pMonitorRect = pRectList + i;
      HDC hMonitorDC = NULL;

      if (hdc != NULL)
        {
          /* make monitor DC */
          hMonitorDC = hdc;
        }

      if (!lpfnEnum(hMonitor, hMonitorDC, pMonitorRect, dwData))
        break;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEA lpDevMode,
  DWORD dwFlags)
{
  BOOL rc;
  UNICODE_STRING DeviceName;
  DEVMODEW lpDevModeW;

  if ( !RtlCreateUnicodeStringFromAsciiz ( &DeviceName, (PCSZ)lpszDeviceName ) )
    {
      SetLastError ( ERROR_OUTOFMEMORY );
      return FALSE;
    }

  memset(&lpDevModeW,0,sizeof(DEVMODEW));
  lpDevModeW.dmSize = sizeof(DEVMODEW);
 
  rc = NtUserEnumDisplaySettings ( &DeviceName, iModeNum, &lpDevModeW,
                                   dwFlags );

#define COPYS(f,len) WideCharToMultiByte( CP_THREAD_ACP, 0, lpDevModeW.f, len, (LPSTR)lpDevMode->f, len, NULL, NULL )
#define COPYN(f) lpDevMode->f = lpDevModeW.f
  COPYS(dmDeviceName, CCHDEVICENAME );
  COPYN(dmSpecVersion);
  COPYN(dmDriverVersion);
  switch ( lpDevModeW.dmSize )
    {
    case SIZEOF_DEVMODEW_300:
      lpDevMode->dmSize = SIZEOF_DEVMODEA_300;
      break;
    case SIZEOF_DEVMODEW_400:
      lpDevMode->dmSize = SIZEOF_DEVMODEA_400;
      break;
    case SIZEOF_DEVMODEW_500:
    default: /* FIXME what to do??? */
      lpDevMode->dmSize = SIZEOF_DEVMODEA_500;
      break;
    }
  COPYN(dmDriverExtra);
  COPYN(dmFields);
  COPYN(dmPosition.x);
  COPYN(dmPosition.y);
  COPYN(dmScale);
  COPYN(dmCopies);
  COPYN(dmDefaultSource);
  COPYN(dmPrintQuality);
  COPYN(dmColor);
  COPYN(dmDuplex);
  COPYN(dmYResolution);
  COPYN(dmTTOption);
  COPYN(dmCollate);
  COPYS(dmFormName,CCHFORMNAME);
  COPYN(dmLogPixels);
  COPYN(dmBitsPerPel);
  COPYN(dmPelsWidth);
  COPYN(dmPelsHeight);
  COPYN(dmDisplayFlags); // aka dmNup
  COPYN(dmDisplayFrequency);

  if ( lpDevModeW.dmSize <= SIZEOF_DEVMODEW_300 )
    goto done; // we're done with 0x300 fields

  COPYN(dmICMMethod);
  COPYN(dmICMIntent);
  COPYN(dmMediaType);
  COPYN(dmDitherType);
  COPYN(dmReserved1);
  COPYN(dmReserved2);

  if ( lpDevModeW.dmSize <= SIZEOF_DEVMODEW_400 )
    goto done; // we're done with 0x400 fields

  COPYN(dmPanningWidth);
  COPYN(dmPanningHeight);

done:
  RtlFreeUnicodeString ( &DeviceName );


  return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDisplaySettingsA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEA lpDevMode)
{
  return EnumDisplaySettingsExA ( lpszDeviceName, iModeNum, lpDevMode, 0 );
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode,
  DWORD dwFlags)
{
  BOOL rc;
  UNICODE_STRING DeviceName;

  RtlInitUnicodeString ( &DeviceName, lpszDeviceName );

  rc = NtUserEnumDisplaySettings ( &DeviceName, iModeNum, lpDevMode, dwFlags );

  return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDisplaySettingsW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode)
{
  return EnumDisplaySettingsExW ( lpszDeviceName, iModeNum, lpDevMode, 0 );
}


/*
 * @implemented
 */
BOOL
STDCALL
GetMonitorInfoA(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  if (lpmi->cbSize == sizeof (MONITORINFO))
    {
      return NtUserGetMonitorInfo(hMonitor, lpmi);
    }
  else if (lpmi->cbSize != sizeof (MONITORINFOEXA))
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  else
    {
      MONITORINFOEXW miExW;
      INT res;

      miExW.cbSize = sizeof (MONITORINFOEXW);
      if (!NtUserGetMonitorInfo(hMonitor, (LPMONITORINFO)&miExW))
        {
          return FALSE;
        }
      memcpy(lpmi, &miExW, sizeof (MONITORINFO));
      res = WideCharToMultiByte(CP_THREAD_ACP, 0, miExW.szDevice, -1,
                                ((LPMONITORINFOEXA)lpmi)->szDevice, CCHDEVICENAME,
                                NULL, NULL);
      if (res == 0)
        {
          DPRINT("WideCharToMultiByte() failed!\n");
          return FALSE;
        }
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetMonitorInfoW(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  return NtUserGetMonitorInfo(hMonitor, lpmi);
}


/*
 * @implemented
 */
HMONITOR
STDCALL
MonitorFromPoint(
	IN POINT ptPoint,
	IN DWORD dwFlags )
{
  return NtUserMonitorFromPoint(ptPoint, dwFlags);
}


/*
 * @implemented
 */
HMONITOR
STDCALL
MonitorFromRect(
	IN LPCRECT lpcRect,
	IN DWORD dwFlags )
{
  return NtUserMonitorFromRect(lpcRect, dwFlags);
}


/*
 * @implemented
 */
HMONITOR
STDCALL
MonitorFromWindow(
	IN HWND hWnd,
	IN DWORD dwFlags )
{
  return NtUserMonitorFromWindow(hWnd, dwFlags);
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  LPDEVMODEA lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  LONG rc;
  UNICODE_STRING DeviceName;
  PUNICODE_STRING pDeviceName = &DeviceName;
  LPDEVMODEW pDevModeW;

  if (lpszDeviceName != NULL)
    {
      if ( !RtlCreateUnicodeStringFromAsciiz ( pDeviceName, (PCSZ)lpszDeviceName ) )
        {
          SetLastError ( ERROR_OUTOFMEMORY );
          return DISP_CHANGE_BADPARAM; /* FIXME what to return? */
        }
    }
  else
    pDeviceName = NULL;

  if (lpDevMode != NULL)
    pDevModeW = GdiConvertToDevmodeW(lpDevMode);
  else
    pDevModeW = NULL;

  rc = NtUserChangeDisplaySettings ( pDeviceName, pDevModeW, hwnd, dwflags, lParam );

  if (lpszDeviceName != NULL)
    RtlFreeUnicodeString ( &DeviceName );

  return rc;
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsA(
  LPDEVMODEA lpDevMode,
  DWORD dwflags)
{
  return ChangeDisplaySettingsExA ( NULL, lpDevMode, NULL, dwflags, 0 );
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  LONG rc;
  UNICODE_STRING DeviceName;
  PUNICODE_STRING pDeviceName = &DeviceName;

  if (lpszDeviceName != NULL)
    RtlInitUnicodeString ( pDeviceName, lpszDeviceName );
  else
    pDeviceName = NULL;

  rc = NtUserChangeDisplaySettings ( pDeviceName, lpDevMode, hwnd, dwflags, lParam );

  if (lpszDeviceName != NULL)
    RtlFreeUnicodeString ( pDeviceName );

  return rc;
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsW(
  LPDEVMODEW lpDevMode,
  DWORD dwflags)
{
  return ChangeDisplaySettingsExW ( NULL, lpDevMode, NULL, dwflags, 0 );
}
