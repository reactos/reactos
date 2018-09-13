/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1994   Microsoft Corporation

Module Name:

    perfname.c

Abstract:

    This file returns the Counter names or help text.



Author:

    HonWah Chan  10/12/93

Revision History:



--*/
#define UNICODE
#define _UNICODE
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpsapi.h>
#include <ntdddisk.h>
#include <ntregapi.h>
#include <ntioapi.h>
#include <ntprfctr.h>
#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4
#define QUERY_COUNTER   5
#define QUERY_HELP      6
#define QUERY_ADDCOUNTER   7
#define QUERY_ADDHELP      8

#define  LANG_ID_START  25
WCHAR    FileNameTemplate[] = L"\\SystemRoot\\system32\\perf0000.dat";
WCHAR    DefaultLangId[] = L"009";
WCHAR    NativeLangId[4] = L"\0";

extern   WCHAR COUNTER_STRING[];
extern   WCHAR HELP_STRING[];
extern   WCHAR ADDCOUNTER_STRING[];
extern   WCHAR ADDHELP_STRING[];


NTSTATUS
PerfGetNames    (
   IN    DWORD    QueryType,
   IN    PUNICODE_STRING lpValueName,
   OUT   LPBYTE   lpData,
   OUT   LPDWORD  lpcbData,
   OUT   LPDWORD  lpcbLen  OPTIONAL,
   IN    LPWSTR   lpLanguageId   OPTIONAL
   )
/*++

PerfGetCounterName

Arguments - Get either counter names or help text for the given language.
      If there is no language ID specified in the input, the default English
      version is returned.

Inputs -

   QueryType      -  Either QUERY_COUNTER or QUERY_HELP
                     or QUERY_ADDCOUNTER or QUERY_ADDHELP

   lpValueName    -  Either "Counter ???" or "Explain ???"
                     or "Addcounter ???" or "Addexplain ???"

   lpData         -  pointer to a buffer to reveive the names

   lpcbData       -  pointer to a variable containing the size in bytes of
                     the output buffer; on output, will receive the number
                     of bytes actually returned

   lpcbLen        -  Return the number of bytes to transmit to
                     the client (used by RPC) (optional).

   lpLanguageId   -  Input string for the language id desired.

   Return Value -

            error code indicating status of call or
            ERROR_SUCCESS if all ok


--*/
{
    UNICODE_STRING NtFileName;
    NTSTATUS Status;
    WCHAR    Names[50];
    ULONG    NameLen;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION FileInformation;
    HANDLE  File;
    LPWSTR  pLangIdRequest;
    BOOL    bAddNames;

    // build the file name
    RtlMoveMemory (Names, FileNameTemplate, sizeof(FileNameTemplate));

    if (QueryType == QUERY_ADDCOUNTER || QueryType == QUERY_ADDHELP) {
        bAddNames = TRUE;
    } else {
        bAddNames = FALSE;
    }

    if (QueryType == QUERY_COUNTER || QueryType == QUERY_ADDCOUNTER) {
        Names[LANG_ID_START] = L'c';
        NameLen = wcslen(COUNTER_STRING);
    } else {
        NameLen = wcslen(HELP_STRING);
        Names[LANG_ID_START] = L'h';
    }

    if (lpLanguageId) {
        pLangIdRequest = lpLanguageId;
    } else {
        // get the lang id from the input lpValueName
        pLangIdRequest = lpValueName->Buffer + NameLen;
        do {
            if (lpValueName->Length < (NameLen + 3) * sizeof(WCHAR)) {
                // lpValueName is too small to contain the lang id, use default
                pLangIdRequest = DefaultLangId;
                break;
            }

            if (*pLangIdRequest >= L'0' && *pLangIdRequest <= L'9') {
                // found the first digit
                break;
            }
            pLangIdRequest++;
            NameLen++;
        } while (TRUE);
    }

    Names[LANG_ID_START + 1] = *pLangIdRequest++;
    Names[LANG_ID_START + 2] = *pLangIdRequest++;
    Names[LANG_ID_START + 3] = *pLangIdRequest;

    RtlInitUnicodeString(&NtFileName, Names);

    // open the file for info
    InitializeObjectAttributes( &ObjectAttributes,
                                &NtFileName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE)NULL,
                                NULL
                              );
    if (bAddNames) {
        // writing name to data file

        LARGE_INTEGER   ByteOffset;

        ByteOffset.LowPart = ByteOffset.HighPart = 0;
        Status = NtCreateFile( &File,
                               SYNCHRONIZE | GENERIC_WRITE,
                               &ObjectAttributes,
                               &IoStatus,
                               NULL,               // no initial size
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ,
                               FILE_SUPERSEDE,     // always create
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,               // no ea buffer
                               0                   // no ea buffer
                           );
        if (!NT_SUCCESS( Status )) {
            return( Status );
        }
        Status = NtWriteFile( File,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatus,
                              lpData,
                              *lpcbData,
                              &ByteOffset,
                              NULL
                             );

        if (!NT_SUCCESS( Status )) {
            NtClose( File );
            return( Status );
        }
    } else {
        // reading name from data file
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

        if (!ARGUMENT_PRESENT (lpData) ||
            *lpcbData < FileInformation.EndOfFile.LowPart) {
            NtClose( File );
            if (ARGUMENT_PRESENT (lpcbLen)) {
                // no data yet for the rpc
                *lpcbLen = 0;
            }
            *lpcbData = FileInformation.EndOfFile.LowPart;
            if (ARGUMENT_PRESENT (lpData)) {
                return (STATUS_BUFFER_OVERFLOW);
            }

            return(STATUS_SUCCESS);
        }


        Status = NtReadFile( File,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             lpData,
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
            }
        }

        if (NT_SUCCESS( Status )) {
            *lpcbData = FileInformation.EndOfFile.LowPart;

            if (ARGUMENT_PRESENT (lpcbLen))
                *lpcbLen = FileInformation.EndOfFile.LowPart;
        }
    } // end of reading names

    NtClose (File);
    return (Status);

}


