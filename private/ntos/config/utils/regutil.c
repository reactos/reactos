/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regutil.c

Abstract:

    Utility routines for use by REGINI and REGDMP programs.
Author:

    Steve Wood (stevewo)  10-Mar-92

Revision History:

--*/

#include "regutil.h"

#define RtlAllocateHeap(x,y,z) malloc(z)
#define RtlFreeHeap(x,y,z) free(z)

UNICODE_STRING RiOnKeyword;
UNICODE_STRING RiYesKeyword;
UNICODE_STRING RiTrueKeyword;
UNICODE_STRING RiOffKeyword;
UNICODE_STRING RiNoKeyword;
UNICODE_STRING RiFalseKeyword;
UNICODE_STRING RiDeleteKeyword;
UNICODE_STRING RiRegKeyword;
UNICODE_STRING RiRegNoneKeyword;
UNICODE_STRING RiRegSzKeyword;
UNICODE_STRING RiRegExpandSzKeyword;
UNICODE_STRING RiRegDwordKeyword;
UNICODE_STRING RiRegBinaryKeyword;
UNICODE_STRING RiRegBinaryFileKeyword;
UNICODE_STRING RiRegLinkKeyword;
UNICODE_STRING RiRegMultiSzKeyword;
UNICODE_STRING RiRegMultiSzFileKeyword;
UNICODE_STRING RiRegDateKeyword;

void
RegInitialize( void )
{
    RtlInitUnicodeString( &RiOnKeyword, L"ON" );
    RtlInitUnicodeString( &RiYesKeyword, L"YES" );
    RtlInitUnicodeString( &RiTrueKeyword, L"TRUE" );
    RtlInitUnicodeString( &RiOffKeyword, L"OFF" );
    RtlInitUnicodeString( &RiNoKeyword, L"NO" );
    RtlInitUnicodeString( &RiFalseKeyword, L"FALSE" );
    RtlInitUnicodeString( &RiDeleteKeyword, L"DELETE" );
    RtlInitUnicodeString( &RiRegKeyword, L"REG_" );
    RtlInitUnicodeString( &RiRegNoneKeyword, L"REG_NONE" );
    RtlInitUnicodeString( &RiRegSzKeyword, L"REG_SZ" );
    RtlInitUnicodeString( &RiRegExpandSzKeyword, L"REG_EXPAND_SZ" );
    RtlInitUnicodeString( &RiRegDwordKeyword, L"REG_DWORD" );
    RtlInitUnicodeString( &RiRegBinaryKeyword, L"REG_BINARY" );
    RtlInitUnicodeString( &RiRegBinaryFileKeyword, L"REG_BINARYFILE" );
    RtlInitUnicodeString( &RiRegLinkKeyword, L"REG_LINK" );
    RtlInitUnicodeString( &RiRegMultiSzKeyword, L"REG_MULTI_SZ" );
    RtlInitUnicodeString( &RiRegMultiSzFileKeyword, L"REG_MULTISZFILE" );
    RtlInitUnicodeString( &RiRegDateKeyword, L"REG_DATE" );
}

NTSTATUS
RegReadMultiSzFile(
    IN PUNICODE_STRING FileName,
    OUT PVOID *ValueBuffer,
    OUT PULONG ValueLength
    )
{
    NTSTATUS Status;
    UNICODE_STRING NtFileName;
    PWSTR s;
    UNICODE_STRING MultiSource;
    UNICODE_STRING MultiValue;
    REG_UNICODE_FILE MultiSzFile;
    ULONG MultiSzFileSize;


    FileName->Buffer[ FileName->Length/sizeof(WCHAR) ] = UNICODE_NULL;

    RtlDosPathNameToNtPathName_U( FileName->Buffer,
                                  &NtFileName,
                                  NULL,
                                  NULL );

    Status = RegLoadAsciiFileAsUnicode( &NtFileName, &MultiSzFile );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    MultiSzFileSize = (MultiSzFile.EndOfFile -
                       MultiSzFile.NextLine) * sizeof(WCHAR);

    *ValueLength = 0;
    *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                    MultiSzFileSize);

    MultiSource.Buffer = MultiSzFile.NextLine;
    if (MultiSzFileSize <= MAXUSHORT) {
        MultiSource.Length =
        MultiSource.MaximumLength = (USHORT)MultiSzFileSize;
    } else {
        MultiSource.Length =
        MultiSource.MaximumLength = MAXUSHORT;
    }

    while (RegGetMultiString(&MultiSource, &MultiValue)) {
        RtlMoveMemory( (PUCHAR)*ValueBuffer + *ValueLength,
                       MultiValue.Buffer,
                       MultiValue.Length );
        *ValueLength += MultiValue.Length;

        s = MultiSource.Buffer;
        while ( *s != L'"' &&
                *s != L',' &&
                ((s - MultiSource.Buffer) * sizeof(WCHAR)) <
                    MultiSource.Length ) s++;
        if ( ((s - MultiSource.Buffer) * sizeof(WCHAR)) ==
             MultiSource.Length ||
             *s == L',' ||
             *s == L';'   ) {

            ((PWSTR)*ValueBuffer)[ *ValueLength / sizeof(WCHAR) ] =
                UNICODE_NULL;
            *ValueLength += sizeof(UNICODE_NULL);
            if ( *s ==  L';' ) {
                break;
            }
        }

        if ( (MultiSzFile.EndOfFile - MultiSource.Buffer) * sizeof(WCHAR) >=
              MAXUSHORT ) {
            MultiSource.Length =
            MultiSource.MaximumLength = MAXUSHORT;
        } else {
            MultiSource.Length =
            MultiSource.MaximumLength =
                (USHORT)((MultiSzFile.EndOfFile - MultiSource.Buffer) *
                         sizeof(WCHAR));
        }
    }

    ((PWSTR)*ValueBuffer)[ *ValueLength / sizeof(WCHAR) ] = UNICODE_NULL;
    *ValueLength += sizeof(UNICODE_NULL);

    // Virtual memory for reading of MultiSzFile freed at process
    // death?

    return( TRUE );
}

