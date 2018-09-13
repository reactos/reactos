/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sepaudit.c

Abstract:

    This Module implements the audit and alarm procedures that are
    private to the security component.

Author:

    Robert Reichel      (robertre)     September 10, 1991

Environment:

    Kernel Mode

Revision History:

--*/

#include <nt.h>
#include <ntlsa.h>
#include <msaudite.h>
#include "tokenp.h"
#include "adt.h"
#include "adtp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeAuditHandleDuplication)
// #pragma alloc_text(PAGE,SepAdtAuditThisEvent)
#pragma alloc_text(PAGE,SepAdtPrivilegeObjectAuditAlarm)
#pragma alloc_text(PAGE,SepAdtPrivilegedServiceAuditAlarm)
#pragma alloc_text(PAGE,SepAdtOpenObjectAuditAlarm)
#pragma alloc_text(PAGE,SepAdtOpenObjectForDeleteAuditAlarm)
#pragma alloc_text(PAGE,SepAdtHandleAuditAlarm)
#pragma alloc_text(PAGE,SepAdtObjectReferenceAuditAlarm)
#pragma alloc_text(PAGE,SepQueryNameString)
#pragma alloc_text(PAGE,SepQueryTypeString)
#pragma alloc_text(PAGE,SeAuditProcessCreation)
#pragma alloc_text(PAGE,SeAuditProcessExit)
#pragma alloc_text(PAGE,SepAdtGenerateDiscardAudit)
#endif


#define SepSetParmTypeSid( AuditParameters, Index, Sid )                       \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeSid;         \
        (AuditParameters).Parameters[(Index)].Length = SeLengthSid( (Sid) );   \
        (AuditParameters).Parameters[(Index)].Address = (Sid);                 \
    }


#define SepSetParmTypeString( AuditParameters, Index, String )                 \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeString;      \
        (AuditParameters).Parameters[(Index)].Length =                         \
                sizeof(UNICODE_STRING)+(String)->Length;                       \
        (AuditParameters).Parameters[(Index)].Address = (String);              \
    }


#define SepSetParmTypeFileSpec( AuditParameters, Index, String )               \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeFileSpec;    \
        (AuditParameters).Parameters[(Index)].Length =                         \
                sizeof(UNICODE_STRING)+(String)->Length;                       \
        (AuditParameters).Parameters[(Index)].Address = (String);              \
    }

#define SepSetParmTypeUlong( AuditParameters, Index, Ulong )                   \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeUlong;       \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Ulong) );     \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG)(Ulong);        \
    }

#define SepSetParmTypeNoLogon( AuditParameters, Index )                        \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeNoLogonId;   \
    }

#define SepSetParmTypeLogonId( AuditParameters, Index, LogonId )                   \
    {                                                                          \
        LUID UNALIGNED * TmpLuid;                                                           \
                                                                                 \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeLogonId;       \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (LogonId) );     \
        TmpLuid = (LUID UNALIGNED *)(&(AuditParameters).Parameters[(Index)].Data[0]);                  \
        *TmpLuid = (LogonId);                                                     \
    }

#define SepSetParmTypeAccessMask( AuditParameters, Index, AccessMask, ObjectTypeIndex )  \
    {                                                                                    \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeAccessMask;            \
        (AuditParameters).Parameters[(Index)].Length = sizeof( ACCESS_MASK );            \
        (AuditParameters).Parameters[(Index)].Data[0] = (AccessMask);                    \
        (AuditParameters).Parameters[(Index)].Data[1] = (ObjectTypeIndex);               \
    }

#define SepSetParmTypePrivileges( AuditParameters, Index, Privileges )                      \
    {                                                                                       \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypePrivs;                    \
        (AuditParameters).Parameters[(Index)].Length = SepPrivilegeSetSize( (Privileges) ); \
        (AuditParameters).Parameters[(Index)].Address = (Privileges);                       \
    }

#define SepSetParmTypeObjectTypes( AuditParameters, Index, ObjectTypes, ObjectTypeCount, ObjectTypeIndex )             \
    {                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeObjectTypes;            \
        (AuditParameters).Parameters[(Index)].Length = sizeof( SE_ADT_OBJECT_TYPE ) * (ObjectTypeCount);\
        (AuditParameters).Parameters[(Index)].Address = (ObjectTypes);                    \
        (AuditParameters).Parameters[(Index)].Data[1] = (ObjectTypeIndex);               \
    }




BOOLEAN
SepAdtPrivilegeObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName OPTIONAL,
    IN PVOID HandleId,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN PVOID ProcessId,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET CapturedPrivileges,
    IN BOOLEAN AccessGranted
    )

/*++

Routine Description:

    Implements NtPrivilegeObjectAuditAlarm after parameters have been
    captured.

    This routine is used to generate audit and alarm messages when an
    attempt is made to perform privileged operations on a protected
    subsystem object after the object is already opened.  This routine may
    result in several messages being generated and sent to Port objects.
    This may result in a significant latency before returning.  Design of
    routines that must call this routine must take this potential latency
    into account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - New handle ID

    Parameter[3] - Subject's process id

    Parameter[4] - Subject's primary authentication ID

    Parameter[5] - Subject's client authentication ID

    Parameter[6] - Privileges used for open

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    CapturedPrivileges - The set of privileges required for the requested
        operation.  Those privileges that were held by the subject are
        marked using the UsedForAccess flag of the attributes
        associated with each privilege.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

Return value:

--*/
{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID CapturedUserSid;
    LUID ClientAuthenticationId;
    LUID PrimaryAuthenticationId;

    PAGED_CODE();

    //
    // Determine if we are auditing the use of privileges
    //

    if ( SepAdtAuditThisEvent( AuditCategoryPrivilegeUse, &AccessGranted ) &&
         SepFilterPrivilegeAudits( CapturedPrivileges )) {

        if ( ARGUMENT_PRESENT( ClientToken )) {

            CapturedUserSid = SepTokenUserSid( ClientToken );
            ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );

        } else {

            CapturedUserSid = SepTokenUserSid( PrimaryToken );
        }

        if ( RtlEqualSid( SeLocalSystemSid, CapturedUserSid )) {

            return (FALSE);
        }

        PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_PRIVILEGE_USE;
        AuditParameters.AuditId = SE_AUDITID_PRIVILEGED_OBJECT;
        AuditParameters.ParameterCount = 0;

        if ( AccessGranted ) {

            AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

        } else {

            AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
        }

        //
        //    Parameter[0] - User Sid
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[1] - Subsystem name (if available)
        //

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[1] - Subsystem name (if available)
        //

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[2] - New handle ID
        //

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)HandleId) );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[3] - Subject's process id
        //

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)ProcessId) );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[4] - Subject's primary authentication ID
        //

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[5] - Subject's client authentication ID
        //

        if ( ARGUMENT_PRESENT( ClientToken )) {

            SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

        } else {

            SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount );
        }

        AuditParameters.ParameterCount++;

        //
        //    Parameter[6] - Privileges used for open
        //

        if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

            SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
        }

        AuditParameters.ParameterCount++;

        SepAdtLogAuditRecord( &AuditParameters );

        return( TRUE );

    }

    return( FALSE );
}


