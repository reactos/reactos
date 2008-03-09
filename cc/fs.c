/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/fs.c
 * PURPOSE:         Implements cache managers functions useful for File Systems
 *
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


#ifndef ROUND_DOWN
#define ROUND_DOWN(X,Y)    ((X) & ~((Y) - 1))
#endif

/* GLOBALS   *****************************************************************/

extern PCACHE_VIEW CcCacheViewArray;
extern ULONG CcCacheViewArrayCount;
extern FAST_MUTEX CcCacheViewLock;

NTSTATUS STDCALL MmUnmapViewInSystemCache (PCACHE_VIEW);

NTSTATUS STDCALL MmChangeSectionSize (PSECTION_OBJECT Section, PLARGE_INTEGER NewMaxSize);

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
LARGE_INTEGER STDCALL
CcGetDirtyPages (IN PVOID LogHandle, 
                 IN PDIRTY_PAGE_ROUTINE DirtyPageRoutine, 
                 IN PVOID Context1, 
                 IN PVOID Context2)
{
    LARGE_INTEGER i;
    UNIMPLEMENTED;
    i.QuadPart = 0;
    return i;
}

/*
 * @implemented
 */
PFILE_OBJECT STDCALL
CcGetFileObjectFromBcb (IN PVOID Bcb)
{
    PINTERNAL_BCB iBcb = (PINTERNAL_BCB) Bcb;
    return iBcb->Bcb->FileObject;
}

/*
 * @unimplemented
 */
LARGE_INTEGER STDCALL
CcGetLsnForFileObject (IN PFILE_OBJECT FileObject, 
                       OUT PLARGE_INTEGER OldestLsn OPTIONAL)
{
    LARGE_INTEGER i;
    UNIMPLEMENTED;
    i.QuadPart = 0;
    return i;
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL
CcIsThereDirtyData (IN PVPB Vpb)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL
CcPurgeCacheSection (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                     IN PLARGE_INTEGER FileOffset OPTIONAL, 
                     IN ULONG Length, 
                     IN BOOLEAN UninitializeCacheMaps)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @implemented
 */
VOID STDCALL
CcSetFileSizes (IN PFILE_OBJECT FileObject, 
                IN PCC_FILE_SIZES FileSizes)
{
    PBCB Bcb;
    NTSTATUS Status;
    ULONG i;

    DPRINT ("CcSetFileSizes(FileObject 0x%p, FileSizes 0x%p)\n", FileObject, FileSizes);
    DPRINT ("AllocationSize %d, FileSize %d, ValidDataLength %d\n",
            (ULONG) FileSizes->AllocationSize.QuadPart,
            (ULONG) FileSizes->FileSize.QuadPart, (ULONG) FileSizes->ValidDataLength.QuadPart);
    DPRINT ("%wZ\n", &FileObject->FileName);

    Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
    if (Bcb == NULL)
    {
        return;
    }
    DPRINT ("AllocationSize %d, FileSize %d, ValidDataLength %d\n",
            (ULONG) Bcb->FileSizes.AllocationSize.QuadPart,
            (ULONG) Bcb->FileSizes.FileSize.QuadPart, (ULONG) Bcb->FileSizes.ValidDataLength.QuadPart);
    ExAcquireFastMutex (&CcCacheViewLock);

    DPRINT ("%d\n", Bcb->FileSizes.FileSize.u.LowPart);

    for (i = ROUND_DOWN (FileSizes->AllocationSize.QuadPart, CACHE_VIEW_SIZE) / CACHE_VIEW_SIZE;
         i < ROUND_UP (Bcb->FileSizes.AllocationSize.QuadPart, CACHE_VIEW_SIZE) / CACHE_VIEW_SIZE; i++)
    {
        if (Bcb->CacheView[i] != NULL)
        {
            if (Bcb->CacheView[i]->Bcb != Bcb)
            {
                KEBUGCHECK (0);
            }
            if (Bcb->CacheView[i]->RefCount > 0)
            {
                KEBUGCHECK (0);
            }
            Status = MmUnmapViewInSystemCache (Bcb->CacheView[i]);
            if (!NT_SUCCESS (Status))
            {
                KEBUGCHECK (0);
            }
            Bcb->CacheView[i]->RefCount = 0;
            Bcb->CacheView[i]->Bcb = NULL;
            Bcb->CacheView[i] = NULL;
        }
    }

#if 0
    for (i = 0; i < CcCacheViewArrayCount; i++)
    {
        if (CcCacheViewArray[i].Bcb == Bcb)
        {
            if (PAGE_ROUND_UP (FileSizes->AllocationSize.QuadPart) <= CcCacheViewArray[i].SectionData.ViewOffset ||
                (PAGE_ROUND_UP (FileSizes->AllocationSize.QuadPart) > CcCacheViewArray[i].SectionData.ViewOffset &&
                 PAGE_ROUND_UP (FileSizes->AllocationSize.QuadPart) <=
                 CcCacheViewArray[i].SectionData.ViewOffset + CACHE_VIEW_SIZE))

            {
                if (CcCacheViewArray[i].RefCount > 0)
                {
                    KEBUGCHECK (0);
                }
                Status = MmUnmapViewInSystemCache (&CcCacheViewArray[i]);
                if (!NT_SUCCESS (Status))
                {
                    KEBUGCHECK (0);
                }
                CcCacheViewArray[i].RefCount = 0;
                CcCacheViewArray[i].Bcb = NULL;
            }
        }
    }
#endif
    Status = MmChangeSectionSize ((PSECTION_OBJECT)Bcb->Section, &FileSizes->FileSize); 
    Bcb->FileSizes = *FileSizes;

    ExReleaseFastMutex (&CcCacheViewLock);
}

/*
 * @unimplemented
 */
VOID STDCALL
CcSetLogHandleForFile (IN PFILE_OBJECT FileObject, 
                       IN PVOID LogHandle, 
                       IN PFLUSH_TO_LSN FlushToLsnRoutine)
{
    UNIMPLEMENTED;
}
