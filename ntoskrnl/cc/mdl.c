/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/mdl.c
 * PURPOSE:         Implements MDL Cache Manager Functions
 *
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
CcMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    OUT PMDL * MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%I64d Length=%lu\n",
        FileObject, FileOffset->QuadPart, Length);

    UNIMPLEMENTED;
}

/*
 * NAME                            INTERNAL
 * CcMdlReadComplete2@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * MdlChain
 * DeviceObject
 *
 * RETURN VALUE
 *  None.
 *
 * NOTE
 *     Used by CcMdlReadComplete@8 and FsRtl
 *
 */
VOID
NTAPI
CcMdlReadComplete2 (
    IN PFILE_OBJECT FileObject,
    IN PMDL MemoryDescriptorList
)
{
    PMDL Mdl;

    /* Free MDLs */
    while ((Mdl = MemoryDescriptorList))
    {
        MemoryDescriptorList = Mdl->Next;
        MmUnlockPages(Mdl);
        IoFreeMdl(Mdl);
    }
}

/*
 * NAME    EXPORTED
 * CcMdlReadComplete@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * None.
 *
 * NOTE
 * From Bo Branten's ntifs.h v13.
 *
 * @implemented
 */
VOID
NTAPI
CcMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject = NULL;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Fast Dispatch Data */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlReadComplete)
    {
        /* Use the fast path */
        if (FastDispatch->MdlReadComplete(FileObject,
                                          MdlChain,
                                          DeviceObject))
        {
            /* Request was handled */
            return;
        }
    }

    /* Use slow path */
    CcMdlReadComplete2(FileObject, MdlChain);
}

/*
 * @implemented
 */
VOID
NTAPI
CcMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject = NULL;
    PFAST_IO_DISPATCH FastDispatch;

    /* Get Fast Dispatch Data */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;

    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlWriteComplete)
    {
        /* Use the fast path */
        if (FastDispatch->MdlWriteComplete(FileObject,
                                           FileOffset,
                                           MdlChain,
                                           DeviceObject))
        {
            /* Request was handled */
            return;
        }
    }

    /* Use slow path */
    CcMdlWriteComplete2(FileObject,FileOffset, MdlChain);
}

VOID
NTAPI
CcMdlWriteComplete2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcMdlWriteAbort (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    OUT PMDL * MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus)
{
    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%I64d Length=%lu\n",
        FileObject, FileOffset->QuadPart, Length);

    UNIMPLEMENTED;
}
