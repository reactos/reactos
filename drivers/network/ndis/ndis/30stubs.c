/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/30stubs.c
 * PURPOSE:     NDIS 3.0 Stubs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "ndissys.h"

#include <afilter.h>

/*
 * @unimplemented
 */
VOID
EXPORT
ArcFilterDprIndicateReceive(
    IN  PARC_FILTER Filter,
    IN  PUCHAR      pRawHeader,
    IN  PUCHAR      pData,
    IN  UINT        Length)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
EXPORT
ArcFilterDprIndicateReceiveComplete(
    IN  PARC_FILTER Filter)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiFilterDprIndicateReceive(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     MacReceiveContext,
    IN  PCHAR           Address,
    IN  UINT            AddressLength,
    IN  PVOID           HeaderBuffer,
    IN  UINT            HeaderBufferSize,
    IN  PVOID           LookaheadBuffer,
    IN  UINT            LookaheadBufferSize,
    IN  UINT            PacketSize)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiFilterDprIndicateReceiveComplete(
    IN  PFDDI_FILTER    Filter)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisReadBindingInformation (
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_STRING    * Binding,
    IN  NDIS_HANDLE     ConfigurationHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
EXPORT
TrFilterDprIndicateReceive(
    IN  PTR_FILTER  Filter,
    IN  NDIS_HANDLE MacReceiveContext,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize)
{
    UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
EXPORT
TrFilterDprIndicateReceiveComplete(
    IN  PTR_FILTER  Filter)
{
    UNIMPLEMENTED;
}

/* EOF */
