/****************************** Module Header ******************************\
* Module Name: logoff.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis user to implement logoff functionality of winlogon
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/



// Exported function prototypes
//


extern BOOL SystemProcessShutdown ;

int
InitiateLogoff(
    PTERMINAL pTerm,
    LONG Flags
    );

BOOL
Logoff(
    PTERMINAL pTerm,
    int Result
    );

BOOL
ShutdownMachine(
    PTERMINAL pTerm,
    int Flags
    );

VOID
RebootMachine(
    PTERMINAL pTerm
    );

VOID
PowerdownMachine(
    PTERMINAL pTerm
    );

BOOL
LogoffLockInit(
    VOID
    );

typedef DWORD   (*PWNETNUKECONN) (
                    HWND
                    );

typedef DWORD   (*PWNETOPENENUM) (
                    DWORD,
                    DWORD,
                    DWORD,
                    LPNETRESOURCE,
                    LPHANDLE
                    );

typedef DWORD   (*PWNETENUMRESOURCE) (
                    HANDLE,
                    LPDWORD,
                    LPVOID,
                    LPDWORD
                    );

typedef DWORD   (*PWNETCLOSEENUM) (
                    HANDLE
                    );

typedef DWORD
(APIENTRY * PRASENUMCONNECTIONSW)( LPRASCONNW, LPDWORD, LPDWORD );

typedef DWORD
(APIENTRY * PRASHANGUPW) ( HRASCONN );
