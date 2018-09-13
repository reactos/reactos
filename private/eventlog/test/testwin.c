/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    TESTWIN.C

Abstract:

    Test program for the eventlog service. This program calls the Win
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
#include <windows.h>
#include <netevent.h>

//
// Turn on NotifyChangeEventLog
//
#define     TEST_NOTIFY         1
//#define     TEST_REMOTE         1

#define     READ_BUFFER_SIZE        1024*2      // Use 2K buffer

#define     SIZE_DATA_ARRAY         65

//
// Global buffer used to emulate "binary data" when writing an event
// record.
//
    DWORD    Data[SIZE_DATA_ARRAY];
//    LPWSTR   ServerName=L"\\\\danl2";
    LPWSTR   ServerName=NULL;


VOID
Initialize (
    VOID
    )
{
    DWORD   i;

    // Initialize the values in the data buffer.
    //
    for (i=0; i< SIZE_DATA_ARRAY; i++)
        Data[i] = i;

}


BOOL
Usage (
    VOID
    )
{
    printf( "usage: \n" );
    printf( "-b <filename>  Tests BackupEventLog API\n");
    printf( "-c             Tests ClearEventLog API\n");
    printf( "-n             Tests NotifyChangeEventlog\n");
    printf( "-rsb           Reads event log sequentially backwards\n");
    printf( "-rsf           Reads event log sequentially forwards\n");
    printf( "-rrb <record>  Reads event log from <record> backwards\n");
    printf( "-rrf <record>  Reads event log from <record> forwards\n");
    printf( "-w <count>     Tests ReportEvent API <count> times\n");
    return ERROR_INVALID_PARAMETER;

} // Usage



BOOL
WriteLogEntry ( HANDLE LogHandle, DWORD EventID )

{
#define NUM_STRINGS     2

    BOOL    Status;
    WORD    EventType, i;
    DWORD   DataSize;
    PSID    pUserSid;

    PWSTR   Strings[NUM_STRINGS] = {L"StringAOne",
                                   L"StringATwo"
                                  };

    EventType = EVENTLOG_INFORMATION_TYPE;
    pUserSid   = NULL;
    DataSize  = sizeof(DWORD) * SIZE_DATA_ARRAY;

    for (i=0; i< SIZE_DATA_ARRAY; i++)
        Data[i] += i;

    Status = ReportEventW (
                    LogHandle,
                    EventType,
                    0,            // event category
                    EventID,
                    pUserSid,
                    NUM_STRINGS,
                    DataSize,
                    Strings,
                    (PVOID)Data
                    );

    return (Status);
}

DWORD
WriteLogEntryMsg ( HANDLE LogHandle, DWORD EventID )
/*
    This function requires a registry entry in the Applications section
    of the Eventlog for TESTWINAPP, it will use the netevent.dll message file.
*/

{
#define NUM_STRINGS     2

    WORD    EventType;
    DWORD   DataSize;
    PSID    pUserSid;

    PWSTR   Strings[NUM_STRINGS];

    Strings[0] = L"This is a BOGUS message for TEST purposes Ignore this substitution text";
    Strings[1] = L"GHOST SERVICE in the long string format - I wanted a long string to pass into this function";

    EventType = EVENTLOG_INFORMATION_TYPE;
    pUserSid   = NULL;
    DataSize  = sizeof(DWORD) * SIZE_DATA_ARRAY;


    if (!ReportEventW (
                    LogHandle,
                    EventType,
                    0,            // event category
                    EVENT_SERVICE_START_FAILED_NONE,
                    pUserSid,
                    NUM_STRINGS,
                    0,              // DataSize
                    Strings,
                    (PVOID)NULL     // Data
                    )) {

        printf("ReportEventW failed %d\n",GetLastError());
        return(GetLastError());
    }
    return (NO_ERROR);
}


VOID
DisplayEventRecords( PVOID Buffer,
                     DWORD  BufSize,
                     ULONG *NumRecords)

