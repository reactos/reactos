/*
 * PROJECT:          ReactOS kernel
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             services/eventlog/eventlog.c
 * PURPOSE:          Event logging service
 * COPYRIGHT:        Copyright 2002 Eric Kohl
 *                   Copyright 2005 Saveliy Tretiakov
 */


#include "eventlog.h"

VOID CALLBACK ServiceMain(DWORD argc, LPTSTR *argv);

SERVICE_TABLE_ENTRY ServiceTable[2] =
{
  {L"EventLog", (LPSERVICE_MAIN_FUNCTION)ServiceMain},
  {NULL, NULL}
};

/* GLOBAL VARIABLES */
HANDLE MyHeap = NULL;
PLOGFILE SystemLog = NULL;
PLOGFILE ApplicationLog = NULL;
BOOL onLiveCD = FALSE; // On livecd events will go to debug output only

VOID CALLBACK ServiceMain(DWORD argc, LPTSTR *argv)
{
    HANDLE hThread;

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)
                            PortThreadRoutine,
                           NULL,
                           0,
                           NULL);
    
    if(!hThread) DPRINT("Can't create PortThread\n");
    else CloseHandle(hThread);
    
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)
                            RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if(!hThread) DPRINT("Can't create RpcThread\n");
    else CloseHandle(hThread);
}


int main(int argc, char *argv[])
{
	WCHAR LogPath[MAX_PATH];
	MyHeap = HeapCreate(0, 1024*256, 0);

	if(MyHeap==NULL)
	{
		DbgPrint("EventLog: FATAL ERROR, can't create heap.\n");
		return 1;
	}
	
	/*
	This will be fixed in near future
	 */
	
	GetWindowsDirectory(LogPath, MAX_PATH);
	if(GetDriveType(LogPath) == DRIVE_CDROM)
	{
		DPRINT("LiveCD detected\n");
		onLiveCD = TRUE;
	}
	else
	{
		lstrcat(LogPath, L"\\system32\\config\\SysEvent.evt");

		SystemLog = LogfCreate(L"System", LogPath);

		if(SystemLog == NULL)
		{
			DbgPrint("EventLog: FATAL ERROR, can't create %S\n", LogPath);
			HeapDestroy(MyHeap);
			return 1;
		}

		GetWindowsDirectory(LogPath, MAX_PATH);
		lstrcat(LogPath, L"\\system32\\config\\AppEvent.evt");

		ApplicationLog = LogfCreate(L"Application", LogPath);

		if(ApplicationLog == NULL)
		{
			DbgPrint("EventLog: FATAL ERROR, can't create %S\n", LogPath);
			HeapDestroy(MyHeap);
			return 1;
		}
	}

    StartServiceCtrlDispatcher(ServiceTable);

	LogfClose(SystemLog);
	HeapDestroy(MyHeap);

    return 0;
}

VOID EventTimeToSystemTime(DWORD EventTime, 
                           SYSTEMTIME *pSystemTime)
{
	SYSTEMTIME st1970 = { 1970, 1, 0, 1, 0, 0, 0, 0 };
	FILETIME ftLocal;
	union {
		FILETIME ft;
		ULONGLONG ll;
	} u1970, uUCT;
	
	uUCT.ft.dwHighDateTime = 0;
	uUCT.ft.dwLowDateTime = EventTime;
	SystemTimeToFileTime(&st1970, &u1970.ft);
	uUCT.ll = uUCT.ll * 10000000 + u1970.ll;
	FileTimeToLocalFileTime(&uUCT.ft, &ftLocal);
	FileTimeToSystemTime(&ftLocal, pSystemTime);
}

VOID SystemTimeToEventTime(SYSTEMTIME *pSystemTime,
						   DWORD *pEventTime)
{
	SYSTEMTIME st1970 = { 1970, 1, 0, 1, 0, 0, 0, 0 };
	union {
		FILETIME ft;
		ULONGLONG ll;
	} Time, u1970;

	SystemTimeToFileTime(pSystemTime, &Time.ft);
	SystemTimeToFileTime(&st1970, &u1970.ft);
	*pEventTime = (Time.ll - u1970.ll) / 10000000; 
}

