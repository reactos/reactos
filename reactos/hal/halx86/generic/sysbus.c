/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/sysbus.c
 * PURPOSE:         System bus handler functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  09/04/2000 Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

ULONG
NTAPI
HalpGetSystemInterruptVector(ULONG BusNumber,
                             ULONG BusInterruptLevel,
                             ULONG BusInterruptVector,
                             PKIRQL Irql,
                             PKAFFINITY Affinity)
{
    ULONG Vector = IRQ2VECTOR(BusInterruptVector);
    *Irql = (KIRQL)VECTOR2IRQL(Vector);
    *Affinity = 0xFFFFFFFF;
    return Vector;
}

/* EOF */
