/* $Id: direntry.c,v 1.17 2004/08/01 21:57:17 navaraf Exp $
 *
 *
 * FILE:             DirEntry.c
 * PURPOSE:          Routines to manipulate directory entries.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Rex Jolliff (rex@lvcablemodem.com)
 *                   Hartmut Birr
 */

/*  -------------------------------------------------------  INCLUDES  */

#include <ddk/ntddk.h>
#include <wchar.h>
#include <limits.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

#define ENTRIES_PER_PAGE   (PAGE_SIZE / sizeof (FATDirEntry))

ULONG 
vfatDirEntryGetFirstCluster (PDEVICE_EXTENSION  pDeviceExt,
                             PFAT_DIR_ENTRY  pFatDirEntry)
{
  ULONG  cluster;

  if (pDeviceExt->FatInfo.FatType == FAT32)
  {
    cluster = pFatDirEntry->FirstCluster + 
      pFatDirEntry->FirstClusterHigh * 65536;
  }
  else
  {
    cluster = pFatDirEntry->FirstCluster;
  }

  return  cluster;
}

BOOL VfatIsDirectoryEmpty(PVFATFCB Fcb)
{
   LARGE_INTEGER FileOffset;
   PVOID Context = NULL;
   PFAT_DIR_ENTRY FatDirEntry;
   ULONG Index, MaxIndex;

   if (vfatFCBIsRoot(Fcb))
     {
       Index = 0;
     }
   else
     {
       Index = 2;
     }

   FileOffset.QuadPart = 0LL;
   MaxIndex = Fcb->RFCB.FileSize.u.LowPart / sizeof(FAT_DIR_ENTRY);

   while (Index < MaxIndex)
     {
       if (Context == NULL || (Index % ENTRIES_PER_PAGE) == 0)
         {
	   if (Context != NULL)
	     {
	       CcUnpinData(Context);
	     }
	    if (!CcMapData(Fcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, &Context, (PVOID*)&FatDirEntry))
	      {
		return TRUE;
	      }
	    FatDirEntry += Index % ENTRIES_PER_PAGE;
	 }
       if (ENTRY_END(FatDirEntry))
	  {
	    CcUnpinData(Context);
	    return TRUE;
	  }
       if (!ENTRY_DELETED(FatDirEntry))
         {
	   CcUnpinData(Context);
	   return FALSE;
	 }
       Index++;
       FatDirEntry++;
     }
   if (Context)
     {
       CcUnpinData(Context);
     }
   return TRUE;
}