{
    PEVENTLOGRECORD     pLogRecord;
    ANSI_STRING         StringA;
    UNICODE_STRING      StringU;
    PWSTR               pwString;
    DWORD               Count = 0;
    DWORD               Offset = 0;
    DWORD               i;

    pLogRecord = (PEVENTLOGRECORD) Buffer;

    while ((DWORD)Offset < BufSize) {

        printf("\nRecord # %lu\n", ++Count);

        printf("Length: 0x%lx TimeGenerated: 0x%lx  EventID: 0x%lx EventType: 0x%x\n",
                pLogRecord->Length, pLogRecord->TimeGenerated, pLogRecord->EventID,
                pLogRecord->EventType);

        printf("NumStrings: 0x%x StringOffset: 0x%lx UserSidLength: 0x%lx TimeWritten: 0x%lx\n",
                pLogRecord->NumStrings, pLogRecord->StringOffset,
                pLogRecord->UserSidLength, pLogRecord->TimeWritten);

        printf("UserSidOffset: 0x%lx    DataLength: 0x%lx    DataOffset:  0x%lx \n",
                pLogRecord->UserSidOffset, pLogRecord->DataLength,
                pLogRecord->DataOffset);

        //
        // Print out module name
        //
        pwString = (PWSTR)((DWORD)pLogRecord + sizeof(EVENTLOGRECORD));
        RtlInitUnicodeString (&StringU, pwString);
        RtlUnicodeStringToAnsiString (&StringA, &StringU, TRUE);

        printf("ModuleName:  %s ", StringA.Buffer);
        RtlFreeAnsiString (&StringA);

        //
        // Display ComputerName
        //
        pwString = pwString + (wcslen(pwString) + 1);

        RtlInitUnicodeString (&StringU, pwString);
        RtlUnicodeStringToAnsiString (&StringA, &StringU, TRUE);

        printf("ComputerName: %s\n",StringA.Buffer);
        RtlFreeAnsiString (&StringA);

        //
        // Display strings
        //
        pwString = (PWSTR)((DWORD)Buffer + pLogRecord->StringOffset);

        printf("\nStrings: \n");
        for (i=0; i<pLogRecord->NumStrings; i++) {

            RtlInitUnicodeString (&StringU, pwString);
            RtlUnicodeStringToAnsiString (&StringA, &StringU, TRUE);

            printf("  %s  \n",StringA.Buffer);

            RtlFreeAnsiString (&StringA);

            pwString = (PWSTR)((DWORD)pwString + StringU.MaximumLength);
        }

        // Get next record
        //
        Offset += pLogRecord->Length;

        pLogRecord = (PEVENTLOGRECORD)((DWORD)Buffer + Offset);

    }
    *NumRecords = Count;

}


BOOL
ReadFromLog ( HANDLE LogHandle,
             PVOID  Buffer,
             ULONG *pBytesRead,
             DWORD  ReadFlag,
             DWORD  Record
             )
{
    BOOL        Status;
    DWORD       MinBytesNeeded;
    DWORD       ErrorCode;

    Status = ReadEventLogW (
                        LogHandle,
                        ReadFlag,
                        Record,
                        Buffer,
                        READ_BUFFER_SIZE,
                        pBytesRead,
                        &MinBytesNeeded
                        );


    if (!Status) {
         ErrorCode = GetLastError();
         if (ErrorCode == ERROR_HANDLE_EOF) {
             Status = TRUE;
         }
         else if (ErrorCode == ERROR_NO_MORE_FILES) {
            printf("Buffer too small. Need %lu bytes min\n", MinBytesNeeded);
         }
         else {
             printf("Error from ReadEventLog %d \n", ErrorCode);
         }

    }

    return (Status);
}




BOOL
TestReadEventLog (DWORD Count, DWORD ReadFlag, DWORD Record)

