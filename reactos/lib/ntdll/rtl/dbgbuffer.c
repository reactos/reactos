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
 *                    Start Locks and Heap support. 
 *                    Test: Create remote thread to help query remote                    
 *                    processes and use view mapping to read them.
 *
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ntos/types.h>
#include <napi/teb.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <ddk/ntddk.h>

#include <rosrtl/thread.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
PDEBUG_BUFFER STDCALL
RtlCreateQueryDebugBuffer(IN ULONG Size,
                          IN BOOLEAN EventPair)
{
   NTSTATUS Status;
   PDEBUG_BUFFER Buf = NULL;
   ULONG SectionSize  = 100 * PAGE_SIZE;
   
   Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                    (PVOID)&Buf,
                                     0,
                                    &SectionSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
        return NULL;
     }

   Buf->SectionBase = Buf;
   Buf->SectionSize = SectionSize;
   
   DPRINT("RtlCQDB: BA: %x BS: %d\n", Buf->SectionBase, Buf->SectionSize);

   return Buf;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER Buf)
{
   NTSTATUS Status = STATUS_SUCCESS;

   if (NULL != Buf)
     {
     Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                  (PVOID)&Buf,
                                  &Buf->SectionSize,
                                   MEM_RELEASE);
     }
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("RtlDQDB: Failed to free VM!\n");
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
   Buf->SizeOfInfo = 0;

   DPRINT("QueryProcessDebugInformation Start\n");
   
   if (DebugInfoMask & PDI_MODULES)
     {
    PMODULE_INFORMATION Mp;
    ULONG MSize;

    Mp = (PMODULE_INFORMATION)(Buf + sizeof(DEBUG_BUFFER) + Buf->SizeOfInfo);
    MSize = sizeof(MODULE_INFORMATION);
    Buf->ModuleInformation = Mp;        
    Buf->SizeOfInfo = Buf->SizeOfInfo + MSize;
     }
     
   if (DebugInfoMask & PDI_HEAPS)
     {
   PDEBUG_HEAP_INFORMATION Hp;
   ULONG HSize;

   Hp = (PDEBUG_HEAP_INFORMATION)(Buf + sizeof(DEBUG_BUFFER) + Buf->SizeOfInfo);
   HSize = sizeof(DEBUG_HEAP_INFORMATION);
        if (DebugInfoMask & PDI_HEAP_TAGS)
          {
          }
        if (DebugInfoMask & PDI_HEAP_BLOCKS)
          {
          }
   Buf->HeapInformation = Hp;        
   Buf->SizeOfInfo = Buf->SizeOfInfo + HSize;
        
     }
     
   if (DebugInfoMask & PDI_LOCKS)
     {
   PDEBUG_LOCK_INFORMATION Lp;
   ULONG LSize;
   
   Lp = (PDEBUG_LOCK_INFORMATION)(Buf + sizeof(DEBUG_BUFFER) + Buf->SizeOfInfo);
   LSize = sizeof(DEBUG_LOCK_INFORMATION);
   Buf->LockInformation = Lp;        
   Buf->SizeOfInfo = Buf->SizeOfInfo + LSize;
    }

   DPRINT("QueryProcessDebugInformation end \n");
   DPRINT("QueryDebugInfo : %d\n", Buf->SizeOfInfo);

   return Status;

}

/* EOL */
