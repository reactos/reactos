
/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    pnpmap.c

Abstract:

    This module contains the code that translates the device info returned from
    the PnP BIOS into root enumerated devices.

Author:

    Robert B. Nelson (RobertN) 22-Sep-1997

Environment:

    Kernel mode

Revision History :

--*/

#include "iop.h"
#pragma hdrstop
#include "pnpcvrt.h"
#include "pbios.h"

#if UMODETEST
#undef IsNEC_98
#define IsNEC_98 0
#endif

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'PpaM')
#endif

//
// Debugging stuff
//

#if DBG

#define MAPPER_ERROR       0x00000001
#define MAPPER_INFORMATION 0x00000002
#define MAPPER_PNP_ID      0x00000004
#define MAPPER_RESOURCES   0x00000008
#define MAPPER_REGISTRY    0x00000010
#define MAPPER_VERBOSE     0x00008000

ULONG PnPBiosMapperDebugMask =    MAPPER_ERROR       |
                               // MAPPER_INFORMATION |
                               // MAPPER_REGISTRY    |
                               // MAPPER_PNP_ID      |
                               // MAPPER_RESOURCES   |
                               // MAPPER_VERBOSE     |
                               0;

#define DebugPrint(X) PnPBiosDebugPrint X

VOID
PnPBiosDebugPrint(
    ULONG  DebugMask,
    PCCHAR DebugMessage,
    ...
    );

#else

#define DebugPrint(X)

#endif // DBG

#if UMODETEST
#define MULTIFUNCTION_KEY_NAME L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\TestSystem\\MultifunctionAdapter"
#define ENUMROOT_KEY_NAME L"\\Registry\\Machine\\System\\TestControlSet\\Enum\\Root"
#else
#define MULTIFUNCTION_KEY_NAME L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter"
#define ENUMROOT_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\Root"
#endif

#define BIOSINFO_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Biosinfo\\PNPBios"
#define BIOSINFO_VALUE_NAME L"DisableNodes"
#define DECODEINFO_VALUE_NAME L"FullDecodeChipsetOverride"

#define INSTANCE_ID_PREFIX      L"PnPBIOS_"

#define DEFAULT_STRING_SIZE     80
#define DEFAULT_VALUE_SIZE      80

#define DEFAULT_DEVICE_DESCRIPTION  L"Unknown device class"


#define EXCLUSION_ENTRY(a)  { a, sizeof(a) - sizeof(UNICODE_NULL) }

typedef struct  _EXCLUDED_PNPNODE  {
    PWCHAR  Id;
    ULONG   IdLength;
} EXCLUDED_PNPNODE, *PEXCLUDED_PNPNODE;

EXCLUDED_PNPNODE ExcludedDevices[] =  {
    EXCLUSION_ENTRY(L"*PNP03"),     // Keyboards
    EXCLUSION_ENTRY(L"*PNP0A"),     // PCI Busses
    EXCLUSION_ENTRY(L"*PNP0E"),     // PCMCIA Busses
    EXCLUSION_ENTRY(L"*PNP0F"),     // Mice
    EXCLUSION_ENTRY(L"*nEC13"),     // Keyboard for NEC98
    EXCLUSION_ENTRY(L"*nEC1E"),     // PCMCIA Busses for NEC98
    EXCLUSION_ENTRY(L"*nEC1F"),     // Mouse for NEC98
    EXCLUSION_ENTRY(L"*IBM3780"),   // IBM Trackpoint Mouse
    EXCLUSION_ENTRY(L"*IBM3781")    // IBM Trackpoint Mouse
};

#define EXCLUDED_DEVICES_COUNT  (sizeof(ExcludedDevices) / sizeof(ExcludedDevices[0]))

EXCLUDED_PNPNODE ExcludeIfDisabled[] = {
    EXCLUSION_ENTRY(L"*PNP0C01"),   // Motherboard resources
    EXCLUSION_ENTRY(L"*PNP0C02")    // Motherboard resources
};

#define EXCLUDE_DISABLED_COUNT  (sizeof(ExcludeIfDisabled) / sizeof(ExcludeIfDisabled[0]))

typedef struct _CLASSDATA {
    ULONG   Value;
    PWCHAR  Description;
} CLASSDATA;

CLASSDATA Class1Descriptions[] = {
    { 0x0000, L"SCSI Controller" },
    { 0x0100, L"IDE Controller" },
    { 0x0200, L"Floppy Controller" },
    { 0x0300, L"IPI Controller" },
    { 0x0400, L"RAID Controller" },
    { 0x8000, L"Other Mass Storage" }
};

CLASSDATA Class2Descriptions[] = {
    { 0x0000, L"Ethernet" },
    { 0x0100, L"Token ring" },
    { 0x0200, L"FDDI" },
    { 0x0300, L"ATM" },
    { 0x8000, L"Other network" }
};

CLASSDATA Class3Descriptions[] = {
    { 0x0000, L"VGA" },
    { 0x0001, L"SVGA" },
    { 0x0100, L"XGA" },
    { 0x8000, L"Other display" }
};

CLASSDATA Class4Descriptions[] = {
    { 0x0000, L"Video device" },
    { 0x0100, L"Audio device" },
    { 0x8000, L"Other multimedia" }
};

CLASSDATA Class5Descriptions[] = {
    { 0x0000, L"RAM memory" },
    { 0x0100, L"Flash memory" },
    { 0x8000, L"Other memory" }
};

CLASSDATA Class6Descriptions[] = {
    { 0x0000, L"HOST / PCI" },
    { 0x0100, L"PCI / ISA" },
    { 0x0200, L"PCI / EISA" },
    { 0x0300, L"PCI / MCA" },
    { 0x0400, L"PCI / PCI" },
    { 0x0500, L"PCI / PCMCIA" },
    { 0x0600, L"NuBus" },
    { 0x0700, L"Cardbus" },
    { 0x8000, L"Other bridge" }
};

CLASSDATA Class7Descriptions[] = {
    { 0x0000, L"XT Serial" },
    { 0x0001, L"16450" },
    { 0x0002, L"16550" },
    { 0x0100, L"Parallel output only" },
    { 0x0101, L"BiDi Parallel" },
    { 0x0102, L"ECP 1.x parallel" },
    { 0x8000, L"Other comm" }
};

CLASSDATA Class8Descriptions[] = {
    { 0x0000, L"Generic 8259" },
    { 0x0001, L"ISA PIC" },
    { 0x0002, L"EISA PIC" },
    { 0x0100, L"Generic 8237" },
    { 0x0101, L"ISA DMA" },
    { 0x0102, L"EISA DMA" },
    { 0x0200, L"Generic 8254" },
    { 0x0201, L"ISA timer" },
    { 0x0202, L"EISA timer" },
    { 0x0300, L"Generic RTC" },
    { 0x0301, L"ISA RTC" },
    { 0x8000, L"Other system device" }
};

CLASSDATA Class9Descriptions[] = {
    { 0x0000, L"Keyboard" },
    { 0x0100, L"Digitizer" },
    { 0x0200, L"Mouse" },
    { 0x8000, L"Other input" }
};

CLASSDATA Class10Descriptions[] = {
    { 0x0000, L"Generic dock" },
    { 0x8000, L"Other dock" },
};

CLASSDATA Class11Descriptions[] = {
    { 0x0000, L"386" },
    { 0x0100, L"486" },
    { 0x0200, L"Pentium" },
    { 0x1000, L"Alpha" },
    { 0x4000, L"Co-processor" }
};

CLASSDATA Class12Descriptions[] = {
    { 0x0000, L"Firewire" },
    { 0x0100, L"Access bus" },
    { 0x0200, L"SSA" },
    { 0x8000, L"Other serial bus" }
};

#define CLASSLIST_ENTRY(a)   { a, sizeof(a) / sizeof(a[0]) }

struct _CLASS_DESCRIPTIONS_LIST  {

    CLASSDATA *Descriptions;
    ULONG      Count;

}   ClassDescriptionsList[] =  {
    { NULL, 0 },
    CLASSLIST_ENTRY( Class1Descriptions ),
    CLASSLIST_ENTRY( Class2Descriptions ),
    CLASSLIST_ENTRY( Class3Descriptions ),
    CLASSLIST_ENTRY( Class4Descriptions ),
    CLASSLIST_ENTRY( Class5Descriptions ),
    CLASSLIST_ENTRY( Class6Descriptions ),
    CLASSLIST_ENTRY( Class7Descriptions ),
    CLASSLIST_ENTRY( Class8Descriptions ),
    CLASSLIST_ENTRY( Class9Descriptions ),
    CLASSLIST_ENTRY( Class10Descriptions ),
    CLASSLIST_ENTRY( Class11Descriptions ),
    CLASSLIST_ENTRY( Class12Descriptions )

};

#define CLASSLIST_COUNT  ( sizeof(ClassDescriptionsList) / sizeof(ClassDescriptionsList[0]) )

typedef struct _BIOS_DEVNODE_INFO  {
    WCHAR   ProductId[10];  // '*' + 7 char ID + NUL + NUL for REG_MULTI_SZ
    UCHAR   Handle;         // BIOS Node # / Handle
    UCHAR   TypeCode[3];
    USHORT  Attributes;
    PWSTR   Replaces;       // Instance ID of Root enumerated device being replaced

    PCM_RESOURCE_LIST               BootConfig;
    ULONG                           BootConfigLength;
    PIO_RESOURCE_REQUIREMENTS_LIST  BasicConfig;
    ULONG                           BasicConfigLength;
    PWSTR                           CompatibleIDs;  // REG_MULTI_SZ list of compatible IDs (including ProductId)
    ULONG                           CompatibleIDsLength;
    BOOLEAN                         FirmwareDisabled; // determined that it's disabled by firmware

}   BIOS_DEVNODE_INFO, *PBIOS_DEVNODE_INFO;

NTSTATUS
PbBiosResourcesToNtResources (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PUCHAR *BiosData,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *ReturnedList,
    OUT PULONG ReturnedLength
    );

VOID
PnPBiosExpandProductId(
    PUCHAR CompressedId,
    PWCHAR ProductIDStr
    );

NTSTATUS
PnPBiosIoResourceListToCmResourceList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResourceList,
    OUT PCM_RESOURCE_LIST *CmResourceList,
    OUT ULONG *CmResourceListSize
    );

NTSTATUS
PnPBiosExtractCompatibleIDs(
    IN  PUCHAR *DevNodeData,
    IN  ULONG DevNodeDataLength,
    OUT PWSTR *CompatibleIDs,
    OUT ULONG *CompatibleIDsLength
    );

NTSTATUS
PnPBiosTranslateInfo(
    IN VOID *BiosInfo,
    IN ULONG BiosInfoLength,
    OUT PBIOS_DEVNODE_INFO *DevNodeInfoList,
    OUT ULONG *NumberNodes
    );

LONG
PnPBiosFindMatchingDevNode(
    IN PWCHAR MapperName,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    );

NTSTATUS
PnPBiosEliminateDupes(
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    );

PWCHAR
PnPBiosGetDescription(
    IN PBIOS_DEVNODE_INFO DevNodeInfoEntry
    );

NTSTATUS
PnPBiosWriteInfo(
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    );

VOID
PnPBiosCopyIoDecode(
    IN HANDLE EnumRootKey,
    IN PBIOS_DEVNODE_INFO DevNodeInfo
    );

NTSTATUS
PnPBiosFreeDevNodeInfo(
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    );

NTSTATUS
PnPBiosCheckForHardwareDisabled(
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResourceList,
    IN OUT PBOOLEAN Disabled
    );

BOOLEAN
PnPBiosCheckForExclusion(
    IN PEXCLUDED_PNPNODE ExclusionArray,
    IN ULONG ExclusionCount,
    IN PWCHAR PnpDeviceName,
    IN PWCHAR PnpCompatIds
    );

NTSTATUS
PnPBiosMapper(
    VOID
    );

VOID
PpFilterNtResource (
    IN PWCHAR PnpDeviceName,
    PIO_RESOURCE_REQUIREMENTS_LIST ResReqList
    );

NTSTATUS
ComPortDBAdd(
    IN  HANDLE  DeviceParamKey,
    IN  PWSTR   PortName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PnPBiosExpandProductId)
