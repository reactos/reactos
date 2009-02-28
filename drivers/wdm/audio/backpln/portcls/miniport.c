/*
    ReactOS Operating System

    Port Class API
    IMiniPortMidi Implementation

    by Andrew Greenwood

    REFERENCE:
        http://www.osronline.com/ddkx/stream/audmp-routines_64vn.htm
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

    DPRINT1("PcNewMiniport entered\n");

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

