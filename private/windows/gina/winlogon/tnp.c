//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       tnp.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-04-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
    HANDLE Event ;
    OBJECT_ATTRIBUTES   EventAttr;
    UNICODE_STRING      usName;
    NTSTATUS            Status;
    ULONG               ulWin32Error;

    RtlInitUnicodeString(&usName, L"\\Security\\NetworkProviderLoad" );

    InitializeObjectAttributes(&EventAttr, &usName,
                                OBJ_CASE_INSENSITIVE,
                                NULL, NULL);

    Status = NtOpenEvent(   &Event,
                            EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE ,
                            &EventAttr);

    if (!NT_SUCCESS(Status))
    {
        ulWin32Error = RtlNtStatusToDosError( Status );
        SetLastError(ulWin32Error);
        return( 0 );
    }


    SetEvent( Event );

    CloseHandle( Event );

    return 0 ;
}
