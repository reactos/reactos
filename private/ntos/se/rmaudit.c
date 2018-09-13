/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rmaudit.c

Abstract:

   This module contains the Reference Monitor Auditing Command Workers.
   These workers call functions in the Auditing sub-component to do the real
   work.

Author:

    Scott Birrell      (ScottBi)        November 14,1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include <nt.h>
#include <ntlsa.h>
#include <ntos.h>
#include <ntrmlsa.h>
#include "sep.h"
#include "adt.h"
#include "adtp.h"
#include "rmp.h"

VOID
SepRmSetAuditLogWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepRmSetAuditEventWrkr)
#pragma alloc_text(PAGE,SepRmSetAuditLogWrkr)
#endif



VOID
SepRmSetAuditEventWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function carries out the Reference Monitor Set Audit Event
    Command.  This command enables or disables auditing and optionally
    sets the auditing events.


Arguments:

    CommandMessage - Pointer to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmSetAuditStateCommand) and a single command
        parameter in structure form.

    ReplyMessage - Pointer to structure containing RM reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    VOID

--*/

{

    PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
    POLICY_AUDIT_EVENT_TYPE EventType;

    PAGED_CODE();

    SepAdtInitializeBounds();

    ReplyMessage->ReturnedStatus = STATUS_SUCCESS;

    //
    // Strict check that command is correct one for this worker.
    //

    ASSERT( CommandMessage->CommandNumber == RmAuditSetCommand );

    //
    // Extract the AuditingMode flag and put it in the right place.
    //

    SepAdtAuditingEnabled = (((PLSARM_POLICY_AUDIT_EVENTS_INFO) CommandMessage->CommandParams)->
                                AuditingMode);

    //
    // For each element in the passed array, process changes to audit
    // nothing, and then success or failure flags.
    //

    EventAuditingOptions = ((PLSARM_POLICY_AUDIT_EVENTS_INFO) CommandMessage->CommandParams)->
                           EventAuditingOptions;


    for ( EventType=AuditEventMinType;
          EventType <= AuditEventMaxType;
          EventType++ ) {

        SeAuditingState[EventType].AuditOnSuccess = FALSE;
        SeAuditingState[EventType].AuditOnFailure = FALSE;

        if ( EventAuditingOptions[EventType] & POLICY_AUDIT_EVENT_SUCCESS ) {

            SeAuditingState[EventType].AuditOnSuccess = TRUE;
        }

        if ( EventAuditingOptions[EventType] & POLICY_AUDIT_EVENT_FAILURE ) {

            SeAuditingState[EventType].AuditOnFailure = TRUE;
        }
    }

    //
    // Set the flag to indicate that we're auditing detailed events.
    // This is merely a timesaver so we can skip auditing setup in
    // time critical places like process creation.
    //

    //
    // Despite what the UI may imply, we never audit failures for detailed events, since
    // none of them can fail for security related reasons, and we're not interested in
    // auditing out of memory errors and stuff like that.  So just set this flag when
    // they want to see successes and ignore the failure case.
    //
    // We may have to revisit this someday.
    //

    if ( SeAuditingState[AuditCategoryDetailedTracking].AuditOnSuccess && SepAdtAuditingEnabled ) {

        SeDetailedAuditing = TRUE;

    } else {

        SeDetailedAuditing = FALSE;
    }

    return;
}



VOID
SepRmSetAuditLogWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function carries out the Reference Monitor Set Audit Log
    Command.  This command stores parameters related to the Audit Log.

Arguments:

    CommandMessage - Pointer to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmSetAuditStateCommand) and a single command
        parameter in structure form.

    ReplyMessage - Pointer to structure containing RM reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    None.  A status code is returned in ReplyMessage->ReturnedStatus

--*/

{
    //
    // Strict check that command is correct one for this worker.
    //

/* BUGWARNING - SCOTTBI - Auditing is disabled

    ASSERT( CommandMessage->CommandNumber == RmSetAuditLogCommand );

*/

    PAGED_CODE();

#if DBG
    DbgPrint("Security: RM Set Audit Log Command Received\n");
#endif

    //
    // Call private function in Auditing Sub-component to do the work.
    //

    SepAdtSetAuditLogInformation(
        (PPOLICY_AUDIT_LOG_INFO) CommandMessage->CommandParams
        );

    ReplyMessage->ReturnedStatus = STATUS_SUCCESS;
}

