/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/block.c
 * PURPOSE:         Cache manager
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

static ULONG CbHash(PDCCB Dccb, ULONG BlockNr)
{ 
   DPRINT("CbHash(Dccb %x, BlockNr %d\n",Dccb,BlockNr);
   DPRINT("CbHash() = %d\n",BlockNr % Dccb->HashTblSize);
   return(BlockNr % Dccb->HashTblSize);
}

#if 0
static VOID CbDereferenceCcb(PDCCB Dccb, PCCB Ccb)
{
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&Dccb->HashTblLock,&oldlvl);
   Ccb->References--;
   KeReleaseSpinLock(&Dccb->HashTblLock,oldlvl);
}
#endif

static PCCB CbGetCcbFromHashTable(PDCCB Dccb, ULONG BlockNr)
{
   KIRQL oldlvl;
   PCCB Ccb;
   
   KeAcquireSpinLock(&Dccb->HashTblLock,&oldlvl);
   Ccb = Dccb->HashTbl[CbHash(Dccb,BlockNr)];
   if (Ccb!=NULL)
     {
	Ccb->References++;
     }
   KeReleaseSpinLock(&Dccb->HashTblLock,oldlvl);
   return(Ccb);
}

#if 0
static BOOLEAN CbRemoveCcbFromHashTable(PDCCB Dccb, PCCB Ccb)
{
   KIRQL oldlvl;
   BOOLEAN Status;
   
   KeAcquireSpinLock(&Dccb->HashTblLock,&oldlvl);
   if (Ccb->References == 0)
     {
	Dccb->HashTbl[CbHash(Dccb,Ccb->BlockNr)]=NULL;
	Status = TRUE;
     }
   else
     {
	Status = FALSE;
     }
   KeReleaseSpinLock(&Dccb->HashTblLock,oldlvl);
   return(Status);
}
#endif

static BOOLEAN CbLockForWrite(PCCB Ccb)
{
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&Ccb->Lock,&oldlvl);
   if (Ccb->ActiveWriter || Ccb->ActiveReaders > 0)
     {
	KeReleaseSpinLock(&Ccb->Lock,oldlvl);
	return(FALSE);
     }
   Ccb->ActiveWriter = TRUE;
   KeReleaseSpinLock(&Ccb->Lock,oldlvl);
   return(TRUE);
   
}

static BOOLEAN CbLockForRead(PCCB Ccb)
{
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&Ccb->Lock,&oldlvl);
   if (Ccb->ActiveWriter)
     {
	KeReleaseSpinLock(&Ccb->Lock,oldlvl);
	return(FALSE);
     }
   Ccb->ActiveReaders++;
   Ccb->References++;
   KeReleaseSpinLock(&Ccb->Lock,oldlvl);
   return(TRUE);
}

static BOOLEAN CbLockForDelete(PDCCB Dccb, PCCB Ccb)
/*
 * FUNCTION: Attempts to lock a ccb for delete
 * ARGUMENTS:
 *          Ccb = CCB to lock
 * RETURNS: True if the lock succeeded 
 *          False otherwise
 * NOTE: This routine always defers to anyone waiting for a another type of
 * lock
 */
{
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&Ccb->Lock,&oldlvl);
   if (Ccb->ActiveWriter || Ccb->WriteInProgress || Ccb->ActiveReaders
       || Ccb->References > 0)
     {
	KeReleaseSpinLock(&Ccb->Lock,oldlvl);
	return(FALSE);
     }
   Ccb->ActiveWriter = TRUE;
   KeReleaseSpinLock(&Ccb->Lock,oldlvl);
   return(TRUE);
}

#if 0
static VOID CbUnlockForDelete(PDCCB Dccb, PCCB Ccb)
{
   Ccb->ActiveWriter = FALSE;
   KeSetEvent(&Ccb->FinishedNotify,IO_NO_INCREMENT,FALSE);
}
#endif

