/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    elfexts.c

Abstract:

    This function contains the eventlog ntsd debugger extensions

Author:

    Dan Hinsley (DanHi) 22-May-1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <elf.h>
#include <elfdef.h>
#include <elfcommn.h>
#include <elfproto.h>
#include <svcs.h>
#include <elfextrn.h>

//#define DbgPrint(_x_) (lpOutputRoutine) _x_
#define DbgPrint(_x_)
#define MAX_NAME 256
#define printf (lpOutputRoutine)
#define GET_DATA(DebugeeAddr, LocalAddr, Length) \
    Status = ReadProcessMemory(                  \
                GlobalhCurrentProcess,           \
                (LPVOID)DebugeeAddr,             \
                LocalAddr,                       \
                Length,                          \
                NULL                             \
                );

PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;

HANDLE GlobalhCurrentProcess;
BOOL Status;

//
// Initialize the global function pointers
//

VOID
InitFunctionPointers(
    HANDLE hCurrentProcess,
    PNTSD_EXTENSION_APIS lpExtensionApis
    )
{
    //
    // Load these to speed access if we haven't already
    //

    if (!lpOutputRoutine) {
        lpOutputRoutine = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;
    }

    //
    // Stick this in a global
    //

    GlobalhCurrentProcess = hCurrentProcess;
}

LPWSTR
GetUnicodeString(
    PUNICODE_STRING pUnicodeString
    )
{
    DWORD Pointer;
    UNICODE_STRING UnicodeString;

    GET_DATA(pUnicodeString, &UnicodeString, sizeof(UNICODE_STRING))
    Pointer = (DWORD) UnicodeString.Buffer;
    UnicodeString.Buffer = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
        UnicodeString.Length + sizeof(WCHAR));
    GET_DATA(Pointer, UnicodeString.Buffer, UnicodeString.Length)

    return(UnicodeString.Buffer);
}

DWORD
GetLogFileAddress(
    LPSTR LogFileName,
    PLOGFILE LogFile
    )
{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    DWORD Pointer;
    DWORD LogFileAnchor;
    LPWSTR ModuleName;

    //
    // Convert the string to UNICODE
    //

    RtlInitAnsiString(&AnsiString, LogFileName);
    RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);

    //
    // Walk the logfile list looking for a match
    //

    LogFileAnchor = (lpGetExpressionRoutine)("LogFilesHead");

    GET_DATA(LogFileAnchor, &Pointer, sizeof(DWORD))

    while (Pointer != LogFileAnchor) {
        GET_DATA(Pointer, LogFile, sizeof(LOGFILE))
        ModuleName = GetUnicodeString(LogFile->LogModuleName);
        if (!wcsicmp(ModuleName, UnicodeString.Buffer)) {
            break;
        }
        LocalFree(ModuleName);
        Pointer = (DWORD) LogFile->FileList.Flink;
    }

    RtlFreeUnicodeString(&UnicodeString);

    if (Pointer == LogFileAnchor) {
        return(0);
    }
    else {
        LocalFree(ModuleName);
        return(Pointer);
    }
}

//
// Dump an individual record
//

