/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/multimedia/portcls/stubs.c
 * PURPOSE:         Port Class driver / Stubs
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.h"
#include <portcls.h>

/*
    Factory Stubs
*/



/* ===============================================================
    Power Management
*/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRequestNewPowerState(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  DEVICE_POWER_STATE RequestedNewState)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/* ===============================================================
    Properties
*/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcCompletePendingPropertyRequest(
    IN  PPCPROPERTY_REQUEST PropertyRequest,
    IN  NTSTATUS NtStatus)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

