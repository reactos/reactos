//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       sc.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-03-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SC_H__
#define __SC_H__



typedef enum _SC_EVENT_TYPE {
    ScInsert,
    ScRemove
} SC_EVENT_TYPE ;

typedef struct _SC_DATA {
    WLX_SC_NOTIFICATION_INFO ScInfo ;
} SC_DATA, * PSC_DATA ;

typedef struct _SC_EVENT {
    LIST_ENTRY      List ;
    SC_EVENT_TYPE   Type ;
    PSC_DATA        Data ;    
} SC_EVENT, * PSC_EVENT ;

typedef struct _SC_THREAD_CONTROL {
    PTERMINAL   pTerm ;
    HANDLE      Thread ;
    HANDLE      Callback ;
} SC_THREAD_CONTROL, * PSC_THREAD_CONTROL ;


BOOL
ScInit(
    VOID
    );

BOOL
ScAddEvent(
    SC_EVENT_TYPE   Type,
    PSC_DATA        Data
    );

BOOL
ScRemoveEvent(
    SC_EVENT_TYPE * Type,
    PSC_DATA *      Data
    );

VOID
ScFreeEventData(
    PSC_DATA Data
    );

BOOL
IsSmartCardReaderPresent(
    PTERMINAL pTerm
    );

BOOL
WaitForSmartCardServiceToStart(
    DWORD dwTimeout
    );

BOOL
StartListeningForSC(
    PTERMINAL pTerm
    );

BOOL
StopListeningForSC(
    PTERMINAL pTerm
    );

extern PUCHAR SCData ;

#endif
