/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fs/context.c
 * PURPOSE:         File and Stream Context Functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* PRIVATE FUNCTIONS**********************************************************/

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(IN PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
                            IN PFSRTL_PER_STREAM_CONTEXT Ptr)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlRemovePerFileObjectContext(IN PFSRTL_ADVANCED_FCB_HEADER PerFileObjectContext,
                                IN PVOID OwnerId OPTIONAL,
                                IN PVOID InstanceId OPTIONAL)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlTeardownPerStreamContexts(IN PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader)
{
    UNIMPLEMENTED;
}

/* EOF */