VOID CbAcquireForDelete(PDCCB Dccb, PCCB Ccb)
/*
 * FUNCTION: Acquire the ccb for deletion
 * ARGUMENTS:
 *           Dccb = DCCB for the target device
 *           Ccb = Ccb to acquire 
 */
{
   while (!CbLockForDelete(Dccb,Ccb))
     {
	KeWaitForSingleObject(&Ccb->FinishedNotify,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
     }
}

static NTSTATUS CbReadBlock(PDCCB Dccb, PCCB Ccb)
{
   PIRP Irp;
   LARGE_INTEGER Offset;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   KEVENT Event;
   
   DPRINT("CbReadBlock(Dccb %x, Ccb %x)\n",Dccb,Ccb);
   
   if (Ccb->Buffer==NULL)
     {
	Ccb->Buffer=ExAllocatePool(NonPagedPool,Dccb->SectorSize);
     }
   
   SET_LARGE_INTEGER_HIGH_PART(Offset, 0);
   SET_LARGE_INTEGER_LOW_PART(Offset, Ccb->BlockNr * Dccb->SectorSize);
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				      Dccb->DeviceObject,
				      Ccb->Buffer,
				      Dccb->SectorSize,
				      &Offset,
				      &Event,
				      &IoStatusBlock);
   Status = IoCallDriver(Dccb->DeviceObject, Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoStatusBlock.Status;
     }
   Ccb->Modified=FALSE;
   Ccb->State = CCB_CACHED;
   return(Status);
}

static NTSTATUS CbWriteBlock(PDCCB Dccb, PCCB Ccb)
/*
 * FUNCTION: Writes the block mapped by a CCB back to disk
 * ARGUMENTS:
 *        Dccb = DCCB for the device the CCB maps
 *        Ccb = CCB to write 
 * RETURNS: Status
 * NOTE: It is the caller responsibility to acquire the appropiate locks
 */
{
   PIRP Irp;
   LARGE_INTEGER Offset;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;
   KEVENT Event;

   SET_LARGE_INTEGER_HIGH_PART(Offset, 0);
   SET_LARGE_INTEGER_LOW_PART(Offset, Ccb->BlockNr * Dccb->SectorSize);
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				      Dccb->DeviceObject,
				      Ccb->Buffer,
				      Dccb->SectorSize,
				      &Offset,
				      &Event,
				      &IoStatusBlock);
   Status = IoCallDriver(Dccb->DeviceObject, Irp);
   if (Status == STATUS_PENDING)
     {
	KeWaitForSingleObject(&Event,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	Status = IoStatusBlock.Status;
     }
   return(Status);
}

PCCB CbFindModifiedCcb(PDCCB Dccb, PCCB Start)
{
  UNIMPLEMENTED; 
}

#if 0
static VOID CbDeleteAllCcbs(PDCCB Dccb)
/*
 * FUNCTION: Delete all the ccbs associated with a dccb
 * ARGUMENTS:
 *          Dccb = DCCB to delete
 * NOTE: There must be no other cache activity during this call
 */
{
   PCCB Ccb;
   
   ExFreePool(Dccb->HashTbl);
   
   while (Dccb->CcbListHead.Flink != &Dccb->CcbListHead)
     {
	Ccb = CONTAINING_RECORD(Dccb->CcbListHead.Flink,CCB,Entry);
	if (Ccb->Modified)
	  {
	     CbWriteBlock(Dccb,Ccb);
	  }
	if (Ccb->Buffer != NULL)
	  {
	     ExFreePool(Ccb->Buffer);
	  }
	Dccb->CcbListHead.Flink = Dccb->CcbListHead.Flink->Flink;
	ExFreePool(Ccb);
     }
}
#endif

#if 0
static VOID CbFreeCcb(PDCCB Dccb, PCCB Ccb)
{
   KIRQL oldlvl;
   
   if (Ccb->Buffer != NULL && Ccb->Modified)
     {
	CbWriteBlock(Dccb,Ccb);
     }
   if (Ccb->Buffer != NULL)
     {
	ExFreePool(Ccb->Buffer);
     }
   KeAcquireSpinLock(&Dccb->CcbListLock,&oldlvl);
   RemoveEntryList(&Ccb->Entry);
   KeReleaseSpinLock(&Dccb->CcbListLock,oldlvl);
   ExFreePool(Ccb);
}
#endif

