/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    adtutil.c - Security Auditing - Utility Routines

Abstract:

    This Module contains miscellaneous utility routines private to the
    Security Auditing Component.

Author:

    Robert Reichel      (robertre)     September 10, 1991

Environment:

    Kernel Mode

Revision History:

--*/

#include <nt.h>
#include "tokenp.h"
#include "adt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepDumpString)
#endif


VOID
SepDumpString(
    IN PUNICODE_STRING String
    )

{
    PAGED_CODE();

    if ( String == NULL) {
        KdPrint(("<NULL>"));
        return;
    }

    KdPrint(("%Z",String->Buffer));

}

