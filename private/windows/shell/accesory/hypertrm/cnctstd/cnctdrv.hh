/*	File: D:\WACKER\cnctstd\cnctdrv.hh (Created: 19-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:00p $
 */

typedef struct stCnctDrvPrivate *HHDRIVER;

/* --- Macros --- */

#define STATE_START 		100

#define STATE_DISCONNECT	200

/* --- Data structures --- */

struct stCnctDrvPrivate
	{
	HCNCT	 hCnct; 		// public connection handle passed to create func
	HSESSION hSession;		// public session handle passed to create func

	HANDLE	 hDiscnctEvent, // event semaphore for disconnects
			 hMatchEvent,	// signals a string match has occured
			 hThread;		// connection thread

	CRITICAL_SECTION cs;	// critical section semaphore

	int 	 iStatus,		// connection status
			 iState;		// current connection state

	unsigned uFlags;		// connection flags passed in

	DWORD	 dwTime;		// used for MultipleWaitForObjects()
	};

/* --- Function Prototypes --- */

HDRIVER WINAPI cnctdrvCreate(const HCNCT hCnct, const HSESSION hSession);
int WINAPI cnctdrvDestroy(const HHDRIVER hhDriver);
void cnctdrvLock(const HHDRIVER hhDriver);
void cnctdrvUnlock(const HHDRIVER hhDriver);
int WINAPI cnctdrvQueryStatus(const HHDRIVER hhDriver);
int WINAPI cnctdrvConnect(const HHDRIVER hhDriver, const unsigned int uFlags);
int WINAPI cnctdrvDisconnect(const HHDRIVER hhDriver, const unsigned int uFlags);
DWORD WINAPI ConnectLoop(const HHDRIVER hhDriver);
int WINAPI cnctdrvComEvent(const HHDRIVER hhDriver);