{
    BOOL    bStatus,IStatus;
    DWORD   status;
    HANDLE  LogHandle;
    LPWSTR  ModuleName;
    DWORD   NumRecords, BytesReturned;
    PVOID   Buffer;
    DWORD   RecordOffset;
    DWORD   NumberOfRecords;
    DWORD   OldestRecord;

    printf("Testing ReadEventLog API to read %lu entries\n",Count);

    Buffer = malloc (READ_BUFFER_SIZE);

    //
    // Initialize the strings
    //
    NumRecords = Count;
    ModuleName = L"TESTWINAPP";

    //
    // Open the log handle
    //
    printf("OpenEventLog - ");
    LogHandle = OpenEventLogW (
                    ServerName,
                    ModuleName
                    );

    if (LogHandle == NULL) {
         printf("Error - %d\n", GetLastError());

    } else {
        printf("SUCCESS\n");

        //
        // Get and print record information
        //

        bStatus = GetNumberOfEventLogRecords(LogHandle, & NumberOfRecords);
        if (bStatus) {
           bStatus = GetOldestEventLogRecord(LogHandle, & OldestRecord);
        }

        if (!bStatus) {
           printf("Query of record information failed with %X", GetLastError());
           return(bStatus);
        }

        printf("\nThere are %d records in the file, %d is the oldest"
         " record number\n", NumberOfRecords, OldestRecord);

        RecordOffset = Record;

        printf("Reading %u records\r", Count);

        while (Count) {

            //
            // Read from the log
            //
            bStatus = ReadFromLog ( LogHandle,
                                   Buffer,
                                   &BytesReturned,
                                   ReadFlag,
                                   RecordOffset
                                 );
            if (bStatus) {
                printf("Bytes read = 0x%lx\n", BytesReturned);
                printf("Read %u records\n", NumRecords);
                DisplayEventRecords(Buffer, BytesReturned, &NumRecords);
                Count -= NumRecords;
                RecordOffset += NumRecords;
            } else {
                break;
            }

            if (BytesReturned == 0)
                break;
        }
        printf("\n");

        if (!bStatus) {
            printf ("ReadFromLog Error - %d. Remaining count %lu\n", GetLastError(),
                Count);
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling CloseEventLog\n");
        IStatus = CloseEventLog (LogHandle);
    }

    return (bStatus);
}



BOOL
TestWriteEventLog (DWORD Count)

{
    DWORD       Status=NO_ERROR;
    BOOL        IStatus;
    HANDLE      LogHandle;
    LPWSTR      ModuleName;
    DWORD       EventID = 99;
    DWORD       WriteCount;
    DWORD       DataNum=0;

    printf("Testing ReportEvent API\n");

    //
    // Initialize the strings
    //
    ModuleName = L"TESTWINAPP";

    printf("Calling RegisterEventSource for WRITE %lu times\n", Count);
    while ((Count > 0) && (Status== NO_ERROR)) {
        //
        // Open the log handle
        //
        LogHandle = RegisterEventSourceW (
                        ServerName,
                        ModuleName
                        );

        if (LogHandle == NULL) {
            Status = GetLastError();
            printf("RegisterEventSource Failure - %d\n", Status);
            return(Status);

        } else {
            printf("Registered - ");
            WriteCount = 5;
            printf("Record # %u:  ", Count);

            while ((WriteCount>0) && (Status==NO_ERROR)) {

                //
                // Write an entry into the log
                //
                Data[0] = DataNum;                     // Make data "unique"
                EventID = (EventID + DataNum) % 100;   // Vary the eventids
                Status = WriteLogEntryMsg( LogHandle, EventID );
                DataNum++;
                WriteCount--;

                if (Status != NO_ERROR) {
                    printf ("WriteLogEntry Error - %d. Remaining count %lu\n",Status,Count);
                } else {
                    printf ("%d,",WriteCount);
                }
            }

            IStatus = DeregisterEventSource (LogHandle);
            printf(" - Deregistered\n");
        }
        Count--;
    }

    printf("\n");
    return (Status);
}



BOOL
TestClearLogFile ()

{
    BOOL        Status, IStatus;
    HANDLE      LogHandle;
    LPWSTR ModuleName, BackupName;

    printf("Testing ClearLogFile API\n");
    //
    // Initialize the strings
    //
    ModuleName = L"TESTWINAPP";

    //
    // Open the log handle
    //
    printf("Calling OpenEventLog for CLEAR - ");
    LogHandle = OpenEventLogW (
                    NULL,
                    ModuleName
                    );

    if (LogHandle == NULL) {
         printf("OpenEventLog Error - %d\n", GetLastError());

    } else {
        printf("SUCCESS\n");

        //
        // Clear the log file and back it up to "view.log"
        //

        printf("Calling ClearEventLog backing up to view.log  ");
        BackupName = L"\\\\danhi386\\roote\\view.log";

        Status = ClearEventLogW (
                        LogHandle,
                        BackupName
                        );

        if (!Status) {
            printf ("ClearEventLog Error - %d\n", GetLastError());
        } else {
            printf ("SUCCESS\n");
        }

        //
        // Now just clear the file without backing it up
        //
        printf("Calling ClearEventLog with no backup  ");
        Status = ClearEventLogW (
                        LogHandle,
                        NULL
                        );

        if (!Status) {
            printf ("ClearEventLogError - %d\n", GetLastError());
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling CloseEventLog\n");
        IStatus = CloseEventLog (LogHandle);
    }

    return(Status);
}

BOOL
TestBackupLogFile (LPSTR BackupFileName)

{
    BOOL        Status, IStatus;
    HANDLE      LogHandle;
    LPWSTR ModuleName;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    printf("Testing BackupLogFile API\n");
    //
    // Initialize the strings
    //
    ModuleName = L"TESTWINAPP";

    //
    // Open the log handle
    //
    printf("Calling OpenEventLog for BACKUP - ");
    LogHandle = OpenEventLogW (
                    NULL,
                    ModuleName
                    );

    if (LogHandle == NULL) {
         printf("OpenEventLog Failure %d\n", GetLastError());

    } else {
        printf("OpenEventLog SUCCESS\n");

        //
        // Backup the log file to BackupFileName
        //

        printf("Calling BackupEventLog backing up to %s  ", BackupFileName);

    RtlInitAnsiString(&AnsiString, BackupFileName);
    RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);

    Status = BackupEventLogW (LogHandle, UnicodeString.Buffer);

        if (!Status) {
            printf ("BackupEventLog failure - %d\n", GetLastError());
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling CloseEventLog\n");
        IStatus = CloseEventLog (LogHandle);
    }

    return(Status);
}

VOID
NotifyThread(
    HANDLE  hEventLog)
{
    Sleep(30000);
    printf("NotifyThread: Writing an event...\n");
    if (!WriteLogEntryMsg(hEventLog,1)) {
        printf("NotifyThread: WriteLogEntryMsg failed\n");
    }
    else {
        printf("Event was written\n");
    }
    ExitThread(NO_ERROR);
}

VOID
TestChangeNotify(
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    HANDLE      hEvent;
    HANDLE      hThread;
    HANDLE      hEventLog;
    DWORD       threadId;
    DWORD       status;

    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (hEvent == NULL) {
        printf("CreateEvent Failed %d\n",GetLastError());
        return;
    }
#ifdef TEST_REMOTE
    hEventLog = RegisterEventSourceW(L"\\\\DANL2",L"TESTWINAPP");
#else
    hEventLog = RegisterEventSourceW(NULL,L"TESTWINAPP");
#endif
    if (hEventLog == NULL) {
        printf("OpenEventLog failed %d\n",GetLastError());
    }

#ifdef TEST_NOTIFY

    if (!NotifyChangeEventLog(hEventLog,hEvent)) {
        printf("NotifyChangeEventLog failed %d\n",GetLastError());
    }
#endif  // TEST_NOTIFY

    hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)NotifyThread,hEventLog,0,&threadId);
    if (hThread == NULL) {
        printf("CreateThread Failed %d\n",GetLastError());
        CloseHandle(hEvent);
        return;
    }

    CloseHandle(hThread);

    printf("Wait for event to become signaled\n");
    status = WaitForSingleObject(hEvent,INFINITE);
    if (status == WAIT_OBJECT_0) {
        printf("The Event was signaled\n");
    }
    else {
        printf("The Event was NOT signaled\n");
    }
    return;
}
/****************************************************************************/
DWORD __cdecl
main (
    IN SHORT argc,
    IN PSZ argv[],
    IN PSZ envp[]
    )
