/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/miniport.c
 * PURPOSE:         Miniport construction api
 * PROGRAMMER:      Andrew Greenwood
 */

#include "private.h"

/*
 * @implemented
 */
NTSTATUS NTAPI
PcNewMiniport(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    DPRINT("PcNewMiniport entered\n");
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!OutMiniport)
    {
        DPRINT("PcNewMiniport was supplied a NULL OutPort parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (IsEqualGUIDAligned(ClassId, &CLSID_MiniportDriverDMusUART) ||
        IsEqualGUIDAligned(ClassId, &CLSID_MiniportDriverUart) ||
        IsEqualGUIDAligned(ClassId, &CLSID_MiniportDriverDMusUARTCapture))
    {
        Status = NewMiniportDMusUART(OutMiniport, ClassId);
    }
    else if (IsEqualGUIDAligned(ClassId, &CLSID_MiniportDriverFmSynth) ||
             IsEqualGUIDAligned(ClassId, &CLSID_MiniportDriverFmSynthWithVol))
    {
        Status = NewMiniportFmSynth(OutMiniport, ClassId);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    DPRINT("PcNewMiniport Status %x\n", Status);
    return Status;
}

