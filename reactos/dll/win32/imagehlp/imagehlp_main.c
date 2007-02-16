/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */



/* INCLUDES ******************************************************************/

#include "precomp.h"

//#define NDEBUG
#include <debug.h>
#define _WINNT_H
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imagehlp);

/**********************************************************************/
HANDLE IMAGEHLP_hHeap = NULL;

/***********************************************************************
 *           DllMain (IMAGEHLP.init)
 */
BOOL IMAGEAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hinstDLL);
      IMAGEHLP_hHeap = HeapCreate(0, 0x10000, 0);
      break;
    case DLL_PROCESS_DETACH:
      HeapDestroy(IMAGEHLP_hHeap);
      IMAGEHLP_hHeap = NULL;
      break;
    default:
      break;
    }
  return TRUE;
}