VOID
SepAdtPrivilegedServiceAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PUNICODE_STRING CapturedServiceName,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN PPRIVILEGE_SET CapturedPrivileges,
    IN BOOLEAN AccessGranted
    )

/*++

Routine Description:

    This routine is the active part of NtPrivilegedServiceAuditAlarm.

    This routine is used to generate audit and alarm messages when an
    attempt is made to perform privileged system service operations.  This
    routine may result in several messages being generated and sent to Port
    objects.  This may result in a significant latency before returning.
    Design of routines that must call this routine must take this potential
    latency into account.  This may have an impact on the approach taken
    for data structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.  The test for this privilege is assumed to
    have occurred at a higher level.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Subject's primary authentication ID

    Parameter[3] - Subject's client authentication ID

    Parameter[4] - Privileges used for open

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling the routine.

    ServiceName - Supplies a name of the privileged subsystem service.  For
        example, "RESET RUNTIME LOCAL SECURITY POLICY" might be specified
        by a Local Security Authority service used to update the local
        security policy database.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    Privileges - Points to a set of privileges required to perform the
        privileged operation.  Those privileges that were held by the
        subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.


Return value:


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID CapturedUserSid;
    LUID ClientAuthenticationId;
    LUID PrimaryAuthenticationId;
    PUNICODE_STRING SubsystemName;

    PAGED_CODE();

    //
    // Determine if we are auditing privileged services
    //

    if ( SepAdtAuditThisEvent( AuditCategoryPrivilegeUse, &AccessGranted )) {

        if ( ARGUMENT_PRESENT( ClientToken )) {

            CapturedUserSid = SepTokenUserSid( ClientToken );
            ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );

        } else {

            CapturedUserSid = SepTokenUserSid( PrimaryToken );
        }

        PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

        if ( !ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SubsystemName = &SeSubsystemName;

        } else {

            SubsystemName = CapturedSubsystemName;
        }

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_PRIVILEGE_USE;
        AuditParameters.AuditId = SE_AUDITID_PRIVILEGED_SERVICE;
        AuditParameters.ParameterCount = 0;

        if ( AccessGranted ) {

            AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

        } else {

            AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
        }


    //
    //    Parameter[0] - User Sid
    //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

        AuditParameters.ParameterCount++;

    //
    //    Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, SubsystemName );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[2] - Server
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, SubsystemName );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[3] - Service name (if available)
    //

    if ( ARGUMENT_PRESENT( CapturedServiceName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedServiceName );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[3] - Subject's primary authentication ID
    //


    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

    AuditParameters.ParameterCount++;


    //
    //    Parameter[4] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount );
    }

    AuditParameters.ParameterCount++;


    //
    //    Parameter[5] - Privileges used for open
    //


    if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

        SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
    }

    AuditParameters.ParameterCount++;


    SepAdtLogAuditRecord( &AuditParameters );

    }

}






BOOLEAN
SepAdtOpenObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID *HandleId OPTIONAL,
    IN PUNICODE_STRING CapturedObjectTypeName,
    IN PVOID Object OPTIONAL,
    IN PUNICODE_STRING CapturedObjectName OPTIONAL,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess,
    IN PLUID OperationId,
    IN PPRIVILEGE_SET CapturedPrivileges OPTIONAL,
    IN BOOLEAN ObjectCreated,
    IN BOOLEAN AccessGranted,
    IN BOOLEAN GenerateAudit,
    IN BOOLEAN GenerateAlarm,
    IN HANDLE ProcessID,
    IN POLICY_AUDIT_EVENT_TYPE AuditType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PACCESS_MASK GrantedAccessArray OPTIONAL
    )

/*++

    Routine Description:

    Implements NtOpenObjectAuditAlarm after parameters have been captured.

    This routine is used to generate audit and alarm messages when an
    attempt is made to access an existing protected subsystem object or
    create a new one.  This routine may result in several messages being
    generated and sent to Port objects.  This may result in a significant
    latency before returning.  Design of routines that must call this
    routine must take this potential latency into account.  This may have
    an impact on the approach taken for data structure mutex locking, for
    example.  This API requires the caller have SeTcbPrivilege privilege.
    The test for this privilege is always against the primary token of the
    calling process, not the impersonation token of the thread.


    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Server name (if available)

    Parameter[3] - Object Type Name

    Parameter[4] - Object Name

    Parameter[5] - New handle ID

    Parameter[6] - Subject's process id

    Parameter[7] - Subject's primary authentication ID

    Parameter[8] - Subject's client authentication ID

    Parameter[9] - DesiredAccess mask

    Parameter[10] - Privileges used for open

    Parameter[11] - Guid/Level/AccessMask of objects/property sets/properties accesses.

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.  If the access attempt was not successful (AccessGranted is
        FALSE), then this parameter is ignored.

    CapturedObjectTypeName - Supplies the name of the type of object being
        accessed.

    CapturedObjectName - Supplies the name of the object the client
        accessed or attempted to access.

    CapturedSecurityDescriptor - A pointer to the security descriptor of
        the object being accessed.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GrantedAccess - The mask of accesses that were actually granted.

    CapturedPrivileges - Optionally points to a set of privileges that were
        required for the access attempt.  Those privileges that were held
        by the subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    ObjectCreation - A boolean flag indicating whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

    GenerateOnClose - Points to a boolean that is set by the audit
        generation routine and must be passed to NtCloseObjectAuditAlarm()
        when the object handle is closed.

    GenerateAudit - Indicates if we should generate an audit for this operation.

    GenerateAlarm - Indicates if we should generate an alarm for this operation.

    AuditType - Specifies the type of audit to be generated.  Valid values
        are: AuditCategoryObjectAccess and AuditCategoryDirectoryServiceAccess.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GrantedAccessArray - If non NULL, specifies an array of access mask granted
        to each object in ObjectTypeList.

Return Value:

    Returns TRUE if audit is generated, FALSE otherwise.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    ULONG ObjectTypeIndex;
    PSID CapturedUserSid;
    LUID PrimaryAuthenticationId;
    LUID ClientAuthenticationId;
    PSE_ADT_OBJECT_TYPE AdtObjectTypeBuffer = NULL;

    PAGED_CODE();

    if ( ARGUMENT_PRESENT( ClientToken )) {

        CapturedUserSid = SepTokenUserSid( ClientToken );
        ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );

    } else {

        CapturedUserSid = SepTokenUserSid( PrimaryToken );
    }

    PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

    //
    // A completely zero'd entry will be interpreted
    // as a "null string" or not supplied parameter.
    //
    // Initializing the entire array up front will allow
    // us to avoid filling in each not supplied entry.
    //

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    ASSERT( ( AuditType == AuditCategoryObjectAccess ) ||
            ( AuditType == AuditCategoryDirectoryServiceAccess ) );
    
    if (AuditType == AuditCategoryObjectAccess) {
        
        AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
    } else {

        AuditParameters.CategoryId = SE_CATEGID_DS_ACCESS;
    }

    AuditParameters.AuditId = SE_AUDITID_OPEN_HANDLE;
    AuditParameters.ParameterCount = 0;

    if ( AccessGranted ) {

        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    } else {

        AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
    }

    //
    //  Parameter[0] - User Sid
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[2] - Object Server (if available)
    //

    if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[3] - Object Type Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectTypeName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedObjectTypeName );
        ObjectTypeIndex = AuditParameters.ParameterCount;
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[4] - Object Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectName )) {

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, CapturedObjectName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[5] - New handle ID
    //

    if ( ARGUMENT_PRESENT( HandleId )) {

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)*HandleId) );
    }

    AuditParameters.ParameterCount++;

    if ( ARGUMENT_PRESENT( OperationId )) {

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).HighPart );

        AuditParameters.ParameterCount++;

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).LowPart );

        AuditParameters.ParameterCount++;

    } else {

        AuditParameters.ParameterCount += 2;
    }

    //
    //  Parameter[6] - Subject's process id
    //

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)ProcessID) );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[7] - Subject's primary authentication ID
    //

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[8] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount  );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[9] - DesiredAccess mask
    //

    if ( AccessGranted ) {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, GrantedAccess, ObjectTypeIndex );

    } else {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, DesiredAccess, ObjectTypeIndex );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[10] - Privileges used for open
    //

    if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

        SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[11] - ObjectTypes of Audited objects/parameter sets/parameters
    //

    if ( ObjectTypeListLength != 0 ) {
        ULONG GuidCount;
        ULONG i;
        USHORT FlagMask = AccessGranted ? OBJECT_SUCCESS_AUDIT : OBJECT_FAILURE_AUDIT;

        //
        // Count the number of GUIDs to audit.
        //

        GuidCount = 0;
        for ( i=0; i<ObjectTypeListLength; i++ ) {

            if ( i == 0 ) {
                GuidCount++;
            } else if ( ObjectTypeList[i].Flags & FlagMask ) {
                GuidCount ++;
            } 
        }

        //
        // If there are any Guids to audit,
        //  copy them into a locally allocated buffer.
        //

        if ( GuidCount > 0 ) {

            AdtObjectTypeBuffer = ExAllocatePoolWithTag( PagedPool, GuidCount * sizeof(SE_ADT_OBJECT_TYPE), 'pAeS' );

            //
            // If the buffer can be allocated,
            //  fill it in.
            // If not,
            //  generate a truncated audit.
            //

            if ( AdtObjectTypeBuffer != NULL ) {

                //
                // Copy the GUIDs and optional access masks to the buffer.
                //

                GuidCount = 0;
                for ( i=0; i<ObjectTypeListLength; i++ ) {

                    if ( ( i > 0 ) && !( ObjectTypeList[i].Flags & FlagMask ) ) {

                        continue;
                        
                    } else {
                    
                        AdtObjectTypeBuffer[GuidCount].ObjectType = ObjectTypeList[i].ObjectType;
                        AdtObjectTypeBuffer[GuidCount].Level      = ObjectTypeList[i].Level;

                        if ( i == 0 ) {
                            //
                            // Always copy the GUID representing the object itself.
                            //  Mark it as a such to avoid including it in the audit.
                            //
                            AdtObjectTypeBuffer[GuidCount].Flags      = SE_ADT_OBJECT_ONLY;
                            AdtObjectTypeBuffer[GuidCount].AccessMask = 0;

                        } else  {

                            AdtObjectTypeBuffer[GuidCount].Flags = 0;
                            if ( ARGUMENT_PRESENT(GrantedAccessArray) && AccessGranted ) {

                                AdtObjectTypeBuffer[GuidCount].AccessMask = GrantedAccessArray[i];
                            }
                        }
                        GuidCount ++;
                    }
                }

                //
                // Store the Object Types.
                //

                SepSetParmTypeObjectTypes( AuditParameters, AuditParameters.ParameterCount, AdtObjectTypeBuffer, GuidCount, ObjectTypeIndex );
                AuditParameters.ParameterCount ++;
                AuditParameters.AuditId = SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE;
            }
        }

    }



    //
    // Audit it.
    //
    SepAdtLogAuditRecord( &AuditParameters );

    if ( AdtObjectTypeBuffer != NULL ) {
        ExFreePool( AdtObjectTypeBuffer );
    }

    return( TRUE );
}


BOOLEAN
SepAdtOpenObjectForDeleteAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID *HandleId OPTIONAL,
    IN PUNICODE_STRING CapturedObjectTypeName,
    IN PVOID Object OPTIONAL,
    IN PUNICODE_STRING CapturedObjectName OPTIONAL,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess,
    IN PLUID OperationId,
    IN PPRIVILEGE_SET CapturedPrivileges OPTIONAL,
    IN BOOLEAN ObjectCreated,
    IN BOOLEAN AccessGranted,
    IN BOOLEAN GenerateAudit,
    IN BOOLEAN GenerateAlarm,
    IN HANDLE ProcessID
    )

/*++

    Routine Description:

    Implements SeOpenObjectForDeleteAuditAlarm after parameters have been
    captured.

    This routine is used to generate audit and alarm messages when an
    attempt is made to access an existing protected subsystem object or
    create a new one.  This routine may result in several messages being
    generated and sent to Port objects.  This may result in a significant
    latency before returning.  Design of routines that must call this
    routine must take this potential latency into account.  This may have
    an impact on the approach taken for data structure mutex locking, for
    example.  This API requires the caller have SeTcbPrivilege privilege.
    The test for this privilege is always against the primary token of the
    calling process, not the impersonation token of the thread.


    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Server name (if available)

    Parameter[3] - Object Type Name

    Parameter[4] - Object Name

    Parameter[5] - New handle ID

    Parameter[6] - Subject's process id

    Parameter[7] - Subject's primary authentication ID

    Parameter[8] - Subject's client authentication ID

    Parameter[9] - DesiredAccess mask

    Parameter[10] - Privileges used for open

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.  If the access attempt was not successful (AccessGranted is
        FALSE), then this parameter is ignored.

    CapturedObjectTypeName - Supplies the name of the type of object being
        accessed.

    CapturedObjectName - Supplies the name of the object the client
        accessed or attempted to access.

    CapturedSecurityDescriptor - A pointer to the security descriptor of
        the object being accessed.

    ClientToken - Optionally provides a pointer to the client token
        (only if the caller is currently impersonating)

    PrimaryToken - Provides a pointer to the caller's primary token.

    DesiredAccess - The desired access mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GrantedAccess - The mask of accesses that were actually granted.

    CapturedPrivileges - Optionally points to a set of privileges that were
        required for the access attempt.  Those privileges that were held
        by the subject are marked using the UsedForAccess flag of the
        attributes associated with each privilege.

    ObjectCreation - A boolean flag indicating whether the access will
        result in a new object being created if granted.  A value of TRUE
        indicates an object will be created, FALSE indicates an existing
        object will be opened.

    AccessGranted - Indicates whether the requested access was granted or
        not.  A value of TRUE indicates the access was granted.  A value of
        FALSE indicates the access was not granted.

    GenerateOnClose - Points to a boolean that is set by the audit
        generation routine and must be passed to NtCloseObjectAuditAlarm()
        when the object handle is closed.

    GenerateAudit - Indicates if we should generate an audit for this operation.

    GenerateAlarm - Indicates if we should generate an alarm for this operation.

Return Value:

    Returns TRUE if audit is generated, FALSE otherwise.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    ULONG ObjectTypeIndex;
    PSID CapturedUserSid;
    LUID PrimaryAuthenticationId;
    LUID ClientAuthenticationId;

    PAGED_CODE();

    if ( ARGUMENT_PRESENT( ClientToken )) {

        CapturedUserSid = SepTokenUserSid( ClientToken );
        ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );

    } else {

        CapturedUserSid = SepTokenUserSid( PrimaryToken );
    }

    PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

    //
    // A completely zero'd entry will be interpreted
    // as a "null string" or not supplied parameter.
    //
    // Initializing the entire array up front will allow
    // us to avoid filling in each not supplied entry.
    //

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
    AuditParameters.AuditId = SE_AUDITID_OPEN_OBJECT_FOR_DELETE;
    AuditParameters.ParameterCount = 0;

    if ( AccessGranted ) {

        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    } else {

        AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
    }

    //
    //  Parameter[0] - User Sid
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, CapturedUserSid );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[2] - Object Server (if available)
    //

    if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[3] - Object Type Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectTypeName )) {

        SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedObjectTypeName );
        ObjectTypeIndex = AuditParameters.ParameterCount;
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[4] - Object Name
    //

    if ( ARGUMENT_PRESENT( CapturedObjectName )) {

        SepSetParmTypeFileSpec( AuditParameters, AuditParameters.ParameterCount, CapturedObjectName );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[5] - New handle ID
    //

    if ( ARGUMENT_PRESENT( HandleId )) {

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)*HandleId) );
    }

    AuditParameters.ParameterCount++;

    if ( ARGUMENT_PRESENT( OperationId )) {

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).HighPart );

        AuditParameters.ParameterCount++;

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (*OperationId).LowPart );

        AuditParameters.ParameterCount++;

    } else {

        AuditParameters.ParameterCount += 2;
    }

    //
    //  Parameter[6] - Subject's process id
    //

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)ProcessID) );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[7] - Subject's primary authentication ID
    //

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, PrimaryAuthenticationId );

    AuditParameters.ParameterCount++;

    //
    //  Parameter[8] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, AuditParameters.ParameterCount  );
    }

    AuditParameters.ParameterCount++;

    //
    //  Parameter[9] - DesiredAccess mask
    //

    if ( AccessGranted ) {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, GrantedAccess, ObjectTypeIndex );

    } else {

        SepSetParmTypeAccessMask( AuditParameters, AuditParameters.ParameterCount, DesiredAccess, ObjectTypeIndex );
    }

    AuditParameters.ParameterCount++;

    //
    //    Parameter[10] - Privileges used for open
    //

    if ( (CapturedPrivileges != NULL) && (CapturedPrivileges->PrivilegeCount > 0) ) {

        SepSetParmTypePrivileges( AuditParameters, AuditParameters.ParameterCount, CapturedPrivileges );
    }

    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );

    return( TRUE );
}




VOID
SepAdtCloseObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID HandleId,
    IN PVOID Object,
    IN PSID UserSid,
    IN LUID AuthenticationId
    )

/*++

Routine Description:

    This routine implements NtCloseObjectAuditAlarm after parameters have
    been captured.

    This routine is used to generate audit and alarm messages when a handle
    to a protected subsystem object is deleted.  This routine may result in
    several messages being generated and sent to Port objects.  This may
    result in a significant latency before returning.  Design of routines
    that must call this routine must take this potential latency into
    account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.  It is assumed that this privilege has been
    tested at a higher level.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - New handle ID

    Parameter[3] - Subject's process id

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    Object - The address of the object being closed

    UserSid - The Sid identifying the current caller.



Return value:

    None.


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    BOOLEAN AccessGranted = TRUE;
    HANDLE ProcessId;

    PAGED_CODE();

    if ( SepAuditOptions.DoNotAuditCloseObjectEvents ) {

        return;
    }
    
    if ( SepAdtAuditThisEvent( AuditCategoryObjectAccess, &AccessGranted ) ) {

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
        AuditParameters.AuditId = SE_AUDITID_CLOSE_HANDLE;
        AuditParameters.ParameterCount = 0;
        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;


        //
        //  Parameter[0] - User Sid
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );

        AuditParameters.ParameterCount++;


        //
        //  Parameter[1] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //  Parameter[2] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //    Parameter[3] - New handle ID
        //

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)HandleId) );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[4] - Subject's process id
        //

        ProcessId =  PsProcessAuditId( PsGetCurrentProcess() );

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)ProcessId) );

        AuditParameters.ParameterCount++;

        SepAdtLogAuditRecord( &AuditParameters );

    }
}



VOID
SepAdtDeleteObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID HandleId,
    IN PVOID Object,
    IN PSID UserSid,
    IN LUID AuthenticationId
    )

/*++

Routine Description:

    This routine implements NtDeleteObjectAuditAlarm after parameters have
    been captured.

    This routine is used to generate audit and alarm messages when an object
    in a protected subsystem object is deleted.  This routine may result in
    several messages being generated and sent to Port objects.  This may
    result in a significant latency before returning.  Design of routines
    that must call this routine must take this potential latency into
    account.  This may have an impact on the approach taken for data
    structure mutex locking, for example.

    This API requires the caller have SeTcbPrivilege privilege.  The test
    for this privilege is always against the primary token of the calling
    process, allowing the caller to be impersonating a client during the
    call with no ill effects.  It is assumed that this privilege has been
    tested at a higher level.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Handle ID

    Parameter[3] - Subject's process id

Arguments:

    CapturedSubsystemName - Supplies a name string identifying the
        subsystem calling the routine.

    HandleId - A unique value representing the client's handle to the
        object.

    Object - The address of the object being closed

    UserSid - The Sid identifying the current caller.



Return value:

    None.


--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    BOOLEAN AccessGranted = TRUE;
    HANDLE ProcessId;

    PAGED_CODE();

    if ( SepAdtAuditThisEvent( AuditCategoryObjectAccess, &AccessGranted ) ) {

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
        AuditParameters.AuditId = SE_AUDITID_DELETE_OBJECT;
        AuditParameters.ParameterCount = 0;
        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;


        //
        //  Parameter[0] - User Sid
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );

        AuditParameters.ParameterCount++;


        //
        //  Parameter[1] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //  Parameter[2] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( CapturedSubsystemName )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, CapturedSubsystemName );
        }

        AuditParameters.ParameterCount++;

        //
        //    Parameter[3] - New handle ID
        //

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)HandleId) );

        AuditParameters.ParameterCount++;

        //
        //    Parameter[4] - Subject's process id
        //

        ProcessId =  PsProcessAuditId( PsGetCurrentProcess() );

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)ProcessId) );

        AuditParameters.ParameterCount++;

        SepAdtLogAuditRecord( &AuditParameters );

    }
}



//
//VOID
//SepAdtTraverseAuditAlarm(
//    IN PLUID OperationId,
//    IN PVOID DirectoryObject,
//    IN PSID UserSid,
//    IN LUID AuthenticationId,
//    IN ACCESS_MASK DesiredAccess,
//    IN PPRIVILEGE_SET Privileges OPTIONAL,
//    IN BOOLEAN AccessGranted,
//    IN BOOLEAN GenerateAudit,
//    IN BOOLEAN GenerateAlarm
//    )
///*++
//
//Routine Description:
//
//    This routine constructs an audit record to record that a request
//    to traverse a directory has occurred.
//
//Arguments:
//
//    OperationID - LUID identifying the operation in progress
//
//    DirectoryObject - Pointer to the directory being traversed.
//
//    UserSid - Provides the User Sid for the caller.
//
//    DesiredAccess - Mask to indicate the traverse access for this object
//        type.
//
//    Privileges - Optional parameter to indicate any privilges that the
//        subject may have used to gain access to the object.
//
//    AccessGranted - Indicates if the access was granted or denied based on
//        the access check or privilege check.
//
//    GenerateAudit - Indicates if we should generate an audit for this operation.
//
//    GenerateAlarm - Indicates if we should generate an alarm for this operation.
//
//Return Value:
//
//    None.
//
//--*/
//{
//    POLICY_AUDIT_TRAVERSE AuditTraverse;
//
//    UNREFERENCED_PARAMETER( GenerateAudit );
//    UNREFERENCED_PARAMETER( GenerateAlarm );
//    UNREFERENCED_PARAMETER( DirectoryObject );
//    UNREFERENCED_PARAMETER( DesiredAccess );
//
//    //
//    // BUGWARNING need a way to get the directory name from
//    // the directory object
//    //
//
//    AuditTraverse.AccessGranted = AccessGranted;
//    AuditTraverse.DirectoryName = NULL;
//    AuditTraverse.OperationId = *OperationId;
//    AuditTraverse.PrivilegeSet = Privileges;
//    AuditTraverse.UserSid = UserSid;
//    AuditTraverse.AuthenticationId = AuthenticationId;
//
////    SepAdtLogAuditRecord( AuditEventTraverse, &AuditTraverse );
//}




