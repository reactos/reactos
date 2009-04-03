/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        object.c
 * PURPOSE:     Implements the NDIS 6.0 object interface
 * PROGRAMMERS: Cameron Gutman (aicommander@gmail.com)
 */

#include "ndissys.h"

PNDIS_GENERIC_OBJECT
EXPORT
NdisAllocateGenericObject(
   IN PDRIVER_OBJECT DriverObject OPTIONAL,
   IN ULONG Tag,
   IN USHORT Size)
{
  PNDIS_GENERIC_OBJECT Object;

  Object = ExAllocatePoolWithTag(NonPagedPool, sizeof(NDIS_GENERIC_OBJECT) + Size, Tag);
  if (!Object) return NULL;

  RtlZeroMemory(Object, sizeof(NDIS_GENERIC_OBJECT) + Size);

  Object->DriverObject = DriverObject;
  Object->Header.Type = NDIS_OBJECT_TYPE_GENERIC_OBJECT;
  Object->Header.Revision = NDIS_GENERIC_OBJECT_REVISION_1;
  Object->Header.Size = sizeof(NDIS_GENERIC_OBJECT);

  return Object;
}

VOID
EXPORT
NdisFreeGenericObject(
   IN PNDIS_GENERIC_OBJECT NdisGenericObject)
{
  ExFreePool(NdisGenericObject);
}