DWORD
DumpRecord(
    DWORD Record,
    DWORD RecordNumber,
    DWORD StartOfFile,
    DWORD EndOfFile
    )
{
    DWORD BufferLen;
    PCHAR TimeBuffer;
    PEVENTLOGRECORD EventLogRecord;
    LPWSTR Module;
    LPWSTR Computer;
    DWORD FirstPiece = 0;

    GET_DATA(Record, &BufferLen, sizeof(DWORD))

    //
    // See if it's a ELF_SKIP_DWORD, and if it is, return the top of the
    // file
    //

    if (BufferLen == ELF_SKIP_DWORD) {
        return(StartOfFile + sizeof(ELF_LOGFILE_HEADER));
    }

    //
    // See if it's the EOF record
    //

    if (BufferLen == ELFEOFRECORDSIZE) {
        return(0);
    }

    BufferLen += sizeof(DWORD); // get room for length of next record
    EventLogRecord = (PEVENTLOGRECORD) LocalAlloc(LMEM_ZEROINIT, BufferLen);

    //
    // If the record wraps, grab it piecemeal
    //

    if (EndOfFile && BufferLen + Record > EndOfFile) {
        FirstPiece = EndOfFile - Record;
        GET_DATA(Record, EventLogRecord, FirstPiece);
        GET_DATA((StartOfFile + sizeof(ELF_LOGFILE_HEADER)),
            ((PBYTE) EventLogRecord + FirstPiece), BufferLen - FirstPiece)
    }
    else {
        GET_DATA(Record, EventLogRecord, BufferLen)
    }

    //
    // If it's greater than the starting record, print it out
    //

    if (EventLogRecord->RecordNumber >= RecordNumber) {
        printf("\nRecord %d is %d [0x%X] bytes long starting at 0x%X\n",
            EventLogRecord->RecordNumber, EventLogRecord->Length,
            EventLogRecord->Length, Record);
        Module = (LPWSTR)(EventLogRecord+1);
        Computer = (LPWSTR)((PBYTE) Module + ((wcslen(Module) + 1) * sizeof(WCHAR)));
        printf("\tGenerated by %ws from system %ws\n", Module, Computer);

        TimeBuffer = ctime((time_t *)&(EventLogRecord->TimeGenerated));
        if (TimeBuffer) {
            printf("\tGenerated at %s", TimeBuffer);
        }
        else {
            printf("\tGenerated time field is blank\n");
        }
        TimeBuffer = ctime((time_t *)&(EventLogRecord->TimeWritten));
        if (TimeBuffer) {
            printf("\tWritten at %s", TimeBuffer);
        }
        else {
            printf("\tTime written field is blank\n");
        }

        printf("\tEvent Id = %d\n", EventLogRecord->EventID);
        printf("\tEventType = ");
        switch (EventLogRecord->EventType) {
            case EVENTLOG_SUCCESS:
                printf("Success\n");
                break;
            case EVENTLOG_ERROR_TYPE:
                printf("Error\n");
                break;
            case EVENTLOG_WARNING_TYPE:
                printf("Warning\n");
                break;
            case EVENTLOG_INFORMATION_TYPE:
                printf("Information\n");
                break;
            case EVENTLOG_AUDIT_SUCCESS:
                printf("Audit Success\n");
                break;
            case EVENTLOG_AUDIT_FAILURE:
                printf("Audit Failure\n");
                break;
            default:
                printf("Invalid value 0x%X\n", EventLogRecord->EventType);
        }
        printf("\t%d strings at offset 0x%X\n", EventLogRecord->NumStrings,
            EventLogRecord->StringOffset);
        printf("\t%d bytes of data at offset 0x%X\n", EventLogRecord->DataLength,
            EventLogRecord->DataOffset);
    }

    if (FirstPiece) {
        Record = StartOfFile + sizeof(ELF_LOGFILE_HEADER) + BufferLen -
            FirstPiece;
    }
    else {
        Record += EventLogRecord->Length;
    }

    LocalFree(EventLogRecord);
    return(Record);
}

//
// Dump a record, or all records, or n records
//

VOID
record(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    DWORD Pointer;
    LOGFILE LogFile;
    DWORD StartOfFile;
    DWORD EndOfFile = 0;
    DWORD RecordNumber = 0;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    //
    // Evaluate the argument string to get the address of
    // the record to dump.
    //

    if (lpArgumentString && *lpArgumentString) {
        if (*lpArgumentString == '.') {
            if (GetLogFileAddress(lpArgumentString+1, &LogFile) == 0) {
                printf("Logfile %s not found\n", lpArgumentString+1);
                return;
            }
            Pointer = ((DWORD) (LogFile.BaseAddress)) + LogFile.BeginRecord;
        }
        else if (*lpArgumentString == '#') {
            RecordNumber = atoi(lpArgumentString + 1);
            printf("Dumping records starting at record #%d\n", RecordNumber);
            lpArgumentString = NULL;
        }
        else if (*lpArgumentString) {
            Pointer = (lpGetExpressionRoutine)(lpArgumentString);
        }
        else {
            printf("Invalid lead character 0x%02X\n", *lpArgumentString);
            return;
        }
    }

    if (!lpArgumentString || *lpArgumentString) {
        if (GetLogFileAddress("system", &LogFile) == 0) {
            printf("System Logfile not found\n");
            return;
        }
        Pointer = ((DWORD) (LogFile.BaseAddress)) + LogFile.BeginRecord;
    }

    StartOfFile = (DWORD) LogFile.BaseAddress;
    EndOfFile = (DWORD) LogFile.BaseAddress + LogFile.ActualMaxFileSize;

    //
    // Dump records starting wherever they told us to
    //

    while (Pointer < EndOfFile && Pointer && !(lpCheckControlCRoutine)()) {
        Pointer = DumpRecord(Pointer, RecordNumber, StartOfFile, EndOfFile);
    }


    return;
}

