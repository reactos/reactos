/* $Id: fcb.c,v 1.1 2001/05/02 03:18:03 rex Exp $
 *
 *
 * FILE:             fcb.c
 * PURPOSE:          Routines to manipulate FCBs.
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

/*  --------------------------------------------------------  DEFINES  */

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_FCB TAG('V', 'F', 'C', 'B')

/*  --------------------------------------------------------  PUBLICS  */

PVFATFCB  vfatNewFCB (PWCHAR pFileName)
{
  PVFATFCB  rcFCB;

  rcFCB = ExAllocatePoolWithTag (NonPagedPool, sizeof (VFATFCB), TAG_FCB);
  memset (rcFCB, 0, sizeof (VFATFCB));
  if (pFileName)
  {
    wcscpy (rcFCB->PathName, pFileName);
    if (wcsrchr (rcFCB->PathName, '\\') != 0)
    {
      rcFCB->ObjectName = wcsrchr (rcFCB->PathName, '\\');
    }
    else
    {
      rcFCB->ObjectName = rcFCB->PathName;
    }
  }

  return  rcFCB;
}

void  vfatGrabFCB (PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->RefCount++;
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

void  vfatReleaseFCB (PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->RefCount--;
  if (pFCB->RefCount <= 0)
  {
    RemoveEntryList (&pFCB->FcbListEntry);
    ExFreePool (pFCB);
  }
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

void  vfatAddFCBToTable (PDEVICE_EXTENSION  pVCB,  PVFATFCB  pFCB)
{
  KIRQL  oldIrql;

  KeAcquireSpinLock (&pVCB->FcbListLock, &oldIrql);
  pFCB->pDevExt = pVCB;
  InsertTailList (&pVCB->FcbListHead, &pFCB->FcbListEntry);
  KeReleaseSpinLock (&pVCB->FcbListLock, oldIrql);
}

PVFATFCB  vfatGrabFCBFromTable (PDEVICE_EXTENSION  pDeviceExt, PWSTR  pFileName)
{
  KIRQL  oldIrql;
  PVFATFCB  rcFCB;
  PLIST_ENTRY  current_entry;

  KeAcquireSpinLock (&pDeviceExt->FcbListLock, &oldIrql);
  current_entry = pDeviceExt->FcbListHead.Flink;
  while (current_entry != &pDeviceExt->FcbListHead)
  {
    rcFCB = CONTAINING_RECORD (current_entry, VFATFCB, FcbListEntry);

    DPRINT ("Scanning %x(%S)\n", rcFCB, rcFCB->PathName);

    if (wstrcmpi (pFileName, rcFCB->PathName))
    {
      rcFCB->RefCount++;
      KeReleaseSpinLock (&pDeviceExt->FcbListLock, oldIrql);
      return  rcFCB;
    }
    current_entry = current_entry->Flink;
  }
  KeReleaseSpinLock (&pDeviceExt->FcbListLock, oldIrql);

  return  NULL;
}
