/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    geninst.c

Abstract:

    This modules contains routines to implement GenInstall of an inf section.
    This is based on the code from the setupapi. Currently, it only supports
    a subset of GenInstall functionality i.e AddReg and DelReg and BitReg.

Author:

    Santosh Jodh (santoshj) 08-Aug-1998


Environment:

    Kernel mode.

Revision History:

--*/


#include "cmp.h"
#include "stdlib.h"
#include "parseini.h"
#include "geninst.h"

typedef
BOOLEAN
(* PFN_INFRULE)(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN PVOID RefData
    );

typedef
BOOLEAN
(* PFN_REGLINE)(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    );

BOOLEAN
CmpProcessReg(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN PVOID RefData
    );

NTSTATUS
CmpProcessAddRegLine(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    );

NTSTATUS
CmpProcessDelRegLine(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    );

NTSTATUS
CmpProcessBitRegLine(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    );

NTSTATUS
CmpGetAddRegInfData(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN ULONG ValueType,
    OUT PVOID *Data,
    OUT PULONG DataSize
    );

NTSTATUS
CmpOpenRegKey(
    IN OUT PHANDLE Key,
    IN OUT PULONG Disposition,
    IN PCHAR Root,
    IN PCHAR SubKey,
    IN ULONG DesiredAccess,
    IN BOOLEAN Create
    );

NTSTATUS
CmpAppendStringToMultiSz(
    IN HANDLE Key,
    IN PCHAR ValueName,
    IN OUT PVOID *Data,
    IN OUT PULONG DataSize
    );

//
// Copied from setupapi.h
//
// Flags for AddReg section lines in INF.  The corresponding value
// is <ValueType> in the AddReg line format given below:
//
// <RegRootString>,<SubKey>,<ValueName>,<ValueType>,<Value>...
//
// The low word contains basic flags concerning the general data type
// and AddReg action. The high word contains values that more specifically
// identify the data type of the registry value.  The high word is ignored
// by the 16-bit Windows 95 SETUPX APIs.
//

#define FLG_ADDREG_BINVALUETYPE     ( 0x00000001 )
#define FLG_ADDREG_NOCLOBBER        ( 0x00000002 )
#define FLG_ADDREG_DELVAL           ( 0x00000004 )
#define FLG_ADDREG_APPEND           ( 0x00000008 ) // Currently supported only
                                                   // for REG_MULTI_SZ values.
#define FLG_ADDREG_KEYONLY          ( 0x00000010 ) // Just create the key, ignore value
#define FLG_ADDREG_OVERWRITEONLY    ( 0x00000020 ) // Set only if value already exists

#define FLG_ADDREG_TYPE_MASK        ( 0xFFFF0000 | FLG_ADDREG_BINVALUETYPE )
#define FLG_ADDREG_TYPE_SZ          ( 0x00000000                           )
#define FLG_ADDREG_TYPE_MULTI_SZ    ( 0x00010000                           )
#define FLG_ADDREG_TYPE_EXPAND_SZ   ( 0x00020000                           )
#define FLG_ADDREG_TYPE_BINARY      ( 0x00000000 | FLG_ADDREG_BINVALUETYPE )
#define FLG_ADDREG_TYPE_DWORD       ( 0x00010000 | FLG_ADDREG_BINVALUETYPE )
#define FLG_ADDREG_TYPE_NONE        ( 0x00020000 | FLG_ADDREG_BINVALUETYPE )

#define FLG_BITREG_CLEAR            ( 0x00000000 )
#define FLG_BITREG_SET              ( 0x00000001 )
#define FLG_BITREG_TYPE_BINARY      ( 0x00000000 )
#define FLG_BITREG_TYPE_DWORD       ( 0x00000002 )

//
// We currently only support AddReg and DelReg sections.
//

#define NUM_OF_INF_RULES    3

//
// GenInstall methods we support.
//

