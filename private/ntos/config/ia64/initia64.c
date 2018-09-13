/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    init386.c

Abstract:

    This module is responsible to build any x86 specific entries in
    the hardware tree of registry.

Author:

    Ken Reneris (kenr) 04-Aug-1992


Environment:

    Kernel mode.

Revision History:

    shielint - add BIOS date and version detection.

--*/

#include "cmp.h"
#include "stdio.h"

//
// Title Index is set to 0.
// (from ..\cmconfig.c)
//

#define TITLE_INDEX_VALUE 0

extern PCHAR SearchStrings[];
extern PCHAR BiosBegin;
extern PCHAR Start;
extern PCHAR End;
extern UCHAR CmpID1[];
extern UCHAR CmpID2[];
extern WCHAR CmpVendorID[];
extern WCHAR CmpProcessorNameString[];
extern WCHAR CmpFeatureBits[];
extern WCHAR CmpMHz[];
extern WCHAR CmpUpdateSignature[];
extern UCHAR CmpCyrixID[];
extern UCHAR CmpIntelID[];
extern UCHAR CmpAmdID[];

//
// Bios date and version definitions
//

#define BIOS_DATE_LENGTH 9
#define MAXIMUM_BIOS_VERSION_LENGTH 128
#define SYSTEM_BIOS_START 0xF0000
#define SYSTEM_BIOS_LENGTH 0x10000
#define INT10_VECTOR 0x10
#define VIDEO_BIOS_START 0xC0000
#define VIDEO_BIOS_LENGTH 0x8000
#define VERSION_DATA_LENGTH PAGE_SIZE

//
// Extended CPUID function definitions
//

#define CPUID_PROCESSOR_NAME_STRING_SZ  48
#define CPUID_EXTFN_BASE                0x80000000
#define CPUID_EXTFN_PROCESSOR_NAME      0x80000002


extern ULONG CmpConfigurationAreaSize;
extern PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData;


BOOLEAN
CmpGetBiosVersion (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR VersionString
    );

BOOLEAN
CmpGetBiosDate (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR DateString
    );

ULONG
Ke386CyrixId (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpGetBiosDate)
#pragma alloc_text(INIT,CmpGetBiosVersion)
#pragma alloc_text(INIT,CmpInitializeMachineDependentConfiguration)
#endif


BOOLEAN
CmpGetBiosDate (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR DateString
    )

/*++

Routine Description:

    This routine finds the most recent date in the computer/video
    card's ROM.  When GetRomDate encounters a datae, it checks the
    previously found date to see if the new date is more recent.

Arguments:

    SearchArea - the area to search for a date.

    SearchLength - Length of search.

    DateString - Supplies a pointer to a fixed length memory to receive
                 the date string.

Return Value:

    NT_SUCCESS if a date is found.

--*/

