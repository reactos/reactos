/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Hardware specific functions
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "nic.h"

#include <debug.h>

BOOLEAN
NTAPI
NICRecognizeHardware(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return FALSE;
}

NDIS_STATUS
NTAPI
NICInitializeAdapterResources(
    IN PE1000_ADAPTER Adapter,
    IN PNDIS_RESOURCE_LIST ResourceList)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICAllocateResources(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICRegisterInterrupts(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICUnregisterInterrupts(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICReleaseIoResources(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
NTAPI
NICPowerOn(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICSoftReset(
    IN PE1000_ADAPTER adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICEnableTxRx(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICDisableTxRx(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICGetPermanentMacAddress(
    IN PE1000_ADAPTER Adapter,
    OUT PUCHAR MacAddress)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICApplyInterruptMask(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICDisableInterrupts(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

USHORT
NTAPI
NICInterruptRecognized(
    IN PE1000_ADAPTER Adapter,
    OUT PBOOLEAN InterruptRecognized)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return 0;
}

VOID
NTAPI
NICAcknowledgeInterrupts(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
}

VOID
NTAPI
NICUpdateLinkStatus(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
}

NDIS_STATUS
NTAPI
NICApplyPacketFilter(
    IN PE1000_ADAPTER Adapter)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICTransmitPacket(
    IN PE1000_ADAPTER Adapter,
    IN UCHAR TxDesc,
    IN ULONG PhysicalAddress,
    IN ULONG Length)
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return NDIS_STATUS_FAILURE;
}