NTSTATUS
RegReadBinaryFile(
    IN PUNICODE_STRING FileName,
    OUT PVOID *ValueBuffer,
    OUT PULONG ValueLength
    )
{
    NTSTATUS Status;
    UNICODE_STRING NtFileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE File;
    FILE_STANDARD_INFORMATION FileInformation;
    WCHAR FileNameBuffer[ 256 ];
    PWSTR s;

    FileName->Buffer[ FileName->Length/sizeof(WCHAR) ] = UNICODE_NULL;
    wcscpy( FileNameBuffer, L"\\DosDevices\\" );
    s = wcscat( FileNameBuffer, FileName->Buffer );
    while (*s != UNICODE_NULL) {
        if (*s == L'/') {
            *s = L'\\';
            }
        s++;
        }
    RtlInitUnicodeString( &NtFileName, FileNameBuffer );

    InitializeObjectAttributes( &ObjectAttributes,
                                &NtFileName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE)NULL,
                                NULL
                              );

    Status = NtOpenFile( &File,
                         SYNCHRONIZE | GENERIC_READ,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_DELETE |
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_NON_DIRECTORY_FILE
                       );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = NtQueryInformationFile( File,
                                     &IoStatus,
                                     (PVOID)&FileInformation,
                                     sizeof( FileInformation ),
                                     FileStandardInformation
                                   );
    if (NT_SUCCESS( Status )) {
        if (FileInformation.EndOfFile.HighPart) {
            Status = STATUS_BUFFER_OVERFLOW;
            }
        }
    if (!NT_SUCCESS( Status )) {
        NtClose( File );
        return( Status );
        }

    *ValueLength = FileInformation.EndOfFile.LowPart;
    *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, *ValueLength );
    if (*ValueBuffer == NULL) {
        Status = STATUS_NO_MEMORY;
        }

    if (NT_SUCCESS( Status )) {
        Status = NtReadFile( File,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             *ValueBuffer,
                             *ValueLength,
                             NULL,
                             NULL
                           );

        if (NT_SUCCESS( Status )) {
            Status = IoStatus.Status;

            if (NT_SUCCESS( Status )) {
                if (IoStatus.Information != *ValueLength) {
                    Status = STATUS_END_OF_FILE;
                    }
                }
            }

        if (!NT_SUCCESS( Status )) {
            RtlFreeHeap( RtlProcessHeap(), 0, *ValueBuffer );
            }
        }

    NtClose( File );
    return( Status );
}