{
    BOOLEAN FoundFlag = TRUE;        // Set to TRUE if the item was found
    CHAR PrevDate[BIOS_DATE_LENGTH]; // Date currently being examined
    CHAR CurrDate[BIOS_DATE_LENGTH]; // Date currently being examined
    PCHAR String;
    USHORT i;                        // Looping variable
    USHORT Length;                   // Number of characters to move
    PCHAR Start = SearchArea + 2;
    PCHAR End = SearchArea + SearchLength - 5;

    //
    // Clear out the previous date
    //

    RtlZeroMemory(PrevDate, BIOS_DATE_LENGTH);

    while (FoundFlag) {

        String = NULL;

        //
        // Search for '/' with a digit on either side and another
        // '/' 3 character away.
        //

        while (Start < End) {
            if (*Start == '/' && *(Start+3) == '/' &&
                (*(Start+1) <= '9' && *(Start+1) >= '0') &&
                (*(Start-1) <= '9' && *(Start-1) >= '0') &&
                (*(Start+5) <= '9' && *(Start+5) >= '0') &&
                (*(Start+4) <= '9' && *(Start+4) >= '0') &&
                (*(Start+2) <= '9' && *(Start+2) >= '0')) {

                String = Start;
                break;
            } else {
                Start++;
            }
        }

        if (String) {
            Start = String + 3;
            String -= 2;                 // Move String to the beginning of
                                         //   date.
            //
            // Copy the year into CurrDate
            //

            CurrDate[0] = String[6];
            CurrDate[1] = String[7];
            CurrDate[2] = '/';           // The 1st "/" for YY/MM/DD

            //
            // Copy the month & day into CurrDate
            //   (Process properly if this is a one digit month)
            //

            if (*String > '9' || *String < '0') {
                CurrDate[3] = '0';
                String++;
                i = 4;
                Length = 4;
            } else {
                i = 3;
                Length = 5;
            }

            RtlMoveMemory(&CurrDate[i], String, Length);

            //
            // Compare the dates, to see which is more recent
            //

            if (memcmp (PrevDate, CurrDate, BIOS_DATE_LENGTH - 1) < 0) {
                RtlMoveMemory(PrevDate, CurrDate, BIOS_DATE_LENGTH - 1);
            }
        } else {
            FoundFlag = FALSE;
        }
    }

    //
    // If we did not find a date
    //

    if (PrevDate[0] == '\0') {
        DateString[0] = '\0';
        return (FALSE);
    }

    //
    // Put the date from chPrevDate's YY/MM/DD format
    //   into pchDateString's MM/DD/YY format

    DateString[5] = '/';
    DateString[6] = PrevDate[0];
    DateString[7] = PrevDate[1];
    RtlMoveMemory(DateString, &PrevDate[3], 5);
    DateString[8] = '\0';

    return (TRUE);
}

BOOLEAN
CmpGetBiosVersion (
    PCHAR SearchArea,
    ULONG SearchLength,
    PCHAR VersionString
    )

/*++

Routine Description:

    This routine finds the version number stored in ROM, if any.

Arguments:

    SearchArea - the area to search for the version.

    SearchLength - Length of search

    VersionString - Supplies a pointer to a fixed length memory to receive
                 the version string.

Return Value:

    TRUE if a version number is found.  Else a value of FALSE is returned.

--*/
{
    PCHAR String;
    USHORT Length;
    USHORT i;
    CHAR Buffer[MAXIMUM_BIOS_VERSION_LENGTH];
    PCHAR BufferPointer;

        if (SearchArea != NULL) {

        //
        // If caller does not specify the search area, we will search
        // the area left from previous search.
        //

        BiosBegin = SearchArea;
        Start = SearchArea + 1;
        End = SearchArea + SearchLength - 2;
    }

    while (1) {

         //
         // Search for a period with a digit on either side
         //

         String = NULL;
         while (Start <= End) {
             if (*Start == '.' && *(Start+1) >= '0' && *(Start+1) <= '9' &&
                 *(Start-1) >= '0' && *(Start-1) <= '9') {
                 String = Start;
                 break;
             } else {
                 Start++;
             }
         }

         if (Start > End) {
             return(FALSE);
         } else {
             Start += 2;
         }

         Length = 0;
         Buffer[MAXIMUM_BIOS_VERSION_LENGTH - 1] = '\0';
         BufferPointer = &Buffer[MAXIMUM_BIOS_VERSION_LENGTH - 1];

         //
         // Search for the beginning of the string
         //

         String--;
         while (Length < MAXIMUM_BIOS_VERSION_LENGTH - 8 &&
                String >= BiosBegin &&
                *String >= ' ' && *String <= 127 &&
                *String != '$') {
             --BufferPointer;
             *BufferPointer = *String;
             --String, ++Length;
         }
         ++String;

         //
         // Can one of the search strings be found
         //

         for (i = 0; SearchStrings[i]; i++) {
             if (strstr(BufferPointer, SearchStrings[i])) {
                 goto Found;
             }
         }
    }

Found:

    //
    // Skip leading white space
    //

    for (; *String == ' '; ++String)
      ;

    //
    // Copy the string to user supplied buffer
    //

    for (i = 0; i < MAXIMUM_BIOS_VERSION_LENGTH - 1 &&
         String <= (End + 1) &&
         *String >= ' ' && *String <= 127 && *String != '$';
         ++i, ++String) {
         VersionString[i] = *String;
    }
    VersionString[i] = '\0';
    return (TRUE);
}