VOID PRINT_HEADER(PFILE_HEADER header)
{
	DPRINT("SizeOfHeader=%d\n",header->SizeOfHeader);
	DPRINT("Signature=0x%x\n",header->Signature);
	DPRINT("MajorVersion=%d\n",header->MajorVersion);
	DPRINT("MinorVersion=%d\n",header->MinorVersion);
	DPRINT("FirstRecordOffset=%d\n",header->FirstRecordOffset);
	DPRINT("EofOffset=0x%x\n",header->EofOffset);
	DPRINT("NextRecord=%d\n",header->NextRecord);
	DPRINT("OldestRecord=%d\n",header->OldestRecord);
	DPRINT("unknown1=0x%x\n",header->unknown1);
	DPRINT("unknown2=0x%x\n",header->unknown2);
	DPRINT("SizeOfHeader2=%d\n",header->SizeOfHeader2);
	DPRINT("Flags: ");
	if(header->Flags & LOGFILE_FLAG1)DPRINT("LOGFILE_FLAG1 ");
	if(header->Flags & LOGFILE_FLAG2)DPRINT("| LOGFILE_FLAG2 ");
	if(header->Flags & LOGFILE_FLAG3)DPRINT("| LOGFILE_FLAG3 ");
	if(header->Flags & LOGFILE_FLAG4)DPRINT("| LOGFILE_FLAG4");
	DPRINT("\n"); 
}

VOID PRINT_RECORD(PEVENTLOGRECORD pRec)
{
	UINT i;
	WCHAR *str;
	SYSTEMTIME time;
	
	DPRINT("Length=%d\n", pRec->Length );
	DPRINT("Reserved=0x%x\n", pRec->Reserved );
	DPRINT("RecordNumber=%d\n", pRec->RecordNumber );
	
	EventTimeToSystemTime(pRec->TimeGenerated, &time);
	DPRINT("TimeGenerated=%d.%d.%d %d:%d:%d\n", 
			time.wDay, time.wMonth, time.wYear,
			time.wHour, time.wMinute, time.wSecond);

	EventTimeToSystemTime(pRec->TimeWritten, &time);  
	DPRINT("TimeWritten=%d.%d.%d %d:%d:%d\n", 
			time.wDay, time.wMonth, time.wYear,
			time.wHour, time.wMinute, time.wSecond);

	DPRINT("EventID=%d\n", pRec->EventID ); 

	switch(pRec->EventType)
	{
		case EVENTLOG_ERROR_TYPE:
			DPRINT("EventType = EVENTLOG_ERROR_TYPE\n");
			break;
		case EVENTLOG_WARNING_TYPE:
			DPRINT("EventType = EVENTLOG_WARNING_TYPE\n");
			break;
		case EVENTLOG_INFORMATION_TYPE:
			DPRINT("EventType = EVENTLOG_INFORMATION_TYPE\n");
 			break;
		case EVENTLOG_AUDIT_SUCCESS:
			DPRINT("EventType = EVENTLOG_AUDIT_SUCCESS\n");
			break;
		case EVENTLOG_AUDIT_FAILURE:
			DPRINT("EventType = EVENTLOG_AUDIT_FAILURE\n");
			break;
		default:
			DPRINT("EventType = %x\n");
	}	

	DPRINT("NumStrings=%d\n",  pRec->NumStrings );
	DPRINT("EventCategory=%d\n",  pRec->EventCategory); 
	DPRINT("ReservedFlags=0x%x\n", pRec->ReservedFlags);
	DPRINT("ClosingRecordNumber=%d\n", pRec->ClosingRecordNumber);
	DPRINT("StringOffset=%d\n", pRec->StringOffset); 
	DPRINT("UserSidLength=%d\n", pRec->UserSidLength);  
	DPRINT("UserSidOffset=%d\n", pRec->UserSidOffset); 
	DPRINT("DataLength=%d\n", pRec->DataLength); 
	DPRINT("DataOffset=%d\n", pRec->DataOffset); 

	DPRINT("SourceName: %S\n", (WCHAR *)(((PBYTE)pRec)+sizeof(EVENTLOGRECORD)));
	i = (lstrlenW((WCHAR *)(((PBYTE)pRec)+sizeof(EVENTLOGRECORD)))+1)*sizeof(WCHAR);
	DPRINT("ComputerName: %S\n", (WCHAR *)(((PBYTE)pRec)+sizeof(EVENTLOGRECORD)+i));
	
	if(pRec->StringOffset < pRec->Length && pRec->NumStrings){
		DPRINT("Strings:\n");
		str = (WCHAR*)(((PBYTE)pRec)+pRec->StringOffset);
		for(i = 0; i < pRec->NumStrings; i++)
		{
			DPRINT("[%d] %S\n", i, str);
			str = str+lstrlenW(str)+1;
		}
	}

	DPRINT("Length2=%d\n", *(PDWORD)(((PBYTE)pRec)+pRec->Length-4));
}



