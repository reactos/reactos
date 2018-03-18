/*
 * PROJECT:         ReactOS Kernel - Vista+ APIs
 * LICENSE:         GPL v2 - See COPYING in the top level directory
 * FILE:            lib/drivers/ntoskrnl_vista/ke.c
 * PURPOSE:         Ke functions of Vista+
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <ntdef.h>
#include <ntifs.h>

NTKERNELAPI
ULONG
NTAPI
KeQueryActiveProcessorCount(OUT PKAFFINITY ActiveProcessors OPTIONAL)
{
    RTL_BITMAP Bitmap;
    KAFFINITY ActiveMap = KeQueryActiveProcessors();

    if (ActiveProcessors != NULL)
    {
        *ActiveProcessors = ActiveMap;
    }

    RtlInitializeBitMap(&Bitmap, (PULONG)&ActiveMap,  sizeof(ActiveMap) * 8);
    return RtlNumberOfSetBits(&Bitmap);
}
