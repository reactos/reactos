/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    editreg.c

Abstract:

    This program acts as an interactive shell allowing a user to view
    and manipulate the configuration registry.  Also, it has some specific
    commands for support of the NTFT component of the registry.

Author:

    Mike Glass
    Bob Rinne

Environment:

    User process.

Notes:

    The commands "disk", "fix", "restore" are commands that know where
    the configuration information is for the NTFT component of the NT
    system.

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ntdskreg.h"
#include "ntddft.h"

//
// Tempory stuff to get types and values to print in help.
//

PUCHAR TypeNames[] =
{
    "REG_NONE",
    "REG_SZ",
    "REG_BINARY",
    "REG_DWORD",
    "REG_DWORD_LITTLE_ENDIAN",
    "REG_DWORD_BIG_ENDIAN",
    "REG_LINK",
    "REG_MULTI_SZ",
    "REG_RESOURCE_LIST",
    NULL
};

ULONG TypeNumbers[] =
{
    REG_NONE,
    REG_SZ,
    REG_BINARY,
    REG_DWORD,
    REG_DWORD_LITTLE_ENDIAN,
    REG_DWORD_BIG_ENDIAN,
    REG_LINK,
    REG_MULTI_SZ,
    REG_RESOURCE_LIST
};

//
// Special support for the driver load lists in the registry.
//

PUCHAR StartDescription[] =
{
    "Boot loader",
    "System",
    "2",
    "3",
    
    //
    // Anything above 3 is not loaded.
    //

    NULL
};

PUCHAR TypeDescription[] =
{
    "System driver",
    "File system",
    "Service",
    NULL
};


//
// Constants and defines.
//

#define WORK_BUFFER_SIZE 4096

//
// Amount to fudge when mallocing for strings.
//

#define FUDGE 8

//
// Registry base.
//

#define REGISTRY_BASE "\\REGISTRY\\MACHINE"

//
// Default type value when key value set.
//

#define DEFAULT_TYPE REG_SZ

//
// Base location for component descriptions of FT elements.
//

#define FT_REGISTRY_ROOT "\\REGISTRY\\MACHINE\\SYSTEM\\NTFT"

//
// Subkey name located in the FT_REGISTRY_ROOT for stripes.
//

#define FT_STRIPE_BASE   "Stripe%d"

//
// Subkey name located in the FT_REGISTRY_ROOT for mirrors.
//

#define FT_MIRROR_BASE   "Mirror%d"

//
// Subkey name located in the FT_REGISTRY_ROOT for volume sets.
//

#define FT_VOLSET_BASE   "VolSet%d"


//
// Constants for the command values.
//

#define INVALID   -1
#define DIR       0
#define CREATE    1
#define LIST      2
#define CHDIR     3
#define HELP      4
#define QUIT      5
#define DDEBUG    6
#define SETVALUE  7
#define DELKEY    8
#define DELVALUE  9
#define DIRLONG  10
#define INLONG   11
#define INSHORT  12
#define INBYTE   13
#define DUMP     14
#define DISKREG  15
#define FIXDISK  16
#define RESTORE  17
#define DRIVERS  18
#define ORPHAN   19
#define REGEN    20
#define INIT     21
#define MAKEFT   22

#define CTRL_C 0x03

//
// Table of recognized commands.
//

PUCHAR Commands[] = {
    "dir",
    "keys",
    "lc",
    "ls",
    "create",
    "set",
    "unset",
    "erase",
    "delete",
    "rm",
    "list",
    "values",
    "display",
    "cd",
    "chdir",
    "help",
    "?",
    "quit",
    "exit",
    "debug",
    "longs",
    "shorts",
    "bytes",
    "dump",
    "disks",
    "fix",
    "restore",
    "drivers",
    "orphan",
    "regenerate",
    "initialize",
    "makeft",
    NULL
};

//
// Using the index from the match on the commands in Commands[], this
// table gives the proper command value to be executed.  This allows
// for multiple entries in Commands[] for the same command code.
//

int CommandMap[] = {

    DIRLONG,
    DIR,
    DIR,
    DIR,
    CREATE,
    SETVALUE,
    DELVALUE,
    DELVALUE,
    DELKEY,
    DELKEY,
    LIST,
    LIST,
    LIST,
    CHDIR,
    CHDIR,
    HELP,
    HELP,
    QUIT,
    QUIT,
    DDEBUG,
    INLONG,
    INSHORT,
    INBYTE,
    DUMP,
    DISKREG,
    FIXDISK,
    RESTORE,
    DRIVERS,
    ORPHAN,
    REGEN,
    INIT,
    MAKEFT
};

//
// CommandHelp is an array of help strings for each of the commands.
// The array is indexed by the result of CommandMap[i] for the Commands[]
// array.  This way the same help message will print for each of the
// commands aliases.
//

PUCHAR   CommandHelp[] = {

    "Displays keys.",
    "Create a new key.",
    "Displays values withing a key.",
    "Change current location in registry.",
    "This help information.",
    "Exit the program.",
    "Set internal debug on for this program.",
    "Set a new value within a key.",
    "Delete a key.",
    "Unset (erase) a key value.",
    "Unset (erase) a key value.",
    "Change dump format to Longs (default).",
    "Change dump format to Shorts.",
    "Change dump format to Bytes.",
    "Toggle dump mode (force hex dump for all value types).",
    "Display the disk registry.",
    "Set disk signatures in registry.",
    "Restore an FT orphan to working state.",
    "List the information on the drivers from the registry.",
    "Orphan a member of an FT set.",
    "Mark a FT set member for regeneration on next boot.",
    "Mark a stripe with parity for initialization on next boot.",
    "Construct an FT set from existing partitions",
    NULL

};

//
// Space for working location string in registry.
//

UCHAR WorkingDirectory[512];

//
// Space for current location string in registry.
//

UCHAR CurrentDirectory[512];

//
// Space for command input.
//

UCHAR CommandLine[512];

//
// Prompt strings for getting definition for an FT_COPY request.
//

PUCHAR SetPrompts[] = {

    "Name => ",
    "Value => ",
    "Index => ",
    NULL
};

//
// Version indicator.  Should be changed every time a major edit occurs.
//

PUCHAR Version = "Version 1.30";

//
// Debug print level.
//

ULONG Debug = 0;

//
// Dump control values.
//

typedef enum _DUMP_CONTROL {

    InBytes,
    InShorts,
    InLongs

} DUMP_CONTROL, *PDUMP_CONTROL;

ULONG ForceDump = 0;

DUMP_CONTROL DumpControl = InLongs;

NTSTATUS
FtOpenKey(
    PHANDLE HandlePtr,
    PUCHAR  KeyName
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS          status;
    STRING            keyString;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING    unicodeKeyName;

    RtlInitString(&keyString,
                  KeyName);

    (VOID)RtlAnsiStringToUnicodeString(&unicodeKeyName,
                                       &keyString,
                                       (BOOLEAN) TRUE);

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenKey(HandlePtr,
                       MAXIMUM_ALLOWED,
                       &objectAttributes);

    RtlFreeUnicodeString(&unicodeKeyName);

    if (Debug == 1) {
        if (!NT_SUCCESS(status)) {
            printf("Failed NtOpenKey for %s => %x\n",
                   KeyName,
                   status);
        }
    }

    return status;
}


NTSTATUS
FtDeleteKey(
    PUCHAR KeyName
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS status;
    HANDLE   keyToDelete;

    status = FtOpenKey(&keyToDelete,
                       KeyName);

    if (!NT_SUCCESS(status)) {
        printf("Key %s not found (0x%x).\n", KeyName, status);
        return status;
    }

    status = NtDeleteKey(keyToDelete);

    if (Debug == 1) {
        if (!NT_SUCCESS(status)) {
            printf("Could not delete key %s => %x\n",
                   KeyName,
                   status);
        }
    }

    NtClose(keyToDelete);
    return status;
}


