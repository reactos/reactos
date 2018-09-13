/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmcontrl.c

Abstract:

    The module contains CmGetSystemControlValues, see cmdat.c for data.

Author:

    Bryan M. Willman (bryanwi) 12-May-92

Revision History:

--*/

#include    "cmp.h"

extern WCHAR   CmDefaultLanguageId[];
extern ULONG   CmDefaultLanguageIdLength;
extern ULONG   CmDefaultLanguageIdType;

extern WCHAR   CmInstallUILanguageId[];
extern ULONG   CmInstallUILanguageIdLength;
extern ULONG   CmInstallUILanguageIdType;

HCELL_INDEX
CmpWalkPath(
    PHHIVE      SystemHive,
    HCELL_INDEX ParentCell,
    PWSTR       Path
    );

LANGID
CmpConvertLangId(
    PWSTR LangIdString,
    ULONG LangIdStringLength
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmGetSystemControlValues)
#pragma alloc_text(INIT,CmpWalkPath)
#endif

VOID
CmGetSystemControlValues(
    PVOID                   SystemHiveBuffer,
    PCM_SYSTEM_CONTROL_VECTOR  ControlVector
    )
/*++

Routine Description:

    Look for registry values in current control set, as specified
    by entries in ControlVector.  Report data for value entries
    (if any) to variables ControlVector points to.

Arguments:

    SystemHiveBuffer - pointer to flat image of the system hive

    ControlVector - pointer to structure that describes what values
                    to suck out and store

Return Value:

    NONE.

--*/
{
    NTSTATUS        status;
    PHHIVE          SystemHive;
    CMHIVE          TempHive;
    HCELL_INDEX     RootCell;
    HCELL_INDEX     BaseCell;
    UNICODE_STRING  Name;
    PHCELL_INDEX    Index;
    HCELL_INDEX     KeyCell;
    HCELL_INDEX     ValueCell;
    PCM_KEY_VALUE   ValueBody;
    PVOID           ValueData;
    ULONG           Length;
    BOOLEAN         AutoSelect;
    BOOLEAN         small;
    ULONG           tmplength;


    //
    // set up to read flat system hive image loader passes us
    //
    RtlZeroMemory((PVOID)&TempHive, sizeof(TempHive));
    SystemHive = &(TempHive.Hive);
    status = HvInitializeHive(
                SystemHive,
                HINIT_FLAT,
                HIVE_VOLATILE,
                HFILE_TYPE_PRIMARY,
                SystemHiveBuffer,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                1,
                NULL
                );
    if (!NT_SUCCESS(status)) {
        KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO,1,1,0,0);
    }

    //
    // get hive.cell of root of current control set
    //
    RootCell = ((PHBASE_BLOCK)SystemHiveBuffer)->RootCell;
    RtlInitUnicodeString(&Name, L"current");
    BaseCell = CmpFindControlSet(
                    SystemHive,
                    RootCell,
                    &Name,
                    &AutoSelect
                    );
    if (BaseCell == HCELL_NIL) {
        KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO,1,2,0,0);
    }

    RtlInitUnicodeString(&Name, L"control");
    BaseCell = CmpFindSubKeyByName(SystemHive,
                                   (PCM_KEY_NODE)HvGetCell(SystemHive,BaseCell),
                                   &Name);
    if (BaseCell == HCELL_NIL) {
        KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO,1,3,0,0);
    }

    //
    // SystemHive.BaseCell = \registry\machine\system\currentcontrolset\control
    //

    //
    // step through vector, trying to fetch each value
    //
    while (ControlVector->KeyPath != NULL) {

        //
        //  Assume we will fail to find the key or value.
        //
        
        Length = (ULONG)-1;

        KeyCell = CmpWalkPath(SystemHive, BaseCell, ControlVector->KeyPath);

        if (KeyCell != HCELL_NIL) {

            //
            // found the key, look for the value entry
            //
            RtlInitUnicodeString(&Name, ControlVector->ValueName);
            ValueCell = CmpFindValueByName(SystemHive,
                                           (PCM_KEY_NODE)HvGetCell(SystemHive,KeyCell),
                                           &Name);
            if (ValueCell != HCELL_NIL) {

                //
                // SystemHive.ValueCell is value entry body
                //

                if (ControlVector->BufferLength == NULL) {
                    tmplength = sizeof(ULONG);
                } else {
                    tmplength = *(ControlVector->BufferLength);
                }

                ValueBody = (PCM_KEY_VALUE)HvGetCell(SystemHive, ValueCell);

                small = CmpIsHKeyValueSmall(Length, ValueBody->DataLength);

                if (tmplength < Length) {
                    Length = tmplength;
                }

                if (Length > 0) {

                    if (small == TRUE) {
                        ValueData = (PVOID)(&(ValueBody->Data));
                    } else {
                        ValueData = (PVOID)HvGetCell(SystemHive, ValueBody->Data);
                    }

                    ASSERT((small ? (Length <= CM_KEY_VALUE_SMALL) : TRUE));

                    RtlMoveMemory(
                        ControlVector->Buffer,
                        ValueData,
                        Length
                        );
                }

                if (ControlVector->Type != NULL) {
                    *(ControlVector->Type) = ValueBody->Type;
                }
            }
        }

        //
        // Stash the length of result (-1 if nothing was found)
        //
        
        if (ControlVector->BufferLength != NULL) {
            *(ControlVector->BufferLength) = Length;
        }

        ControlVector++;
    }

    //
    // Get the default locale ID for the system from the registry.
    //

    if (CmDefaultLanguageIdType == REG_SZ) {
        PsDefaultSystemLocaleId = (LCID) CmpConvertLangId( 
                                                CmDefaultLanguageId,
                                                CmDefaultLanguageIdLength);
    } else {
        PsDefaultSystemLocaleId = 0x00000409;
    }

    //
    // Get the install (native UI) language ID for the system from the registry.
    //

    if (CmInstallUILanguageIdType == REG_SZ) {
        PsInstallUILanguageId =  CmpConvertLangId( 
                                                CmInstallUILanguageId,
                                                CmInstallUILanguageIdLength);
    } else {
        PsInstallUILanguageId = LANGIDFROMLCID(PsDefaultSystemLocaleId);
    }

    //
    // Set the default thread locale to the default system locale
    // for now.  This will get changed as soon as somebody logs in.
    // Use the install (native) language id as our default UI language id. 
    // This also will get changed as soon as somebody logs in.
    //

    PsDefaultThreadLocaleId = PsDefaultSystemLocaleId;
    PsDefaultUILanguageId = PsInstallUILanguageId;
}