#pragma alloc_text(INIT, PnPBiosIoResourceListToCmResourceList)
#pragma alloc_text(INIT, PnPBiosExtractCompatibleIDs)
#pragma alloc_text(INIT, PnPBiosTranslateInfo)
#pragma alloc_text(INIT, PnPBiosFindMatchingDevNode)
#pragma alloc_text(INIT, PnPBiosEliminateDupes)
#pragma alloc_text(INIT, PnPBiosGetDescription)
#pragma alloc_text(INIT, PnPBiosWriteInfo)
#pragma alloc_text(INIT, PnPBiosCopyIoDecode)
#pragma alloc_text(INIT, PnPBiosFreeDevNodeInfo)
#pragma alloc_text(INIT, PnPBiosCheckForHardwareDisabled)
#pragma alloc_text(INIT, PnPBiosCheckForExclusion)
#pragma alloc_text(INIT, PnPBiosMapper)
#pragma alloc_text(INIT, PpFilterNtResource)
#pragma alloc_text(PAGE, PnPBiosGetBiosInfo)
#endif

NTSTATUS
PnPBiosGetBiosInfo(
    OUT PVOID *BiosInfo,
    OUT ULONG *BiosInfoLength
    )
/*++

Routine Description:

    This function retrieves the PnP BIOS info accumulated by NTDETECT.COM and
    placed in the registry.

Arguments:

    BiosInfo - Set to a dynamically allocated block of information retrieved
        from the PnP BIOS by NTDETECT.  This block should be freed using
        ExFreePool.  The contents of the block are the PnP BIOS
        Installation Check Structure followed by the DevNode Structures reported
        by the BIOS.  The detailed format is documented in the PnP BIOS spec.

    BiosInfoLength - Length of the block whose address is stored in BiosInfo.


Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    UNICODE_STRING                  multifunctionKeyName, biosKeyName, valueName;
    HANDLE                          multifunctionKey = NULL, biosKey = NULL;
    PKEY_BASIC_INFORMATION          keyBasicInfo = NULL;
    ULONG                           keyBasicInfoLength;
    PKEY_VALUE_PARTIAL_INFORMATION  valueInfo = NULL;
    ULONG                           valueInfoLength;
    ULONG                           returnedLength;

    PCM_FULL_RESOURCE_DESCRIPTOR    biosValue;

    ULONG                           index;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;

    //
    // The PnP BIOS info is written to one of the subkeys under
    // MULTIFUNCTION_KEY_NAME.  The appropriate key is determined by
    // enumerating the subkeys and using the first one which has a value named
    // "Identifier" that is "PNP BIOS".
    //

    RtlInitUnicodeString(&multifunctionKeyName, MULTIFUNCTION_KEY_NAME);

    status = IopOpenRegistryKeyEx( &multifunctionKey,
                                   NULL,
                                   &multifunctionKeyName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {

        DebugPrint( (MAPPER_ERROR,
                    "Could not open %S, status = %8.8X\n",
                    MULTIFUNCTION_KEY_NAME,
                    status) );

        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate memory for key names returned from ZwEnumerateKey and values
    // returned from ZwQueryValueKey.
    //
    keyBasicInfoLength = sizeof(KEY_BASIC_INFORMATION) + DEFAULT_STRING_SIZE;
    keyBasicInfo = ExAllocatePool(PagedPool, keyBasicInfoLength + sizeof(UNICODE_NULL));

    if (keyBasicInfo == NULL)  {

        ZwClose( multifunctionKey );

        return STATUS_NO_MEMORY;
    }

    valueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + DEFAULT_STRING_SIZE;
    valueInfo = ExAllocatePool(PagedPool, valueInfoLength);

    if (valueInfo == NULL)  {

        ExFreePool( keyBasicInfo );

        ZwClose( multifunctionKey );

        return STATUS_NO_MEMORY;
    }

    //
    // Enumerate each key under HKLM\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter
    // to locate the one representing the PnP BIOS information.
    //
    for (index = 0; ; index++) {

        status = ZwEnumerateKey( multifunctionKey,   // handle of key to enumerate
                                 index,              // index of subkey to enumerate
                                 KeyBasicInformation,
                                 keyBasicInfo,
                                 keyBasicInfoLength,
                                 &returnedLength);

        if (!NT_SUCCESS(status)) {

            if (status != STATUS_NO_MORE_ENTRIES)  {

                DebugPrint( (MAPPER_ERROR,
                            "Could not enumerate under key %S, status = %8.8X\n",
                            MULTIFUNCTION_KEY_NAME,
                            status) );
            }

            break;
        }

        //
        // We found a subkey, NUL terminate the name and open the subkey.
        //
        keyBasicInfo->Name[ keyBasicInfo->NameLength / 2 ] = L'\0';

        RtlInitUnicodeString(&biosKeyName, keyBasicInfo->Name);

        status = IopOpenRegistryKeyEx( &biosKey,
                                       multifunctionKey,
                                       &biosKeyName,
                                       KEY_READ
                                       );

        if (!NT_SUCCESS(status)) {

            DebugPrint( (MAPPER_ERROR,
                        "Could not open registry key %S\\%S, status = %8.8X\n",
                        MULTIFUNCTION_KEY_NAME,
                        keyBasicInfo->Name,
                        status) );
            break;
        }

        //
        // Now we need to check the Identifier value in the subkey to see if
        // it is PNP BIOS.
        //
        RtlInitUnicodeString(&valueName, L"Identifier");

        status = ZwQueryValueKey( biosKey,
                                  &valueName,
                                  KeyValuePartialInformation,
                                  valueInfo,
                                  valueInfoLength,
                                  &returnedLength);


        // lets see if its the PNP BIOS identifier
        if (NT_SUCCESS(status)) {

            if (wcscmp((PWSTR)valueInfo->Data, L"PNP BIOS") == 0) {

                //
                // We found the PnP BIOS subkey, retrieve the BIOS info which
                // is stored in the "Configuration Data" value.
                //
                // We'll start off with our default value buffer and increase
                // its size if necessary.
                //

                RtlInitUnicodeString(&valueName, L"Configuration Data");

                status = ZwQueryValueKey( biosKey,
                                          &valueName,
                                          KeyValuePartialInformation,
                                          valueInfo,
                                          valueInfoLength,
                                          &returnedLength);

                if (!NT_SUCCESS(status)) {

                    if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW) {

                        //
                        // The default buffer was too small, free it and reallocate
                        // it to the required size.
                        //
                        ExFreePool( valueInfo );

                        valueInfoLength = returnedLength;
                        valueInfo = ExAllocatePool( PagedPool, valueInfoLength );

                        if (valueInfo != NULL)  {

                            status = ZwQueryValueKey( biosKey,
                                                      &valueName,
                                                      KeyValuePartialInformation,
                                                      valueInfo,
                                                      valueInfoLength,
                                                      &returnedLength );
                        } else {

                            status = STATUS_NO_MEMORY;
                        }
                    }
                }

                if (NT_SUCCESS(status)) {

                    //
                    // We now have the PnP BIOS data but it is buried inside
                    // the resource structures.  Do some consistency checks and
                    // then extract it into its own buffer.
                    //

                    ASSERT(valueInfo->Type == REG_FULL_RESOURCE_DESCRIPTOR);

                    biosValue = (PCM_FULL_RESOURCE_DESCRIPTOR)valueInfo->Data;

                    //
                    // BUGBUG - The WMI folks added another list so we should
                    // search for the PnPBIOS one, but for now the BIOS one is
                    // always first.
                    //
                    //ASSERT(biosValue->PartialResourceList.Count == 1);

                    *BiosInfoLength = biosValue->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize;
                    *BiosInfo = ExAllocatePool(PagedPool, *BiosInfoLength);

                    if (*BiosInfo != NULL) {

                        RtlCopyMemory( *BiosInfo,
                                       &biosValue->PartialResourceList.PartialDescriptors[1],
                                       *BiosInfoLength );

                        status = STATUS_SUCCESS;

                    } else {

                        *BiosInfoLength = 0;

                        status = STATUS_NO_MEMORY;
                    }

                } else {

                    DebugPrint( (MAPPER_ERROR,
                                "Error retrieving %S\\%S\\Configuration Data, status = %8.8X\n",
                                MULTIFUNCTION_KEY_NAME,
                                keyBasicInfo->Name,
                                status) );
                }

                //
                // We found the PnP BIOS entry, so close the key handle and
                // return.
                //

                ZwClose(biosKey);

                break;
            }
        }

        //
        // That wasn't it so close this handle and try the next subkey.
        //
        ZwClose(biosKey);
    }

    //
    // Cleanup the dynamically allocated temporary buffers.
    //

    if (valueInfo != NULL) {

        ExFreePool(valueInfo);
    }

    if (keyBasicInfo != NULL) {

        ExFreePool(keyBasicInfo);
    }

    ZwClose(multifunctionKey);

    return status;
}

VOID
PnPBiosExpandProductId(
    PUCHAR CompressedId,
    PWCHAR ProductIDStr
    )
/*++

Routine Description:

    This function expands a PnP Device ID from the 4 byte compressed form into
    an 7 character unicode string.  The string is then NUL terminated.

Arguments:

    CompressedId - Pointer to the 4 byte compressed Device ID as defined in the
        PnP Specification.

    ProductIDStr - Pointer to the 16 byte buffer in which the unicode string
        version of the ID is placed.


Return Value:

    NONE.

--*/
{
    static CHAR HexDigits[16] = "0123456789ABCDEF";

    ProductIDStr[0] = (CompressedId[0] >> 2) + 0x40;
    ProductIDStr[1] = (((CompressedId[0] & 0x03) << 3) | (CompressedId[1] >> 5)) + 0x40;
    ProductIDStr[2] = (CompressedId[1] & 0x1f) + 0x40;
    ProductIDStr[3] = HexDigits[CompressedId[2] >> 4];
    ProductIDStr[4] = HexDigits[CompressedId[2] & 0x0F];
    ProductIDStr[5] = HexDigits[CompressedId[3] >> 4];
    ProductIDStr[6] = HexDigits[CompressedId[3] & 0x0F];
    ProductIDStr[7] = 0x00;
}

BOOLEAN
PnPBiosIgnoreNode (
    PWCHAR PnpID,
    PWCHAR excludeNodes
    )
{
    BOOLEAN bRet=FALSE;
    ULONG   keyLen;
    PWCHAR  pTmp;

    ASSERT (excludeNodes);

    //
    //excludeNodes is multi-sz, so walk through each one and check it.
    //
    pTmp=excludeNodes;

    while (*pTmp != '\0') {

        keyLen = wcslen (pTmp);

        if (RtlCompareMemory (PnpID,pTmp,keyLen*sizeof (WCHAR)) == keyLen*sizeof (WCHAR)) {
            bRet=TRUE;
            break;
        }
        pTmp=pTmp+keyLen+1;

    }


    return bRet;
}

