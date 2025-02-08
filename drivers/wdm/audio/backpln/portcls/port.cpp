/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port.cpp
 * PURPOSE:         Port construction API
 * PROGRAMMER:      Johannes Anderwald
 *                  Andrew Greenwood
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
PcNewPort(
    OUT PPORT* OutPort,
    IN  REFCLSID ClassId)
{
    NTSTATUS Status;
    UNICODE_STRING GuidString;

    DPRINT("PcNewPort entered\n");

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!OutPort)
    {
        DPRINT("PcNewPort was supplied a NULL OutPort parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (IsEqualGUIDAligned(ClassId, CLSID_PortMidi))
        Status = NewPortDMus(OutPort);
    else if (IsEqualGUIDAligned(ClassId, CLSID_PortDMus))
        Status = NewPortDMus(OutPort);
    else if (IsEqualGUIDAligned(ClassId, CLSID_PortTopology))
        Status = NewPortTopology(OutPort);
    else if (IsEqualGUIDAligned(ClassId, CLSID_PortWaveCyclic))
        Status = NewPortWaveCyclic(OutPort);
    else if (IsEqualGUIDAligned(ClassId, CLSID_PortWavePci))
        Status = NewPortWavePci(OutPort);
    else if (IsEqualGUIDAligned(ClassId, CLSID_PortWaveRT))
        Status = NewPortWaveRT(OutPort);
    else
    {

        if (RtlStringFromGUID(ClassId, &GuidString) == STATUS_SUCCESS)
        {
            DPRINT1("unknown interface %S\n", GuidString.Buffer);
            RtlFreeUnicodeString(&GuidString);
        }

        Status = STATUS_NOT_SUPPORTED;
        return Status;
     }
    DPRINT("PcNewPort Status %lx\n", Status);

    return Status;
}