struct {
    PCHAR       Name;
    PFN_INFRULE Action;
    PVOID       RefData;
} gInfRuleTable[NUM_OF_INF_RULES] =
{
    {"AddReg", CmpProcessReg, CmpProcessAddRegLine},
    {"DelReg", CmpProcessReg, CmpProcessDelRegLine},
    {"BitReg", CmpProcessReg, CmpProcessBitRegLine}
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpAppendStringToMultiSz)
#pragma alloc_text(INIT,CmpOpenRegKey)
#pragma alloc_text(INIT,CmpGetAddRegInfData)
#pragma alloc_text(INIT,CmpProcessReg)
#pragma alloc_text(INIT,CmpProcessAddRegLine)
#pragma alloc_text(INIT,CmpProcessDelRegLine)
#pragma alloc_text(INIT,CmpProcessBitRegLine)
#pragma alloc_text(INIT,CmpGenInstall)
#endif

BOOLEAN
CmpGenInstall(
    IN PVOID InfHandle,
    IN PCHAR Section
    )

/*++

    Routine Description:

        This routine does a GenInstall of the section in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

    Return Value:

        TRUE iff the entire section was processed successfully.

--*/

{
    ULONG   ruleNumber;
    ULONG   i;
    PCHAR   ruleName;
    PCHAR   regSection;
    BOOLEAN result = FALSE;

    if (CmpSearchInfSection(InfHandle, Section))
    {
        //
        // Go through all the rules in the section and try to process
        // each of them.
        //

        for (   ruleNumber = 0;
                ruleName = CmpGetKeyName(InfHandle, Section, ruleNumber);
                ruleNumber++)
        {

            //
            // Search for the proceesing function in our table.
            //

            for (   i = 0;
                    i < NUM_OF_INF_RULES &&
                        _stricmp(ruleName, gInfRuleTable[i].Name);
                    i++);

            if (    i >= NUM_OF_INF_RULES ||
                    (regSection = CmpGetSectionLineIndex(   InfHandle,
                                                            Section,
                                                            ruleNumber,
                                                            0)) == NULL ||
                    !CmpSearchInfSection(InfHandle, Section))
            {
                result = FALSE;
                break;
            }

            if (!(*gInfRuleTable[i].Action)(InfHandle, regSection, gInfRuleTable[i].RefData))
            {
                result = FALSE;
            }
        }

        //
        // All inf rules processed.
        //

        if (ruleNumber)
        {
            result = TRUE;
        }
    }

    return (result);
}

BOOLEAN
CmpProcessReg(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN PVOID RefData
    )

/*++

    Routine Description:

        This routine processes a AddReg section in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

    Return Value:

        TRUE iff the entire section was processed successfully.

--*/

{
    ULONG       lineIndex;
    NTSTATUS    status = STATUS_SUCCESS;
    NTSTATUS    temp;

    //
    // Process all the lines in the xxxReg Section.
    //

    for (   lineIndex = 0;
            CmpSearchInfLine(InfHandle, Section, lineIndex);
            lineIndex++)
    {
        temp = (*(PFN_REGLINE)RefData)(InfHandle, Section, lineIndex);
        if (!NT_SUCCESS(temp))
        {
            status = temp;
        }
    }

    if (NT_SUCCESS(status))
    {
        return (TRUE);
    }

    return (FALSE);
}

NTSTATUS
CmpProcessAddRegLine(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    )