//
//VOID
//SepAdtCreateObjectAuditAlarm(
//    IN PLUID OperationID,
//    IN PUNICODE_STRING DirectoryName,
//    IN PUNICODE_STRING ComponentName,
//    IN PSID UserSid,
//    IN LUID AuthenticationId,
//    IN ACCESS_MASK DesiredAccess,
//    IN BOOLEAN AccessGranted,
//    IN BOOLEAN GenerateAudit,
//    IN BOOLEAN GenerateAlarm
//    )
///*++
//
//Routine Description:
//
//    description-of-function.
//
//Arguments:
//
//
//    GenerateAudit - Indicates if we should generate an audit for this operation.
//
//    GenerateAlarm - Indicates if we should generate an alarm for this operation.
//
//Return Value:
//
//    return-value - Description of conditions needed to return value. - or -
//    None.
//
//--*/
//
//{
//    POLICY_AUDIT_CREATE_OBJECT AuditCreateObject;
//
//    UNREFERENCED_PARAMETER( GenerateAudit );
//    UNREFERENCED_PARAMETER( GenerateAlarm );
//
//
//    AuditCreateObject.AccessGranted = AccessGranted;
//    AuditCreateObject.DesiredAccess = DesiredAccess;
//    AuditCreateObject.DirectoryName = DirectoryName;
//    AuditCreateObject.ComponentName = ComponentName;
//    AuditCreateObject.OperationId = *OperationID;
//    AuditCreateObject.UserSid = UserSid;
//    AuditCreateObject.AuthenticationId = AuthenticationId;
//
////    SepAdtLogAuditRecord( AuditEventCreateObject, &AuditCreateObject );
//}



