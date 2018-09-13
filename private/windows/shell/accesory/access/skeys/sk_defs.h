/*--------------------------------------------------------------
 *
 * FILE:			SK_defs.h
 *
 * PURPOSE:			Global Variables & Defines
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Defines ---------------------------------------------------------*/

//	Commands Process by the Service  inside the MainService Routine

#ifdef DEFDATA
#define	EXTERN	
#define PATHSZ	MAX_PATH
#else
#define	EXTERN	extern
#define PATHSZ	
#endif

// Main Service Defines ---------------------------------------

#define	SC_CLEAR		0
#define SC_LOG_OUT		1
#define SC_LOG_IN		2
#define SC_CHANGE_COMM	3
#define SC_DISABLE_SKEY	4
#define SC_ENABLE_SKEY	5

// Variables ---------------------------------------------------


// Structures ---------------------------------------------------
EXTERN SERIALKEYS	skNewKey, skCurKey;
EXTERN LPSERIALKEYS lpskSKey;

EXTERN TCHAR szNewActivePort[PATHSZ];
EXTERN TCHAR szNewPort[PATHSZ];
EXTERN TCHAR szCurActivePort[PATHSZ];
EXTERN TCHAR szCurPort[PATHSZ];


#define	SERKF_ACTIVE		0x00000040

#define REG_DEF			1
#define REG_USER		2

#define ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))

void DoServiceCommand(DWORD dwServiceCommand);