//
// Dump a single LogModule structure if it matches MatchName (NULL matches
// all)
//

PLIST_ENTRY
DumpLogModule(
    HANDLE hCurrentProcess,
    DWORD pLogModule,
    LPWSTR MatchName
    )
{
    LOGMODULE LogModule;
    WCHAR ModuleName[MAX_NAME / sizeof(WCHAR)];

    GET_DATA(pLogModule, &LogModule, sizeof(LogModule))
    GET_DATA(LogModule.ModuleName, &ModuleName, MAX_NAME)

    if (!MatchName || !wcsicmp(MatchName, ModuleName)) {
        printf("\tModule Name %ws\n", ModuleName);
        printf("\tModule Atom 0x%X\n", LogModule.ModuleAtom);
        printf("\tPointer to LogFile 0x%X\n", LogModule.LogFile);
    }

    return (LogModule.ModuleList.Flink);
}

//
// Dump selected, or all, LogModule structures
//

VOID
logmodule(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    DWORD pLogModule;
    DWORD LogModuleAnchor;
    LPWSTR wArgumentString = NULL;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);
    UnicodeString.Buffer = NULL;

    //
    // Evaluate the argument string to get the address of
    // the logmodule to dump.  If no parm, dump them all.
    //

    if (lpArgumentString && *lpArgumentString == '.') {
        lpArgumentString++;
        RtlInitAnsiString(&AnsiString, lpArgumentString);
        RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
    }
    else if (lpArgumentString && *lpArgumentString) {
        pLogModule = (lpGetExpressionRoutine)(lpArgumentString);
        DumpLogModule(hCurrentProcess, pLogModule, NULL);
        return;
    }

    LogModuleAnchor = (lpGetExpressionRoutine)("LogModuleHead");

    GET_DATA(LogModuleAnchor, &pLogModule, sizeof(DWORD))

    while (pLogModule != LogModuleAnchor && !(lpCheckControlCRoutine)()) {
        pLogModule =
            (DWORD) DumpLogModule(hCurrentProcess, pLogModule,
                UnicodeString.Buffer);
        if (!UnicodeString.Buffer) {
            printf("\n");
        }
    }
    if (UnicodeString.Buffer) {
        RtlFreeUnicodeString(&UnicodeString);
    }

    return;
}

//
// Dump a single LogFile structure if it matches MatchName (NULL matches
// all)
//

PLIST_ENTRY
DumpLogFile(
    HANDLE hCurrentProcess,
    DWORD pLogFile,
    LPWSTR MatchName
    )
{
    LOGFILE LogFile;
    LPWSTR UnicodeName;

    //
    // Get the fixed part of the structure
    //

    GET_DATA(pLogFile, &LogFile, sizeof(LogFile))

    //
    // Get the Default module name
    //

    UnicodeName = GetUnicodeString(LogFile.LogModuleName);

    //
    // See if we're just looking for a particular one.  If we are and
    // this isn't it, bail out.
    //

    if (MatchName && wcsicmp(MatchName, UnicodeName)) {
        LocalFree(UnicodeName);
        return (LogFile.FileList.Flink);
    }

    //
    // Otherwise print it out
    //

    printf("%ws", UnicodeName);
    LocalFree(UnicodeName);

    //
    // Now the file name of this logfile
    //

    UnicodeName = GetUnicodeString(LogFile.LogFileName);
    printf(" : %ws\n", UnicodeName);
    LocalFree(UnicodeName);

    if (LogFile.Notifiees.Flink == LogFile.Notifiees.Blink) {
        printf("\tNo active ChangeNotifies on this log\n");
    }
    else {
        printf("\tActive Change Notify!  Dump of this list not implemented\n");
    }

    printf("\tReference Count: %d\n\tFlags: ", LogFile.RefCount);
    if (LogFile.Flags == 0) {
        printf("No flags set ");
    }
    else {
        if (LogFile.Flags & ELF_LOGFILE_HEADER_DIRTY) {
            printf("Dirty ");
        }
        if (LogFile.Flags & ELF_LOGFILE_HEADER_WRAP) {
            printf("Wrapped ");
        }
        if (LogFile.Flags & ELF_LOGFILE_LOGFULL_WRITTEN) {
             printf("Logfull Written ");
        }
    }
    printf("\n");

    printf("\tMax Files Sizes [Cfg:Curr:Next]  0x%X : 0x%X : 0x%X\n",
        LogFile.ConfigMaxFileSize, LogFile.ActualMaxFileSize,
        LogFile.NextClearMaxFileSize);

    printf("\tRecord Numbers [Oldest:Curr] %d : %d\n",
        LogFile.OldestRecordNumber, LogFile.CurrentRecordNumber);

    printf("\tRetention period in days: %d\n", LogFile.Retention / 86400);

    printf("\tBase Address: 0x%X\n", LogFile.BaseAddress);

    printf("\tView size: 0x%X\n", LogFile.ViewSize);

    printf("\tOffset of beginning record: 0x%X\n", LogFile.BeginRecord);

    printf("\tOffset of ending record: 0x%X\n", LogFile.EndRecord);

    return (LogFile.FileList.Flink);
}