NTSTATUS
RegLoadAsciiFileAsUnicode(
    IN PUNICODE_STRING FileName,
    OUT PREG_UNICODE_FILE UnicodeFile
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE File;
    FILE_BASIC_INFORMATION FileDateTimeInfo;
    FILE_STANDARD_INFORMATION FileInformation;
    ULONG BufferSize, i, i1, LineCount;
    PVOID BufferBase;
    PCHAR Src, Src1;
    PWSTR Dst;

    InitializeObjectAttributes( &ObjectAttributes,
                                FileName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE)NULL,
                                NULL
                              );

    Status = NtOpenFile( &File,
                         SYNCHRONIZE | GENERIC_READ,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_DELETE |
                            FILE_SHARE_READ |
                            FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_NON_DIRECTORY_FILE
                       );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    Status = NtQueryInformationFile( File,
                                     &IoStatus,
                                     (PVOID)&FileInformation,
                                     sizeof( FileInformation ),
                                     FileStandardInformation
                                   );
    if (NT_SUCCESS( Status )) {
        if (FileInformation.EndOfFile.HighPart) {
            Status = STATUS_BUFFER_OVERFLOW;
            }
        }
    if (!NT_SUCCESS( Status )) {
        NtClose( File );
        return( Status );
        }


    BufferSize = FileInformation.EndOfFile.LowPart * sizeof( WCHAR );
    BufferSize += sizeof( UNICODE_NULL );
    BufferBase = NULL;
    Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&BufferBase,
                                      0,
                                      &BufferSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    if (NT_SUCCESS( Status )) {
        Src = (PCHAR)BufferBase + ((FileInformation.EndOfFile.LowPart+1) & ~1);
        Dst = (PWSTR)BufferBase;
        Status = NtReadFile( File,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             Src,
                             FileInformation.EndOfFile.LowPart,
                             NULL,
                             NULL
                           );

        if (NT_SUCCESS( Status )) {
            Status = IoStatus.Status;

            if (NT_SUCCESS( Status )) {
                if (IoStatus.Information != FileInformation.EndOfFile.LowPart) {
                    Status = STATUS_END_OF_FILE;
                    }
                else {
                    Status = NtQueryInformationFile( File,
                                                     &IoStatus,
                                                     (PVOID)&FileDateTimeInfo,
                                                     sizeof( FileDateTimeInfo ),
                                                     FileBasicInformation
                                                   );
                    }
                }
            }

        if (!NT_SUCCESS( Status )) {
            NtFreeVirtualMemory( NtCurrentProcess(),
                                 (PVOID *)&BufferBase,
                                 &BufferSize,
                                 MEM_RELEASE
                               );
            }
        }

    NtClose( File );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    i = 0;
    while (i < FileInformation.EndOfFile.LowPart) {
        if (i > 1 && (Src[-2] == ' ' || Src[-2] == '\t') &&
            Src[-1] == '\\' && (*Src == '\r' || *Src == '\n')
           ) {
            if (Dst[-1] == L'\\') {
                --Dst;
                }
            while (Dst > (PWSTR)BufferBase) {
                if (Dst[-1] > L' ') {
                    break;
                    }
                Dst--;
                }
            LineCount = 0;
            while (i < FileInformation.EndOfFile.LowPart) {
                if (*Src == '\n') {
                    i++;
                    Src++;
                    LineCount++;
                    }
                else
                if (*Src == '\r' &&
                    (i+1) < FileInformation.EndOfFile.LowPart &&
                    Src[ 1 ] == '\n'
                   ) {
                    i += 2;
                    Src += 2;
                    LineCount++;
                    }
                else {
                    break;
                    }
                }

            if (LineCount > 1) {
                *Dst++ = L'\n';
                }
            else {
                *Dst++ = L' ';
                while (i < FileInformation.EndOfFile.LowPart && (*Src == ' ' || *Src == '\t')) {
                    i++;
                    Src++;
                    }
                }

            if (i >= FileInformation.EndOfFile.LowPart) {
                break;
                }
            }
        else
        if ((*Src == '\r' && Src[1] == '\n') || *Src == '\n') {
            while (TRUE) {
                while (i < FileInformation.EndOfFile.LowPart && (*Src == '\r' || *Src == '\n')) {
                    i++;
                    Src++;
                    }
                Src1 = Src;
                i1 = i;
                while (i1 < FileInformation.EndOfFile.LowPart && (*Src1 == ' ' || *Src1 == '\t')) {
                    i1++;
                    Src1++;
                    }
                if (i1 < FileInformation.EndOfFile.LowPart &&
                    (*Src1 == '\r' && Src1[1] == '\n') || *Src1 == '\n'
                   ) {
                    Src = Src1;
                    i = i1;
                    }
                else {
                    break;
                    }
                }

            *Dst++ = L'\n';
            }
        else {
            i++;
            *Dst++ = RtlAnsiCharToUnicodeChar( &Src );
            }
        }

    if (NT_SUCCESS( Status )) {
        *Dst = UNICODE_NULL;
        UnicodeFile->FileContents = BufferBase;
        UnicodeFile->EndOfFile = Dst;
        UnicodeFile->BeginLine = NULL;
        UnicodeFile->EndOfLine = NULL;
        UnicodeFile->NextLine = BufferBase;
        UnicodeFile->LastWriteTime = FileDateTimeInfo.LastWriteTime;
        }
    else {
        NtFreeVirtualMemory( NtCurrentProcess(),
                             (PVOID *)&BufferBase,
                             &BufferSize,
                             MEM_RELEASE
                           );
        }

    return( Status );
}


BOOLEAN
RegGetNextLine(
    IN OUT PREG_UNICODE_FILE UnicodeFile,
    OUT PULONG IndentAmount,
    OUT PWSTR *FirstEqual
    )
{
    PWSTR s, s1;

    while (TRUE) {
        if (!(s = UnicodeFile->NextLine)) {
            return( FALSE );
            }

        *IndentAmount = 0;
        while (*s <= L' ') {
            if (*s == L' ') {
                *IndentAmount += 1;
                }
            else
            if (*s == L'\t') {
                *IndentAmount = ((*IndentAmount + 8) -
                                 (*IndentAmount % 8)
                                );
                }

            if (++s >= UnicodeFile->EndOfFile) {
                return( FALSE );
                }
            }

        UnicodeFile->BeginLine = s;

        *FirstEqual = NULL;
        UnicodeFile->NextLine = NULL;
        while (s < UnicodeFile->EndOfFile) {
            if (*s == L'=') {
                if (*FirstEqual == NULL) {
                    *FirstEqual = s;
                    }
                }
            else
            if (*s == L'\n') {
                s1 = s;
                while (s > UnicodeFile->BeginLine && s[ -1 ] <= L' ') {
                    s--;
                    }
                UnicodeFile->EndOfLine = s;
                do {
                    if (++s1 >= UnicodeFile->EndOfFile) {
                        s1 = NULL;
                        break;
                        }
                    }
                while (*s1 == L'\r' || *s1 == L'\n');

                UnicodeFile->NextLine = s1;
                break;
                }

            if (++s == UnicodeFile->EndOfFile) {
                break;
                }
            }

        if (UnicodeFile->EndOfLine > UnicodeFile->BeginLine) {
            if (DebugOutput) {
                fprintf( stderr, "%02u  %.*ws\n",
                         *IndentAmount,
                         UnicodeFile->EndOfLine - UnicodeFile->BeginLine,
                         UnicodeFile->BeginLine
                       );
                }

            return( TRUE );
            }
        }

    return( FALSE );
}