/*++

    Routine Description:

        This routine processes a AddReg line in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

    Return Value:

        Standard NT status value.

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PCHAR               rootKeyName;
    PCHAR               subKeyName;
    PCHAR               valueName;
    ULONG               flags;
    ULONG               valueType;
    PCHAR               buffer;
    HANDLE              key;
    ULONG               disposition;
    BOOLEAN             dontSet;
    PVOID               data = 0;
    ULONG               dataSize = 0;
    ANSI_STRING         ansiString;
    UNICODE_STRING      unicodeString;
    OBJECT_ATTRIBUTES   objectAttributes;

    //
    // Get the root-key name.
    //

    rootKeyName = CmpGetSectionLineIndex(   InfHandle,
                                            Section,
                                            LineIndex,
                                            0);
    if (rootKeyName)
    {
        //
        // Get the optional sub-key name.
        //

        subKeyName = CmpGetSectionLineIndex(    InfHandle,
                                                Section,
                                                LineIndex,
                                                1);

        //
        // Value name is optional. Can be NULL or "".
        //

        valueName = CmpGetSectionLineIndex( InfHandle,
                                            Section,
                                            LineIndex,
                                            2);
        //
        // If we don't have a value name, the type is REG_SZ to force
        // the right behavior in RegSetValueEx. Otherwise get the data type.
        //

        valueType = REG_SZ;

        //
        // Read in the flags.
        //

        if (!CmpGetIntField(    InfHandle,
                                Section,
                                LineIndex,
                                3,
                                &flags))
        {
            flags = 0;
        }

        //
        // Convert the flags to the registry type.
        //

        switch(flags & FLG_ADDREG_TYPE_MASK)
        {

            case FLG_ADDREG_TYPE_SZ:

                valueType = REG_SZ;
                break;

            case FLG_ADDREG_TYPE_MULTI_SZ:

                valueType = REG_MULTI_SZ;
                break;

            case FLG_ADDREG_TYPE_EXPAND_SZ:

                valueType = REG_EXPAND_SZ;
                break;

            case FLG_ADDREG_TYPE_BINARY:

                valueType = REG_BINARY;
                break;

            case FLG_ADDREG_TYPE_DWORD:

                valueType = REG_DWORD;
                break;

            case FLG_ADDREG_TYPE_NONE:

                valueType = REG_NONE;
                break;

            default :

                //
                // If the FLG_ADDREG_BINVALUETYPE is set, then the highword
                // can contain just about any random reg data type ordinal value.
                //

                if(flags & FLG_ADDREG_BINVALUETYPE)
                {
                    //
                    // Disallow the following reg data types:
                    //
                    //    REG_NONE, REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ
                    //

                    valueType = HIGHWORD(flags);

                    if(valueType < REG_BINARY || valueType == REG_MULTI_SZ)
                    {
                        return (STATUS_INVALID_PARAMETER);
                    }

                }
                else
                {
                    return (STATUS_INVALID_PARAMETER);
                }
                break;
        }

        //
        // Presently, the append behavior flag is only supported for
        // REG_MULTI_SZ values.
        //

        if((flags & FLG_ADDREG_APPEND) && valueType != REG_MULTI_SZ)
        {
            return (STATUS_INVALID_PARAMETER);
        }

        //
        // W9x compatibility.
        //

        if( (!valueName || *valueName == '\0') && valueType == REG_EXPAND_SZ)
        {
            valueType = REG_SZ;
        }

        status = CmpGetAddRegInfData(   InfHandle,
                                        Section,
                                        LineIndex,
                                        4,
                                        valueType,
                                        &data,
                                        &dataSize);

        if (NT_SUCCESS(status))
        {
            //
            // Open the specified key if possible.
            //

            status = CmpOpenRegKey( &key,
                                    &disposition,
                                    rootKeyName,
                                    subKeyName,
                                    KEY_QUERY_VALUE | KEY_SET_VALUE,
                                    (BOOLEAN)!(flags & FLG_ADDREG_OVERWRITEONLY));
            //
            // This variable gets set to TRUE if we dont actually want to set
            // the value.
            //

            dontSet = FALSE;
            if (NT_SUCCESS(status))
            {
                if (flags & FLG_ADDREG_APPEND)
                {
                    status = CmpAppendStringToMultiSz(  key,
                                                        valueName,
                                                        &data,
                                                        &dataSize);
                }
                if (NT_SUCCESS(status))
                {
                    //
                    // W9x compatibility.
                    //

                    if (disposition == REG_OPENED_EXISTING_KEY)
                    {
                        if (    (flags & FLG_ADDREG_NOCLOBBER) &&
                                (valueName == NULL || *valueName == '\0'))
                        {
                            RtlInitAnsiString(&ansiString, "");
                            status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
                            if (NT_SUCCESS(status))
                            {

                                status = NtQueryValueKey(   key,
                                                            &unicodeString,
                                                            KeyValueBasicInformation,
                                                            NULL,
                                                            0,
                                                            &disposition);
                                if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL)
                                {
                                    flags &= ~FLG_ADDREG_NOCLOBBER;
                                }
                                status = STATUS_SUCCESS;

                                RtlFreeUnicodeString(&unicodeString);
                            }
                        }

                        if (flags & FLG_ADDREG_DELVAL)
                        {
                            //
                            // setupx compatibility.
                            //

                            dontSet = TRUE;
                            if (valueName)
                            {
                                //
                                // Delete the specified value.
                                //

                                RtlInitAnsiString(&ansiString, valueName);
                                status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
                                if (NT_SUCCESS(status))
                                {
                                    status = NtDeleteValueKey(key, &unicodeString);
                                    RtlFreeUnicodeString(&unicodeString);
                                }
                            }
                        }
                    }
                    else
                    {
                        flags &= ~FLG_ADDREG_NOCLOBBER;
                    }

                    if (!dontSet)
                    {
                        //
                        // Respect the key only flag.
                        //

                        if (!(flags & FLG_ADDREG_KEYONLY))
                        {
                            //
                            // If no clobber flag is set, make sure that the value does not
                            // already exist.
                            //

                            RtlInitAnsiString(&ansiString, valueName);
                            status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
                            if (NT_SUCCESS(status))
                            {
                                NTSTATUS    existStatus;

                                if (flags & FLG_ADDREG_NOCLOBBER)
                                {
                                    existStatus = NtQueryValueKey(  key,
                                                                    &unicodeString,
                                                                    KeyValueBasicInformation,
                                                                    NULL,
                                                                    0,
                                                                    &disposition);
                                    if (NT_SUCCESS(existStatus) || existStatus == STATUS_BUFFER_TOO_SMALL) {
                                        dontSet = TRUE;
                                    }
                                }
                                else
                                {
                                    if (flags & FLG_ADDREG_OVERWRITEONLY)
                                    {
                                        existStatus = NtQueryValueKey(  key,
                                                                        &unicodeString,
                                                                        KeyValueBasicInformation,
                                                                        NULL,
                                                                        0,
                                                                        &disposition);
                                        if (!NT_SUCCESS(existStatus) && existStatus != STATUS_BUFFER_TOO_SMALL) {
                                            dontSet = TRUE;
                                        }
                                    }
                                }

                                if (!dontSet)
                                {
                                    status = NtSetValueKey( key,
                                                            &unicodeString,
                                                            0,
                                                            valueType,
                                                            data,
                                                            dataSize);
                                }

                                RtlFreeUnicodeString(&unicodeString);
                            }
                        }
                    }
                }

                NtClose(key);
            }
            else if (flags & FLG_ADDREG_OVERWRITEONLY)
            {
                status = STATUS_SUCCESS;
            }
        }
    }

    return (status);
}

NTSTATUS
CmpProcessDelRegLine(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    )

/*++

    Routine Description:

        This routine processes a DelReg line in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

    Return Value:

        Standard NT status value.

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PCHAR               rootKeyName;
    PCHAR               subKeyName;
    PCHAR               valueName;
    HANDLE              key;
    ULONG               disposition;
    ANSI_STRING         ansiString;
    UNICODE_STRING      unicodeString;

    //
    // Read the required fields.
    //

    rootKeyName = CmpGetSectionLineIndex(   InfHandle,
                                            Section,
                                            LineIndex,
                                            0);

    subKeyName = CmpGetSectionLineIndex(    InfHandle,
                                            Section,
                                            LineIndex,
                                            1);

    if (rootKeyName && subKeyName)
    {
        //
        // Read the optional field.
        //

        valueName = CmpGetSectionLineIndex( InfHandle,
                                            Section,
                                            LineIndex,
                                            2);

        //
        // Open the specified registry key.
        //

        status = CmpOpenRegKey( &key,
                                &disposition,
                                rootKeyName,
                                subKeyName,
                                KEY_ALL_ACCESS,
                                FALSE);

        //
        // Proceed if we successfully opened the registry key.
        //

        if (NT_SUCCESS(status))
        {

            //
            // If the key was successfully opened, do the DelReg.
            //

            if (valueName)
            {
                //
                // Delete the specified value.
                //

                RtlInitAnsiString(&ansiString, valueName);
                status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
                if (NT_SUCCESS(status))
                {
                    status = NtDeleteValueKey(key, &unicodeString);
                    RtlFreeUnicodeString(&unicodeString);
                }
            }
            else
            {
                //
                // No value specified. The subkey needs to be deleted.
                //

                status = NtDeleteKey(key);
            }

            //
            // Close the key handle.
            //

            NtClose(key);
        }
    }

    return (status);
}

NTSTATUS
CmpProcessBitRegLine(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    )

/*++

    Routine Description:

        This routine processes a BitReg line in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

    Return Value:

        Standard NT status value.

--*/

