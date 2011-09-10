/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Io Regressions KM-Test (Mdl)
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

START_TEST(IoMdl)
{
    PMDL Mdl;
    PIRP Irp;
    PVOID VirtualAddress;
    ULONG MdlSize = 2*4096+184; // 2 pages and some random value

    // Try to alloc 2Gb MDL
    Mdl = IoAllocateMdl(NULL, 2048UL*0x100000, FALSE, FALSE, NULL);

    ok(Mdl == NULL,
      "IoAllocateMdl should fail allocation of 2Gb or more, but got Mdl=0x%X",
      (UINT_PTR)Mdl);

    if (Mdl)
        IoFreeMdl(Mdl);

    // Now create a valid MDL
    VirtualAddress = ExAllocatePool(NonPagedPool, MdlSize);
    Mdl = IoAllocateMdl(VirtualAddress, MdlSize, FALSE, FALSE, NULL);
    ok(Mdl != NULL, "Mdl allocation failed");
    // Check fields of the allocated struct
    ok(Mdl->Next == NULL, "Mdl->Next should be NULL, but is 0x%X",
        (UINT_PTR)Mdl->Next);
    ok(Mdl->ByteCount == MdlSize,
        "Mdl->ByteCount should be equal to MdlSize, but is 0x%X",
        (UINT_PTR)Mdl->ByteCount);
    // TODO: Check other fields of MDL struct

    IoFreeMdl(Mdl);
    // Allocate now with pointer to an Irp
    Irp = IoAllocateIrp(1, FALSE);
    ok(Irp != NULL, "IRP allocation failed");
    Mdl = IoAllocateMdl(VirtualAddress, MdlSize, FALSE, FALSE, Irp);
    ok(Mdl != NULL, "Mdl allocation failed");
    ok(Irp->MdlAddress == Mdl, "Irp->MdlAddress should be 0x%X, but is 0x%X",
        (UINT_PTR)Mdl, (UINT_PTR)Irp->MdlAddress);

    IoFreeMdl(Mdl);

    // TODO: Check a case when SecondaryBuffer == TRUE

    IoFreeIrp(Irp);
    ExFreePool(VirtualAddress);
}
