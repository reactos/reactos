/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    removabl.h

Abstract:

    This file contains prototype definitions of services exported
    from removabl.c

    
Author:

    Jim Kelly (JimK) Oct-19-1994

Environment:

    Part of Winlogon.

Revision History:


--*/

#ifndef  _REMOVABL_
#define  _REMOVABL_

BOOL
RmvInitializeRemovableMediaSrvcs(
    VOID
    );

VOID
RmvAllocateRemovableMedia(
    IN  PSID            AllocatorId
    );



#endif //_REMOVABL_