void
RegDumpKeyValue(
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
        cbPrefix += fprintf( fh, "%wZ ", &ValueName );
        }
    cbPrefix += fprintf( fh, "= " );

    if (KeyValueInformation->DataLength == 0) {
        fprintf( fh, " [no data] \n");
        return;
    }

    switch( KeyValueInformation->Type ) {
    case REG_SZ:
    case REG_EXPAND_SZ:

        if (KeyValueInformation->Type == REG_EXPAND_SZ) {
            cbPrefix += fprintf( fh, "REG_EXPAND_SZ " );
        }
        pw = (PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
        *(PWSTR)((PCHAR)pw + KeyValueInformation->DataLength) = UNICODE_NULL;
        i = 0;
        while (*pw) {
            if ((cbPrefix + wcslen(pw)) > 80) {
                pw1 = pw;
                while (*pw1 && *pw1 > L' ') {
                    pw1++;
                }

                if (*pw1) {
                    *pw1++ = UNICODE_NULL;
                    while (*pw1 && *pw1 <= L' ') {
                        pw1++;
                    }
                }
            } else {
                pw1 = NULL;
            }
            if (i > 0) {
                fprintf( fh, " \\\n%.*s",
                         cbPrefix,
                         "                                                                                  "
                       );
            }

            fprintf( fh, "%ws", pw );
            if (!pw1) {
                break;
                }
            i++;
            pw = pw1;
        }
        break;

    case REG_BINARY:
        fprintf( fh, "REG_BINARY 0x%08lx", KeyValueInformation->DataLength );
        p = (PULONG)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
        i = (KeyValueInformation->DataLength + 3) / sizeof( ULONG );
        if (!SummaryOutput || i <= 8) {
            for (j=0; j<i; j++) {
                if ((j % 8) == 0) {
                    fprintf( fh, "\n%.*s",
                             IndentLevel+4,
                             "                                                                                  "
                           );
                    }

                fprintf( fh, "0x%08lx  ", *p++ );
                }
            }
        else {
            fprintf( fh, " *** value display suppressed ***" );
            }
        fprintf( fh, "\n" );
        break;

//  case REG_DWORD_LITTLE_ENDIAN:
    case REG_DWORD:
        fprintf( fh, "REG_DWORD 0x%08lx",
                 *((PULONG)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset))
               );
        break;

    case REG_DWORD_BIG_ENDIAN:
        fprintf( fh, "REG_DWORD_BIG_ENDIAN 0x%08lx",
                 *((PULONG)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset))
               );
        break;

    case REG_LINK:
        fprintf( fh, "REG_LINK %ws",
                 ((PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset))
               );
        break;

    case REG_MULTI_SZ:
        cbPrefix += fprintf( fh, "REG_MULTI_SZ " );
        pw = (PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
        i  = 0;
        if (*pw)
        while (i < ((KeyValueInformation->DataLength-1) / sizeof(WCHAR))) {
            if (i > 0) {
                fprintf( fh, " \\\n%.*s",
                         cbPrefix,
                         "                                                                                  "
                       );
            }
            fprintf(fh, "\"%ws\" ",pw+i);
            do {
                ++i;
            } while ( pw[i] != UNICODE_NULL );
            ++i;
        }
        break;

    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
        {
        PCM_RESOURCE_LIST ResourceList = ((PCM_RESOURCE_LIST)((PCHAR)KeyValueInformation +
                                          KeyValueInformation->DataOffset));
        PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
        ULONG k, l, count;
        PWSTR TypeName;
        PWSTR FlagName;
        ULONG Size = KeyValueInformation->DataLength;

        if (KeyValueInformation->Type == REG_RESOURCE_LIST) {

            fprintf( fh, "   REG_RESOURCE_LIST\n");

            fprintf( fh, "%.*sNumber of Full resource Descriptors = %d",
                     IndentLevel,
                     "                                                                                  ",
                     ResourceList->Count
                   );

             count = ResourceList->Count;
             FullDescriptor = &ResourceList->List[0];

        } else {

            fprintf( fh, "   REG_FULL_RESOURCE_DESCRIPTOR\n");
            count = 1;
            FullDescriptor = ((PCM_FULL_RESOURCE_DESCRIPTOR)
                ((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset));

        }

        for (i=0; i< count; i++) {

            fprintf( fh, "\n%.*sPartial List number %d\n",
                     IndentLevel+4,
                     "                                                                                  ",
                     i
                   );

            switch(FullDescriptor->InterfaceType) {

            case Internal:      TypeName = L"Internal";         break;
            case Isa:           TypeName = L"Isa";              break;
            case Eisa:          TypeName = L"Eisa";             break;
            case MicroChannel:  TypeName = L"MicroChannel";     break;
            case TurboChannel:  TypeName = L"TurboChannel";     break;
            case PCIBus:        TypeName = L"PCI";              break;
            case VMEBus:        TypeName = L"VME";              break;
            case NuBus:         TypeName = L"NuBus";            break;
            case PCMCIABus:     TypeName = L"PCMCIA";           break;
            case CBus:          TypeName = L"CBUS";             break;
            case MPIBus:        TypeName = L"MPI";              break;

            default:
                TypeName = L"***invalid bus type***";
                break;
            }

            fprintf( fh, "%.*sINTERFACE_TYPE %ws\n",
                     IndentLevel+8,
                     "                                                                                  ",
                     TypeName
                   );

            fprintf( fh, "%.*sBUS_NUMBER  %d\n",
                     IndentLevel+8,
                     "                                                                                  ",
                     FullDescriptor->BusNumber
                   );

            //
            // This is a basic test to see if the data format is right.
            // We know at least some video resource list are bogus ...
            //

            if (Size < FullDescriptor->PartialResourceList.Count *
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) ) {

                fprintf( fh, "\n%.*s *** !!! Invalid ResourceList !!! *** \n",
                         IndentLevel+8,
                         "                                                                                  ",
                         i
                       );

                break;
            }

            Size -= FullDescriptor->PartialResourceList.Count *
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);



            for (j=0; j<FullDescriptor->PartialResourceList.Count; j++) {

                fprintf( fh, "%.*sDescriptor number %d\n",
                         IndentLevel+12,
                         "                                                                                  ",
                         j
                       );

                PartialResourceDescriptor =
            &(FullDescriptor->PartialResourceList.PartialDescriptors[j]);

                switch(PartialResourceDescriptor->ShareDisposition) {

                case CmResourceShareUndetermined:
                    TypeName = L"CmResourceShareUndetermined";
                    break;
                case CmResourceShareDeviceExclusive:
                    TypeName = L"CmResourceDeviceExclusive";
                    break;
                case CmResourceShareDriverExclusive:
                    TypeName = L"CmResourceDriverExclusive";
                    break;
                case CmResourceShareShared:
                    TypeName = L"CmResourceShared";
                    break;
                default:
                    TypeName = L"***invalid share disposition***";
                    break;
                }

                fprintf( fh, "%.*sShare Disposition %ws\n",
                         IndentLevel+12,
                         "                                                                                  ",
                         TypeName
                       );

                FlagName = L"***invalid Flags";

                switch(PartialResourceDescriptor->Type) {

                case CmResourceTypeNull:
                    TypeName = L"NULL";
                    FlagName = L"***Unused";
                    break;
                case CmResourceTypePort:
                    TypeName = L"PORT";
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_PORT_MEMORY) {
                        FlagName = L"CM_RESOURCE_PORT_MEMORY";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_PORT_IO) {
                        FlagName = L"CM_RESOURCE_PORT_IO";
                    }
                    break;
                case CmResourceTypeInterrupt:
                    TypeName = L"INTERRUPT";
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
                        FlagName = L"CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_INTERRUPT_LATCHED) {
                        FlagName = L"CM_RESOURCE_INTERRUPT_LATCHED";
                    }
                    break;
                case CmResourceTypeMemory:
                    TypeName = L"MEMORY";
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_MEMORY_READ_WRITE) {
                        FlagName = L"CM_RESOURCE_MEMORY_READ_WRITE";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_MEMORY_READ_ONLY) {
                        FlagName = L"CM_RESOURCE_MEMORY_READ_ONLY";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_MEMORY_WRITE_ONLY) {
                        FlagName = L"CM_RESOURCE_MEMORY_WRITE_ONLY";
                    }
                    break;
                case CmResourceTypeDma:
                    TypeName = L"DMA";
                    FlagName = L"***Unused";
                    break;
                case CmResourceTypeDeviceSpecific:
                    TypeName = L"DEVICE SPECIFIC";
                    FlagName = L"***Unused";
                    break;
                default:
                    TypeName = L"***invalid type***";
                    break;
                }

                fprintf( fh, "%.*sTYPE              %ws\n",
                         IndentLevel+12,
                         "                                                                                  ",
                         TypeName
                       );

                fprintf( fh, "%.*sFlags             %ws\n",
                         IndentLevel+12,
                         "                                                                                  ",
                         FlagName
                       );

                switch(PartialResourceDescriptor->Type) {

                case CmResourceTypePort:
                    fprintf( fh, "%.*sSTART 0x%08lx  LENGTH 0x%08lx\n",
                             IndentLevel+12,
                             "                                                                                  ",
                             PartialResourceDescriptor->u.Port.Start.LowPart,
                             PartialResourceDescriptor->u.Port.Length
                           );
                    break;

                case CmResourceTypeInterrupt:
                    fprintf( fh, "%.*sLEVEL %d  VECTOR %d  AFFINITY %d\n",
                             IndentLevel+12,
                             "                                                                                  ",
                             PartialResourceDescriptor->u.Interrupt.Level,
                             PartialResourceDescriptor->u.Interrupt.Vector,
                             PartialResourceDescriptor->u.Interrupt.Affinity
                           );
                    break;

                case CmResourceTypeMemory:
                    fprintf( fh, "%.*sSTART 0x%08lx%08lx  LENGTH 0x%08lx\n",
                             IndentLevel+12,
                             "                                                                                  ",
                             PartialResourceDescriptor->u.Memory.Start.HighPart,
                             PartialResourceDescriptor->u.Memory.Start.LowPart,
                             PartialResourceDescriptor->u.Memory.Length
                           );
                    break;

                case CmResourceTypeDma:
                    fprintf( fh, "%.*sCHANNEL %d  PORT %d\n",
                             IndentLevel+12,
                             "                                                                                  ",
                             PartialResourceDescriptor->u.Dma.Channel,
                             PartialResourceDescriptor->u.Dma.Port
                           );
                    break;

                case CmResourceTypeDeviceSpecific:
                    fprintf( fh, "%.*sDataSize 0x%08lx\n",
                             IndentLevel+12,
                             "                                                                                  ",
                             PartialResourceDescriptor->u.DeviceSpecificData.DataSize
                           );

                    p = (PULONG)(PartialResourceDescriptor + 1);
                    k = (PartialResourceDescriptor->u.DeviceSpecificData.DataSize + 3) / sizeof( ULONG );
                    for (l=0; l<k; l++) {
                        if ((l % 8) == 0) {
                            fprintf( fh, "\n%.*s",
                                IndentLevel+12,
                                "                                                                                  "
                               );
                        }

                        fprintf( fh, "0x%08lx  ", *p++ );
                    }
                    fprintf( fh, "\n" );
                    break;

                default:
                    fprintf( fh, "%.*s*** Unknown resource list type: %c ****\n",
                             IndentLevel+12,
                             "                                                                                  ",
                             PartialResourceDescriptor->Type
                           );
                    break;
                }

                fprintf( fh, "\n" );
            }

            FullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) (PartialResourceDescriptor+1);
        }

        break;
        }

    case REG_NONE:
    default:
        if (KeyValueInformation->Type == REG_NONE) {
            fprintf( fh, "REG_NONE\n");
            }
        else {
            fprintf( fh, "*** Unknown registry type (%08lx)",
                     KeyValueInformation->Type
                   );
            }
        fprintf( fh, "%.*s",
             IndentLevel,
             "                                                                                  "
             );
        fprintf( fh, "    Length: 0x%lx\n", KeyValueInformation->DataLength );
                fprintf( fh, "\n%.*s",
                    IndentLevel,
                    "                                                                                  "
                    );
        fprintf( fh, "      Data: ");
        pbyte = ((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
        for ( k=0, m=1; k<KeyValueInformation->DataLength; k++,m++) {
            fprintf( fh, "%02x ", (*pbyte) );
            pbyte++;

            if (m==8) {
                fprintf( fh, "\n%.*s",
                    IndentLevel+12,
                    "                                                                                  "
                    );
                m=0;
            }
        }
        break;
    }

    fprintf( fh, "\n" );
    return;
}

