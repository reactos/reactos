/*--

Copyright (C) Microsoft Corporation, 1999

--*/

#include "cdrom.h"

NTSTATUS
NTAPI
CdRomGetRpc0Settings(
    IN PDEVICE_OBJECT Fdo
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCDROM_DATA cddata = (PCDROM_DATA)(commonExtension->DriverData);

    cddata->Rpc0SystemRegion = (UCHAR)(~1);        // region one
    cddata->Rpc0SystemRegionResetCount = 0; // no resets

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CdRomSetRpc0Settings(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR NewRegion
    )
{
    return STATUS_SUCCESS;
}
