/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KSE 'IgnoreFreePoolTag' shim implementation
 * COPYRIGHT:   Copyright 2026 Mark Jansen <mark.jansen@reactos.org>
 */

/*
 * Some drivers free memory with a different tag than it was allocated with.
 * Windows only validates the tag when freeing protected pool, while checked
 * builds of ReactOS validate it for every allocation and bugcheck with
 * BAD_POOL_CALLER (0xC2, 0x0A).
 *
 * Example: the VMware SVGA 3D driver (vmx_svga.sys) frees 'vmGB' memory with
 * the 'vmKM' tag when an OpenGL application exits (CORE-14623).
 */

#include <ntoskrnl.h>
#include <initguid.h>

#define NDEBUG
#include <debug.h>

DEFINE_GUID(KseIgnoreFreePoolTagGuid, 0x8d0ed225, 0x0892, 0x4efc, 0x9e, 0x72, 0x50, 0x89, 0x12, 0x42, 0x74, 0x53);

extern KSE_SHIM KseIgnoreFreePoolTagShim;

static VOID NTAPI
ShimExFreePoolWithTag(
    _In_ PVOID P,
    _In_ ULONG Tag)
{
    VOID (NTAPI *OriginalFunction)(PVOID, ULONG);

    OriginalFunction = KseIgnoreFreePoolTagShim.HookCollectionsArray->HookArray[0].OriginalFunction;

    /* Keep validating explicit frees of protected pool, like Windows does */
    if (!(Tag & PROTECTED_POOL))
        Tag = 0;

    OriginalFunction(P, Tag);
}

static KSE_HOOK KseIgnoreFreePoolTagHooksNT[] = {
    { KseHookFunction, { "ExFreePoolWithTag" }, (PVOID)(ULONG_PTR)ShimExFreePoolWithTag },
    { KseHookInvalid }
};

static KSE_HOOK_COLLECTION KseIgnoreFreePoolTagCollections[] = {
    { KseCollectionNtExport, NULL, KseIgnoreFreePoolTagHooksNT },
    { KseCollectionInvalid },
};

KSE_SHIM KseIgnoreFreePoolTagShim = { sizeof(KSE_SHIM), &KseIgnoreFreePoolTagGuid, L"IgnoreFreePoolTag", NULL, NULL, NULL, KseIgnoreFreePoolTagCollections };

CODE_SEG("INIT")
NTSTATUS
NTAPI
KseIgnoreFreePoolTagInitialize(VOID)
{
    NTSTATUS Status;

    Status = KseRegisterShim(&KseIgnoreFreePoolTagShim, NULL, 0);
    if (!NT_SUCCESS(Status))
        DPRINT1("KSE: Unable to register IgnoreFreePoolTag shim (0x%lx)\n", Status);

    return Status;
}
