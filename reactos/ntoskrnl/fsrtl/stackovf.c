/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/stackovf.c
 * PURPOSE:         Provides Stack Overflow support for File System Drivers
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlPostPagingFileStackOverflow
 * @unimplemented NT 4.0
 *
 *     The FsRtlPostPagingFileStackOverflow routine
 *
 * @param Context
 *
 * @param Event
 *
 * @param StackOverflowRoutine
 *
 * @return
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
FsRtlPostPagingFileStackOverflow(IN PVOID Context,
                                 IN PKEVENT Event,
                                 IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine)
{
    UNIMPLEMENTED;
}

/*++
 * @name FsRtlPostStackOverflow
 * @unimplemented NT 4.0
 *
 *     The FsRtlPostStackOverflow routine
 *
 * @param Context
 *
 * @param Event
 *
 * @param StackOverflowRoutine
 *
 * @return
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
FsRtlPostStackOverflow(IN PVOID Context,
                       IN PKEVENT Event,
                       IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine)
{
    KEBUGCHECK(0);
}

/* EOF */
