/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/pool.h>
#include <internal/io.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

VOID STDCALL
CcSetFileSizes (
   IN PFILE_OBJECT FileObject,
   IN PCC_FILE_SIZES FileSizes)
{
  KIRQL oldirql;
  PBCB Bcb;
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;

  DPRINT("CcSetFileSizes(FileObject %x, FileSizes %x)\n", FileObject, FileSizes);
  DPRINT("AllocationSize %d, FileSize %d, ValidDataLength %d\n",
         (ULONG)FileSizes->AllocationSize.QuadPart,
         (ULONG)FileSizes->FileSize.QuadPart,
         (ULONG)FileSizes->ValidDataLength.QuadPart);

  Bcb = ((REACTOS_COMMON_FCB_HEADER*)FileObject->FsContext)->Bcb;

  KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);

  if (FileSizes->AllocationSize.QuadPart < Bcb->AllocationSize.QuadPart)
  {
    current_entry = Bcb->CacheSegmentListHead.Flink;
    while (current_entry != &Bcb->CacheSegmentListHead)
    {
      current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbListEntry);
      current_entry = current_entry->Flink;
      if (current->FileOffset > FileSizes->AllocationSize.QuadPart)
      {
        current_entry = current_entry->Flink;
        CcRosFreeCacheSegment(Bcb, current);
      }
    }
  }
  Bcb->AllocationSize = FileSizes->AllocationSize;
  Bcb->FileSize = FileSizes->FileSize;
  KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
}

