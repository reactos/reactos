/*
 *  FreeLoader
 *  Copyright (C) 1998-2003 Brian Palmer   <brianp@sginet.com>
 *  Copyright (C) 2006      Aleksey Bragin <aleksey@reactos.org>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

PVOID MmHighestUserAddress = (PVOID)MI_HIGHEST_USER_ADDRESS;

#if DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
    // boot-code is always ok
}
#endif

PVOID
NTAPI
RtlpAllocateMemory(ULONG Bytes,
                   ULONG Tag)
{
    return FrLdrHeapAllocateEx(FrLdrDefaultHeap, Bytes, Tag);
}


VOID
NTAPI
RtlpFreeMemory(PVOID Mem,
               ULONG Tag)
{
    FrLdrHeapFreeEx(FrLdrDefaultHeap, Mem, Tag);
}

NTSTATUS
NTAPI
RtlpSafeCopyMemory(
   _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
   _In_reads_bytes_(Length) CONST VOID UNALIGNED *Source,
   _In_ SIZE_T Length)
{
    RtlCopyMemory(Destination, Source, Length);
    return STATUS_SUCCESS;
}
