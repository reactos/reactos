/* $Id: local.c,v 1.6 2002/09/07 15:12:27 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 *              Copyright (C) 1996, Onno Hovers, All rights reserved
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/kernel32/mem/local.c
 * PURPOSE:     Manages the local heap
 * PROGRAMER:   Onno Hovers (original wfc version)
 *              David Welch (adapted for ReactOS)
 * UPDATE HISTORY:
 *              9/4/98: Adapted from the wfc project
 */


/* NOTES
 * 
 * The local heap is the same as the global heap for win32 and both are only
 * required for legacy apps
 *
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ***************************************************************/


HLOCAL STDCALL
LocalAlloc(UINT uFlags,
	   UINT uBytes)
{
   return (HLOCAL)GlobalAlloc(uFlags, uBytes);
}


UINT STDCALL
LocalCompact(UINT uMinFree)
{
   return RtlCompactHeap(hProcessHeap, 0);
}


UINT STDCALL
LocalFlags(HLOCAL hMem)
{
   return GlobalFlags((HGLOBAL)hMem);
}


HLOCAL STDCALL
LocalFree(HLOCAL hMem)
{
   return (HLOCAL)GlobalFree((HGLOBAL)hMem);
}


HLOCAL STDCALL
LocalHandle(LPCVOID pMem)
{
   return (HLOCAL)GlobalHandle(pMem);
}


LPVOID STDCALL
LocalLock(HLOCAL hMem)
{
   return GlobalLock((HGLOBAL)hMem);
}


HLOCAL STDCALL
LocalReAlloc(HLOCAL hMem,
	     UINT uBytes,
	     UINT uFlags)
{
   return (HLOCAL)GlobalReAlloc((HGLOBAL)hMem, uBytes, uFlags);
}


UINT STDCALL
LocalShrink(HLOCAL hMem, UINT cbNewSize)
{
   return RtlCompactHeap(hProcessHeap, 0);
}


UINT STDCALL
LocalSize(HLOCAL hMem)
{
   return GlobalSize((HGLOBAL)hMem);
}


BOOL STDCALL
LocalUnlock(HLOCAL hMem)
{
   return GlobalUnlock((HGLOBAL)hMem);
}

/* EOF */
