/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fullfat.c
 * PURPOSE:         FullFAT integration routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* GLOBALS ******************************************************************/

#define TAG_FULLFAT 'FLUF'

/* FUNCTIONS ****************************************************************/

VOID *
FF_Malloc(FF_T_UINT32 allocSize)
{
    return ExAllocatePoolWithTag(PagedPool, allocSize, TAG_FULLFAT);
}

VOID
FF_Free(VOID *pBuffer)
{
    return ExFreePoolWithTag(pBuffer, TAG_FULLFAT);
}

FF_T_SINT32
FatWriteBlocks(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam)
{
    DPRINT1("FatWriteBlocks %p %d %d %p\n", pBuffer, SectorAddress, Count, pParam);

    return 0;
}

FF_T_SINT32
FatReadBlocks(FF_T_UINT8 *DestBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam)
{
    LARGE_INTEGER Offset;
    PVOID Buffer;
    PVCB Vcb = (PVCB)pParam;
    PBCB Bcb;
    ULONG SectorSize = 512; // FIXME: hardcoding 512 is bad

    DPRINT1("FatReadBlocks %p %d %d %p\n", DestBuffer, SectorAddress, Count, pParam);

    /* Calculate the offset */
    Offset.QuadPart = Int32x32To64(SectorAddress, SectorSize);

    if (!CcMapData(Vcb->StreamFileObject,
                  &Offset,
                  Count * SectorSize,
                  TRUE,
                  &Bcb,
                  &Buffer))
    {
        ASSERT(FALSE);
        /* Mapping failed */
        return 0;
    }

    /* Copy data to the buffer */
    RtlCopyMemory(DestBuffer, Buffer, Count * SectorSize);

    /* Unpin unneeded data */
    CcUnpinData(Bcb);

    /* Return amount of read data in sectors */
    return Count;
}

/* EOF */