#if 0
static VOID CbReclaimMemory(PDCCB Dccb)
{
   PCCB RedundantCcbs[25];
   ULONG NrRedundantCcbs;
   PCCB Ccb;
   PLIST_ENTRY CurrentEntry;
   KIRQL oldlvl;
   ULONG i;
   
   NrRedundantCcbs=0;
   
   KeAcquireSpinLock(&Dccb->CcbListLock,&oldlvl);
   CurrentEntry = Dccb->CcbListHead.Flink;
   while (CurrentEntry != &Dccb->CcbListHead &&
	  NrRedundantCcbs < 25)
     {
	Ccb = CONTAINING_RECORD(CurrentEntry,CCB,Entry);
	if (CbLockForDelete(Dccb,Ccb))
	  {
	     if (CbRemoveCcbFromHashTable(Dccb,Ccb))
	       {
		  if (Ccb->Modified)
		    {
		       CbWriteBlock(Dccb,Ccb);
		    }
		  Ccb->State = CCB_DELETE_PENDING;
		  RedundantCcbs[NrRedundantCcbs]=Ccb;
		  NrRedundantCcbs++;	
	       }
	     else
	       {
		  CbUnlockForDelete(Dccb,Ccb);
	       }
	  }
	CurrentEntry = CurrentEntry->Flink;
     }
   KeReleaseSpinLock(&Dccb->CcbListLock,oldlvl);
   
   for (i=0;i<NrRedundantCcbs;i++)
     {
	CbFreeCcb(Dccb,RedundantCcbs[i]);
     }
}
#endif

#if 0
static VOID CbDeleteCcb(PDCCB Dccb, PCCB Ccb)
/*
 * FUNCTION: Deletes a CCB
 * ARGUMENTS:
 *       Dccb = DCCB for the target device
 *       Ccb = CCB to delete
 * NOTE: This function is implemented so that other ccb requests override 
 * a deletion. Filesystems that delete CCBs during normal operation should be
 * aware of the possibility of starvation. 
 */
{   
   CbAcquireForDelete(Dccb,Ccb);   
   Ccb->State = CCB_DELETE_PENDING;
}
#endif

VOID CbReinitCcb(PDCCB Dccb, PCCB Ccb, ULONG BlockNr)
{
   DPRINT("CbReinitCcb(Dccb %x, Ccb %x, BlockNr %d\n",Dccb,Ccb,BlockNr);
   
   Ccb->BlockNr = BlockNr;
   Ccb->State = CCB_NOT_CACHED;
   Ccb->Buffer = NULL;
   Ccb->ActiveReaders = 0;
   Ccb->ActiveWriter = FALSE;
   KeInitializeEvent(&Ccb->FinishedNotify,SynchronizationEvent,FALSE);
   Ccb->Modified=FALSE;
}

VOID CbInitCcb(PDCCB Dccb, PCCB Ccb, ULONG BlockNr)
/*
 * FUNCTION: Initializes a ccb and adds it to the dccb hash table
 * ARGUMENTS:
 *          Dccb = Device cache to initialize the ccb for
 *          Ccb = Caller allocated CCB
 *          BlockNr = Block that the ccb will map
 */
{
   DPRINT("CbInitCcb(Dccb %x, Ccb %x, BlockNr %d)\n",Dccb,Ccb,BlockNr);
   
   CbReinitCcb(Dccb,Ccb,BlockNr);
   Dccb->HashTbl[CbHash(Dccb,BlockNr)] = Ccb;
   ExInterlockedInsertTailList(&Dccb->CcbListHead,&Ccb->Entry,
			       &Dccb->CcbListLock);
}

VOID CbInitDccb(PDCCB Dccb, PDEVICE_OBJECT DeviceObject, ULONG SectorSize,
		ULONG NrSectors, ULONG PercentageToCache)
/*
 * FUNCTION: Initializes a dccb for a device
 * ARGUMENTS:
 *          Dccb = Caller allocated DCCB
 *          DeviceObject = Device associated with the DCCB
 *          SectorSize = Sector size of the device
 *          NrSectors = Number of sectors on the device
 *          PercentageToCache = Maximum percentage of the device that will
 *                              be in the cache at any one time
 * NOTE: The PercentageToCache parameter only effects the size of the DCCB
 * hash table not the amount of physical memory used to cache actual sectors
 */
{
   DPRINT("CbInitDccb(Dccb %x, DeviceObject %x, SectorSize %d, "
	   "NrSectors %d, PercentageToCache %d)\n",Dccb,DeviceObject,
	   SectorSize,NrSectors,PercentageToCache);
   
   Dccb->HashTblSize = (NrSectors / 100) * PercentageToCache;
   Dccb->HashTbl = ExAllocatePool(NonPagedPool,sizeof(PCCB)*
				  Dccb->HashTblSize);
   Dccb->DeviceObject = DeviceObject;
   Dccb->SectorSize = SectorSize;
   Dccb->NrCcbs = 0;
   Dccb->NrModifiedCcbs = 0;
   InitializeListHead(&Dccb->CcbListHead);
   KeInitializeSpinLock(&Dccb->CcbListLock);
}


