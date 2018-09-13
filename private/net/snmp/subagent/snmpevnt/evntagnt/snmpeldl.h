#ifndef	SNMPELDLL_H
#define	SNMPELDLL_H

extern	DWORD			SnmpEvLogProc(void);
extern	HANDLE			hMutex;									// handle for mutex object
extern	VOID			WriteTrace(UINT nLvl, LPSTR CONST szStuff, ...);
extern	TCHAR			szTraceFileName[];						// trace filename
extern	DWORD			nTraceLevel;							// trace level
extern  BOOL            fTraceFileName;                         // trace filename param existance flag

		LPTSTR			lpszEventLogs = (LPTSTR) NULL;			// pointer to event log name array
		PHANDLE 		phEventLogs = (PHANDLE) NULL;			// Pointer to event log handle array
		PHMODULE		phPrimHandles = (PHMODULE) NULL;		// pointer to primary module handle array
		UINT    		uNumEventLogs = 0;						// Number of event logs present
		INT				iLogNameSize = 0;						// size of event log name array
		UINT			nMaxTrapSize = 0;						// maximum trap size

		HKEY			hkRegResult = (HKEY) NULL;				// handle to Parameters registry entry
		HANDLE			hWriteEvent;							// handle to write log events
		HANDLE			hStopAll;								// handle to global dll shutdown event
		HANDLE			hServThrd;								// handle to SNMPELPT thread
		HANDLE			hEventNotify;							// handle to notify SNMPELDL that a trap is ready to process
		HANDLE			hRegChanged;							// handle to registry key changed event

		BOOL			fGlobalTrim = TRUE;						// global message trimming flag (trim msg first or insertion strings first)
		BOOL			fThresholdEnabled = TRUE;				// global threshold checking enabled flag
		BOOL			fTrimFlag = FALSE;						// global trimming flag (do or don't do trimming at all)
		BOOL			fTrapSent = FALSE;						// global trap sent flag
		BOOL			fRegNotify = FALSE;						// registry notification initialization flag
		BOOL			fRegOk = FALSE;							// registry notification in effect flag
		BOOL			fLogInit = FALSE;						// indicate log file registry information not yet read
		BOOL			fThreshold = FALSE;						// global performance threshold flag
		BOOL			fSendThresholdTrap = FALSE;				// indicator to send threshold reached trap
		BOOL			fDoLogonEvents = TRUE;					// do we need to send logon events

		TCHAR			szBaseOID[MAX_PATH+1] = TEXT("");		// base enterprise OID
		TCHAR			szSupView[MAX_PATH+1] = TEXT("");		// supported view OID

		DWORD			dwTimeZero;								// time zero reference
		DWORD			dwThresholdCount;						// threshold count for performance
		DWORD			dwThresholdTime;						// time in seconds for threshold for performance
		DWORD			dwTrapCount = 0;						// number of traps sent
		DWORD			dwTrapStartTime = 0;					// time when first trap in time period sent
		DWORD			dwLastBootTime = 0;							// time the last boot occurred
		DWORD			dwTrapQueueSize = 0;

		PVarBindQueue	lpVarBindQueue = (PVarBindQueue) NULL;	// pointer to first varbind queue entry
		PSourceHandleList	lpSourceHandleList = (PSourceHandleList) NULL;	// pointer to first source/handle entry

		AsnObjectIdentifier	thresholdOID;						// OID used for threshold reached trap
		RFC1157VarBindList	thresholdVarBind;					// varbind list used for threshold reached trap
const	TCHAR			lpszThreshold[] = TEXT("SNMP EventLog Extension Agent is quiescing trap processing due to performance threshold parameters.");

#endif							// end of snmpeldl.h definitions
