/*
 * PROJECT:         ReactOS NMI Debug Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Driver Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/ketypes.h>

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
NmiDbgCallback(
    _In_opt_ PVOID Context,
    _In_ BOOLEAN Handled)
{
    DbgPrint("NMI Callback entered!\n");

    /* We can add any other debugging information for a specific platform as needed */

    /* Do not handle the NMI */
    return FALSE;
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    PAGED_CODE();

    /* Register NMI callback */
    KeRegisterNmiCallback(&NmiDbgCallback, NULL);

    /* Return success */
    return STATUS_SUCCESS;
}

/* EOF */
