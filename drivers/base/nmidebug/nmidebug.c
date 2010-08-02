/*
 * PROJECT:         ReactOS NMI Debug Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/base/nmidebug/nmidebug.c
 * PURPOSE:         Driver Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntddk.h>

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
NmiDbgCallback(IN PVOID Context,
               IN BOOLEAN Handled)
{
    //
    // Let the user know we are alive
    //         
    DbgPrint("NMI Callback entered! Letting the system crash...\n");

    //
    // Do not handle the NMI
    //
    return FALSE;
}
     
NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PAGED_CODE();

    //
    // Register NMI callback
    //
    KeRegisterNmiCallback(&NmiDbgCallback, NULL);

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/* EOF */
