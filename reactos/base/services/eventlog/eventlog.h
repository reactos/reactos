/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/eventlog.h
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2005 Saveliy Tretiakov            
 */
 
 
#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__

#define WIN32_NO_STATUS
#include <windows.h>
#include <lpctypes.h>
#include <lpcfuncs.h>
#include <rtlfuncs.h>
#include <obfuncs.h>
#include <iotypes.h>
#include <debug.h>
#include <pseh/pseh.h>
#include "eventlogrpc_s.h"


typedef struct _IO_ERROR_LPC
{
    PORT_MESSAGE Header;
    IO_ERROR_LOG_MESSAGE Message;
} IO_ERROR_LPC, *PIO_ERROR_LPC;

#define LOGHANDLE unsigned int
#define PLOGHANDLE unsigned int* 

#define MAJORVER 1
#define MINORVER 1

/*
 *  Our file format will be compatible with NT's
 */

#define LOGFILE_SIGNATURE 0x654c664c 

/*  
 *  FIXME
 *  Flags used in logfile header
 */
#define LOGFILE_FLAG1 1
#define LOGFILE_FLAG2 2
#define LOGFILE_FLAG3 4
#define LOGFILE_FLAG4 8

typedef struct {
	DWORD SizeOfHeader;
	DWORD Signature;
	DWORD MajorVersion;
	DWORD MinorVersion;
	DWORD FirstRecordOffset;
	DWORD EofOffset;
	DWORD NextRecord;
	DWORD OldestRecord;
	DWORD unknown1;
	DWORD Flags;
	DWORD unknown2; 
	DWORD SizeOfHeader2; 
} FILE_HEADER, *PFILE_HEADER;

typedef struct {
	DWORD Size1;
	DWORD Ones; // Must be 0x11111111
	DWORD Twos; // Must be 0x22222222
	DWORD Threes; // Must be 0x33333333
	DWORD Fours; // Must be 0x44444444
	DWORD StartOffset;
	DWORD EndOffset;
	DWORD NextRecordNumber;
	DWORD OldestRecordNumber;
	DWORD Size2;
} EOF_RECORD, *PEOF_RECORD;

typedef struct {
	ULONG EventNumber;
	ULONG EventOffset;
} EVENT_OFFSET_INFO, *PEVENT_OFFSET_INFO;

typedef struct {
	HANDLE hFile;
	FILE_HEADER Header;
	WCHAR *LogName;
	WCHAR *FileName;
    CRITICAL_SECTION cs;
	PEVENT_OFFSET_INFO OffsetInfo;
	ULONG OffsetInfoSize;
	ULONG OffsetInfoNext;
	PVOID Next;
	PVOID Prev;
} LOGFILE, *PLOGFILE;


/* file.c */
PLOGFILE LogfListHead();

INT LogfListItemCount();

PLOGFILE LogfListItemByIndex(INT Index);

PLOGFILE LogfListItemByName(WCHAR *Name);

INT LogfListItemIndexByName(WCHAR *Name);

VOID LogfListAddItem(PLOGFILE Item);

VOID LogfListRemoveItem(PLOGFILE Item);

BOOL LogfReadEvent(PLOGFILE LogFile,
                   DWORD Flags,
                   DWORD RecordNumber,
                   DWORD BufSize,
                   PBYTE Buffer,
                   DWORD *BytesRead,
                   DWORD *BytesNeeded);

BOOL LogfWriteData(PLOGFILE LogFile,
                    DWORD BufSize,
                    PBYTE Buffer);

PLOGFILE LogfCreate(WCHAR *LogName, 
                    WCHAR *FileName);

VOID LogfClose(PLOGFILE LogFile);

BOOL LogfInitializeNew(PLOGFILE LogFile);

BOOL LogfInitializeExisting(PLOGFILE LogFile);

DWORD LogfGetOldestRecord(PLOGFILE LogFile);

ULONG LogfOffsetByNumber(PLOGFILE LogFile,
						 DWORD RecordNumber);

BOOL LogfAddOffsetInformation(PLOGFILE LogFile, 
							  ULONG ulNumber,
							  ULONG ulOffset);

DWORD LogfBuildNewRecord(PBYTE Buffer, 
						 DWORD dwRecordNumber,
						 WORD wType,
						 WORD wCategory,
						 DWORD dwEventId,
						 LPCWSTR SourceName,
						 LPCWSTR ComputerName,
						 DWORD dwSidLength,
						 PSID lpUserSid,
						 WORD wNumStrings,
						 WCHAR *lpStrings,
						 DWORD dwDataSize,
						 LPVOID lpRawData);