//
//VOID
//SepAdtImplicitObjectAuditAlarm(
//    IN PLUID OperationId OPTIONAL,
//    IN PVOID Object,
//    IN PSID UserSid,
//    IN ACCESS_MASK DesiredAccess,
//    IN PPRIVILEGE_SET Privileges OPTIONAL,
//    IN BOOLEAN AccessGranted,
//    IN BOOLEAN GenerateAudit,
//    IN BOOLEAN GenerateAlarm
//    )
///*++
//
//Routine Description:
//
//    description-of-function.
//
//Arguments:
//
//    GenerateAudit - Indicates if we should generate an audit for this operation.
//
//    GenerateAlarm - Indicates if we should generate an alarm for this operation.
//
//
//Return Value:
//
//    None
//
//--*/
//
//{
//    POLICY_AUDIT_IMPLICIT_ACCESS AuditImplicitAccess;
//
//    UNREFERENCED_PARAMETER( GenerateAudit );
//    UNREFERENCED_PARAMETER( GenerateAlarm );
//
//
//    //
//    // BUGWARNING need a way to obtain the object type
//    //
//
//    AuditImplicitAccess.AccessGranted = AccessGranted;
//    AuditImplicitAccess.DesiredAccess = DesiredAccess;
//    AuditImplicitAccess.ObjectTypeName = NULL;
//    AuditImplicitAccess.OperationId = *OperationId;
//    AuditImplicitAccess.PrivilegeSet = Privileges;
//    AuditImplicitAccess.UserSid = UserSid;
//
//    SepAdtLogAuditRecord( AuditEventImplicitAccess, &AuditImplicitAccess );
//}