HCELL_INDEX
CmpWalkPath(
    PHHIVE      SystemHive,
    HCELL_INDEX ParentCell,
    PWSTR       Path
    )
/*++

Routine Description:

    Walk the path.

Arguments:

    SystemHive - hive

    ParentCell - where to start

    Path - string to walk

Return Value:

    HCELL_INDEX of found key cell, or HCELL_NIL for error

--*/
{
    NTSTATUS        status;
    UNICODE_STRING  PathString;
    UNICODE_STRING  NextName;
    BOOLEAN         Last;
    PHCELL_INDEX    Index;
    HCELL_INDEX     KeyCell;

    KeyCell = ParentCell;
    RtlInitUnicodeString(&PathString, Path);

    while (TRUE) {

        CmpGetNextName(&PathString, &NextName, &Last);

        if (NextName.Length == 0) {
            return KeyCell;
        }

        KeyCell = CmpFindSubKeyByName(SystemHive,
                                      (PCM_KEY_NODE)HvGetCell(SystemHive,KeyCell),
                                      &NextName);

        if (KeyCell == HCELL_NIL) {
            return HCELL_NIL;
        }
    }
}

LANGID
CmpConvertLangId(
    PWSTR LangIdString,
    ULONG LangIdStringLength
)
{


    USHORT i, Digit;
    WCHAR c;
    LANGID LangId;

    LangId = 0;
    LangIdStringLength = LangIdStringLength / sizeof( WCHAR );
    for (i=0; i < LangIdStringLength; i++) {
        c = LangIdString[ i ];

        if (c >= L'0' && c <= L'9') {
            Digit = c - L'0';

        } else if (c >= L'A' && c <= L'F') {
            Digit = c - L'A' + 10;

        } else if (c >= L'a' && c <= L'f') {
            Digit = c - L'a' + 10;

        } else {
            break;
        }

        if (Digit >= 16) {
            break;
        }

        LangId = (LangId << 4) | Digit;
    }

    return LangId;
}

