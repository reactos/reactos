/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ob/sdcache.c
 * PURPOSE:         No purpose listed.
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* TYPES ********************************************************************/

typedef struct _SD_CACHE_ENTRY
{
  LIST_ENTRY ListEntry;
  ULONG HashValue;
  ULONG Index;
  ULONG RefCount;
} SD_CACHE_ENTRY, *PSD_CACHE_ENTRY;


/* GLOBALS ******************************************************************/

PLIST_ENTRY ObpSdCache;
KSPIN_LOCK ObpSdCacheSpinLock;
KIRQL ObpSdCacheIrql;


#define SD_CACHE_ENTRIES 0x100

/* FUNCTIONS ****************************************************************/

NTSTATUS
ObpInitSdCache(VOID)
{
  ULONG i;

  ObpSdCache = ExAllocatePool(NonPagedPool,
			      SD_CACHE_ENTRIES * sizeof(LIST_ENTRY));
  if (ObpSdCache == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  for (i = 0; i < SD_CACHE_ENTRIES; i++)
    {
      InitializeListHead(&ObpSdCache[i]);
    }

  KeInitializeSpinLock(&ObpSdCacheSpinLock);

  return STATUS_SUCCESS;
}


static VOID
ObpSdCacheLock(VOID)
{
  KeAcquireSpinLock(&ObpSdCacheSpinLock,
		    &ObpSdCacheIrql);
}


static VOID
ObpSdCacheUnlock(VOID)
{
  KeReleaseSpinLock(&ObpSdCacheSpinLock,
		    ObpSdCacheIrql);
}


static ULONG
ObpHash(PVOID Buffer,
	ULONG Length)
{
  PUCHAR Ptr;
  ULONG Value;
  ULONG i;

  Ptr = (PUCHAR)Buffer;
  Value = 0;
  for (i = 0; i < Length; i++)
    {
      Value += *Ptr;
      Ptr++;
    }

  return Value;
}


static ULONG
ObpHashSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  ULONG Value;
  BOOLEAN Defaulted;
  BOOLEAN DaclPresent;
  BOOLEAN SaclPresent;
  PSID Owner = NULL;
  PSID Group = NULL;
  PACL Dacl = NULL;
  PACL Sacl = NULL;

  RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
				&Owner,
				&Defaulted);

  RtlGetGroupSecurityDescriptor(SecurityDescriptor,
				&Group,
				&Defaulted);

  RtlGetDaclSecurityDescriptor(SecurityDescriptor,
			       &DaclPresent,
			       &Dacl,
			       &Defaulted);

  RtlGetSaclSecurityDescriptor(SecurityDescriptor,
			       &SaclPresent,
			       &Sacl,
			       &Defaulted);

  Value = 0;
  if (Owner != NULL)
    {
      Value += ObpHash(Owner, RtlLengthSid(Owner));
    }

  if (Group != NULL)
    {
      Value += ObpHash(Group, RtlLengthSid(Group));
    }

  if (DaclPresent == TRUE && Dacl != NULL)
    {
      Value += ObpHash(Dacl, Dacl->AclSize);
    }

  if (SaclPresent == TRUE && Sacl != NULL)
    {
      Value += ObpHash(Sacl, Sacl->AclSize);
    }

  return Value;
}


static PSD_CACHE_ENTRY
ObpCreateCacheEntry(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		    IN ULONG HashValue,
		    IN ULONG Index,
		    OUT PSECURITY_DESCRIPTOR *NewSD)
{
  PSECURITY_DESCRIPTOR Sd;
  PSD_CACHE_ENTRY CacheEntry;
  ULONG Length;

  DPRINT("ObpCreateCacheEntry() called\n");

  Length = RtlLengthSecurityDescriptor(SecurityDescriptor);

  CacheEntry = ExAllocatePool(NonPagedPool,
			      sizeof(SD_CACHE_ENTRY) + Length);
  if (CacheEntry == NULL)
    {
      DPRINT1("ExAllocatePool() failed\n");
      return NULL;
    }

  CacheEntry->HashValue = HashValue;
  CacheEntry->Index = Index;
  CacheEntry->RefCount = 1;

  Sd = (PSECURITY_DESCRIPTOR)(CacheEntry + 1);
  RtlCopyMemory(Sd,
		SecurityDescriptor,
		Length);

  *NewSD = Sd;

  DPRINT("ObpCreateCacheEntry() done\n");

  return CacheEntry;
}


