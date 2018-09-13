/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1991-1992          **/
/*****************************************************************/

/* NPHOOK.H -- Internal header for hooking calls into network providers.
 *
 *
 * History:
 *  05/17/94    lens   Created.
 *
 */

#include <npdefs.h>
#include <netspi.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#define ORD_NPSHookMPR               222
#define ORD_NPSUnHookMPR             223
#define ORD_NPSUnHookMe              224
#define ORD_NPSGetHandleFromInstance 225

#define NPSHookMPR NPSHookMPRA
#define NPSUnHookMPR NPSUnHookMPRA
#define NPSUnHookMe NPSUnHookMeA
#define NPSGetHandleFromInstance NPSGetHandleFromInstanceA

typedef HMODULE F_LoadLibrary(
    LPCTSTR  lpszLibFile
    );
typedef F_LoadLibrary *PF_LoadLibrary;

typedef BOOL F_FreeLibrary(
    HMODULE hLibModule
    );
typedef F_FreeLibrary *PF_FreeLibrary;

typedef FARPROC F_GetProcAddress(
    HMODULE hModule,
    LPCSTR  lpszProc
    );
typedef F_GetProcAddress *PF_GetProcAddress;

typedef HANDLE16 F_LoadLibrary16(
    LPCTSTR  lpszLibFile
    );
typedef F_LoadLibrary16 *PF_LoadLibrary16;

typedef VOID F_FreeLibrary16(
    HANDLE16 hLibModule
    );
typedef F_FreeLibrary16 *PF_FreeLibrary16;

typedef DWORD WINAPI F_GetProcAddressByName16(
    LPCSTR   lpszProc,
    HANDLE16 hModule
    );
typedef F_GetProcAddressByName16 *PF_GetProcAddressByName16;

typedef DWORD WINAPI F_GetProcAddressByOrdinal16(
    WORD     wOrdinal,
    HANDLE16 hModule
    );
typedef F_GetProcAddressByOrdinal16 *PF_GetProcAddressByOrdinal16;

typedef DWORD NPSERVICE F_NPSHookMPR(
    struct _MPRCALLS *pMPRCalls
    );
typedef F_NPSHookMPR *PF_NPSHookMPR;

F_NPSHookMPR NPSHookMPR;

typedef DWORD NPSERVICE F_UnHookMPR(
    PF_NPSHookMPR pfNPSHookMPR, 
    struct _MPRCALLS *pMPRCalls
    );
typedef F_UnHookMPR *PF_UnHookMPR;

typedef DWORD NPSERVICE F_NPSUnHookMPR(
    PF_NPSHookMPR pfReqNPSHookMPR, 
    struct _MPRCALLS *pReqMPRCalls,
    struct _MPRCALLS *pChainedMPRCalls
    );
typedef F_NPSUnHookMPR *PF_NPSUnHookMPR;

F_NPSUnHookMPR NPSUnHookMPR;

typedef DWORD NPSERVICE F_NPSUnHookMe(
    PF_NPSHookMPR pfMyNPSHookMPR, 
    struct _MPRCALLS *pChainedMPRCalls
    );
typedef F_NPSUnHookMe *PF_NPSUnHookMe;

F_NPSUnHookMe NPSUnHookMe;

typedef struct _MPRCALLS {
    PF_NPSHookMPR       pfNPSHookMPR;       /* NPSHookMPR call */
    PF_UnHookMPR        pfUnHookMPR;        /* UnHookMPR call */
    PF_LoadLibrary      pfLoadLibrary;      /* LoadLibrary call */
    PF_FreeLibrary      pfFreeLibrary;      /* FreeLibrary call */
    PF_GetProcAddress   pfGetProcAddress;   /* GetProcAddress call */
    PF_LoadLibrary16    pfLoadLibrary16;    /* LoadLibrary call */
    PF_FreeLibrary16    pfFreeLibrary16;    /* FreeLibrary call */
    PF_GetProcAddressByName16 pfGetProcAddressByName16; /* GetProcAddress call */
    PF_GetProcAddressByOrdinal16 pfGetProcAddressByOrdinal16; /* GetProcAddress call */
} MPRCALLS, *PMPRCALLS;

typedef HPROVIDER NPSERVICE F_NPSGetHandleFromInstance(
    BOOL    bWinnet16, 
	LPVOID  phInstance
	);
typedef F_NPSGetHandleFromInstance *PF_NPSGetHandleFromInstance;

F_NPSGetHandleFromInstance NPSGetHandleFromInstance;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

