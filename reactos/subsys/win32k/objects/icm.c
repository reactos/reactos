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
/* $Id: icm.c,v 1.6 2003/05/18 17:16:18 ea Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/icm.h>

#define NDEBUG
#include <win32k/debug1.h>

BOOL
STDCALL
W32kCheckColorsInGamut(HDC  hDC,
                             LPVOID  RGBTriples,
                             LPVOID  Buffer,
                             UINT  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget, 
                             DWORD  Action)
{
  UNIMPLEMENTED;
}

HCOLORSPACE
STDCALL
W32kCreateColorSpace(LPLOGCOLORSPACE  LogColorSpace)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kDeleteColorSpace(HCOLORSPACE  hColorSpace)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEnumICMProfiles(HDC  hDC,  
                         ICMENUMPROC  EnumICMProfilesFunc,
                         LPARAM lParam)
{
  UNIMPLEMENTED;
}

HCOLORSPACE
STDCALL
W32kGetColorSpace(HDC  hDC)
{
  /* FIXME: Need to to whatever GetColorSpace actually does */
  return  0;
}

BOOL
STDCALL
W32kGetDeviceGammaRamp(HDC  hDC,  
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetICMProfile(HDC  hDC,  
                        LPDWORD  NameSize,  
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACE  Buffer,  
                           DWORD  Size)
{
  UNIMPLEMENTED;
}

HCOLORSPACE
STDCALL
W32kSetColorSpace(HDC  hDC,
                               HCOLORSPACE  hColorSpace)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kSetICMMode(HDC  hDC,
                    INT  EnableICM)
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
W32kSetICMProfile(HDC  hDC,
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kUpdateICMRegKey(DWORD  Reserved,  
                          LPWSTR  CMID, 
                          LPWSTR  Filename,
                          UINT  Command)
{
  UNIMPLEMENTED;
}

/* EOF */