//
// Dump selected, or all, LogFile structures
//

VOID
logfile(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    DWORD pLogFile;
    DWORD LogFileAnchor;
    LPWSTR wArgumentString = NULL;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    BOOL AllocateString = FALSE;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);
    UnicodeString.Buffer = NULL;

    //
    // Evaluate the argument string to get the address of
    // the logfile to dump.  If no parm, dump them all.
    //

    if (lpArgumentString && *lpArgumentString) {
        if(*lpArgumentString == '.') {
            lpArgumentString++;
            RtlInitAnsiString(&AnsiString, lpArgumentString);
            RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
        }
        else {
            pLogFile = (lpGetExpressionRoutine)(lpArgumentString);
            DumpLogFile(hCurrentProcess, pLogFile, NULL);
            return;
        }
    }

    LogFileAnchor = (lpGetExpressionRoutine)("LogFilesHead");

    GET_DATA(LogFileAnchor, &pLogFile, sizeof(DWORD))

    while (pLogFile != LogFileAnchor) {
        pLogFile =
            (DWORD) DumpLogFile(hCurrentProcess, pLogFile,
                UnicodeString.Buffer);
        if (!UnicodeString.Buffer) {
            printf("\n");
        }
    }

    if (UnicodeString.Buffer) {
        RtlFreeUnicodeString(&UnicodeString);
    }

    return;
}

//
// Dump a request packet structure
//

