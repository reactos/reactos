/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    init.c

Abstract:

    This module is responsible to build any mips specific entries in
    the hardware tree of registry which the arc environment doesn't
    normally provide for.

Author:

    Ken Reneris (kenr) 04-Aug-1992


Environment:

    Kernel mode.

Revision History:

    Nigel Haslock 10-Oct-1995
        Set up firmware version and possibly date in the registry.

--*/

#include "cmp.h"

#define TITLE_INDEX_VALUE 0

NTSTATUS
CmpInitializeMachineDependentConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine creates alpha specific entries in the registry.

Arguments:

    LoaderBlock - supplies a pointer to the LoaderBlock passed in from the
                  OS Loader.

Returns:

    NTSTATUS code for sucess or reason of failure.

--*/

{

    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING ValueData;
    ANSI_STRING AnsiString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParentHandle;

    InitializeObjectAttributes(&ObjectAttributes,
				               &CmRegistryMachineHardwareDescriptionSystemName,
				               OBJ_CASE_INSENSITIVE,
				               NULL,
				               NULL);

    Status = NtOpenKey(&ParentHandle,
		               KEY_READ,
		               &ObjectAttributes);

    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&ValueName,
    			             L"SystemBiosVersion");

        RtlInitAnsiString(&AnsiString,
    		              &LoaderBlock->u.Alpha.FirmwareVersion[0]);

        RtlAnsiStringToUnicodeString(&ValueData,
    				                 &AnsiString,
    				                 TRUE);

        Status = NtSetValueKey(ParentHandle,
    			               &ValueName,
    			               TITLE_INDEX_VALUE,
    			               REG_SZ,
    			               ValueData.Buffer,
    			               ValueData.Length + sizeof(UNICODE_NULL));

        RtlFreeUnicodeString(&ValueData);

        //
        // If the firmware build number is included in the loader block,
        // then store it in the registry.
        //

        if (LoaderBlock->u.Alpha.FirmwareBuildTimeStamp[0] != 0 ) {
            RtlInitUnicodeString(&ValueName,
                                 L"SystemBiosDate");

            RtlInitAnsiString(&AnsiString,
                              &LoaderBlock->u.Alpha.FirmwareBuildTimeStamp[0]);

            RtlAnsiStringToUnicodeString(&ValueData,
        				                 &AnsiString,
        				                 TRUE);

            Status = NtSetValueKey(ParentHandle,
        			               &ValueName,
        			               TITLE_INDEX_VALUE,
        			               REG_SZ,
        			               ValueData.Buffer,
        			               ValueData.Length + sizeof(UNICODE_NULL));

            RtlFreeUnicodeString(&ValueData);
        }
    }

    return STATUS_SUCCESS;
}