VOID
SepAdtHandleAuditAlarm(
    IN PUNICODE_STRING Source,
    IN LUID OperationId,
    IN HANDLE Handle,
    IN PSID UserSid
    )

/*++

Routine Description:

    Creates an audit record for the creation of an object handle.

Arguments:


Return Value:

    None.

--*/

{
    BOOLEAN AccessGranted = TRUE;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    HANDLE ProcessID;

    PAGED_CODE();

    if ( SepAdtAuditThisEvent( AuditCategoryObjectAccess, &AccessGranted )) {

        //
        // A completely zero'd entry will be interpreted
        // as a "null string" or not supplied parameter.
        //
        // Initializing the entire array up front will allow
        // us to avoid filling in each not supplied entry.
        //

        RtlZeroMemory (
           (PVOID) &AuditParameters,
           sizeof( AuditParameters )
           );

        ASSERT( SeAdtParmTypeNone == 0 );

        AuditParameters.CategoryId = SE_CATEGID_OBJECT_ACCESS;
        AuditParameters.AuditId = SE_AUDITID_CREATE_HANDLE;
        AuditParameters.ParameterCount = 0;
        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;


        //
        //  Parameter[0] - User Sid
        //

        SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );

        AuditParameters.ParameterCount++;


        //
        //  Parameter[1] - Subsystem name (if available)
        //

        if ( ARGUMENT_PRESENT( Source )) {

            SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, Source );
        }

        AuditParameters.ParameterCount++;

        //
        //    Parameter[2] - New handle ID
        //

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)Handle) );

        AuditParameters.ParameterCount++;

        //
        //    Parameters 3,4 - Operation ID
        //


        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, OperationId.HighPart );

        AuditParameters.ParameterCount++;

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, OperationId.LowPart );

        AuditParameters.ParameterCount++;


        //
        //    Parameter[5] - Subject's process id
        //

        ProcessID =  PsProcessAuditId( PsGetCurrentProcess() );

        SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)ProcessID) );

        AuditParameters.ParameterCount++;

        SepAdtLogAuditRecord( &AuditParameters );

    }
}