NTSTATUS
FtCreateKey(
    PUCHAR KeyName,
    PUCHAR KeyClass,
    ULONG  Index
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS          status;
    STRING            keyString;
    UNICODE_STRING    unicodeKeyName;
    STRING            classString;
    UNICODE_STRING    unicodeClassName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG             disposition;
    HANDLE            tempHandle;

#if DBG
    if ((KeyName == NULL) ||
        (KeyClass == NULL)) {
        printf("FtCreateKey: Invalid parameter 0x%x, 0x%x\n",
               KeyName,
               KeyClass);
        ASSERT(0);
    }
#endif

    //
    // Initialize the object for the key.
    //

    RtlInitString(&keyString,
                  KeyName);

    (VOID)RtlAnsiStringToUnicodeString(&unicodeKeyName,
                                       &keyString,
                                       (BOOLEAN) TRUE);

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Setup the unicode class value.
    //

    RtlInitString(&classString,
                  KeyClass);
    (VOID)RtlAnsiStringToUnicodeString(&unicodeClassName,
                                       &classString,
                                       (BOOLEAN) TRUE);

    //
    // Create the key.
    //

    status = NtCreateKey(&tempHandle,
                         MAXIMUM_ALLOWED,
                         &objectAttributes,
                         Index,
                         &unicodeClassName,
                         REG_OPTION_NON_VOLATILE,
                         &disposition);

    if (NT_SUCCESS(status)) {
        switch (disposition)
        {
        case REG_CREATED_NEW_KEY:
            break;

        case REG_OPENED_EXISTING_KEY:
            printf("Warning: Creation was for an existing key!\n");
            break;

        default:
            printf("New disposition returned == 0x%x\n", disposition);
            break;
        }
    }

    //
    // Free all allocated space.
    //

    RtlFreeUnicodeString(&unicodeKeyName);
    RtlFreeUnicodeString(&unicodeClassName);
    NtClose(tempHandle);
    return status;
}


NTSTATUS
FtDeleteValue(
    HANDLE KeyHandle,
    PUCHAR ValueName
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS       status;
    STRING         valueString;
    UNICODE_STRING unicodeValueName;

    RtlInitString(&valueString,
                  ValueName);
    status = RtlAnsiStringToUnicodeString(&unicodeValueName,
                                          &valueString,
                                          (BOOLEAN) TRUE);
    if (!NT_SUCCESS(status)) {
        printf("FtDeleteValue: internal conversion error 0x%x\n", status);
        return status;
    }

    status = NtDeleteValueKey(KeyHandle,
                              &unicodeValueName);
    if (Debug == 1) {
        if (!NT_SUCCESS(status)) {
            printf("Could not delete value %s => %x\n",
                   ValueName,
                   status);
        }
    }

    RtlFreeUnicodeString(&unicodeValueName);
    return status;
}

NTSTATUS
FtSetValue(
    HANDLE KeyHandle,
    PUCHAR ValueName,
    PVOID  DataBuffer,
    ULONG  DataLength,
    ULONG  Type
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS          status;
    STRING            valueString;
    UNICODE_STRING    unicodeValueName;

    RtlInitString(&valueString,
                  ValueName);
    RtlAnsiStringToUnicodeString(&unicodeValueName,
                                 &valueString,
                                 (BOOLEAN) TRUE);
    status = NtSetValueKey(KeyHandle,
                           &unicodeValueName,
                           0,
                           Type,
                           DataBuffer,
                           DataLength);
    if (Debug == 1) {
        if (!NT_SUCCESS(status)) {
            printf("Could not set value %s => %x\n",
                   ValueName,
                   status);
        }
    }

    RtlFreeUnicodeString(&unicodeValueName);
    return status;
}


PUCHAR
FindTypeString(
    ULONG Type
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    int i;

    for (i = 0; TypeNames[i] != NULL; i++) {

        if (TypeNumbers[i] == Type) {
            return TypeNames[i];
        }
    }
    return "(Unknown)";
}


BOOLEAN
ProcessHex(
    PUCHAR String,
    PULONG Value
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    ULONG  workValue;
    int    i;
    PUCHAR cp;

    if (String == NULL) {
        return FALSE;
    }

    cp = String;

    //
    // 'i' is an index value.  It contains the maximum index into the String.
    // Therefore it is initialized to -1.
    //

    i = -1;
    while ((*cp) && (*cp != '\n')) {
        i++;
        cp++;
    }

    if (i >= 8) {

        //
        // String to long for a long.
        //

        return FALSE;
    }

    workValue = 0;
    cp = String;
    while (*cp) {
        *cp = (UCHAR) tolower(*cp);

        switch (*cp) {

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            workValue |= (((*cp) - '0') << (i * 4));
            break;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            workValue |= ((((*cp) - 'a') + 10) << (i * 4));
            break;

        default:

            //
            // Illegal value, just punt.
            //

            return FALSE;
            break;
        }
        cp++;
        i--;
    }

    *Value = workValue;
    return TRUE;
}


VOID
Dump(
    PVOID Buffer,
    ULONG Length
    )

/*++

Routine Description:

    Dump the value data from a buffer in the format specified.

Arguments:

    Buffer - pointer to the data.
    Length - length of the data.

Return Value:

    None.

--*/
{
    PUCHAR location;
    PUCHAR internalBuffer;
    int    i;
    int    j;
    int    numberLines;
    UCHAR  outHexLine[128];
    UCHAR  outPrintable[64];

    numberLines = (Length + 15) / 16;

    //
    // Since the amount of data displayed has been rounded up, this
    // routine mallocs enough space and copies the data in.  This way
    // it won't fault if the data is at the end of memory.
    //

    internalBuffer = (PUCHAR) malloc(numberLines * 16);
    RtlMoveMemory(internalBuffer, Buffer, Length);
    location = (PUCHAR) internalBuffer;

    for (i = 0; i < numberLines; i++) {

        sprintf(outHexLine, "%8x: ", (i * 16));
        sprintf(outPrintable, "*");
        switch (DumpControl) {

        case InBytes:

            for (j = 0; j < 16; j++) {
                sprintf(outHexLine, "%s%2X ", outHexLine, *location);
                sprintf(outPrintable, "%s%c", outPrintable,
                        (isprint(location[0])) ? location[0] : '.');
                location++;
            }
            break;

        case InShorts:

            for (j = 0; j < 8; j++) {
                sprintf(outHexLine, "%s%4X ", outHexLine,
                        *((PUSHORT)location));
                sprintf(outPrintable, "%s%c%c", outPrintable,
                        (isprint(location[0])) ? location[0] : '.',
                        (isprint(location[1])) ? location[1] : '.');
                location += 2;
            }
            break;

        default:
        case InLongs:

            for (j = 0; j < 4; j++) {
                sprintf(outHexLine, "%s%8X ", outHexLine,
                        *((PULONG)location));
                sprintf(outPrintable, "%s%c%c%c%c", outPrintable,
                        (isprint(location[0])) ? location[0] : '.',
                        (isprint(location[1])) ? location[1] : '.',
                        (isprint(location[2])) ? location[2] : '.',
                        (isprint(location[3])) ? location[3] : '.');
                location += 4;
            }
            break;
        }

        printf("%s   %s*\n", outHexLine, outPrintable);
    }
    printf("\n");
    free(internalBuffer);
}


void
UnicodePrint(
    PUNICODE_STRING  UnicodeString
    )

/*++

Routine Description:

    Print a unicode string.

Arguments:

    UnicodeString - pointer to the string.

Return Value:

    None.

--*/
{
    ANSI_STRING ansiString;
    PUCHAR      tempbuffer = (PUCHAR) malloc(WORK_BUFFER_SIZE);

    ansiString.MaximumLength = WORK_BUFFER_SIZE;
    ansiString.Length = 0L;
    ansiString.Buffer = tempbuffer;

    RtlUnicodeStringToAnsiString(&ansiString,
                                 UnicodeString,
                                 (BOOLEAN) FALSE);
    printf("%s", ansiString.Buffer);
    free(tempbuffer);
    return;
}


NTSTATUS
Directory(
    HANDLE  KeyHandle,
    BOOLEAN LongListing
    )