VOID
PnPGetDevnodeExcludeList (
    OUT  PKEY_VALUE_FULL_INFORMATION  *ExcludeList
    )
{
    UNICODE_STRING biosKeyName;
    HANDLE  biosKey;
    NTSTATUS status;

    RtlInitUnicodeString(&biosKeyName, BIOSINFO_KEY_NAME);
    status = IopOpenRegistryKeyEx( &biosKey,
                                   NULL,
                                   &biosKeyName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {

        //
        // Don't really need to complain, likely not there.
        //
        return;
    }

    (VOID) IopGetRegistryValue (biosKey,BIOSINFO_VALUE_NAME,ExcludeList);

    ZwClose (biosKey);
}

BOOLEAN
PnPCheckFixedIoOverrideDecodes(
    VOID
    )
{
    PKEY_VALUE_FULL_INFORMATION decodeFlagInfo=NULL;
    UNICODE_STRING biosKeyName;
    HANDLE  biosKey;
    NTSTATUS status;
    BOOLEAN overrideDecodes = FALSE;

    RtlInitUnicodeString(&biosKeyName, BIOSINFO_KEY_NAME);
    status = IopOpenRegistryKeyEx( &biosKey,
                                   NULL,
                                   &biosKeyName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {

        //
        // Don't really need to complain, likely not there.
        //
        return FALSE;
    }

    status = IopGetRegistryValue(biosKey, DECODEINFO_VALUE_NAME, &decodeFlagInfo);

    if (NT_SUCCESS(status)) {

        ASSERT(decodeFlagInfo->Type == REG_DWORD);
        if (decodeFlagInfo->DataLength == sizeof(ULONG)) {

            overrideDecodes = (BOOLEAN) *((PULONG) decodeFlagInfo->Name);
        }
    }

    if (decodeFlagInfo) {

        ExFreePool(decodeFlagInfo);
    }

    ZwClose (biosKey);

    return overrideDecodes;
}

BOOLEAN
PnPBiosCheckForExclusion(
    IN EXCLUDED_PNPNODE *Exclusions,
    IN ULONG  ExclusionCount,
    IN PWCHAR PnpDeviceName,
    IN PWCHAR PnpCompatIds
    )
{
    PWCHAR idPtr;
    ULONG exclusionIndex;

    for (exclusionIndex = 0; exclusionIndex < ExclusionCount; exclusionIndex++) {

        idPtr = PnpDeviceName;

        if (RtlCompareMemory( idPtr,
                              Exclusions[ exclusionIndex ].Id,
                              Exclusions[ exclusionIndex ].IdLength) != Exclusions[ exclusionIndex ].IdLength )  {

            idPtr = PnpCompatIds;

            if (idPtr != NULL)  {

                while (*idPtr != '\0') {

                    if (RtlCompareMemory( idPtr,
                                          Exclusions[ exclusionIndex ].Id,
                                          Exclusions[ exclusionIndex ].IdLength) == Exclusions[ exclusionIndex ].IdLength )  {

                        break;
                    }

                    idPtr += 9;
                }

                if (*idPtr == '\0') {

                    idPtr = NULL;
                }
            }
        }

        if (idPtr != NULL)  {

            break;
        }
    }

    if (exclusionIndex < ExclusionCount) {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
PnPBiosIoResourceListToCmResourceList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResourceList,
    OUT PCM_RESOURCE_LIST *CmResourceList,
    OUT ULONG *CmResourceListSize
    )
/*++

Routine Description:

    Converts an IO_RESOURCE_REQUIREMENTS_LIST into a CM_RESOURCE_LIST.  This
    routine is used to convert the list of resources currently being used by a
    device into a form suitable for writing to the BootConfig registry value.

Arguments:

    IoResourceList - Pointer to the input list.

    CmResourceList - Pointer to a PCM_RESOURCE_LIST which is set to the
        dynamically allocated and filled in using the data from IoResourceList.

    CmResourceListSize - Pointer to a variable which is set to the size in bytes
        of the dynamically allocated *CmResourceList.

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    PCM_PARTIAL_RESOURCE_LIST       partialList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;
    PIO_RESOURCE_DESCRIPTOR         ioDescriptor;
    ULONG                           descIndex;

    //
    // Since this routine is only used to translate the allocated resources
    // returned by the PnP BIOS, we can assume that there is only 1 alternative
    // list
    //

    ASSERT(IoResourceList->AlternativeLists == 1);

    //
    // Calculate the size of the translated list and allocate memory for it.
    //
    *CmResourceListSize = sizeof(CM_RESOURCE_LIST) +
                          (IoResourceList->AlternativeLists - 1) * sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                          (IoResourceList->List[0].Count - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    *CmResourceList = ExAllocatePool( PagedPool, *CmResourceListSize );

    if (*CmResourceList == NULL) {

        *CmResourceListSize = 0;

        return STATUS_NO_MEMORY;
    }

    //
    // Copy the header info from the requirements list to the resource list.
    //
    (*CmResourceList)->Count = 1;

    (*CmResourceList)->List[ 0 ].InterfaceType = IoResourceList->InterfaceType;
    (*CmResourceList)->List[ 0 ].BusNumber = IoResourceList->BusNumber;

    partialList = &(*CmResourceList)->List[ 0 ].PartialResourceList;

    partialList->Version = IoResourceList->List[ 0 ].Version;
    partialList->Revision = IoResourceList->List[ 0 ].Revision;
    partialList->Count = 0;

    //
    // Translate each resource descriptor, currently we only handle ports,
    // memory, interrupts, and dma.  The current implementation of the routine
    // which converts from ISA PnP Resource data to IO_RESOURCE_REQUIREMENTS
    // won't generate any other descriptor types given the data returned from
    // the BIOS.
    //

    partialDescriptor = &partialList->PartialDescriptors[ 0 ];
    for (descIndex = 0; descIndex < IoResourceList->List[ 0 ].Count; descIndex++) {

        ioDescriptor = &IoResourceList->List[ 0 ].Descriptors[ descIndex ];

        switch (ioDescriptor->Type) {

        case CmResourceTypePort:
            partialDescriptor->u.Port.Start = ioDescriptor->u.Port.MinimumAddress;
            partialDescriptor->u.Port.Length = ioDescriptor->u.Port.Length;
            break;

        case CmResourceTypeInterrupt:
            if (ioDescriptor->u.Interrupt.MinimumVector == (ULONG)(IsNEC_98 ? 7 : 2) ) {
                *CmResourceListSize -= sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                continue;
            }
            partialDescriptor->u.Interrupt.Level = ioDescriptor->u.Interrupt.MinimumVector;
            partialDescriptor->u.Interrupt.Vector = ioDescriptor->u.Interrupt.MinimumVector;
            partialDescriptor->u.Interrupt.Affinity = ~0;
            break;

        case CmResourceTypeMemory:
            partialDescriptor->u.Memory.Start = ioDescriptor->u.Memory.MinimumAddress;
            partialDescriptor->u.Memory.Length = ioDescriptor->u.Memory.Length;
            break;

        case CmResourceTypeDma:
            partialDescriptor->u.Dma.Channel = ioDescriptor->u.Dma.MinimumChannel;
            partialDescriptor->u.Dma.Port = 0;
            partialDescriptor->u.Dma.Reserved1 = 0;
            break;

        default:
            DebugPrint( (MAPPER_ERROR,
                        "Unexpected ResourceType (%d) in I/O Descriptor\n",
                        ioDescriptor->Type) );

#if DBG
            // DbgBreakPoint();
#endif
            break;
        }

        partialDescriptor->Type = ioDescriptor->Type;
        partialDescriptor->ShareDisposition = ioDescriptor->ShareDisposition;
        partialDescriptor->Flags = ioDescriptor->Flags;
        partialDescriptor++;

        partialList->Count++;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PnPBiosExtractCompatibleIDs(
    IN  PUCHAR *DevNodeData,
    IN  ULONG DevNodeDataLength,
    OUT PWSTR *CompatibleIDs,
    OUT ULONG *CompatibleIDsLength
    )
{
    PWCHAR  idPtr;
    PUCHAR  currentPtr, endPtr;
    UCHAR   tagName;
    ULONG   increment;
    ULONG   compatibleCount;

    endPtr = &(*DevNodeData)[DevNodeDataLength];

    compatibleCount = 0;

    for (currentPtr = *DevNodeData; currentPtr < endPtr; currentPtr += increment) {

        tagName = *currentPtr;

        if (tagName == TAG_COMPLETE_END)  {

            break;
        }

        //
        // Determine the size of the BIOS resource descriptor
        //

        if (!(tagName & LARGE_RESOURCE_TAG)) {
            increment = (USHORT)(tagName & SMALL_TAG_SIZE_MASK);
            increment++;     // length of small tag
            tagName &= SMALL_TAG_MASK;
        } else {
            increment = *(USHORT UNALIGNED *)(&currentPtr[1]);
            increment += 3;     // length of large tag
        }

        if (tagName == TAG_COMPATIBLE_ID) {

            compatibleCount++;
        }
    }

    if (compatibleCount == 0) {
        *CompatibleIDs = NULL;
        *CompatibleIDsLength = 0;

        return STATUS_SUCCESS;
    }

    *CompatibleIDsLength = (compatibleCount * 9 + 1) * sizeof(WCHAR);
    *CompatibleIDs = ExAllocatePool(PagedPool, *CompatibleIDsLength);

    if (*CompatibleIDs == NULL)  {

        *CompatibleIDsLength = 0;
        return STATUS_NO_MEMORY;
    }

    idPtr = *CompatibleIDs;

    for (currentPtr = *DevNodeData; currentPtr < endPtr; currentPtr += increment) {

        tagName = *currentPtr;

        if (tagName == TAG_COMPLETE_END)  {

            break;
        }

        //
        // Determine the size of the BIOS resource descriptor
        //

        if (!(tagName & LARGE_RESOURCE_TAG)) {
            increment = (USHORT)(tagName & SMALL_TAG_SIZE_MASK);
            increment++;     // length of small tag
            tagName &= SMALL_TAG_MASK;
        } else {
            increment = *(USHORT UNALIGNED *)(&currentPtr[1]);
            increment += 3;     // length of large tag
        }

        if (tagName == TAG_COMPATIBLE_ID) {

            *idPtr = '*';
            PnPBiosExpandProductId(&currentPtr[1], &idPtr[1]);
            idPtr += 9;
        }
    }

    *idPtr++ = '\0';  // Extra NUL for REG_MULTI_SZ
    *CompatibleIDsLength = (ULONG)(idPtr - *CompatibleIDs) * sizeof(WCHAR);

    return STATUS_SUCCESS;
}

NTSTATUS
PnPBiosTranslateInfo(
    IN VOID *BiosInfo,
    IN ULONG BiosInfoLength,
    OUT PBIOS_DEVNODE_INFO *DevNodeInfoList,
    OUT ULONG *NumberNodes
    )
/*++

Routine Description:

    Translates the devnode info retrieved from the BIOS.

Arguments:

    BiosInfo - The PnP BIOS Installation Check Structure followed by the
        DevNode Structures reported by the BIOS.  The detailed format is
        documented in the PnP BIOS spec.

    BiosInfoLength - Length in bytes of the block whose address is stored in
        BiosInfo.

    DevNodeInfoList - Dynamically allocated array of BIOS_DEVNODE_INFO
        structures, one for each device reported by the BIOS.  The information
        supplied by the BIOS: device ID, type, current resources, and supported
        configurations is converted into a more useful format.  For example the
        current resource allocation is converted from ISA PnP descriptors into
        an IO_RESOURCE_REQUIREMENTS_LIST and then into a CM_RESOURCE_LIST for
        storing into the BootConfig registry value.

    NumberNodes - Number of BIOS_DEVNODE_INFO elements pointed to by
        DevNodeInfoList.

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    PCM_PNP_BIOS_INSTALLATION_CHECK biosInstallCheck;
    PCM_PNP_BIOS_DEVICE_NODE        devNodeHeader;
    PBIOS_DEVNODE_INFO              devNodeInfo;
    PKEY_VALUE_FULL_INFORMATION     excludeList=NULL;

    PIO_RESOURCE_REQUIREMENTS_LIST  tempResReqList;

    PUCHAR                          currentPtr;
    LONG                            lengthRemaining;

    LONG                            remainingNodeLength;

    ULONG                           numNodes;
    ULONG                           nodeIndex;
    PUCHAR                          configPtr;
    ULONG                           configListLength;
    NTSTATUS                        status;
    ULONG                           convertFlags = 0;

    //
    // Make sure the data is at least large enough to hold the BIOS Installation
    // Check structure and check that the PnP signature is correct.
    //
    if (BiosInfoLength < sizeof(CM_PNP_BIOS_INSTALLATION_CHECK)) {

        DebugPrint( (MAPPER_ERROR,
                    "BiosInfoLength (%d) is smaller than sizeof(PNPBIOS_INSTALLATION_CHECK) (%d)\n",
                    BiosInfoLength,
                    sizeof(CM_PNP_BIOS_INSTALLATION_CHECK)) );

        return STATUS_UNSUCCESSFUL;
    }

    biosInstallCheck = (PCM_PNP_BIOS_INSTALLATION_CHECK)BiosInfo;

    if (biosInstallCheck->Signature[0] != '$' ||
        biosInstallCheck->Signature[1] != 'P' ||
        biosInstallCheck->Signature[2] != 'n' ||
        biosInstallCheck->Signature[3] != 'P') {

        return STATUS_UNSUCCESSFUL;
    }

    //
    // First scan the data and count the devnodes to determine the size of our
    // allocated data structures.
    //
    currentPtr = (PUCHAR)BiosInfo + biosInstallCheck->Length;
    lengthRemaining = BiosInfoLength - biosInstallCheck->Length;

    for (numNodes = 0; lengthRemaining > sizeof(CM_PNP_BIOS_DEVICE_NODE); numNodes++) {

        devNodeHeader = (PCM_PNP_BIOS_DEVICE_NODE)currentPtr;

        if (devNodeHeader->Size > lengthRemaining) {

            DebugPrint( (MAPPER_ERROR,
                        "Node # %d, invalid size (%d), length remaining (%d)\n",
                        devNodeHeader->Node,
                        devNodeHeader->Size,
                        lengthRemaining) );

            return STATUS_UNSUCCESSFUL;
        }

        currentPtr += devNodeHeader->Size;
        lengthRemaining -= devNodeHeader->Size;
    }

    //
    // Allocate the list of translated devnodes.
    //
    devNodeInfo = ExAllocatePool( PagedPool, numNodes * sizeof(BIOS_DEVNODE_INFO) );

    if (devNodeInfo == NULL) {

        return STATUS_NO_MEMORY;
    }

    //
    // Should we force all fixed IO decodes to 16bit?
    //
    if (PnPCheckFixedIoOverrideDecodes()) {

        convertFlags |= PPCONVERTFLAG_FORCE_FIXED_IO_16BIT_DECODE;
    }

    //
    // Now scan the data translating the info for each devnode into an entry in
    // our devNodeInfo array.
    //

    currentPtr = (PUCHAR)BiosInfo + biosInstallCheck->Length;
    lengthRemaining = BiosInfoLength - biosInstallCheck->Length;

    for (nodeIndex = 0; nodeIndex < numNodes; nodeIndex++) {

        devNodeHeader = (PCM_PNP_BIOS_DEVICE_NODE)currentPtr;

        if (devNodeHeader->Size > lengthRemaining) {

            DebugPrint( (MAPPER_ERROR,
                        "Node # %d, invalid size (%d), length remaining (%d)\n",
                        devNodeHeader->Node,
                        devNodeHeader->Size,
                        lengthRemaining) );

            break;
        }

        //
        // We use the Product ID field as the DeviceID key name.  So we insert
        // an initial asterisk so we don't have to copy and mangle it later.
        //
        devNodeInfo[nodeIndex].ProductId[0] = '*';

        PnPBiosExpandProductId((PUCHAR)&devNodeHeader->ProductId, &devNodeInfo[nodeIndex].ProductId[1]);

        devNodeInfo[nodeIndex].ProductId[9] = '\0';  // Extra NUL for REG_MULTI_SZ

        //
        // The handle is used as part of the Instance ID
        devNodeInfo[nodeIndex].Handle = devNodeHeader->Node;

        //
        // The type code and attributes aren't currently used but are copied
        // for completeness.
        //
        RtlCopyMemory( &devNodeInfo[nodeIndex].TypeCode,
                       devNodeHeader->DeviceType,
                       sizeof(devNodeInfo[nodeIndex].TypeCode) );

        devNodeInfo[nodeIndex].Attributes = devNodeHeader->DeviceAttributes;

        //
        // Replaces will eventually be set to the path of the Firmware
        // Enumerated devnode which duplicates this one (if a duplicate exists).
        //
        devNodeInfo[nodeIndex].Replaces = NULL;

        //
        // CompatibleIDs will be set to the list of compatible IDs.
        //
        devNodeInfo[nodeIndex].CompatibleIDs = NULL;

        //
        // Convert the allocated resources from ISA PnP resource descriptor
        // format to an IO_RESOURCE_REQUIREMENTS_LIST.
        //
        configPtr = currentPtr + sizeof(*devNodeHeader);
        remainingNodeLength = devNodeHeader->Size - sizeof(*devNodeHeader);

        devNodeInfo[nodeIndex].BootConfig = NULL;
        devNodeInfo[nodeIndex].FirmwareDisabled = FALSE;

        status = PpBiosResourcesToNtResources( 0,            /* BusNumber */
                                               0,            /* SlotNumber */
                                               &configPtr,   /* BiosData */
                                               convertFlags, /* ConvertFlags */
                                               &tempResReqList, /* ReturnedList */
                                               &configListLength);    /* ReturnedLength */

        remainingNodeLength = devNodeHeader->Size - (LONG)(configPtr - (PUCHAR)devNodeHeader);

        if (NT_SUCCESS( status )) {

            if (tempResReqList != NULL) {

                PpFilterNtResource (
                    devNodeInfo[nodeIndex].ProductId,
                    tempResReqList
                );

                //
                // Now we need to convert from a IO_RESOURCE_REQUIREMENTS_LIST to a
                // CM_RESOURCE_LIST.
                //
                status = PnPBiosIoResourceListToCmResourceList( tempResReqList,
                                                                &devNodeInfo[nodeIndex].BootConfig,
                                                                &devNodeInfo[nodeIndex].BootConfigLength );

                status = PnPBiosCheckForHardwareDisabled(tempResReqList,&devNodeInfo[nodeIndex].FirmwareDisabled);

                ExFreePool( tempResReqList );

            }

        } else {

            DebugPrint( (MAPPER_ERROR,
                        "Error converting allocated resources for devnode # %d, status = %8.8X\n",
                        devNodeInfo[nodeIndex].Handle,
                        status) );
        }

        //
        // Convert the supported resource configurations from ISA PnP resource
        // descriptor format to an IO_RESOURCE_REQUIREMENTS_LIST.
        //
        status = PpBiosResourcesToNtResources( 0,            /* BusNumber */
                                               0,            /* SlotNumber */
                                               &configPtr,   /* BiosData */
                                               convertFlags | PPCONVERTFLAG_SET_RESTART_LCPRI, /* ConvertFlags */
                                               &devNodeInfo[nodeIndex].BasicConfig, /* ReturnedList */
                                               &devNodeInfo[nodeIndex].BasicConfigLength );  /* ReturnedLength */

        remainingNodeLength = devNodeHeader->Size - (LONG)(configPtr - (PUCHAR)devNodeHeader);

        if (!NT_SUCCESS( status )) {

            devNodeInfo[nodeIndex].BasicConfig = NULL;

            DebugPrint( (MAPPER_ERROR,
                        "Error converting allowed resources for devnode # %d, status = %8.8X\n",
                        devNodeInfo[nodeIndex].Handle,
                        status) );
        } else {

            PpFilterNtResource (
                devNodeInfo[nodeIndex].ProductId,
                devNodeInfo[nodeIndex].BasicConfig
            );
        }

        //
        // Convert the list of compatible IDs if present
        //

        ASSERT(remainingNodeLength >= 0);

        status = PnPBiosExtractCompatibleIDs( &configPtr,       // BiosData
                                              (ULONG)remainingNodeLength,
                                              &devNodeInfo[nodeIndex].CompatibleIDs,
                                              &devNodeInfo[nodeIndex].CompatibleIDsLength );

        currentPtr += devNodeHeader->Size;
        lengthRemaining -= devNodeHeader->Size;

    }

    *DevNodeInfoList = devNodeInfo;
    *NumberNodes = numNodes;
    return STATUS_SUCCESS;
}

