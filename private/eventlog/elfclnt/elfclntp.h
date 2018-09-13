/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    elfclntp.h

Abstract:

    Common include file for all the client-side modules for the
    event logging facility.

Author:

    Rajen Shah	(rajens)    29-Jul-1991


Revision History:

    29-Jul-1991 	RajenS
        Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <rpc.h>
#include <ntrpcp.h>
#include <lmcons.h>
#include <lmerr.h>

#include <elf.h>
#include <elfcommn.h>


DWORD
ElfpGetComputerName (
    OUT  LPSTR   *ComputerNamePtrA,
    OUT  LPWSTR  *ComputerNamePtrW
    );
