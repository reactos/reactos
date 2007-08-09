/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

BOOL
STDCALL
NtGdiColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget,
                             DWORD  Action)
{
  UNIMPLEMENTED;
  return FALSE;
}

HANDLE
APIENTRY
NtGdiCreateColorSpace(
    IN PLOGCOLORSPACEEXW pLogColorSpace)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
APIENTRY
NtGdiDeleteColorSpace(
    IN HANDLE hColorSpace)
{
  UNIMPLEMENTED;
  return FALSE;
}

INT
STDCALL
NtGdiEnumICMProfiles(HDC    hDC,
                    LPWSTR lpstrBuffer,
                    UINT   cch )
{
  /*
   * FIXME - build list of file names into lpstrBuffer.
   * (MULTI-SZ would probably be best format)
   * return (needed) length of buffer in bytes
   */
  UNIMPLEMENTED;
  return 0;
}

HCOLORSPACE
STDCALL
NtGdiGetColorSpace(HDC  hDC)
{
  /* FIXME: Need to to whatever GetColorSpace actually does */
  return  0;
}

BOOL
STDCALL
NtGdiGetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiGetICMProfile(HDC  hDC,
                        LPDWORD  NameSize,
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACEW  Buffer,
                           DWORD  Size)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiSetColorSpace(IN HDC hdc,
                   IN HCOLORSPACE hColorSpace)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
STDCALL
NtGdiSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
  return FALSE;
}

INT
STDCALL
NtGdiSetIcmMode(HDC  hDC,
                ULONG nCommand,
                ULONG EnableICM) // ulMode
{
  /* FIXME: this should be coded someday  */
  if (EnableICM == ICM_OFF)
    {
      return  ICM_OFF;
    }
  if (EnableICM == ICM_ON)
    {
      return  0;
    }
  if (EnableICM == ICM_QUERY)
    {
      return  ICM_OFF;
    }

  return  0;
}

BOOL
STDCALL
NtGdiSetICMProfile(HDC  hDC,
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiUpdateICMRegKey(DWORD  Reserved,
                          LPWSTR  CMID,
                          LPWSTR  Filename,
                          UINT  Command)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