LONG
PnPBiosFindMatchingDevNode(
    IN PWCHAR MapperName,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    )
/*++

Routine Description:

    Given a list of resources this routine finds an entry in the
    DevNodeInfoList whose BootConfig resources match.  A match is defined as
    having at least overlapping I/O Ports or Memory Ranges.  If ResourceList doesn't
    include any I/O Ports or Memory Ranges then a match is defined as exactly
    the same interrupts and/or DMA channels.

    This routine is used to find PnP BIOS reported devices which match devices
    created by the Firmware Mapper.

Arguments:

    ResourceList - Pointer to CM_RESOURCE_LIST describing the resources
        currently used by the device for which a match is being searched.

    DevNodeInfoList - Array of BIOS_DEVNODE_INFO structures, one for each device
        reported by the BIOS.

    NumberNodes - Number of BIOS_DEVNODE_INFO elements pointed to by
        DevNodeInfoList.


Return Value:

    Index of the entry in DevNodeInfoList whose BootConfig matches the resources
    listed in ResourceList.  If no matching entry is found then -1 is returned.

--*/
{
    PCM_PARTIAL_RESOURCE_LIST       sourceList;
    PCM_PARTIAL_RESOURCE_LIST       targetList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR sourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR targetDescriptor;
    ULONG                           nodeIndex, sourceIndex, targetIndex;
    LONG                            firstMatch = -1;
    LONG                            bestMatch = -1;
    ULONG                           numResourcesMatch;
    ULONG                           score, possibleScore, bestScore = 0;
    PWCHAR                          idPtr;
    BOOLEAN                         idsMatch;
    BOOLEAN                         bestIdsMatch = FALSE;

#if DEBUG_DUP_MATCH
    CHAR                            sourceMapping[256];
    CHAR                            targetMapping[256];
#endif

    //
    // In order to simplify the problem we assume there is only one list.  This
    // assumption holds true in the BootConfig structures generated by the
    // current firmware mapper.
    //
    ASSERT( ResourceList->Count == 1 );

    sourceList = &ResourceList->List[0].PartialResourceList;

#if DEBUG_DUP_MATCH
    //
    // For debugging purposes we keep track of which resource entries map to
    // each other.  These relationships are stored in a fixed CHAR array, thus
    // the restriction on the number of descriptors.
    //
    ASSERT( sourceList->Count < 255 );
#endif

    //
    // Loop through each devnode and try and match it to the source resource
    // list.
    //
    for (nodeIndex = 0; nodeIndex < NumberNodes; nodeIndex++) {

        if (DevNodeInfoList[ nodeIndex ].BootConfig == NULL) {

            continue;
        }

        //
        // We found at least one potential match.  Let's double check if
        // the PNP ids also match.  We use a lack of ID match to disqualify
        // entries which don't match at least I/O ports or memory.
        //

        idPtr = DevNodeInfoList[ nodeIndex ].ProductId;

        if (RtlCompareMemory( idPtr, MapperName, 12 ) != 12) {

            idPtr = DevNodeInfoList[ nodeIndex ].CompatibleIDs;

            if (idPtr != NULL) {

                while (*idPtr != '\0') {

                    if (RtlCompareMemory( idPtr, MapperName, 12 ) == 12) {

                        break;
                    }

                    idPtr += 9;
                }

                if (*idPtr == '\0') {

                    idPtr = NULL;
                }
            }
        }

        idsMatch = idPtr != NULL;

        ASSERT( DevNodeInfoList[ nodeIndex ].BootConfig->Count == 1 );

        targetList = &DevNodeInfoList[ nodeIndex ].BootConfig->List[0].PartialResourceList;

#if DEBUG_DUP_MATCH
        RtlFillMemory( sourceMapping, sizeof(sourceMapping), -1 );
        RtlFillMemory( targetMapping, sizeof(targetMapping), -1 );
#endif

        numResourcesMatch = 0;
        possibleScore = 0;
        score = 0;

        //
        // Loop through each source descriptor (resource) and try and match it
        // to one of this devnode's descriptors.
        //

        for (sourceIndex = 0; sourceIndex < sourceList->Count; sourceIndex++) {

            sourceDescriptor = &sourceList->PartialDescriptors[sourceIndex];

            //
            // We are recalculating the possible score unnecessarily each time
            // we process a devnode.  We might save a small amount of time by
            // looping through the source descriptors once at the beginning but
            // its not clear it would make all that much difference given the
            // few devices reported by the BIOS.
            //

            switch (sourceDescriptor->Type) {

            case CmResourceTypePort:
                possibleScore += 0x1100;
                break;

            case CmResourceTypeInterrupt:
                possibleScore += 0x0001;
                break;

            case CmResourceTypeMemory:
                possibleScore += 0x1100;
                break;

            case CmResourceTypeDma:
                possibleScore += 0x0010;
                break;

            default:
                continue;
            }

            //
            // Try to find a resource in the target devnode which matches the
            // current source resource.
            //
            for (targetIndex = 0; targetIndex < targetList->Count; targetIndex++) {

                targetDescriptor = &targetList->PartialDescriptors[targetIndex];

                if (sourceDescriptor->Type == targetDescriptor->Type) {
                    switch (sourceDescriptor->Type) {
                    case CmResourceTypePort:
                        if ((sourceDescriptor->u.Port.Start.LowPart + sourceDescriptor->u.Port.Length) <=
                             targetDescriptor->u.Port.Start.LowPart ||
                            (targetDescriptor->u.Port.Start.LowPart + targetDescriptor->u.Port.Length) <=
                             sourceDescriptor->u.Port.Start.LowPart) {
                            continue;
                        }
                        if (sourceDescriptor->u.Port.Start.LowPart ==
                                targetDescriptor->u.Port.Start.LowPart &&
                            sourceDescriptor->u.Port.Length ==
                                targetDescriptor->u.Port.Length) {

                            score += 0x1100;

                        } else {

                            DebugPrint( (MAPPER_INFORMATION,
                                        "Overlapping port resources, source = %4.4X-%4.4X, target = %4.4X-%4.4X\n",
                                        sourceDescriptor->u.Port.Start.LowPart,
                                        sourceDescriptor->u.Port.Start.LowPart + sourceDescriptor->u.Port.Length - 1,
                                        targetDescriptor->u.Port.Start.LowPart,
                                        targetDescriptor->u.Port.Start.LowPart + targetDescriptor->u.Port.Length - 1) );

                            score += 0x1000;

                        }
                        break;

                    case CmResourceTypeInterrupt:
                        if (sourceDescriptor->u.Interrupt.Level !=
                            targetDescriptor->u.Interrupt.Level) {
                            continue;
                        }
                        score += 0x0001;
                        break;

                    case CmResourceTypeMemory:
                        if ((sourceDescriptor->u.Memory.Start.LowPart + sourceDescriptor->u.Memory.Length) <=
                             targetDescriptor->u.Memory.Start.LowPart ||
                            (targetDescriptor->u.Memory.Start.LowPart + targetDescriptor->u.Memory.Length) <=
                             sourceDescriptor->u.Memory.Start.LowPart) {

                            continue;
                        }
                        if (sourceDescriptor->u.Memory.Start.LowPart ==
                                targetDescriptor->u.Memory.Start.LowPart &&
                            sourceDescriptor->u.Memory.Length ==
                                targetDescriptor->u.Memory.Length) {

                            score += 0x1100;

                        } else {

                            score += 0x1000;

                        }
                        break;

                    case CmResourceTypeDma:
                        if (sourceDescriptor->u.Dma.Channel !=
                            targetDescriptor->u.Dma.Channel) {

                            continue;
                        }
                        score += 0x0010;
                        break;

                    }
                    break;
                }
            }

            if (targetIndex < targetList->Count) {
#if DEBUG_DUP_MATCH
                sourceMapping[sourceIndex] = (CHAR)targetIndex;
                targetMapping[targetIndex] = (CHAR)sourceIndex;
#endif
                numResourcesMatch++;
            }
        }

        if (numResourcesMatch != 0) {
            if (firstMatch == -1) {
                firstMatch = nodeIndex;
            }

            if ((score > bestScore) || (score == bestScore && !bestIdsMatch && idsMatch))  {
                bestScore = score;
                bestMatch = nodeIndex;
                bestIdsMatch = idsMatch;
            }
        }
    }

    if (bestMatch != -1) {

        if (bestScore == possibleScore) {

            DebugPrint( (MAPPER_INFORMATION,
                        "Perfect match, score = %4.4X, possible = %4.4X, index = %d\n",
                        bestScore,
                        possibleScore,
                        bestMatch) );

            if (possibleScore < 0x1000 && !bestIdsMatch) {

                bestMatch = -1;

            }

        } else if (possibleScore > 0x1000 && bestScore >= 0x1000) {

            DebugPrint( (MAPPER_INFORMATION,
                        "Best match is close enough, score = %4.4X, possible = %4.4X, index = %d\n",
                        bestScore,
                        possibleScore,
                        bestMatch) );

        } else  {

            DebugPrint( (MAPPER_INFORMATION,
                        "Best match is less than threshold, score = %4.4X, possible = %4.4X, index = %d\n",
                        bestScore,
                        possibleScore,
                        bestMatch) );

            bestMatch = -1;

        }
    }

    return bestMatch;
}

