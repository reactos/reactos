/*
 * NTOSKRNL Io Regressions KM-Test
 * ReactOS Kernel Mode Regression Testing framework
 *
 * Copyright 2006 Aleksey Bragin <aleksey@reactos.org>
 * Copyright 2008 Etersoft (Alexander Morozov)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include <ddk/ntddk.h>
#include "kmtest.h"

#define NDEBUG
#include "debug.h"

VOID NtoskrnlIoDeviceInterface();


/* PUBLIC FUNCTIONS ***********************************************************/

VOID NtoskrnlIoMdlTest()
{
    PMDL Mdl;
    PIRP Irp;
    PVOID VirtualAddress;
    ULONG MdlSize = 2*4096+184; // 2 pages and some random value

    StartTest();

    // Try to alloc 2Gb MDL
    Mdl = IoAllocateMdl(NULL, 2048UL*0x100000, FALSE, FALSE, NULL);

    ok(Mdl == NULL,
      "IoAllocateMdl should fail allocation of 2Gb or more, but got Mdl=0x%X",
      (UINT32)Mdl);

    if (Mdl)
        IoFreeMdl(Mdl);

    // Now create a valid MDL
    VirtualAddress = ExAllocatePool(NonPagedPool, MdlSize);
    Mdl = IoAllocateMdl(VirtualAddress, MdlSize, FALSE, FALSE, NULL);
    ok(Mdl != NULL, "Mdl allocation failed");
    // Check fields of the allocated struct
    ok(Mdl->Next == NULL, "Mdl->Next should be NULL, but is 0x%X",
        (UINT32)Mdl->Next);
    ok(Mdl->ByteCount == MdlSize,
        "Mdl->ByteCount should be equal to MdlSize, but is 0x%X",
        (UINT32)Mdl->ByteCount);
    // TODO: Check other fields of MDL struct

    IoFreeMdl(Mdl);
    // Allocate now with pointer to an Irp
    Irp = IoAllocateIrp(1, FALSE);
    ok(Irp != NULL, "IRP allocation failed");
    Mdl = IoAllocateMdl(VirtualAddress, MdlSize, FALSE, FALSE, Irp);
    ok(Mdl != NULL, "Mdl allocation failed");
    ok(Irp->MdlAddress == Mdl, "Irp->MdlAddress should be 0x%X, but is 0x%X",
        (UINT32)Mdl, (UINT32)Irp->MdlAddress);

    IoFreeMdl(Mdl);

    // TODO: Check a case when SecondaryBuffer == TRUE

    IoFreeIrp(Irp);
    ExFreePool(VirtualAddress);

    FinishTest("NTOSKRNL Io Mdl");
}

VOID NtoskrnlIoIrpTest()
{
    USHORT size;
    IRP *iorp;

    StartTest();

    // 1st test
    size = sizeof(IRP) + 5 * sizeof(IO_STACK_LOCATION);
    iorp = ExAllocatePool(NonPagedPool, size);

    if (NULL != iorp)
    {
        IoInitializeIrp(iorp, size, 5);

        ok(6 == iorp->Type, "Irp type should be 6, but got %d\n", iorp->Type);
        ok(iorp->Size == size, "Irp size should be %d, but got %d\n",
            iorp->Size, size);
        ok(5 == iorp->StackCount, "Irp StackCount should be 5, but got %d\n",
            iorp->StackCount);
        ok(6 == iorp->CurrentLocation, "Irp CurrentLocation should be 6, but got %d\n",
            iorp->CurrentLocation);
        ok(IsListEmpty(&iorp->ThreadListEntry), "IRP thread list is not empty\n");
        ok ((PIO_STACK_LOCATION)(iorp + 1) + 5 ==
            iorp->Tail.Overlay.CurrentStackLocation,
            "CurrentStackLocation mismatch\n");

        ExFreePool(iorp);
    }

    // 2nd test
    size = sizeof(IRP) + 2 * sizeof(IO_STACK_LOCATION);
    iorp = IoAllocateIrp(2, FALSE);

    if (NULL != iorp)
    {
        ok(6 == iorp->Type, "Irp type should be 6, but got %d\n", iorp->Type);
        ok(iorp->Size >= size,
            "Irp size should be more or equal to %d, but got %d\n",
            iorp->Size, size);
        ok(2 == iorp->StackCount, "Irp StackCount should be 2, but got %d\n",
            iorp->StackCount);
        ok(3 == iorp->CurrentLocation, "Irp CurrentLocation should be 3, but got %d\n",
            iorp->CurrentLocation);
        ok(IsListEmpty(&iorp->ThreadListEntry), "IRP thread list is not empty\n");
        ok ((PIO_STACK_LOCATION)(iorp + 1) + 2 ==
            iorp->Tail.Overlay.CurrentStackLocation,
            "CurrentStackLocation mismatch\n");
        ok((IRP_ALLOCATED_FIXED_SIZE & iorp->AllocationFlags),
            "IRP Allocation flags lack fixed size attribute\n");
        ok(!(IRP_LOOKASIDE_ALLOCATION & iorp->AllocationFlags),
            "IRP Allocation flags should not have lookaside allocation\n");

        IoFreeIrp(iorp);
    }

    // 3rd test
    size = sizeof(IRP) + 2 * sizeof(IO_STACK_LOCATION);
    iorp = IoAllocateIrp(2, TRUE);

    if (NULL != iorp)
    {
        ok(6 == iorp->Type, "Irp type should be 6, but got %d\n", iorp->Type);
        ok(iorp->Size >= size,
            "Irp size should be more or equal to %d, but got %d\n",
            iorp->Size, size);
        ok(2 == iorp->StackCount, "Irp StackCount should be 2, but got %d\n",
            iorp->StackCount);
        ok(3 == iorp->CurrentLocation, "Irp CurrentLocation should be 3, but got %d\n",
            iorp->CurrentLocation);
        ok(IsListEmpty(&iorp->ThreadListEntry), "IRP thread list is not empty\n");
        ok ((PIO_STACK_LOCATION)(iorp + 1) + 2 ==
            iorp->Tail.Overlay.CurrentStackLocation,
            "CurrentStackLocation mismatch\n");
        ok((IRP_ALLOCATED_FIXED_SIZE & iorp->AllocationFlags),
            "IRP Allocation flags lack fixed size attribute\n");
        ok((IRP_LOOKASIDE_ALLOCATION & iorp->AllocationFlags),
            "IRP Allocation flags lack lookaside allocation\n");

        IoFreeIrp(iorp);
    }

    FinishTest("NTOSKRNL Io Irp");
}

VOID NtoskrnlIoTests()
{
    NtoskrnlIoMdlTest();
    NtoskrnlIoDeviceInterface();
    NtoskrnlIoIrpTest();
}
