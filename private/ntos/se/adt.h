/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adt.h

Abstract:

    Auditing - Defines, Fuction Prototypes and Macro Functions.
               These are public to the Security Component only.

Author:

    Scott Birrell       (ScottBi)       January 17, 1991

Environment:

Revision History:

--*/

#include <ntlsa.h>


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Auditing Routines visible to rest of Security Component outside Auditing //
// subcomponent.                                                            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


/*++

BOOLEAN
SepAdtEventOnSuccess(
    IN POLICY_AUDIT_EVENT_TYPE AuditEventType
    )

Routine Description:

    This macro function checks if a given Audit Event Type is enabled for
    Auditing of successful occurrences of the Event.

Arguments:

    AuditEventType - Specifies the type of the Audit Event to be checked.

Return Value:

    BOOLEAN - TRUE if the event type is enabled for auditing of successful
        occurrences of the event, else FALSE
--*/

#define SepAdtEventOnSuccess(AuditEventType)                               \
            (SepAdtState.EventAuditingOptions[AuditEventType] &            \
                POLICY_AUDIT_EVENT_SUCCESS)


/*++

BOOLEAN
SepAdtEventOnFailure(
    IN POLICY_AUDIT_EVENT_TYPE AuditEventType
    )

Routine Description:

    This macro function checks if a given Audit Event Type is enabled for
    Auditing of unsuccessful attempts to cause an event of the given type
    to occur.

Arguments:

    AuditEventType - Specifies the type of the Audit Event to be checked.

Return Value:

    BOOLEAN - TRUE if the event type is enabled for auditing of unsuccessful
        attempts to make the event type occur, else FALSE
--*/

#define SepAdtEventOnFailure(AuditEventType)                               \
            (SepAdtState.EventAuditingOptions[AuditEventType] &            \
            POLICY_AUDIT_EVENT_FAILURE)

/*++

BOOLEAN
SepAdtAuditingEvent(
    IN POLICY_AUDIT_EVENT_TYPE AuditEventType
    )

Routine Description:

    This macro function checks if a given Audit Event Type is enabled for
    Auditing.

Arguments:

    AuditEventType - Specifies the type of the Audit Event to be checked.

Return Value:

    BOOLEAN - TRUE if the event type is enabled for auditing, else FALSE.

--*/

#define SepAdtAuditingEvent(AuditEventType)                     \
            (SepAdtEventOnSuccess(AuditEventType) ||            \
            (SepAdtEventOnFailure(AuditEventType))

/*++

BOOLEAN
SepAdtAuditingEnabled()

Routine Description:

    This macro function tests if auditing is enabled.

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if auditing is enabled, else FALSE

--*/

#define SepAdtAuditingEnabled() (SepAdtState.AuditingMode == TRUE)


/*++

BOOLEAN
SepAdtAuditingDisabled()

Routine Description:

    This macro function tests if auditing is disabled.

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if auditing is disabled, else FALSE

--*/

#define SepAdtAuditingDisabled() (!SepAdtAuditingEnabled)

//
// Audit Event Information array.  Although internal to the Auditing
// Subcomponent, this structure is exported to all of Security so that the
// above macro functions can be used to access it efficiently from there.
//

extern POLICY_AUDIT_EVENTS_INFO SepAdtState;

BOOLEAN
SepAdtInitializePhase0();

BOOLEAN
SepAdtInitializePhase1();

//VOID
//SepAdtLogAuditRecord(
//    IN POLICY_AUDIT_EVENT_TYPE AuditEventType,
//    IN PVOID AuditInformation
//    );

VOID
SepAdtLogAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    );

NTSTATUS
SepAdtCopyToLsaSharedMemory(
    IN HANDLE LsaProcessHandle,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PVOID *LsaBufferAddress
    );
