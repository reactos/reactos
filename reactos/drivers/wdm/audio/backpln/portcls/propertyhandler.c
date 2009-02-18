#include "private.h"

NTSTATUS
NTAPI
PinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    switch(Request->Id)
    {
        case KSPROPERTY_PIN_CTYPES:
        case KSPROPERTY_PIN_DATAFLOW:
        case KSPROPERTY_PIN_DATARANGES:
        case KSPROPERTY_PIN_INTERFACES:
        case KSPROPERTY_PIN_MEDIUMS:
        case KSPROPERTY_PIN_COMMUNICATION:
        case KSPROPERTY_PIN_CATEGORY:
        case KSPROPERTY_PIN_NAME:
            // KsPinPropertyHandler
            break;
        case KSPROPERTY_PIN_DATAINTERSECTION:
        case KSPROPERTY_PIN_CINSTANCES:
        case KSPROPERTY_PIN_GLOBALCINSTANCES:
        case KSPROPERTY_PIN_NECESSARYINSTANCES:
        case KSPROPERTY_PIN_PHYSICALCONNECTION:
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
            DPRINT1("Unhandled %x\n", Request->Id);
            Status = STATUS_SUCCESS;
            break;
        default:
            Status = STATUS_NOT_FOUND;
    }


    return Status;
}


NTSTATUS
NTAPI
TopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    return KsTopologyPropertyHandler(Irp,
                                     Request,
                                     Data,
                                     NULL /* FIXME */);
}

