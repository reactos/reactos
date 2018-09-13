/*	File: D:\WACKER\tdll\cnct.h (Created: 10-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */

/* --- Connection driver handle definition (struct never defined) --- */

typedef struct stCnctDriverPublic *HDRIVER;

/* --- Error return codes --- */

#define CNCT_BAD_HANDLE 		-1	// invalid connection handle
#define CNCT_NO_THREAD			-2	// couldn't create a connection thread
#define CNCT_ALREADY_OPEN		-3	// connection is already opened
#define CNCT_FIND_DLL_FAILED	-4	// couldn't find DLL
#define CNCT_LOAD_DLL_FAILED	-5	// couldn't load DLL
#define CNCT_ERROR				-6	// general error
#define CNCT_NOT_SUPPORTED		-7	// driver doesn't support this function

/* --- Connection status codes --- */

#define CNCT_STATUS_FALSE			   0	// disconnected state
#define CNCT_STATUS_TRUE			   1	// connected state
#define CNCT_STATUS_CONNECTING		   2	// trying to connect
#define CNCT_STATUS_DISCONNECTING	   3	// trying to disconnect
#define CNCT_STATUS_ANSWERING          4    // waiting for caller

/* --- cnctConnect Flags (must be powers of two) --- */

#define CNCT_NOCONFIRM				0x0001	// do not pop-up confirmation dialog
#define CNCT_NEW					0x0002	// this is a new connection
#define CNCT_DIALNOW				0x0004	// disconnect flag that forces redial
#define CNCT_PORTONLY				0x0008	// don't dial phone number
#define DISCNCT_NOBEEP				0x0010	// don't beep on disconnect
#define CNCT_WINSOCK                0x0020  // try to connect w/ Winsock to ip address
#define CNCT_ANSWER                 0x0040  // wait for a call
#ifdef INCL_EXIT_ON_DISCONNECT
#define DISCNCT_EXIT				0x0080	// exit on disconnect
#else
#define DISCNCT_EXIT				0x0000	// no meaning whatsoever - just a placeholder
#endif

/* --- Function Prototypes --- */

HCNCT cnctCreateHdl(const HSESSION hSession);
void cnctDestroyHdl(const HCNCT hCnct);
int cnctQueryStatus(const HCNCT hCnct);
int cnctConnect(const HCNCT hCnct, const unsigned int uCnctFlags);
int cnctSetDevice(const HCNCT hCnct, const LPTSTR pachDevice);
int cnctDisconnect(const HCNCT hCnct, const unsigned int uCnctFlags);
int cnctComEvent(const HCNCT hCnct, const enum COM_EVENTS event);
HDRIVER cnctQueryDriverHdl(const HCNCT hCnct);
int cnctLoad(const HCNCT hCnct);
int cnctSave(const HCNCT hCnct);
int cnctSetStartTime(HCNCT hCnct);
int cnctQueryStartTime(const HCNCT hCnct, time_t *pTime);
int cnctQueryElapsedTime(HCNCT hCnct, time_t *pTime);
int cnctInit(const HCNCT hCnct);
void cnctMessage(const HCNCT hCnct, const int idMsg);
int cnctSetDestination(const HCNCT hCnct, TCHAR * const ach, const size_t cb);
int cnctGetComSettingsString(const HCNCT hCnct, LPTSTR pach, const size_t cb);
