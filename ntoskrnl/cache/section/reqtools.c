/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cache/section/reqtools.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Emanuele Aliberti
 *                  Eugene Ingerman
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Guido de Jong
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Filip Navara
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Gregor Anich
 *                  Steven Edwards
 *                  Herve Poussineau
 */

/*
  This file contains functions used by fault.c to do blocking resource
  acquisition.  To call one of these functions, fill out your
  MM_REQUIRED_RESOURCES with a pointer to the desired function and configure
  the other members as below.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#define NDEBUG
#include <debug.h>

#define DPRINTC DPRINT

VOID
NTAPI
MmBuildMdlFromPages(PMDL Mdl, PPFN_NUMBER Pages);

/*

Blocking function to acquire zeroed pages from the balancer.

Upon entry:

Required->Amount: Number of pages to acquire
Required->Consumer: consumer to charge the page to

Upon return:

Required->Pages[0..Amount]: Allocated pages.

The function fails unless all requested pages can be allocated.

 */
NTSTATUS
NTAPI
MiGetOnePage(PMMSUPPORT AddressSpace,
             PMEMORY_AREA MemoryArea,
             PMM_REQUIRED_RESOURCES Required)
{
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i = 0; i < Required->Amount; i++)
    {
        DPRINTC("MiGetOnePage(%s:%d)\n", Required->File, Required->Line);
        Status = MmRequestPageMemoryConsumer(Required->Consumer,
                                             TRUE,
                                             &Required->Page[i]);
        if (!NT_SUCCESS(Status))
        {
            while (i > 0)
            {
                MmReleasePageMemoryConsumer(Required->Consumer,
                                            Required->Page[i-1]);
                i--;
            }
            return Status;
        }
    }

    return Status;
}

/*

Blocking function to read (part of) a page from a file.

Upon entry:

Required->Context: a FILE_OBJECT to read
Required->Consumer: consumer to charge the page to
Required->FileOffset: Offset to read at
Required->Amount: Number of bytes to read (0 -> 4096)

Upon return:

Required->Page[Required->Offset]: The allocated and read in page

The indicated page is filled to Required->Amount with file data and zeroed
afterward.

 */

NTSTATUS
NTAPI
MiReadFilePage(PMMSUPPORT AddressSpace,
               PMEMORY_AREA MemoryArea,
               PMM_REQUIRED_RESOURCES RequiredResources)
{
    PFILE_OBJECT FileObject = RequiredResources->Context;
    PPFN_NUMBER Page = &RequiredResources->Page[RequiredResources->Offset];
    PLARGE_INTEGER FileOffset = &RequiredResources->FileOffset;
    NTSTATUS Status;
    PVOID PageBuf = NULL;
    KEVENT Event;
    IO_STATUS_BLOCK IOSB;
    UCHAR MdlBase[sizeof(MDL) + sizeof(ULONG)];
    PMDL Mdl = (PMDL)MdlBase;
    KIRQL OldIrql;

    DPRINTC("Pulling page %I64x from %wZ to %Ix\n",
            FileOffset->QuadPart,
            &FileObject->FileName,
            *Page);

    Status = MmRequestPageMemoryConsumer(RequiredResources->Consumer,
                                         TRUE,
                                         Page);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Status: %x\n", Status);
        return Status;
    }

    MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
    MmBuildMdlFromPages(Mdl, Page);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoPageRead(FileObject, Mdl, FileOffset, &Event, &IOSB);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IOSB.Status;
    }
    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
    }

    PageBuf = MiMapPageInHyperSpace(PsGetCurrentProcess(), *Page, &OldIrql);
    if (!PageBuf)
    {
        MmReleasePageMemoryConsumer(RequiredResources->Consumer, *Page);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory((PCHAR)PageBuf+RequiredResources->Amount,
                  PAGE_SIZE-RequiredResources->Amount);

    MiUnmapPageInHyperSpace(PsGetCurrentProcess(), PageBuf, OldIrql);

    DPRINT("Read Status %x (Page %x)\n", Status, *Page);

    if (!NT_SUCCESS(Status))
    {
        MmReleasePageMemoryConsumer(RequiredResources->Consumer, *Page);
        DPRINT("Status: %x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

/*

Blocking function to read a swap page into a memory page.

Upon entry:

Required->Consumer: consumer to charge the page to
Required->SwapEntry: swap entry to use

Upon return:

Required->Page[Required->Offset]: Populated page

*/

NTSTATUS
NTAPI
MiSwapInPage(PMMSUPPORT AddressSpace,
             PMEMORY_AREA MemoryArea,
             PMM_REQUIRED_RESOURCES Resources)
{
    NTSTATUS Status;

    Status = MmRequestPageMemoryConsumer(Resources->Consumer,
                                         TRUE,
                                         &Resources->Page[Resources->Offset]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmRequestPageMemoryConsumer failed, status = %x\n", Status);
        return Status;
    }

    Status = MmReadFromSwapPage(Resources->SwapEntry,
                                Resources->Page[Resources->Offset]);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
        return Status;
    }

    MmSetSavedSwapEntryPage(Resources->Page[Resources->Offset],
                            Resources->SwapEntry);

    DPRINT1("MiSwapInPage(%x,%x)\n",
            Resources->Page[Resources->Offset],
            Resources->SwapEntry);

    return Status;
}

/*

A way to write a page without a lock acquired using the same blocking mechanism
as resource acquisition.

Upon entry:

Required->Page[Required->Offset]: Page to write
Required->Context: FILE_OBJECT to write to
Required->FileOffset: offset to write at

This always does a paging write with whole page size.  Note that paging IO
doesn't change the valid data length of a file.

*/

NTSTATUS
NTAPI
MiWriteFilePage(PMMSUPPORT AddressSpace,
                PMEMORY_AREA MemoryArea,
                PMM_REQUIRED_RESOURCES Required)
{
    DPRINT1("MiWriteFilePage(%x,%x)\n",
            Required->Page[Required->Offset],
            Required->FileOffset.LowPart);

    return MiWriteBackPage(Required->Context,
                           &Required->FileOffset,
                           PAGE_SIZE,
                           Required->Page[Required->Offset]);
}
