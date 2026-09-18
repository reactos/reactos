

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/**
* @name NtfsFlushBuffers
* @implemented
*
* Handles IRP_MJ_FLUSH_BUFFERS.
*
* @param IrpContext
* Points to an NTFS_IRP_CONTEXT which describes the request
*
* @return
* STATUS_SUCCESS if everything reached the media, otherwise the status the
* storage stack returned.
*
*/
NTSTATUS
NtfsFlushBuffers(PNTFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;

    DPRINT("NtfsFlushBuffers(%p)\n", IrpContext);

    IrpContext->Irp->IoStatus.Information = 0;

    /* Nothing is mounted on the main device object */
    if (IrpContext->DeviceObject == NtfsGlobalData->DeviceObject)
    {
        return STATUS_SUCCESS;
    }

    DeviceExt = IrpContext->DeviceObject->DeviceExtension;

    if (DeviceExt->Flags & VCB_VOLUME_DISMOUNTED)
    {
        return STATUS_VOLUME_DISMOUNTED;
    }

    return NtfsFlushDevice(DeviceExt->StorageDevice);
}

/* EOF */
