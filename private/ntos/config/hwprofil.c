/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    hwprofil.c

Abstract:

    This module contains support for changing the Hardware profile
    based on the current docking state, either at boot time or by
    ACPI dock.

Author:

    Kenneth D. Ray (kenray) Jan 1998

Revision History:

--*/

#include "cmp.h"

NTSTATUS
CmDeleteKeyRecursive(
    HANDLE  hKeyRoot,
    PWSTR   Key,
    PVOID   TemporaryBuffer,
    ULONG   LengthTemporaryBuffer,
    BOOLEAN ThisKeyToo
    );

NTSTATUS
CmpGetAcpiProfileInformation (
    IN  HANDLE  IDConfigDB,
    OUT PCM_HARDWARE_PROFILE_LIST * ProfileList,
    OUT PCM_HARDWARE_PROFILE_ACPI_ALIAS_LIST * AliasList,
    IN  PWCHAR  NameBuffer,
    IN  PUCHAR  ValueBuffer,
    IN  ULONG   Len
    );

NTSTATUS
CmpFilterAcpiDockingState (
    IN     PPROFILE_ACPI_DOCKING_STATE  NewDockState,
    IN     ULONG                        CurrentDockingState,
    IN     PWCHAR                       CurrentAcpiSN,
    IN     ULONG                        CurrentProfileNumber,
    IN OUT PCM_HARDWARE_PROFILE_LIST    ProfileList,
    IN OUT PCM_HARDWARE_PROFILE_ACPI_ALIAS_LIST AliasList
    );

NTSTATUS
CmpMoveBiosAliasTable (
    IN HANDLE   IDConfigDB,
    IN HANDLE   CurrentInfo,
    IN ULONG    CurrentProfileNumber,
    IN ULONG    NewProfileNumber,
    IN PWCHAR   nameBuffer,
    IN PCHAR    valueBuffer,
    IN ULONG    bufferLen
    );

#pragma alloc_text(PAGE,CmpCloneHwProfile)
#pragma alloc_text(PAGE,CmSetAcpiHwProfile)
#pragma alloc_text(PAGE,CmpFilterAcpiDockingState)
#pragma alloc_text(PAGE,CmpGetAcpiProfileInformation)
#pragma alloc_text(PAGE,CmpAddAcpiAliasEntry)
#pragma alloc_text(PAGE,CmpMoveBiosAliasTable)
#pragma alloc_text(PAGE,CmpCreateHwProfileFriendlyName)

extern UNICODE_STRING  CmSymbolicLinkValueName;

NTSTATUS
CmpGetAcpiProfileInformation (
    IN  HANDLE  IDConfigDB,
    OUT PCM_HARDWARE_PROFILE_LIST * ProfileList,
    OUT PCM_HARDWARE_PROFILE_ACPI_ALIAS_LIST * AliasList,
    IN  PWCHAR  nameBuffer,
    IN  PUCHAR  valueBuffer,
    IN  ULONG   bufferLen
    )
