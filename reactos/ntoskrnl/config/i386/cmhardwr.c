/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/i386/cmhardwr.c
 * PURPOSE:         Configuration Manager - Hardware-Specific Code
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "../cm.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitializeMachineDependentConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNICODE_STRING KeyName, ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG HavePae;
    NTSTATUS Status;
    HANDLE KeyHandle;

    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\"
                         L"Control\\Session Manager\\Memory Management");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, KEY_READ | KEY_WRITE, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Detect if PAE is enabled */
        HavePae = SharedUserData->ProcessorFeatures[PF_PAE_ENABLED];

        /* Set the value */
        RtlInitUnicodeString(&ValueName, L"PhysicalAddressExtension");
        NtSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_DWORD,
                      &HavePae,
                      sizeof(HavePae));

        /* Close the key */
        NtClose(KeyHandle);
    }

    /* All done*/
    return STATUS_SUCCESS;
}
