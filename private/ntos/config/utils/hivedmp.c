/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivedmp.c

Abstract:

    Utility to display all or part of the registry in a format that
    is suitable for input to the REGINI program.

    HIVEDMP [-r] -f filename

    Will ennumerate and dump out the subkeys and values of KeyPath,
    and then apply itself recursively to each subkey it finds.

    Handles all value types (e.g. REG_???) defined in ntregapi.h

    -r forces ALL value type to be output in RAW (hex) form.

    Default KeyPath if none specified is \Registry

Author:

    Steve Wood (stevewo)  12-Mar-92

Revision History:

    30-Nov-92   bryanwi     Add -r switch

--*/

#include "regutil.h"
#include "edithive.h"

void
DumpValues(
    HANDLE HiveHandle,
    HANDLE KeyHandle,
    ULONG IndentLevel
    );

void
DumpKeys(
    HANDLE HiveHandle,
    HANDLE KeyHandle,
    PUNICODE_STRING KeyName,
    ULONG IndentLevel
    );

void
RegDumpKeyValueR(
    FILE *fh,
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    ULONG IndentLevel
    );

PVOID ValueBuffer;
ULONG ValueBufferSize;

BOOLEAN RawOutput = FALSE;

void
Usage( void )
{
    fprintf( stderr, "usage: HIVEDMP [-f hivefile]\n" );
    exit( 1 );
}


void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    char *s;
    ANSI_STRING AnsiString;
    UNICODE_STRING KeyName;
    UNICODE_STRING DosName;
    UNICODE_STRING FileName;
    UNICODE_STRING RootName;
    HANDLE HiveHandle = NULL;
    HANDLE RootKey = NULL;
    BOOLEAN ArgumentSeen;
    LPSTR HiveFile=NULL;

    ValueBufferSize = VALUE_BUFFER_SIZE;
    ValueBuffer = VirtualAlloc( NULL, ValueBufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (ValueBuffer == NULL) {
        fprintf( stderr, "REGDMP: Unable to allocate value buffer.\n" );
        exit( 1 );
        }

    ArgumentSeen = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'd':
                        DebugOutput = TRUE;
                        break;

                    case 's':
                        SummaryOutput = TRUE;
                        break;

                    case 'r':
                        RawOutput = TRUE;
                        break;

                    case 'f':
                        if (argc--) {
                            RtlInitString( &AnsiString, *++argv );
                            RtlAnsiStringToUnicodeString( &DosName,
                                                          &AnsiString,
                                                          TRUE );
                            RtlDosPathNameToNtPathName_U( DosName.Buffer,
                                                          &FileName,
                                                          NULL,
                                                          NULL );
                            HiveHandle = EhOpenHive( &FileName,
                                                     &RootKey,
                                                     &RootName,
                                                     TYPE_SIMPLE );
                            ArgumentSeen = TRUE;
                            break;
                        }

                    default:    Usage();
                    }
                }
            }
#if 0
        else {
            RtlInitString( &AnsiString, s );
            RtlAnsiStringToUnicodeString( &KeyName, &AnsiString, TRUE );
            DumpKeys( HiveHandle, RootKey, &KeyName, 0 );
            ArgumentSeen = TRUE;
            }
#endif
        }

    if (ArgumentSeen) {
        if (HiveHandle != NULL) {
            DumpKeys( HiveHandle, RootKey, &RootName, 0 );
        } else {
            fprintf(stderr, "Couldn't open hive file %wZ\n",&DosName);
        }
    } else {
        Usage();
    }


    exit( 0 );
}


