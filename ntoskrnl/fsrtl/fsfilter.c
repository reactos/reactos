/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/fsfilter.c
 * PURPOSE:         Provides support for the Filter Manager
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlCreateSectionForDataScan
 * @unimplemented
 *
 * FILLME
 *
 * @param SectionHandle
 *        FILLME
 *
 * @param SectionObject
 *        FILLME
 *
 * @param SectionFileSize
 *        FILLME
 *
 * @param FileObject
 *        FILLME
 *
 * @param DesiredAccess
 *        FILLME
 *
 * @param ObjectAttributes
 *        FILLME
 *
 * @param MaximumSize
 *        FILLME
 *
 * @param SectionPageProtection
 *        FILLME
 *
 * @param AllocationAttributes
 *        FILLME
 *
 * @param Flags
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlCreateSectionForDataScan(OUT PHANDLE SectionHandle,
                              OUT PVOID *SectionObject,
                              OUT PLARGE_INTEGER SectionFileSize OPTIONAL,
                              IN PFILE_OBJECT FileObject,
                              IN ACCESS_MASK DesiredAccess,
                              IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                              IN PLARGE_INTEGER MaximumSize OPTIONAL,
                              IN ULONG SectionPageProtection,
                              IN ULONG AllocationAttributes,
                              IN ULONG Flags)
{
    /* Unimplemented */
    KeBugCheck(FILE_SYSTEM);
    return STATUS_NOT_IMPLEMENTED;
}

