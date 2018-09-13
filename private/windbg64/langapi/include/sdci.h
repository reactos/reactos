//
// sdci.h
//
// ODBC/DBLib private entry point 
// Copyright (C) 1996, Microsoft Corp.	All Rights Reserved.
//

#ifndef __SDCI_INCLUDED__
#define __SDCI_INCLUDED__

#ifndef ULONG
typedef unsigned long ULONG;
#endif 

enum {
	cbMaxMchName = 32,
	cbMaxDLLName = 16
};

typedef struct _SDCI {
	ULONG		cbLength;			// size of this struct
	ULONG		dbgpid;				// pid of debugger
	ULONG		pid;				// pid to start/stop debugging 
	char		rgchMchName[cbMaxMchName];	// machine name of debugger
	UNALIGNED void *pvData;				// future use
	ULONG		cbData;				// size of data
	char		rgchSDIDLLName[cbMaxDLLName];	// name of DLL to load
	ULONG		fOption;			// 1 - start debugging; 0 - stop debugging
} SDCI, *PSDCI;


BOOL _stdcall SQLDebug(SDCI *psdci);

typedef BOOL (_stdcall *pfnSQLDebug) (SDCI *psdci);

#define	SDCI_CODE_BREAK		1
#define	SDCI_CODE_ENABLE	2
#define	SDCI_CODE_RESUME	3

#endif // __SDCI_INCLUDED__
