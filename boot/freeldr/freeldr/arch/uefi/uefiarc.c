/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Build ARC names from UEFI device paths (UEFI -> ARC bridge)
 * AUTHOR:      ChatGPT (based on ReactOS/FreeLdr style and UEFI specs)
 */

#include <uefildr.h>
#include <freeldr.h>
#include <arcname.h>
#include "ntldr/winldr.h" 

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* -------------------------------------------------------------------------- */
/* External symbols from other freeldr units                                  */
/* -------------------------------------------------------------------------- */

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE       GlobalImageHandle;

/* From winldr.c (already defined there) — it is a POINTER */
extern PLOADER_SYSTEM_BLOCK WinLdrSystemBlock;

/* Optional: used elsewhere; leave NULL here unless you detect FS type */
extern PCWSTR BootFileSystem;

/* -------------------------------------------------------------------------- */
/* Local helpers & types                                                      */
/* -------------------------------------------------------------------------- */

/* Fallback for SetDevicePathNodeLength if not pulled in via headers */
#ifndef SetDevicePathNodeLength
#define SetDevicePathNodeLength(Node, Length) do {        \
    (Node)->Length[0] = (UINT8)((Length) & 0xFF);         \
    (Node)->Length[1] = (UINT8)(((Length) >> 8) & 0xFF);  \
} while (0)
#endif

/* Minimal pool helpers using BootServices */
static __inline VOID FreePoolSafe(IN VOID* Ptr)
{
    if (Ptr) GlobalSystemTable->BootServices->FreePool((VOID*)Ptr);
}

static PVOID UefiAllocateZeroPool(IN UINTN Size)
{
    PVOID Buffer = NULL;
    if (!EFI_ERROR(GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData, Size, &Buffer)) && Buffer)
        RtlZeroMemory(Buffer, Size);
    return Buffer;
}

static PVOID UefiAllocateCopyPool(IN UINTN Size, IN CONST VOID* Src)
{
    PVOID Buffer = NULL;
    if (!EFI_ERROR(GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData, Size, &Buffer)) && Buffer && Src)
        RtlCopyMemory(Buffer, Src, Size);
    return Buffer;
}

/* Minimal device-path helpers (don’t depend on library) */
static UINTN UefiGetDevicePathSize(IN EFI_DEVICE_PATH_PROTOCOL* Dp)
{
    UINTN Size = 0;
    EFI_DEVICE_PATH_PROTOCOL* Node = Dp;
    if (!Node) return 0;

    while (!IsDevicePathEnd(Node))
    {
        Size += DevicePathNodeLength(Node);
        Node = NextDevicePathNode(Node);
    }

    /* add end node */
    Size += sizeof(EFI_DEVICE_PATH_PROTOCOL);
    return Size;
}

static VOID UefiSetDevicePathEndNode(OUT EFI_DEVICE_PATH_PROTOCOL* Node)
{
    if (!Node) return;
    Node->Type    = END_DEVICE_PATH_TYPE;
    Node->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
    /* Write length bytes directly to avoid macro conflicts */
    {
        UINT16 len = (UINT16)sizeof(EFI_DEVICE_PATH_PROTOCOL);
        Node->Length[0] = (UINT8)(len & 0xFF);
        Node->Length[1] = (UINT8)((len >> 8) & 0xFF);
    }
}

/* String compare for CHAR16 (avoid external dep) */
static INTN StrCmp16(IN CONST CHAR16* A, IN CONST CHAR16* B)
{
    if (A == B) return 0;
    if (!A) return -1;
    if (!B) return 1;
    while (*A && (*A == *B)) { ++A; ++B; }
    return (INTN)(*A) - (INTN)(*B);
}

typedef struct _UEFI_DISK_REC
{
    EFI_HANDLE DiskHandle;                 /* Physical disk handle (BlockIo, !LogicalPartition) */
    EFI_DEVICE_PATH_PROTOCOL* DiskDp;      /* Its device path */
    UINT32 RDiskIndex;                     /* Assigned rdisk() index */
} UEFI_DISK_REC;