void
DumpKeys(
    HANDLE HiveHandle,
    HANDLE KeyHandle,
    PUNICODE_STRING KeyName,
    ULONG IndentLevel
    )
{
    NTSTATUS Status;
    HANDLE SubKeyHandle;
    WCHAR KeyBuffer[ 512 ];
    PKEY_BASIC_INFORMATION KeyInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG SubKeyIndex;
    UNICODE_STRING SubKeyName;
    ULONG ResultLength;


    //
    // Print name of node we are about to dump out
    //
    printf( "%.*s%wZ\n",
            IndentLevel,
            "                                                                                  ",
            KeyName
          );

    //
    // Print out node's values
    //
    DumpValues( HiveHandle, KeyHandle, IndentLevel+4 );

    //
    // Enumerate node's children and apply ourselves to each one
    //

    KeyInformation = (PKEY_BASIC_INFORMATION)KeyBuffer;
    for (SubKeyIndex = 0; TRUE; SubKeyIndex++) {
        Status = EhEnumerateKey( HiveHandle,
                                 KeyHandle,
                                 SubKeyIndex,
                                 KeyBasicInformation,
                                 KeyInformation,
                                 sizeof( KeyBuffer ),
                                 &ResultLength
                               );

        if (Status == STATUS_NO_MORE_ENTRIES) {
            return;
            }
        else
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr,
                     "REGDMP: NtEnumerateKey failed - Status ==%08lx\n",
                     Status
                   );
            exit( 1 );
            }

        SubKeyName.Buffer = (PWSTR)&(KeyInformation->Name[0]);
        SubKeyName.Length = (USHORT)KeyInformation->NameLength;
        SubKeyName.MaximumLength = (USHORT)KeyInformation->NameLength;

        Status = EhOpenChildByName( HiveHandle,
                                    KeyHandle,
                                    &SubKeyName,
                                    &SubKeyHandle );
        if (NT_SUCCESS(Status)) {
            DumpKeys( HiveHandle, SubKeyHandle, &SubKeyName, IndentLevel+4 );
        }
    }

}


void
DumpValues(
    HANDLE HiveHandle,
    HANDLE KeyHandle,
    ULONG IndentLevel
    )
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG ValueIndex;
    ULONG ResultLength;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)ValueBuffer;
    for (ValueIndex = 0; TRUE; ValueIndex++) {
        Status = EhEnumerateValueKey( HiveHandle,
                                      KeyHandle,
                                      ValueIndex,
                                      KeyValueFullInformation,
                                      KeyValueInformation,
                                      ValueBufferSize,
                                      &ResultLength
                                    );
        if (Status == STATUS_NO_MORE_ENTRIES) {
            return;
        } else if (!NT_SUCCESS( Status )) {
            fprintf( stderr,
                     "REGDMP: NtEnumerateValueKey failed - Status == %08lx\n",
                     Status
                   );
            exit( 1 );
        }

        if (RawOutput == TRUE) {
            RegDumpKeyValueR( stdout, KeyValueInformation, IndentLevel );
        } else {
            RegDumpKeyValue( stdout, KeyValueInformation, IndentLevel );
        }
    }
}


void
RegDumpKeyValueR(
    FILE *fh,
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    ULONG IndentLevel
    )
{
    PULONG p;
    PWSTR pw, pw1;
    ULONG i, j, k, m, cbPrefix;
    UNICODE_STRING ValueName;
    PUCHAR pbyte;

    cbPrefix = fprintf( fh, "%.*s",
                        IndentLevel,
                        "                                                                                  "
                      );
    ValueName.Buffer = (PWSTR)&(KeyValueInformation->Name[0]);
    ValueName.Length = (USHORT)KeyValueInformation->NameLength;
    ValueName.MaximumLength = (USHORT)KeyValueInformation->NameLength;

    if (ValueName.Length) {
        cbPrefix += fprintf( fh, "%wS ", &ValueName );
        }
    cbPrefix += fprintf( fh, "= " );

    if (KeyValueInformation->DataLength == 0) {
        fprintf( fh, " [no data] \n");
        return;
    }

    fprintf( fh, "REG_BINARY 0x%08lx", KeyValueInformation->DataLength );
    p = (PULONG)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
    i = (KeyValueInformation->DataLength + 3) / sizeof( ULONG );
    for (j=0; j<i; j++) {
        if ((j % 8) == 0) {
            fprintf( fh, "\n%.*s",
                     IndentLevel+4,
                     "                                                                                  "
                   );
            }

        fprintf( fh, "0x%08lx  ", *p++ );
        }
    fprintf( fh, "\n" );

    fprintf( fh, "\n" );
    return;
}











