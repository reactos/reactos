#ifndef	SNMPELTRLG_H
#define	SNMPELTRLG_H

extern	HANDLE	hWriteEvent;		// handle to write event log records

TCHAR	szTraceFileName[MAX_PATH+1] = TEXT("");	// file name for trace information (from registry)
TCHAR	szelMsgModuleName[MAX_PATH+1] = TEXT("");	// space for expanded DLL message module

BOOL	fTraceFileName = FALSE;		// flag indicating registry read for trace file name
UINT	nTraceLevel = 0x20;			// trace level for message information

HMODULE	hMsgModule;					// handle to message module
BOOL	fMsgModule = FALSE;			// flag indicating registry read for message module

#endif	// end of SNMPTRLG.H definitions
