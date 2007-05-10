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

/* GLOBALS *******************************************************************/

PCHAR CmpID1 = "80%u86-%c%x";
PCHAR CmpID2 = "x86 Family %u Model %u Stepping %u";

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitializeMachineDependentConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNICODE_STRING KeyName, ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG HavePae;
    NTSTATUS Status;
    HANDLE KeyHandle, BiosHandle, SystemHandle, FpuHandle;
    ULONG Disposition;
    CONFIGURATION_COMPONENT_DATA ConfigData;
    CHAR Buffer[128];
    ULONG i;
    PKPRCB Prcb;
    USHORT IndexTable[MaximumType + 1] = {0};

    /* Open the SMSS Memory Management key */
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

    /* Open the hardware description key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\Description\\System");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&SystemHandle, KEY_READ | KEY_WRITE, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the BIOS Information key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\"
                         L"Control\\BIOSINFO");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&BiosHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status) && !ExpInTextModeSetup) return Status;

    /* Create the CPU Key, and check if it already existed */
    RtlInitUnicodeString(&KeyName, L"CentralProcessor");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               SystemHandle,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    NtClose(KeyHandle);

    /* This key -never- exists on x86 machines, except in ReactOS! */
    //if (Disposition == REG_CREATED_NEW_KEY)
    {
        /* Allocate the configuration data for cmconfig.c */
        CmpConfigurationData = ExAllocatePoolWithTag(PagedPool,
                                                     CmpConfigurationAreaSize,
                                                     TAG_CM);
        if (!CmpConfigurationData) return STATUS_INSUFFICIENT_RESOURCES;

        /* Loop all CPUs */
        for (i = 0; i < KeNumberProcessors; i++)
        {
            /* Get the PRCB */
            Prcb = KiProcessorBlock[i];

            /* Setup the Configuration Entry for the Processor */
            RtlZeroMemory(&ConfigData, sizeof (ConfigData));
            ConfigData.ComponentEntry.Class = ProcessorClass;
            ConfigData.ComponentEntry.Type = CentralProcessor;
            ConfigData.ComponentEntry.Key = i;
            ConfigData.ComponentEntry.AffinityMask = AFFINITY_MASK(i);
            ConfigData.ComponentEntry.Identifier = Buffer;

            /* Check if the CPU doesn't support CPUID */
            if (!Prcb->CpuID)
            {
                /* Build ID1-style string for older CPUs */
                sprintf(Buffer,
                        CmpID1,
                        Prcb->CpuType,
                        (Prcb->CpuStep >> 8) + 'A',
                        Prcb->CpuStep & 0xff);
            }
            else
            {
                /* Build ID2-style string for newer CPUs */
                sprintf(Buffer,
                        CmpID2,
                        Prcb->CpuType,
                        (Prcb->CpuStep >> 8),
                        Prcb->CpuStep & 0xff);
            }

            /* Save the ID string length now that we've created it */
            ConfigData.ComponentEntry.IdentifierLength = strlen(Buffer) + 1;

            /* Initialize the registry configuration node for it */
            Status = CmpInitializeRegistryNode(&ConfigData,
                                               SystemHandle,
                                               &KeyHandle,
                                               InterfaceTypeUndefined,
                                               0xFFFFFFFF,
                                               IndexTable);
            if (!NT_SUCCESS(Status)) return(Status);

            /* Check if we have an FPU */
            if (KeI386NpxPresent)
            {
                /* Setup the Configuration Entry for the FPU */
                RtlZeroMemory(&ConfigData, sizeof(ConfigData));
                ConfigData.ComponentEntry.Class = ProcessorClass;
                ConfigData.ComponentEntry.Type = FloatingPointProcessor;
                ConfigData.ComponentEntry.Key = i;
                ConfigData.ComponentEntry.AffinityMask = AFFINITY_MASK(i);
                ConfigData.ComponentEntry.Identifier = Buffer;

                /* For 386 cpus, the CPU String is the identifier */
                if (Prcb->CpuType == 3) strcpy(Buffer, "80387");

                /* Save the ID string length now that we've created it */
                ConfigData.ComponentEntry.IdentifierLength = strlen(Buffer) + 1;

                /* Initialize the registry configuration node for it */
                Status = CmpInitializeRegistryNode(&ConfigData,
                                                   SystemHandle,
                                                   &FpuHandle,
                                                   InterfaceTypeUndefined,
                                                   0xFFFFFFFF,
                                                   IndexTable);
                if (!NT_SUCCESS(Status))
                {
                    /* Failed, close the CPU handle and return */
                    NtClose(KeyHandle);
                    return Status;
                }

                /* Close this new handle */
                NtClose(FpuHandle);

                /* Check if we have features bits */
                if (Prcb->FeatureBits)
                {
                    /* Add them to the registry */
                    RtlInitUnicodeString(&ValueName, L"FeatureSet");
                    Status = NtSetValueKey(KeyHandle,
                                           &ValueName,
                                           0,
                                           REG_DWORD,
                                           &Prcb->FeatureBits,
                                           sizeof(Prcb->FeatureBits));
                }

                /* Check if we detected the CPU Speed */
                if (Prcb->MHz)
                {
                    /* Add it to the registry */
                    RtlInitUnicodeString(&ValueName, L"~MHz");
                    Status = NtSetValueKey(KeyHandle,
                                           &ValueName,
                                           0,
                                           REG_DWORD,
                                           &Prcb->MHz,
                                           sizeof(Prcb->MHz));
                }

                /* Check if we have an update signature */
                if (Prcb->UpdateSignature.QuadPart)
                {
                    /* Add it to the registry */
                    RtlInitUnicodeString(&ValueName, L"Update Signature");
                    Status = NtSetValueKey(KeyHandle,
                                           &ValueName,
                                           0,
                                           REG_BINARY,
                                           &Prcb->UpdateSignature,
                                           sizeof(Prcb->UpdateSignature));
                }

                /* Close the processor handle */
                NtClose(KeyHandle);
            }
        }

        /* Free the configuration data */
        ExFreePool(CmpConfigurationData);
    }

    /* All done*/
    return STATUS_SUCCESS;
}
