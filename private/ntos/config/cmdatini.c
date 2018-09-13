/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

   cmdatini.c

Abstract:

   contains code to init static STRING structures for registry name space.

Author:

    Andre Vachon (andreva) 08-Apr-1992


Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpInitializeRegistryNames)
#endif

extern UNICODE_STRING CmRegistryRootName;
extern UNICODE_STRING CmRegistryMachineName;
extern UNICODE_STRING CmRegistryMachineHardwareName;
extern UNICODE_STRING CmRegistryMachineHardwareDescriptionName;
extern UNICODE_STRING CmRegistryMachineHardwareDescriptionSystemName;
extern UNICODE_STRING CmRegistryMachineHardwareDeviceMapName;
extern UNICODE_STRING CmRegistryMachineHardwareResourceMapName;
extern UNICODE_STRING CmRegistryMachineHardwareOwnerMapName;
extern UNICODE_STRING CmRegistryMachineSystemName;
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSet;
extern UNICODE_STRING CmRegistryUserName;
extern UNICODE_STRING CmRegistrySystemCloneName;
extern UNICODE_STRING CmpSystemFileName;
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumName;
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumRootName;
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetServices;
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetHardwareProfilesCurrent;
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlClass;
extern UNICODE_STRING CmSymbolicLinkValueName;

#ifdef _WANT_MACHINE_IDENTIFICATION
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlBiosInfo;
#endif

extern PWCHAR CmpRegistryRootString;
extern PWCHAR CmpRegistryMachineString;
extern PWCHAR CmpRegistryMachineHardwareString;
extern PWCHAR CmpRegistryMachineHardwareDescriptionString;
extern PWCHAR CmpRegistryMachineHardwareDescriptionSystemString;
extern PWCHAR CmpRegistryMachineHardwareDeviceMapString;
extern PWCHAR CmpRegistryMachineHardwareResourceMapString;
extern PWCHAR CmpRegistryMachineHardwareOwnerMapString;
extern PWCHAR CmpRegistryMachineSystemString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetString;
extern PWCHAR CmpRegistryUserString;
extern PWCHAR CmpRegistrySystemCloneString;
extern PWCHAR CmpRegistrySystemFileNameString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetEnumString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetEnumRootString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetServicesString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetHardwareProfilesCurrentString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetControlClassString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetControlSafeBootString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagementString;

extern PWCHAR CmpRegistryMachineSystemCurrentControlSetControlBootLogString;
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetServicesEventLogString;
extern PWCHAR CmpSymbolicLinkValueName;

#ifdef _WANT_MACHINE_IDENTIFICATION
extern PWCHAR CmpRegistryMachineSystemCurrentControlSetControlBiosInfoString;
#endif



VOID
CmpInitializeRegistryNames(
VOID
)

/*++

Routine Description:

    This routine creates all the Unicode strings for the various names used
    in and by the registry

Arguments:

    None.

Returns:

    None.

--*/
{
    ULONG i;

    RtlInitUnicodeString( &CmRegistryRootName,
                          CmpRegistryRootString );

    RtlInitUnicodeString( &CmRegistryMachineName,
                          CmpRegistryMachineString );

    RtlInitUnicodeString( &CmRegistryMachineHardwareName,
                          CmpRegistryMachineHardwareString );

    RtlInitUnicodeString( &CmRegistryMachineHardwareDescriptionName,
                          CmpRegistryMachineHardwareDescriptionString );

    RtlInitUnicodeString( &CmRegistryMachineHardwareDescriptionSystemName,
                          CmpRegistryMachineHardwareDescriptionSystemString );

    RtlInitUnicodeString( &CmRegistryMachineHardwareDeviceMapName,
                          CmpRegistryMachineHardwareDeviceMapString );

    RtlInitUnicodeString( &CmRegistryMachineHardwareResourceMapName,
                          CmpRegistryMachineHardwareResourceMapString );

    RtlInitUnicodeString( &CmRegistryMachineHardwareOwnerMapName,
                          CmpRegistryMachineHardwareOwnerMapString );

    RtlInitUnicodeString( &CmRegistryMachineSystemName,
                          CmpRegistryMachineSystemString );

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSet,
                          CmpRegistryMachineSystemCurrentControlSetString);

    RtlInitUnicodeString( &CmRegistryUserName,
                          CmpRegistryUserString );

    RtlInitUnicodeString( &CmRegistrySystemCloneName,
                          CmpRegistrySystemCloneString );

    RtlInitUnicodeString( &CmpSystemFileName,
                          CmpRegistrySystemFileNameString );

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetEnumName,
                          CmpRegistryMachineSystemCurrentControlSetEnumString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetEnumRootName,
                          CmpRegistryMachineSystemCurrentControlSetEnumRootString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetServices,
                          CmpRegistryMachineSystemCurrentControlSetServicesString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetHardwareProfilesCurrent,
                          CmpRegistryMachineSystemCurrentControlSetHardwareProfilesCurrentString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetControlClass,
                          CmpRegistryMachineSystemCurrentControlSetControlClassString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetControlSafeBoot,
                          CmpRegistryMachineSystemCurrentControlSetControlSafeBootString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagement,
                          CmpRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagementString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetControlBootLog,
                          CmpRegistryMachineSystemCurrentControlSetControlBootLogString);

    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetServicesEventLog,
                          CmpRegistryMachineSystemCurrentControlSetServicesEventLogString);

    RtlInitUnicodeString( &CmSymbolicLinkValueName,
                          CmpSymbolicLinkValueName);

#ifdef _WANT_MACHINE_IDENTIFICATION
    RtlInitUnicodeString( &CmRegistryMachineSystemCurrentControlSetControlBiosInfo,
                          CmpRegistryMachineSystemCurrentControlSetControlBiosInfoString);
#endif

    //
    // Initialize the type names for the hardware tree.
    //

    for (i = 0; i <= MaximumType; i++) {

        RtlInitUnicodeString( &(CmTypeName[i]),
                              CmTypeString[i] );

    }

    //
    // Initialize the class names for the hardware tree.
    //

    for (i = 0; i <= MaximumClass; i++) {

        RtlInitUnicodeString( &(CmClassName[i]),
                              CmClassString[i] );

    }

    return;
}