/*++


Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS        status;
    ULONG           index;
    ULONG           resultLength;
    UNICODE_STRING  unicodeValueName;
    PKEY_BASIC_INFORMATION keyInformation;

    keyInformation = (PKEY_BASIC_INFORMATION) malloc(WORK_BUFFER_SIZE);

    for (index = 0; TRUE; index++) {

        RtlZeroMemory(keyInformation, WORK_BUFFER_SIZE);

        status = NtEnumerateKey(KeyHandle,
                                index,
                                KeyBasicInformation,
                                keyInformation,
                                WORK_BUFFER_SIZE,
                                &resultLength);

        if (status == STATUS_NO_MORE_ENTRIES) {

            break;

        } else if (!NT_SUCCESS(status)) {

            printf("readreg: Error on Enumerate status = %x\n", status);
            break;

        }

        unicodeValueName.Length = (USHORT)keyInformation->NameLength;
        unicodeValueName.MaximumLength = (USHORT)keyInformation->NameLength;
        unicodeValueName.Buffer = (PWSTR)&keyInformation->Name[0];
        UnicodePrint(&unicodeValueName);
        printf("\n");

        if (LongListing) {
        }
    }

    free(keyInformation);
    return status;
}


NTSTATUS
List(
    HANDLE KeyHandle,
    PUCHAR ItemName
    )

/*++


Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS       status;
    ULONG          index;
    ULONG          resultLength;
    ULONG          type;
    PUCHAR         typeString;
    UNICODE_STRING unicodeValueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    UNREFERENCED_PARAMETER(ItemName);

    resultLength = WORK_BUFFER_SIZE;
    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)malloc(WORK_BUFFER_SIZE);

    for (index = 0; TRUE; index++) {

        while (1) {

            RtlZeroMemory(keyValueInformation, resultLength);
            status = NtEnumerateValueKey(KeyHandle,
                                         index,
                                         KeyValueFullInformation,
                                         keyValueInformation,
                                         resultLength,
                                         &resultLength);

            if (status == STATUS_BUFFER_OVERFLOW) {
                free(keyValueInformation);
                keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                           malloc(resultLength + 10);
            } else {
                break;
            }
        }

        if (status == STATUS_NO_MORE_ENTRIES) {

            break;

        } else if (!NT_SUCCESS(status)) {

            printf("readreg: Cannot list (%x)\n", status);
            break;

        }

        type = keyValueInformation->Type;
        typeString = FindTypeString(type);
        unicodeValueName.Length = (USHORT)keyValueInformation->NameLength;
        unicodeValueName.MaximumLength =(USHORT)keyValueInformation->NameLength;
        unicodeValueName.Buffer = (PWSTR)&keyValueInformation->Name[0];
        printf("Name-> """);
        UnicodePrint(&unicodeValueName);
        printf("""\n");
        printf("\ttype = %s (%d)\ttitle index = %d\tdata length = %d\n",
               typeString,
               type,
               keyValueInformation->TitleIndex,
               keyValueInformation->DataLength);
        printf("\tData:\n");

        if (ForceDump) {
            type = REG_BINARY;
        }

        switch (type) {

        case REG_DWORD:
        // case REG_DWORD_LITTLE_ENDIAN:
            printf("\tDWORD value == %d, (0x%x)\n",
               *((PULONG)((PUCHAR)keyValueInformation +
                                  keyValueInformation->DataOffset)),
               *((PULONG)((PUCHAR)keyValueInformation +
                                  keyValueInformation->DataOffset)));
            break;

        case REG_SZ:

            unicodeValueName.Length = (USHORT)keyValueInformation->DataLength;
            unicodeValueName.MaximumLength = (USHORT)
                                                keyValueInformation->DataLength;
            unicodeValueName.Buffer = (PWSTR) ((PUCHAR) keyValueInformation +
                                               keyValueInformation->DataOffset);
            UnicodePrint(&unicodeValueName);
            break;

        case REG_BINARY:
        default:
            Dump(((PUCHAR)keyValueInformation +keyValueInformation->DataOffset),
                 keyValueInformation->DataLength);
        }
        printf("\n");
    }

    free(keyValueInformation);
    return status;
}


UCHAR
GetCharacter(
    BOOLEAN Batch
    )

/*++

Routine Description:

    This routine returns a single character from the input stream.
    It discards leading blanks if the input is not from the console.

Arguments:

    Batch - a boolean indicating if the input it coming from the console.

Return Value:

    A character

--*/

{
    UCHAR c;

    if (Batch) {

        while ((c = (UCHAR) getchar()) == ' ')
            ;

    } else {

        c = (UCHAR) getchar();
    }

    return c;
} // GetCharacter


PUCHAR
GetArgumentString(
    BOOLEAN Batch,
    PUCHAR  Prompt,
    BOOLEAN ConvertToLower
    )

/*++

Routine Description:

    This routine prints the prompt if the input is coming from the console,
    then proceeds to collect the user input until a carraige return is typed.

Arguments:

    Batch  - a boolean indicating if the input is coming from the console.
    Prompt - String to prompt with.

Return Value:

    A pointer to the input string.
    NULL if the user escaped.

--*/

{
    //
    // The command line data area is used to store the argument string.
    //

    PUCHAR argument = CommandLine;
    int    i;
    UCHAR  c;

    if (!Batch) {

        printf("%s", Prompt);
    }

    while ((c = GetCharacter(Batch)) == ' ') {

        //
        // Ignore leading spaces.
        //
    }

    i = 0;
    while (c) {

        putchar(c);

        if (c == CTRL_C) {

            return NULL;
        }

        if ((c == '\n') || (c == '\r')) {

            putchar('\n');

            if (i == 0) {
                return NULL;
            } else {
                break;
            }
        }

        if (c == '\b') {

            if (i > 0) {

                //
                // blank over last char
                //

                putchar(' ');
                putchar('\b');
                i--;

            } else {

                //
                // space forward to keep prompt in the same place.
                //

                putchar(' ');
            }

        } else {

            //
            // Collect the argument.
            //

            if (ConvertToLower == TRUE) {
                argument[i] = (UCHAR) tolower(c);
            } else {
                argument[i] = (UCHAR) c;
            }
            i++;

        }

        c = GetCharacter(Batch);
    }

    argument[i] = '\0';
    return CommandLine;

} // GetArgumentString


ULONG
ParseArgumentNumeric(
    PUCHAR  *ArgumentPtr
    )

/*++

Routine Description:

    This routine prints the prompt if the input is coming from the console.

Arguments:

    Batch - a boolean indicating if the input is coming from the console.

Return Value:

    None

--*/

{
    UCHAR   c;
    ULONG   number;
    int     i;
    BOOLEAN complete = FALSE;
    PUCHAR  argument = *ArgumentPtr;

    while (*argument == ' ') {

        //
        // skip spaces.
        //

        argument++;
    }

    //
    // Assume there is only one option to parse until proven
    // otherwise.
    //

    *ArgumentPtr = NULL;

    i = 0;

    while (complete == FALSE) {

        c = argument[i];

        switch (c) {

        case '\n':
        case '\r':
        case '\t':
        case ' ':

            //
            // Update the caller argument pointer to the remaining string.
            //

            *ArgumentPtr = &argument[i + 1];

            //
            // fall through.
            //

        case '\0':

            argument[i] = '\0';
            complete = TRUE;
            break;

        default:

            i++;
            break;
        }

    }

    if (i > 0) {
        number = (ULONG) atoi(argument);
    } else {
        number = (ULONG) -1;
    }

    return number;

} // ParseArgumentNumeric


VOID
PromptUser(
    BOOLEAN Batch
    )

/*++

Routine Description:

    This routine prints the prompt if the input is coming from the console.

Arguments:

    Batch - a boolean indicating if the input is coming from the console.

Return Value:

    None

--*/

{
    if (!Batch) {

        printf("\n%s> ", CurrentDirectory);
    }

} // PromptUser


int
GetCommand(
    BOOLEAN Batch,
    PUCHAR *ArgumentPtr
    )
/*++

Routine Description:

    This routine processes the user input and returns the code for the
    command entered.  If the command has an argument, either the default
    value for the argument (if none is given) or the value provided by the
    user is returned.

Arguments:

    Batch - a boolean indicating if the input it coming from the console.

Return Value:

    A command code

--*/

