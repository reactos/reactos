/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/filtrctx.c
 * PURPOSE:         File Stream Filter Context support for File System Drivers
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlIsPagingFile
 * @implemented NT 4.0
 *
 *     The FsRtlIsPagingFile routine checks if the FileObject is a Paging File.
 *
 * @param FileObject
 *        A pointer to the File Object to be tested.
 *
 * @return TRUE if the File is a Paging File, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
LOGICAL
NTAPI
FsRtlIsPagingFile(IN PFILE_OBJECT FileObject)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @unimplemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlLookupPerStreamContextInternal(IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
                                    IN PVOID OwnerId OPTIONAL,
                                    IN PVOID InstanceId OPTIONAL)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @unimplemented
 */
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlLookupPerFileObjectContext(IN PFILE_OBJECT FileObject,
                                IN PVOID OwnerId OPTIONAL,
                                IN PVOID InstanceId OPTIONAL)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
                            IN PFSRTL_PER_STREAM_CONTEXT Ptr)
{
    KEBUGCHECK(0);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlRemovePerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
                            IN PVOID OwnerId OPTIONAL,
                            IN PVOID InstanceId OPTIONAL)
{
    KEBUGCHECK(0);
    return NULL;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerFileObjectContext(IN PFILE_OBJECT FileObject,
                                IN PFSRTL_PER_FILEOBJECT_CONTEXT Ptr)
{
    KEBUGCHECK(0);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlRemovePerFileObjectContext(IN PFILE_OBJECT PerFileObjectContext,
                                IN PVOID OwnerId OPTIONAL,
                                IN PVOID InstanceId OPTIONAL)
{
    KEBUGCHECK(0);
    return NULL;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlTeardownPerStreamContexts(IN PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader)
{
    KEBUGCHECK(0);
}