{
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    PCHAR                       rootKeyName;
    PCHAR                       subKeyName;
    PCHAR                       valueName;
    ULONG                       flags;
    ULONG                       mask;
    ULONG                       field;
    HANDLE                      key;
    ULONG                       disposition;
    ANSI_STRING                 ansiString;
    UNICODE_STRING              unicodeString;
    PCHAR                       buffer;
    ULONG                       size;
    PKEY_VALUE_FULL_INFORMATION valueInfo;

    //
    // Get the root-key name.
    //

    rootKeyName = CmpGetSectionLineIndex(   InfHandle,
                                            Section,
                                            LineIndex,
                                            0);
    if (rootKeyName)
    {
        //
        // Get the optional sub-key name.
        //

        subKeyName = CmpGetSectionLineIndex(    InfHandle,
                                                Section,
                                                LineIndex,
                                                1);

        //
        // Value name is optional. Can be NULL or "".
        //

        valueName = CmpGetSectionLineIndex( InfHandle,
                                            Section,
                                            LineIndex,
                                            2);
        if (valueName && *valueName)
        {
            //
            // Read in the flags.
            //

            if (!CmpGetIntField(    InfHandle,
                                    Section,
                                    LineIndex,
                                    3,
                                    &flags))
            {
                flags = 0;
            }

            if (!CmpGetIntField(    InfHandle,
                                    Section,
                                    LineIndex,
                                    4,
                                    &mask))
            {
                mask = 0;
            }

            if (!(flags & FLG_BITREG_TYPE_DWORD))
            {
                if (!CmpGetIntField(    InfHandle,
                                        Section,
                                        LineIndex,
                                        5,
                                        &field))
                {
                    return (status);
                }
            }

            //
            // Open the specified registry key.
            //

            status = CmpOpenRegKey( &key,
                                    &disposition,
                                    rootKeyName,
                                    subKeyName,
                                    KEY_ALL_ACCESS,
                                    FALSE);
            if (NT_SUCCESS(status))
            {
                //
                // Read the existing data.
                //

                RtlInitAnsiString(&ansiString, valueName);
                status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
                if (NT_SUCCESS(status))
                {
                    size = 0;
                    status = NtQueryValueKey(   key,
                                                &unicodeString,
                                                KeyValueFullInformation,
                                                NULL,
                                                0,
                                                &size);
                    if (size)
                    {
                        status = STATUS_NO_MEMORY;
                        buffer = ExAllocatePoolWithTag(PagedPool, size, CM_GENINST_TAG);
                        if (buffer)
                        {
                            status = NtQueryValueKey(   key,
                                                        &unicodeString,
                                                        KeyValueFullInformation,
                                                        buffer,
                                                        size,
                                                        &size);
                            if (NT_SUCCESS(status))
                            {
                                valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
                                if (flags & FLG_BITREG_TYPE_DWORD)
                                {
                                    if (valueInfo->Type == REG_DWORD && valueInfo->DataLength == sizeof(ULONG))
                                    {
                                        if (flags & FLG_BITREG_SET)
                                        {
                                            *(PULONG)(buffer + valueInfo->DataOffset) |= mask;
                                        }
                                        else
                                        {
                                            *(PULONG)(buffer + valueInfo->DataOffset) &= ~mask;
                                        }
                                    }
                                }
                                else
                                {
                                    if (valueInfo->Type == REG_BINARY && field < valueInfo->DataLength)
                                    {
                                        if (flags & FLG_BITREG_SET)
                                        {
                                            *(PUCHAR)(buffer + valueInfo->DataOffset + field) |= mask;
                                        }
                                        else
                                        {
                                            *(PUCHAR)(buffer + valueInfo->DataOffset + field) &= ~mask;
                                        }
                                    }
                                }
                                status = NtSetValueKey( key,
                                                        &unicodeString,
                                                        0,
                                                        valueInfo->Type,
                                                        buffer + valueInfo->DataOffset,
                                                        valueInfo->DataLength);
                            }
                            else
                            {
                                DbgPrint("Value cannot be read for BitReg in %s line %d\n", Section, LineIndex);
                                ASSERT(NT_SUCCESS(status));
                            }
                            ExFreePool(buffer);
                        }
                        else
                        {
                            ASSERT(buffer);
                            status = STATUS_NO_MEMORY;
                        }
                    }

                    RtlFreeUnicodeString(&unicodeString);
                }
                //
                // Close the key handle.
                //

                NtClose(key);
            }
        }
    }

    return (status);
}

