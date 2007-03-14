/*
    ReactOS Operating System
    Port Class API / Port Factory

    by Andrew Greenwood
*/

#include <portcls.h>
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
        CLSID_PortDMus -> IPortDMus (dmusicks.h)
        CLSID_PortMidi -> IPortMidi
        CLSID_PortTopology -> IPortTopology
        CLSID_PortWaveCyclic -> IPortWaveCyclic
        CLSID_PortWavePci -> IPortWavePci
    */

    PPORT new_port = NULL;

    if ( ! OutPort )
    {
        DPRINT("PcNewPort was supplied a NULL OutPort parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME - do not hack, for it is bad */
    //new_port = new PPortMidi;

//    STD_CREATE_BODY_(CPortMidi, (PUNKNOWN) &new_port, NULL, 0, PUNKNOWN);

/*
    if ( ClassId == CLSID_PortMidi )
        new_port = new IPortMidi;
    else if ( ClassId == CLSID_PortTopology )
        new_port = new IPortTopology;
    else if ( ClassId == CLSID_PortWaveCyclic )
        new_port = new IPortWaveCyclic;
    else if ( ClassId == CLSID_PortWavePci )
        new_port = new IPortWavePci;
    else
*/
        return STATUS_NOT_SUPPORTED;

    /* Fill the caller's PPORT* to point to the new port */
    *OutPort = new_port;

    return STATUS_SUCCESS;
}