VOID CbReleaseFromRead(PDCCB Dccb, PCCB Ccb)
/*
 * FUNCTION: Releases a CCB that the caller had acquired for reading
 * ARGUMENTS:
 *         Dccb = DCCB for the device the CCB maps a block on
 *         Ccb = CCB to release
 */
{
   DPRINT("CbReleaseFromRead(Dccb %x, Ccb %x)\n",Dccb,Ccb);
   
   Ccb->ActiveReaders--;
   if (Ccb->ActiveReaders==0)
     {
	KeSetEvent(&Ccb->FinishedNotify,IO_NO_INCREMENT,FALSE);
     }
}

VOID CbReleaseFromWrite(PDCCB Dccb, PCCB Ccb, BOOLEAN Modified, 
			BOOLEAN WriteThrough)
/*
 * FUNCTION: Release a ccb acquired for writing
 * ARGUMENTS: 
 *        Dccb = DCCB for the CCB
 *        Ccb = CCB to release
 *        Modified = True if the caller modified the block while it was locked
 *        WriteThrough = True if the block should be written back to disk
 */
{
   DPRINT("CbReleaseFromWrite(Dccb %x, Ccb %x, Modified %d, "
	  "WriteThrough %d)\n",Dccb,Ccb,Modified,WriteThrough);
   
   if (Modified)
     {
	Ccb->Modified=TRUE;
     }
   if (WriteThrough)
     {
	CbWriteBlock(Dccb,Ccb);
     }
   
   Ccb->ActiveWriter=FALSE;
   KeSetEvent(&Ccb->FinishedNotify,IO_NO_INCREMENT,FALSE);
}

static PCCB CbCheckCcb(PDCCB Dccb, ULONG BlockNr)
/*
 * FUNCTION: Return a CCB that maps a block
 * ARGUMENTS:
 *        Dccb = DCCB for the device the block is on
 *        BlockNr = Block to map
 * RETURNS: The CCB 
 * NOTE: The caller must lock the returned ccb
 */
{
   KIRQL oldlvl;
   PCCB Ccb;
   
   DPRINT("CbCheckCcb(Dccb %x, BlockNr %d)\n",Dccb,BlockNr);
   
   KeAcquireSpinLock(&Dccb->CcbListLock,&oldlvl);
   Ccb = CbGetCcbFromHashTable(Dccb,BlockNr);
   if (Ccb == NULL)
     {
	Ccb = ExAllocatePool(NonPagedPool,sizeof(CCB));
	CbInitCcb(Dccb,Ccb,BlockNr);
     }   
   KeReleaseSpinLock(&Dccb->CcbListLock,oldlvl);
   if (Ccb->BlockNr != BlockNr)
     {
	CbAcquireForDelete(Dccb,Ccb);
	if (Ccb->Modified)
	  {
	     CbWriteBlock(Dccb,Ccb);
	  }
	CbReinitCcb(Dccb,Ccb,BlockNr);
     }
   return(Ccb);
}

PCCB CbAcquireForWrite(PDCCB Dccb, ULONG BlockNr)
{
   PCCB Ccb;
   
   DPRINT("CbAcquireForWrite(Dccb %x, BlockNr %d)\n",Dccb,BlockNr);
   
   Ccb = CbCheckCcb(Dccb,BlockNr);
   while (!CbLockForWrite(Ccb))
     {
	KeWaitForSingleObject(&Ccb->FinishedNotify,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
     }
   if (Ccb->State != CCB_CACHED)
     {
	CbReadBlock(Dccb,Ccb);
     }
   return(Ccb);
}

PCCB CbAcquireForRead(PDCCB Dccb, ULONG BlockNr)
/*
 * FUNCTION: Acquires a ccb for reading 
 * ARGUMENTS:
 *         Dccb = DCCB for the device 
 *         BlockNr = Block the CCB should map
 * RETURNS: The acquired CCB
 */
{
   PCCB Ccb;
   
   DPRINT("CbAcquireForRead(Dccb %x, BlockNr %d)\n",Dccb,BlockNr);
   
   Ccb = CbCheckCcb(Dccb,BlockNr);
   while (!CbLockForRead(Ccb))
     {
	KeWaitForSingleObject(&Ccb->FinishedNotify,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
     }
   
   if (Ccb->State != CCB_CACHED)
     {
	CbReadBlock(Dccb,Ccb);
     }
   return(Ccb);
}

