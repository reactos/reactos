/* $Id: direntry.c,v 1.1 2001/07/05 01:51:52 rex Exp $
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

#define  ENTRIES_PER_PAGE(pDeviceExt) (ENTRIES_PER_SECTOR * \
           (PAGESIZE / ((pDeviceExt)->BytesPerSector)))

ULONG 
vfatDirEntryGetFirstCluster (PDEVICE_EXTENSION  pDeviceExt,
                             PFAT_DIR_ENTRY  pFatDirEntry)
{
  ULONG  cluster;

  if (pDeviceExt->FatType == FAT32)
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

void
vfatGetDirEntryName (PFAT_DIR_ENTRY  dirEntry,  PWSTR  entryName)
{
  vfat8Dot3ToString (dirEntry->Filename, dirEntry->Ext, entryName);
}

NTSTATUS
vfatGetNextDirEntry (PDEVICE_EXTENSION  pDeviceExt,
                     PVFATFCB  pDirectoryFCB,
                     ULONG * pDirectoryIndex,
                     PWSTR pLongFileName,
                     PFAT_DIR_ENTRY pDirEntry)
{
  NTSTATUS  status;
  ULONG  indexInPage = *pDirectoryIndex % ENTRIES_PER_PAGE(pDeviceExt);
  ULONG  pageNumber = *pDirectoryIndex / ENTRIES_PER_PAGE(pDeviceExt);
  PVOID  currentPage = NULL;
  PCACHE_SEGMENT  cacheSegment = NULL; 
  FATDirEntry * fatDirEntry;
  slot * longNameEntry;
  ULONG  cpos;

  DPRINT ("vfatGetNextDirEntry (%x,%x,%d,%x,%x)\n",
          pDeviceExt,
          pDirectoryFCB,
          *pDirectoryIndex,
          pLongFileName,
          pDirEntry);

  *pLongFileName = 0;

  DPRINT ("Validating current directory page\n");
  status = vfatRequestAndValidateRegion (pDeviceExt, 
                                         pDirectoryFCB, 
                                         pageNumber * PAGESIZE,
                                         (PVOID *) &currentPage,
                                         &cacheSegment,
                                         FALSE);
  if (!NT_SUCCESS (status))
  {
    return  status;
  }

  while (TRUE)
  {
    fatDirEntry = (FATDirEntry *) currentPage;

    if (vfatIsDirEntryEndMarker (&fatDirEntry [indexInPage]))
    {
      DPRINT ("end of directory, returning no more entries\n");
      status = vfatReleaseRegion (pDeviceExt,
                                  pDirectoryFCB,
                                  cacheSegment);
      return  STATUS_NO_MORE_ENTRIES;
    }
    else if (vfatIsDirEntryLongName (&fatDirEntry [indexInPage]))
    {
      DPRINT ("  long name entry found at %d\n", *pDirectoryIndex);
      longNameEntry = (slot *) currentPage;

      DPRINT ("  name chunk1:[%.*S] chunk2:[%.*S] chunk3:[%.*S]\n",
              5, longNameEntry [indexInPage].name0_4,
              6, longNameEntry [indexInPage].name5_10,
              2, longNameEntry [indexInPage].name11_12);

      vfat_initstr (pLongFileName, 256);
      vfat_wcsncpy (pLongFileName, longNameEntry [indexInPage].name0_4, 5);
      vfat_wcsncat (pLongFileName, longNameEntry [indexInPage].name5_10, 5, 6);
      vfat_wcsncat (pLongFileName, longNameEntry [indexInPage].name11_12, 11, 2);

      DPRINT ("  longName: [%S]\n", pLongFileName);

      cpos = 0;
      while ((longNameEntry [indexInPage].id != 0x41) && 
             (longNameEntry [indexInPage].id != 0x01) &&
             (longNameEntry [indexInPage].attr > 0))
      {
        (*pDirectoryIndex)++;
        indexInPage++;
        if (indexInPage == ENTRIES_PER_PAGE(pDeviceExt))
        {
          indexInPage = 0;
          pageNumber++;

          status = vfatReleaseRegion (pDeviceExt,
                                      pDirectoryFCB,
                                      cacheSegment);
          if (!NT_SUCCESS (status))
          {
            return  status;
          }
          status = vfatRequestAndValidateRegion (pDeviceExt, 
                                                 pDirectoryFCB, 
                                                 pageNumber * PAGESIZE,
                                                 (PVOID *) &currentPage,
                                                 &cacheSegment,
                                                 FALSE);
          if (!NT_SUCCESS (status))
          {
            return  status;
          }
          longNameEntry = (slot *) currentPage;
        }
        DPRINT ("  index %d\n", *pDirectoryIndex);

        DPRINT ("  name chunk1:[%.*S] chunk2:[%.*S] chunk3:[%.*S]\n",
                5, longNameEntry [indexInPage].name0_4,
                6, longNameEntry [indexInPage].name5_10,
                2, longNameEntry [indexInPage].name11_12);

        cpos++;
        vfat_movstr (pLongFileName, 13, 0, cpos * 13);
        vfat_wcsncpy (pLongFileName, longNameEntry [indexInPage].name0_4, 5);
        vfat_wcsncat (pLongFileName, longNameEntry [indexInPage].name5_10, 5, 6);
        vfat_wcsncat (pLongFileName, longNameEntry [indexInPage].name11_12, 11, 2);

        DPRINT ("  longName: [%S]\n", pLongFileName);

      }
      (*pDirectoryIndex)++;
      indexInPage++;
      if (indexInPage == ENTRIES_PER_PAGE(pDeviceExt))
      {
        indexInPage = 0;
        pageNumber++;

        status = vfatReleaseRegion (pDeviceExt,
                                    pDirectoryFCB,
                                    cacheSegment);
        if (!NT_SUCCESS (status))
        {
          return  status;
        }
        status = vfatRequestAndValidateRegion (pDeviceExt, 
                                               pDirectoryFCB, 
                                               pageNumber * PAGESIZE,
                                               (PVOID *) &currentPage,
                                               &cacheSegment,
                                               FALSE);
        if (!NT_SUCCESS (status))
        {
          return  status;
        }
      }
    }
    else
    {
      memcpy (pDirEntry, &fatDirEntry [indexInPage], sizeof (FAT_DIR_ENTRY));
      (*pDirectoryIndex)++;
      break;
    }
  }

  DPRINT ("Releasing current directory page\n");
  status = vfatReleaseRegion (pDeviceExt,
                              pDirectoryFCB,
                              cacheSegment);

  return  status;
}





