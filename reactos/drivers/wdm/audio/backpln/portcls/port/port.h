/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/multimedia/portcls/port/port.h
 * PURPOSE:         Port Class driver / Private header for IPort
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */


/*
    Private header for Port implementations
*/

#ifndef PORT_PRIVATE_H
#define PORT_PRIVATE_H

#include <stdunk.h>
#include <portcls.h>

typedef struct CPort
{
    union
    {
        IUnknown IUnknown;
        IPort IPort;
    };

    LONG m_ref_count;
    PUNKNOWN m_outer_unknown;
} CPort;

#endif