VOID
SepAdtObjectReferenceAuditAlarm(
    IN PLUID OperationId OPTIONAL,
    IN PVOID Object,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN BOOLEAN GenerateAudit,
    IN BOOLEAN GenerateAlarm
    )

/*++

Routine Description:

    description-of-function.

    This routine will create an SE_ADT_PARAMETERS array organized as follows:

    Parameter[0] - User Sid

    Parameter[1] - Subsystem name (if available)

    Parameter[2] - Object Type Name

    Parameter[3] - Object Name

    Parameter[4] - Subject's process id

    Parameter[5] - Subject's primary authentication ID

    Parameter[6] - Subject's client authentication ID

    Parameter[7] - DesiredAccess mask


Arguments:

    GenerateAudit - Indicates if we should generate an audit for this operation.

    GenerateAlarm - Indicates if we should generate an alarm for this operation.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    ULONG ObjectTypeIndex;
    POBJECT_NAME_INFORMATION ObjectNameInformation;
    PUNICODE_STRING ObjectTypeInformation;
    PSID UserSid;
    LUID PrimaryAuthenticationId;
    LUID ClientAuthenticationId;

    PTOKEN ClientToken = (PTOKEN)SubjectSecurityContext->ClientToken;
    PTOKEN PrimaryToken = (PTOKEN)SubjectSecurityContext->PrimaryToken;

    PAGED_CODE();


    if ( ARGUMENT_PRESENT( ClientToken )) {

        UserSid = SepTokenUserSid( ClientToken );
        ClientAuthenticationId =  SepTokenAuthenticationId( ClientToken );

    } else {

        UserSid = SepTokenUserSid( PrimaryToken );
    }

    PrimaryAuthenticationId = SepTokenAuthenticationId( PrimaryToken );

    //
    // A completely zero'd entry will be interpreted
    // as a "null string" or not supplied parameter.
    //
    // Initializing the entire array up front will allow
    // us to avoid filling in each not supplied entry.
    //

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_INDIRECT_REFERENCE;
    AuditParameters.ParameterCount = 8;

    if ( AccessGranted ) {

        AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    } else {

        AuditParameters.Type = EVENTLOG_AUDIT_FAILURE;
    }

    //
    // Obtain the object name and object type name from the
    // object.
    //

    ObjectNameInformation = SepQueryNameString( Object );


    ObjectTypeInformation = SepQueryTypeString( Object );




    //
    //  Parameter[0] - User Sid
    //

    SepSetParmTypeSid( AuditParameters, 0, UserSid );


    //
    //  Parameter[1] - Subsystem name (if available)
    //

    SepSetParmTypeString( AuditParameters, 1, &SeSubsystemName );


    //
    //  Parameter[2] - Object Type Name
    //

    if ( ObjectTypeInformation != NULL ) {

        SepSetParmTypeString( AuditParameters, 2, ObjectTypeInformation );
        ObjectTypeIndex = 2;
    }



    //
    //  Parameter[3] - Object Name
    //

    if ( ObjectNameInformation != NULL ) {

        SepSetParmTypeString( AuditParameters, 3, &ObjectNameInformation->Name );
    }




    //
    //  Parameter[4] - Subject's process id
    //

    //
    // BUGWARNING: The process Id is currently unavailable.
    //

    SepSetParmTypeUlong( AuditParameters, 4, (ULONG)((ULONG_PTR)(SubjectSecurityContext->ProcessAuditId)) );




    //
    //  Parameter[5] - Subject's primary authentication ID
    //


    SepSetParmTypeLogonId( AuditParameters, 5, PrimaryAuthenticationId );




    //
    //  Parameter[6] - Subject's client authentication ID
    //

    if ( ARGUMENT_PRESENT( ClientToken )) {

        SepSetParmTypeLogonId( AuditParameters, 6, ClientAuthenticationId );

    } else {

        SepSetParmTypeNoLogon( AuditParameters, 6 );

    }

    //
    //  Parameter[7] - DesiredAccess mask
    //


    SepSetParmTypeAccessMask( AuditParameters, 7, DesiredAccess, ObjectTypeIndex );


    SepAdtLogAuditRecord( &AuditParameters );

    if ( ObjectNameInformation != NULL ) {
        ExFreePool( ObjectNameInformation );
    }

    if ( ObjectTypeInformation != NULL ) {
        ExFreePool( ObjectTypeInformation );
    }

}


//VOID
//SepAdtCreateInstanceAuditAlarm(
//    IN PLUID OperationID,
//    IN PVOID Object,
//    IN PSID UserSid,
//    IN LUID AuthenticationId,
//    IN ACCESS_MASK DesiredAccess,
//    IN PPRIVILEGE_SET Privileges OPTIONAL,
//    IN BOOLEAN AccessGranted,
//    IN BOOLEAN GenerateAudit,
//    IN BOOLEAN GenerateAlarm
//    )
//
///*++
//
//Routine Description:
//
//    description-of-function.
//
//Arguments:
//
//    GenerateAudit - Indicates if we should generate an audit for this operation.
//
//    GenerateAlarm - Indicates if we should generate an alarm for this operation.
//
//
//Return Value:
//
//    return-value - Description of conditions needed to return value. - or -
//    None.
//
//--*/
//
//{
//    POLICY_AUDIT_CREATE_INSTANCE AuditCreateInstance;
//
//    UNREFERENCED_PARAMETER( GenerateAudit );
//    UNREFERENCED_PARAMETER( GenerateAlarm );
//
//    //
//    // BUGWARNING Must obtain the object type object from the passed
//    // object
//    //
//
//    AuditCreateInstance.AccessGranted = AccessGranted;
//    AuditCreateInstance.ObjectTypeName = NULL;
//    AuditCreateInstance.OperationId = *OperationID;
//    AuditCreateInstance.UserSid = UserSid;
//    AuditCreateInstance.AuthenticationId = AuthenticationId;
//
////    SepAdtLogAuditRecord( AuditEventCreateInstance, &AuditCreateInstance );
//
//    return;
//}