/*++
*
* Routine Description:
*
*
*
* Arguments:
*
*
*
*
* Return Value:
*
*
*
--*/
/****************************************************************************/
{

    DWORD   ReadFlags;

    Initialize();           // Init any data

    if ( argc < 2 ) {
        printf( "Not enough parameters\n" );
        return Usage( );
    }

    if ( stricmp( argv[1], "-c" ) == 0 ) {

        if ( argc < 3 ) {
            return TestClearLogFile();
        }
    }
    else if ( stricmp( argv[1], "-b" ) == 0 ) {

        if ( argc < 3 ) {
            printf("You must supply a filename to backup to\n");
            return(FALSE);
        }

            return TestBackupLogFile(argv[2]);

    } else if (stricmp ( argv[1], "-rsf" ) == 0 ) {

        ReadFlags = EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags,0 );
        } else  {
            return Usage();
        }
    } else if (stricmp ( argv[1], "-rsb" ) == 0 ) {

        ReadFlags = EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags,0 );
        } else  {
            return Usage();
        }
    } else if (stricmp ( argv[1], "-n" ) == 0 ) {
        TestChangeNotify();

    } else if (stricmp ( argv[1], "-rrf" ) == 0 ) {

        ReadFlags = EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags ,1);
        } else if (argc == 3) {
            return (TestReadEventLog (1, ReadFlags, atoi(argv[2])));
        }
    } else if (stricmp ( argv[1], "-rrb" ) == 0 ) {

        ReadFlags = EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags, 1);
        } else if (argc == 3) {
            return (TestReadEventLog (1, ReadFlags, atoi(argv[2])));
        }
    } else if (stricmp ( argv[1], "-w" ) == 0 ) {

        if ( argc < 3 ) {
            return TestWriteEventLog(1);
        } else if (argc == 3) {
            return (TestWriteEventLog (atoi(argv[2])));
        }

    } else {

        return Usage();
    }

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(envp);


}
