/* $Id: local.c,v 1.9 2003/07/10 18:50:51 chorns Exp $
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
#include <kernel32/kernel32.h>

/* FUNCTIONS ***************************************************************/


/*
 * @implemented
 */
HLOCAL STDCALL
LocalAlloc(UINT uFlags,
	   UINT uBytes)
{
   return (HLOCAL)GlobalAlloc(uFlags, uBytes);
}


/*
 * @implemented
 */
UINT STDCALL
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
	     UINT uBytes,
	     UINT uFlags)
{
   return (HLOCAL)GlobalReAlloc((HGLOBAL)hMem, uBytes, uFlags);
}


/*
 * @implemented
 */
UINT STDCALL
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
