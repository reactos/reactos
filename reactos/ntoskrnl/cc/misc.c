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

/**********************************************************************
 * NAME							INTERNAL
 * 	CcMdlReadCompleteDev@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	MdlChain
 *	DeviceObject
 *	
 * RETURN VALUE
 * 	None.
 *
 * NOTE
 * 	Used by CcMdlReadComplete@8 and FsRtl
 */
VOID STDCALL
CcMdlReadCompleteDev (IN	PMDL		MdlChain,
		      IN	PDEVICE_OBJECT	DeviceObject)
{
  UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	CcMdlReadComplete@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 	None.
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v13.
 */
VOID STDCALL
CcMdlReadComplete (IN	PFILE_OBJECT	FileObject,
		   IN	PMDL		MdlChain)
{
   PDEVICE_OBJECT	DeviceObject = NULL;
   
   DeviceObject = IoGetRelatedDeviceObject (FileObject);
   /* FIXME: try fast I/O first */
   CcMdlReadCompleteDev (MdlChain,
			 DeviceObject);
}

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
    current_entry = Bcb->BcbSegmentListHead.Flink;
    while (current_entry != &Bcb->BcbSegmentListHead)
    {
      current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
      current_entry = current_entry->Flink;
      if (current->FileOffset > FileSizes->AllocationSize.QuadPart)
      {
        CcRosFreeCacheSegment(Bcb, current);
      }
    }
  }
  Bcb->AllocationSize = FileSizes->AllocationSize;
  Bcb->FileSize = FileSizes->FileSize;
  KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
}