//
//  Define an upcase macro for temporary use by the upcase routines
//

#define upcase(C) (WCHAR )(((C) >= 'a' && (C) <= 'z' ? (C) - ('a' - 'A') : (C)))

BOOLEAN
RegGetMultiString(
    IN OUT PUNICODE_STRING ValueString,
    OUT PUNICODE_STRING MultiString
    )

/*++

Routine Description:

    This routine parses multi-strings of the form

        "foo" "bar" "bletch"

    Each time it is called, it strips the first string in quotes from
    the input string, and returns it as the multi-string.

    INPUT ValueString: "foo" "bar" "bletch"

    OUTPUT ValueString: "bar" "bletch"
           MultiString: foo

Arguments:

    ValueString - Supplies the string from which the multi-string will be
                  parsed
                - Returns the remaining string after the multi-string is
                  removed

    MultiString - Returns the multi-string removed from ValueString

Return Value:

    TRUE - multi-string found and removed.

    FALSE - no more multi-strings remaining.

--*/

{
    //
    // Find the first quote mark.
    //
    while ((*(ValueString->Buffer) != L'"') &&
           (ValueString->Length > 0)) {
        ++ValueString->Buffer;
        ValueString->Length -= sizeof(WCHAR);
        ValueString->MaximumLength -= sizeof(WCHAR);
    }

    if (ValueString->Length == 0) {
        return(FALSE);
    }

    //
    // We have found the start of the multi-string.  Now find the end,
    // building up our return MultiString as we go.
    //
    ++ValueString->Buffer;
    ValueString->Length -= sizeof(WCHAR);
    ValueString->MaximumLength -= sizeof(WCHAR);
    MultiString->Buffer = ValueString->Buffer;
    MultiString->Length = 0;
    MultiString->MaximumLength = 0;
    while ((*(ValueString->Buffer) != L'"') &&
           (ValueString->Length > 0)) {
        ++ValueString->Buffer;
        ValueString->Length -= sizeof(WCHAR);
        ValueString->MaximumLength -= sizeof(WCHAR);

        MultiString->Length += sizeof(WCHAR);
        MultiString->MaximumLength += sizeof(WCHAR);
    }

    if (ValueString->Length == 0) {
        return(FALSE);
    }

    ++ValueString->Buffer;
    ValueString->Length -= sizeof(WCHAR);
    ValueString->MaximumLength -= sizeof(WCHAR);

    return( TRUE );

}


