/*
 *  ReactOS shell32 - 
 *
 *  _stubs.cpp
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
/* $Id: stubs.cpp,
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/shell32/misc/stubs.c
 * PURPOSE:         C++ Stubbed exports
 * PROGRAMMER:      Robert Dickenson (robd@reactos.org)
 */

//#include <ddk/ntddk.h>
#ifdef _MSC_VER
#include <Objbase.h>
#else
#include <windows.h>
#endif
//#define NDEBUG
//#include <debug.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "shell32.h"


#define  STUB  \
  do  \
  {   \
  }   \
  while (0)

//    DbgPrint ("%s(%d):%s not implemented\n", __FILE__, __LINE__, __FUNCTION__);


#ifndef __GNUC__

//long __stdcall
STDAPI
DllGetClassObject(const CLSID & rclsid, const IID & riid, void ** ppv)
{
  STUB;
/*
This function supports the standard return values:
    E_INVALIDARG
    E_OUTOFMEMORY
    E_UNEXPECTED
as well as the following: 
    S_OK                      - The object was retrieved successfully
    CLASS_E_CLASSNOTAVAILABLE - The DLL does not support the class (object definition)
 */
  return CLASS_E_CLASSNOTAVAILABLE;
}

#else

//VOID STDCALL
long __stdcall
DllGetClassObject(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
  return CLASS_E_CLASSNOTAVAILABLE;
}

#endif

#ifdef __cplusplus
};
#endif