NTSTATUS
CmpGetAddRegInfData(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN ULONG ValueType,
    OUT PVOID *Data,
    OUT PULONG DataSize
    )

/*++

    Routine Description:

        This routine reads AddReg data from the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

        ValueIndex - Index of the value to be read.

        ValueType - Data type to be read.

        Data - Receives pointer to the buffer in which data has been read.

        DataSize - Receives the size of the data buffer.

    Return Value:

        Standard NT status value.

--*/

{
    NTSTATUS        status = STATUS_UNSUCCESSFUL;
    PCHAR           str;
    ULONG           count;
    ULONG           i;
    ANSI_STRING     ansiString;
    UNICODE_STRING  unicodeString;

    //
    // Validate the required fields.
    //

    ASSERT(Data);
    ASSERT(DataSize);

    switch (ValueType)
    {
        case REG_DWORD:

            *DataSize = sizeof(ULONG);
            *Data = ExAllocatePoolWithTag(PagedPool, *DataSize, CM_GENINST_TAG);
            if (*Data)
            {
                //
                // DWORD data is specified as four bytes in W9x.
                //

                if (CmpGetSectionLineIndexValueCount(   InfHandle,
                                                        Section,
                                                        LineIndex) == 8)
                {
                    if (!CmpGetBinaryField( InfHandle,
                                            Section,
                                            LineIndex,
                                            ValueIndex,
                                            *Data,
                                            *DataSize,
                                            NULL))
                    {
                        *((PULONG)*Data) = 0;
                    }

                    status = STATUS_SUCCESS;
                }
                else
                {
                    //
                    // Get the DWORD value.
                    //

                    if (!CmpGetIntField(    InfHandle,
                                            Section,
                                            LineIndex,
                                            4,
                                            *Data))
                    {
                        *((PULONG)*Data) = 0;
                    }

                    status = STATUS_SUCCESS;
                }
            }
            else
            {
                ASSERT(*Data);
                status = STATUS_NO_MEMORY;
            }

            break;

        case REG_SZ:
        case REG_EXPAND_SZ:

            //
            // Null terminated string. Gets converted to unicode before being
            // added into the registry.
            //

            str = CmpGetSectionLineIndex(   InfHandle,
                                            Section,
                                            LineIndex,
                                            ValueIndex);
            if (str)
            {
                RtlInitAnsiString(&ansiString, str);
                *DataSize = (ansiString.Length << 1) + sizeof(UNICODE_NULL);
                unicodeString.MaximumLength = (USHORT)*DataSize;
                unicodeString.Buffer = ExAllocatePoolWithTag(PagedPool, *DataSize, CM_GENINST_TAG);
                *Data = NULL;
                if (unicodeString.Buffer)
                {
                    status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, FALSE);
                    if (NT_SUCCESS(status))
                    {
                        *Data = unicodeString.Buffer;
                        status = STATUS_SUCCESS;
                    }
                }
                else
                {
                    ASSERT(unicodeString.Buffer);
                    status = STATUS_NO_MEMORY;
                }
            }
            else
            {
                ASSERT(str);
                status = STATUS_NO_MEMORY;
            }

            break;

        case REG_MULTI_SZ:

            *DataSize = 0;
            *Data = NULL;

            //
            // Loop to determine the total memory that needs to be allocated.
            //

            count = CmpGetSectionLineIndexValueCount(   InfHandle,
                                                        Section,
                                                        LineIndex);
            if (count > ValueIndex)
            {
                count -= ValueIndex;
                for (i = 0; i < count; i++)
                {
                    str = CmpGetSectionLineIndex(   InfHandle,
                                                    Section,
                                                    LineIndex,
                                                    ValueIndex + i);
                    if (str == NULL)
                    {
                        break;
                    }

                    *DataSize += ((strlen(str) * sizeof(WCHAR)) + sizeof(UNICODE_NULL));
                }

                if (i == count)
                {
                    //
                    // Account for the terminating NULL.
                    //

                    *DataSize += sizeof(UNICODE_NULL);
                    *Data = ExAllocatePoolWithTag(PagedPool, *DataSize, CM_GENINST_TAG);
                    if (*Data)
                    {
                        for (   i = 0, unicodeString.Buffer = *Data;
                                i < count;
                                i++, (PCHAR)unicodeString.Buffer += unicodeString.MaximumLength)
                        {
                            str = CmpGetSectionLineIndex(   InfHandle,
                                                            Section,
                                                            LineIndex,
                                                            ValueIndex + i);
                            if (str == NULL)
                            {
                                break;
                            }
                            RtlInitAnsiString(&ansiString, str);
                            unicodeString.MaximumLength = (ansiString.Length * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
                            status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, FALSE);
                            if (!NT_SUCCESS(status))
                            {
                                break;
                            }
                        }

                        //
                        // Terminate the multi-sz string.
                        //

                        if (i == count)
                        {
                            unicodeString.Buffer[0] = UNICODE_NULL;
                            status = STATUS_SUCCESS;
                        }
                    }
                    else
                    {
                        ASSERT(*Data);
                        status = STATUS_NO_MEMORY;
                    }
                }
            }

            break;

        case REG_BINARY:
        default:

            //
            // Free form binary data.
            //

            if (CmpGetBinaryField(  InfHandle,
                                    Section,
                                    LineIndex,
                                    ValueIndex,
                                    NULL,
                                    0,
                                    DataSize) && *DataSize)
            {
                *Data = ExAllocatePoolWithTag(PagedPool, *DataSize, CM_GENINST_TAG);
                if (*Data)
                {
                    if (CmpGetBinaryField( InfHandle,
                                            Section,
                                            LineIndex,
                                            4,
                                            *Data,
                                            *DataSize,
                                            NULL))
                    {
                        status = STATUS_SUCCESS;
                    }
                }
                else
                {
                    ASSERT(*Data);
                    status = STATUS_NO_MEMORY;
                }
            }
            else
            {
                status = STATUS_UNSUCCESSFUL;
            }

            break;
    }

    return (status);
}

