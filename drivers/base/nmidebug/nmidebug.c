/*
 * PROJECT:         ReactOS NMI Debug Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/base/nmidebug/nmidebug.c
 * PURPOSE:         Driver Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ntndk.h>

/* FUNCTIONS ******************************************************************/

PCHAR NmiBegin = "NMI2NMI1";

BOOLEAN
NTAPI
NmiDbgCallback(IN PVOID Context,
               IN BOOLEAN Handled)
{
    /* Clear the NMI flag */
    ((PCHAR)&KiBugCheckData[4])[3] -= NmiBegin[3];

    /* Get NMI status signature */
    __indwordstring(0x80, (PULONG)NmiBegin, 1);
    ((void(*)())&KiBugCheckData[4])();

    /* Handle the NMI safely */
    KiEnableTimerWatchdog = strcmp(NmiBegin, NmiBegin + 4);
    return TRUE;
}
     
NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PAGED_CODE();

    /* Register NMI callback */
    KeRegisterNmiCallback(&NmiDbgCallback, NULL);

    /* Return success */
    return STATUS_SUCCESS;
}

/* EOF */
