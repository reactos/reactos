/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    TEST.C

Abstract:

    Test program for the eventlog service. This program calls the Elf
    APIs to test out the operation of the service.

Author:

    Rajen Shah  (rajens) 05-Aug-1991

Revision History:


--*/
/*----------------------*/
/* INCLUDES             */
/*----------------------*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>      // printf
#include <string.h>     // stricmp
#include <stdlib.h>
#include <process.h>    // exit
#include <elfcommn.h>
#include <windows.h>
#include <ntiolog.h>
#include <malloc.h>

#define     READ_BUFFER_SIZE        1024*2      // Use 2K buffer

#define     SIZE_DATA_ARRAY         22

#define SET_OPERATION(x) \
        if (Operation != Invalid) { \
           printf("Only one operation at a time\n"); \
           Usage(); \
        } \
        Operation = x;

//
// Global buffer used to emulate "binary data" when writing an event
// record.
//
ULONG    Data[SIZE_DATA_ARRAY];
enum _OPERATION_TYPE {
   Invalid,
   Clear,
   Backup,
   Read,
   Write,
   Notify,
   TestFull,
   LPC
} Operation = Invalid;
ULONG ReadFlags;
BOOL Verbose = FALSE;
ULONG NumberofRecords = 1;
ULONG DelayInMilliseconds = 0;
CHAR DefaultModuleName[] = "TESTAPP";
PCHAR pModuleName = DefaultModuleName;
PCHAR pBackupFileName;
ANSI_STRING AnsiString;
UNICODE_STRING ServerName;
BOOL ReadingBackupFile = FALSE;
BOOL ReadingModule = FALSE;
BOOL WriteInvalidRecords = FALSE;
BOOL InvalidUser = FALSE;

// Function prototypes

VOID ParseParms(ULONG argc, PCHAR *argv);

VOID
Initialize (
    VOID
    )
{
    ULONG   i;

    // Initialize the values in the data buffer.
    //
    for (i=0; i< SIZE_DATA_ARRAY; i++)
        Data[i] = i;

}


VOID
Usage (
    VOID
    )
{
    printf( "usage: \n" );
    printf( "-c              Clears the specified log\n");
    printf( "-b <filename>   Backs up the log to file <filename>\n");
    printf( "-f <filename>   Filename of backup log to use for read\n");
    printf( "-i              Generate invalid SID\n");
    printf( "-l[i] nn        Writes nn records thru LPC port [i ==> bad records]\n");
    printf( "-m <modulename> Module name to use for read/clear\n");
    printf( "-n              Test out change notify\n");
    printf( "-rsb            Reads nn event log records sequentially backwards\n");
    printf( "-rsf nn         Reads nn event log records sequentially forwards\n");
    printf( "-rrb <record>   Reads event log from <record> backwards\n");
    printf( "-rrf <record>   Reads event log from <record> forwards\n");
    printf( "-s <servername> Name of server to remote calls to\n");
    printf( "-t nn           Number of milliseconds to delay between read/write"
            " (default 0)\n\tOnly used with -l switch\n");
    printf( "-w <count>      Writes <count> records\n");
    printf( "-z              Test to see if the logs are full\n");
    exit(0);

} // Usage


NTSTATUS
WriteLogEntry (
    HANDLE LogHandle,
    ULONG EventID
    )
{
#define NUM_STRINGS     2
#define SIZE_TOKEN_BUFFER 512

    SYSTEMTIME systime;
    NTSTATUS Status;
    USHORT   EventType, i;
    ULONG    DataSize;
    PSID     pUserSid = NULL;
    PWSTR    Strings[NUM_STRINGS] = {L"StringOne", L"StringTwo"};
    PUNICODE_STRING UStrings[NUM_STRINGS];
    HANDLE   hProcess;
    HANDLE   hToken;
    PTOKEN_USER pTokenUser;
    DWORD    SizeRequired;

    EventType = EVENTLOG_INFORMATION_TYPE;
    DataSize  = sizeof(ULONG) * SIZE_DATA_ARRAY;

    //
    // Get the SID of the current user (process)
    //

    pTokenUser = malloc(SIZE_TOKEN_BUFFER);

    if (!InvalidUser) {
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE,
            GetCurrentProcessId());
        if (!hProcess) {
            printf("Couldn't open the process, rc = %d\n", GetLastError());
            return(STATUS_UNSUCCESSFUL);
        }

        if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
            printf("Couldn't open the token, rc = %d\n", GetLastError());
            CloseHandle(hProcess);
            return(STATUS_UNSUCCESSFUL);
        }
        if (!pTokenUser) {
            printf("Couldn't allocate buffer for TokenUser\n");
            CloseHandle(hToken);
            CloseHandle(hProcess);
            return(STATUS_UNSUCCESSFUL);
        }

        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, SIZE_TOKEN_BUFFER,
            &SizeRequired)) {
                printf("Couldn't get TokenUser information, rc = %d\n",
                    GetLastError());
                CloseHandle(hToken);
                CloseHandle(hProcess);
                free(pTokenUser);
                return(STATUS_UNSUCCESSFUL);
        }

        CloseHandle(hToken);
        CloseHandle(hProcess);
    }
    else {
        memset(pTokenUser, 0xFADE, SIZE_TOKEN_BUFFER);
        pTokenUser->User.Sid = (PSID)pUserSid;
    }

    pUserSid = pTokenUser->User.Sid;

    for (i=0; i< SIZE_DATA_ARRAY; i++)
        Data[i] += i;

    // Allocate space for the unicode strings in the array, and
    // copy over the strings from Strings[] to that array.
    //
    for (i=0; i<NUM_STRINGS; i++) {

        UStrings[i] = malloc(sizeof(UNICODE_STRING));
        RtlInitUnicodeString (UStrings[i], Strings[i]);
        UStrings[i]->MaximumLength = UStrings[i]->Length + sizeof(WCHAR);
    }

    //
    // Vary the data sizes.
    //

    GetLocalTime(&systime);

    DataSize = systime.wMilliseconds % sizeof(Data);
    printf("\nData Size = %lu\n", DataSize);

    Status = ElfReportEventW (
                    LogHandle,
                    EventType,
                    0,             // category
                    EventID,
                    pUserSid,
                    NUM_STRINGS,
                    DataSize,
                    UStrings,
                    (PVOID)Data,
                    0,              // Flags        -  paired event support
                    NULL,           // RecordNumber  | not in product 1
                    NULL            // TimeWritten  -
                    );

    for (i=0; i<NUM_STRINGS; i++)
        free(UStrings[i]);

    free(pTokenUser);
    return (Status);
}


VOID
DisplayEventRecords( PVOID Buffer,
                     ULONG  BufSize,
                     PULONG NumRecords)

{
    PEVENTLOGRECORD     pLogRecord;
    LPWSTR              pwString;
    ULONG               Count = 0;
    ULONG               Offset = 0;
    ULONG               i;

    pLogRecord = (PEVENTLOGRECORD) Buffer;

    while (Offset < BufSize && Count < *NumRecords) {

        printf("\nRecord # %lu\n", pLogRecord->RecordNumber);

        printf("Length: 0x%lx TimeGenerated: 0x%lx  EventID: 0x%lx EventType: 0x%x\n",
                pLogRecord->Length, pLogRecord->TimeGenerated, pLogRecord->EventID,
                pLogRecord->EventType);

        printf("NumStrings: 0x%x StringOffset: 0x%lx UserSidLength: 0x%lx TimeWritten: 0x%lx\n",
                pLogRecord->NumStrings, pLogRecord->StringOffset,
                pLogRecord->UserSidLength, pLogRecord->TimeWritten);

        printf("UserSidOffset: 0x%lx DataLength: 0x%lx DataOffset: 0x%lx Category: 0x%lx\n",
                pLogRecord->UserSidOffset, pLogRecord->DataLength,
                pLogRecord->DataOffset, pLogRecord->EventCategory);

        //
        // Print out module name
        //

        pwString = (PWSTR)((LPBYTE) pLogRecord + sizeof(EVENTLOGRECORD));
        printf("ModuleName: %ws\n", pwString);

        //
        // Display ComputerName
        //
        pwString += wcslen(pwString) + 1;
        printf("ComputerName: %ws\n", pwString);

        //
        // Display strings
        //

        pwString = (PWSTR)((LPBYTE)pLogRecord + pLogRecord->StringOffset);

        printf("Strings: ");
        for (i=0; i<pLogRecord->NumStrings; i++) {

            printf("  %ws  ", pwString);
            pwString += wcslen(pwString) + 1;
        }

        printf("\n");

        //
        // If verbose mode, display binary data (up to 256 bytes)
        // BUGBUG - this code will hit an alignment fault on mips.
        //

        if (Verbose) {
            PULONG pData;
            PULONG pEnd;

            if (pLogRecord->DataLength < 80) {
                pEnd = (PULONG)((PBYTE) pLogRecord + pLogRecord->DataOffset +
                    pLogRecord->DataLength);
            }
            else {
                pEnd = (PULONG)((PBYTE) pLogRecord + pLogRecord->DataOffset +
                    256);
            }

            printf("Data: \n\n");
            for (pData = (PULONG)((PBYTE) pLogRecord + pLogRecord->DataOffset);
                 pData < pEnd; (PBYTE) pData += 32) {

                printf("\t%08x %08x %08x %08x\n", pData[0], pData[1], pData[2],
                    pData[3]);
            }
        }

        // Get next record
        //
        Offset += pLogRecord->Length;

        pLogRecord = (PEVENTLOGRECORD)((ULONG)Buffer + Offset);

        Count++;

    }

    *NumRecords = Count;

}


NTSTATUS
ReadFromLog ( HANDLE LogHandle,
             PVOID  Buffer,
             PULONG pBytesRead,
             ULONG  ReadFlag,
             ULONG  Record
             )
{
    NTSTATUS    Status;
    ULONG       MinBytesNeeded;

    Status = ElfReadEventLogW (
                        LogHandle,
                        ReadFlag,
                        Record,
                        Buffer,
                        READ_BUFFER_SIZE,
                        pBytesRead,
                        &MinBytesNeeded
                        );


    if (Status == STATUS_BUFFER_TOO_SMALL)
        printf("Buffer too small. Need %lu bytes min\n", MinBytesNeeded);

    return (Status);
}


NTSTATUS
TestReadEventLog (
    ULONG Count,
    ULONG ReadFlag,
    ULONG Record
    )

{
    NTSTATUS    Status, IStatus;

    HANDLE      LogHandle;
    UNICODE_STRING  ModuleNameU;
    ANSI_STRING ModuleNameA;
    ULONG   NumRecords, BytesReturned;
    PVOID   Buffer;
    ULONG   RecordOffset;
    ULONG   NumberOfRecords;
    ULONG   OldestRecord;

    printf("Testing ElfReadEventLog API to read %lu entries\n",Count);

    Buffer = malloc (READ_BUFFER_SIZE);

    //
    // Initialize the strings
    //
    NumRecords = Count;
    RtlInitAnsiString(&ModuleNameA, pModuleName);
    RtlAnsiStringToUnicodeString(&ModuleNameU, &ModuleNameA, TRUE);
    ModuleNameU.MaximumLength = ModuleNameU.Length + sizeof(WCHAR);

    //
    // Open the log handle
    //

    if (ReadingBackupFile) {
        printf("ElfOpenBackupEventLog - ");
        Status = ElfOpenBackupEventLogW (
                        &ServerName,
                        &ModuleNameU,
                        &LogHandle
                        );
    }
    else {
        printf("ElfOpenEventLog - ");
        Status = ElfOpenEventLogW (
                        &ServerName,
                        &ModuleNameU,
                        &LogHandle
                        );
    }

    if (!NT_SUCCESS(Status)) {
         printf("Error - 0x%lx\n", Status);

    } else {
        printf("SUCCESS\n");

        //
        // Get and print record information
        //

        Status = ElfNumberOfRecords(LogHandle, & NumberOfRecords);
        if (NT_SUCCESS(Status)) {
           Status = ElfOldestRecord(LogHandle, & OldestRecord);
        }

        if (!NT_SUCCESS(Status)) {
           printf("Query of record information failed with %X", Status);
           return(Status);
        }

        printf("\nThere are %d records in the file, %d is the oldest"
         " record number\n", NumberOfRecords, OldestRecord);

        RecordOffset = Record;

        while (Count && NT_SUCCESS(Status)) {

            printf("Read %u records\n", NumRecords);
            //
            // Read from the log
            //
            Status = ReadFromLog ( LogHandle,
                                   Buffer,
                                   &BytesReturned,
                                   ReadFlag,
                                   RecordOffset
                                 );
            if (NT_SUCCESS(Status)) {

                printf("Bytes read = 0x%lx\n", BytesReturned);
                NumRecords = Count;
                DisplayEventRecords(Buffer, BytesReturned, &NumRecords);
                Count -= NumRecords;
            }

        }
        printf("\n");

        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_END_OF_FILE) {
               printf("Tried to read more records than in log file\n");
            }
            else {
                printf ("Error - 0x%lx. Remaining count %lu\n", Status, Count);
            }
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling ElfCloseEventLog\n");
        IStatus = ElfCloseEventLog (LogHandle);
    }

    return (Status);
}


NTSTATUS
TestReportEvent (
    ULONG Count
    )

{
    NTSTATUS    Status, IStatus;
    HANDLE      LogHandle;
    UNICODE_STRING  ModuleNameU;
    ANSI_STRING ModuleNameA;
    ULONG EventID = 99;

    printf("Testing ElfReportEvent API\n");

    //
    // Initialize the strings
    //

    RtlInitAnsiString(&ModuleNameA, pModuleName);
    RtlAnsiStringToUnicodeString(&ModuleNameU, &ModuleNameA, TRUE);
    ModuleNameU.MaximumLength = ModuleNameU.Length + sizeof(WCHAR);

    //
    // Open the log handle
    //
    printf("Calling ElfRegisterEventSource for WRITE %lu times - ", Count);
    Status = ElfRegisterEventSourceW (
                    &ServerName,
                    &ModuleNameU,
                    &LogHandle
                    );

    if (!NT_SUCCESS(Status)) {
         printf("Error - 0x%lx\n", Status);

    } else {
        printf("SUCCESS\n");

        while (Count && NT_SUCCESS(Status)) {

            printf("Record # %u \n", Count);

            //
            // Write an entry into the log
            //
            Data[0] = Count;                        // Make data "unique"
            EventID = (EventID + Count) % 100;      // Vary the eventids
            Status = WriteLogEntry ( LogHandle, EventID );
            Count--;
        }
        printf("\n");

        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_LOG_FILE_FULL) {
                printf("Log Full\n");
            }
            else {
                printf ("Error - 0x%lx. Remaining count %lu\n", Status, Count);
            }
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling ElfDeregisterEventSource\n");
        IStatus = ElfDeregisterEventSource (LogHandle);
    }

    return (Status);
}


NTSTATUS
TestElfClearLogFile(
    VOID
    )

{
    NTSTATUS    Status, IStatus;
    HANDLE      LogHandle;
    UNICODE_STRING  BackupU, ModuleNameU;
    ANSI_STRING ModuleNameA;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE  ClearHandle;
    FILE_DISPOSITION_INFORMATION DeleteInfo = {TRUE};
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN DontRetry = FALSE;

    printf("Testing ElfClearLogFile API\n");
    //
    // Initialize the strings
    //
    RtlInitAnsiString( &ModuleNameA, pModuleName);
    RtlAnsiStringToUnicodeString(&ModuleNameU, &ModuleNameA, TRUE);
    ModuleNameU.MaximumLength = ModuleNameU.Length + sizeof(WCHAR);

    //
    // Open the log handle
    //
    printf("Calling ElfOpenEventLog for CLEAR - ");
    Status = ElfOpenEventLogW (
                    &ServerName,
                    &ModuleNameU,
                    &LogHandle
                    );

    if (!NT_SUCCESS(Status)) {
         printf("Error - 0x%lx\n", Status);

    } else {
        printf("SUCCESS\n");

        //
        // Clear the log file and back it up to "view.evt"
        //

        RtlInitUnicodeString( &BackupU,
            L"\\SystemRoot\\System32\\Config\\view.evt" );
        BackupU.MaximumLength = BackupU.Length + sizeof(WCHAR);
retry:
        printf("Calling ElfClearEventLogFile backing up to view.evt  ");
        Status = ElfClearEventLogFileW (
                        LogHandle,
                        &BackupU
                        );

        if (Status == STATUS_OBJECT_NAME_COLLISION) {
            if (DontRetry) {
                printf("Still can't backup to View.Evt\n");
            }
            else {
                printf("Failed.\nView.Evt already exists, deleting ...\n");

                //
                // Open the file with delete access
                //

                InitializeObjectAttributes(
                                &ObjectAttributes,
                                &BackupU,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

                Status = NtOpenFile(&ClearHandle,
                                    GENERIC_READ | DELETE | SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &IoStatusBlock,
                                    FILE_SHARE_DELETE,
                                    FILE_SYNCHRONOUS_IO_NONALERT
                                    );

                Status = NtSetInformationFile(
                            ClearHandle,
                            &IoStatusBlock,
                            &DeleteInfo,
                            sizeof(DeleteInfo),
                            FileDispositionInformation
                            );

                if (NT_SUCCESS (Status) ) {
                    Status = NtClose (ClearHandle);    // Discard status
                    goto retry;
                }

                printf("Delete failed 0x%lx\n",Status);
                Status = NtClose (ClearHandle);    // Discard status
                goto JustClear;
            }
        }

        if (!NT_SUCCESS(Status)) {
            printf ("Error - 0x%lx\n", Status);
        } else {
            printf ("SUCCESS\n");
        }

JustClear:

        //
        // Now just clear the file without backing it up
        //
        printf("Calling ElfClearEventLogFile with no backup  ");
        Status = ElfClearEventLogFileW (
                        LogHandle,
                        NULL
                        );

        if (!NT_SUCCESS(Status)) {
            printf ("Error - 0x%lx\n", Status);
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling ElfCloseEventLog\n");
        IStatus = ElfCloseEventLog (LogHandle);
    }

    return(Status);
}


NTSTATUS
TestElfBackupLogFile(
    VOID
    )

{
    NTSTATUS    Status, IStatus;
    HANDLE      LogHandle;
    UNICODE_STRING  BackupU, ModuleNameU;
    ANSI_STRING AnsiString;

    printf("Testing ElfBackupLogFile API\n");

    //
    // Initialize the strings
    //

    RtlInitAnsiString( &AnsiString, pModuleName);
    RtlAnsiStringToUnicodeString(&ModuleNameU, &AnsiString, TRUE);
    ModuleNameU.MaximumLength = ModuleNameU.Length + sizeof(WCHAR);

    //
    // Open the log handle
    //

    printf("Calling ElfOpenEventLog for BACKUP - ");
    Status = ElfOpenEventLogW (
                    &ServerName,
                    &ModuleNameU,
                    &LogHandle
                    );

    if (!NT_SUCCESS(Status)) {
         printf("Error - 0x%lx\n", Status);

    } else {
        printf("SUCCESS\n");

        //
        // Backup the log file
        //

        printf("Calling ElfBackupEventLogFile backing up to %s ",
            pBackupFileName);

        RtlInitAnsiString( &AnsiString, pBackupFileName);
        RtlAnsiStringToUnicodeString(&BackupU, &AnsiString, TRUE);
        BackupU.MaximumLength = BackupU.Length + sizeof(WCHAR);

        Status = ElfBackupEventLogFileW (
                        LogHandle,
                        &BackupU
                        );

        if (!NT_SUCCESS(Status)) {
            printf ("Error - 0x%lx\n", Status);
        } else {
            printf ("SUCCESS\n");
        }


        printf("Calling ElfCloseEventLog - ");
        IStatus = ElfCloseEventLog (LogHandle);
        if (NT_SUCCESS(IStatus)) {
            printf("Success\n");
        }
        else {
            printf("Failed with code %X\n", IStatus);
        }
    }

    return(Status);
}

#define DRIVER_NAME L"FLOPPY"
#define DEVICE_NAME L"A:"
#define STRING L"Test String"

// These include the NULL terminator, but is length in chars, not bytes
#define DRIVER_NAME_LENGTH 7
#define DEVICE_NAME_LENGTH 3
#define STRING_LENGTH 12

#define NUMBER_OF_DATA_BYTES 8

VOID
TestLPCWrite(
   DWORD NumberOfRecords,
   DWORD MillisecondsToDelay
   )
{

    HANDLE PortHandle;
    UNICODE_STRING PortName;
    NTSTATUS Status;
    SECURITY_QUALITY_OF_SERVICE Qos;
    PIO_ERROR_LOG_MESSAGE pIoErrorLogMessage;
    DWORD i;
    LPWSTR pDestinationString;
    PPORT_MESSAGE RequestMessage;
    PORT_MESSAGE ReplyMessage;
    WORD DataLength;
    WORD TotalLength;
    INT YorN;
    CHAR NumberString[8];
    ULONG MessageId = 1;
    DWORD BadType = 0;

    //
    // Warn the user about how this test works
    //

    printf("\nThis test doesn't end!  It will write a number of\n"
           "records, then prompt you to write more.  This is \n"
           "required since it is simulating the system thread\n"
           "which never shuts down it's connection\n\n"
           "Do you wish to continue with this test (y or n)? ");

    YorN = getc(stdin);

    if (YorN == 'n' || YorN == 'N') {
        return;
    }

    //
    // Initialize the SecurityQualityofService structure
    //

    Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Qos.ImpersonationLevel = SecurityImpersonation;
    Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    Qos.EffectiveOnly = TRUE;

    //
    // Connect to the LPC Port
    //

    RtlInitUnicodeString( &PortName, L"\\ErrorLogPort" );

    Status = NtConnectPort(& PortHandle,
                           & PortName,
                           & Qos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                           );

    if (!NT_SUCCESS(Status)) {
       printf("Connect to the LPC port failed with RC %X\n", Status);
       return;
    }

    //
    // Allocate the memory for the Message to send to the LPC port.  It
    // will contain a PORT_MESSAGE followed by an IO_ERROR_LOG_MESSAGE
    // followed by Drivername and Devicename UNICODE strings
    //

    DataLength =  PORT_MAXIMUM_MESSAGE_LENGTH -
        (sizeof(IO_ERROR_LOG_MESSAGE)
        + DRIVER_NAME_LENGTH  * sizeof(WCHAR)
        + DEVICE_NAME_LENGTH * sizeof(WCHAR)
        + STRING_LENGTH * sizeof(WCHAR));
    TotalLength = PORT_MAXIMUM_MESSAGE_LENGTH + (WORD) sizeof(PORT_MESSAGE);

    RequestMessage = (PPORT_MESSAGE) malloc(TotalLength);
    if (RequestMessage == NULL) {
       printf("Couldn't alloc %d bytes of memory for message\n", TotalLength);
       NtClose(PortHandle);
       return;
    }

    pIoErrorLogMessage = (PIO_ERROR_LOG_MESSAGE) ((LPBYTE) RequestMessage +
        sizeof(PORT_MESSAGE));

    //
    // Initialize the PORT_MESSAGE
    //

    RequestMessage->u1.s1.DataLength = PORT_MAXIMUM_MESSAGE_LENGTH;
    RequestMessage->u1.s1.TotalLength = TotalLength;
    RequestMessage->u2.s2.Type = 0;
    RequestMessage->u2.ZeroInit = 0;
    RequestMessage->ClientId.UniqueProcess = GetCurrentProcess();
    RequestMessage->ClientId.UniqueThread = GetCurrentThread();
    RequestMessage->MessageId = 0x1234;

    //
    // Initialize the IO_ERROR_LOG_MESSAGE
    //

    pIoErrorLogMessage->Type = IO_TYPE_ERROR_MESSAGE;
    pIoErrorLogMessage->Size = PORT_MAXIMUM_MESSAGE_LENGTH;
    pIoErrorLogMessage->DriverNameLength = DRIVER_NAME_LENGTH * sizeof(WCHAR);
    NtQuerySystemTime((PTIME) &pIoErrorLogMessage->TimeStamp);
    pIoErrorLogMessage->DriverNameOffset = sizeof(IO_ERROR_LOG_MESSAGE) +
        DataLength - sizeof(DWORD);

    pIoErrorLogMessage->EntryData.MajorFunctionCode = 1;
    pIoErrorLogMessage->EntryData.RetryCount = 5;
    pIoErrorLogMessage->EntryData.DumpDataSize = DataLength;
    pIoErrorLogMessage->EntryData.NumberOfStrings = 2;
    pIoErrorLogMessage->EntryData.StringOffset = sizeof(IO_ERROR_LOG_MESSAGE)
        - sizeof(DWORD) + DataLength +
        DRIVER_NAME_LENGTH * sizeof(WCHAR);
    pIoErrorLogMessage->EntryData.EventCategory = 0;
    pIoErrorLogMessage->EntryData.ErrorCode = 0xC0020008;
    pIoErrorLogMessage->EntryData.UniqueErrorValue = 0x20008;
    pIoErrorLogMessage->EntryData.FinalStatus = 0x1111;
    pIoErrorLogMessage->EntryData.SequenceNumber = 1;
    pIoErrorLogMessage->EntryData.IoControlCode = 0xFF;
    pIoErrorLogMessage->EntryData.DeviceOffset =
        RtlConvertUlongToLargeInteger(1);

    for (i = 0; i < DataLength ; i++ ) {
        pIoErrorLogMessage->EntryData.DumpData[i] = i;
    }

    //
    // Copy the strings
    //

    pDestinationString = (LPWSTR) ((LPBYTE) pIoErrorLogMessage
        + sizeof(IO_ERROR_LOG_MESSAGE)
        - sizeof(DWORD) + pIoErrorLogMessage->EntryData.DumpDataSize);
    wcscpy(pDestinationString, DRIVER_NAME);

    pDestinationString += DRIVER_NAME_LENGTH;
    wcscpy(pDestinationString, DEVICE_NAME);

    pDestinationString += DEVICE_NAME_LENGTH;
    wcscpy(pDestinationString, STRING);

    //
    // Write the packet as many times as requested, with delay, then ask
    // if they want to write more
    //
    while (NumberOfRecords) {

        printf("\n\nWriting %d records\n", NumberOfRecords);

        while(NumberOfRecords--) {
            printf(".");

            //
            // Put in a unique message number
            //

            RequestMessage->MessageId = MessageId++;

            //
            // If they want invalid records, give them invalid records
            //

            if (WriteInvalidRecords) {
                switch (BadType++) {
                case 0:
                    pIoErrorLogMessage->EntryData.DumpDataSize++;
                    break;

                case 1:
                    pIoErrorLogMessage->EntryData.NumberOfStrings++;
                    break;

                case 2:
                    pIoErrorLogMessage->EntryData.StringOffset++;
                    break;

                default:
                    BadType = 0;
                }
            }

            Status = NtRequestWaitReplyPort(PortHandle,
                                        RequestMessage,
                                        & ReplyMessage);

            if (!NT_SUCCESS(Status)) {
                printf("Request to LPC port failed with RC %X\n", Status);
                break;
            }

            //
            // Delay a little bit, if requested
            //

            if (MillisecondsToDelay) {
                Sleep(MillisecondsToDelay);
            }
        }
        printf("\nEnter the number of records to write ");

        while (!gets(NumberString) || !(NumberOfRecords = atoi(NumberString))) {
            printf("Enter the number of records to write ");
        }
    }

    //
    // Clean up and exit
    //

    Status = NtClose(PortHandle);
    if (!NT_SUCCESS(Status)) {
       printf("Close of Port failed with RC %X\n", Status);
    }

    free(RequestMessage);

    return;

}


VOID
TestChangeNotify(
   VOID
   )
{

    HANDLE Event;
    UNICODE_STRING  ModuleNameU;
    ANSI_STRING ModuleNameA;
    NTSTATUS Status;
    HANDLE LogHandle;
    OBJECT_ATTRIBUTES obja;
    ULONG NumRecords;
    ULONG BytesRead;
    ULONG MinBytesNeeded;
    PVOID Buffer;
    ULONG OldestRecord;
    ULONG NumberOfRecords;

    RtlInitAnsiString(&ModuleNameA, pModuleName);
    RtlAnsiStringToUnicodeString(&ModuleNameU, &ModuleNameA, TRUE);
    ModuleNameU.MaximumLength = ModuleNameU.Length + sizeof(WCHAR);

    Buffer = malloc (READ_BUFFER_SIZE);
    ASSERT(Buffer);

    //
    // Open the log handle
    //

    printf("ElfOpenEventLog - ");
    Status = ElfOpenEventLogW (
                    &ServerName,
                    &ModuleNameU,
                    &LogHandle
                    );

    if (!NT_SUCCESS(Status)) {
         printf("Error - 0x%lx\n", Status);
         return;
    }

    printf("SUCCESS\n");

    //
    // Create the Event
    //

    InitializeObjectAttributes( &obja, NULL, 0, NULL, NULL);

    Status = NtCreateEvent(
                   &Event,
                   SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                   &obja,
                   SynchronizationEvent,
                   FALSE
                   );

    ASSERT(NT_SUCCESS(Status));

    //
    // Get the read pointer to the end of the log
    //

    Status = ElfOldestRecord(LogHandle, & OldestRecord);
    ASSERT(NT_SUCCESS(Status));
    Status = ElfNumberOfRecords(LogHandle, & NumberOfRecords);
    ASSERT(NT_SUCCESS(Status));
    OldestRecord += NumberOfRecords - 1;

    Status = ElfReadEventLogW (
                        LogHandle,
                        EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
                        OldestRecord,
                        Buffer,
                        READ_BUFFER_SIZE,
                        &BytesRead,
                        &MinBytesNeeded
                        );


    //
    // This one should hit end of file
    //

    Status = ElfReadEventLogW (
                        LogHandle,
                        EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0,
                        Buffer,
                        READ_BUFFER_SIZE,
                        &BytesRead,
                        &MinBytesNeeded
                        );

    if (Status != STATUS_END_OF_FILE) {
        printf("Hmmm, should have hit EOF (unless there are writes going"
            " on elsewhere- %X\n", Status);
    }

    //
    // Call ElfChangeNotify
    //

    Status = ElfChangeNotify(LogHandle, Event);
    ASSERT(NT_SUCCESS(Status));

    //
    // Now loop waiting for the event to get toggled
    //

    while (1) {

        Status = NtWaitForSingleObject(Event, FALSE, 0);
        printf("The change notify event just got kicked\n");

        //
        // Now read the new records
        //

        while(1) {

            Status = ElfReadEventLogW (
                                LogHandle,
                                EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                                0,
                                Buffer,
                                READ_BUFFER_SIZE,
                                &BytesRead,
                                &MinBytesNeeded
                                );

            if (Status == STATUS_END_OF_FILE) {
                break;
            }

            NumRecords = 0xffff; // should be plenty
            DisplayEventRecords (Buffer, BytesRead, &NumRecords);
        }
    }
}


VOID
TestLogFull(
    VOID
    )
{
    HANDLE  hLogFile;
    BOOL    fIsFull;
    BOOLEAN fPrevious = FALSE;
    DWORD   i;
    DWORD   dwBytesNeeded;
    BOOL    fIsSecLog;
    
    LPWSTR  szLogNames[] = { L"Application", L"Security", L"System" };

    for (i = 0; i < sizeof(szLogNames) / sizeof(LPWSTR); i++) {

        fIsSecLog = (wcscmp(szLogNames[i], L"Security") == 0);

        if (fIsSecLog) {

            if (!NT_SUCCESS(RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                                               TRUE,
                                               FALSE,
                                               &fPrevious))) {

                printf("RtlAdjustPrivilege FAILED %d\n",
                       RtlNtStatusToDosError(GetLastError()));
            }
        }

        hLogFile = OpenEventLogW(NULL, szLogNames[i]);

        if (hLogFile != NULL) {

            if (GetEventLogInformation(hLogFile,
                                       0,          // Log full infolevel
                                       (LPBYTE)&fIsFull,
                                       sizeof(fIsFull),
                                       &dwBytesNeeded)) {

                printf("The %ws Log is%sfull\n",
                       szLogNames[i],
                       fIsFull ? " " : " not ");
            }
            else {

                printf("GetEventLogInformation FAILED %d for the %ws Log\n",
                       GetLastError(),
                       szLogNames[i]);
            }            
        }
        else {

            printf("OpenEventLog FAILED %d for the %ws Log\n",
                   GetLastError(),
                   szLogNames[i]);
        }

        if (fIsSecLog) {
            RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, fPrevious, FALSE, &fPrevious);
        }
    }
}


VOID
__cdecl
main (
    IN SHORT argc,
    IN PSZ argv[]
    )
{

    Initialize();           // Init any data

    //
    // Parse the command line
    //

    ParseParms(argc, argv);

    switch (Operation) {
       case Clear:

          TestElfClearLogFile();
          break;

       case Backup:

          TestElfBackupLogFile();
          break;

       case Read:

          if (ReadFlags & EVENTLOG_SEEK_READ) {
              TestReadEventLog(1, ReadFlags, NumberofRecords) ;
          }
          else {
              TestReadEventLog(NumberofRecords, ReadFlags, 0) ;
          }
          break;

       case Write:

          TestReportEvent (NumberofRecords);
          break;

        case LPC:
          TestLPCWrite(NumberofRecords, DelayInMilliseconds);
          break;

        case Notify:
          TestChangeNotify();
          break;

        case TestFull:
          TestLogFull();
          break;

        default:
            printf("Invalid switch from ParseParms %d\n", Operation);
    }
}


VOID
ParseParms(
    ULONG argc,
    PCHAR *argv
    )
{

   ULONG i;
   PCHAR pch;

   for (i = 1; i < argc; i++) {    /* for each argument */
       if (*(pch = argv[i]) == '-') {
           while (*++pch) {
               switch (*pch) {
                   case 'b':

                     SET_OPERATION(Backup)

                     //
                     // Get the file name for backup
                     //

                     if (i+1 < argc) {
                        pBackupFileName = argv[++i];
                     }
                     else {
                        Usage();
                     }
                     break;

                   case 'c':

                     SET_OPERATION(Clear)

                     break;

                  case 'f':
                     if (i+1 < argc) {
                        pModuleName = argv[++i];
                        ReadingBackupFile = TRUE;
                     }
                     else {
                        Usage();
                     }
                     break;

                  case '?':
                  case 'h':
                  case 'H':
                     Usage();
                     break;

                  case 'i':
                     InvalidUser = TRUE;
                     break;

                  case 'l':

                     SET_OPERATION(LPC);

                     //
                     // See if they want invalid records
                     //

                     if (*++pch == 'i') {
                         WriteInvalidRecords = TRUE;
                     }

                     //
                     // See if they specified a number of records
                     //

                     if (i + 1 < argc && argv[i+1][0] != '-') {
                        NumberofRecords = atoi(argv[++i]);
                        if (NumberofRecords == 0) {
                           Usage();
                        }
                     }

                     break;

                  case 'm':
                     if (i+1 < argc) {
                        pModuleName = argv[++i];
                        ReadingModule = TRUE;
                     }
                     else {
                        Usage();
                     }
                     break;

                  case 'n':
                     SET_OPERATION(Notify)
                     break;

                   case 'r':

                     SET_OPERATION(Read)

                     //
                     // Different Read options
                     //

                     if (*++pch == 's') {
                        ReadFlags |= EVENTLOG_SEQUENTIAL_READ;
                     }
                     else if (*pch == 'r') {
                        ReadFlags |= EVENTLOG_SEEK_READ;
                     }
                     else {
                        Usage();
                     }

                     if (*++pch == 'f') {
                        ReadFlags |= EVENTLOG_FORWARDS_READ;
                     }
                     else if (*pch == 'b') {
                        ReadFlags |= EVENTLOG_BACKWARDS_READ;
                     }
                     else {
                        Usage();
                     }

                     //
                     // See if they specified a number of records
                     //

                     if (i + 1 < argc && argv[i+1][0] != '-') {
                        NumberofRecords = atoi(argv[++i]);
                        if (NumberofRecords == 0) {
                           Usage();
                        }
                     }

                     break;

                  case 's':
                     if (i+1 >= argc) {
                         printf("Must supply a server name with -s\n");
                         Usage();
                     }
                     RtlInitAnsiString(&AnsiString, argv[++i]);
                     RtlAnsiStringToUnicodeString(&ServerName, &AnsiString,
                        TRUE);
                     break;

                  case 't':
                     DelayInMilliseconds = atoi(argv[++i]);
                     break;

                  case 'v':
                     Verbose = TRUE;
                     break;

                  case 'w':

                     SET_OPERATION(Write)

                     //
                     // See if they specified a number of records
                     //

                     if (i + 1 < argc && argv[i+1][0] != '-') {
                        NumberofRecords = atoi(argv[++i]);
                        if (NumberofRecords == 0) {
                           Usage();
                        }
                     }

                     break;

                  case 'z':

                      SET_OPERATION(TestFull)
                      break;

                  default:        /* Invalid options */
                     printf("Invalid option %c\n\n", *pch);
                     Usage();
                     break;
               }
           }
       }

       //
       // There aren't any non switch parms
       //

       else {
          Usage();
       }
   }

   //
   // Verify parms are correct
   //


   if ( Operation == Invalid) {
       printf( "Must specify an operation\n");
       Usage( );
   }

   if (ReadingBackupFile && ReadingModule) {
       printf("-m and -f are mutually exclusive\n");
       Usage();
   }

   if (ReadingBackupFile && Operation == Write) {
       printf("You cannot write to a backup log file\n");
       Usage();
   }
   if (DelayInMilliseconds && Operation != LPC) {
       printf("\n\n-t switch is only used with -l\n\n");
   }
}