BOOLEAN
RegGetKeyValue(
    IN PUNICODE_STRING InitialKeyValue,
    IN OUT PREG_UNICODE_FILE UnicodeFile,
    OUT PULONG ValueType,
    OUT PVOID *ValueBuffer,
    OUT PULONG ValueLength
    )
{
    ULONG PrefixLength;
    PWSTR s;
    PULONG p;
    ULONG n;
    NTSTATUS Status;
    ULONG IndentAmount;
    PWSTR FirstEqual;
    UNICODE_STRING KeyValue;
    UNICODE_STRING MultiValue;
    BOOLEAN GetDataFromBinaryFile = FALSE;
    BOOLEAN GetDataFromMultiSzFile = FALSE;
    BOOLEAN ParseDateTime = FALSE;

    KeyValue = *InitialKeyValue;
    if (RtlPrefixUnicodeString( &RiDeleteKeyword, &KeyValue, TRUE )) {
        *ValueBuffer = NULL;
        return( TRUE );
        }
    else
    if (!RtlPrefixUnicodeString( &RiRegKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_SZ;
        PrefixLength = 0;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegNoneKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_NONE;
        PrefixLength = RiRegNoneKeyword.Length;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegSzKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_SZ;
        PrefixLength = RiRegSzKeyword.Length;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegExpandSzKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_EXPAND_SZ;
        PrefixLength = RiRegExpandSzKeyword.Length;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegDwordKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_DWORD;
        PrefixLength = RiRegDwordKeyword.Length;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegBinaryFileKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_BINARY;
        PrefixLength = RiRegBinaryFileKeyword.Length;
        GetDataFromBinaryFile = TRUE;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegBinaryKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_BINARY;
        PrefixLength = RiRegBinaryKeyword.Length;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegLinkKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_LINK;
        PrefixLength = RiRegLinkKeyword.Length;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegMultiSzFileKeyword, &KeyValue, TRUE)) {
        *ValueType = REG_MULTI_SZ;
        PrefixLength = RiRegMultiSzFileKeyword.Length;
        GetDataFromMultiSzFile = TRUE;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegMultiSzKeyword, &KeyValue, TRUE)) {
        *ValueType = REG_MULTI_SZ;
        PrefixLength = RiRegMultiSzKeyword.Length;
        }
    else
    if (RtlPrefixUnicodeString( &RiRegDateKeyword, &KeyValue, TRUE )) {
        *ValueType = REG_BINARY;
        ParseDateTime = TRUE;
        PrefixLength = RiRegDateKeyword.Length;
        }
    else {
        return( FALSE );
        }

    if (*ValueType != REG_NONE) {
        s = (PWSTR)
            ((PCHAR)KeyValue.Buffer + PrefixLength);
        KeyValue.Length -= (USHORT)PrefixLength;
        while (KeyValue.Length != 0 && *s <= L' ') {
            s++;
            KeyValue.Length -= sizeof( WCHAR );
            }
        KeyValue.Buffer = s;
        }
    else {
        *ValueType = REG_SZ;
        }


    if (GetDataFromBinaryFile) {
        Status = RegReadBinaryFile( &KeyValue, ValueBuffer, ValueLength );
        if (NT_SUCCESS( Status )) {
            return( TRUE );
            }
        else {
            fprintf( stderr, "REGINI: Unable to read data from %wZ - Status == %lx\n", &KeyValue, Status );
            return( FALSE );
            }
        }

    if (GetDataFromMultiSzFile) {
        Status = RegReadMultiSzFile( &KeyValue, ValueBuffer, ValueLength );
        if (NT_SUCCESS( Status )) {
            return( TRUE );
            }
        else {
            fprintf( stderr, "REGINI: Unable to read data from %wZ - Status == %lx\n", &KeyValue, Status );
            return( FALSE );
            }
        }

    switch( *ValueType ) {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_LINK:
        *ValueLength = KeyValue.Length + sizeof( UNICODE_NULL );
        *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, *ValueLength );
        if (*ValueBuffer == NULL) {
            return( FALSE );
            }

        RtlMoveMemory( *ValueBuffer, KeyValue.Buffer, KeyValue.Length );
        ((PWSTR)*ValueBuffer)[ KeyValue.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
        return( TRUE );

    case REG_DWORD:
        *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( ULONG ) );
        if (*ValueBuffer == NULL) {
            return( FALSE );
            }

        if (RtlPrefixUnicodeString( &RiTrueKeyword, &KeyValue, TRUE ) ||
            RtlPrefixUnicodeString( &RiYesKeyword, &KeyValue, TRUE ) ||
            RtlPrefixUnicodeString( &RiOnKeyword, &KeyValue, TRUE )
           ) {
            *(PULONG)*ValueBuffer = (ULONG)TRUE;
            }
        else
        if (RtlPrefixUnicodeString( &RiFalseKeyword, &KeyValue, TRUE ) ||
            RtlPrefixUnicodeString( &RiNoKeyword, &KeyValue, TRUE ) ||
            RtlPrefixUnicodeString( &RiOffKeyword, &KeyValue, TRUE )
           ) {
            *(PULONG)*ValueBuffer = (ULONG)FALSE;
            }
        else {
            Status = RtlUnicodeStringToInteger( &KeyValue, 0, (PULONG)*ValueBuffer );
            if (!NT_SUCCESS( Status )) {
                fprintf( stderr, "REGINI: CharToInteger( %wZ ) failed - Status == %lx\n", &KeyValue, Status );
                RtlFreeHeap( RtlProcessHeap(), 0, *ValueBuffer );
                return( FALSE );
                }
            }

        *ValueLength = sizeof( ULONG );
        return( TRUE );

    case REG_BINARY:
        if (ParseDateTime) {
#define NUMBER_DATE_TIME_FIELDS 6
            ULONG FieldIndexes[ NUMBER_DATE_TIME_FIELDS  ] = {1, 2, 0, 3, 4, 7};
            //
            // Month/Day/Year HH:MM DayOfWeek
            //

            ULONG CurrentField = 0;
            PCSHORT Fields;
            TIME_FIELDS DateTimeFields;
            UNICODE_STRING Field;
            ULONG FieldValue;

            RtlZeroMemory( &DateTimeFields, sizeof( DateTimeFields ) );
            Fields = &DateTimeFields.Year;
            while (KeyValue.Length) {
                if (CurrentField >= 7) {
                    return( FALSE );
                    }

                s = KeyValue.Buffer;
                while (KeyValue.Length && *s == L' ') {
                    KeyValue.Length--;
                    s++;
                    }

                Field.Buffer = s;
                while (KeyValue.Length) {
                    if (CurrentField == (NUMBER_DATE_TIME_FIELDS-1)) {
                        }
                    else
                    if (*s < L'0' || *s > L'9') {
                        break;
                        }

                    KeyValue.Length--;
                    s++;
                    }

                Field.Length = (USHORT)((PCHAR)s - (PCHAR)Field.Buffer);
                Field.MaximumLength = Field.Length;

                if (KeyValue.Length) {
                    KeyValue.Length--;
                    s++;
                    }
                KeyValue.Buffer = s;

                if (CurrentField == (NUMBER_DATE_TIME_FIELDS-1)) {
                    if (Field.Length < 3) {
                        printf( "REGINI: %wZ invalid day of week length\n", &Field );
                        return FALSE;
                        }

                    if (DateTimeFields.Year != 0) {
                        printf( "REGINI: Year must be zero to specify day of week\n" );
                        return FALSE;
                        }

                    if (!_wcsnicmp( Field.Buffer, L"SUN", 3 )) {
                        FieldValue = 0;
                        }
                    else
                    if (!_wcsnicmp( Field.Buffer, L"MON", 3 )) {
                        FieldValue = 1;
                        }
                    else
                    if (!_wcsnicmp( Field.Buffer, L"TUE", 3 )) {
                        FieldValue = 2;
                        }
                    else
                    if (!_wcsnicmp( Field.Buffer, L"WED", 3 )) {
                        FieldValue = 3;
                        }
                    else
                    if (!_wcsnicmp( Field.Buffer, L"THU", 3 )) {
                        FieldValue = 4;
                        }
                    else
                    if (!_wcsnicmp( Field.Buffer, L"FRI", 3 )) {
                        FieldValue = 5;
                        }
                    else
                    if (!_wcsnicmp( Field.Buffer, L"SAT", 3 )) {
                        FieldValue = 6;
                        }
                    else {
                        printf( "REGINI: %wZ invalid day of week\n", &Field );
                        return FALSE;
                        }
                    }
                else {
                    Status = RtlUnicodeStringToInteger( &Field, 10, &FieldValue );
                    if (!NT_SUCCESS( Status )) {
                        return( FALSE );
                        }
                    }

                Fields[ FieldIndexes[ CurrentField++ ] ] = (CSHORT)FieldValue;
                }

            if (DateTimeFields.Year == 0) {
                if (DateTimeFields.Day > 5) {
                    printf( "REGINI: Day must be 0 - 5 if year is zero.\n" );
                    return FALSE;
                    }
                }
            else
            if (DateTimeFields.Year < 100) {
                DateTimeFields.Year += 1900;
                }

            *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( DateTimeFields ) );
            *ValueLength = sizeof( DateTimeFields );
            RtlMoveMemory( *ValueBuffer, &DateTimeFields, sizeof( DateTimeFields ) );
            return TRUE;
            }
        else {
            Status = RtlUnicodeStringToInteger( &KeyValue, 0, ValueLength );
            if (!NT_SUCCESS( Status )) {
                return( FALSE );
                }
            s = KeyValue.Buffer;
            while (KeyValue.Length != 0 && *s > L' ') {
                s++;
                KeyValue.Length -= sizeof( WCHAR );
                }
            KeyValue.Buffer = s;
            }
        break;

    case REG_MULTI_SZ:
        *ValueLength = 0;
        *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, KeyValue.Length + sizeof( UNICODE_NULL ) );
        while (RegGetMultiString(&KeyValue, &MultiValue)) {
            RtlMoveMemory( (PUCHAR)*ValueBuffer + *ValueLength,
                           MultiValue.Buffer,
                           MultiValue.Length );
            *ValueLength += MultiValue.Length;
            ((PWSTR)*ValueBuffer)[ *ValueLength / sizeof(WCHAR) ] = UNICODE_NULL;
            *ValueLength += sizeof(UNICODE_NULL);
        }
        ((PWSTR)*ValueBuffer)[ *ValueLength / sizeof(WCHAR) ] = UNICODE_NULL;
        *ValueLength += sizeof(UNICODE_NULL);

        return( TRUE );

    default:
        return( FALSE );
    }

    *ValueBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, *ValueLength );
    p = *ValueBuffer;
    n = (*ValueLength + sizeof( ULONG ) - 1) / sizeof( ULONG );
    while (n--) {
        if (KeyValue.Length == 0) {
            if (!RegGetNextLine( UnicodeFile, &IndentAmount, &FirstEqual )) {
                RtlFreeHeap( RtlProcessHeap(), 0, *ValueBuffer );
                return( FALSE );
                }
            KeyValue.Buffer = UnicodeFile->BeginLine;
            KeyValue.Length = (USHORT)
                ((PCHAR)UnicodeFile->EndOfLine - (PCHAR)UnicodeFile->BeginLine);
            KeyValue.MaximumLength = KeyValue.Length;
            }

        s = KeyValue.Buffer;
        while (KeyValue.Length != 0 && *s <= L' ') {
            s++;
            KeyValue.Length -= sizeof( WCHAR );
            }
        KeyValue.Buffer = s;
        if (KeyValue.Length != 0) {
            Status = RtlUnicodeStringToInteger( &KeyValue, 0, p );
            if (!NT_SUCCESS( Status )) {
                RtlFreeHeap( RtlProcessHeap(), 0, *ValueBuffer );
                return( FALSE );
                }
            p++;

            s = KeyValue.Buffer;
            while (KeyValue.Length != 0 && *s > L' ') {
                s++;
                KeyValue.Length -= sizeof( WCHAR );
                }
            KeyValue.Buffer = s;
            }
        }

    return( TRUE );
}

BOOLEAN
RtlPrefixUnicodeString(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )

/*++

Routine Description:

    The RtlPrefixUnicodeString function determines if the String1
    counted string parameter is a prefix of the String2 counted string
    parameter.

    The CaseInSensitive parameter specifies if case is to be ignored when
    doing the comparison.

Arguments:

    String1 - Pointer to the first unicode string.

    String2 - Pointer to the second unicode string.

    CaseInsensitive - TRUE if case should be ignored when doing the
        comparison.

Return Value:

    Boolean value that is TRUE if String1 equals a prefix of String2 and
    FALSE otherwise.

--*/

{
    PWSTR s1, s2;
    ULONG n;
    WCHAR c1, c2;

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    if (String2->Length < String1->Length) {
        return( FALSE );
        }

    n = String1->Length / sizeof( c1 );
    while (n) {
        c1 = *s1++;
        c2 = *s2++;

        if (CaseInSensitive) {
            c1 = upcase(c1);
            c2 = upcase(c2);
        }
        if (c1 != c2) {
            return( FALSE );
            }

        n--;
        }

    return( TRUE );
}