typedef struct _UEFI_PART_REC
{
    EFI_HANDLE PartHandle;                 /* Partition handle (BlockIo, LogicalPartition) */
    EFI_DEVICE_PATH_PROTOCOL* PartDp;      /* Its device path */
    EFI_HANDLE ParentDisk;                 /* Resolved parent physical disk handle */
    UINT32 RDiskIndex;                     /* Inherited from parent */
    UINT32 PartitionIndex;                 /* 1-based partition() index within that disk */
} UEFI_PART_REC;

typedef struct _UEFI_ARC_CTX
{
    UEFI_DISK_REC* Disks;
    UINTN          DiskCount;

    UEFI_PART_REC* Parts;
    UINTN          PartCount;

    /* DevicePathToText disabled in this build; we’ll use pointer ordering */
    VOID*          DpToText;
} UEFI_ARC_CTX;

/* -------------------------------------------------------------------------- */
/* Small utilities                                                            */
/* -------------------------------------------------------------------------- */

static CHAR16* DpToTextMaybe(UEFI_ARC_CTX* Ctx, EFI_DEVICE_PATH_PROTOCOL* Dp)
{
    /* DevicePathToText not available — return NULL */
    UNREFERENCED_PARAMETER(Ctx);
    UNREFERENCED_PARAMETER(Dp);
    return NULL;
}

static INTN StrCmpNullSafe(CHAR16* A, CHAR16* B)
{
    if (A && B) return StrCmp16(A, B);
    if (A && !B) return 1;
    if (!A && B) return -1;
    return 0;
}

/* Compare DevicePaths deterministically (text if available, else pointer) */
static INTN CmpDpStable(UEFI_ARC_CTX* Ctx, EFI_DEVICE_PATH_PROTOCOL* A, EFI_DEVICE_PATH_PROTOCOL* B)
{
    CHAR16* ta = DpToTextMaybe(Ctx, A);
    CHAR16* tb = DpToTextMaybe(Ctx, B);
    INTN r = StrCmpNullSafe(ta, tb);
    FreePoolSafe(ta);
    FreePoolSafe(tb);
    if (r != 0) return r;

    /* Fallback: compare addresses (stable within this boot) */
    if (A < B) return -1;
    if (A > B) return 1;
    return 0;
}

/* Walk a device path and strip partition/media nodes to get the parent disk handle */
static EFI_HANDLE FindParentDiskHandle(EFI_HANDLE PartHandle)
{
    EFI_STATUS Status;
    EFI_DEVICE_PATH_PROTOCOL* Dp = NULL;
    EFI_DEVICE_PATH_PROTOCOL* Work = NULL;
    EFI_HANDLE Handle = NULL;

    Status = GlobalSystemTable->BootServices->HandleProtocol(PartHandle, &gEfiDevicePathProtocolGuid, (VOID**)&Dp);
    if (EFI_ERROR(Status) || !Dp) return NULL;

    /* Copy the DP so we can mutate */
    UINTN Sz = UefiGetDevicePathSize(Dp);
    Work = UefiAllocateCopyPool(Sz, Dp);
    if (!Work) return NULL;

    /* Trim nodes from the end until LocateDevicePath gives a non-logical BlockIo */
    EFI_DEVICE_PATH_PROTOCOL* Node = Work;
    EFI_DEVICE_PATH_PROTOCOL* LastGood = Work;

    while (!IsDevicePathEnd(Node))
    {
        LastGood = Node;
        Node = NextDevicePathNode(Node);
    }

    /* Pop nodes progressively */
    while (LastGood && LastGood != Work)
    {
        /* Terminate here */
        UefiSetDevicePathEndNode(LastGood);

        EFI_DEVICE_PATH_PROTOCOL* Tmp = Work;
        Handle = NULL;
        Status = GlobalSystemTable->BootServices->LocateDevicePath(&gEfiBlockIoProtocolGuid, &Tmp, &Handle);
        if (!EFI_ERROR(Status) && Handle)
        {
            EFI_BLOCK_IO_PROTOCOL* Blk = NULL;
            if (!EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(Handle, &gEfiBlockIoProtocolGuid, (VOID**)&Blk))
                && Blk && !Blk->Media->LogicalPartition)
            {
                /* Found a physical disk handle */
                FreePoolSafe(Work);
                return Handle;
            }
        }

        /* Move LastGood back one node: find previous */
        /* Re-scan from start to the node before LastGood */
        EFI_DEVICE_PATH_PROTOCOL* Prev = NULL;
        EFI_DEVICE_PATH_PROTOCOL* Cur  = Work;
        while (!IsDevicePathEnd(Cur) && Cur != LastGood)
        {
            Prev = Cur;
            Cur = NextDevicePathNode(Cur);
        }
        LastGood = Prev;
    }

    FreePoolSafe(Work);
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* Enumeration and indexing                                                   */
/* -------------------------------------------------------------------------- */

