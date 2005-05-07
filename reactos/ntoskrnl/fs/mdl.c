/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/fs/mdl.c
 * PURPOSE:         Cached MDL Access Helper Routines for File System Drivers
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern ULONG CcFastReadResourceMiss;
extern ULONG CcFastReadNoWait;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
FsRtlIncrementCcFastReadResourceMiss( VOID )
{
    CcFastReadResourceMiss++;
}

/*
 * @implemented
 */
VOID
STDCALL
FsRtlIncrementCcFastReadNotPossible( VOID )
{
    CcFastReadNotPossible++;
}

/*
 * @implemented
 */
VOID
STDCALL
FsRtlIncrementCcFastReadWait( VOID )
{
    CcFastReadWait++;
}

/*
 * @implemented
 */
VOID
STDCALL
FsRtlIncrementCcFastReadNoWait( VOID )
{
    CcFastReadNoWait++;
}

/*
 * NAME    EXPORTED
 * FsRtlMdlRead@24
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlMdlRead(IN PFILE_OBJECT FileObject,
             IN PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             IN ULONG LockKey,
             OUT PMDL *MdlChain,
             OUT PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    
    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;
    
    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlRead)
    {
        /* Use the fast path */
        return FastDispatch->MdlRead(FileObject,
                                     FileOffset,
                                     Length,
                                     LockKey,
                                     MdlChain,
                                     IoStatus,
                                     DeviceObject);
    }
    
    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;
    
    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->MdlRead && 
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }
    
    /* No fast path, use slow path */
    return FsRtlMdlReadDev(FileObject,
                           FileOffset,
                           Length,
                           LockKey,
                           MdlChain,
                           IoStatus,
                           DeviceObject);
}

/*
 * NAME    EXPORTED
 * FsRtlMdlReadComplete@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
BOOLEAN 
STDCALL
FsRtlMdlReadComplete(IN PFILE_OBJECT FileObject,
                     IN OUT PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    
    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;
    
    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlReadComplete)
    {
        /* Use the fast path */
        return FastDispatch->MdlReadComplete(FileObject,
                                             MdlChain,
                                             DeviceObject);
    }
    
    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;
    
    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->MdlReadComplete && 
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }
    
    /* No fast path, use slow path */
    return FsRtlMdlReadCompleteDev(FileObject, MdlChain, DeviceObject);
}


/*
 * NAME    EXPORTED
 * FsRtlMdlReadCompleteDev@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * From Bo Branten's ntifs.h v13.
 * (CcMdlReadCompleteDev declared in internal/cc.h)
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlMdlReadCompleteDev(IN PFILE_OBJECT FileObject,
                        IN PMDL MdlChain,
                        IN PDEVICE_OBJECT DeviceObject)
{
    /* Call the Cache Manager */
    CcMdlReadCompleteDev(MdlChain, FileObject);
    return TRUE;
}

/*
 * NAME    EXPORTED
 * FsRtlMdlReadDev@28
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlMdlReadDev(IN PFILE_OBJECT FileObject,
                IN PLARGE_INTEGER FileOffset,
                IN ULONG Length,
                IN ULONG LockKey,
                OUT PMDL *MdlChain,
                OUT PIO_STATUS_BLOCK IoStatus,
                IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * NAME    EXPORTED
 * FsRtlMdlWriteComplete@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlMdlWriteComplete(IN PFILE_OBJECT FileObject,
                      IN PLARGE_INTEGER FileOffset,
                      IN PMDL MdlChain)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    
    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;
    
    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->MdlWriteComplete)
    {
        /* Use the fast path */
        return FastDispatch->MdlWriteComplete(FileObject,
                                              FileOffset,
                                              MdlChain,
                                              DeviceObject);
    }
    
    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;
    
    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->MdlWriteComplete && 
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }
    
    /* No fast path, use slow path */
    return FsRtlMdlWriteCompleteDev(FileObject, 
                                    FileOffset, 
                                    MdlChain, 
                                    DeviceObject);
}

/*
 * NAME EXPORTED
 * FsRtlMdlWriteCompleteDev@16
 *
 * DESCRIPTION
 * 
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlMdlWriteCompleteDev(IN PFILE_OBJECT FileObject,
                         IN PLARGE_INTEGER FileOffset,
                         IN PMDL MdlChain,
                         IN PDEVICE_OBJECT DeviceObject)
{
    /* Call the Cache Manager */
    CcMdlWriteCompleteDev(FileOffset, MdlChain, FileObject);
    return TRUE;
}


/*
 * NAME    EXPORTED
 * FsRtlPrepareMdlWrite@24
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlPrepareMdlWrite(IN  PFILE_OBJECT FileObject,
                     IN  PLARGE_INTEGER FileOffset,
                     IN  ULONG Length,
                     IN  ULONG LockKey,
                     OUT PMDL *MdlChain,
                     OUT PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_OBJECT DeviceObject, BaseDeviceObject;
    PFAST_IO_DISPATCH FastDispatch;
    
    /* Get Device Object and Fast Calls */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    FastDispatch = DeviceObject->DriverObject->FastIoDispatch;
    
    /* Check if we support Fast Calls, and check this one */
    if (FastDispatch && FastDispatch->PrepareMdlWrite)
    {
        /* Use the fast path */
        return FastDispatch->PrepareMdlWrite(FileObject,
                                             FileOffset,
                                             Length,
                                             LockKey,
                                             MdlChain,
                                             IoStatus,
                                             DeviceObject);
    }
    
    /* Get the Base File System (Volume) and Fast Calls */
    BaseDeviceObject = IoGetBaseFileSystemDeviceObject(FileObject);
    FastDispatch = BaseDeviceObject->DriverObject->FastIoDispatch;
    
    /* If the Base Device Object has its own FastDispatch Routine, fail */
    if (FastDispatch && FastDispatch->PrepareMdlWrite && 
        BaseDeviceObject != DeviceObject)
    {
        return FALSE;
    }
    
    /* No fast path, use slow path */
    return FsRtlPrepareMdlWriteDev(FileObject,
                                   FileOffset,
                                   Length,
                                   LockKey,
                                   MdlChain,
                                   IoStatus,
                                   DeviceObject);
}

/*
 * NAME EXPORTED
 * FsRtlPrepareMdlWriteDev@28
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlPrepareMdlWriteDev(IN  PFILE_OBJECT FileObject,
                        IN  PLARGE_INTEGER FileOffset,
                        IN  ULONG Length,
                        IN  ULONG LockKey,
                        OUT PMDL *MdlChain,
                        OUT PIO_STATUS_BLOCK IoStatus,
                        IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}


/* EOF */