NTSTATUS
CmpInitializeMachineDependentConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine creates x86 specific entries in the registry.

Arguments:

    LoaderBlock - supplies a pointer to the LoaderBlock passed in from the
                  OS Loader.

Returns:

    NTSTATUS code for sucess or reason of failure.

--*/
{
    NTSTATUS Status;
    ULONG VideoBiosStart;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING ValueData;
    ANSI_STRING AnsiString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Disposition;
    HANDLE ParentHandle;
    HANDLE BaseHandle, NpxHandle;
    HANDLE CurrentControlSet;
    CONFIGURATION_COMPONENT_DATA CurrentEntry;
    PUCHAR VendorID;
    UCHAR  Buffer[MAXIMUM_BIOS_VERSION_LENGTH];
    PKPRCB Prcb;
    ULONG  i, Junk;
    ULONG VersionsLength = 0, Length;
    PCHAR VersionStrings, VersionPointer;
    UNICODE_STRING SectionName;
    ULONG ViewSize;
    LARGE_INTEGER ViewBase;
    PVOID BaseAddress;
    HANDLE SectionHandle;
    USHORT DeviceIndexTable[NUMBER_TYPES];
    ULONG CpuIdFunction;
    ULONG MaxExtFn;
    PULONG NameString = NULL;
    struct {
        union {
            UCHAR   Bytes[CPUID_PROCESSOR_NAME_STRING_SZ];
            ULONG   DWords[1];
        } u;
    } ProcessorNameString;

    for (i = 0; i < NUMBER_TYPES; i++) {
        DeviceIndexTable[i] = 0;
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                &CmRegistryMachineHardwareDescriptionSystemName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenKey( &ParentHandle,
                        KEY_READ,
                        &ObjectAttributes
                      );

    if (!NT_SUCCESS(Status)) {
        // Something is really wrong...
        return Status;
    }


    //
    // On an ARC machine the processor(s) are included in the hardware
    // configuration passed in from bootup.  Since there's no standard
    // way to get all the ARC information for each processor in an MP
    // machine via pc-ROMs the information will be added here (if it's
    // not already present).
    //

    RtlInitUnicodeString( &KeyName,
                          L"CentralProcessor"
                        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        0,
        ParentHandle,
        NULL
        );

    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    Status = NtCreateKey(
                &BaseHandle,
                KEY_READ | KEY_WRITE,
                &ObjectAttributes,
                TITLE_INDEX_VALUE,
                &CmClassName[ProcessorClass],
                0,
                &Disposition
                );

    NtClose (BaseHandle);

    if (Disposition == REG_CREATED_NEW_KEY) {

        //
        // The ARC rom didn't add the processor(s) into the registry.
        // Do it now.
        //

        CmpConfigurationData = (PCM_FULL_RESOURCE_DESCRIPTOR)ExAllocatePool(
                                            PagedPool,
                                            CmpConfigurationAreaSize
                                            );

        if (CmpConfigurationData == NULL) {
            // bail out
            NtClose (ParentHandle);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        for (i=0; i < (ULONG)KeNumberProcessors; i++) {
            Prcb = KiProcessorBlock[i];

            RtlZeroMemory (&CurrentEntry, sizeof CurrentEntry);
            CurrentEntry.ComponentEntry.Class = ProcessorClass;
            CurrentEntry.ComponentEntry.Type = CentralProcessor;
            CurrentEntry.ComponentEntry.Key = i;
            CurrentEntry.ComponentEntry.AffinityMask = 1 << i;

            CurrentEntry.ComponentEntry.Identifier = Buffer;
#ifndef _IA64_
            if (Prcb->CpuID == 0) {

                //
                // Old style stepping format
                //

                sprintf (Buffer, CmpID1,
                    Prcb->CpuType,
                    (Prcb->CpuStep >> 8) + 'A',
                    Prcb->CpuStep & 0xff
                    );

            } else {

                //
                // New style stepping format
                //

                sprintf (Buffer, CmpID2,
                    Prcb->CpuType,
                    (Prcb->CpuStep >> 8),
                    Prcb->CpuStep & 0xff
                    );
            }
#endif _IA64_

            CurrentEntry.ComponentEntry.IdentifierLength =
                strlen (Buffer) + 1;

            Status = CmpInitializeRegistryNode(
                &CurrentEntry,
                ParentHandle,
                &BaseHandle,
                -1,
                (ULONG)-1,
                DeviceIndexTable
                );

            if (!NT_SUCCESS(Status)) {
                return(Status);
            }


#ifndef _IA64_
            if (KeI386NpxPresent) {
                RtlZeroMemory (&CurrentEntry, sizeof CurrentEntry);
                CurrentEntry.ComponentEntry.Class = ProcessorClass;
                CurrentEntry.ComponentEntry.Type = FloatingPointProcessor;
                CurrentEntry.ComponentEntry.Key = i;
                CurrentEntry.ComponentEntry.AffinityMask = 1 << i;

                CurrentEntry.ComponentEntry.Identifier = Buffer;

                if (Prcb->CpuType == 3) {

                    //
                    // 386 processors have 387's installed, else
                    // use processor identifier as the NPX identifier
                    //

                    strcpy (Buffer, "80387");
                }

                CurrentEntry.ComponentEntry.IdentifierLength =
                    strlen (Buffer) + 1;

                Status = CmpInitializeRegistryNode(
                    &CurrentEntry,
                    ParentHandle,
                    &NpxHandle,
                    -1,
                    (ULONG)-1,
                    DeviceIndexTable
                    );

                if (!NT_SUCCESS(Status)) {
                    NtClose(BaseHandle);
                    return(Status);
                }

                NtClose(NpxHandle);
            }

            //
            // If processor supports Cpu Indentification then
            // go obtain that information for the registry
            //

            VendorID = Prcb->CpuID ? Prcb->VendorString : NULL;

            //
            // Move to target processor and get other related
            // processor information for the registery
            //

            KeSetSystemAffinityThread(Prcb->SetMember);

            if (!Prcb->CpuID) {

                //
                // Test for Cyrix processor
                //

                if (Ke386CyrixId ()) {
                    VendorID = CmpCyrixID;
                }
            } else {

                //
                // If this processor has extended CPUID functions, get
                // the ProcessorNameString.  This is currently only
                // true for AMD processors.  Intel should be adding
                // this support shortly.  On AMD processors that don't
                // support extended functions, the CPUID instruction
                // returns 0 for function 0x80000000.
                //

                if (strcmp(VendorID, CmpAmdID) == 0) {

                    CPUID(CPUID_EXTFN_BASE, &MaxExtFn, &Junk, &Junk, &Junk);

                    if (MaxExtFn >= (CPUID_EXTFN_PROCESSOR_NAME + 2)) {

                        //
                        // This processor supports extended CPUID functions
                        // up to and (at least) including processor name string.
                        //
                        // Each CPUID call for the processor name string will
                        // return 16 bytes, 48 bytes in all, zero terminated.
                        //

                        NameString = &ProcessorNameString.u.DWords[0];

                        for (CpuIdFunction = CPUID_EXTFN_PROCESSOR_NAME;
                             CpuIdFunction <= (CPUID_EXTFN_PROCESSOR_NAME+2);
                             CpuIdFunction++) {

                            CPUID(CpuIdFunction,
                                  NameString,
                                  NameString + 1,
                                  NameString + 2,
                                  NameString + 3);
                            NameString += 4;
                        }

                        //
                        // Enforce 0 byte terminator.
                        //

                        ProcessorNameString.u.Bytes[CPUID_PROCESSOR_NAME_STRING_SZ-1] = 0;
                    }
                }
            }

            //
            // Restore thread's affinity to all processors
            //

            KeRevertToUserAffinityThread();

            if (NameString) {

                //
                // Add Processor Name String to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpProcessorNameString
                    );

                RtlInitAnsiString(
                    &AnsiString,
                    ProcessorNameString.u.Bytes
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            ValueData.Buffer,
                            ValueData.Length + sizeof( UNICODE_NULL )
                            );

                RtlFreeUnicodeString(&ValueData);
            }

            if (VendorID) {

                //
                // Add Vendor Indentifier to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpVendorID
                    );

                RtlInitAnsiString(
                    &AnsiString,
                    VendorID
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_SZ,
                            ValueData.Buffer,
                            ValueData.Length + sizeof( UNICODE_NULL )
                            );

                RtlFreeUnicodeString(&ValueData);
            }

            if (Prcb->FeatureBits) {
                //
                // Add processor feature bits to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpFeatureBits
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_DWORD,
                            &Prcb->FeatureBits,
                            sizeof (Prcb->FeatureBits)
                            );
            }

            if (Prcb->MHz) {
                //
                // Add processor MHz to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpMHz
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_DWORD,
                            &Prcb->MHz,
                            sizeof (Prcb->MHz)
                            );
            }

            if (Prcb->UpdateSignature.QuadPart) {
                //
                // Add processor MHz to the registery
                //

                RtlInitUnicodeString(
                    &ValueName,
                    CmpUpdateSignature
                    );

                Status = NtSetValueKey(
                            BaseHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_BINARY,
                            &Prcb->UpdateSignature,
                            sizeof (Prcb->UpdateSignature)
                            );
            }

            NtClose(BaseHandle);
#endif _IA64_
        }

        ExFreePool((PVOID)CmpConfigurationData);
    }