NTSTATUS
PnPBiosEliminateDupes(
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    )
/*++

Routine Description:

    This routine enumerates the Firmware Mapper generated devices under
    Enum\Root.  Those that match entries in DevNodeInfoList have their registry
    key name stored in the DevNodeInfoList entry so that the Firmare Mapper
    instance may be removed later.

Arguments:

    DevNodeInfoList - Array of BIOS_DEVNODE_INFO structures, one for each device
        reported by the BIOS.

    NumberNodes - Number of BIOS_DEVNODE_INFO elements pointed to by
        DevNodeInfoList.

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    UNICODE_STRING                  enumRootKeyName, valueName;
    HANDLE                          enumRootKey;
    PKEY_BASIC_INFORMATION          deviceBasicInfo = NULL;
    ULONG                           deviceBasicInfoLength;
    UNICODE_STRING                  deviceKeyName;
    HANDLE                          deviceKey = NULL;
    PKEY_BASIC_INFORMATION          instanceBasicInfo = NULL;
    ULONG                           instanceBasicInfoLength;
    WCHAR                           logConfStr[DEFAULT_STRING_SIZE];
    UNICODE_STRING                  logConfKeyName;
    HANDLE                          logConfKey = NULL;

    PKEY_VALUE_PARTIAL_INFORMATION  valueInfo = NULL;
    ULONG                           valueInfoLength;
    ULONG                           returnedLength;

    ULONG                           deviceIndex, instanceIndex;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;

    RtlInitUnicodeString(&enumRootKeyName, ENUMROOT_KEY_NAME);

    status = IopOpenRegistryKeyEx( &enumRootKey,
                                   NULL,
                                   &enumRootKeyName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {

        DebugPrint( (MAPPER_ERROR,
                    "Could not open registry key %S, status = %8.8X\n",
                    ENUMROOT_KEY_NAME,
                    status) );

        return STATUS_UNSUCCESSFUL;
    }

    deviceBasicInfoLength = sizeof(KEY_BASIC_INFORMATION) + DEFAULT_STRING_SIZE;
    deviceBasicInfo = ExAllocatePool(PagedPool, deviceBasicInfoLength);

    instanceBasicInfoLength = sizeof(KEY_BASIC_INFORMATION) + DEFAULT_STRING_SIZE;
    instanceBasicInfo = ExAllocatePool(PagedPool, instanceBasicInfoLength);

    valueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + DEFAULT_STRING_SIZE;
    valueInfo = ExAllocatePool(PagedPool, valueInfoLength);

    if (deviceBasicInfo != NULL && instanceBasicInfo != NULL && valueInfo != NULL) {

        for (deviceIndex = 0; ; deviceIndex++) {

            status = ZwEnumerateKey( enumRootKey,
                                     deviceIndex,
                                     KeyBasicInformation,
                                     deviceBasicInfo,
                                     deviceBasicInfoLength,
                                     &returnedLength);

            if (!NT_SUCCESS(status)) {

                if (status != STATUS_NO_MORE_ENTRIES)  {

                    DebugPrint( (MAPPER_ERROR,
                                "Could not enumerate under key %S, status = %8.8X\n",
                                ENUMROOT_KEY_NAME,
                                status) );
                } else {
                    status = STATUS_SUCCESS;
                }
                break;
            }

            if (deviceBasicInfo->Name[0] != '*') {
                continue;
            }

            deviceBasicInfo->Name[ deviceBasicInfo->NameLength / 2 ] = L'\0';
            RtlInitUnicodeString(&deviceKeyName, deviceBasicInfo->Name);

            status = IopOpenRegistryKeyEx( &deviceKey,
                                           enumRootKey,
                                           &deviceKeyName,
                                           KEY_READ
                                           );

            if (!NT_SUCCESS(status)) {

                DebugPrint( (MAPPER_ERROR,
                            "Could not open registry key %S\\%S, status = %8.8X\n",
                            ENUMROOT_KEY_NAME,
                            deviceBasicInfo->Name,
                            status) );
                break;
            }

            for (instanceIndex = 0; ; instanceIndex++) {

                status = ZwEnumerateKey( deviceKey,
                                         instanceIndex,
                                         KeyBasicInformation,
                                         instanceBasicInfo,
                                         instanceBasicInfoLength,
                                         &returnedLength);

                if (!NT_SUCCESS(status)) {

                    if (status != STATUS_NO_MORE_ENTRIES)  {
                        DebugPrint( (MAPPER_ERROR,
                                    "Could not enumerate under key %S\\%S, status = %8.8X\n",
                                    ENUMROOT_KEY_NAME,
                                    deviceBasicInfo->Name,
                                    status) );
                    } else {
                        status = STATUS_SUCCESS;
                    }
                    break;
                }

                if (RtlCompareMemory( instanceBasicInfo->Name,
                                      INSTANCE_ID_PREFIX,
                                      sizeof(INSTANCE_ID_PREFIX) - sizeof(UNICODE_NULL)
                                      ) == (sizeof(INSTANCE_ID_PREFIX) - sizeof(UNICODE_NULL))) {

                    continue;
                }

                instanceBasicInfo->Name[ instanceBasicInfo->NameLength / 2 ] = L'\0';

                RtlCopyMemory( logConfStr,
                               instanceBasicInfo->Name,
                               instanceBasicInfo->NameLength );

                logConfStr[ instanceBasicInfo->NameLength / 2 ] = L'\\';

                RtlCopyMemory( &logConfStr[ instanceBasicInfo->NameLength / 2 + 1 ],
                               REGSTR_KEY_LOGCONF,
                               sizeof(REGSTR_KEY_LOGCONF) );

                RtlInitUnicodeString( &logConfKeyName, logConfStr );

                status = IopOpenRegistryKeyEx( &logConfKey,
                                               deviceKey,
                                               &logConfKeyName,
                                               KEY_READ
                                               );

                if (!NT_SUCCESS(status)) {

                    DebugPrint( (MAPPER_ERROR,
                                "Could not open registry key %S\\%S\\%S, status = %8.8X\n",
                                ENUMROOT_KEY_NAME,
                                deviceBasicInfo->Name,
                                logConfStr,
                                status) );
                    continue;
                }

                RtlInitUnicodeString( &valueName, REGSTR_VAL_BOOTCONFIG );

                status = ZwQueryValueKey( logConfKey,
                                          &valueName,
                                          KeyValuePartialInformation,
                                          valueInfo,
                                          valueInfoLength,
                                          &returnedLength );

                if (!NT_SUCCESS(status)) {

                    if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW) {

                        ExFreePool( valueInfo );

                        valueInfoLength = returnedLength;
                        valueInfo = ExAllocatePool( PagedPool, valueInfoLength );

                        if (valueInfo != NULL) {

                            status = ZwQueryValueKey( logConfKey,
                                                      &valueName,
                                                      KeyValuePartialInformation,
                                                      valueInfo,
                                                      valueInfoLength,
                                                      &returnedLength );
                        } else {
                            DebugPrint( (MAPPER_ERROR,
                                        "Error allocating memory for %S\\%S\\LogConf\\BootConfig value\n",
                                        ENUMROOT_KEY_NAME,
                                        deviceBasicInfo->Name) );
                            valueInfoLength = 0;
                            status = STATUS_NO_MEMORY;

                            break;
                        }

                    } else {
                        DebugPrint( (MAPPER_ERROR,
                                    "Error retrieving %S\\%S\\LogConf\\BootConfig size, status = %8.8X\n",
                                    ENUMROOT_KEY_NAME,
                                    deviceBasicInfo->Name,
                                    status) );

                        status = STATUS_UNSUCCESSFUL;
                    }
                }

                if (NT_SUCCESS( status )) {
                    PCM_RESOURCE_LIST   resourceList;
                    LONG                matchingIndex;

                    resourceList = (PCM_RESOURCE_LIST)valueInfo->Data;

                    matchingIndex = PnPBiosFindMatchingDevNode( deviceBasicInfo->Name,
                                                                resourceList,
                                                                DevNodeInfoList,
                                                                NumberNodes );

                    if (matchingIndex != -1) {

                        DevNodeInfoList[ matchingIndex ].Replaces = ExAllocatePool( PagedPool,
                                                                                    deviceBasicInfo->NameLength + instanceBasicInfo->NameLength + 2 * sizeof(UNICODE_NULL));

                        if (DevNodeInfoList[ matchingIndex ].Replaces != NULL) {

                            RtlCopyMemory( DevNodeInfoList[ matchingIndex ].Replaces,
                                           deviceBasicInfo->Name,
                                           deviceBasicInfo->NameLength );

                            DevNodeInfoList[ matchingIndex ].Replaces[ deviceBasicInfo->NameLength / 2 ] = '\\';

                            RtlCopyMemory( &DevNodeInfoList[ matchingIndex ].Replaces[ deviceBasicInfo->NameLength / 2 + 1 ],
                                           instanceBasicInfo->Name,
                                           instanceBasicInfo->NameLength );

                            DevNodeInfoList[ matchingIndex ].Replaces[ (deviceBasicInfo->NameLength + instanceBasicInfo->NameLength) / 2 + 1 ] = '\0';

                            DebugPrint( (MAPPER_INFORMATION,
                                        "Match found: %S\\%S%d replaces %S\n",
                                        DevNodeInfoList[ matchingIndex ].ProductId,
                                        INSTANCE_ID_PREFIX,
                                        DevNodeInfoList[ matchingIndex ].Handle,
                                        DevNodeInfoList[ matchingIndex ].Replaces) );
                        } else {
                            DebugPrint( (MAPPER_ERROR,
                                        "Error allocating memory for %S\\%S%d\\Replaces\n",
                                        DevNodeInfoList[ matchingIndex ].ProductId,
                                        INSTANCE_ID_PREFIX,
                                        DevNodeInfoList[ matchingIndex ].Handle) );
                        }
                    } else {
                        DebugPrint( (MAPPER_INFORMATION,
                                    "No matching PnP Bios DevNode found for FW Enumerated device %S\\%S\n",
                                    deviceBasicInfo->Name,
                                    instanceBasicInfo->Name) );
                    }
                } else {
                    DebugPrint( (MAPPER_ERROR,
                                "Error retrieving %S\\%S\\%S\\BootConfig, status = %8.8X\n",
                                ENUMROOT_KEY_NAME,
                                deviceBasicInfo->Name,
                                logConfStr,
                                status) );
                }

                ZwClose(logConfKey);

                logConfKey = NULL;
            }

            ZwClose(deviceKey);

            deviceKey = NULL;
        }
    } else {
        status = STATUS_NO_MEMORY;
    }

    if (valueInfo != NULL) {
        ExFreePool(valueInfo);
    }

    if (instanceBasicInfo != NULL) {
        ExFreePool(instanceBasicInfo);
    }

    if (deviceBasicInfo != NULL) {
        ExFreePool(deviceBasicInfo);
    }

    if (logConfKey != NULL) {
        ZwClose(logConfKey);
    }

    if (deviceKey != NULL) {
        ZwClose(deviceKey);
    }

    ZwClose(enumRootKey);

    return status;
}

PWCHAR
PnPBiosGetDescription(
    IN PBIOS_DEVNODE_INFO DevNodeInfoEntry
    )
{
    ULONG       class, subClass;
    LONG        index;
    CLASSDATA   *classDescriptions;
    LONG        descriptionCount;

    class = DevNodeInfoEntry->TypeCode[0];
    subClass = (DevNodeInfoEntry->TypeCode[1] << 8) | DevNodeInfoEntry->TypeCode[2];

    if (class > 0 && class < CLASSLIST_COUNT) {

        classDescriptions = ClassDescriptionsList[ class ].Descriptions;
        descriptionCount = ClassDescriptionsList[ class ].Count;

        //
        // The last description entry is the default so there is no use
        // comparing it, if we get that far just use it.
        //
        for (index = 0; index < (descriptionCount - 1); index++) {

            if (subClass == classDescriptions[ index ].Value)  {

                break;
            }
        }

        return classDescriptions[ index ].Description;
    }

    return DEFAULT_DEVICE_DESCRIPTION;
}

NTSTATUS
PnPBiosCopyDeviceParamKey(
    IN HANDLE EnumRootKey,
    IN PWCHAR SourcePath,
    IN PWCHAR DestinationPath
    )
/*++

Routine Description:

    Copy the Device Parameters key from the firmware mapper node in
    DevNodeInfo->Replaces to the BIOS mapper node represented by DevNodeInfo.

Arguments:

    EnumRootKey - Handle to Enum\Root.

    SourcePath - Instance path of FW Mapper node relative to Enum\Root.

    DestinationKey - Handle to destination instance key.

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    NTSTATUS                    status;
    UNICODE_STRING              sourceInstanceKeyName;
    HANDLE                      sourceInstanceKey = NULL;

    UNICODE_STRING              deviceParamKeyName;
    HANDLE                      sourceDeviceParamKey = NULL;
    HANDLE                      destinationDeviceParamKey = NULL;
    UNICODE_STRING              destinationInstanceKeyName;

    PKEY_VALUE_FULL_INFORMATION valueFullInfo = NULL;
    ULONG                       valueFullInfoLength;
    ULONG                       resultLength;

    UNICODE_STRING              valueName;

    ULONG                       index;

    RtlInitUnicodeString( &sourceInstanceKeyName, SourcePath );

    status = IopOpenRegistryKeyEx( &sourceInstanceKey,
                                   EnumRootKey,
                                   &sourceInstanceKeyName,
                                   KEY_ALL_ACCESS
                                   );

    if (!NT_SUCCESS(status)) {

        DebugPrint( (MAPPER_ERROR,
                    "PnPBiosCopyDeviceParamKey() - Could not open source instance key %S, status = %8.8X\n",
                    SourcePath,
                    status) );

        return status;
    }

    RtlInitUnicodeString(&deviceParamKeyName, REGSTR_KEY_DEVICEPARAMETERS);

    status = IopOpenRegistryKeyEx( &sourceDeviceParamKey,
                                   sourceInstanceKey,
                                   &deviceParamKeyName,
                                   KEY_ALL_ACCESS
                                   );

    if (!NT_SUCCESS(status)) {

        if (status != STATUS_OBJECT_NAME_NOT_FOUND) {

            DebugPrint( (MAPPER_ERROR,
                        "PnPBiosCopyDeviceParamKey() - Could not open source device parameter key %S\\%S, status = %8.8X\n",
                        SourcePath,
                        deviceParamKeyName.Buffer,
                        status) );
        }

        goto Cleanup;
    }

    RtlInitUnicodeString(&destinationInstanceKeyName, DestinationPath);

    status = IopOpenDeviceParametersSubkey( &destinationDeviceParamKey,
                                            EnumRootKey,
                                            &destinationInstanceKeyName,
                                            KEY_ALL_ACCESS );

    if (!NT_SUCCESS(status)) {

        DebugPrint( (MAPPER_ERROR,
                    "PnPBiosCopyDeviceParamKey() - Could not open destination device parameter key %S\\%S, status = %8.8X\n",
                    DestinationPath,
                    REGSTR_KEY_DEVICEPARAMETERS,
                    status) );

        goto Cleanup;
    }

    valueFullInfoLength = sizeof(KEY_VALUE_FULL_INFORMATION) + DEFAULT_STRING_SIZE + DEFAULT_VALUE_SIZE;
    valueFullInfo = ExAllocatePool(PagedPool, valueFullInfoLength);

    if (valueFullInfo == NULL) {

        goto Cleanup;
    }

    for (index = 0; ; index++) {
        status = ZwEnumerateValueKey( sourceDeviceParamKey,
                                      index,
                                      KeyValueFullInformation,
                                      valueFullInfo,
                                      valueFullInfoLength,
                                      &resultLength );

        if (NT_SUCCESS(status)) {
            UNICODE_STRING  sourcePathString;
            UNICODE_STRING  serialPrefixString;
            UNICODE_STRING  portNameString;

            valueName.Length = (USHORT)valueFullInfo->NameLength;
            valueName.MaximumLength = valueName.Length;
            valueName.Buffer = valueFullInfo->Name;

            RtlInitUnicodeString(&sourcePathString, SourcePath);
            RtlInitUnicodeString(&serialPrefixString, L"*PNP0501");

            if (sourcePathString.Length > serialPrefixString.Length) {
                sourcePathString.Length = serialPrefixString.Length;
            }

            if (RtlCompareUnicodeString(&sourcePathString, &serialPrefixString, TRUE) == 0) {

                RtlInitUnicodeString(&portNameString, L"PortName");

                if (valueName.Length == 16 &&
                    RtlCompareUnicodeString(&valueName, &portNameString, TRUE) == 0)  {

                    // ComPortDBRemove(SourcePath, &unicodeValue);
                    ComPortDBAdd(destinationDeviceParamKey, (PWSTR)((PUCHAR)valueFullInfo + valueFullInfo->DataOffset));
                    continue;
                }
            }

            status = ZwSetValueKey( destinationDeviceParamKey,
                                    &valueName,
                                    valueFullInfo->TitleIndex,
                                    valueFullInfo->Type,
                                    (PUCHAR)valueFullInfo + valueFullInfo->DataOffset,
                                    valueFullInfo->DataLength );
        } else {
            if (status == STATUS_BUFFER_OVERFLOW) {
                ExFreePool( valueFullInfo );

                valueFullInfoLength = resultLength;
                valueFullInfo = ExAllocatePool(PagedPool, valueFullInfoLength);

                if (valueFullInfo == NULL) {
                    status = STATUS_NO_MEMORY;
                } else {
                    index--;
                    continue;
                }
            } else if (status != STATUS_NO_MORE_ENTRIES)  {
                DebugPrint( (MAPPER_ERROR,
                            "Could not enumerate under key %S\\%S, status = %8.8X\n",
                            SourcePath,
                            deviceParamKeyName.Buffer,
                            status) );
            } else {
                status = STATUS_SUCCESS;
            }

            break;
        }
    }

Cleanup:
    if (sourceInstanceKey != NULL) {
        ZwClose( sourceInstanceKey );
    }

    if (sourceDeviceParamKey != NULL) {
        ZwClose( sourceDeviceParamKey );
    }

    if (destinationDeviceParamKey != NULL) {
        ZwClose( destinationDeviceParamKey );
    }

    if (valueFullInfo != NULL) {
        ExFreePool( valueFullInfo );
    }

    return status;
}

NTSTATUS
PnPBiosWriteInfo(
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    )
/*++

Routine Description:

    Creates an entry under Enum\Root for each DevNodeInfoList element.  Also
    removes any duplicate entries which were created by the Firmware Mapper.

    Note: Currently entries for the Keyboard, Mouse, and PCI bus are ignored.

Arguments:

    DevNodeInfoList - Array of BIOS_DEVNODE_INFO structures, one for each device
        reported by the BIOS.

    NumberNodes - Number of BIOS_DEVNODE_INFO elements pointed to by
        DevNodeInfoList.

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    PKEY_VALUE_FULL_INFORMATION     excludeList=NULL;

    UNICODE_STRING                  enumRootKeyName;
    HANDLE                          enumRootKey;
    WCHAR                           instanceNameStr[DEFAULT_STRING_SIZE];
    UNICODE_STRING                  instanceKeyName;
    HANDLE                          instanceKey;
    UNICODE_STRING                  controlKeyName;
    HANDLE                          controlKey;
    UNICODE_STRING                  logConfKeyName;
    HANDLE                          logConfKey;

    UNICODE_STRING                  valueName;
    ULONG                           dwordValue;
    ULONG                           disposition;

    PWCHAR                          descriptionStr;
    ULONG                           descriptionStrLength;

    ULONG                           nodeIndex;
    NTSTATUS                        status;

    BOOLEAN                         isNewDevice;

    RtlInitUnicodeString(&enumRootKeyName, ENUMROOT_KEY_NAME);

    status = IopOpenRegistryKeyEx( &enumRootKey,
                                   NULL,
                                   &enumRootKeyName,
                                   KEY_ALL_ACCESS
                                   );

    if (!NT_SUCCESS(status)) {

        DebugPrint( (MAPPER_ERROR,
                    "Could not open registry key %S, status = %8.8X\n",
                    ENUMROOT_KEY_NAME,
                    status) );

        return STATUS_UNSUCCESSFUL;

    }

    //
    // Reasons why a node might be excluded (i.e not enumerated)
    // * included in ExcludedDevices array (non-conditional)
    // * included in CCS\Control\BiosInfo\PnpBios\DisableNodes via biosinfo.inf
    // * resources are disabled and device is included in the
    //   ExcludeIfDisabled array

    PnPGetDevnodeExcludeList (&excludeList);

    for (nodeIndex = 0; nodeIndex < NumberNodes; nodeIndex++) {

        //
        // Check if this node is in the 'ignore on this machine' list.
        //

        if ( excludeList &&
             PnPBiosIgnoreNode( &DevNodeInfoList[ nodeIndex ].ProductId[1],
                                (PWCHAR)((PUCHAR)excludeList+excludeList->DataOffset))) {
            continue;
        }

        // Checking for nodes we always exclude
        if ( PnPBiosCheckForExclusion( ExcludedDevices,
                                       EXCLUDED_DEVICES_COUNT,
                                       DevNodeInfoList[ nodeIndex ].ProductId,
                                       DevNodeInfoList[ nodeIndex ].CompatibleIDs)) {
            //
            // If we are skipping the device, we need to first copy the decode
            // info that the BIOS supplied to the ntdetected device's Boot
            // Config which was generated by the FW mapper.
            //
            PnPBiosCopyIoDecode( enumRootKey, &DevNodeInfoList[ nodeIndex ] );

            //
            // Skip excluded devices, ie busses, mice and keyboards for now.
            //

            continue;
        }

        // Checking for nodes we exclude if disabled
        if ( DevNodeInfoList[ nodeIndex ].FirmwareDisabled &&
             PnPBiosCheckForExclusion( ExcludeIfDisabled,
                                       EXCLUDE_DISABLED_COUNT,
                                       DevNodeInfoList[ nodeIndex ].ProductId,
                                       NULL)) {
            continue;
        }

        swprintf( instanceNameStr,
                  L"%s\\%s%d",
                  DevNodeInfoList[ nodeIndex ].ProductId,
                  INSTANCE_ID_PREFIX,
                  DevNodeInfoList[ nodeIndex ].Handle );

        RtlInitUnicodeString( &instanceKeyName, instanceNameStr );

        status = IopCreateRegistryKeyEx( &instanceKey,
                                         enumRootKey,
                                         &instanceKeyName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         &disposition
                                         );

        if (NT_SUCCESS(status))  {

            isNewDevice = disposition == REG_CREATED_NEW_KEY;

            if (isNewDevice) {

                RtlInitUnicodeString( &valueName, L"DeviceDesc" );

                descriptionStr = PnPBiosGetDescription( &DevNodeInfoList[ nodeIndex ] );
                descriptionStrLength = wcslen(descriptionStr) * 2 + sizeof(UNICODE_NULL);

                status = ZwSetValueKey( instanceKey,
                                        &valueName,
                                        0,
                                        REG_SZ,
                                        descriptionStr,
                                        descriptionStrLength );
            }

            RtlInitUnicodeString( &valueName, REGSTR_VAL_FIRMWAREIDENTIFIED );
            dwordValue = 1;

            status = ZwSetValueKey( instanceKey,
                                    &valueName,
                                    0,
                                    REG_DWORD,
                                    &dwordValue,
                                    sizeof(dwordValue) );

            if (isNewDevice)  {

                RtlInitUnicodeString( &valueName, L"HardwareID" );

                status = ZwSetValueKey( instanceKey,
                                        &valueName,
                                        0,
                                        REG_MULTI_SZ,
                                        DevNodeInfoList[ nodeIndex ].ProductId,
                                        sizeof(DevNodeInfoList[ nodeIndex ].ProductId) );

                if (DevNodeInfoList[ nodeIndex ].CompatibleIDs != NULL) {

                    RtlInitUnicodeString( &valueName, L"CompatibleIDs" );

                    status = ZwSetValueKey( instanceKey,
                                            &valueName,
                                            0,
                                            REG_MULTI_SZ,
                                            DevNodeInfoList[ nodeIndex ].CompatibleIDs,
                                            DevNodeInfoList[ nodeIndex ].CompatibleIDsLength );
                }
            }

            RtlInitUnicodeString( &valueName, L"Replaces" );

            if (DevNodeInfoList[ nodeIndex ].Replaces != NULL) {

                status = ZwSetValueKey( instanceKey,
                                        &valueName,
                                        0,
                                        REG_SZ,
                                        DevNodeInfoList[ nodeIndex ].Replaces,
                                        wcslen(DevNodeInfoList[ nodeIndex ].Replaces) * 2 + sizeof(UNICODE_NULL) );

            } else if (!isNewDevice) {

                status = ZwDeleteValueKey( instanceKey,
                                           &valueName );
            }

            RtlInitUnicodeString( &controlKeyName, REGSTR_KEY_DEVICECONTROL );

            status = IopCreateRegistryKeyEx( &controlKey,
                                             instanceKey,
                                             &controlKeyName,
                                             KEY_ALL_ACCESS,
                                             REG_OPTION_VOLATILE,
                                             NULL
                                             );

            if (NT_SUCCESS(status))  {

                RtlInitUnicodeString( &valueName, REGSTR_VAL_FIRMWAREMEMBER );
                dwordValue = 1;

                status = ZwSetValueKey( controlKey,
                                        &valueName,
                                        0,
                                        REG_DWORD,
                                        &dwordValue,
                                        sizeof(dwordValue) );

                RtlInitUnicodeString( &valueName, L"PnpBiosDeviceHandle" );
                dwordValue = DevNodeInfoList[ nodeIndex ].Handle;

                status = ZwSetValueKey( controlKey,
                                        &valueName,
                                        0,
                                        REG_DWORD,
                                        &dwordValue,
                                        sizeof(dwordValue) );

                RtlInitUnicodeString( &valueName, REGSTR_VAL_FIRMWAREDISABLED );
                dwordValue = DevNodeInfoList[ nodeIndex ].FirmwareDisabled;

                status = ZwSetValueKey( controlKey,
                                        &valueName,
                                        0,
                                        REG_DWORD,
                                        &dwordValue,
                                        sizeof(dwordValue) );

                RtlInitUnicodeString( &valueName, L"PnpBiosDeviceHandle" );
                dwordValue = DevNodeInfoList[ nodeIndex ].Handle;

                ZwClose( controlKey );

            } else {

                DebugPrint( (MAPPER_ERROR,
                            "Could not open registry key %S\\%S\\%S\\Control, status = %8.8X\n",
                            ENUMROOT_KEY_NAME,
                            DevNodeInfoList[ nodeIndex ].ProductId,
                            instanceNameStr,
                            status) );

                ZwClose( instanceKey );
                status = STATUS_UNSUCCESSFUL;

                goto Cleanup;
            }

            RtlInitUnicodeString( &logConfKeyName, REGSTR_KEY_LOGCONF );

            status = IopCreateRegistryKeyEx( &logConfKey,
                                           instanceKey,
                                           &logConfKeyName,
                                           KEY_ALL_ACCESS,
                                           REG_OPTION_NON_VOLATILE,
                                           NULL
                                           );

            if (NT_SUCCESS(status))  {

                if (DevNodeInfoList[ nodeIndex ].BootConfig != NULL) {

                    RtlInitUnicodeString( &valueName, REGSTR_VAL_BOOTCONFIG );

                    status = ZwSetValueKey( logConfKey,
                                            &valueName,
                                            0,
                                            REG_RESOURCE_LIST,
                                            DevNodeInfoList[ nodeIndex ].BootConfig,
                                            DevNodeInfoList[ nodeIndex ].BootConfigLength );

                }

                if (DevNodeInfoList[ nodeIndex ].BasicConfig != NULL) {

                    RtlInitUnicodeString( &valueName, REGSTR_VAL_BASICCONFIGVECTOR );

                    status = ZwSetValueKey( logConfKey,
                                            &valueName,
                                            0,
                                            REG_RESOURCE_REQUIREMENTS_LIST,
                                            DevNodeInfoList[ nodeIndex ].BasicConfig,
                                            DevNodeInfoList[ nodeIndex ].BasicConfigLength );

                }

                ZwClose( logConfKey );

            } else {

                DebugPrint( (MAPPER_ERROR,
                            "Could not open registry key %S\\%S\\%S\\LogConf, status = %8.8X\n",
                            ENUMROOT_KEY_NAME,
                            DevNodeInfoList[ nodeIndex ].ProductId,
                            instanceNameStr,
                            status) );

                ZwClose( instanceKey );
                status = STATUS_UNSUCCESSFUL;

                goto Cleanup;
            }

            //
            // If we are replacing a FW Mapper devnode we need to copy the
            // Device Parameters subkey.
            //
            if (isNewDevice && DevNodeInfoList[ nodeIndex ].Replaces != NULL) {

                status = PnPBiosCopyDeviceParamKey( enumRootKey,
                                                    DevNodeInfoList[ nodeIndex ].Replaces,
                                                    instanceNameStr );
            }

            ZwClose( instanceKey );

        } else {

            DebugPrint( (MAPPER_ERROR,
                        "Could not open registry key %S\\%S\\%S, status = %8.8X\n",
                        ENUMROOT_KEY_NAME,
                        DevNodeInfoList[ nodeIndex ].ProductId,
                        instanceNameStr,
                        status) );

            ZwClose( instanceKey );
            status = STATUS_UNSUCCESSFUL;

            goto Cleanup;
        }

        //
        // Now check if the entry just written duplicates one written by the
        // Firmware Mapper.  If it does then remove the Firmware Mapper entry.
        //

        if (DevNodeInfoList[ nodeIndex ].Replaces != NULL) {

            IopDeleteKeyRecursive( enumRootKey, DevNodeInfoList[ nodeIndex ].Replaces );

        }
    }

    status = STATUS_SUCCESS;

 Cleanup:
    ZwClose( enumRootKey );

    if (excludeList) {
        ExFreePool (excludeList);
    }

    return status;
}
VOID
PnPBiosCopyIoDecode(
    IN HANDLE EnumRootKey,
    IN PBIOS_DEVNODE_INFO DevNodeInfo
    )
{
    HANDLE                          deviceKey;
    WCHAR                           logConfKeyNameStr[DEFAULT_STRING_SIZE];
    UNICODE_STRING                  logConfKeyName;
    HANDLE                          logConfKey;
    UNICODE_STRING                  valueName;
    PKEY_VALUE_PARTIAL_INFORMATION  valueInfo = NULL;
    ULONG                           valueInfoLength;
    ULONG                           returnedLength;
    NTSTATUS                        status;
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;
    ULONG                           index;
    USHORT                          flags;

    if (DevNodeInfo->Replaces == NULL || DevNodeInfo->BootConfig == NULL) {

        //
        // If we didn't find a FW Mapper created devnode then there is nothing
        // to do.
        //
        return;
    }

    //
    // Search through the Boot Config and see if the device's I/O ports are
    // 16 bit decode.
    //

    ASSERT(DevNodeInfo->BootConfig->Count == 1);

    partialResourceList = &DevNodeInfo->BootConfig->List[0].PartialResourceList;

    partialDescriptor = &partialResourceList->PartialDescriptors[0];

    flags = (USHORT)~0;

#define DECODE_FLAGS ( CM_RESOURCE_PORT_10_BIT_DECODE | \
                       CM_RESOURCE_PORT_12_BIT_DECODE | \
                       CM_RESOURCE_PORT_16_BIT_DECODE | \
                       CM_RESOURCE_PORT_POSITIVE_DECODE )

    for ( index = 0; index < partialResourceList->Count; index++ ) {
        if (partialDescriptor->Type == CmResourceTypePort) {
            if (flags == (USHORT)~0) {
                flags = partialDescriptor->Flags & DECODE_FLAGS;
            } else {
                ASSERT(flags == (partialDescriptor->Flags & DECODE_FLAGS));
            }
        }
        partialDescriptor++;
    }

    if (!(flags & (CM_RESOURCE_PORT_16_BIT_DECODE | CM_RESOURCE_PORT_POSITIVE_DECODE)))  {
        return;
    }

    swprintf( logConfKeyNameStr,
              L"%s\\%s",
              DevNodeInfo->Replaces,
              REGSTR_KEY_LOGCONF
              );

    RtlInitUnicodeString( &logConfKeyName, logConfKeyNameStr );

    status = IopCreateRegistryKeyEx( &logConfKey,
                                     EnumRootKey,
                                     &logConfKeyName,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) {

        DebugPrint( (MAPPER_ERROR,
                    "Could not open registry key %S\\%S\\%S, status = %8.8X\n",
                    ENUMROOT_KEY_NAME,
                    DevNodeInfo->Replaces,
                    REGSTR_KEY_LOGCONF,
                    status) );

        return;
    }

    valueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + DEFAULT_STRING_SIZE;
    valueInfo = ExAllocatePool(PagedPool, valueInfoLength);

    if (valueInfo == NULL)  {

        ZwClose( logConfKey );

        return;
    }

    RtlInitUnicodeString( &valueName, REGSTR_VAL_BOOTCONFIG );

    status = ZwQueryValueKey( logConfKey,
                              &valueName,
                              KeyValuePartialInformation,
                              valueInfo,
                              valueInfoLength,
                              &returnedLength);

    if (!NT_SUCCESS(status)) {

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW) {

            //
            // The default buffer was too small, free it and reallocate
            // it to the required size.
            //
            ExFreePool( valueInfo );

            valueInfoLength = returnedLength;
            valueInfo = ExAllocatePool( PagedPool, valueInfoLength );

            if (valueInfo != NULL)  {

                status = ZwQueryValueKey( logConfKey,
                                          &valueName,
                                          KeyValuePartialInformation,
                                          valueInfo,
                                          valueInfoLength,
                                          &returnedLength );

                if (!NT_SUCCESS(status)) {
                    DebugPrint( (MAPPER_ERROR,
                                "Could not query registry value %S\\%S\\LogConf\\BootConfig, status = %8.8X\n",
                                ENUMROOT_KEY_NAME,
                                DevNodeInfo->Replaces,
                                status) );

                    ExFreePool( valueInfo );

                    ZwClose( logConfKey );

                    return;
                }
            } else {

                DebugPrint( (MAPPER_ERROR,
                            "Could not allocate memory for BootConfig value\n"
                            ) );

                ZwClose( logConfKey );

                return;
            }
        }
    }

    partialResourceList = &((PCM_RESOURCE_LIST)valueInfo->Data)->List[0].PartialResourceList;

    partialDescriptor = &partialResourceList->PartialDescriptors[0];

    for ( index = 0; index < partialResourceList->Count; index++ ) {
        if (partialDescriptor->Type == CmResourceTypePort) {
            partialDescriptor->Flags &= ~DECODE_FLAGS;
            partialDescriptor->Flags |= flags;
        }
        partialDescriptor++;
    }

    status = ZwSetValueKey( logConfKey,
                            &valueName,
                            0,
                            REG_RESOURCE_LIST,
                            valueInfo->Data,
                            valueInfo->DataLength );

    if (!NT_SUCCESS(status)) {
        DebugPrint( (MAPPER_ERROR,
                    "Could not set registry value %S\\%S\\LogConf\\BootConfig, status = %8.8X\n",
                    ENUMROOT_KEY_NAME,
                    DevNodeInfo->Replaces,
                    status) );
    }

    ExFreePool(valueInfo);

    ZwClose(logConfKey);
}

NTSTATUS
PnPBiosCheckForHardwareDisabled(
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResourceList,
    IN OUT PBOOLEAN Disabled
    )
/*++

Routine Description:

    If this device has been assigned one or more resources, and each resource has a length of zero, then it is
    hardware disabled.

Arguments:

    IoResourceList - Resource obtained from BIOS that we're about to map to a CmResourceList

    Disabled - Set to TRUE if the device is deemed to be disabled

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    BOOLEAN ParsedResource = FALSE;
    PIO_RESOURCE_DESCRIPTOR ioDescriptor;
    ULONG descIndex;
    //
    // Since this routine is only used to translate the allocated resources
    // returned by the PnP BIOS, we can assume that there is only 1 alternative
    // list
    //

    ASSERT(IoResourceList->AlternativeLists == 1);
    ASSERT(Disabled != NULL);

    *Disabled = FALSE;

    //
    // Translate each resource descriptor, currently we only handle ports,
    // memory, interrupts, and dma.  The current implementation of the routine
    // which converts from ISA PnP Resource data to IO_RESOURCE_REQUIREMENTS
    // won't generate any other descriptor types given the data returned from
    // the BIOS.
    //

    for (descIndex = 0; descIndex < IoResourceList->List[ 0 ].Count; descIndex++) {

        ioDescriptor = &IoResourceList->List[ 0 ].Descriptors[ descIndex ];

        switch (ioDescriptor->Type) {

        case CmResourceTypePort:
            if (ioDescriptor->u.Port.Length) {
                return STATUS_SUCCESS;
            }
            ParsedResource = TRUE;
            break;

        case CmResourceTypeInterrupt:
            if (ioDescriptor->u.Interrupt.MinimumVector != (ULONG)(-1)) {
                return STATUS_SUCCESS;
            }
            ParsedResource = TRUE;
            break;

        case CmResourceTypeMemory:
            if (ioDescriptor->u.Memory.Length) {
                return STATUS_SUCCESS;
            }
            ParsedResource = TRUE;
            break;

        case CmResourceTypeDma:
            if (ioDescriptor->u.Dma.MinimumChannel != (ULONG)(-1)) {
                return STATUS_SUCCESS;
            }
            ParsedResource = TRUE;
            break;

        default:
            DebugPrint( (MAPPER_ERROR,
                        "Unexpected ResourceType (%d) in I/O Descriptor\n",
                        ioDescriptor->Type) );

#if DBG
            // DbgBreakPoint();
#endif
            break;
        }
    }

    if (ParsedResource) {
        //
        // at least one empty resource, no non-empty resources
        //
        *Disabled = TRUE;
    }

    return STATUS_SUCCESS;

}


NTSTATUS
PnPBiosFreeDevNodeInfo(
    IN PBIOS_DEVNODE_INFO DevNodeInfoList,
    IN ULONG NumberNodes
    )
/*++

Routine Description:

    Free the dynamically allocated DevNodeInfoList as well as any dynamically
    allocated dependent structures.

Arguments:

    DevNodeInfoList - Array of BIOS_DEVNODE_INFO structures, one for each device
        reported by the BIOS.

    NumberNodes - Number of BIOS_DEVNODE_INFO elements pointed to by
        DevNodeInfoList.

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    ULONG   nodeIndex;

    for (nodeIndex = 0; nodeIndex < NumberNodes; nodeIndex++) {

        if (DevNodeInfoList[nodeIndex].Replaces != NULL) {
            ExFreePool( DevNodeInfoList[nodeIndex].Replaces );
        }

        if (DevNodeInfoList[nodeIndex].CompatibleIDs != NULL) {
            ExFreePool( DevNodeInfoList[nodeIndex].CompatibleIDs );
        }

        if (DevNodeInfoList[nodeIndex].BootConfig != NULL) {
            ExFreePool( DevNodeInfoList[nodeIndex].BootConfig );
        }

        if (DevNodeInfoList[nodeIndex].BasicConfig != NULL) {
            ExFreePool( DevNodeInfoList[nodeIndex].BasicConfig );
        }
    }

    ExFreePool( DevNodeInfoList );

    return STATUS_SUCCESS;
}

NTSTATUS
PnPBiosMapper()
/*++

Routine Description:

    Map the information provided from the PnP BIOS and stored in the registry by
    NTDETECT into root enumerated devices.

Arguments:

    NONE

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    PCM_RESOURCE_LIST   biosInfo;
    ULONG               length;
    NTSTATUS            status;
    PBIOS_DEVNODE_INFO  devNodeInfoList;
    ULONG               numberNodes;

    status = PnPBiosGetBiosInfo( &biosInfo, &length );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    status = PnPBiosTranslateInfo( biosInfo,
                                   length,
                                   &devNodeInfoList,
                                   &numberNodes );

    ExFreePool( biosInfo );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    status = PnPBiosEliminateDupes( devNodeInfoList, numberNodes );

    if (NT_SUCCESS( status )) {

        status = PnPBiosWriteInfo( devNodeInfoList, numberNodes );

    }

    PnPBiosFreeDevNodeInfo( devNodeInfoList, numberNodes );

    return status;
}

VOID
PpFilterNtResource (
    IN PWCHAR PnpDeviceName,
    PIO_RESOURCE_REQUIREMENTS_LIST ResReqList
)
{
    PIO_RESOURCE_LIST ioResourceList;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptors;

    if (ResReqList == NULL) {
        return;
    }

    if (IsNEC_98) {
        return;
    }

#if 0 //_X86_
    if (KeI386MachineType == MACHINE_TYPE_EISA) {

        PCM_FULL_RESOURCE_DESCRIPTOR    fullDescriptor;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;
        PUCHAR                          nextDescriptor;
        ULONG                           j;
        ULONG                           lastResourceIndex;

        fullDescriptor = &ResourceList->List[0];

        for (i = 0; i < ResourceList->Count; i++) {

            partialResourceList = &fullDescriptor->PartialResourceList;

            for (j = 0; j < partialResourceList->Count; j++) {
                partialDescriptor = &partialResourceList->PartialDescriptors[j];

                if (partialDescriptor->Type == CmResourceTypePort) {
                    if (partialDescriptor->u.Port.Start.HighPart == 0 &&
                        (partialDescriptor->u.Port.Start.LowPart & 0x00000300) == 0) {
                        partialDescriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
                    }
                }
            }

            nextDescriptor = (PUCHAR)fullDescriptor + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

            //
            // account for any resource descriptors in addition to the single
            // imbedded one I've already accounted for (if there aren't any,
            // then I'll end up subtracting off the extra imbedded descriptor
            // from the previous step)
            //
            //
            // finally, account for any extra device specific data at the end of
            // the last partial resource descriptor (if any)
            //
            if (partialResourceList->Count > 0) {

                nextDescriptor += (partialResourceList->Count - 1) *
                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

                lastResourceIndex = partialResourceList->Count - 1;

                if (partialResourceList->PartialDescriptors[lastResourceIndex].Type ==
                          CmResourceTypeDeviceSpecific) {

                    nextDescriptor += partialResourceList->PartialDescriptors[lastResourceIndex].
                               u.DeviceSpecificData.DataSize;
                }
            }

            fullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)nextDescriptor;
        }
    }
#endif

    if (RtlCompareMemory(PnpDeviceName,
                         L"*PNP06",
                         sizeof(L"*PNP06") - sizeof(WCHAR)) ==
                         sizeof(L"*PNP06") - sizeof(WCHAR)) {

        ULONG i, j;

        ioResourceList = ResReqList->List;

        for (j = 0; j < ResReqList->AlternativeLists; j++) {

            ioResourceDescriptors = ioResourceList->Descriptors;

            for (i = 0; i < ioResourceList->Count; i++) {

                if (ioResourceDescriptors[i].Type == CmResourceTypePort) {

                    //
                    // some bios asks for 1 too many io port for ide channel
                    //
                    if ((ioResourceDescriptors[i].u.Port.Length == 2) &&
                            (ioResourceDescriptors[i].u.Port.MaximumAddress.QuadPart ==
                            (ioResourceDescriptors[i].u.Port.MinimumAddress.QuadPart + 1))) {

                            ioResourceDescriptors[i].u.Port.Length = 1;
                        ioResourceDescriptors[i].u.Port.MaximumAddress =
                            ioResourceDescriptors[i].u.Port.MinimumAddress;
                    }
                }
            }

            ioResourceList = (PIO_RESOURCE_LIST) (ioResourceDescriptors + ioResourceList->Count);
        }
    }
}



#if DBG
VOID
PnPBiosDebugPrint(
    ULONG  DebugMask,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the firmware mapper

Arguments:

    Check the mask value to see if the debug message is requested.

Return Value:

    None

--*/

{
    va_list ap;
    CHAR    buffer[256];

    va_start( ap, DebugMessage );

#define DBG_MSG_PREFIX  "BiosMapper: "

    strcpy(buffer, DBG_MSG_PREFIX);

    if ( DebugMask & PnPBiosMapperDebugMask ) {

        _vsnprintf( &buffer[sizeof(DBG_MSG_PREFIX) - 1],
                   sizeof(buffer) - sizeof(DBG_MSG_PREFIX),
                   DebugMessage,
                   ap );

        DbgPrint( buffer );
    }

    va_end(ap);

}
#endif

