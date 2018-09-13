/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    eventp.h

Abstract:

    Private include file for eventlog service

Author:

    Rajen Shah (rajens) 12-Jul-1991

Revision History:

--*/

#define UNICODE             // This service uses unicode APIs

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntelfapi.h>
#include <netevent.h>       // Manifest constants for Events

#include <windows.h>
#include <winsvc.h>

#include <lmcons.h>
#include <lmerr.h>
#include <rpc.h>
#include <svcs.h>     // SVCS_ENTRY_POINT, PSVCS_GLOBAL_DATA
#include <regstr.h>

#include <elf.h>

#include <elfdef.h>
#include <elfcommn.h>
#include <elfproto.h>
#include <elfextrn.h>
