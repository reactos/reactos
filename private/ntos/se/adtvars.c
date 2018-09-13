/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtvars.c

Abstract:

    Auditing - Private Variables

Author:

    Scott Birrell       (ScottBi)       November 14, 1991

Environment:

    Kernel Mode only

Revision History:

--*/

#include <nt.h>
#include <ntos.h>
#include "sep.h"
#include "adt.h"
#include "adtp.h"


//
// Auditing State.  This contains the Auditing Mode and the array of
// Event Auditing Options
//

POLICY_AUDIT_EVENTS_INFO SepAdtState;

//
// Audit Log Information
//

POLICY_AUDIT_LOG_INFO SepAdtLogInformation;

//
// High and low water marks to control the length of the audit queue
// These are initialized to their default values in case we can't get
// them out of the registry.
//

ULONG SepAdtMaxListLength = 0x3000;
ULONG SepAdtMinListLength = 0x2000;

ULONG SepAdtCurrentListLength = 0;

//
// Number of events discarded
//

ULONG SepAdtCountEventsDiscarded = 0;

BOOLEAN SepAdtDiscardingAudits = FALSE;

//
// see note in adtp.h regarding SEP_AUDIT_OPTIONS
//
SEP_AUDIT_OPTIONS SepAuditOptions = { 0 };