NTSTATUS vfatGetNextDirEntry(PVOID * pContext,
			     PVOID * pPage,
                             IN PVFATFCB pDirFcb,
			     PVFAT_DIRENTRY_CONTEXT DirContext,
			     BOOLEAN First)
{
    ULONG dirMap;
    PWCHAR pName;
    LARGE_INTEGER FileOffset;
    FATDirEntry * fatDirEntry;
    slot * longNameEntry;
    ULONG index;
    
    UCHAR CheckSum, shortCheckSum;
    USHORT i;
    BOOLEAN Valid = TRUE;
    BOOLEAN Back = FALSE;

    DirContext->LongNameU.Buffer[0] = 0;

    FileOffset.u.HighPart = 0;
    FileOffset.u.LowPart = ROUND_DOWN(DirContext->DirIndex * sizeof(FATDirEntry), PAGE_SIZE);

    if (*pContext == NULL || (DirContext->DirIndex % ENTRIES_PER_PAGE) == 0)
    {
       if (*pContext != NULL)
       {
	  CcUnpinData(*pContext);
       }
       if (!CcMapData(pDirFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, pContext, pPage))
       {
	  *pContext = NULL;
          return STATUS_NO_MORE_ENTRIES;
       }
    }


    fatDirEntry = (FATDirEntry*)(*pPage) + DirContext->DirIndex % ENTRIES_PER_PAGE;
    longNameEntry = (slot*) fatDirEntry;
    dirMap = 0;

    if (First)
      {
        /* This is the first call to vfatGetNextDirEntry. Possible the start index points 
	 * into a long name or points to a short name with an assigned long name. 
	 * We must go back to the real start of the entry */
        while (DirContext->DirIndex > 0 && 
	       !ENTRY_END(fatDirEntry) && 
	       !ENTRY_DELETED(fatDirEntry) && 
	       ((!ENTRY_LONG(fatDirEntry) && !Back) || 
	        (ENTRY_LONG(fatDirEntry) && !(longNameEntry->id & 0x40))))
          {
            DirContext->DirIndex--;
	    Back = TRUE;
            if ((DirContext->DirIndex % ENTRIES_PER_PAGE) == ENTRIES_PER_PAGE - 1)
              {
                CcUnpinData(*pContext);
                FileOffset.u.LowPart -= PAGE_SIZE;
                if (!CcMapData(pDirFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, pContext, pPage))
                  {
	            CHECKPOINT;
	            *pContext = NULL;
	            return STATUS_NO_MORE_ENTRIES;
		  }
                fatDirEntry = (FATDirEntry*)(*pPage) + DirContext->DirIndex % ENTRIES_PER_PAGE;
                longNameEntry = (slot*) fatDirEntry;
              }
	    else
	      {
                fatDirEntry--;
	        longNameEntry--;
	      }
          }

        if (Back && !ENTRY_END(fatDirEntry) && 
	    (ENTRY_DELETED(fatDirEntry) || !ENTRY_LONG(fatDirEntry)))
          {
            DirContext->DirIndex++;
	    if ((DirContext->DirIndex % ENTRIES_PER_PAGE) == 0)
	      {
	        CcUnpinData(*pContext);
	        FileOffset.u.LowPart += PAGE_SIZE;
	        if (!CcMapData(pDirFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, pContext, pPage))
	          {
		    CHECKPOINT;
		    *pContext = NULL;
		    return STATUS_NO_MORE_ENTRIES;
	          }
	        fatDirEntry = (FATDirEntry*)*pPage;
	        longNameEntry = (slot*) *pPage;
	      }
	    else
 	      {
	        fatDirEntry++;
	        longNameEntry++;
	      }
          }
      }

    DirContext->StartIndex = DirContext->DirIndex;
    CheckSum = 0;

    while (TRUE)
      {
	if (ENTRY_END(fatDirEntry))
	  {
	    CcUnpinData(*pContext);
	    *pContext = NULL;
	    return STATUS_NO_MORE_ENTRIES;
	  }

	if (ENTRY_DELETED(fatDirEntry))
	  {
	    dirMap = 0;
	    DirContext->LongNameU.Buffer[0] = 0;
	    DirContext->StartIndex = DirContext->DirIndex + 1;
	  }
	else
	  {
	    if (ENTRY_LONG(fatDirEntry))
	      {
		if (dirMap == 0)
		  {
		    DPRINT ("  long name entry found at %d\n", DirContext->DirIndex);
                    memset(DirContext->LongNameU.Buffer, 0, DirContext->LongNameU.MaximumLength);
		    CheckSum = longNameEntry->alias_checksum;
		    Valid = TRUE;
		  }

		DPRINT ("  name chunk1:[%.*S] chunk2:[%.*S] chunk3:[%.*S]\n",
			 5, longNameEntry->name0_4,
			 6, longNameEntry->name5_10,
			 2, longNameEntry->name11_12);

		index = (longNameEntry->id & 0x1f) - 1;
		dirMap |= 1 << index;
		pName = DirContext->LongNameU.Buffer + 13 * index;

		memcpy(pName, longNameEntry->name0_4, 5 * sizeof(WCHAR));
		memcpy(pName + 5, longNameEntry->name5_10, 6 * sizeof(WCHAR));
		memcpy(pName + 11, longNameEntry->name11_12, 2 * sizeof(WCHAR));
      
		DPRINT ("  longName: [%S]\n", DirContext->LongNameU.Buffer);
		if (CheckSum != longNameEntry->alias_checksum)
		  {
		    DPRINT1("Found wrong alias checksum in long name entry (first %x, current %x, %S)\n",
			    CheckSum, longNameEntry->alias_checksum, DirContext->LongNameU.Buffer);
		    Valid = FALSE;
		  }
	      }
	    else
	      {
	        shortCheckSum = 0;
                for (i = 0; i < 11; i++)
                  {
                    shortCheckSum = (((shortCheckSum & 1) << 7)
                                  | ((shortCheckSum & 0xfe) >> 1))
                                  + fatDirEntry->Filename[i];
		  }
	        if (shortCheckSum != CheckSum && DirContext->LongNameU.Buffer[0])
		  {
		    DPRINT1("Checksum from long and short name is not equal (short: %x, long: %x, %S)\n",
			     shortCheckSum, CheckSum, DirContext->LongNameU.Buffer);
		    DirContext->LongNameU.Buffer[0] = 0;
		  }
	        if (Valid == FALSE)
		  {
		    DirContext->LongNameU.Buffer[0] = 0;
		  }
    
	        memcpy (&DirContext->FatDirEntry, fatDirEntry, sizeof (FAT_DIR_ENTRY));
		break;
	      }
	   }
	DirContext->DirIndex++;
	if ((DirContext->DirIndex % ENTRIES_PER_PAGE) == 0)
	  {
	    CcUnpinData(*pContext);
	    FileOffset.u.LowPart += PAGE_SIZE;
	    if (!CcMapData(pDirFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, pContext, pPage))
	      {
		CHECKPOINT;
		*pContext = NULL;
		return STATUS_NO_MORE_ENTRIES;
	      }
	    fatDirEntry = (FATDirEntry*)*pPage;
	    longNameEntry = (slot*) *pPage;
	  }
	else
	  {
	    fatDirEntry++;
	    longNameEntry++;
	  }
      }
    DirContext->LongNameU.Length = wcslen(DirContext->LongNameU.Buffer) * sizeof(WCHAR);
    vfat8Dot3ToString(&DirContext->FatDirEntry, &DirContext->ShortNameU);
    if (DirContext->LongNameU.Length == 0)
      {
        RtlCopyUnicodeString(&DirContext->LongNameU, &DirContext->ShortNameU);
      }
    return STATUS_SUCCESS;
}
