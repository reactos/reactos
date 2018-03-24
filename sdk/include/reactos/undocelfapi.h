/*
 * PROJECT:     ReactOS EventLog Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Event logging NT client API.
 * COPYRIGHT:   Copyright 2016-2018 Hermes Belusca-Maito
 */

#ifndef _UNDOCELFAPI_H
#define _UNDOCELFAPI_H

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
NTAPI
ElfBackupEventLogFileA(
    IN HANDLE hEventLog,
    IN PANSI_STRING BackupFileNameA);

NTSTATUS
NTAPI
ElfBackupEventLogFileW(
    IN HANDLE hEventLog,
    IN PUNICODE_STRING BackupFileNameU);

NTSTATUS
NTAPI
ElfClearEventLogFileA(
    IN HANDLE hEventLog,
    IN PANSI_STRING BackupFileNameA);

NTSTATUS
NTAPI
ElfClearEventLogFileW(
    IN HANDLE hEventLog,
    IN PUNICODE_STRING BackupFileNameU);

NTSTATUS
NTAPI
ElfCloseEventLog(
    IN HANDLE hEventLog);

NTSTATUS
NTAPI
ElfDeregisterEventSource(
    IN HANDLE hEventLog);

NTSTATUS
NTAPI
ElfNumberOfRecords(
    IN HANDLE hEventLog,
    OUT PULONG NumberOfRecords);

NTSTATUS
NTAPI
ElfOldestRecord(
    IN HANDLE hEventLog,
    OUT PULONG OldestRecordNumber);

NTSTATUS
NTAPI
ElfChangeNotify(
    IN HANDLE hEventLog,
    IN HANDLE hEvent);

NTSTATUS
NTAPI
ElfOpenBackupEventLogA(
    IN PANSI_STRING UNCServerNameA,
    IN PANSI_STRING BackupFileNameA,
    OUT PHANDLE phEventLog);

NTSTATUS
NTAPI
ElfOpenBackupEventLogW(
    IN PUNICODE_STRING UNCServerNameU,
    IN PUNICODE_STRING BackupFileNameU,
    OUT PHANDLE phEventLog);

NTSTATUS
NTAPI
ElfOpenEventLogA(
    IN PANSI_STRING UNCServerNameA,
    IN PANSI_STRING SourceNameA,
    OUT PHANDLE phEventLog);

NTSTATUS
NTAPI
ElfOpenEventLogW(
    IN PUNICODE_STRING UNCServerNameU,
    IN PUNICODE_STRING SourceNameU,
    OUT PHANDLE phEventLog);

NTSTATUS
NTAPI
ElfReadEventLogA(
    IN HANDLE hEventLog,
    IN ULONG ReadFlags,
    IN ULONG RecordOffset,
    OUT LPVOID Buffer,
    IN ULONG NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead,
    OUT PULONG MinNumberOfBytesNeeded);

NTSTATUS
NTAPI
ElfReadEventLogW(
    IN HANDLE hEventLog,
    IN ULONG ReadFlags,
    IN ULONG RecordOffset,
    OUT LPVOID Buffer,
    IN ULONG NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead,
    OUT PULONG MinNumberOfBytesNeeded);

NTSTATUS
NTAPI
ElfRegisterEventSourceA(
    IN PANSI_STRING UNCServerNameA,
    IN PANSI_STRING SourceNameA,
    OUT PHANDLE phEventLog);

NTSTATUS
NTAPI
ElfRegisterEventSourceW(
    IN PUNICODE_STRING UNCServerNameU,
    IN PUNICODE_STRING SourceNameU,
    OUT PHANDLE phEventLog);

NTSTATUS
NTAPI
ElfReportEventA(
    IN HANDLE hEventLog,
    IN USHORT EventType,
    IN USHORT EventCategory,
    IN ULONG EventID,
    IN PSID UserSID,
    IN USHORT NumStrings,
    IN ULONG DataSize,
    IN PANSI_STRING* Strings,
    IN PVOID Data,
    IN USHORT Flags,
    IN OUT PULONG RecordNumber,
    IN OUT PULONG TimeWritten);

NTSTATUS
NTAPI
ElfReportEventW(
    IN HANDLE hEventLog,
    IN USHORT EventType,
    IN USHORT EventCategory,
    IN ULONG EventID,
    IN PSID UserSID,
    IN USHORT NumStrings,
    IN ULONG DataSize,
    IN PUNICODE_STRING* Strings,
    IN PVOID Data,
    IN USHORT Flags,
    IN OUT PULONG RecordNumber,
    IN OUT PULONG TimeWritten);

NTSTATUS
NTAPI
ElfReportEventAndSourceW(
    IN HANDLE hEventLog,
    IN ULONG Time,
    IN PUNICODE_STRING ComputerName,
    IN USHORT EventType,
    IN USHORT EventCategory,
    IN ULONG EventID,
    IN PSID UserSID,
    IN PUNICODE_STRING SourceName,
    IN USHORT NumStrings,
    IN ULONG DataSize,
    IN PUNICODE_STRING* Strings,
    IN PVOID Data,
    IN USHORT Flags,
    IN OUT PULONG RecordNumber,
    IN OUT PULONG TimeWritten);

NTSTATUS
NTAPI
ElfFlushEventLog(
    IN HANDLE hEventLog);

#ifdef __cplusplus
}
#endif

#endif /* _UNDOCELFAPI_H */

/* EOF */