static EFI_STATUS InitArcCtx(UEFI_ARC_CTX* Ctx)
{
    EFI_STATUS Status;
    RtlZeroMemory(Ctx, sizeof(*Ctx));

    /* DevicePathToText not available in this build */
    Ctx->DpToText = NULL;

    /* Get all BlockIo handles */
    EFI_HANDLE* Handles = NULL;
    UINTN Count = 0;
    Status = GlobalSystemTable->BootServices->LocateHandleBuffer(ByProtocol,
                                                                 &gEfiBlockIoProtocolGuid,
                                                                 NULL,
                                                                 &Count,
                                                                 &Handles);
    if (EFI_ERROR(Status) || Count == 0) return EFI_NOT_FOUND;

    /* First pass: classify disks vs partitions */
    Ctx->Disks = (UEFI_DISK_REC*)UefiAllocateZeroPool(sizeof(UEFI_DISK_REC) * Count);
    Ctx->Parts = (UEFI_PART_REC*)UefiAllocateZeroPool(sizeof(UEFI_PART_REC) * Count);
    if (!Ctx->Disks || !Ctx->Parts)
    {
        FreePoolSafe(Ctx->Disks);
        FreePoolSafe(Ctx->Parts);
        FreePoolSafe(Handles);
        return EFI_OUT_OF_RESOURCES;
    }

    for (UINTN i = 0; i < Count; ++i)
    {
        EFI_BLOCK_IO_PROTOCOL* Blk = NULL;
        EFI_DEVICE_PATH_PROTOCOL* Dp = NULL;
        if (EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(Handles[i], &gEfiBlockIoProtocolGuid, (VOID**)&Blk)) || !Blk)
            continue;
        if (EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(Handles[i], &gEfiDevicePathProtocolGuid, (VOID**)&Dp)) || !Dp)
            continue;

        if (!Blk->Media->LogicalPartition)
        {
            UEFI_DISK_REC* d = &Ctx->Disks[Ctx->DiskCount++];
            d->DiskHandle = Handles[i];
            d->DiskDp = Dp;
        }
        else
        {
            UEFI_PART_REC* p = &Ctx->Parts[Ctx->PartCount++];
            p->PartHandle = Handles[i];
            p->PartDp = Dp;
            p->ParentDisk = NULL; /* filled later */
        }
    }

    FreePoolSafe(Handles);

    /* Sort disks deterministically */
    for (UINTN i = 0; i + 1 < Ctx->DiskCount; ++i)
    {
        for (UINTN j = i + 1; j < Ctx->DiskCount; ++j)
        {
            if (CmpDpStable(Ctx, Ctx->Disks[i].DiskDp, Ctx->Disks[j].DiskDp) > 0)
            {
                UEFI_DISK_REC tmp = Ctx->Disks[i];
                Ctx->Disks[i] = Ctx->Disks[j];
                Ctx->Disks[j] = tmp;
            }
        }
        Ctx->Disks[i].RDiskIndex = (UINT32)i;
    }
    if (Ctx->DiskCount)
        Ctx->Disks[Ctx->DiskCount - 1].RDiskIndex = (UINT32)(Ctx->DiskCount - 1);

    /* Map partitions to parent disks and assign partition indices (1-based) */
    for (UINTN i = 0; i < Ctx->PartCount; ++i)
    {
        Ctx->Parts[i].ParentDisk = FindParentDiskHandle(Ctx->Parts[i].PartHandle);
    }

    /* For each disk, collect and sort its partitions, assign partition numbers */
    for (UINTN d = 0; d < Ctx->DiskCount; ++d)
    {
        /* count parts for this disk */
        UINTN localCount = 0;
        for (UINTN p = 0; p < Ctx->PartCount; ++p)
            if (Ctx->Parts[p].ParentDisk == Ctx->Disks[d].DiskHandle)
                ++localCount;

        if (!localCount) continue;

        /* gather indices */
        UINTN* idx = (UINTN*)UefiAllocateZeroPool(sizeof(UINTN) * localCount);
        if (!idx) continue;
        UINTN k = 0;
        for (UINTN p = 0; p < Ctx->PartCount; ++p)
            if (Ctx->Parts[p].ParentDisk == Ctx->Disks[d].DiskHandle)
                idx[k++] = p;

        /* sort by DP (stable) */
        for (UINTN a = 0; a + 1 < localCount; ++a)
        {
            for (UINTN b = a + 1; b < localCount; ++b)
            {
                if (CmpDpStable(Ctx, Ctx->Parts[idx[a]].PartDp, Ctx->Parts[idx[b]].PartDp) > 0)
                {
                    UINTN t = idx[a]; idx[a] = idx[b]; idx[b] = t;
                }
            }
        }

        /* assign partition numbers 1..N and rdisk index */
        for (UINTN n = 0; n < localCount; ++n)
        {
            UEFI_PART_REC* pr = &Ctx->Parts[idx[n]];
            pr->RDiskIndex = Ctx->Disks[d].RDiskIndex;
            pr->PartitionIndex = (UINT32)(n + 1);
        }

        FreePoolSafe(idx);
    }

    return EFI_SUCCESS;
}

