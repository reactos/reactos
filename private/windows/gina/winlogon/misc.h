//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       misc.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-25-94   RichardW   Created
//
//----------------------------------------------------------------------------


DWORD
ReportWinlogonEvent(
    IN PTERMINAL pTerm,
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    );


int TimeoutMessageBox(
    PTERMINAL pTerm,
    HWND hWnd,
    UINT IdText,
    UINT IdCaption,
    UINT wType
    );


void
MainLoop(PTERMINAL pTerm);
