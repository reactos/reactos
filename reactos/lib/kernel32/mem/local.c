/*
 * COPYRIGHT:   See COPYING in the top level directory
 *              Copyright (C) 1996, Onno Hovers, All rights reserved
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/kernel32/mem/local.cc
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
#include <kernel32/heap.h>

/* FUNCTIONS ***************************************************************/

/*********************************************************************
*                    LocalFlags  --  KERNEL32                        *
*********************************************************************/
UINT WINAPI LocalFlags(HLOCAL hmem)
{
   DWORD		retval;
   PGLOBAL_HANDLE	phandle;
   
   if(((ULONG)hmem%8)==0)
   {
      retval=0;
   }
   else
   {
      HeapLock(__ProcessHeap);
      phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
      if(phandle->Magic==MAGIC_GLOBAL_USED)
      {               
         retval=phandle->LockCount + (phandle->Flags<<8);
         if(phandle->Pointer==0)
            retval|= LMEM_DISCARDED;
      }
      else
      {
         dprintf("GlobalSize: invalid handle\n");
         retval=0;
      }
      HeapUnlock(__ProcessHeap);
   }
   return retval;
}


/*********************************************************************
*                    LocalAlloc  --  KERNEL32                        *
*********************************************************************/
HLOCAL WINAPI LocalAlloc(UINT flags, UINT size)
{
   return (HLOCAL) GlobalAlloc( flags, size );
}

/*********************************************************************
*                    LocalLock  --  KERNEL32                         *
*********************************************************************/
LPVOID WINAPI LocalLock(HLOCAL hmem)
{
   return GlobalLock( (HGLOBAL) hmem );
}

/*********************************************************************
*                    LocalUnlock  --  KERNEL32                       *
*********************************************************************/
BOOL WINAPI LocalUnlock(HLOCAL hmem)
{
   return GlobalUnlock( (HGLOBAL) hmem);
}

/*********************************************************************
*                    LocalHandle  --  KERNEL32                       *
*********************************************************************/
HLOCAL WINAPI LocalHandle(LPCVOID pmem)
{
   return (HLOCAL) GlobalHandle(pmem);
}

/*********************************************************************
*                    LocalReAlloc  --  KERNEL32                      *
*********************************************************************/
HLOCAL WINAPI LocalReAlloc(HLOCAL hmem, UINT size, UINT flags)
{
   return (HLOCAL) GlobalReAlloc( (HGLOBAL) hmem, size, flags);
}

/*********************************************************************
*                    LocalFree  --  KERNEL32                         *
*********************************************************************/
HLOCAL WINAPI LocalHeapFree(GetProcessHeap(),0,HLOCAL hmem)
{
   return (HLOCAL) GlobalHeapFree(GetProcessHeap(),0, (HGLOBAL) hmem );
}

/*********************************************************************
*                    LocalSize  --  KERNEL32                         *
*********************************************************************/
UINT WINAPI LocalSize(HLOCAL hmem)
{
   return GlobalSize( (HGLOBAL) hmem );
}

/*********************************************************************
*                    LocalShrink  --  KERNEL32                       *
*********************************************************************/
UINT WINAPI LocalShrink(HLOCAL hmem, UINT newsize)
{
   return (__ProcessHeap->End - (LPVOID) __ProcessHeap);
}