//
//VOID
//SeShutdownAuditAlarm(
//    VOID
//    )
//
///*++
//
//Routine Description:
//
//    This routine will can a shutdown audit record to be generated.
//
//    There must be a forced delay after this routine is called to ensure
//    that the generated audit record actually makes it to disk.
//
//Arguments:
//
//    None
//
//Return Value:
//
//    None.
//
//--*/
//
//{
//    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
//    SE_ADT_PARAMETER_ARRAY AuditParameters;
//    UNICODE_STRING SubsystemName;
//    PSID UserSid;
//
//    //
//    // Make sure we're auditing shutdown events.
//    //
//
//    if ( SepAdtAuditThisEvent( AuditEventShutdown, NULL )) {
//
//        SeCaptureSubjectContext( &SubjectSecurityContext );
//
//
//        //
//        // A completely zero'd entry will be interpreted
//        // as a "null string" or not supplied parameter.
//        //
//        // Initializing the entire array up front will allow
//        // us to avoid filling in each not supplied entry.
//        //
//
//        RtlZeroMemory (
//           (PVOID) &AuditParameters,
//           sizeof( AuditParameters )
//           );
//
//        ASSERT( SeAdtParmTypeNone == 0 );
//
//        AuditParameters.CategoryId = SE_CATEGID_SYSTEM;
//        AuditParameters.AuditId = SE_AUDITID_SYSTEM_SHUTDOWN;
//        AuditParameters.ParameterCount = 2;
//
//        UserSid = SepTokenUserSid(EffectiveToken( &SubjectSecurityContext ));
//
//
//        RtlInitUnicodeString( &SubsystemName, L"Security" );
//
//
//        //
//        //  Parameter[0] - User Sid
//        //
//
//        SepSetParmTypeSid( AuditParameters, 0, UserSid );
//
//
//        //
//        //  Parameter[1] - Subsystem name (if available)
//        //
//
//        SepSetParmTypeString( AuditParameters, 1, &SubsystemName );
//
//
//        SepAdtLogAuditRecord( &AuditParameters );
//
//        SeReleaseSubjectContext( &SubjectSecurityContext );
//
//    }
//
//}





// 
// BOOLEAN
// SepAdtAuditThisEvent(
//     IN POLICY_AUDIT_EVENT_TYPE AuditType,
//     IN PBOOLEAN AccessGranted OPTIONAL
//     )
//
// /*++
//
// Routine Description:
//
//     This routine will return whether or not to generate an audit log
//     record for the passed event type.
//
// Arguments:
//
//     AuditType - The type of event to be audited.
//
//     AccessGranted - An optional flag indicating whether or not
//         the operation was successful.  This does not all apply to all
//         types of audit events.
//
//         Note that a pointer to the flag is passed rather than the
//         flag itself, so that we may tell whether or not the argument
//         is present.
//
// Return Value:
//
//     Flag indicating whether or not to proceed with the audit.
//
// --*/
//
// {
//     PAGED_CODE();
//
//     if (SepAdtAuditingEnabled) {
//
//         if ( ARGUMENT_PRESENT( AccessGranted )) {
//
//             if ((SeAuditingState[AuditType].AuditOnSuccess && *AccessGranted) ||
//                  SeAuditingState[AuditType].AuditOnFailure && !(*AccessGranted)) {
//
//                 return( TRUE );
//
//             }
//         }
//     }
//
//     return( FALSE );
// }





POBJECT_NAME_INFORMATION
SepQueryNameString(
    IN PVOID Object
    )

/*++

Routine Description:

    Takes a pointer to an object and returns the name of the object.

Arguments:

    Object - a pointer to an object.


Return Value:

    A pointer to a buffer containing a POBJECT_NAME_INFORMATION
    structure containing the name of the object.  The string is
    allocated out of paged pool and should be freed by the caller.

    NULL may also be returned.


--*/

{
    NTSTATUS Status;
    ULONG ReturnLength = 0;
    POBJECT_NAME_INFORMATION ObjectNameInfo = NULL;
    PUNICODE_STRING ObjectName = NULL;

    PAGED_CODE();

    Status = ObQueryNameString(
                 Object,
                 ObjectNameInfo,
                 0,
                 &ReturnLength
                 );

    if ( Status == STATUS_INFO_LENGTH_MISMATCH ) {

        ObjectNameInfo = ExAllocatePoolWithTag( PagedPool, ReturnLength, 'nOeS' );

        if ( ObjectNameInfo != NULL ) {

            Status = ObQueryNameString(
                        Object,
                        ObjectNameInfo,
                        ReturnLength,
                        &ReturnLength
                        );

            if ( NT_SUCCESS( Status )) {

                if (ObjectNameInfo->Name.Length != 0) {

                    return( ObjectNameInfo );

                } else {

                    ExFreePool( ObjectNameInfo );
                    return( NULL );
                }
            }
        }
    }

    return( NULL );
}




