#ifndef	SNMPELPT_H
#define	SNMPELPT_H

extern	PVarBindQueue		lpVarBindQueue;						// pointer to varbind queue
extern	PSourceHandleList	lpSourceHandleList;						// pointer to source/handle list
extern	DWORD				dwTimeZero;							// time zero reference
extern	BOOL				fTrimFlag;							// trimming flag
extern	PHANDLE				phEventLogs;						// Opened Event Log Handles
extern	PHMODULE			phPrimHandles;						// PrimaryModule file handle array
extern	UINT				uNumEventLogs;						// Number of Opened Event Logs
extern	LPTSTR				lpszEventLogs;						// event log name array
extern	DWORD				nTraceLevel;						// current trace level

extern	INT					iLogNameSize;						// size of event log name array
extern	UINT				nMaxTrapSize;						// maximum size of trap

extern	VOID				WriteTrace(UINT nLvl, LPSTR CONST szStuff, ...);

extern	HANDLE				hWriteEvent;						// handle to write log events
extern	HANDLE				hStopAll;							// handle to global dll shutdown event
extern	HANDLE				hEventNotify;						// handle to notify dll that trap is ready
extern	HANDLE				hRegChanged;						// handle to registry key changed event
extern	TCHAR				szBaseOID[MAX_PATH+1];				// base OID from registry
extern	TCHAR				szSupView[MAX_PATH+1];				// supported view from registry
extern	TCHAR				szelMsgModuleName[MAX_PATH+1];		// expanded DLL message module
extern	BOOL				fGlobalTrim;						// global trim message flag
extern	BOOL				fRegOk;								// registry notification in effect flag
extern	BOOL				Read_Registry_Parameters(VOID);		// reread registry parameters function
extern	BOOL				fThreshold;							// global performance threshold reached flag
extern	BOOL				fThresholdEnabled;					// global threshold enabled flag
extern	BOOL				fDoLogonEvents;
extern	DWORD				dwLastBootTime;
extern	DWORD				dwTrapQueueSize;
extern	BOOL				Position_to_Log_End(HANDLE  hLog);	// set the position of the eventlog to past last record
extern	HKEY				hkRegResult;

typedef struct	_REGSTRUCT	{
	TCHAR	szOID[2*MAX_PATH+1];		// string area for EnterpriseOID field
	BOOL	fAppend;					// append flag
	BOOL	fLocalTrim;					// local message trim flag
	DWORD	nCount;						// count field
	DWORD	nTime;						// time field
}	REGSTRUCT, *PREGSTRUCT;

typedef	struct	_CNTTABSTRUCT	{
	TCHAR	log[MAX_PATH+1];			// log file for entry
	DWORD	event;						// event id
	TCHAR	source[MAX_PATH+1];			// source for event
	DWORD	curcount;					// current count for event
	DWORD	time;						// last time of event from GetCurrentTime()
	struct	_CNTTABSTRUCT	*lpNext;	// pointer to next entry in the table
}	COUNTTABLE, *PCOUNTTABLE;

		PCOUNTTABLE			lpCountTable = (PCOUNTTABLE) NULL;	// address of count/time table
		HANDLE				hMutex;								// handle for mutex object

#endif								// end of snmpelpt.h definitions