/*++
Routine Description:

    Obtain the alias and hardware profile information from the registry.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    HANDLE      acpiAlias = NULL;
    HANDLE      profiles = NULL;
    HANDLE      entry = NULL;
    ULONG       len = 0;
    ULONG       i, j;
    OBJECT_ATTRIBUTES   attributes;
    UNICODE_STRING      name;
    KEY_FULL_INFORMATION        keyInfo;
    PKEY_VALUE_FULL_INFORMATION value;
    PKEY_BASIC_INFORMATION      basicInfo;

    PAGED_CODE ();

    *ProfileList = NULL;
    *AliasList = NULL;

    value = (PKEY_VALUE_FULL_INFORMATION) valueBuffer;
    basicInfo = (PKEY_BASIC_INFORMATION) valueBuffer;

    //
    // Open a handle to the Profile information
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_HARDWARE_PROFILES);
    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                IDConfigDB,
                                NULL);
    status = ZwOpenKey (&profiles,
                        KEY_READ,
                        &attributes);

    if (!NT_SUCCESS (status)) {
        profiles = NULL;
        goto Clean;
    }

    //
    // Find the number of profile Sub Keys
    //
    status = ZwQueryKey (profiles,
                         KeyFullInformation,
                         &keyInfo,
                         sizeof (keyInfo),
                         &len);

    if (!NT_SUCCESS (status)) {
        goto Clean;
    }

    ASSERT (0 < keyInfo.SubKeys);

    len = sizeof (CM_HARDWARE_PROFILE_LIST)
        + (sizeof (CM_HARDWARE_PROFILE) * (keyInfo.SubKeys - 1));

    * ProfileList = ExAllocatePool (PagedPool, len);
    if (NULL == *ProfileList) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean;
    }
    RtlZeroMemory (*ProfileList, len);

    (*ProfileList)->MaxProfileCount = keyInfo.SubKeys;
    (*ProfileList)->CurrentProfileCount = 0;

    //
    // Iterrate the profiles
    //
    for (i = 0; i < keyInfo.SubKeys; i++) {
        CM_HARDWARE_PROFILE TempProfile;
        UNICODE_STRING      KeyName;
        ULONG               realsize;

        //
        // Get the first key in the list.
        //
        status = ZwEnumerateKey (profiles,
                                 i,
                                 KeyBasicInformation,
                                 basicInfo,
                                 bufferLen - sizeof (UNICODE_NULL), // term 0
                                 &len);

        if (!NT_SUCCESS (status)) {
            //
            // This should never happen.
            //
            break;
        }

        basicInfo->Name [basicInfo->NameLength/sizeof(WCHAR)] = 0;
        name.Length = (USHORT) basicInfo->NameLength;
        name.MaximumLength = (USHORT) basicInfo->NameLength + sizeof (UNICODE_NULL);
        name.Buffer = basicInfo->Name;

        InitializeObjectAttributes (&attributes,
                                    &name,
                                    OBJ_CASE_INSENSITIVE,
                                    profiles,
                                    NULL);
        status = ZwOpenKey (&entry,
                            KEY_READ,
                            &attributes);
        if (!NT_SUCCESS (status)) {
            break;
        }

        //
        // Fill in the temporary profile structure with this
        // profile's data.
        //
        RtlUnicodeStringToInteger(&name, 0, &TempProfile.Id);

        //
        // Find the pref order of this entry.
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PREFERENCE_ORDER);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if ((!NT_SUCCESS (status)) || (value->Type != REG_DWORD)) {
            TempProfile.PreferenceOrder = -1;

        } else {
            TempProfile.PreferenceOrder
                = * (PULONG) ((PUCHAR) value + value->DataOffset);
        }

        //
        // Extract the friendly name
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_FRIENDLY_NAME);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (!NT_SUCCESS (status) || (value->Type != REG_SZ)) {
            WCHAR tmpname[] = L"------"; // as taken from cmboot.c
            ULONG len;
            PVOID buffer;

            len = sizeof (tmpname);
            buffer = ExAllocatePool (PagedPool, len);

            TempProfile.NameLength = len;
            TempProfile.FriendlyName = buffer;
            if (NULL == buffer) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                ZwClose (entry);
                goto Clean;
            }
            RtlCopyMemory (buffer, tmpname, value->DataLength);

        } else {
            PVOID buffer;

            buffer = ExAllocatePool (PagedPool, value->DataLength);
            TempProfile.NameLength = value->DataLength;
            TempProfile.FriendlyName = buffer;
            if (NULL == buffer) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                ZwClose (entry);
                goto Clean;
            }
            RtlCopyMemory (buffer,
                           (PUCHAR) value + value->DataOffset,
                           value->DataLength);
        }

        TempProfile.Flags = 0;
        //
        // Is this aliasable?
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ALIASABLE);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (NT_SUCCESS (status) && (value->Type == REG_DWORD)) {
            if (* (PULONG) ((PUCHAR) value + value->DataOffset)) {
                TempProfile.Flags |= CM_HP_FLAGS_ALIASABLE;
            }

        } else {
            TempProfile.Flags |= CM_HP_FLAGS_ALIASABLE;
        }

        //
        // Is this pristine?
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PRISTINE);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (NT_SUCCESS (status) && (value->Type == REG_DWORD)) {
            if (* (PULONG) ((PUCHAR) value + value->DataOffset)) {
                TempProfile.Flags = CM_HP_FLAGS_PRISTINE;
                // No other flags set;
            }
        }

        //
        // If we see a profile with the ID of zero (AKA an illegal)
        // ID for a hardware profile to possess, then we know that this
        // must be a pristine profile.
        //
        if (0 == TempProfile.Id) {
            TempProfile.Flags = CM_HP_FLAGS_PRISTINE;
            // NO other flags set.

            TempProfile.PreferenceOrder = -1; // move to the end of the list.
        }


        //
        // Insert this new profile into the appropriate spot in the
        // profile array. Entries are sorted by preference order.
        //
        for (j=0; j < (*ProfileList)->CurrentProfileCount; j++) {
            if ((*ProfileList)->Profile[j].PreferenceOrder >=
                TempProfile.PreferenceOrder) {

                //
                // Insert at position j.
                //
                RtlMoveMemory(&(*ProfileList)->Profile[j+1],
                              &(*ProfileList)->Profile[j],
                              sizeof(CM_HARDWARE_PROFILE) *
                              ((*ProfileList)->MaxProfileCount-j-1));
                break;
            }
        }
        (*ProfileList)->Profile[j] = TempProfile;
        ++(*ProfileList)->CurrentProfileCount;

        ZwClose (entry);
    }

    //
    // Open a handle to the ACPI Alias information
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ACPI_ALIAS);
    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                IDConfigDB,
                                NULL);
    status = ZwOpenKey (&acpiAlias,
                        KEY_READ,
                        &attributes);

    if (!NT_SUCCESS (status)) {
        //
        // So we don't have an alias table.  This is ok.
        //
        status = STATUS_SUCCESS;
        acpiAlias = NULL;
        goto Clean;
    }

    //
    // Find the number of Acpi Alias Sub Keys
    //
    status = ZwQueryKey (acpiAlias,
                         KeyFullInformation,
                         &keyInfo,
                         sizeof (keyInfo),
                         &len);

    if (!NT_SUCCESS (status)) {
        goto Clean;
    }


    ASSERT (0 < keyInfo.SubKeys);

    * AliasList = ExAllocatePool (
                        PagedPool,
                        sizeof (CM_HARDWARE_PROFILE_LIST) +
                        (sizeof (CM_HARDWARE_PROFILE) * (keyInfo.SubKeys - 1)));

    if (NULL == *AliasList) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean;
    }

    (*AliasList)->MaxAliasCount =
        (*AliasList)->CurrentAliasCount = keyInfo.SubKeys;

    //
    // Iterrate the alias entries
    //
    for (i = 0; i < keyInfo.SubKeys; i++) {

        //
        // Get the first key in the list.
        //
        status = ZwEnumerateKey (acpiAlias,
                                 i,
                                 KeyBasicInformation,
                                 basicInfo,
                                 bufferLen - sizeof (UNICODE_NULL), // term 0
                                 &len);

        if (!NT_SUCCESS (status)) {
            //
            // This should never happen.
            //
            break;
        }

        basicInfo->Name [basicInfo->NameLength/sizeof(WCHAR)] = 0;
        name.Length = (USHORT) basicInfo->NameLength;
        name.MaximumLength = (USHORT) basicInfo->NameLength + sizeof (UNICODE_NULL);
        name.Buffer = basicInfo->Name;

        InitializeObjectAttributes (&attributes,
                                    &name,
                                    OBJ_CASE_INSENSITIVE,
                                    acpiAlias,
                                    NULL);
        status = ZwOpenKey (&entry,
                            KEY_READ,
                            &attributes);
        if (!NT_SUCCESS (status)) {
            break;
        }

        //
        // Extract The Profile number to which this alias refers.
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PROFILE_NUMBER);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
            status = STATUS_REGISTRY_CORRUPT;
            ZwClose (entry);
            goto Clean;
        }
        (*AliasList)->Alias[i].ProfileNumber =
            * (PULONG) ((PUCHAR) value + value->DataOffset);

        //
        // Extract The Docking State.
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DOCKING_STATE);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
            status = STATUS_REGISTRY_CORRUPT;
            ZwClose (entry);
            goto Clean;
        }
        (*AliasList)->Alias[i].DockState =
            * (PULONG) ((PUCHAR) value + value->DataOffset);


        //
        // Find the SerialNumber
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ACPI_SERIAL_NUMBER);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (!NT_SUCCESS (status) || (value->Type != REG_BINARY)) {
            status = STATUS_REGISTRY_CORRUPT;
            ZwClose (entry);
            goto Clean;
        }

        (*AliasList)->Alias[i].SerialLength = value->DataLength;
        (*AliasList)->Alias[i].SerialNumber =
                    (value->DataLength) ?
                    ExAllocatePool (PagedPool, value->DataLength) :
                    0;

        if (value->DataLength && (NULL == (*AliasList)->Alias[i].SerialNumber)) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            ZwClose (entry);
            goto Clean;
        }

        if (value->DataLength) {
            RtlCopyMemory ((*AliasList)->Alias[i].SerialNumber,
                           (PUCHAR) value + value->DataOffset,
                           value->DataLength);
        }

        ZwClose (entry);
    }

Clean:
    if (NULL != acpiAlias) {
        NtClose (acpiAlias);
    }
    if (NULL != profiles) {
        NtClose (profiles);
    }

    if (!NT_SUCCESS (status)) {
        if (NULL != *ProfileList) {
            for (i = 0; i < (*ProfileList)->CurrentProfileCount; i++) {
                if ((*ProfileList)->Profile[i].FriendlyName) {
                    ExFreePool ((*ProfileList)->Profile[i].FriendlyName);
                }
            }
            ExFreePool (*ProfileList);
            *ProfileList = 0;
        }
        if (NULL != *AliasList) {
            for (i = 0; i < (*AliasList)->CurrentAliasCount; i++) {
                if ((*AliasList)->Alias[i].SerialNumber) {
                    ExFreePool ((*AliasList)->Alias[i].SerialNumber);
                }
            }
            ExFreePool (*AliasList);
            *AliasList = 0;
        }
    }
    return status;
}

NTSTATUS
CmpAddAcpiAliasEntry (
    IN HANDLE                       IDConfigDB,
    IN PPROFILE_ACPI_DOCKING_STATE  NewDockState,
    IN ULONG                        ProfileNumber,
    IN PWCHAR                       nameBuffer,
    IN PVOID                        valueBuffer,
    IN ULONG                        valueBufferLength,
    IN BOOLEAN                      PreventDuplication
    )
/*++
Routine Description:
    Set the Acpi Alais entry.


    Routine Description:
    Create an alias entry in the IDConfigDB database for the given
    hardware profile.

    Create the "AcpiAlias" key if it does not exist.

Parameters:

    IDConfigDB - Pointer to "..\CurrentControlSet\Control\IDConfigDB"

    NewDockState - The new docking state for which this alias points.

    ProfileNumber -The profile number to which this alias points.

    nameBuffer - a temp scratch space for writing things.
                (assumed to be at least 128 WCHARS)

--*/
{
    OBJECT_ATTRIBUTES attributes;
    NTSTATUS        status = STATUS_SUCCESS;
    ANSI_STRING     ansiString;
    UNICODE_STRING  name;
    HANDLE          aliasKey = NULL;
    HANDLE          aliasEntry = NULL;
    ULONG           value;
    ULONG           disposition;
    ULONG           aliasNumber = 0;
    ULONG           len;
    PKEY_VALUE_FULL_INFORMATION   keyInfo;

    PAGED_CODE ();

    keyInfo = (PKEY_VALUE_FULL_INFORMATION) valueBuffer;

    //
    // Find the Alias Key or Create it if it does not already exist.
    //
    RtlInitUnicodeString (&name,CM_HARDWARE_PROFILE_STR_ACPI_ALIAS);

    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                IDConfigDB,
                                NULL);

    status = NtOpenKey (&aliasKey,
                        KEY_READ | KEY_WRITE,
                        &attributes);

    if (STATUS_OBJECT_NAME_NOT_FOUND == status) {
        status = NtCreateKey (&aliasKey,
                              KEY_READ | KEY_WRITE,
                              &attributes,
                              0, // no title
                              NULL, // no class
                              0, // no options
                              &disposition);
    }

    if (!NT_SUCCESS (status)) {
        aliasKey = NULL;
        goto Exit;
    }

    //
    // Create an entry key
    //

    while (aliasNumber < 200) {
        aliasNumber++;

        swprintf (nameBuffer, L"%04d", aliasNumber);
        RtlInitUnicodeString (&name, nameBuffer);

        InitializeObjectAttributes(&attributes,
                                   &name,
                                   OBJ_CASE_INSENSITIVE,
                                   aliasKey,
                                   NULL);

        status = NtOpenKey (&aliasEntry,
                            KEY_READ | KEY_WRITE,
                            &attributes);

        if (NT_SUCCESS (status)) {

            if (PreventDuplication) {
                //
                // If we have a matching DockingState, SerialNumber, and
                // Profile Number, then we should not make this alias
                //

                //
                // Extract The DockingState to which this alias refers.
                //
                RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DOCKING_STATE);
                status = NtQueryValueKey(aliasEntry,
                                         &name,
                                         KeyValueFullInformation,
                                         valueBuffer,
                                         valueBufferLength,
                                         &len);

                if (!NT_SUCCESS (status) || (keyInfo->Type != REG_DWORD)) {
                    status = STATUS_REGISTRY_CORRUPT;
                    goto Exit;
                }

                if (NewDockState->DockingState !=
                    * (PULONG) ((PUCHAR) keyInfo + keyInfo->DataOffset)) {
                    //
                    // Not a dupe
                    //

                    NtClose (aliasEntry);
                    aliasEntry = NULL;
                    continue;
                }

                //
                // Extract the SerialNumber
                //
                RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ACPI_SERIAL_NUMBER);
                status = NtQueryValueKey(aliasEntry,
                                         &name,
                                         KeyValueFullInformation,
                                         valueBuffer,
                                         valueBufferLength,
                                         &len);

                if (!NT_SUCCESS (status) || (keyInfo->Type != REG_BINARY)) {
                    status = STATUS_REGISTRY_CORRUPT;
                    goto Exit;
                }

                if (NewDockState->SerialLength != keyInfo->DataLength) {
                    //
                    // Not a dupe
                    //

                    NtClose (aliasEntry);
                    aliasEntry = NULL;
                    continue;
                }

                if (!RtlEqualMemory (NewDockState->SerialNumber,
                                     ((PUCHAR) keyInfo + keyInfo->DataOffset),
                                     NewDockState->SerialLength)) {
                    //
                    // Not a dupe
                    //

                    NtClose (aliasEntry);
                    aliasEntry = NULL;
                    continue;
                }

                status = STATUS_SUCCESS;
                goto Exit;

            }

        } else if (STATUS_OBJECT_NAME_NOT_FOUND == status) {
            status = STATUS_SUCCESS;
            break;

        } else {
            break;
        }

    }
    if (!NT_SUCCESS (status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: cmpCreateAcpiAliasEntry error finding new set %08lx\n",
                     status));
        }
        aliasEntry = 0;
        goto Exit;
    }

    status = NtCreateKey (&aliasEntry,
                          KEY_READ | KEY_WRITE,
                          &attributes,
                          0,
                          NULL,
                          0,
                          &disposition);

    if (!NT_SUCCESS (status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: cmpCreateAcpiAliasEntry error creating new set %08lx\n",
                     status));
        }
        aliasEntry = 0;
        goto Exit;
    }

    //
    // Write the Docking State;
    //
    value = NewDockState->DockingState;
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DOCKING_STATE);
    status = NtSetValueKey (aliasEntry,
                            &name,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof (value));

    //
    // Write the Serial Number
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ACPI_SERIAL_NUMBER);
    status = NtSetValueKey (aliasEntry,
                            &name,
                            0,
                            REG_BINARY,
                            NewDockState->SerialNumber,
                            NewDockState->SerialLength);

    //
    // Write the Profile Number
    //
    value = ProfileNumber;
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PROFILE_NUMBER);
    status = NtSetValueKey (aliasEntry,
                            &name,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof (value));