PUNICODE_STRING
SepQueryTypeString(
    IN PVOID Object
    )
/*++

Routine Description:

    Takes a pointer to an object and returns the type of the object.

Arguments:

    Object - a pointer to an object.


Return Value:

    A pointer to a UNICODE_STRING that contains the name of the object
    type.  The string is allocated out of paged pool and should be freed
    by the caller.

    NULL may also be returned.


--*/

{

    NTSTATUS Status;
    PUNICODE_STRING TypeName = NULL;
    ULONG ReturnLength;

    PAGED_CODE();

    Status = ObQueryTypeName(
                 Object,
                 TypeName,
                 0,
                 &ReturnLength
                 );

    if ( Status == STATUS_INFO_LENGTH_MISMATCH ) {

        TypeName = ExAllocatePoolWithTag( PagedPool, ReturnLength, 'nTeS' );

        if ( TypeName != NULL ) {

            Status = ObQueryTypeName(
                        Object,
                        TypeName,
                        ReturnLength,
                        &ReturnLength
                        );

            if ( NT_SUCCESS( Status )) {

                return( TypeName );
            }
        }
    }

    return( NULL );
}



VOID
SeAuditProcessCreation(
    PEPROCESS Process,
    PEPROCESS Parent,
    PUNICODE_STRING ImageFileName
    )
/*++

Routine Description:

    Audits the creation of a process.  It is the caller's responsibility
    to determine if process auditing is in progress.


Arguments:

    Process - Points to the new process object.

    Parent - Points to the creator (parent) process object.
    
    ImageFileName - the name of the image

Return Value:

    None.

--*/

{
    ANSI_STRING Ansi;
    LUID UserAuthenticationId;
    NTSTATUS Status;
    PSID UserSid;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    
    PAGED_CODE();

    if ( ImageFileName == NULL )
    {
        return ;
    }

    //
    // NtCreateProcess with no section will cause this to be NULL
    // fork() for posix will do this, or someone calling NtCreateProcess
    // directly.
    //

    if ( ImageFileName->Buffer == NULL )
    {
        return;
    }

    SeCaptureSubjectContext( &SubjectSecurityContext );

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );

    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_PROCESS_CREATED;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    //
    // Use the primary token here, because that's what's going to show up
    // when the created process exits.
    //

    UserSid = SepTokenUserSid( SubjectSecurityContext.PrimaryToken );

    UserAuthenticationId = SepTokenAuthenticationId( SubjectSecurityContext.PrimaryToken );

    //
    // Fill in the AuditParameters structure.
    //

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)Process) );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, ImageFileName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)Parent) );
    AuditParameters.ParameterCount++;

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, UserAuthenticationId );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );

    SeReleaseSubjectContext( &SubjectSecurityContext );

    return;
}


VOID
SeAuditHandleDuplication(
    PVOID SourceHandle,
    PVOID NewHandle,
    PEPROCESS SourceProcess,
    PEPROCESS TargetProcess
    )

/*++

Routine Description:

    This routine generates a handle duplication audit.  It is up to the caller
    to determine if this routine should be called or not.

Arguments:

    SourceHandle -  Original handle

    NewHandle - New handle

    SourceProcess - Process containing SourceHandle

    TargetProcess - Process containing NewHandle

Return Value:

    None.

--*/

{
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    PSID UserSid;

    PAGED_CODE();

    SeCaptureSubjectContext( &SubjectSecurityContext );

    UserSid = SepTokenUserSid( EffectiveToken( &SubjectSecurityContext ));

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );


    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_DUPLICATE_HANDLE;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)SourceHandle) );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)PsProcessAuditId( SourceProcess )));
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)NewHandle) );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)PsProcessAuditId( TargetProcess )));
    AuditParameters.ParameterCount++;


    SepAdtLogAuditRecord( &AuditParameters );

    SeReleaseSubjectContext( &SubjectSecurityContext );
}


VOID
SeAuditProcessExit(
    PEPROCESS Process
    )
/*++

Routine Description:

    Audits the exit of a process.  The caller is responsible for
    determining if this should be called.

Arguments:

    Process - Pointer to the process object that is exiting.

Return Value:

    None.

--*/

{
    PTOKEN Token;
    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID UserSid;
    LUID LogonId;

    PAGED_CODE();

    Token = (PTOKEN)Process->Token;

    UserSid = SepTokenUserSid( Token );
    LogonId = SepTokenAuthenticationId( Token );

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );


    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_DETAILED_TRACKING;
    AuditParameters.AuditId = SE_AUDITID_PROCESS_EXIT;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, (ULONG)((ULONG_PTR)PsProcessAuditId( Process )));
    AuditParameters.ParameterCount++;

    SepSetParmTypeLogonId( AuditParameters, AuditParameters.ParameterCount, LogonId );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );
}



VOID
SepAdtGenerateDiscardAudit(
    VOID
    )

/*++

Routine Description:

    Generates an 'audits discarded' audit.

Arguments:

    none

Return Value:

    None.

--*/

{

    SE_ADT_PARAMETER_ARRAY AuditParameters;
    PSID UserSid;

    PAGED_CODE();

    UserSid = SeLocalSystemSid;

    RtlZeroMemory (
       (PVOID) &AuditParameters,
       sizeof( AuditParameters )
       );


    ASSERT( SeAdtParmTypeNone == 0 );

    AuditParameters.CategoryId = SE_CATEGID_SYSTEM;
    AuditParameters.AuditId = SE_AUDITID_AUDITS_DISCARDED;
    AuditParameters.ParameterCount = 0;
    AuditParameters.Type = EVENTLOG_AUDIT_SUCCESS;

    SepSetParmTypeSid( AuditParameters, AuditParameters.ParameterCount, UserSid );
    AuditParameters.ParameterCount++;

    SepSetParmTypeString( AuditParameters, AuditParameters.ParameterCount, &SeSubsystemName );
    AuditParameters.ParameterCount++;

    SepSetParmTypeUlong( AuditParameters, AuditParameters.ParameterCount, SepAdtCountEventsDiscarded );
    AuditParameters.ParameterCount++;

    SepAdtLogAuditRecord( &AuditParameters );
}
