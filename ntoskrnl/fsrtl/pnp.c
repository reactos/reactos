/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/pnp.c
 * PURPOSE:         Manages PnP support routines for file system drivers.
 * PROGRAMMERS:     heis_spiter@hotmail.com
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <ioevent.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlNotifyVolumeEvent
 * @implemented
 *
 * Notifies system (and applications) that something changed on volume.
 * FSD should call it each time volume status changes. 
 *
 * @param FileObject
 *        FileObject for the volume
 *
 * @param EventCode
 *        Event that occurs one the volume
 *
 * @return STATUS_SUCCESS if notification went well
 *
 * @remarks Only present in NT 5+.
 *
 *--*/
NTSTATUS
NTAPI
FsRtlNotifyVolumeEvent(IN PFILE_OBJECT FileObject,
                       IN ULONG EventCode)
{
    LPGUID Guid = NULL;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PDEVICE_OBJECT DeviceObject = NULL;
    TARGET_DEVICE_CUSTOM_NOTIFICATION Notification;

    /* FIXME: We should call IoGetRelatedTargetDevice here */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (DeviceObject)
    {
        Notification.Version = 1;
        Notification.Size = sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION);
        /* MSDN says that FileObject must be null
           when calling IoReportTargetDeviceChangeAsynchronous */
        Notification.FileObject = NULL;
        Notification.NameBufferOffset = -1;
        /* Find the good GUID associated with the event */
        switch (EventCode)
        {
            case FSRTL_VOLUME_DISMOUNT:
            {
                Guid = (LPGUID)&GUID_IO_VOLUME_DISMOUNT;
                break;
            }
            case FSRTL_VOLUME_DISMOUNT_FAILED:
            {
                Guid = (LPGUID)&GUID_IO_VOLUME_DISMOUNT_FAILED;
                break;
            }
            case FSRTL_VOLUME_LOCK:
            {
                Guid = (LPGUID)&GUID_IO_VOLUME_LOCK;
                break;
            }
            case FSRTL_VOLUME_LOCK_FAILED:
            {
                Guid = (LPGUID)&GUID_IO_VOLUME_LOCK_FAILED;
                break;
            }
            case FSRTL_VOLUME_MOUNT:
            {
                Guid = (LPGUID)&GUID_IO_VOLUME_MOUNT;
                break;
            }
            case FSRTL_VOLUME_UNLOCK:
            {
                Guid = (LPGUID)&GUID_IO_VOLUME_UNLOCK;
                break;
            }
        }
        if (Guid)
        {
            /* Copy GUID to notification structure and then report the change */
            RtlCopyMemory(&(Notification.Event), Guid, sizeof(GUID));

            if (EventCode == FSRTL_VOLUME_MOUNT)
            {
                IoReportTargetDeviceChangeAsynchronous(DeviceObject,
                                                       &Notification,
                                                       NULL,
                                                       NULL);
            }
            else
            {
                IoReportTargetDeviceChange(DeviceObject,
                                           &Notification);
            }

            Status = STATUS_SUCCESS;
        }
        ObfDereferenceObject(DeviceObject);
    }
    return Status;
}