Exit:

    if (aliasKey) {
        NtClose (aliasKey);
    }

    if (aliasEntry) {
        NtClose (aliasEntry);
    }

    return status;
}

NTSTATUS
CmSetAcpiHwProfile (
    IN PPROFILE_ACPI_DOCKING_STATE NewDockState,
    IN PCM_ACPI_SELECTION_ROUTINE  Select,
    IN PVOID Context,
    OUT PHANDLE NewProfile,
    OUT PBOOLEAN ProfileChanged
    )
/*++
Routine Description:

    The ACPI docking state of the machine has changed.

    Based on the new change calculate the new HW Profile(s) consitent with the
    new ACPI docking state.

    Pass the list of known profiles to the callers selection routine.

    Set the new current profile.

    Patch up any ACPI alias entries if a new profile for this ACPI state has
    been used.

Arguments:

    NewDockStateArray - The list of possible Docking States that we might enter.

    Select - Call back to select which profile to enter, given the list of
             possible profiles.

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    HANDLE          IDConfigDB = NULL;
    HANDLE          HardwareProfile = NULL;
    HANDLE          currentInfo = NULL;
    HANDLE          currentSymLink = NULL;
    HANDLE          parent = NULL;
    WCHAR           nameBuffer[128];
    UNICODE_STRING  name;
    UCHAR           valueBuffer[256];
    ULONG           len;
    ULONG           i;
    ULONG           selectedElement;
    ULONG           profileNum;
    ULONG           currentDockingState;
    ULONG           currentProfileNumber;
    ULONG           disposition;
    ULONG           flags;
    PWCHAR          currentAcpiSN = NULL;
    PCM_HARDWARE_PROFILE_ACPI_ALIAS_LIST  AliasList = NULL;
    PCM_HARDWARE_PROFILE_LIST             ProfileList = NULL;
    PKEY_VALUE_FULL_INFORMATION           value;
    OBJECT_ATTRIBUTES                     attributes;

    PAGED_CODE ();

    *ProfileChanged = FALSE;

    value = (PKEY_VALUE_FULL_INFORMATION) valueBuffer;

    //
    // Open The Hardware Profile Database
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DATABASE);
    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    status = ZwOpenKey (&IDConfigDB,
                        KEY_READ,
                        &attributes);

    if (!NT_SUCCESS (status)) {
        IDConfigDB = NULL;
        goto Clean;
    }

    //
    // Obtain the total list of profiles
    //
    status = CmpGetAcpiProfileInformation (IDConfigDB,
                                           &ProfileList,
                                           &AliasList,
                                           nameBuffer,
                                           valueBuffer,
                                           sizeof (valueBuffer));

    if (!NT_SUCCESS (status)) {
        goto Clean;
    }

    //
    // Determine the current Dock information.
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_CCS_CURRENT);
    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    status = ZwOpenKey (&HardwareProfile,
                        KEY_READ,
                        &attributes);
    if (!NT_SUCCESS (status)) {
        HardwareProfile = NULL;
        goto Clean;
    }
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_CURRENT_DOCK_INFO);
    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                IDConfigDB,
                                NULL);

    status = ZwOpenKey (&currentInfo,
                        KEY_READ,
                        &attributes);
    if (!NT_SUCCESS (status)) {
        currentInfo = NULL;
        goto Clean;
    }

    //
    // The current Docking State
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DOCKING_STATE);
    status = NtQueryValueKey (currentInfo,
                              &name,
                              KeyValueFullInformation,
                              valueBuffer,
                              sizeof (valueBuffer),
                              &len);

    if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
        status = STATUS_REGISTRY_CORRUPT;
        goto Clean;
    }
    currentDockingState = * (PULONG) ((PUCHAR) value + value->DataOffset);

    //
    // The current ACPI Serial Number
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ACPI_SERIAL_NUMBER);
    status = NtQueryValueKey(currentInfo,
                             &name,
                             KeyValueFullInformation,
                             valueBuffer,
                             sizeof (valueBuffer),
                             &len);

    if (NT_SUCCESS (status) && (value->Type == REG_BINARY)) {

        currentAcpiSN = ExAllocatePool (PagedPool, value->DataLength);

        if (NULL == currentAcpiSN) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Clean;
        }
        RtlCopyMemory (currentAcpiSN,
                       (PUCHAR) value + value->DataOffset,
                       value->DataLength);
    } else {
        currentAcpiSN = 0;
    }

    //
    // The current Profile Number
    //
    RtlInitUnicodeString(&name, L"CurrentConfig");
    status = NtQueryValueKey(IDConfigDB,
                             &name,
                             KeyValueFullInformation,
                             valueBuffer,
                             sizeof (valueBuffer),
                             &len);

    if (!NT_SUCCESS(status) || (value->Type != REG_DWORD)) {
        status = STATUS_REGISTRY_CORRUPT;
        goto Clean;
    }
    currentProfileNumber = *(PULONG)((PUCHAR)value + value->DataOffset);

    //
    // Filter the current list of hardware profiles based on the current
    // docking state, the new acpi state, and the acpi alias tables
    //
    status = CmpFilterAcpiDockingState (NewDockState,
                                        currentDockingState,
                                        currentAcpiSN,
                                        currentProfileNumber,
                                        ProfileList,
                                        AliasList);

    if (!NT_SUCCESS (status)) {
        goto Clean;
    }

    //
    // Allow the caller a chance to select from the filtered list.
    //
    status = Select (ProfileList, &selectedElement, Context);

    //
    // If the user selected -1 then he is not interested in selecting any of
    // the profiles.
    //
    if (-1 == selectedElement) {
        ASSERT (STATUS_MORE_PROCESSING_REQUIRED == status);
        goto Clean;
    }

    if (!NT_SUCCESS (status)) {
        goto Clean;
    }

    //
    // Fine! We have finally made the new selection.
    // Set it.
    //

    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_CCS_HWPROFILE);
    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
    status = ZwOpenKey (&parent, KEY_READ, &attributes);
    if (!NT_SUCCESS (status)) {
        parent = NULL;
        goto Clean;
    }

    //
    // How did we get here?
    //
    flags = ProfileList->Profile[selectedElement].Flags;
    profileNum = ProfileList->Profile[selectedElement].Id;

    //
    // Check for duplicate
    //
    if (flags & CM_HP_FLAGS_DUPLICATE) {
        //
        // If there is a duplicate then we need to adjust the pnp
        // bios alias table.
        //
        // This happens if we booted PnP bios detected docked, and then
        // we received a set state for ACPI as docked, then we have the
        // potential for duplicates.  See Comment in CmpFilterAcpiDockingState
        // for details.
        //
        // We need to find any pnp bios alias entries that match the current
        // state and point them to the duplicate entry.
        //

        ASSERT (flags & CM_HP_FLAGS_TRUE_MATCH);
        ASSERT (!(flags & CM_HP_FLAGS_PRISTINE));

        status = CmpMoveBiosAliasTable (IDConfigDB,
                                        currentInfo,
                                        currentProfileNumber,
                                        profileNum,
                                        nameBuffer,
                                        valueBuffer,
                                        sizeof (valueBuffer));

        if (!NT_SUCCESS (status)) {
            goto Clean;
        }
    }

    if ((flags & CM_HP_FLAGS_PRISTINE) || (profileNum != currentProfileNumber)){
        //
        // The profile Number Changed or will change.
        //
        *ProfileChanged = TRUE;

        ASSERT (currentInfo);
        ZwClose (currentInfo);
        currentInfo = NULL;

        if (flags & CM_HP_FLAGS_PRISTINE) {
            //
            // If the selected profile is pristine then we need to clone.
            //
            ASSERT (!(flags & CM_HP_FLAGS_TRUE_MATCH));
            status = CmpCloneHwProfile (IDConfigDB,
                                        parent,
                                        HardwareProfile,
                                        profileNum,
                                        NewDockState->DockingState,
                                        &HardwareProfile,
                                        &profileNum);
            if (!NT_SUCCESS (status)) {
                HardwareProfile = 0;
                goto Clean;
            }
        } else {
            ASSERT (HardwareProfile);
            ZwClose (HardwareProfile);

            //
            // Open the new profile
            //
            swprintf (nameBuffer, L"%04d\0", profileNum);
            RtlInitUnicodeString (&name, nameBuffer);
            InitializeObjectAttributes (&attributes,
                                        &name,
                                        OBJ_CASE_INSENSITIVE,
                                        parent,
                                        NULL);
            status = ZwOpenKey (&HardwareProfile, KEY_READ, &attributes);
            if (!NT_SUCCESS (status)) {
                HardwareProfile = NULL;
                goto Clean;
            }
        }

        ASSERT (currentProfileNumber != profileNum);

        //
        // Open the current info for the profile.
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_CURRENT_DOCK_INFO);
        InitializeObjectAttributes (&attributes,
                                    &name,
                                    OBJ_CASE_INSENSITIVE,
                                    IDConfigDB,
                                    NULL);

        status = NtCreateKey (&currentInfo,
                              KEY_READ | KEY_WRITE,
                              &attributes,
                              0,
                              NULL,
                              REG_OPTION_VOLATILE,
                              &disposition);

        if (!NT_SUCCESS (status)) {
            currentInfo = NULL;
            goto Clean;
        }

        //
        // Set CurrentConfig in the Database
        //
        RtlInitUnicodeString(&name, L"CurrentConfig");
        status = NtSetValueKey(IDConfigDB,
                                 &name,
                                 0,
                                 REG_DWORD,
                                 &profileNum,
                                 sizeof (profileNum));

         if (!NT_SUCCESS(status)) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Clean;
        }
    }

    //
    // Write the new Docking State to the current Info key
    //
    i = NewDockState->DockingState;
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DOCKING_STATE);
    status = ZwSetValueKey (currentInfo,
                            &name,
                            0,
                            REG_DWORD,
                            &i,
                            sizeof (ULONG));

    //
    // Write the new ACPI information to the current Info key
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ACPI_SERIAL_NUMBER);
    status = ZwSetValueKey (currentInfo,
                            &name,
                            0,
                            REG_BINARY,
                            NewDockState->SerialNumber,
                            NewDockState->SerialLength);

    if (!(flags & CM_HP_FLAGS_TRUE_MATCH)) {
        //
        // Add the alias entry for this profile.
        //
        status = CmpAddAcpiAliasEntry (IDConfigDB,
                                       NewDockState,
                                       profileNum,
                                       nameBuffer,
                                       valueBuffer,
                                       sizeof (valueBuffer),
                                       FALSE); // Don't Prevent Duplication
    }

    if (profileNum != currentProfileNumber) {
        //
        // Move the symbolic link.
        //
        RtlInitUnicodeString(&name, CM_HARDWARE_PROFILE_STR_CCS_CURRENT);
        InitializeObjectAttributes(&attributes,
                                   &name,
                                   OBJ_CASE_INSENSITIVE | OBJ_OPENLINK,
                                   NULL,
                                   NULL);

        status = NtCreateKey(&currentSymLink,
                             KEY_CREATE_LINK,
                             &attributes,
                             0,
                             NULL,
                             REG_OPTION_OPEN_LINK,
                             &disposition);

        ASSERT (STATUS_SUCCESS == status);
        ASSERT (REG_OPENED_EXISTING_KEY == disposition);

        swprintf (nameBuffer,
                  L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\%04d",
                  profileNum);
        RtlInitUnicodeString (&name, nameBuffer);
        status = NtSetValueKey (currentSymLink,
                                &CmSymbolicLinkValueName,
                                0,
                                REG_LINK,
                                name.Buffer,
                                name.Length);

        ASSERT (STATUS_SUCCESS == status);
    }


Clean:
    if (NT_SUCCESS (status)) {
        // NB more process required is not a success code.
        *NewProfile = HardwareProfile;
    } else if (NULL != HardwareProfile) {
        ZwClose (HardwareProfile);
    }

    if (NULL != IDConfigDB) {
        ZwClose (IDConfigDB);
    }
    if (NULL != currentInfo) {
        ZwClose (currentInfo);
    }
    if (NULL != parent) {
        ZwClose (parent);
    }
    if (NULL != currentAcpiSN) {
        ExFreePool (currentAcpiSN);
    }
    if (NULL != ProfileList) {
        for (i = 0; i < ProfileList->CurrentProfileCount; i++) {
            if (ProfileList->Profile[i].FriendlyName) {
                ExFreePool (ProfileList->Profile[i].FriendlyName);
            }
        }
        ExFreePool (ProfileList);
    }
    if (NULL != AliasList) {
        for (i = 0; i < AliasList->CurrentAliasCount; i++) {
            if (AliasList->Alias[i].SerialNumber) {
                ExFreePool (AliasList->Alias[i].SerialNumber);
            }
        }
        ExFreePool (AliasList);
    }

    return status;
}

NTSTATUS
CmpFilterAcpiDockingState (
    IN     PPROFILE_ACPI_DOCKING_STATE  NewDockingState,
    IN     ULONG                        CurrentDockState,
    IN     PWCHAR                       CurrentAcpiSN,
    IN     ULONG                        CurrentProfileNumber,
    IN OUT PCM_HARDWARE_PROFILE_LIST    ProfileList,
    IN OUT PCM_HARDWARE_PROFILE_ACPI_ALIAS_LIST AliasList
    )
/*++
Routine Description:
    Given the new state of things and the current state of things,
    prune the given list of profiles.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG i = 0;
    ULONG j;
    ULONG len;
    ULONG mask = HW_PROFILE_DOCKSTATE_UNDOCKED | HW_PROFILE_DOCKSTATE_DOCKED;
    ULONG flags;
    PCM_HARDWARE_PROFILE_ACPI_ALIAS alias;
    BOOLEAN trueMatch = FALSE;
    BOOLEAN dupDetect = FALSE;
    BOOLEAN currentListed = FALSE;
    BOOLEAN keepCurrent = FALSE;

    PAGED_CODE ();

    //
    // Check for duplicate:
    //
    // If the user boots undocked, and then hot docks.  We will generate
    // a profile alias for the pnp reported undocked state [A], and one for the
    // ACPI reported docked state [B}.  If the use subsequently reboots docked,
    // then we will create a third pnp reported docked state [C] profile alias.
    // {C] is really a duplicate of [B}, but we wont know this until such time
    // as the ACPI state for {B] is reported.
    //
    // The same can happen for undocked scenerios.
    //
    // Detection: If the Current Dock State is the same as
    // NewDockingState.DockingState then there is a potential for a duplicate.
    // In order to also have a duplicate we must have an acpi already pointing
    // to a profile different than the current one.
    // This must also be the first ACPI change since we booted, therefore
    // CurrentAcpiSn Should be Zero.
    // In other words there must be at least one true match and none of the
    // true matches can point to the current profile.
    //

    if (AliasList) {
        while (i < AliasList->CurrentAliasCount) {
            alias = &AliasList->Alias[i];

            if (((alias->DockState & mask) != 0) &&
                ((alias->DockState & mask) !=
                 (NewDockingState->DockingState & mask))) {

                //
                // This alias claims to be docked or undocked, but does not
                // match the current state.  Therefore skip it.
                //
                ;

            } else if (alias->SerialLength != NewDockingState->SerialLength) {
                //
                // This alias has an incompatible serial number
                //
                ;

            } else if (alias->SerialLength ==
                       RtlCompareMemory (NewDockingState->SerialNumber,
                                         alias->SerialNumber,
                                         alias->SerialLength)) {
                //
                // NB RtlCompareMemory can work with zero length memory
                // addresses.  This is a requirement here.
                //



                //
                // This alias matches so mark the profile.
                //
                for (j = 0; j < ProfileList->CurrentProfileCount; j++) {
                    if (ProfileList->Profile[j].Id == alias->ProfileNumber) {

                        //
                        // Alias entries should never point to a pristine profile
                        //
                        ASSERT (!(ProfileList->Profile[j].Flags &
                                  CM_HP_FLAGS_PRISTINE));

                        ProfileList->Profile[j].Flags |= CM_HP_FLAGS_TRUE_MATCH;
                        trueMatch = TRUE;
                    }
                    if ((CurrentDockState == NewDockingState->DockingState) &&
                        (NULL == CurrentAcpiSN)) {
                        //
                        // The dock state did not change during this acpi
                        // event; therefore, we might just have a duplicate
                        // on our hands.
                        //
                        dupDetect = TRUE;
                    }
                    if (alias->ProfileNumber == CurrentProfileNumber) {
                        //
                        // There exists an entry in the acpi alias table that
                        // if chosen would result in no change of Hardware
                        // Profile.  Therefore, we should chose this one, and
                        // ignore the duplicate.
                        //
                        currentListed = TRUE;
                    }
                }
            }
            i++;
        }
    }

    if ((!dupDetect) &&
        (NULL == CurrentAcpiSN) &&
        (!trueMatch) &&
        (CurrentDockState == NewDockingState->DockingState)) {

        //
        // (1) The docking state did not change,
        // (2) the current profile has not yet, on this boot, been marked with
        //     an ACPI serial number.
        // (3) There was no Alias match.
        //
        // Therefore we should keep the current profile regardless of it being
        // aliasable.
        //

        keepCurrent = TRUE;
        trueMatch = TRUE;  // prevent pristine from being listed.
    }

    i = 0;
    while (i < ProfileList->CurrentProfileCount) {

        flags = ProfileList->Profile[i].Flags;

        if (dupDetect) {
            if (flags & CM_HP_FLAGS_TRUE_MATCH) {
                if (currentListed) {
                    if (ProfileList->Profile[i].Id == CurrentProfileNumber) {
                        //
                        // Let this one live.  This results in no change of
                        // profile number.
                        //
                        i++;
                        continue;
                    }
                    //
                    // Bounce any true matches that do not result in no change
                    // of profile.
                    //
                    ;

                } else {
                    //
                    // We did not find the current one listed so we definately
                    // have a duplicate.
                    //
                    // Mark it as such. and list it live.
                    //
                    ProfileList->Profile[i].Flags |= CM_HP_FLAGS_DUPLICATE;
                    i++;
                    continue;
                }
            }
            //
            // Bounce all non True matches in a duplicate detected situation.
            //
            ;

        } else if ((flags & CM_HP_FLAGS_PRISTINE) && !trueMatch) {
            //
            // Leave this one in the list
            //
            i++;
            continue;

        } else if (flags & CM_HP_FLAGS_ALIASABLE) {
            //
            // Leave this one in the list
            //
            ASSERT (! (flags & CM_HP_FLAGS_PRISTINE));
            i++;
            continue;

        } else if (flags & CM_HP_FLAGS_TRUE_MATCH) {
            //
            // Leave this one in the list
            //
            i++;
            continue;

        } else if (keepCurrent &&
                   (ProfileList->Profile[i].Id == CurrentProfileNumber)) {
            //
            // Leave this one in the list
            //
            i++;
            continue;
        }

        //
        // discard this profile by (1) shifting remaining profiles in
        //   array to fill in the space of this discarded profile
        //   and (2) decrementing profile count
        //
        len = ProfileList->CurrentProfileCount - i - 1;
        if (0 < len) {
            RtlMoveMemory(&ProfileList->Profile[i],
                          &ProfileList->Profile[i+1],
                          sizeof(CM_HARDWARE_PROFILE) * len);
        }

        --ProfileList->CurrentProfileCount;
    }

    return status;
}



NTSTATUS
CmpMoveBiosAliasTable (
    IN HANDLE   IDConfigDB,
    IN HANDLE   CurrentInfo,
    IN ULONG    CurrentProfileNumber,
    IN ULONG    NewProfileNumber,
    IN PWCHAR   nameBuffer,
    IN PCHAR    valueBuffer,
    IN ULONG    bufferLen
    )
/*++
Routine Description:
    Search the Alias table for bios entries which match the current
    docking state, and point from current profile number to new profile number.


    Assumption: If the profile is cloned (therefore created by
    CmpCloneHwProfile, and we have just moved the bios table to point
    away from this entry, then we *should* be able to safely delete
    the old hardware profile key.
    (in both IDConfigDB\HardwareProfiles and CCS\HardwareProfiles


--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    HANDLE          alias = NULL;
    HANDLE          entry = NULL;
    HANDLE          hwprofile = NULL;
    UNICODE_STRING  name;
    ULONG           currentDockId;
    ULONG           currentSerialNumber;
    ULONG           len;
    ULONG           i;
    OBJECT_ATTRIBUTES           attributes;
    KEY_FULL_INFORMATION        keyInfo;
    PKEY_BASIC_INFORMATION      basicInfo;
    PKEY_VALUE_FULL_INFORMATION value;

    PAGED_CODE ();

    value = (PKEY_VALUE_FULL_INFORMATION) valueBuffer;
    basicInfo = (PKEY_BASIC_INFORMATION) valueBuffer;

    //
    // Extract the current Serial Number and DockID
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_SERIAL_NUMBER);
    status = NtQueryValueKey(CurrentInfo,
                             &name,
                             KeyValueFullInformation,
                             valueBuffer,
                             bufferLen,
                             &len);
    if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
        status = STATUS_REGISTRY_CORRUPT;
        goto Clean;
    }
    currentSerialNumber = * (PULONG) ((PUCHAR) value + value->DataOffset);

    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DOCKID);
    status = NtQueryValueKey(CurrentInfo,
                             &name,
                             KeyValueFullInformation,
                             valueBuffer,
                             bufferLen,
                             &len);
    if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
        status = STATUS_REGISTRY_CORRUPT;
        goto Clean;
    }
    currentDockId = * (PULONG) ((PUCHAR) value + value->DataOffset);

    //
    // Open a handle to the Alias information
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ALIAS);
    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                IDConfigDB,
                                NULL);
    status = ZwOpenKey (&alias,
                        KEY_READ,
                        &attributes);

    if (!NT_SUCCESS (status)) {
        //
        // So we don't have an alias table.  This is ok, albeit a bit strange
        //
        status = STATUS_SUCCESS;
        alias = NULL;
        goto Clean;
    }


    status = ZwQueryKey (alias,
                         KeyFullInformation,
                         &keyInfo,
                         sizeof (keyInfo),
                         &len);

    if (!NT_SUCCESS (status)) {
        goto Clean;
    }
    ASSERT (0 < keyInfo.SubKeys);

    //
    // Iterrate the alias entries
    //
    for (i = 0; i < keyInfo.SubKeys; i++) {

        //
        // Get the first key in the list.
        //
        status = ZwEnumerateKey (alias,
                                 i,
                                 KeyBasicInformation,
                                 basicInfo,
                                 bufferLen - sizeof (UNICODE_NULL), // term 0
                                 &len);

        if (!NT_SUCCESS (status)) {
            //
            // This should never happen.
            //
            break;
        }

        basicInfo->Name [basicInfo->NameLength/sizeof(WCHAR)] = 0;
        name.Length = (USHORT) basicInfo->NameLength;
        name.MaximumLength = (USHORT) basicInfo->NameLength + sizeof (UNICODE_NULL);
        name.Buffer = basicInfo->Name;

        InitializeObjectAttributes (&attributes,
                                    &name,
                                    OBJ_CASE_INSENSITIVE,
                                    alias,
                                    NULL);
        status = ZwOpenKey (&entry,
                            KEY_READ | KEY_WRITE,
                            &attributes);
        if (!NT_SUCCESS (status)) {
            break;
        }

        //
        // Extract The Profile number to which this alias refers.
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PROFILE_NUMBER);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Clean;
        }

        if (CurrentProfileNumber != *(PULONG)((PUCHAR)value + value->DataOffset)) {

            //
            // Not a match
            //
            ZwClose (entry);
            entry = NULL;
            continue;
        }

        //
        // Compare the Dock ID
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_DOCKID);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);
        if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Clean;
        }
        if (currentDockId != * (PULONG) ((PUCHAR) value + value->DataOffset)) {
            //
            // Not a match
            //
            ZwClose (entry);
            entry = NULL;
            continue;
        }

        //
        // Compare the SerialNumber
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_SERIAL_NUMBER);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);
        if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Clean;
        }
        if (currentSerialNumber != *(PULONG)((PUCHAR)value + value->DataOffset)) {
            //
            // Not a match
            //
            ZwClose (entry);
            entry = NULL;
            continue;
        }

        //
        // This must be a match.
        // move the profile number
        //

        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PROFILE_NUMBER);
        status = NtSetValueKey (entry,
                                &name,
                                0,
                                REG_DWORD,
                                &NewProfileNumber,
                                sizeof (NewProfileNumber));

        ASSERT (STATUS_SUCCESS == status);

        ZwClose (entry);
        entry = NULL;

        //
        // We most likely have left a dangling profile here.
        // Try to attempt to clean it up.
        //
        // If this profile is cloned then we created it and can therefore
        // get rid of it.
        //

        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_HARDWARE_PROFILES);
        InitializeObjectAttributes (&attributes,
                                    &name,
                                    OBJ_CASE_INSENSITIVE,
                                    IDConfigDB,
                                    NULL);
        status = ZwOpenKey (&hwprofile, KEY_READ | KEY_WRITE, &attributes);
        if (!NT_SUCCESS (status)) {
            hwprofile = NULL;
            status = STATUS_REGISTRY_CORRUPT;
            goto Clean;
        }

        swprintf (nameBuffer, L"%04d\0", CurrentProfileNumber);
        RtlInitUnicodeString (&name, nameBuffer);
        InitializeObjectAttributes (&attributes,
                                    &name,
                                    OBJ_CASE_INSENSITIVE,
                                    hwprofile,
                                    NULL);
        status = ZwOpenKey (&entry, KEY_ALL_ACCESS, &attributes);
        if (!NT_SUCCESS (status)) {
            entry = NULL;
            status = STATUS_REGISTRY_CORRUPT;
            goto Clean;
        }

        //
        // Test for the Cloned Bit.
        //

        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_CLONED);
        status = NtQueryValueKey(entry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 bufferLen,
                                 &len);

        if (!NT_SUCCESS (status) || (value->Type != REG_DWORD)) {
            status = STATUS_REGISTRY_CORRUPT;
            goto Clean;
        }

        if (*(PULONG)((PUCHAR)value + value->DataOffset)) {
            //
            // We cloned this one.
            //
            status = ZwDeleteKey (entry);
            ASSERT (NT_SUCCESS (status));

            ZwClose (entry);
            ZwClose (hwprofile);
            entry = hwprofile = NULL;

            RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_CCS_HWPROFILE);
            InitializeObjectAttributes (&attributes,
                                        &name,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL);
            status = ZwOpenKey (&hwprofile, KEY_READ | KEY_WRITE, &attributes);
            if (!NT_SUCCESS (status)) {
                hwprofile = NULL;
                status = STATUS_REGISTRY_CORRUPT;
                goto Clean;
            }

            swprintf (nameBuffer, L"%04d\0", CurrentProfileNumber);

            status = CmDeleteKeyRecursive (hwprofile,
                                           nameBuffer,
                                           valueBuffer,
                                           bufferLen,
                                           TRUE);

            ASSERT (NT_SUCCESS (status));
            ZwClose (hwprofile);
            hwprofile = NULL;

        } else {
            //
            // We didn't clone this one.
            // don't do anything else.
            //
            ZwClose (entry);
            ZwClose (hwprofile);
            entry = hwprofile = NULL;
        }

        CM_HARDWARE_PROFILE_STR_CCS_HWPROFILE;



    }

Clean:

    if (alias) {
        ZwClose (alias);
    }
    if (entry) {
        ZwClose (entry);
    }
    if (hwprofile) {
        ZwClose (hwprofile);
    }

    return status;
}


NTSTATUS
CmpCloneHwProfile (
    IN HANDLE IDConfigDB,
    IN HANDLE Parent,
    IN HANDLE OldProfile,
    IN ULONG  OldProfileNumber,
    IN USHORT DockingState,
    OUT PHANDLE NewProfile,
    OUT PULONG  NewProfileNumber
    )
/*++
Routine Description

    The given hardware profile key needs cloning.
    Clone the key and then return the new profile.

Return:

    STATUS_SUCCESS - if the profile has been cloned, in which case the new
        profile key has been opened for read / write privs.  The old profile
        will be closed.

    <unsuccessful> - for a given error.  NewProfile is invalid and the Old
        Profile has also been closed.


                (Copied lovingly from CmpCloneControlSet)


--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING newProfileName;
    UNICODE_STRING name;
    UNICODE_STRING friendlyName;
    UNICODE_STRING guidStr;
    PCM_KEY_BODY oldProfileKey;
    PCM_KEY_BODY newProfileKey;
    OBJECT_ATTRIBUTES attributes;
    PSECURITY_DESCRIPTOR security;
    ULONG securityLength;
    WCHAR nameBuffer [64];
    HANDLE IDConfigDBEntry = NULL;
    ULONG disposition;
    ULONG value;
    UUID  uuid;
    PKEY_BASIC_INFORMATION keyBasicInfo;
    PKEY_FULL_INFORMATION keyFullInfo;
    PKEY_VALUE_FULL_INFORMATION keyValueInfo;
    ULONG  length, profileSubKeys, i;
    UCHAR  valueBuffer[256];
    HANDLE hardwareProfiles=NULL;
    HANDLE profileEntry=NULL;

    PAGED_CODE ();

    keyFullInfo  = (PKEY_FULL_INFORMATION)  valueBuffer;
    keyBasicInfo = (PKEY_BASIC_INFORMATION) valueBuffer;
    keyValueInfo = (PKEY_VALUE_FULL_INFORMATION) valueBuffer;

    *NewProfile = 0;
    *NewProfileNumber = OldProfileNumber;

    //
    // Find the new profile number.
    //

    while (*NewProfileNumber < 200) {
        (*NewProfileNumber)++;

        swprintf (nameBuffer, L"%04d", *NewProfileNumber);
        RtlInitUnicodeString (&newProfileName, nameBuffer);

        InitializeObjectAttributes(&attributes,
                                   &newProfileName,
                                   OBJ_CASE_INSENSITIVE,
                                   Parent,
                                   NULL);

        status = NtOpenKey (NewProfile,
                            KEY_READ | KEY_WRITE,
                            &attributes);

        if (NT_SUCCESS (status)) {
            NtClose (*NewProfile);

        } else if (STATUS_OBJECT_NAME_NOT_FOUND == status) {
            status = STATUS_SUCCESS;
            break;

        } else {
            break;
        }

    }
    if (!NT_SUCCESS (status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: CmpCloneHwProfile error finding new profile key %08lx\n", status));
        }
        goto Exit;
    }

    //
    // Get the security descriptor from the old key to create the new clone one.
    //

    status = NtQuerySecurityObject (OldProfile,
                                    DACL_SECURITY_INFORMATION,
                                    NULL,
                                    0,
                                    &securityLength);

    if (STATUS_BUFFER_TOO_SMALL == status) {

        security = ExAllocatePool (PagedPool, securityLength);

        if (security != NULL) {
            status = NtQuerySecurityObject(OldProfile,
                                           DACL_SECURITY_INFORMATION,
                                           security,
                                           securityLength,
                                           &securityLength);
            if (!NT_SUCCESS (status)) {
                CMLOG(CML_BUGCHECK, CMS_INIT) {
                    KdPrint(("CM: CmpCloneHwProfile"
                             " - NtQuerySecurityObject failed %08lx\n", status));
                }
                ExFreePool(security);
                security=NULL;
            }
        }
    } else {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: CmpCloneHwProfile"
                     " - NtQuerySecurityObject returned %08lx\n", status));
        }
        security=NULL;
    }

    //
    // Create the new key
    //
    InitializeObjectAttributes  (&attributes,
                                 &newProfileName,
                                 OBJ_CASE_INSENSITIVE,
                                 Parent,
                                 security);

    status = NtCreateKey (NewProfile,
                          KEY_READ | KEY_WRITE,
                          &attributes,
                          0,
                          NULL,
                          0,
                          &disposition);

    if (NULL != security) {
        ExFreePool (security);
    }
    if (!NT_SUCCESS (status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: CmpCloneHwProfile couldn't create Clone %08lx\n",status));
        }
        goto Exit;
    }

    //
    // Check to make sure the key was created.  If it already exists,
    // something is wrong.
    //
    if (disposition != REG_CREATED_NEW_KEY) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
        //  KdPrint(("CM: CmpCloneHwProfile: Clone tree already exists!\n"));
        }

        //
        // WARNNOTE:
        //      If somebody somehow managed to create a key in our way,
        //      they'll thwart duplication of the prestine.  Tough luck.
        //      Claim it worked and go on.
        //
        status = STATUS_SUCCESS;
        goto Exit;
    }

    //
    // Create the IDConfigDB Entry
    //
    swprintf (nameBuffer, L"Hardware Profiles\\%04d", *NewProfileNumber);
    RtlInitUnicodeString (&name, nameBuffer);

    InitializeObjectAttributes  (&attributes,
                                 &name,
                                 OBJ_CASE_INSENSITIVE,
                                 IDConfigDB,
                                 NULL);

    status = NtCreateKey (&IDConfigDBEntry,
                          KEY_READ | KEY_WRITE,
                          &attributes,
                          0,
                          NULL,
                          0,
                          &disposition);

    if (!NT_SUCCESS (status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: CmpCloneHwProfile couldn't create Clone %08lx\n",status));
        }
        IDConfigDBEntry = NULL;
        goto Exit;
    }

    //
    // Determine the next PreferenceOrder for the new profile.  (The
    // PrefenceOrder for the new profile will be incrementally next from the
    // greatest PreferenceOrder value of all the current profiles; assumes
    // current set of PreferenceOrder values is incremental)
    //

    //
    // Open the Hardware Profiles key
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_HARDWARE_PROFILES);

    InitializeObjectAttributes (&attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                IDConfigDB,
                                NULL);
    status = ZwOpenKey (&hardwareProfiles,
                        KEY_READ,
                        &attributes);
    
    if (!NT_SUCCESS (status)) {
        hardwareProfiles = NULL;
        goto Exit;
    }

    //
    // Find the number of profile Sub Keys
    //
    status = ZwQueryKey (hardwareProfiles,
                         KeyFullInformation,
                         valueBuffer,
                         sizeof (valueBuffer),
                         &length);

    if (!NT_SUCCESS (status)) {
        goto Exit;
    }

    //
    // At very least, the Pristine and the new profile key we just created,
    // should be there. 
    //
    profileSubKeys = keyFullInfo->SubKeys;
    ASSERT (1 < profileSubKeys);

    //
    // Initialize the highest PreferenceOrder value found to -1.
    //
    value = -1;

    //
    // Iterrate the profiles
    //
    for (i = 0; i < profileSubKeys; i++) {
    
        //
        // Enumerate all profile subkeys, noting their PreferenceOrder values.
        //
        status = ZwEnumerateKey (hardwareProfiles,
                                 i,
                                 KeyBasicInformation,
                                 valueBuffer,
                                 sizeof(valueBuffer) - sizeof (UNICODE_NULL), //term 0
                                 &length);
        if(!NT_SUCCESS(status)) {
            break;
        }
        
        //
        // Zero-terminate the subkey name just in case.
        //
        keyBasicInfo->Name[keyBasicInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // If this is the Pristine, or the NewProfile key, ignore it.
        //
        if ((!_wtoi(keyBasicInfo->Name)) || 
            ((ULONG)(_wtoi(keyBasicInfo->Name)) == *NewProfileNumber)) {
            continue;
        }
        
        //
        // Open this profile key
        //
        name.Length = (USHORT) keyBasicInfo->NameLength;
        name.MaximumLength = (USHORT) keyBasicInfo->NameLength + sizeof (UNICODE_NULL);
        name.Buffer = keyBasicInfo->Name;

        InitializeObjectAttributes (&attributes,
                                    &name,
                                    OBJ_CASE_INSENSITIVE,
                                    hardwareProfiles,
                                    NULL);
        status = ZwOpenKey (&profileEntry,
                            KEY_READ,
                            &attributes);
        if (!NT_SUCCESS (status)) {
            profileEntry = NULL;
            continue;
        }

        //
        // Extract The PreferenceOrder value for this Profile.
        //
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PREFERENCE_ORDER);
        status = NtQueryValueKey(profileEntry,
                                 &name,
                                 KeyValueFullInformation,
                                 valueBuffer,
                                 sizeof(valueBuffer),
                                 &length);

        if (!NT_SUCCESS (status) || (keyValueInfo->Type != REG_DWORD)) {
            //
            // No PreferenceOrder; continue on as best we can
            //
            ZwClose(profileEntry);
            profileEntry=NULL;
            continue;
        }

        //
        // If this is a the highest PreferenceOrder so far, reassign value to
        // this PreferenceOrder, OR assign it this valid PreferenceOrder if
        // value is still unassigned.
        //
        if (((*(PULONG) ((PUCHAR)keyValueInfo + keyValueInfo->DataOffset)) > value) ||
            (value == -1)) {
            value = (* (PULONG) ((PUCHAR)keyValueInfo + keyValueInfo->DataOffset));
        }
        
        ZwClose(profileEntry);
        profileEntry=NULL;
    }

    //
    // Increment value one above the greatest PreferenceOrder found.
    // (If no other profiles were found, (value+=1) == 0, the most preferred
    // profile) 
    //
    value += 1;

    //
    // Give the new profile a preference order.
    //
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_PREFERENCE_ORDER);
    status = NtSetValueKey (IDConfigDBEntry,
                            &name,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof (value));

    //
    // Give the new profile a friendly name, based on the DockingState
    //
    status = CmpCreateHwProfileFriendlyName(IDConfigDB,
                                            DockingState,
                                            *NewProfileNumber,
                                            &friendlyName);

    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_FRIENDLY_NAME);
        status = NtSetValueKey (IDConfigDBEntry,
                                &name,
                                0,
                                REG_SZ,
                                friendlyName.Buffer,
                                friendlyName.Length + sizeof(UNICODE_NULL));
        RtlFreeUnicodeString(&friendlyName);
    }

    //
    // Set the aliasable flag on the new "cloned profile" to be false
    //
    value = FALSE;
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_ALIASABLE);
    status = NtSetValueKey (IDConfigDBEntry,
                            &name,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof (value));

    //
    // Set the cloned profile on the new "cloned profile" to be true;
    //
    value = TRUE;
    RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_CLONED);
    status = NtSetValueKey (IDConfigDBEntry,
                            &name,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof (value));

    //
    // Set the HwProfileGuid for the brand new profile
    //

    status = ExUuidCreate (&uuid);
    if (NT_SUCCESS (status)) {

        status = RtlStringFromGUID (&uuid, &guidStr);
        if (NT_SUCCESS (status)) {
            RtlInitUnicodeString (&name, CM_HARDWARE_PROFILE_STR_HW_PROFILE_GUID);

            status = NtSetValueKey (IDConfigDBEntry,
                                    &name,
                                    0,
                                    REG_SZ,
                                    guidStr.Buffer,
                                    guidStr.MaximumLength);

            RtlFreeUnicodeString(&guidStr);
        } else {
            //
            // What's a fella to do?
            // let's just go on.
            //
            status = STATUS_SUCCESS;
        }

    } else {
        //
        // let's just go on.
        //
        status = STATUS_SUCCESS;
    }


    //
    // Clone the key
    //
    // (Copied lovingly from CmpCloneControlSet)
    //
    //
    status = ObReferenceObjectByHandle (OldProfile,
                                        KEY_READ,
                                        CmpKeyObjectType,
                                        KernelMode,
                                        (PVOID *)(&oldProfileKey),
                                        NULL);

    if (!NT_SUCCESS(status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: CmpCloneHWProfile: couldn't reference CurrentHandle %08lx\n",
                     status));
        }
        goto Exit;
    }

    ObReferenceObjectByHandle (*NewProfile,
                               KEY_WRITE,
                               CmpKeyObjectType,
                               KernelMode,
                               (PVOID *)(&newProfileKey),
                               NULL);

    if (!NT_SUCCESS(status)) {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: CmpCloneHWProfile: couldn't reference CurrentHandle %08lx\n",
                     status));
        }
        goto Exit;
    }

    CmpLockRegistryExclusive();

    //
    // Note: This copy tree command does not copy the values in the
    // root keys.  We are relying on this, since the values stored there
    // are things like "pristine" which we do not wish to have moved to the
    // new tree.
    //
    if (CmpCopyTree(oldProfileKey->KeyControlBlock->KeyHive,
                    oldProfileKey->KeyControlBlock->KeyCell,
                    newProfileKey->KeyControlBlock->KeyHive,
                    newProfileKey->KeyControlBlock->KeyCell)) {
        //
        // Set the max subkey name property for the new target key.
        //
        newProfileKey->KeyControlBlock->KeyNode->MaxNameLen = 
            oldProfileKey->KeyControlBlock->KeyNode->MaxNameLen;
        status = STATUS_SUCCESS;
    } else {
        CMLOG(CML_BUGCHECK, CMS_INIT) {
            KdPrint(("CM: CmpCloneHwProfile: tree copy failed.\n"));
        }
        status = STATUS_REGISTRY_CORRUPT;
    }
    CmpUnlockRegistry();


Exit:
    NtClose (OldProfile);
    if (IDConfigDBEntry) {
        NtClose (IDConfigDBEntry);
    }
    if (hardwareProfiles) {
        NtClose (hardwareProfiles);
    }
    if (!NT_SUCCESS (status)) {
        if (*NewProfile) {
            NtClose (*NewProfile);
        }
    }    

    return status;
}

NTSTATUS
CmDeleteKeyRecursive(
    HANDLE  hKeyRoot,
    PWSTR   Key,
    PVOID   TemporaryBuffer,
    ULONG   LengthTemporaryBuffer,
    BOOLEAN ThisKeyToo
    )
/*++

Routine Description:

    Routine to recursively delete all subkeys under the given
    key, including the key given.

Arguments:

    hKeyRoot:    Handle to root relative to which the key to be deleted is
                 specified.

    Key:         Root relative path of the key which is to be recursively deleted.

    ThisKeyToo:  Whether after deletion of all subkeys, this key itself is to
                 be deleted.

Return Value:

    Status is returned.

--*/
{
    ULONG ResultLength;
    PKEY_BASIC_INFORMATION KeyInfo;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    PWSTR SubkeyName;
    HANDLE hKey;

    //
    // Initialize
    //

    KeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;

    //
    // Open the key
    //

    RtlInitUnicodeString (&UnicodeString,Key);

    InitializeObjectAttributes(&Obja,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               hKeyRoot,
                               NULL);

    Status = ZwOpenKey(&hKey,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS(Status) ) {
        return(Status);
    }

    //
    // Enumerate all subkeys of the current key. if any exist they should
    // be deleted first.  since deleting the subkey affects the subkey
    // index, we always enumerate on subkeyindex 0
    //
    while(1) {
        Status = ZwEnumerateKey(
                    hKey,
                    0,
                    KeyBasicInformation,
                    TemporaryBuffer,
                    LengthTemporaryBuffer,
                    &ResultLength
                    );
        if(!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Make a duplicate of the subkey name because the name is
        // in TemporaryBuffer, which might get clobbered by recursive
        // calls to this routine.
        //
        SubkeyName = ExAllocatePool (PagedPool,
                                     ((wcslen (KeyInfo->Name) + 1) *
                                      sizeof (WCHAR)));
        if (!SubkeyName) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        wcscpy(SubkeyName, KeyInfo->Name);
        Status = CmDeleteKeyRecursive( hKey,
                                       SubkeyName,
                                       TemporaryBuffer,
                                       LengthTemporaryBuffer,
                                       TRUE);
        ExFreePool(SubkeyName);
        if(!NT_SUCCESS(Status)) {
            break;
        }
    }

    //
    // Check the status, if the status is anything other than
    // STATUS_NO_MORE_ENTRIES we failed in deleting some subkey,
    // so we cannot delete this key too
    //

    if( Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS (Status)) {
        ZwClose(hKey);
        return (Status);
    }

    //
    // else delete the current key if asked to do so
    //
    if( ThisKeyToo ) {
        Status = ZwDeleteKey (hKey);
    }

    ZwClose(hKey);
    return(Status);
}

NTSTATUS
CmpCreateHwProfileFriendlyName (
    IN HANDLE           IDConfigDB,
    IN ULONG            DockingState,
    IN ULONG            NewProfileNumber,
    OUT PUNICODE_STRING FriendlyName
    )
/*++

Routine Description:

    Create a new FriendlyName for a new Hardware Profile, given the DockState.
    If a new profile name based on the DockState cannot be created, an attempt
    is made to create a default FriendlyName based on NewProfileNumber.  If
    successful, a unicode string with the new profile friendlyName is created.
    It is the responsibility of the caller to free this using
    RtlFreeUnicodeString.  If unsuccesful, no string is returned.

Arguments:

    IDConfigDB:       Handle to the IDConfigDB registry key.

    DockingState:     The Docking State of the profile for which the new
                      FriendlyName is being created.  This should be one of:
                      HW_PROFILE_DOCKSTATE_DOCKED,
                      HW_PROFILE_DOCKSTATE_UNDOCKED, or
                      HW_PROFILE_DOCKSTATE_UNKNOWN

    NewProfileNumber: The number of the new profile being created.  If unable to
                      create a DockState specific FriendlyName, this value will
                      be used to create a (not-so) FriendlyName.

    FriendlyName:     Supplies a unicode string to receive the FriendlyName for this
                      new profile.  The caller is expected to free this with
                      RtlFreeUnicodeString.

Return:

    NTSTATUS code.    Currently returns STATUS_SUCCESS, or STATUS_UNSUCCESSFUL.

Notes:

    The new FriendlyName is generated from the DockState and appropriate
    counter, and may not necessarily be unique among the existing Hardware
    Profiles.

    The naming scheme used here (including the localized strings in the kernel
    message table) should be kept in sync with that provided to the user through
    the Hardware Profile control panel applet.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ANSI_STRING       ansiString;
    UNICODE_STRING    unicodeString;
    UNICODE_STRING    labelName, keyName;
    PMESSAGE_RESOURCE_ENTRY messageEntry;
    PLDR_DATA_TABLE_ENTRY dataTableEntry;
    ULONG             messageId;
    UCHAR             valueBuffer[256];
    WCHAR             friendlyNameBuffer[MAX_FRIENDLY_NAME_LENGTH/sizeof(WCHAR)];
    PKEY_VALUE_FULL_INFORMATION  keyValueInfo;
    ULONG             length, index;
    HANDLE            hardwareProfiles=NULL;
    OBJECT_ATTRIBUTES attributes;

    PAGED_CODE ();

    //
    // Make sure we were given a place to put the FriendlyName
    //
    if (!FriendlyName) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If we don't have a handle to IDConfigDB, try to assign a default
    // FriendlyName on the way out.
    //
    if (!IDConfigDB) {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Determine the appropriate message to use, based on the DockState.
    //
    if ((DockingState & HW_PROFILE_DOCKSTATE_UNKNOWN) == HW_PROFILE_DOCKSTATE_UNKNOWN){
        messageId = HARDWARE_PROFILE_UNKNOWN_STRING;
        RtlInitUnicodeString(&labelName, CM_HARDWARE_PROFILE_STR_UNKNOWN);
    } else if (DockingState & HW_PROFILE_DOCKSTATE_DOCKED) {
        messageId = HARDWARE_PROFILE_DOCKED_STRING;
        RtlInitUnicodeString(&labelName, CM_HARDWARE_PROFILE_STR_DOCKED);
    } else if (DockingState & HW_PROFILE_DOCKSTATE_UNDOCKED) {
        messageId = HARDWARE_PROFILE_UNDOCKED_STRING;
        RtlInitUnicodeString(&labelName, CM_HARDWARE_PROFILE_STR_UNDOCKED);
    } else {
        messageId = HARDWARE_PROFILE_UNKNOWN_STRING;
        RtlInitUnicodeString(&labelName, CM_HARDWARE_PROFILE_STR_UNKNOWN);
    }

    //
    // Find the message entry in the kernel's own message table.  KeLoaderBlock
    // is available when we're creating hardware profiles during system
    // initialization only; for profiles created thereafter, use the the first
    // entry of the PsLoadedModuleList to get the image base of the kernel.
    //
    if (KeLoaderBlock) {
        dataTableEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);
    } else if (PsLoadedModuleList.Flink) {
        dataTableEntry = CONTAINING_RECORD(PsLoadedModuleList.Flink,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);
    } else {
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    status = RtlFindMessage(dataTableEntry->DllBase,
                            (ULONG_PTR)11, // RT_MESSAGETABLE
                            MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT), // System default language
                            messageId,
                            &messageEntry);

    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    if(!(messageEntry->Flags & MESSAGE_RESOURCE_UNICODE)) {
        //
        // If the message is not unicode, convert to unicode.
        // Let the conversion routine allocate the buffer.
        //
        RtlInitAnsiString(&ansiString,messageEntry->Text);
        status = RtlAnsiStringToUnicodeString(&unicodeString,&ansiString,TRUE);
    } else {
        //
        // Message is already unicode. Make a copy.
        //
        status = RtlCreateUnicodeString(&unicodeString,(PWSTR)messageEntry->Text);
    }

    if(!NT_SUCCESS(status)) {
        goto Exit;
    }

    //
    // Strip the trailing CRLF.
    //
    if (unicodeString.Length  > 2 * sizeof(WCHAR)) {
        unicodeString.Length -= 2 * sizeof(WCHAR);
        unicodeString.Buffer[unicodeString.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    //
    // Check that the size of the label, with any numeric tag that may
    // potentially be added (up to 4 digits, preceded by a space) is not too
    // big.
    //
    if ((unicodeString.Length + 5*sizeof(WCHAR) + sizeof(UNICODE_NULL)) >
        MAX_FRIENDLY_NAME_LENGTH) {
        status = STATUS_UNSUCCESSFUL;
        goto Clean;
    }

    //
    // Open the Hardware Profiles key.
    //
    RtlInitUnicodeString(&keyName, CM_HARDWARE_PROFILE_STR_HARDWARE_PROFILES);
    InitializeObjectAttributes(&attributes,
                               &keyName,
                               OBJ_CASE_INSENSITIVE,
                               IDConfigDB,
                               NULL);
    status = ZwOpenKey(&hardwareProfiles,
                       KEY_READ,
                       &attributes);
    if (!NT_SUCCESS(status)) {
        hardwareProfiles = NULL;
        goto Clean;
    }

    //
    // Retrieve the counter of FriendlyNames we have previously assigned, based
    // on this DockState.
    //
    keyValueInfo = (PKEY_VALUE_FULL_INFORMATION) valueBuffer;
    status = ZwQueryValueKey(hardwareProfiles,
                             &labelName,
                             KeyValueFullInformation,
                             valueBuffer,
                             sizeof(valueBuffer),
                             &length);

    if (NT_SUCCESS(status) && (keyValueInfo->Type == REG_DWORD)) {
        //
        // Increment the counter.
        //
        index = (* (PULONG) ((PUCHAR)keyValueInfo + keyValueInfo->DataOffset));
        index++;
    } else {
        //
        // Missing or invalid counter value; start the counter at "1".
        //
        index = 1;
    }               

    //
    // Update the counter in the registry.
    //
    status = ZwSetValueKey(hardwareProfiles,
                           &labelName,
                           0,
                           REG_DWORD,
                           &index,
                           sizeof(index));
    if (!NT_SUCCESS(status)) {
        goto Clean;
    }

    //
    // Copy the FriendlyName, adding the index if necessary.
    //
    if ((messageId == HARDWARE_PROFILE_UNKNOWN_STRING) || (index > 1)) {
        swprintf(friendlyNameBuffer, L"%s %u",
                 unicodeString.Buffer, index);
    } else {
        wcscpy(friendlyNameBuffer, unicodeString.Buffer);
    }

 Clean:

    RtlFreeUnicodeString(&unicodeString);

    if (hardwareProfiles!=NULL) {
        ZwClose(hardwareProfiles);
    }

 Exit:

    if (!NT_SUCCESS(status)) {
        //
        // If we failed to assign a counter-based FriendlyName for whatever
        // reason, give the new profile a new (not so) friendly name as a last
        // resort.
        //
        swprintf (friendlyNameBuffer, L"%04d", NewProfileNumber);
        status = STATUS_SUCCESS;
    }

    //
    // Create the unicode string to return to the caller.
    //
    if (!RtlCreateUnicodeString(FriendlyName, (PWSTR)friendlyNameBuffer)) {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;

} // CmpCreateHwProfileFriendlyName