{
    int    i;
    int    commandIndex;
    int    commandCode;
    UCHAR  c;
    PUCHAR commandPtr;
    PUCHAR command = CommandLine;
    int    argumentIndex = -1;
    PUCHAR argument = NULL;

    PromptUser(Batch);

    while ((c = GetCharacter(Batch)) == ' ') {

        //
        // Ignore leading spaces.
        //
    }

    i = 0;
    while (c) {

        putchar(c);

        if ((c == '\n') || (c == '\r')) {
            putchar('\n');
            if (i == 0) {
                PromptUser(Batch);
                c = GetCharacter(Batch);
                continue;
            }
            break;
        }

        if (c == '\b') {

            if (i > 0) {

                //
                // blank over last char
                //

                putchar(' ');
                putchar('\b');
                i--;

                if (argumentIndex == i) {
                    argumentIndex = -1;
                    argument = NULL;
                }
            } else {

                //
                // space forward to keep prompt in the same place.
                //

                putchar(' ');
            }
        } else {

            //
            // Collect the command.
            //

            command[i] = (UCHAR)tolower(c);
            i++;
        }

        if ((c == ' ') && (argument == NULL)) {

            argument = &command[i];
            argumentIndex = i;
            command[i - 1] = '\0';
        }

        c = GetCharacter(Batch);
    }

    //
    // add end of string.
    //

    command[i] = '\0';

    if (Debug) {
        printf("command => %s$\n", command);
    }

    //
    // Identify the command and return its code.
    //

    commandIndex = 0;

    for (commandPtr = Commands[commandIndex];
         commandPtr != NULL;
         commandPtr = Commands[commandIndex]) {

        if (Debug) {
            printf("Testing => %s$ ... ", commandPtr);
        }

        i = 0;
        while (commandPtr[i] == command[i]) {
            if (command[i] == '\0') {
                break;
            }
            i++;
        }

        if (Debug) {
            printf(" i == %d, command[i] == 0x%x\n", i, command[i]);
        }

        if (command[i]) {

            //
            // Not complete there was a mismatch on the command.
            //

            commandIndex++;
            continue;
        }

        //
        // Have a match on the command.
        //

        if (Debug) {
            printf("Command match %d, argument %s\n",
                   commandIndex,
                   (argument == NULL) ? "(none)" : argument);
        }

        commandCode = CommandMap[commandIndex];
        *ArgumentPtr = argument;
        return commandCode;
    }

    printf("Command was invalid\n");
    return INVALID;
} // GetCommand


VOID
NotImplemented()

/*++

--*/

{
    printf("Sorry, function not implemented yet.\n");
}

NTSTATUS
FtReturnValue(
    IN HANDLE Handle,
    IN PUCHAR ValueName,
    IN PUCHAR Buffer,
    IN ULONG  BufferLength
    )

/*++

Routine Description:

    Formatted display of the disk registry information.

Arguments:

    None.

Return Values:

    None.

--*/

{
    NTSTATUS       status;
    ULONG          resultLength;
    ULONG          length;
    STRING         valueString;
    UNICODE_STRING unicodeValueName;
    PUCHAR         internalBuffer;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)malloc(WORK_BUFFER_SIZE);
    if (keyValueInformation == NULL) {
        printf("FtReturnValue: cannot allocate memory.\n");
        return STATUS_NO_MEMORY;
    }

    RtlInitString(&valueString,
                  ValueName);
    RtlAnsiStringToUnicodeString(&unicodeValueName,
                                 &valueString,
                                 (BOOLEAN) TRUE);
    status = NtQueryValueKey(Handle,
                             &unicodeValueName,
                             KeyValueFullInformation,
                             keyValueInformation,
                             WORK_BUFFER_SIZE,
                             &resultLength);
    RtlFreeUnicodeString(&unicodeValueName);

    if (NT_SUCCESS(status)) {
        length = (resultLength > BufferLength) ? BufferLength : resultLength;
        internalBuffer =
           ((PUCHAR)keyValueInformation + keyValueInformation->DataOffset);

        RtlMoveMemory(Buffer, internalBuffer, length);
    }
    free(keyValueInformation);
    return status;
}

VOID
DiskDump()

/*++

Routine Description:

    Formatted display of the disk registry information.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ULONG               outerLoop;
    ULONG               innerLoop;
    HANDLE              handle;
    NTSTATUS            status;
    PDISK_CONFIG_HEADER configHeader;
    PDISK_REGISTRY      diskRegistry;
    PDISK_DESCRIPTION   diskDescription;
    PDISK_PARTITION     diskPartition;
    PFT_REGISTRY        ftRegistry;
    PFT_DESCRIPTION     ftDescription;
    PFT_MEMBER_DESCRIPTION ftMember;

    status = FtOpenKey(&handle,
                       DISK_REGISTRY_KEY);

    if (!NT_SUCCESS(status)) {
        printf("Currently there is no key in the registry"
               " for the disk information.\n");
        return;
    }

    configHeader = (PDISK_CONFIG_HEADER) malloc(WORK_BUFFER_SIZE);
    if (configHeader == NULL) {
        printf("Unable to allocate memory for the disk registy information.\n");
        return;
    }

    RtlZeroMemory(configHeader, WORK_BUFFER_SIZE);

    status = FtReturnValue(handle,
                           (PUCHAR) DISK_REGISTRY_VALUE,
                           (PUCHAR) configHeader,
                           WORK_BUFFER_SIZE);
    NtClose(handle);

    if (!NT_SUCCESS(status)) {
        printf("There is no disk registry information (%x)\n", status);
        free(configHeader);
        return;
    }

    //
    // Print the header.
    //

    printf("Registry header information:\n");
    printf("\tVersion = 0x%x, Checksum = 0x%x\n",
           configHeader->Version,
           configHeader->CheckSum);
    printf("\tDisk info Offset = 0x%x, Size = 0x%x\n",
           configHeader->DiskInformationOffset,
           configHeader->DiskInformationSize);
    printf("\tFT info Offset = 0x%x, Size = 0x%x\n",
           configHeader->FtInformationOffset,
           configHeader->FtInformationSize);

    //
    // Print the information on disks.
    //

    diskRegistry = (PDISK_REGISTRY)
                 ((PUCHAR) configHeader + configHeader->DiskInformationOffset);
    printf("\nDisk information for %d disks:\n",
           diskRegistry->NumberOfDisks);

    diskDescription = &diskRegistry->Disks[0];
    for (outerLoop = 0;
         outerLoop < diskRegistry->NumberOfDisks;
         outerLoop++) {

        printf("\nDisk %d signature 0x%08x has %d partitions:\n",
               outerLoop,
               diskDescription->Signature,
               diskDescription->NumberOfPartitions);

        printf("       Ln Type  Start              Length             FtGrp  Member\n");
        for (innerLoop = 0;
             innerLoop < diskDescription->NumberOfPartitions;
             innerLoop++) {
            diskPartition = &diskDescription->Partitions[innerLoop];
            printf("  %c: %c %1d   %3d  %08x:%08x  %08x:%08x  %5d  %4d  %s\n",
                   (diskPartition->DriveLetter != '\0') ?
                                               diskPartition->DriveLetter : ' ',
                   (diskPartition->AssignDriveLetter) ? 'A' : ' ',
                   diskPartition->LogicalNumber,
                   diskPartition->FtType,
                   diskPartition->StartingOffset.HighPart,
                   diskPartition->StartingOffset.LowPart,
                   diskPartition->Length.HighPart,
                   diskPartition->Length.LowPart,
                   diskPartition->FtGroup,
                   diskPartition->FtMember,
                   (diskPartition->FtState == Orphaned) ? "Orphan" :
                     (diskPartition->FtState == Regenerating) ? "Regen" :
                     (diskPartition->FtState == Initializing) ? "Init" : "");

        }

        diskDescription = (PDISK_DESCRIPTION)
          &diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    //
    // Print the information for FT.
    //

    if (configHeader->FtInformationSize == 0) {
        printf("There is no FT configuration.\n");
        free(configHeader);
        return;
    }

    ftRegistry = (PFT_REGISTRY)
                 ((PUCHAR) configHeader + configHeader->FtInformationOffset);

    printf("\nNumber of FT components = %d\n",
           ftRegistry->NumberOfComponents);

    ftDescription = &ftRegistry->FtDescription[0];
    for (outerLoop = 0;
         outerLoop < ftRegistry->NumberOfComponents;
         outerLoop++) {

        printf("Component %d has %d members and is type %d\n",
               outerLoop,
               ftDescription->NumberOfMembers,
               ftDescription->Type);

        printf("      State Signature Start              Length            #\n");
        for (innerLoop = 0;
             innerLoop < ftDescription->NumberOfMembers;
             innerLoop++) {
            ftMember = &ftDescription->FtMemberDescription[innerLoop];

            diskPartition = (PDISK_PARTITION)
                 ((PUCHAR) configHeader + ftMember->OffsetToPartitionInfo);
            
            printf("%5x    %2x %08x  %08x:%08x  %08x:%08x %d\n",
                   ftMember->OffsetToPartitionInfo,
                   ftMember->State,
                   ftMember->Signature,
                   diskPartition->StartingOffset.HighPart,
                   diskPartition->StartingOffset.LowPart,
                   diskPartition->Length.HighPart,
                   diskPartition->Length.LowPart,
                   ftMember->LogicalNumber);
        }

        ftDescription = (PFT_DESCRIPTION)
         &ftDescription->FtMemberDescription[ftDescription->NumberOfMembers];
    }
}


VOID
ChangeMemberState(
    IN ULONG Type,
    IN ULONG Group,
    IN ULONG Member,
    IN FT_PARTITION_STATE NewState
    )

/*++

Routine Description:

    Set the FT state for a partition.

Arguments:

    Type   - the FT type.
    Group  - the FT Group number for that type.
    Member - the member number within the group.

Return Values:

    None.

--*/

