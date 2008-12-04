#include "private.h"


const GUID CLSID_PortTopology;
const GUID CLSID_PortMidi;
const GUID CLSID_PortWaveCyclic;
const GUID CLSID_PortWavePci;
const GUID CLSID_PortDMus;

PORTCLASSAPI
NTSTATUS
NTAPI
PcNewPort(
    OUT PPORT* OutPort,
    IN  REFCLSID ClassId)
{
    NTSTATUS Status;

    if (!OutPort)
    {
        DPRINT("PcNewPort was supplied a NULL OutPort parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (IsEqualGUIDAligned(ClassId, &CLSID_PortMidi))
        Status = NewPortMidi(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortDMus))
        Status = NewPortDMus(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortTopology))
        Status = NewPortTopology(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortWaveCyclic))
        Status = NewPortWaveCyclic(OutPort);
    else if (IsEqualGUIDAligned(ClassId, &CLSID_PortWavePci))
        Status = NewPortWavePci(OutPort);
    else
        Status = STATUS_NOT_SUPPORTED;

    DPRINT("PcNewPort Status %lx\n", Status);

    return Status;
}
