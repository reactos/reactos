/*
    ReactOS Operating System
    Port Class API / Port Factory

    by Andrew Greenwood
*/

#define INITGUID

#include "private.h"
#include <portcls.h>
#include <ks.h>
#include <kcom.h>
#include "port.h"

/*
 * @unimplemented
 */
PORTCLASSAPI NTSTATUS NTAPI
PcNewPort(
    OUT PPORT* OutPort,
    IN  REFCLSID ClassId)
{
    /*
        ClassId can be one of the following:
        CLSID_PortDMus -> IPortDMus (dmusicks.h)    -- TODO
        CLSID_PortMidi -> IPortMidi
        CLSID_PortTopology -> IPortTopology
        CLSID_PortWaveCyclic -> IPortWaveCyclic
        CLSID_PortWavePci -> IPortWavePci

        TODO: What about PortWavePciStream?
    */

    PPORT new_port = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if ( ! OutPort )
    {
        DPRINT("PcNewPort was supplied a NULL OutPort parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ( ( IsEqualGUIDAligned(ClassId, &CLSID_PortMidi)         ) ||
         ( IsEqualGUIDAligned(ClassId, &CLSID_PortTopology)     ) ||
         ( IsEqualGUIDAligned(ClassId, &CLSID_PortWaveCyclic)   ) ||
         ( IsEqualGUIDAligned(ClassId, &CLSID_PortWavePci)      ) )
    {
        DPRINT("Calling KoCreateInstance\n");
        /* Call KS.SYS's Kernel-mode COM function */
        status = KoCreateInstance(ClassId, NULL, CLSCTX_KERNEL_SERVER, &IID_IPort, &new_port);
    }
    else
    {

        DPRINT("PcNewPort received a CLSID it does not deal with\n");
        status = STATUS_NOT_SUPPORTED;
    }

    /* If an unsupported CLSID was handed to us, or the creation failed, we fail */
    if ( status != STATUS_SUCCESS )
    {
        return status;
    }

    /* Fill the caller's PPORT* to point to the new port */
    *OutPort = new_port;

    DPRINT("PcNewPort succeeded\n");

    return STATUS_SUCCESS;
}
