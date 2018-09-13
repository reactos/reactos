/*****************************************************************/
/**				  Microsoft Windows								**/
/**		      Copyright (C) Microsoft Corp., 1995				**/
/*****************************************************************/ 

//
//	AUTODIAL.HPP - winsock autodial hook code
//

//	HISTORY:
//	
//	3/22/95	jeremys		Created.
//

#ifndef _AUTODIAL_HPP_
#define _AUTODIAL_HPP_


#include <raserror.h>

// typedefs for function pointers for RNA functions
typedef DWORD 		(WINAPI * RNAENUMDEVICES) (LPBYTE, LPDWORD, LPDWORD);
typedef DWORD 		(WINAPI * RNAIMPLICITDIAL) (HWND,LPSTR);
typedef DWORD 		(WINAPI * RNAACTIVATEENGINE) (VOID);
typedef DWORD 		(WINAPI * RNADEACTIVATEENGINE) (VOID);
typedef DWORD		(WINAPI * RNAENUMCONNENTRIES) (LPSTR,UINT,LPDWORD);
typedef DWORD		(WINAPI * RASCREATEPHONEBOOKENTRY) (HWND,LPSTR);
typedef DWORD		(WINAPI * RASEDITPHONEBOOKENTRY) (HWND,LPSTR,LPSTR);

// typedefs for function pointers for Internet wizard functions
typedef VOID		(WINAPI * INETPERFORMSECURITYCHECK) (HWND,LPBOOL);

// structure for getting proc addresses of api functions
typedef struct APIFCN {
	PVOID * ppFcnPtr;
	LPCSTR pszName;
} APIFCN;

#define SMALLBUFLEN		48	// convenient size for small buffers

#ifndef RAS_MaxEntryName
#undef RAS_MaxEntryName
#endif // RAS_MaxEntryName
#define RAS_MaxEntryName	256


/* Prototypes
 *************/

/* autodial.cpp */

extern BOOL InitAutodialModule(void);
extern void ExitAutodialModule(void);

// opcode ordinals for dwOpCode parameter in hook
#define AUTODIAL_CONNECT		1
#define AUTODIAL_GETHOSTBYADDR	2
#define AUTODIAL_GETHOSTBYNAME	3
#define AUTODIAL_LISTEN			4
#define AUTODIAL_RECVFROM		5
#define AUTODIAL_SENDTO			6

// maximum length of local host name
#define MAX_LOCAL_HOST			255

// max length of exported autodial handler function
#define MAX_AUTODIAL_FCNNAME	48
#endif

