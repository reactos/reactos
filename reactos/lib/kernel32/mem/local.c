/* $Id: local.c,v 1.11 2004/06/13 20:04:55 navaraf Exp $
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

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS ***************************************************************/


/*
 * @implemented
 */
HLOCAL STDCALL
LocalAlloc(UINT uFlags,
	   SIZE_T uBytes)
{
   return (HLOCAL)GlobalAlloc(uFlags, uBytes);
}


/*
 * @implemented
 */
SIZE_T STDCALL
LocalCompact(UINT uMinFree)
{
   return RtlCompactHeap(hProcessHeap, 0);
}


/*
 * @implemented
 */
UINT STDCALL
LocalFlags(HLOCAL hMem)
{
   return GlobalFlags((HGLOBAL)hMem);
}


/*
 * @implemented
 */
HLOCAL STDCALL
LocalFree(HLOCAL hMem)
{
   return (HLOCAL)GlobalFree((HGLOBAL)hMem);
}


/*
 * @implemented
 */
HLOCAL STDCALL
LocalHandle(LPCVOID pMem)
{
   return (HLOCAL)GlobalHandle(pMem);
}


/*
 * @implemented
 */
LPVOID STDCALL
LocalLock(HLOCAL hMem)
{
   return GlobalLock((HGLOBAL)hMem);
}


/*
 * @implemented
 */
HLOCAL STDCALL
LocalReAlloc(HLOCAL hMem,
	     SIZE_T uBytes,
	     UINT uFlags)
{
   return (HLOCAL)GlobalReAlloc((HGLOBAL)hMem, uBytes, uFlags);
}


/*
 * @implemented
 */
SIZE_T STDCALL
LocalShrink(HLOCAL hMem, UINT cbNewSize)
{
   return RtlCompactHeap(hProcessHeap, 0);
}


/*
 * @implemented
 */
UINT STDCALL
LocalSize(HLOCAL hMem)
{
   return GlobalSize((HGLOBAL)hMem);
}


/*
 * @implemented
 */
BOOL STDCALL
LocalUnlock(HLOCAL hMem)
{
   return GlobalUnlock((HGLOBAL)hMem);
}

/* EOF */
