/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/dbgctrl.c
 * PURPOSE:         System debug control
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*++
 * @name NtSystemDebugControl
 * @implemented
 *
 * Perform various queries to debugger.
 * This API is subject to test-case creation to further evaluate its
 * abilities (if needed to at all)
 *
 * See: http://www.osronline.com/showthread.cfm?link=93915
 *      http://void.ru/files/Ntexapi.h
 *      http://www.codeguru.com/code/legacy/system/ntexapi.zip
 *      http://www.securityfocus.com/bid/9694
 *
 * @param ControlCode
 *        Description of the parameter. Wrapped to more lines on ~70th
 *        column.
 *
 * @param InputBuffer
 *        FILLME
 *
 * @param InputBufferLength
 *        FILLME
 *
 * @param OutputBuffer
 *        FILLME
 *
 * @param OutputBufferLength
 *        FILLME
 *
  * @param ReturnLength
 *        FILLME
 *
 * @return STATUS_SUCCESS in case of success, proper error code otherwise
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
NtSystemDebugControl(SYSDBG_COMMAND ControlCode,
                     PVOID InputBuffer,
                     ULONG InputBufferLength,
                     PVOID OutputBuffer,
                     ULONG OutputBufferLength,
                     PULONG ReturnLength)
{
    /* FIXME: TODO */
    return STATUS_SUCCESS;
}
