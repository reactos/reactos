/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/context.c
 * PURPOSE:         File and Stream Context Functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* PRIVATE FUNCTIONS**********************************************************/

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
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
STDCALL
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
STDCALL
FsRtlInsertPerFileObjectContext(IN PFSRTL_ADVANCED_FCB_HEADER PerFileObjectContext,
                                IN PVOID /* PFSRTL_PER_FILE_OBJECT_CONTEXT*/ Ptr)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID /* PFSRTL_PER_FILE_OBJECT_CONTEXT*/
STDCALL
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
STDCALL
FsRtlTeardownPerStreamContexts(IN PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader)
{
    UNIMPLEMENTED;
}

/* EOF */