{
    BOOLEAN             writeBackRegistry = FALSE;
    HANDLE              handle;
    ULONG               outerLoop;
    ULONG               innerLoop;
    NTSTATUS            status;
    PDISK_CONFIG_HEADER configHeader;
    PDISK_REGISTRY      diskRegistry;
    PDISK_DESCRIPTION   diskDescription;
    PDISK_PARTITION     partitionDescription;

    status = FtOpenKey(&handle,
                       DISK_REGISTRY_KEY);

    if (!NT_SUCCESS(status)) {
        printf("Currently there is no key in the registry"
               " for the disk information.\n");
        return;
    }

    configHeader = (PDISK_CONFIG_HEADER) malloc(WORK_BUFFER_SIZE);
    if (configHeader == NULL) {
        printf("Unable to allocate memory for the disk registy information.\n");
        NtClose(handle);
        return;
    }

    RtlZeroMemory(configHeader, WORK_BUFFER_SIZE);

    status = FtReturnValue(handle,
                           (PUCHAR) DISK_REGISTRY_VALUE,
                           (PUCHAR) configHeader,
                           WORK_BUFFER_SIZE);

    if (!NT_SUCCESS(status)) {
        printf("There is no disk registry information (%x)\n", status);
        free(configHeader);
        NtClose(handle);
        return;
    }

    diskRegistry = (PDISK_REGISTRY)
                 ((PUCHAR) configHeader + configHeader->DiskInformationOffset);

    diskDescription = &diskRegistry->Disks[0];
    for (outerLoop = 0;
         outerLoop < diskRegistry->NumberOfDisks;
         outerLoop++) {

        for (innerLoop = 0;
             innerLoop < diskDescription->NumberOfPartitions;
             innerLoop++) {

            partitionDescription = &diskDescription->Partitions[innerLoop];

            if ((partitionDescription->FtType == (FT_TYPE) Type) &&
                (partitionDescription->FtGroup == (USHORT) Group) &&
                (partitionDescription->FtMember == (USHORT) Member)) {

                partitionDescription->FtState = NewState;
                writeBackRegistry = TRUE;
                break;
            }
        }

        if (writeBackRegistry == TRUE) {
            ULONG size;

            if (configHeader->FtInformationSize == 0) {
                printf("Seems a little odd to be setting FT state " // no comma
                       "with no FT information...\n");
                size = configHeader->DiskInformationOffset +
                       configHeader->DiskInformationSize;
            } else {
                size = configHeader->FtInformationOffset +
                       configHeader->FtInformationSize;
            }

            (VOID) FtSetValue(handle,
                              (PUCHAR) DISK_REGISTRY_VALUE,
                              (PUCHAR) configHeader,
                              size,
                              REG_BINARY);
            break;
        }
        diskDescription = (PDISK_DESCRIPTION)
              &diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    NtClose(handle);
    free(configHeader);
}


VOID
RestoreOrphan(
    IN ULONG Type,
    IN ULONG Group,
    IN ULONG Member
    )

/*++

Routine Description:

    Set the FT state for a partition back to Healthy.

Arguments:

    Type   - the FT type.
    Group  - the FT Group number for that type.
    Member - the member number within the group.

Return Values:

    None.

--*/

{
    ChangeMemberState(Type,
                      Group,
                      Member,
                      Healthy);
}


VOID
OrphanMember(
    IN ULONG Type,
    IN ULONG Group,
    IN ULONG Member
    )

/*++

Routine Description:

    Set the FT state for a partition to Orphaned.

Arguments:

    Type   - the FT type.
    Group  - the FT Group number for that type.
    Member - the member number within the group.

Return Values:

    None.

--*/

{
    ChangeMemberState(Type,
                      Group,
                      Member,
                      Orphaned);
}


VOID
RegenerateMember(
    IN ULONG Type,
    IN ULONG Group,
    IN ULONG Member
    )

/*++

Routine Description:

    Set the FT state for a partition to regenerate.

Arguments:

    Type   - the FT type.
    Group  - the FT Group number for that type.
    Member - the member number within the group.

Return Values:

    None.

--*/

{
    ChangeMemberState(Type,
                      Group,
                      Member,
                      Regenerating);
}


VOID
FixDisk()

/*++

Routine Description:

    Fix the disk signatures in the registry.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ULONG               outerLoop;
    ULONG               innerLoop;
    ULONG               length;
    HANDLE              handle;
    NTSTATUS            status;
    PDISK_CONFIG_HEADER configHeader;
    PDISK_REGISTRY      diskRegistry;
    PDISK_DESCRIPTION   diskDescription;
    PFT_REGISTRY        ftRegistry;
    PFT_DESCRIPTION     ftDescription;
    PFT_MEMBER_DESCRIPTION ftMember;
    UCHAR               prompt[128];
    PUCHAR              hexString;
    BOOLEAN             changed = FALSE;

    status = FtOpenKey(&handle,
                       DISK_REGISTRY_KEY);

    if (!NT_SUCCESS(status)) {
        printf("Currently there is no key in the registry"
               " for the disk information.\n");
        return;
    }

    configHeader = (PDISK_CONFIG_HEADER) malloc(WORK_BUFFER_SIZE);
    if (configHeader == NULL) {
        printf("Unable to allocate memory for the disk registy information.\n");
        NtClose(handle);
        return;
    }

    RtlZeroMemory(configHeader, WORK_BUFFER_SIZE);

    status = FtReturnValue(handle,
                           (PUCHAR) DISK_REGISTRY_VALUE,
                           (PUCHAR) configHeader,
                           WORK_BUFFER_SIZE);

    if (!NT_SUCCESS(status)) {
        printf("There is no disk registry information (%x)\n", status);
        free(configHeader);
        NtClose(handle);
        return;
    }

    diskRegistry = (PDISK_REGISTRY)
                 ((PUCHAR) configHeader + configHeader->DiskInformationOffset);
    printf("\nDisk information for %d disks:\n",
           diskRegistry->NumberOfDisks);

    diskDescription = &diskRegistry->Disks[0];
    for (outerLoop = 0;
         outerLoop < diskRegistry->NumberOfDisks;
         outerLoop++) {

        sprintf(prompt,
               "\nDisk %d signature 0x%08x = ",
               outerLoop,
               diskDescription->Signature);

        hexString = GetArgumentString((BOOLEAN) FALSE,
                                      prompt,
                                      (BOOLEAN) TRUE);

        if (hexString != NULL) {

            changed = ProcessHex(hexString, &diskDescription->Signature);
        }

        diskDescription = (PDISK_DESCRIPTION)
          &diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    //
    // Print the information for FT.
    //

    if (configHeader->FtInformationSize == 0) {
        printf("There is no FT configuration.\n");
        free(configHeader);
        NtClose(handle);
        return;
    }

    ftRegistry = (PFT_REGISTRY)
                 ((PUCHAR) configHeader + configHeader->FtInformationOffset);

    printf("\nNumber of FT components = %d\n",
           ftRegistry->NumberOfComponents);

    ftDescription = &ftRegistry->FtDescription[0];
    for (outerLoop = 0;
         outerLoop < ftRegistry->NumberOfComponents;
         outerLoop++) {

        printf("Component %d has %d members and is type %d\n",
               outerLoop,
               ftDescription->NumberOfMembers,
               ftDescription->Type);

        for (innerLoop = 0;
             innerLoop < ftDescription->NumberOfMembers;
             innerLoop++) {
            ftMember = &ftDescription->FtMemberDescription[innerLoop];

            sprintf(prompt,
                    "FT Member Signature 0x%x = ",
                    ftMember->Signature);

            hexString = GetArgumentString((BOOLEAN) FALSE,
                                          prompt,
                                          (BOOLEAN) TRUE);

            if (hexString != NULL) {

                changed = ProcessHex(hexString, &ftMember->Signature);
            }
        }

        ftDescription = (PFT_DESCRIPTION)
         &ftDescription->FtMemberDescription[ftDescription->NumberOfMembers];
    }

    if (changed == TRUE) {

        printf("Attempting to update registry information.\n");

        //
        // Delete the current registry value and write the new one.
        //

        status = FtDeleteValue(handle,
                               DISK_REGISTRY_VALUE);

        if (!NT_SUCCESS(status)) {
            printf("Could not delete value (0x%x).\n", status);
        } else {

            length = (ULONG) ((PCHAR)ftDescription - (PUCHAR)configHeader);
            status = FtSetValue(handle,
                                DISK_REGISTRY_VALUE,
                                configHeader,
                                length,
                                REG_BINARY);
            if (!NT_SUCCESS(status)) {
                printf("Could not write value (0x%x)\n.", status);
            }
        }
    }

    NtClose(handle);
}

PDISK_CONFIG_HEADER
GetDiskInfo()

/*++

--*/

{
    HANDLE              handle;
    ULONG               length;
    NTSTATUS            status;
    PDISK_CONFIG_HEADER configHeader;

    status = FtOpenKey(&handle,
                       DISK_REGISTRY_KEY);

    if (!NT_SUCCESS(status)) {
        printf("Currently there is no key in the registry"
               " for the disk information.\n");
        return NULL;
    }

    configHeader = (PDISK_CONFIG_HEADER) malloc(WORK_BUFFER_SIZE);
    if (configHeader == NULL) {
        printf("Unable to allocate memory for the disk registy information.\n");
        NtClose(handle);
        return NULL;
    }

    RtlZeroMemory(configHeader, WORK_BUFFER_SIZE);

    status = FtReturnValue(handle,
                           (PUCHAR) DISK_REGISTRY_VALUE,
                           (PUCHAR) configHeader,
                           WORK_BUFFER_SIZE);
    NtClose(handle);

    if (!NT_SUCCESS(status)) {
        printf("There is no disk registry information (%x)\n", status);
        free(configHeader);
        return NULL;
    }

    return configHeader;
}


BOOLEAN
CreateFtMember(
    IN PDISK_CONFIG_HEADER ConfigHeader,
    IN ULONG Disk,
    IN ULONG Partition,
    IN ULONG Type,
    IN ULONG Group,
    IN ULONG Member
    )

/*++

--*/

{
    ULONG               innerLoop;
    ULONG               outerLoop;
    ULONG               length;
    NTSTATUS            status;
    PDISK_REGISTRY      diskRegistry;
    PDISK_DESCRIPTION   diskDescription;
    PDISK_PARTITION     diskPartition;

    diskRegistry = (PDISK_REGISTRY)
                 ((PUCHAR) ConfigHeader + ConfigHeader->DiskInformationOffset);
    diskDescription = &diskRegistry->Disks[0];

    //
    // Have to walk the disk information by hand to find a match on
    // disk number and partition
    //

    for (outerLoop = 0;
         outerLoop < diskRegistry->NumberOfDisks;
         outerLoop++) {

        if (outerLoop == Disk) {
            for (innerLoop = 0;
                 innerLoop < diskDescription->NumberOfPartitions;
                 innerLoop++) {
                diskPartition = &diskDescription->Partitions[innerLoop];
    
                if (diskPartition->LogicalNumber == Partition) {
    
                    //
                    // Found a match.
                    //

                    diskPartition->FtType = Type;
                    diskPartition->FtMember = Member;
                    diskPartition->FtGroup = Group;
                    diskPartition->FtState = Healthy;
                    diskPartition->AssignDriveLetter = FALSE;
                    return TRUE;
                }
            }
        }

        diskDescription = (PDISK_DESCRIPTION)
          &diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    //
    // Didn't find it.
    //

    return FALSE;
}


#define DRIVER_KEY "\\REGISTRY\\MACHINE\\System\\CurrentControlSet\\Services"

#define TYPE_KEY     "Type"
#define START_KEY    "Start"
#define GROUP_KEY    "Group"
#define DEPENDENCIES "DependOnGroup"

#if 0
VOID
DisplayLoadInformation(
    IN PUNICODE_STRING DriverKey
    )

/*++

Routine Description:


Arguments:

    DriverKey - a Unicode string pointer for the driver key name.

Return Value:

    None.

--*/

{
    HANDLE         keyHandle;
    UNICODE_STRING unicodeKeyName;
    UNICODE_STRING unicodeValueName;
    ULONG          resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    resultLength = WORK_BUFFER_SIZE;
    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)malloc(WORK_BUFFER_SIZE);

    //
    // Type
    //

    //
    // Start
    //

    //
    // Group
    //

    //
    // DependOnGroup
    //

    while (1) {

        RtlZeroMemory(keyValueInformation, resultLength);
        status = NtEnumerateValueKey(KeyHandle,
                                     0,
                                     KeyValueFullInformation,
                                     keyValueInformation,
                                     resultLength,
                                     &resultLength);

        if (status == STATUS_BUFFER_OVERFLOW) {
            free(keyValueInformation);
            keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                       malloc(resultLength + 10);
        } else {
            break;
        }
    }

    free(keyValueInformation);
    NtClose(keyHandle);
}
#else
VOID
DisplayLoadInformation(
    IN PUCHAR DriverKey
    )
{
    UNREFERENCED_PARAMETER(DriverKey);
}
#endif



#define TEMP_BUFFER_SIZE 256
VOID
ListDrivers()

/*++

Routine Description:

    Got to the load list for the drivers, interpret and display what is there.

Arguments:

    None.

Return Value:

    NONE

--*/

{
    int            index;
    NTSTATUS       status;
    HANDLE         keyHandle;
    ULONG          resultLength;
    UCHAR          tempBuffer[TEMP_BUFFER_SIZE];
    ANSI_STRING    ansiString;
    UNICODE_STRING unicodeValueName;
    PKEY_BASIC_INFORMATION keyInformation;

    keyInformation = (PKEY_BASIC_INFORMATION)malloc(WORK_BUFFER_SIZE);

    status = FtOpenKey(&keyHandle, DRIVER_KEY);

    if (!NT_SUCCESS(status)) {

        printf("Could not open Services key (0x%x).\n", status);
        return;
    }

    for (index = 0; TRUE; index++) {

        RtlZeroMemory(keyInformation, WORK_BUFFER_SIZE);

        status = NtEnumerateKey(keyHandle,
                                index,
                                KeyBasicInformation,
                                keyInformation,
                                WORK_BUFFER_SIZE,
                                &resultLength);

        if (status == STATUS_NO_MORE_ENTRIES) {

            break;

        } else if (!NT_SUCCESS(status)) {

            printf("readreg: Error on Enumerate status = %x\n", status);
            break;

        }

        unicodeValueName.Length = (USHORT)keyInformation->NameLength;
        unicodeValueName.MaximumLength = (USHORT)keyInformation->NameLength;
        unicodeValueName.Buffer = (PWSTR)&keyInformation->Name[0];

        ansiString.MaximumLength = TEMP_BUFFER_SIZE;
        ansiString.Length = 0L;
        ansiString.Buffer = &tempBuffer[0];

        RtlUnicodeStringToAnsiString(&ansiString,
                                     &unicodeValueName,
                                     (BOOLEAN) FALSE);

        //
        // Now have the key name for the driver - concatenate it and
        // call the routine to display what is in the key.
        //

        sprintf(WorkingDirectory,
                "%s\\%s",
                DRIVER_KEY,
                tempBuffer);

        DisplayLoadInformation(WorkingDirectory);
    }

    free(keyInformation);
    NtClose(keyHandle);
}


VOID
main()

/*++

Routine Description:

    The main entry point for the user process.
    This process will prompt the user for the action desired.  This
    includes starting performance, stopping performance, and retreiving
    performance data collected by the FT driver.

Arguments:

    Command line:
        No options.

Return Value:

    NONE

--*/

{
    NTSTATUS status;
    BOOLEAN  batch;
    PUCHAR   argumentString;
    int      commandCode;
    HANDLE   keyHandle;


    status = FtOpenKey(&keyHandle, REGISTRY_BASE);

    if (!NT_SUCCESS(status)) {

        printf("readreg: Unable to open registry base (0x%x)\n", status);
        exit(1);
    }

    sprintf(CurrentDirectory,
            REGISTRY_BASE);

    //
    // See if we are connected to CON
    //

    batch = FALSE;
//  batch = (BOOLEAN)(!isatty(0));

    if (!batch) {
        printf("FT registry edit utility.  %s:\n", Version);
    }

    while(1) {
        while ((commandCode = GetCommand(batch,
                                         &argumentString)) == INVALID) {

            //
            // Continue until we get a valid command.
            //

        }

        if (Debug) {
            printf("Command code == %d, argumentString = %s\n",
                   commandCode,
                   (argumentString == NULL) ? "(none)" : argumentString);
        }

        switch (commandCode) {

        case DIRLONG:

            Directory(keyHandle, (BOOLEAN) TRUE);
            break;

        case DIR:

            Directory(keyHandle, (BOOLEAN) FALSE);
            break;

        case CREATE:
        {
            ULONG   index;
            PUCHAR  keyClass;
            BOOLEAN classAllocated = FALSE;

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "Key Name = ", 
                                                   (BOOLEAN) FALSE);
            }

            if (argumentString == NULL) {
                break;
            }

            sprintf(WorkingDirectory,
                    "%s\\%s",
                    CurrentDirectory,
                    argumentString);

            argumentString = GetArgumentString(batch,
                                               "Key Class = ",
                                               (BOOLEAN) FALSE);

            if (argumentString == NULL) {
                keyClass = "Default Class";
            } else {
                keyClass = (PUCHAR) malloc(strlen(argumentString) + FUDGE);
                classAllocated = TRUE;

                sprintf(keyClass,
                        "%s",
                        argumentString);
            }

            argumentString = GetArgumentString(batch,
                                               "Index = ",
                                               (BOOLEAN) TRUE);

            if (argumentString == NULL) {
                index = 1;
            } else {
                index = ParseArgumentNumeric(&argumentString);
            }

            if (Debug) {
                printf("Creating key %s, index %d with class %s\n",
                       WorkingDirectory,
                       index,
                       keyClass);
            }

            status = FtCreateKey(WorkingDirectory,
                                 keyClass,
                                 index);

            if (!NT_SUCCESS(status)) {

                printf("Could not create key %s (0x%x).\n",
                       WorkingDirectory,
                       status);
            }

            if (classAllocated == TRUE) {
                free(keyClass);
            }

            break;
        }

        case LIST:

            List(keyHandle,
                 argumentString);
            break;

        case CHDIR:

            NtClose(keyHandle);

            if (argumentString == NULL) {

                argumentString = GetArgumentString(batch,
                                                   "New location = ",
                                                   (BOOLEAN) TRUE);
            }

            if (argumentString != NULL) {

                if (*argumentString == '\\') {

                    //
                    // Root relative string.
                    // Use text provided (i.e. %s is to avoid user crashing
                    // by putting %s in the string).
                    //

                    sprintf(WorkingDirectory,
                            "%s",
                            argumentString);

                } else {

                    while ((*argumentString == '.') &&
                           (*(argumentString + 1) == '.')) {

                        if ((*(argumentString + 2) == '\\') ||
                            (*(argumentString + 2) == '\0')) {

                            PUCHAR cptr = CurrentDirectory;

                            //
                            // move argumentString past ".."
                            //

                            argumentString += 2;

                            //
                            // Find end of current directory.
                            //

                            while (*cptr != '\0') {
                                cptr++;
                            }

                            //
                            // Backup to last component.
                            //

                            while (*cptr != '\\') {
                                cptr--;
                            }

                            if (cptr == CurrentDirectory) {

                                //
                                // Cannot backup anymore.  Continue parsing
                                // argument.
                                //

                                continue;
                            }

                            //
                            // Remove component from path.
                            //

                            *cptr = '\0';

                            if (*argumentString == '\0') {

                                //
                                // All done with argument.
                                //

                                break;
                            }

                            //
                            // Step around backslash.
                            //

                            argumentString++;

                        } else {

                            //
                            // Assume it is a real name.
                            //

                            break;
                        }
                    }

                    if (*argumentString != '\0') {
                        sprintf(WorkingDirectory,
                                "%s\\%s",
                                CurrentDirectory,
                                argumentString);
                    } else {
                        sprintf(WorkingDirectory,
                                "%s",
                                CurrentDirectory);
                    }
                }

                status = FtOpenKey(&keyHandle,
                                   WorkingDirectory);

                if (NT_SUCCESS(status)) {

                    sprintf(CurrentDirectory,
                            "%s",
                            WorkingDirectory);
                } else {

                    (VOID) FtOpenKey(&keyHandle,
                                     CurrentDirectory);

                    //
                    // No error checks because this was opened once before.
                    //
                }

            }

            break;

        case HELP:
        {
            int i;

            printf("Valid commands are:\n");

            for (i = 0; Commands[i] != NULL; i++) {
                printf("  %10s  - %s\n",
                       Commands[i],
                       CommandHelp[CommandMap[i]]);
            }
            break;
        }

        case QUIT:

            exit(0);
            break;

        case DDEBUG:

            if (argumentString == NULL) {

                if (Debug) {

                    printf("Debug turned off.\n");
                    Debug = 0;
                } else {

                    Debug = 1;
                }
            } else {

                Debug = atoi(argumentString);
                printf("Debug set to %d\n", Debug);
            }
            break;

        case SETVALUE:
        {
            int    i;
            BOOLEAN convertToUnicode = FALSE;
            PUCHAR valueName;
            PUCHAR valueData;
            ULONG  valueLength;
            ULONG  valueWord;
            PVOID  valuePtr;
            ULONG  type = DEFAULT_TYPE;
            STRING         valueString;
            UNICODE_STRING unicodeValue;
            BOOLEAN dataAllocated = FALSE;
            BOOLEAN unicodeAllocated = FALSE;

            if (argumentString == NULL) {

                argumentString = GetArgumentString(batch,
                                                   "Value Name = ",
                                                   (BOOLEAN) FALSE);
            }

            if (argumentString == NULL) {

                break;
            }

            valueName = (PUCHAR) malloc(strlen(argumentString) + FUDGE);

            sprintf(valueName,
                    "%s",
                    argumentString);

            //
            // print a help banner on type and get the type.
            //

            for (i = 0; TypeNames[i] != NULL; i++) {

                printf("%d - %s\n", TypeNumbers[i], TypeNames[i]);
            }
            printf("# - Other numbers are user defined\n");
            argumentString = GetArgumentString(batch,
                                               "Numeric value for type = ",
                                               (BOOLEAN) TRUE);

            if (argumentString != NULL) {
                type = ParseArgumentNumeric(&argumentString);
            }

            switch(type)
            {
            default:
            case REG_SZ:
                if (type == REG_SZ) {
                    convertToUnicode = TRUE;
                    printf("Typed in string will be converted to unicode...\n");
                    argumentString = GetArgumentString(batch,
                                                       "Value Data = ",
                                                       (BOOLEAN) FALSE);
                } else {
                    printf("For now the data must be typed in...\n");
                    argumentString = GetArgumentString(batch,
                                                       "Value Data = ",
                                                       (BOOLEAN) FALSE);
                }

                if (argumentString == NULL) {
                    valueData = "Default Data";
                    valueLength = strlen(valueData);
                } else {
                    valueData = (PUCHAR) malloc(strlen(argumentString) + FUDGE);
                    dataAllocated = TRUE;
                    sprintf(valueData,
                            "%s",
                            argumentString);
                    if (convertToUnicode == TRUE) {
                        RtlInitString(&valueString,
                                      valueData);
                        RtlAnsiStringToUnicodeString(&unicodeValue,
                                                     &valueString,
                                                     (BOOLEAN) TRUE);
                        unicodeAllocated = TRUE;
                        valueLength = unicodeValue.Length + 2;
                    } else {
                        valueLength = strlen(valueData);
                    }
                }

                break;

            case REG_DWORD:
                argumentString = GetArgumentString(batch,
                                                   "Value Data Word = ",
                                                   (BOOLEAN) TRUE);
                if (argumentString == NULL) {
                    valueWord = 0;
                } else {
                    valueWord = ParseArgumentNumeric(&argumentString);
                }

                valueLength = sizeof(ULONG);
                break;
            }

            switch (type) {

            case REG_DWORD:
                valuePtr = (PVOID) &valueWord;
                break;

            case REG_SZ:
                valuePtr = (PVOID) unicodeValue.Buffer;
                break;

            default:
                valuePtr = (PVOID) valueData;
                break;
            }
            status = FtSetValue(keyHandle,
                                valueName,
                                valuePtr,
                                valueLength,
                                type);

            if (!NT_SUCCESS(status)) {
                printf("Could not set value %s (0x%x).\n", valueName, status);
            }

            free(valueName);
            if (dataAllocated == TRUE) {
                free(valueData);
            }
            if (unicodeAllocated == TRUE) {
                RtlFreeUnicodeString(&unicodeValue);
            }
            break;
        }

        case DELKEY:
        {
            if (argumentString == NULL) {

                argumentString = GetArgumentString(batch,
                                                   "Key Name = ",
                                                   (BOOLEAN) TRUE);
            }

            if (argumentString == NULL) {

                break;
            }

            sprintf(WorkingDirectory,
                    "%s\\%s",
                    CurrentDirectory,
                    argumentString);

            status = FtDeleteKey(WorkingDirectory);

            if (!NT_SUCCESS(status)) {
                printf("Unable to delete key %s (0x%x)\n",
                       WorkingDirectory,
                       status);
            }

            break;
        }

        case DELVALUE:
        {
            if (argumentString == NULL) {

                argumentString = GetArgumentString(batch,
                                                   "Key Name = ",
                                                   (BOOLEAN) TRUE);
            }

            if (argumentString == NULL) {

                break;
            }

            status = FtDeleteValue(keyHandle,
                                   argumentString);

            if (!NT_SUCCESS(status)) {

                printf("Unable to delete value %s (0x%x)\n",
                       argumentString,
                       status);
            }
            break;
        }

        case INLONG:
            DumpControl = InLongs;
            break;

        case INSHORT:
            DumpControl = InShorts;
            break;

        case INBYTE:
            DumpControl = InBytes;
            break;

        case DUMP:

            if (ForceDump) {
                ForceDump = 0;
            } else {
                ForceDump++;
            }
            break;

        case DISKREG:
            DiskDump();
            break;

        case FIXDISK:
            FixDisk();
            break;

        case RESTORE:
        {
            ULONG type;
            ULONG group;
            ULONG member;

            printf("FT types that can be restored are:\n");
            printf("\t%d - for Mirrors\n", Mirror);
            printf("\t%d - for Stripes with parity\n", StripeWithParity);

            //
            // Get the type
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT volume type = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                type = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            //
            // Get the group
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT group number = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                group = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            //
            // Get the member
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT member number = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                member = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            RestoreOrphan(type, group, member);
            break;
        }

        case DRIVERS:
            NotImplemented();
            // ListDrivers();
            break;

        case ORPHAN:
        {
            ULONG type;
            ULONG group;
            ULONG member;

            printf("FT types that can be orphaned are:\n");
            printf("\t%d - for Mirrors\n", Mirror);
            printf("\t%d - for Stripes with parity\n", StripeWithParity);

            //
            // Get the type
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT volume type = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                type = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            //
            // Get the group
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT group number = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                group = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            //
            // Get the member
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT member number = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                member = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            OrphanMember(type, group, member);
            break;
        }

        case REGEN:
        {
            ULONG type;
            ULONG group;
            ULONG member;

            printf("FT types that can be regenerated are:\n");
            printf("\t%d - for Mirrors\n", Mirror);
            printf("\t%d - for Stripes with parity\n", StripeWithParity);

            //
            // Get the type
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT volume type = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                type = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            //
            // Get the group
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT group number = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                group = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            //
            // Get the member
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "FT member number = ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                member = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            RegenerateMember(type, group, member);
            break;
        }

        case INIT:
        {
            ULONG type;
            ULONG group;
            ULONG member;

            printf("Only stripes with parity are initialized.\n");

            //
            // Get the group
            //

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                               "Parity stripe group number = ",
                                               (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                group = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            ChangeMemberState(StripeWithParity,
                              group,
                              0,
                              Initializing);
            break;
        }

        case MAKEFT:
        {
            ULONG type;
            ULONG group;
            ULONG member;
            ULONG disk;
            ULONG partition;
            PDISK_CONFIG_HEADER configHeader;
            BOOLEAN doUpdate = TRUE;

            configHeader = GetDiskInfo();
            if (configHeader == NULL) {
                break;
            }
            printf("\t%d for Mirrors\n", Mirror);
            printf("\t%d for Stripe Set\n", Stripe);
            printf("\t%d for Stripe with parity\n", StripeWithParity);
            printf("\t%d for Volume Set\n", VolumeSet);

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "Which FT set to create? ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                type = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            if (argumentString == NULL) {
                argumentString = GetArgumentString(batch,
                                                   "Please give an FT group # - ",
                                                   (BOOLEAN) TRUE);
            }
            if (argumentString != NULL) {
                group = ParseArgumentNumeric(&argumentString);
            } else {
                break;
            }

            for (member = 0; TRUE; member++) {
                printf("Information for member %d\n", member);

                if (argumentString == NULL) {
                    argumentString = GetArgumentString(batch,
                                                       "Disk Number = ",
                                                       (BOOLEAN) TRUE);
                }

                if (argumentString != NULL) {
                    disk = ParseArgumentNumeric(&argumentString);
                } else {
                    break;
                }

                if (argumentString == NULL) {
                    argumentString = GetArgumentString(batch,
                                                       "Partition Number = ",
                                                       (BOOLEAN) TRUE);
                }

                if (argumentString != NULL) {
                    partition = ParseArgumentNumeric(&argumentString);
                } else {
                    break;
                }

                if (CreateFtMember(configHeader, disk, partition, type, group, member) == FALSE) {
                    printf("Failed to change member state\n");
                    printf("No update will be made\n");
                    doUpdate = FALSE;
                    break;
                }
            }
            if (doUpdate == TRUE) {
                PDISK_REGISTRY diskRegistry;
                diskRegistry = (PDISK_REGISTRY)
                             ((PUCHAR) configHeader + configHeader->DiskInformationOffset);
                DiskRegistrySet(diskRegistry);
            }
            free(configHeader);
            break;
        }

        default:

            printf("WDF homer?!?\n");
            break;
        }
    }
} // main
