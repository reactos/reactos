/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/*
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode Debug Buffer support
 * FILE:              lib/ntdll/rtl/dbgbuffer.c
 * PROGRAMER:         James Tabor
 * Fixme:             Add Process and Thread event pair support.
 *                    Fix Heap support.
 *
 */

/* INCLUDES *****************************************************************/

#include <ntos/types.h>
#include <napi/teb.h>
#include <ntdll/rtl.h>
#include <ddk/ntddk.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
PDEBUG_BUFFER STDCALL
RtlCreateQueryDebugBuffer(IN ULONG Size,
                          IN BOOLEAN EventPair)
{
   PDEBUG_BUFFER Buf = NULL;
   
   if (Size < sizeof(DEBUG_BUFFER))
     {
        Size = sizeof(DEBUG_BUFFER);
     }
      Buf = (PDEBUG_BUFFER) RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
      memset(Buf, 0, Size);

   return Buf;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER Buf)
{
   NTSTATUS Status = STATUS_SUCCESS;

   if (NULL != Buf) {
     if (NULL != Buf->ModuleInformation)
         RtlFreeHeap(RtlGetProcessHeap(), 0, Buf->ModuleInformation);

     if (NULL != Buf->HeapInformation)
         RtlFreeHeap(RtlGetProcessHeap(), 0, Buf->HeapInformation);

     if (NULL != Buf->LockInformation)
         RtlFreeHeap(RtlGetProcessHeap(), 0, Buf->LockInformation);

     RtlFreeHeap(RtlGetProcessHeap(), 0, Buf);
   }
   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL 
RtlQueryProcessDebugInformation(IN ULONG ProcessId, 
                                IN ULONG DebugInfoMask, 
                                IN OUT PDEBUG_BUFFER Buf)
{
   NTSTATUS Status = STATUS_SUCCESS;

   Buf->InfoClassMask = DebugInfoMask;
   
   if (DebugInfoMask & PDI_MODULES)
     {
        PDEBUG_MODULE_INFORMATION info = 
     	   RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DEBUG_MODULE_INFORMATION));
        memset(info, 0, sizeof(DEBUG_MODULE_INFORMATION));
        Buf->ModuleInformation = info;
     }
     
   if (DebugInfoMask & PDI_HEAPS)
     {
        PDEBUG_HEAP_INFORMATION info = 
           RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DEBUG_HEAP_INFORMATION));
        memset(info, 0, sizeof(DEBUG_HEAP_INFORMATION));
     
        if (DebugInfoMask & PDI_HEAP_TAGS)
          {
          }
        if (DebugInfoMask & PDI_HEAP_BLOCKS)
          {
          }
        Buf->HeapInformation = info;
     }
     
   if (DebugInfoMask & PDI_LOCKS)
     {
        PDEBUG_LOCK_INFORMATION info = 
           RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(DEBUG_LOCK_INFORMATION));
        memset(info, 0, sizeof(DEBUG_LOCK_INFORMATION));
        Buf->LockInformation = info;
    }
   return Status;

}

/* EOL */
