/* $Id: direntry.c,v 1.10 2002/11/11 21:49:18 hbirr Exp $
 *
 *
 * FILE:             DirEntry.c
 * PURPOSE:          Routines to manipulate directory entries.
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Rex Jolliff (rex@lvcablemodem.com)
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

BOOL
vfatIsDirEntryDeleted (FATDirEntry * pFatDirEntry)
{
  return  pFatDirEntry->Filename [0] == 0xe5;
}

BOOL
vfatIsDirEntryEndMarker (FATDirEntry * pFatDirEntry)
{
  return  pFatDirEntry->Filename [0] == 0;
}

BOOL
vfatIsDirEntryLongName (FATDirEntry * pFatDirEntry)
{
  return  pFatDirEntry->Attrib == 0x0f;
}

BOOL
vfatIsDirEntryVolume (FATDirEntry * pFatDirEntry)
{
  return pFatDirEntry->Attrib == 0x28;
}

void
vfatGetDirEntryName (PFAT_DIR_ENTRY  dirEntry,  PWSTR  entryName)
{
  vfat8Dot3ToString (dirEntry->Filename, dirEntry->Ext, entryName);
}


NTSTATUS vfatGetNextDirEntry(PVOID * pContext,
			     PVOID * pPage,
			     IN PVFATFCB pDirFcb,
			     IN OUT PULONG pDirIndex,
			     OUT PWSTR pFileName,
			     OUT PFAT_DIR_ENTRY pDirEntry,
			     OUT PULONG pStartIndex)
{
    ULONG dirMap;
    PWCHAR pName;
    LARGE_INTEGER FileOffset;
    FATDirEntry * fatDirEntry;
    slot * longNameEntry;
    ULONG index;

    DPRINT ("vfatGetNextDirEntry (%x,%x,%d,%x,%x)\n",
            DeviceExt,
            pDirFcb,
            *pDirIndex,
            pFileName,
            pDirEntry);

    *pFileName = 0;

    FileOffset.u.HighPart = 0;
    FileOffset.u.LowPart = ROUND_DOWN(*pDirIndex * sizeof(FATDirEntry), PAGE_SIZE);

    if (*pContext == NULL || (*pDirIndex % ENTRIES_PER_PAGE) == 0)
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


    fatDirEntry = (FATDirEntry*)(*pPage) + *pDirIndex % ENTRIES_PER_PAGE;
    longNameEntry = (slot*) fatDirEntry;
    dirMap = 0;

    if (pStartIndex)
    {
	*pStartIndex = *pDirIndex;
    }

    while (TRUE)
    {
	if (vfatIsDirEntryEndMarker(fatDirEntry))
	{
	    CcUnpinData(*pContext);
	    *pContext = NULL;
	    return STATUS_NO_MORE_ENTRIES;
	}

	if (vfatIsDirEntryDeleted (fatDirEntry))
	{
	    dirMap = 0;
	    *pFileName = 0;
	    if (pStartIndex)
	    {
		*pStartIndex = *pDirIndex + 1;
	    }
	}
	else
	{
	    if (vfatIsDirEntryLongName (fatDirEntry))
	    {
		if (dirMap == 0)
		{
		    DPRINT ("  long name entry found at %d\n", *pDirIndex);
                    memset(pFileName, 0, 256 * sizeof(WCHAR));
		}

		DPRINT ("  name chunk1:[%.*S] chunk2:[%.*S] chunk3:[%.*S]\n",
			 5, longNameEntry->name0_4,
			 6, longNameEntry->name5_10,
			 2, longNameEntry->name11_12);

		index = (longNameEntry->id & 0x1f) - 1;
		dirMap |= 1 << index;
		pName = pFileName + 13 * index;

		memcpy(pName, longNameEntry->name0_4, 5 * sizeof(WCHAR));
		memcpy(pName + 5, longNameEntry->name5_10, 6 * sizeof(WCHAR));
		memcpy(pName + 11, longNameEntry->name11_12, 2 * sizeof(WCHAR));
      
		DPRINT ("  longName: [%S]\n", pFileName);
	    }
	    else
	    {
	        memcpy (pDirEntry, fatDirEntry, sizeof (FAT_DIR_ENTRY));
		break;
	    }
	}
	(*pDirIndex)++;
	if ((*pDirIndex % ENTRIES_PER_PAGE) == 0)
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
    return STATUS_SUCCESS;
}








