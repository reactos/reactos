#include "regutil.h"
#include "edithive.h"

NTSTATUS
RiInitializeRegistryFromAsciiFile(
    IN PUNICODE_STRING HiveName,
    IN PUNICODE_STRING FileName
    );

void
Usage( void )
{
    fprintf( stderr, "usage: HIVEINI -f hivefile [files...]\n" );
    exit( 1 );
}

PVOID OldValueBuffer;
ULONG OldValueBufferSize;

typedef struct _KEY_INFO {
    ULONG IndentAmount;
    UNICODE_STRING Name;
    HANDLE HiveHandle;
    HANDLE Handle;
    LARGE_INTEGER LastWriteTime;
} KEY_INFO, *PKEY_INFO;

#define MAX_KEY_DEPTH 64

NTSTATUS
RiInitializeRegistryFromAsciiFile(
    IN PUNICODE_STRING HiveName,
    IN PUNICODE_STRING FileName
    )
{
    NTSTATUS Status;
    REG_UNICODE_FILE UnicodeFile;
    PWSTR EndKey, FirstEqual, BeginValue;
    ULONG IndentAmount;
    UNICODE_STRING InputLine;
    UNICODE_STRING KeyName;
    UNICODE_STRING KeyValue;
    PKEY_VALUE_FULL_INFORMATION OldValueInformation;
    PKEY_BASIC_INFORMATION KeyInformation;
    UCHAR KeyInformationBuffer[ 512 ];
    ULONG ResultLength;
    ULONG OldValueLength;
    PVOID ValueBuffer;
    ULONG ValueLength;
    ULONG ValueType;
    KEY_INFO KeyPath[ MAX_KEY_DEPTH ];
    PKEY_INFO CurrentKey;
    ULONG KeyPathLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Class;
    ULONG Disposition;
    BOOLEAN UpdateKeyValue;
    ULONG i;
    HANDLE HiveHandle;
    HANDLE RootKey;
    UNICODE_STRING RootName;

    HiveHandle = EhOpenHive(HiveName,
                            &KeyPath[0].Handle,
                            &KeyPath[0].Name,
                            TYPE_SIMPLE);
    if (HiveHandle == NULL) {
        return(STATUS_OBJECT_PATH_NOT_FOUND);
    }
    KeyPath[0].Handle = (HANDLE)HCELL_NIL;

    OldValueInformation = (PKEY_VALUE_FULL_INFORMATION)OldValueBuffer;
    Class.Buffer = NULL;
    Class.Length = 0;

    Status = RegLoadAsciiFileAsUnicode( FileName,
                                        &UnicodeFile
                                      );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    KeyPathLength = 0;
    while (RegGetNextLine( &UnicodeFile, &IndentAmount, &FirstEqual )) {
#if 0
        InputLine.Buffer = UnicodeFile.BeginLine;
        InputLine.Length = (USHORT)((PCHAR)UnicodeFile.EndOfLine - (PCHAR)UnicodeFile.BeginLine);
        InputLine.MaximumLength = InputLine.Length;
        printf( "GetNextLine: (%02u) '%wZ'\n", IndentAmount, &InputLine );
#endif
        if (FirstEqual == NULL) {
            KeyName.Buffer = UnicodeFile.BeginLine;
            KeyName.Length = (USHORT)((PCHAR)UnicodeFile.EndOfLine - (PCHAR)KeyName.Buffer);
            KeyName.MaximumLength = (USHORT)(KeyName.Length + 1);

#if 0
            printf( "%02u %04u  KeyName: %wZ\n", KeyPathLength, IndentAmount, &KeyName );
#endif
            CurrentKey = &KeyPath[ KeyPathLength ];
            if (KeyPathLength == 0 ||
                IndentAmount > CurrentKey->IndentAmount
               ) {
                if (KeyPathLength == MAX_KEY_DEPTH) {
                    fprintf( stderr,
                             "HIVEINI: %wZ key exceeded maximum depth (%u) of tree.\n",
                             &KeyName,
                             MAX_KEY_DEPTH
                           );

                    return( STATUS_UNSUCCESSFUL );
                    }
                KeyPathLength++;
                CurrentKey++;
                }
            else {
                do {
                    CurrentKey->Handle = NULL;
                    if (IndentAmount == CurrentKey->IndentAmount) {
                        break;
                        }
                    CurrentKey--;
                    if (--KeyPathLength == 1) {
                        break;
                        }
                    }
                while (IndentAmount <= CurrentKey->IndentAmount);
                }

#if 0
            printf( "  (%02u)\n", KeyPathLength );
#endif
            CurrentKey->Name = KeyName;
            CurrentKey->IndentAmount = IndentAmount;
            Status = EhCreateChild(HiveHandle,
                                   KeyPath[ KeyPathLength-1 ].Handle,
                                   &KeyName,
                                   &CurrentKey->Handle,
                                   &Disposition);

            if (NT_SUCCESS( Status )) {
                if (DebugOutput) {
                    fprintf( stderr, "    Created key %02x %wZ (%08x)\n",
                                     CurrentKey->IndentAmount,
                                     &CurrentKey->Name,
                                     CurrentKey->Handle
                           );
                    }
                KeyInformation = (PKEY_BASIC_INFORMATION)KeyInformationBuffer;
                Status = EhQueryKey( HiveHandle,
                                     CurrentKey->Handle,
                                     KeyBasicInformation,
                                     KeyInformation,
                                     sizeof( KeyInformationBuffer ),
                                     &ResultLength
                                   );
                if (NT_SUCCESS( Status )) {
                    CurrentKey->LastWriteTime = KeyInformation->LastWriteTime;
                    }
                else {
                    RtlZeroMemory( &CurrentKey->LastWriteTime,
                                   sizeof( CurrentKey->LastWriteTime )
                                 );

                    }

                if (Disposition == REG_CREATED_NEW_KEY) {
                    printf( "Created Key: " );
                    for (i=0; i<KeyPathLength; i++) {
                        printf( "%wZ\\", &KeyPath[ i ].Name );
                        }
                    printf( "%wZ\n", &KeyName );
                    }
                }
            else {
                fprintf( stderr,
                         "HIVEINI: CreateKey (%wZ) relative to handle (%lx) failed - %lx\n",
                         &KeyName,
                         ObjectAttributes.RootDirectory,
                         Status
                       );
                }
            }
        else {
            if (FirstEqual == UnicodeFile.BeginLine) {
                KeyName.Buffer = NULL;
                KeyName.Length = 0;
                KeyName.MaximumLength = 0;
                }
            else {
                EndKey = FirstEqual;
                while (EndKey > UnicodeFile.BeginLine && EndKey[ -1 ] <= L' ') {
                    EndKey--;
                    }
                KeyName.Buffer = UnicodeFile.BeginLine;
                KeyName.Length = (USHORT)((PCHAR)EndKey - (PCHAR)KeyName.Buffer);
                KeyName.MaximumLength = (USHORT)(KeyName.Length + 1);
                }

            BeginValue = FirstEqual + 1;
            while (BeginValue < UnicodeFile.EndOfLine && *BeginValue <= L' ') {
                BeginValue++;
                }
            KeyValue.Buffer = BeginValue;
            KeyValue.Length = (USHORT)((PCHAR)UnicodeFile.EndOfLine - (PCHAR)BeginValue);
            KeyValue.MaximumLength = (USHORT)(KeyValue.Length + 1);

            while (IndentAmount <= CurrentKey->IndentAmount) {
                if (DebugOutput) {
                    fprintf( stderr, "    Popping from key %02x %wZ (%08x)\n",
                                     CurrentKey->IndentAmount,
                                     &CurrentKey->Name,
                                     CurrentKey->Handle
                           );
                    }
                CurrentKey->Handle = NULL;
                CurrentKey--;
                if (--KeyPathLength == 1) {
                    break;
                    }
                }
            if (DebugOutput) {
                fprintf( stderr, "    Adding value '%wZ = %wZ' to key %02x %wZ (%08x)\n",
                                 &KeyName,
                                 &KeyValue,
                                 CurrentKey->IndentAmount,
                                 &CurrentKey->Name,
                                 CurrentKey->Handle
                       );
                }

            if (RegGetKeyValue( &KeyValue,
                                &UnicodeFile,
                                &ValueType,
                                &ValueBuffer,
                                &ValueLength
                              )
               ) {
                if (ValueBuffer == NULL) {
                    Status = EhDeleteValueKey( HiveHandle,
                                               KeyPath[ KeyPathLength+1 ].Handle,
                                               &KeyValue
                                             );
                    if (NT_SUCCESS( Status )) {
                        printf( "Delete value for Key: " );
                        for (i=0; i<KeyPathLength; i++) {
                            printf( "%wZ\\", &KeyPath[ i ].Name );
                            }
                        printf( "%wZ\n", &KeyName );
                        }
                    }
                else {
                    if ( UnicodeFile.LastWriteTime.QuadPart >
                         CurrentKey->LastWriteTime.QuadPart
                       ) {
                        Status = STATUS_UNSUCCESSFUL;
                        UpdateKeyValue = TRUE;
                        }
                    else {
                        Status = EhQueryValueKey( HiveHandle,
                                                  CurrentKey->Handle,
                                                  &KeyName,
                                                  KeyValueFullInformation,
                                                  OldValueInformation,
                                                  OldValueBufferSize,
                                                  &OldValueLength
                                                );
                        if (NT_SUCCESS( Status )) {
                            UpdateKeyValue = TRUE;
                            }
                        else {
                            UpdateKeyValue = FALSE;
                            }
                        }

                    if (!NT_SUCCESS( Status ) ||
                        OldValueInformation->Type != ValueType ||
                        OldValueInformation->DataLength != ValueLength ||
                        !RtlEqualMemory( (PCHAR)OldValueInformation +
                                            OldValueInformation->DataOffset,
                                          ValueBuffer,
                                          ValueLength )
                       ) {

                        Status = EhSetValueKey( HiveHandle,
                                                CurrentKey->Handle,
                                                &KeyName,
                                                0,
                                                ValueType,
                                                ValueBuffer,
                                                ValueLength
                                              );
                        if (NT_SUCCESS( Status )) {
                            printf( "%s value for Key: ",
                                    UpdateKeyValue ? "Updated" : "Created"
                                  );
                            for (i=1; i<=KeyPathLength; i++) {
                                printf( "%wZ\\", &KeyPath[ i ].Name );
                                }

                            if (KeyName.Length) {
                                printf( "%wZ ", &KeyName );
                                }
                            printf( "= '%wZ'\n", &KeyValue );
                            }
                        else {
                            fprintf( stderr,
                                     "HIVEINI: SetValueKey (%wZ) failed - %lx\n",
                                     &KeyName,
                                     Status
                                   );
                            }
                        }

                    RtlFreeHeap( RtlProcessHeap(), 0, ValueBuffer );
                    }
                }
            else {
                fprintf( stderr,
                         "HIVEINI: Invalid key (%wZ) value (%wZ)\n", &KeyName,
                         &KeyValue
                       );
                }
            }
        }

        EhCloseHive(HiveHandle);

    return( Status );
}


