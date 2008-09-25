/*
 * Dummy NIC Driver
 *
 * Copyright (C) 2008 Cameron Gutman <cgutman@reactos.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <ndis.h>

#define NDEBUG
#include <debug.h>
#include "debug.h"

static VOID
STDCALL
MiniportHandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext)
{
  DbgPrint("MiniportHandleInterrupt Called\n");

  ASSERT(MiniportAdapterContext);
  ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);
}



static VOID
STDCALL
MiniportHalt(
    IN NDIS_HANDLE MiniportAdapterContext)
{
  DbgPrint("MiniportHalt Called\n");
  ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
  ASSERT(MiniportAdapterContext);
}

static NDIS_STATUS
STDCALL
MiniportInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext)
{
  UINT i = 0;
  NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

  DbgPrint("MiniportInitialize Called\n");

  ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
  ASSERT(MiniportAdapterHandle);
  ASSERT(WrapperConfigurationContext);

  for(i = 0; i < MediumArraySize; i++)
    if(MediumArray[i] == NdisMedium802_3)
      break;

  if(i == MediumArraySize)
     Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
  else
     *SelectedMediumIndex = i;

  *OpenErrorStatus = Status;

  return Status;
}

static VOID
STDCALL
MiniportISR(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext)
{
  DbgPrint("MiniportISR Called\n");
  ASSERT(MiniportAdapterContext);
}

static NDIS_STATUS
STDCALL
MiniportReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext)
{
  DbgPrint("MiniportReset Called\n");

  ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
  ASSERT(MiniportAdapterContext);

  *AddressingReset = FALSE;

  return NDIS_STATUS_SUCCESS;
}

static NDIS_STATUS
STDCALL
MiniportSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags)
{
  DbgPrint("MiniportSend Called\n");

  ASSERT(MiniportAdapterContext);
  ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

  return NDIS_STATUS_SUCCESS;
}

NTSTATUS
STDCALL
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
  NDIS_HANDLE WrapperHandle;
  NDIS_MINIPORT_CHARACTERISTICS Characteristics;
  NDIS_STATUS Status;

  RtlZeroMemory(&Characteristics, sizeof(NDIS_MINIPORT_CHARACTERISTICS));
  Characteristics.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
  Characteristics.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
  Characteristics.HaltHandler = MiniportHalt;
  Characteristics.HandleInterruptHandler = MiniportHandleInterrupt;
  Characteristics.InitializeHandler = MiniportInitialize;
  Characteristics.ISRHandler = MiniportISR;
  //Characteristics.QueryInformationHandler = MiniportQueryInformation;
  Characteristics.ResetHandler = MiniportReset;
  //Characteristics.SetInformationHandler = MiniportSetInformation;
  Characteristics.SendHandler = MiniportSend;

  NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, 0);

  if (!WrapperHandle)
      return NDIS_STATUS_FAILURE;

  Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(NDIS_MINIPORT_CHARACTERISTICS));
  if (Status != NDIS_STATUS_SUCCESS) {
      NdisTerminateWrapper(WrapperHandle, 0);
      return NDIS_STATUS_FAILURE;
  }

  return NDIS_STATUS_SUCCESS;
}