static BOOLEAN
ObpCompareSecurityDescriptors(IN PSECURITY_DESCRIPTOR Sd1,
			      IN PSECURITY_DESCRIPTOR Sd2)
{
  ULONG Length1;
  ULONG Length2;

  Length1 = RtlLengthSecurityDescriptor(Sd1);
  Length2 = RtlLengthSecurityDescriptor(Sd2);
  if (Length1 != Length2)
    return FALSE;

  if (RtlCompareMemory(Sd1, Sd2, Length1) != Length1)
    return FALSE;

  return TRUE;
}


NTSTATUS
ObpAddSecurityDescriptor(IN PSECURITY_DESCRIPTOR SourceSD,
			 OUT PSECURITY_DESCRIPTOR *DestinationSD)
{
  PSECURITY_DESCRIPTOR Sd;
  PLIST_ENTRY CurrentEntry;
  PSD_CACHE_ENTRY CacheEntry;
  ULONG HashValue;
  ULONG Index;
  NTSTATUS Status;

  DPRINT("ObpAddSecurityDescriptor() called\n");

  HashValue = ObpHashSecurityDescriptor(SourceSD);
  Index = HashValue & 0xFF;

  ObpSdCacheLock();

  if (!IsListEmpty(&ObpSdCache[Index]))
    {
      CurrentEntry = ObpSdCache[Index].Flink;
      while (CurrentEntry != &ObpSdCache[Index])
	{
	  CacheEntry = CONTAINING_RECORD(CurrentEntry,
					 SD_CACHE_ENTRY,
					 ListEntry);
	  Sd = (PSECURITY_DESCRIPTOR)(CacheEntry + 1);

	  if (CacheEntry->HashValue == HashValue &&
	      ObpCompareSecurityDescriptors(SourceSD, Sd))
	    {
	      CacheEntry->RefCount++;
	      DPRINT("RefCount %lu\n", CacheEntry->RefCount);
	      *DestinationSD = Sd;

	      ObpSdCacheUnlock();

	      DPRINT("ObpAddSecurityDescriptor() done\n");

	      return STATUS_SUCCESS;
	    }

	  CurrentEntry = CurrentEntry->Flink;
	}
    }

  CacheEntry = ObpCreateCacheEntry(SourceSD,
				   HashValue,
				   Index,
				   DestinationSD);
  if (CacheEntry == NULL)
    {
      DPRINT1("ObpCreateCacheEntry() failed\n");
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      DPRINT("RefCount 1\n");
      InsertTailList(&ObpSdCache[Index], &CacheEntry->ListEntry);
      Status = STATUS_SUCCESS;
    }

  ObpSdCacheUnlock();

  DPRINT("ObpAddSecurityDescriptor() done\n");

  return Status;
}


NTSTATUS
ObpRemoveSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  PSD_CACHE_ENTRY CacheEntry;

  DPRINT("ObpRemoveSecurityDescriptor() called\n");

  ObpSdCacheLock();

  CacheEntry = (PSD_CACHE_ENTRY)((ULONG_PTR)SecurityDescriptor - sizeof(SD_CACHE_ENTRY));

  CacheEntry->RefCount--;
  DPRINT("RefCount %lu\n", CacheEntry->RefCount);
  if (CacheEntry->RefCount == 0)
    {
      DPRINT("Remove cache entry\n");
      RemoveEntryList(&CacheEntry->ListEntry);
      ExFreePool(CacheEntry);
    }

  ObpSdCacheUnlock();

  DPRINT("ObpRemoveSecurityDescriptor() done\n");

  return STATUS_SUCCESS;
}


VOID
ObpReferenceCachedSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  PSD_CACHE_ENTRY CacheEntry;

  DPRINT("ObpReferenceCachedSecurityDescriptor() called\n");

  ObpSdCacheLock();

  CacheEntry = (PSD_CACHE_ENTRY)((ULONG_PTR)SecurityDescriptor - sizeof(SD_CACHE_ENTRY));

  CacheEntry->RefCount++;
  DPRINT("RefCount %lu\n", CacheEntry->RefCount);

  ObpSdCacheUnlock();

  DPRINT("ObpReferenceCachedSecurityDescriptor() done\n");
}


VOID
ObpDereferenceCachedSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  DPRINT("ObpDereferenceCachedSecurityDescriptor() called\n");

  ObpRemoveSecurityDescriptor(SecurityDescriptor);

  DPRINT("ObpDereferenceCachedSecurityDescriptor() done\n");
}

/* EOF */