VOID
request(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    ELF_REQUEST_RECORD Request;
    DWORD Pointer;
    DWORD RecordSize;
    WRITE_PKT WritePkt;
    READ_PKT ReadPkt;
    CLEAR_PKT ClearPkt;
    BACKUP_PKT BackupPkt;
    LPWSTR FileName;
    CHAR Address[18];

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    //
    // Evaluate the argument string to get the address of
    // the request packet to dump.
    //

    if (lpArgumentString && *lpArgumentString) {
        Pointer = (lpGetExpressionRoutine)(lpArgumentString);
    }
    else {
        printf("Must supply a request packet address\n");
        return;
    }

    GET_DATA(Pointer, &Request, sizeof(ELF_REQUEST_RECORD))

    switch (Request.Command ) {
        case ELF_COMMAND_READ:
            printf("\nRead packet\n");
            GET_DATA(Request.Pkt.ReadPkt, &ReadPkt, sizeof(READ_PKT))
            printf("\tLast Seek Position = %d\n", ReadPkt.LastSeekPos);
            printf("\tLast Seek Record = %d\n", ReadPkt.LastSeekRecord);
            printf("\tStart at record number %d\n", ReadPkt.RecordNumber);
            printf("\tRead %d bytes into buffer at 0x%X\n",
                ReadPkt.BufferSize, ReadPkt.Buffer);
            if (ReadPkt.Flags & ELF_IREAD_UNICODE) {
                printf("\tReturn in ANSI\n");
            }
            else {
                printf("\tReturn in UNICODE\n");
            }
            printf("\tRead flags: ");
            if (ReadPkt.ReadFlags & EVENTLOG_SEQUENTIAL_READ) {
                printf("Sequential ");
            }
            if (ReadPkt.ReadFlags & EVENTLOG_SEEK_READ) {
                printf("Seek ");
            }
            if (ReadPkt.ReadFlags & EVENTLOG_FORWARDS_READ) {
                printf("Forward ");
            }
            if (ReadPkt.ReadFlags & EVENTLOG_BACKWARDS_READ) {
                printf("Backwards ");
            }
            printf("\n");
            break;

        case ELF_COMMAND_WRITE:
            printf("\nWrite packet\n");
            if (Request.Flags == ELF_FORCE_OVERWRITE) {
                printf("with ELF_FORCE_OVERWRITE enabled\n");
            }
            else {
                printf("\n");
            }
            GET_DATA(Request.Pkt.WritePkt, &WritePkt, sizeof(WRITE_PKT))
            RecordSize = (WritePkt.Datasize);
            DumpRecord((DWORD)WritePkt.Buffer, 0, 0, 0);
            break;

        case ELF_COMMAND_CLEAR:
            printf("\nClear packet\n");
            GET_DATA(Request.Pkt.ClearPkt, &ClearPkt, sizeof(CLEAR_PKT))
            FileName = GetUnicodeString(ClearPkt.BackupFileName);
            printf("Backup filename = %ws\n", FileName);
            LocalFree(FileName);
            break;

        case ELF_COMMAND_BACKUP:
            printf("\nBackup packet\n");
            GET_DATA(Request.Pkt.BackupPkt, &BackupPkt, sizeof(BACKUP_PKT))
            FileName = GetUnicodeString(BackupPkt.BackupFileName);
            printf("Backup filename = %ws\n", FileName);
            LocalFree(FileName);
            break;

        case ELF_COMMAND_WRITE_QUEUED:
            printf("\nQueued Write packet\n");
            if (Request.Flags == ELF_FORCE_OVERWRITE) {
                printf("with ELF_FORCE_OVERWRITE enabled\n");
            }
            else {
                printf("\n");
            }
            printf("NtStatus = 0x%X\n", Request.Status);
            break;

        default:
            printf("\nInvalid packet\n");
    }

    printf("\nLogFile for this packet:\n\n");
    itoa((DWORD) Request.LogFile, Address, 16);
    logfile(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis,
        Address);

    printf("\nLogModule for this packet:\n\n");
    itoa((DWORD)Request.Module, Address, 16);
    logmodule(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis,
        Address);

    return;
}

//
// Online help
//

VOID
help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    printf("\nEventlog NTSD Extensions\n");

    if (!lpArgumentString || *lpArgumentString == '\0' ||
        *lpArgumentString == '\n' || *lpArgumentString == '\r') {
        printf("\tlogmodule - dump a logmodule structure\n");
        printf("\tlogfile   - dump a logfile structure\n");
        printf("\trequest   - dump a request record\n");
        printf("\trecord    - dump a eventlog record\n");
        printf("\n\tEnter help <cmd> for detailed help on a command\n");
    }
    else {
        if (!stricmp(lpArgumentString, "logmodule")) {
            printf("\tlogmodule <arg>, where <arg> can be one of:\n");
            printf("\t\tno argument - dump all logmodule structures\n");
            printf("\t\taddress     - dump the logmodule at specified address\n");
            printf("\t\t.string     - dump the logmodule with name string\n");
        }
        else if (!stricmp(lpArgumentString, "logfile")) {
            printf("\tlogfile <arg>, where <arg> can be one of:\n");
            printf("\t\tno argument - dump all logfile structures\n");
            printf("\t\taddress     - dump the logfile at specified address\n");
            printf("\t\t.string     - dump the logfile with name string\n");
        }
        else if (!stricmp(lpArgumentString, "record")) {
            printf("\trecord <arg>, where <arg> can be one of:\n");
            printf("\t\tno argument - dump all records in system log\n");
            printf("\t\taddress     - dump records starting at specified address\n");
            printf("\t\t.string     - dump all records in the <string> log\n");
            printf("\t\t#<nnn>      - dumps records starting at nnn in system log\n");
            printf("\t\t#<nnn> .string  - dumps records starting at nnn in <string> log\n");
        }
        else if (!stricmp(lpArgumentString, "request")) {
            printf("\trequest - dump the request record at specified address\n");
        }
        else {
            printf("\tInvalid command [%s]\n", lpArgumentString);
        }
    }
}