#ifndef _IA64_
    //
    // Next we try to collect System BIOS date and version strings.
    // BUGBUG This code should be moved to ntdetect.com after product 1.
    //

    //
    // Open a physical memory section to map in physical memory.
    //

    RtlInitUnicodeString(
        &SectionName,
        L"\\Device\\PhysicalMemory"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SectionName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    Status = ZwOpenSection(
        &SectionHandle,
        SECTION_ALL_ACCESS,
        &ObjectAttributes
        );

    if (!NT_SUCCESS(Status)) {

        //
        // If fail, forget the bios data and version
        //

        goto AllDone;
    }

    //
    // Examine the first page of physical memory for int 10 segment
    // address.
    //

    BaseAddress = 0;
    ViewSize = 0x1000;
    ViewBase.LowPart = 0;
    ViewBase.HighPart = 0;

    Status =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(Status)) {
        VideoBiosStart = VIDEO_BIOS_START;
    } else {
        VideoBiosStart = (*((PULONG)BaseAddress + INT10_VECTOR) & 0xFFFF0000) >> 12;
        VideoBiosStart += (*((PULONG)BaseAddress + INT10_VECTOR) & 0x0000FFFF);
        VideoBiosStart &= 0xffff8000;
        if (VideoBiosStart < VIDEO_BIOS_START) {
            VideoBiosStart = VIDEO_BIOS_START;
        }
        Status = ZwUnmapViewOfSection(
            NtCurrentProcess(),
            BaseAddress
            );
    }

    VersionStrings = ExAllocatePool(PagedPool, VERSION_DATA_LENGTH);
    BaseAddress = 0;
    ViewSize = SYSTEM_BIOS_LENGTH;
    ViewBase.LowPart = SYSTEM_BIOS_START;
    ViewBase.HighPart = 0;

    Status =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_READWRITE
        );

    if (NT_SUCCESS(Status)) {
        if (CmpGetBiosDate(BaseAddress, SYSTEM_BIOS_LENGTH, Buffer)) {

            //
            // Convert ascii date string to unicode string and
            // store it in registry.
            //

            RtlInitUnicodeString(
                &ValueName,
                L"SystemBiosDate"
                );

            RtlInitAnsiString(
                &AnsiString,
                Buffer
                );

            RtlAnsiStringToUnicodeString(
                &ValueData,
                &AnsiString,
                TRUE
                );

            Status = NtSetValueKey(
                        ParentHandle,
                        &ValueName,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        ValueData.Buffer,
                        ValueData.Length + sizeof( UNICODE_NULL )
                        );

            RtlFreeUnicodeString(&ValueData);
        }

        if (VersionStrings && CmpGetBiosVersion(BaseAddress, SYSTEM_BIOS_LENGTH, Buffer)) {
            VersionPointer = VersionStrings;
            do {

                //
                // Try to detect ALL the possible BIOS version strings.
                // Convert them to unicode strings and copy them to our
                // VersionStrings buffer.
                //

                RtlInitAnsiString(
                    &AnsiString,
                    Buffer
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Length = ValueData.Length + sizeof(UNICODE_NULL);
                RtlMoveMemory(VersionPointer,
                              ValueData.Buffer,
                              Length
                              );
                VersionsLength += Length;
                RtlFreeUnicodeString(&ValueData);
                if (VersionsLength + (MAXIMUM_BIOS_VERSION_LENGTH +
                    sizeof(UNICODE_NULL)) * 2 > PAGE_SIZE) {
                    break;
                }
                VersionPointer += Length;
            } while (CmpGetBiosVersion(NULL, 0, Buffer));

            if (VersionsLength != 0) {

                //
                // Append a UNICODE_NULL to the end of VersionStrings
                //

                *(PWSTR)VersionPointer = UNICODE_NULL;
                VersionsLength += sizeof(UNICODE_NULL);

                //
                // If any version string is found, we set up a ValueName and
                // initialize its value to the string(s) we found.
                //

                RtlInitUnicodeString(
                    &ValueName,
                    L"SystemBiosVersion"
                    );

                Status = NtSetValueKey(
                            ParentHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_MULTI_SZ,
                            VersionStrings,
                            VersionsLength
                            );
            }
        }
        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }

    //
    // Next we try to collect Video BIOS date and version strings.
    //

    BaseAddress = 0;
    ViewSize = VIDEO_BIOS_LENGTH;
    ViewBase.LowPart = VideoBiosStart;
    ViewBase.HighPart = 0;

    Status =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_READWRITE
        );

    if (NT_SUCCESS(Status)) {
        if (CmpGetBiosDate(BaseAddress, VIDEO_BIOS_LENGTH, Buffer)) {

            RtlInitUnicodeString(
                &ValueName,
                L"VideoBiosDate"
                );

            RtlInitAnsiString(
                &AnsiString,
                Buffer
                );

            RtlAnsiStringToUnicodeString(
                &ValueData,
                &AnsiString,
                TRUE
                );

            Status = NtSetValueKey(
                        ParentHandle,
                        &ValueName,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        ValueData.Buffer,
                        ValueData.Length + sizeof( UNICODE_NULL )
                        );

            RtlFreeUnicodeString(&ValueData);
        }

        if (VersionStrings && CmpGetBiosVersion(BaseAddress, VIDEO_BIOS_LENGTH, Buffer)) {
            VersionPointer = VersionStrings;
            do {

                //
                // Try to detect ALL the possible BIOS version strings.
                // Convert them to unicode strings and copy them to our
                // VersionStrings buffer.
                //

                RtlInitAnsiString(
                    &AnsiString,
                    Buffer
                    );

                RtlAnsiStringToUnicodeString(
                    &ValueData,
                    &AnsiString,
                    TRUE
                    );

                Length = ValueData.Length + sizeof(UNICODE_NULL);
                RtlMoveMemory(VersionPointer,
                              ValueData.Buffer,
                              Length
                              );
                VersionsLength += Length;
                RtlFreeUnicodeString(&ValueData);
                if (VersionsLength + (MAXIMUM_BIOS_VERSION_LENGTH +
                    sizeof(UNICODE_NULL)) * 2 > PAGE_SIZE) {
                    break;
                }
                VersionPointer += Length;
            } while (CmpGetBiosVersion(NULL, 0, Buffer));

            if (VersionsLength != 0) {

                //
                // Append a UNICODE_NULL to the end of VersionStrings
                //

                *(PWSTR)VersionPointer = UNICODE_NULL;
                VersionsLength += sizeof(UNICODE_NULL);

                RtlInitUnicodeString(
                    &ValueName,
                    L"VideoBiosVersion"
                    );

                Status = NtSetValueKey(
                            ParentHandle,
                            &ValueName,
                            TITLE_INDEX_VALUE,
                            REG_MULTI_SZ,
                            VersionStrings,
                            VersionsLength
                            );
            }
        }
        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }
    ZwClose(SectionHandle);
    if (VersionStrings) {
        ExFreePool((PVOID)VersionStrings);
    }

AllDone:

#endif _IA64_

    NtClose (ParentHandle);

    //
    // Add any other x86 specific code here...
    //

    return STATUS_SUCCESS;
}