/* eventlog.c */
VOID PRINT_HEADER(PFILE_HEADER header);

VOID PRINT_RECORD(PEVENTLOGRECORD pRec);

VOID EventTimeToSystemTime(DWORD EventTime, 
						   SYSTEMTIME *SystemTime);

VOID SystemTimeToEventTime(SYSTEMTIME *pSystemTime,
						   DWORD *pEventTime);

/* logport.c */
NTSTATUS STDCALL PortThreadRoutine(PVOID Param);

NTSTATUS InitLogPort(VOID);

NTSTATUS ProcessPortMessage(VOID);

/* rpc.c */
DWORD STDCALL RpcThreadRoutine(LPVOID lpParameter);

NTSTATUS EventLogClearW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	wchar_t *BackupName);

NTSTATUS EventLogBackupW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	wchar_t *FileName);

NTSTATUS EventLogClose(
	handle_t BindingHandle,
	PLOGHANDLE Handle);

NTSTATUS EventLogUnregSrc(
	handle_t BindingHandle,
	PLOGHANDLE Handle);

NTSTATUS EventLogRecordsNumber(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long *RecordsNumber);

NTSTATUS EventLogGetOldestRec(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long *OldestRecNumber);

NTSTATUS EventLogChangeNotify( 
	handle_t BindingHandle);

NTSTATUS EventLogOpenW(
	handle_t BindingHandle,
	LPWSTR ServerName,
	wchar_t *FileName, 
	wchar_t *NullStr, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle);

NTSTATUS EventLogRegSrcW(
	handle_t BindingHandle,
	LPWSTR ServerName,
	wchar_t *LogName, 
	wchar_t *NullStr, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle);

NTSTATUS EventLogOpenBackupW(
	handle_t BindingHandle,
	LPWSTR ServerName,
	wchar_t *BackupName, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle);

NTSTATUS EventLogReadW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Flags,
	unsigned long Offset,
	unsigned long BufSize,
	unsigned char *Buffer,
	unsigned long *BytesRead,
	unsigned long *BytesNeeded);

NTSTATUS EventLogReportEventW(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Time,
	unsigned short Type,
	unsigned short Category,
	unsigned long ID,
	unsigned short NumStrings,
	unsigned long DataSize,
	wchar_t *ComputerName,
	unsigned char *SID,
	wchar_t *Strings,
	unsigned char *Data,
	unsigned short Flags,
	unsigned long * unknown1,
	unsigned long * unknown2);

NTSTATUS EventLogClearA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	char *BackupName);

NTSTATUS EventLogBackupA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	char *BackupName);

NTSTATUS EventLogOpenA(
	handle_t BindingHandle,
	LPSTR ServerName,
	char *LogName, 
	char *NullStr, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle);

NTSTATUS EventLogRegSrcA(
	handle_t BindingHandle,
	LPSTR ServerName,
	char *LogName, 
	char *NullStr,
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle);

NTSTATUS EventLogOpenBackupA(
	handle_t BindingHandle,
	LPSTR ServerName,
	char *BackupName, 
	unsigned long MajorVer,
	unsigned long MinorVer,
	PLOGHANDLE Handle);

NTSTATUS EventLogReadA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Flags,
	unsigned long Offset,
	unsigned long BufSize,
	unsigned char *Buffer,
	unsigned long *BytesRead,
	unsigned long *BytesNeeded);

NTSTATUS EventLogReportEventA(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long Time,
	unsigned short Type,
	unsigned short Category,
	unsigned long ID,
	unsigned short NumStrings,
	unsigned long DataSize,
	char *ComputerName,
	unsigned char *SID,
	char* Strings,
	unsigned char *Data,
	unsigned short Flags,
	unsigned long * unknown1,
	unsigned long * unknown2);

NTSTATUS EventLogRegisterClusterSvc( 
	handle_t BindingHandle);
	
NTSTATUS EventLogDeregisterClusterSvc( 
	handle_t BindingHandle);

NTSTATUS EventLogWriteClusterEvents( 
	handle_t BindingHandle);

NTSTATUS EventLogGetInfo(
	handle_t BindingHandle,
	LOGHANDLE Handle,
	unsigned long InfoLevel,
	unsigned char *Buffer,
	unsigned long BufSize,
	unsigned long *BytesNeeded);

NTSTATUS EventLogFlush( 
	handle_t BindingHandle);


#endif /* __EVENTLOG_H__ */