int
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    int i;
    char *s;
    NTSTATUS Status;
    BOOL FileArgumentSeen;
    BOOL HiveArgumentSeen=FALSE;
    ANSI_STRING AnsiString;
    UNICODE_STRING DosFileName;
    UNICODE_STRING FileName;
    UNICODE_STRING DosHiveName;
    UNICODE_STRING HiveName;

    OldValueBufferSize = VALUE_BUFFER_SIZE;
    OldValueBuffer = VirtualAlloc( NULL, OldValueBufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (OldValueBuffer == NULL) {
        fprintf( stderr, "HIVEINI: Unable to allocate value buffer.\n" );
        exit( 1 );
        }

    RegInitialize();

    FileArgumentSeen = FALSE;
    HiveName.Length = HiveName.MaximumLength = 0;
    HiveName.Buffer = NULL;
    for (i=1; i<argc; i++) {
        s = argv[ i ];
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'd':
                        DebugOutput = TRUE;
                        break;

                    case 'f':
                        if (++i < argc) {
                            RtlInitAnsiString(&AnsiString, argv[i]);
                            RtlAnsiStringToUnicodeString( &DosHiveName,
                                                          &AnsiString,
                                                          TRUE );
                            RtlDosPathNameToNtPathName_U( DosHiveName.Buffer,
                                                          &HiveName,
                                                          NULL,
                                                          NULL );
                            break;
                        }

                    default:    Usage();
                    }
                }
            }
        else {
            FileArgumentSeen = TRUE;
            RtlInitAnsiString( &AnsiString, s );
            Status = RtlAnsiStringToUnicodeString( &DosFileName, &AnsiString, TRUE );
            if (NT_SUCCESS( Status )) {
                if (RtlDosPathNameToNtPathName_U( DosFileName.Buffer,
                                                  &FileName,
                                                  NULL,
                                                  NULL
                                                )
                   ) {
                    Status = RiInitializeRegistryFromAsciiFile( &HiveName,
                                                                &FileName );
                    if (!NT_SUCCESS( Status )) {
                        fprintf( stderr,
                                 "HIVEINI: Failed to load from %wZ - Status == %lx\n",
                                 &FileName,
                                 Status
                           );
                        }
                    }
                else {
                    Status = STATUS_UNSUCCESSFUL;
                    fprintf( stderr,
                             "HIVEINI: Unable to map Dos Name (%wZ) to NT name\n",
                             &DosFileName
                       );
                    }
                }
            else {
                fprintf( stderr,
                         "HIVEINI: Unable to convert %s to unicode - Status == %lx\n",
                         &AnsiString,
                         Status
                       );
                }
            }
        }

    if (!FileArgumentSeen) {
        RtlInitUnicodeString( &FileName, L"\\SystemRoot\\System32\\Config\\registry.sys" );
        Status = RiInitializeRegistryFromAsciiFile( &HiveName,
                                                    &FileName );
        if (!NT_SUCCESS( Status )) {
            fprintf( stderr,
                     "HIVEINI: Failed to load from %wZ - Status == %lx\n",
                     &FileName,
                     Status
                   );
            }
        else {
            RtlInitUnicodeString( &FileName, L"\\SystemRoot\\System32\\Config\\registry.usr" );
            Status = RiInitializeRegistryFromAsciiFile( &HiveName,
                                                        &FileName );
            if (!NT_SUCCESS( Status )) {
                fprintf( stderr,
                         "HIVEINI: Failed to load from %wZ - Status == %lx\n",
                         &FileName,
                         Status
                       );
                }
            }
        }

    return( 0 );
}
