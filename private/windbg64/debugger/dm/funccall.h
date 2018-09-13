/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    funccall.h

Abstract:

    Contains the types and protypes for funccall.c

Author:

    Jim Schaad (jimsch) 05-06-92

Environment:

    Win32 - User

--*/

/*
 *
 */

typedef struct _EXECUTE_OBJECT_DM {
    ADDR        addrStart;      /* Starting address of function call     */
    ADDR        addrStack;      /* Starting stack offset                 */
    BOOL        fIgnoreEvents;  /* Ignore events during execution        */
    HTHDX       hthd;           /* Thread for evaluating fucntion        */
    BREAKPOINT * lpbp;          /* Pointer to breakpoint at starting addres */
    BREAKPOINT * pbpSave;       /* Breakpoint thread is on at start     */
    int         tmp;
} EXECUTE_OBJECT_DM;

typedef EXECUTE_OBJECT_DM FAR * LPEXECUTE_OBJECT_DM;

/**********************************************************************/

extern  VOID    ProcessSetupExecuteCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessStartExecuteCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    ProcessCleanUpExecuteCmd(HPRCX, HTHDX, LPDBB);
extern  VOID    EvntException(DEBUG_EVENT64 *, HTHDX);
extern  VOID    EvntExitProcess(DEBUG_EVENT64 *, HTHDX);
extern  VOID    EvntBreakpoint(DEBUG_EVENT64 *, HTHDX);

extern  XOSD    SetupFunctionCall(LPEXECUTE_OBJECT_DM, LPEXECUTE_STRUCT);
extern  BOOL    CompareStacks(LPEXECUTE_OBJECT_DM);
