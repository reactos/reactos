/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/ms/fsctrl.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "msfs.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS DEFAULTAPI
MsfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
                      PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    NTSTATUS Status;

    DPRINT1("MsfsFileSystemControl(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;

    DPRINT1("Mailslot name: %wZ\n", &Fcb->Name);

    switch (IoStack->Parameters.FileSystemControl.FsControlCode)
    {
#if 0

#endif
    default:
        Status = STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/* EOF */