static VOID FreeArcCtx(UEFI_ARC_CTX* Ctx)
{
    FreePoolSafe(Ctx->Disks);
    FreePoolSafe(Ctx->Parts);
    RtlZeroMemory(Ctx, sizeof(*Ctx));
}

/* Resolve which (disk,partition) corresponds to the current boot image */
static EFI_STATUS ResolveBootPartition(UEFI_ARC_CTX* Ctx,
                                       UINT32* OutRDisk,
                                       UINT32* OutPartition)
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_DEVICE_PATH_PROTOCOL*  ImageDp = NULL;

    if (!OutRDisk || !OutPartition) return EFI_INVALID_PARAMETER;

    Status = GlobalSystemTable->BootServices->HandleProtocol(GlobalImageHandle,
                                                             &gEfiLoadedImageProtocolGuid,
                                                             (VOID**)&LoadedImage);
    TRACE("UEFI ARC: ResolveBootPartition: HandleProtocol(LoadedImage) -> %lx\n",
              (ULONG_PTR)Status);
    if (EFI_ERROR(Status) || !LoadedImage)
        return EFI_NOT_FOUND;

    /* Prefer the device we were loaded from (not the image file path) */
    Status = GlobalSystemTable->BootServices->HandleProtocol(LoadedImage->DeviceHandle,
                                                             &gEfiDevicePathProtocolGuid,
                                                             (VOID**)&ImageDp);
    if (EFI_ERROR(Status) || !ImageDp)
        return EFI_NOT_FOUND;

    /* Find a matching partition handle: match DevicePath prefix */
    EFI_HANDLE PartHandle = NULL;

    /* Simplest: find BlockIo(LogicalPartition) whose DP is a prefix of ImageDp */
    for (UINTN i = 0; i < Ctx->PartCount; ++i)
    {
        EFI_DEVICE_PATH_PROTOCOL* Copy = (EFI_DEVICE_PATH_PROTOCOL*)UefiAllocateCopyPool(UefiGetDevicePathSize(ImageDp), ImageDp);
        if (!Copy) continue;

        EFI_DEVICE_PATH_PROTOCOL* Tmp = Copy;
        EFI_HANDLE H = NULL;
        if (!EFI_ERROR(GlobalSystemTable->BootServices->LocateDevicePath(&gEfiBlockIoProtocolGuid, &Tmp, &H)))
        {
            if (H == Ctx->Parts[i].PartHandle)
            {
                PartHandle = H;
                FreePoolSafe(Copy);
                break;
            }
        }
        FreePoolSafe(Copy);
    }

    if (!PartHandle)
    {
        /* Fallback: if the image’s DeviceHandle itself is a logical partition */
        EFI_BLOCK_IO_PROTOCOL* Blk = NULL;
        if (!EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(LoadedImage->DeviceHandle,
                                                                       &gEfiBlockIoProtocolGuid,
                                                                       (VOID**)&Blk)) &&
            Blk && Blk->Media->LogicalPartition)
        {
            PartHandle = LoadedImage->DeviceHandle;
        }
    }

    if (!PartHandle) return EFI_NOT_FOUND;

    for (UINTN p = 0; p < Ctx->PartCount; ++p)
    {
        if (Ctx->Parts[p].PartHandle == PartHandle)
        {
            TRACE("UEFI ARC: matched partition rdisk(%u) partition(%u)\n", Ctx->Parts[p].RDiskIndex, Ctx->Parts[p].PartitionIndex);
            *OutRDisk     = Ctx->Parts[p].RDiskIndex;
            *OutPartition = Ctx->Parts[p].PartitionIndex ? Ctx->Parts[p].PartitionIndex : 1;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}

/* -------------------------------------------------------------------------- */
/* Public: Build ARC device & full path                                       */
/* -------------------------------------------------------------------------- */

/* Builds:
 *   ArcBootOut:  "multi(0)disk(0)rdisk(N)partition(M)"
 *   BootPathOut: "multi(0)disk(0)rdisk(N)partition(M)\<SystemPath>"
 *
 * SystemPath may be NULL (then BootPathOut ends with '\').
 */
BOOLEAN
UefiBuildArcBootPaths(OUT CHAR*  BootPathOut,
                      IN  SIZE_T BootPathCapacity,
                      OUT CHAR*  ArcBootOut,
                      IN  SIZE_T ArcBootCapacity,
                      IN  PCSTR  SystemPath /* e.g. "\\reactos\\" or "\\Windows\\" */)
{
    EFI_STATUS Status;
    UEFI_ARC_CTX Ctx;
    UINT32 rdisk = 0, part = 1;

    if (!BootPathOut || !ArcBootOut) return FALSE;

    BootPathOut[0] = '\0';
    ArcBootOut[0]  = '\0';

    Status = InitArcCtx(&Ctx);
    if (EFI_ERROR(Status))
        return FALSE;

    Status = ResolveBootPartition(&Ctx, &rdisk, &part);
    if (EFI_ERROR(Status))
    {
        /* Fallback: first disk, first partition */
        WARN("UEFI ARC: failed to resolve boot partition (Status=%lx) — using rdisk(0) partition(1)\n", (ULONG_PTR)Status);
        rdisk = 0;
        part = 1;
    }

    /* Compose ARC device */
    RtlStringCbPrintfA(ArcBootOut, ArcBootCapacity,
                       "multi(0)disk(0)rdisk(%u)partition(%u)",
                       (unsigned)rdisk, (unsigned)part);

    /* Compose full BootPath */
    if (SystemPath && *SystemPath)
    {
        /* Ensure it starts with '\' and ends with '\' */
        CHAR sysNorm[MAX_PATH];
        RtlStringCbCopyA(sysNorm, sizeof(sysNorm), SystemPath);
        if (sysNorm[0] != '\\' && sysNorm[0] != '/')
            RtlStringCbPrintfA(sysNorm, sizeof(sysNorm), "\\%s", SystemPath);
        SIZE_T L = strlen(sysNorm);
        if (L == 0 || sysNorm[L - 1] != '\\')
            RtlStringCbCatA(sysNorm, sizeof(sysNorm), "\\");

        RtlStringCbPrintfA(BootPathOut, BootPathCapacity, "%s%s", ArcBootOut, sysNorm);
    }
    else
    {
        RtlStringCbPrintfA(BootPathOut, BootPathCapacity, "%s\\", ArcBootOut);
    }

    TRACE("UEFI ARC: ArcBoot=\"%s\"  BootPath=\"%s\"\n", ArcBootOut, BootPathOut);

    /* Stash ArcBootDeviceName for winldr.c consumers */
    if (WinLdrSystemBlock)
    {
        /* Copy only the device part (ArcBootOut) */
        RtlStringCbCopyA(WinLdrSystemBlock->ArcBootDeviceName,
                         sizeof(WinLdrSystemBlock->ArcBootDeviceName),
                         ArcBootOut);
    }

    FreeArcCtx(&Ctx);
    return TRUE;
}