NTSTATUS
CmpOpenRegKey(
    IN OUT PHANDLE Key,
    IN OUT PULONG Disposition,
    IN PCHAR Root,
    IN PCHAR SubKey,
    IN ULONG DesiredAccess,
    IN BOOLEAN Create
    )

/*++

    Routine Description:

        This routine opens\creates a handle to the registry key.

    Input Parameters:

        Key - Receives the handle to the key.

        Disposition - Receives the disposition of the key.

        Root - Abbreviated name of the root key.

        SubKey - Name of the subkey under the root.

        DesiredAccess - Desired access flags for the key.

        Create - TRUE if the key needs to be created instead of opened.

    Return Value:

        Standard NT status value.

--*/

{
    NTSTATUS            status = STATUS_OBJECT_NAME_INVALID;
    ULONG               size;
    PCHAR               str;
    ANSI_STRING         ansiString;
    UNICODE_STRING      unicodeString;
    OBJECT_ATTRIBUTES   objectAttributes;

    str = NULL;
    size = strlen(SubKey) + 1;

    //
    // Check if we understand the specified root name.
    //

    if (_stricmp(Root, "HKLM") == 0)
    {
        size += strlen("\\Registry\\Machine\\");
        str = ExAllocatePoolWithTag(PagedPool, size, CM_GENINST_TAG);
        if (str)
        {
            strcpy(str, "\\Registry\\Machine\\");
            strcat(str, SubKey);
        }
        else
        {
            ASSERT(str);
            status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        ASSERT(_stricmp(Root, "HKLM") == 0);
    }

    //
    // Proceed if we have a valid key name.
    //

    if (str)
    {
        RtlInitAnsiString(&ansiString, str);
        status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
        if (NT_SUCCESS(status))
        {
            InitializeObjectAttributes( &objectAttributes,
                                        &unicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL);
            if (Create)
            {
                //
                // Create a new key or open an existing one.
                //

                status = NtCreateKey(   Key,
                                        DesiredAccess,
                                        &objectAttributes,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        Disposition ? Disposition : &size);
            }
            else
            {
                //
                // Open existing key.
                //

                if (Disposition)
                {
                    *Disposition = REG_OPENED_EXISTING_KEY;
                }
                status = NtOpenKey( Key,
                                    DesiredAccess,
                                    &objectAttributes);
            }

            RtlFreeUnicodeString(&unicodeString);
        }
        else
        {
            ASSERT(NT_SUCCESS(status));
        }

        ExFreePool(str);
    }

    return (status);
}

NTSTATUS
CmpAppendStringToMultiSz(
    IN HANDLE Key,
    IN PCHAR ValueName,
    IN OUT PVOID *Data,
    IN OUT PULONG DataSize
    )

/*++

    Routine Description:

        This routine opens\creates a handle to the registry key.

    Input Parameters:

        Key - Receives the handle to the key.

        ValueName - Name of the value to be appended to.

        Data - Buffer containing the multi-sz to be appended.

        DataSize - Size of the data.

    Return Value:

        Standard NT status value.

--*/

{
    NTSTATUS                    status;
    ULONG                       size;
    ANSI_STRING                 ansiString;
    UNICODE_STRING              unicodeString;
    PKEY_VALUE_FULL_INFORMATION valueInfo;
    PVOID                       buffer;
    PVOID                       str;

    ASSERT(DataSize && *DataSize);
    ASSERT(*Data);

    RtlInitAnsiString(&ansiString, ValueName);
    status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
    if (NT_SUCCESS(status))
    {
        size = 0;
        status = NtQueryValueKey(   Key,
                                    &unicodeString,
                                    KeyValueFullInformation,
                                    NULL,
                                    0,
                                    &size);
        if (size)
        {
            buffer = ExAllocatePoolWithTag(PagedPool, size, CM_GENINST_TAG);
            if (buffer)
            {
                status = NtQueryValueKey(   Key,
                                            &unicodeString,
                                            KeyValueFullInformation,
                                            buffer,
                                            size,
                                            &size);
                if (NT_SUCCESS(status))
                {
                    valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;
                    str = ExAllocatePoolWithTag(    PagedPool,
                                                    valueInfo->DataLength +
                                                        *DataSize - sizeof(UNICODE_NULL),
                                                    CM_GENINST_TAG);
                    if (str)
                    {
                        memcpy( str,
                                (PCHAR)buffer + valueInfo->DataOffset,
                                valueInfo->DataLength);
                        memcpy( (PCHAR)str + valueInfo->DataLength - sizeof(UNICODE_NULL),
                                *Data,
                                *DataSize);
                        ExFreePool(*Data);
                        *Data = str;
                        *DataSize += valueInfo->DataLength - sizeof(UNICODE_NULL);
                    }
                    else
                    {
                        DbgPrint("CmpAppendStringToMultiSz: Failed to allocate memory!\n");
                        ASSERT(str);
                        status = STATUS_NO_MEMORY;
                    }
                }
                ExFreePool(buffer);
            }
            else
            {
                DbgPrint("CmpAppendStringToMultiSz: Failed to allocate memory!\n");
                ASSERT(buffer);
                status = STATUS_NO_MEMORY;
            }
        }
        RtlFreeUnicodeString(&unicodeString);
    }

    return (status);
}
