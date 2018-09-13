/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ctseacc.c

Abstract:

    Common security accessibility test routines.

    These routines are used in both the kernel and user mode RTL tests.

    This test assumes the security runtime library routines are
    functioning correctly.



Author:

    Jim Kelly       (JimK)     23-Mar-1990

Environment:

    Test of security.

Revision History:

    v5: robertre
        Updated ACL_REVISION

--*/

#include "tsecomm.c"    // Mode dependent macros and routines.



////////////////////////////////////////////////////////////////
//                                                            //
// Module wide variables                                      //
//                                                            //
////////////////////////////////////////////////////////////////

    NTSTATUS Status;
    STRING  Event1Name, Process1Name;
    UNICODE_STRING UnicodeEvent1Name, UnicodeProcess1Name;

    OBJECT_ATTRIBUTES NullObjectAttributes;

    HANDLE Event1;
    OBJECT_ATTRIBUTES Event1ObjectAttributes;
    PSECURITY_DESCRIPTOR Event1SecurityDescriptor;
    PSID Event1Owner;
    PSID Event1Group;
    PACL Event1Dacl;
    PACL Event1Sacl;

    PACL TDacl;
    BOOLEAN TDaclPresent;
    BOOLEAN TDaclDefaulted;

    PACL TSacl;
    BOOLEAN TSaclPresent;
    BOOLEAN TSaclDefaulted;

    PSID TOwner;
    BOOLEAN TOwnerDefaulted;
    PSID TGroup;
    BOOLEAN TGroupDefaulted;


HANDLE Process1;
OBJECT_ATTRIBUTES Process1ObjectAttributes;




////////////////////////////////////////////////////////////////
//                                                            //
// Initialization Routine                                     //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestSeInitialize()
{

    Event1SecurityDescriptor = (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );

    RtlInitString(&Event1Name, "\\SecurityTestEvent1");
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeEvent1Name,
                 &Event1Name,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );
    RtlInitString(&Process1Name, "\\SecurityTestProcess1");
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeProcess1Name,
                 &Process1Name,
                 TRUE );  SEASSERT_SUCCESS( NT_SUCCESS(Status) );

    InitializeObjectAttributes(&NullObjectAttributes, NULL, 0, NULL, NULL);

    //
    // Build an ACL or two for use.

    TDacl        = (PACL)TstAllocatePool( PagedPool, 256 );
    TSacl        = (PACL)TstAllocatePool( PagedPool, 256 );

    TDacl->AclRevision=TSacl->AclRevision=ACL_REVISION;
    TDacl->Sbz1=TSacl->Sbz1=0;
    TDacl->Sbz2=TSacl->Sbz2=0;
    TDacl->AclSize=256;
    TSacl->AclSize=8;
    TDacl->AceCount=TSacl->AceCount=0;

    return TRUE;
}



////////////////////////////////////////////////////////////////
//                                                            //
// Test routines                                              //
//                                                            //
////////////////////////////////////////////////////////////////

BOOLEAN
TestSeUnnamedCreate()
//
//  Test:
//      No Security Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl Inheritence With Creator ID
//          Dacl & Sacl Inheritence
//
//      Empty Security Descriptor Explicitly Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl & Sacl Inheritence
//
//      Explicit Dacl Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl & Sacl Inheritence
//
//      Explicit Sacl Specified (W/Privilege)
//          No Inheritence
//          Dacl & Sacl Inheritence
//
//      Default Dacl Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl & Sacl Inheritence
//
//      Default Sacl Specified (W/Privilege)
//          No Inheritence
//          Dacl & Sacl Inheritence
//
//      Explicit Sacl Specified (W/O Privilege - should be rejected)
//      Default Sacl Specified (W/O Privilege - should be rejected)
//
//      Valid Owner Explicitly Specified
//      Invalid Owner Explicitly Specified
//
//      Explicit Group Specified
//
{


    BOOLEAN CompletionStatus = TRUE;

    InitializeObjectAttributes(&Event1ObjectAttributes, NULL, 0, NULL, NULL);
    DbgPrint("Se:     No Security Descriptor...                            Test\n");
    DbgPrint("Se:         No Inheritence...                                  ");

    Status = NtCreateEvent(
                 &Event1,
                 DELETE,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint(" **** Failed ****\n");
        CompletionStatus = FALSE;
    }
    ASSERT(NT_SUCCESS(Status));
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:         Dacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Sacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl Inheritence W/ Creator ID...                  ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl And Sacl Inheritence...                       ");
    DbgPrint("  Not Implemented.\n");

    return CompletionStatus;

}

BOOLEAN
TestSeNamedCreate()
//
//  Test:
//      No Security Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl Inheritence With Creator ID
//          Dacl & Sacl Inheritence
//
//      Empty Security Descriptor Explicitly Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl & Sacl Inheritence
//
//      Explicit Dacl Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl & Sacl Inheritence
//
//      Explicit Sacl Specified (W/Privilege)
//          No Inheritence
//          Dacl & Sacl Inheritence
//
//      Default Dacl Specified
//          No Inheritence
//          Dacl Inheritence
//          Sacl Inheritence
//          Dacl & Sacl Inheritence
//
//      Default Sacl Specified (W/Privilege)
//          No Inheritence
//          Dacl & Sacl Inheritence
//
//      Explicit Sacl Specified (W/O Privilege - should be rejected)
//      Default Sacl Specified (W/O Privilege - should be rejected)
//
//      Valid Owner Explicitly Specified
//      Invalid Owner Explicitly Specified
//
//      Explicit Group Specified
//
{

    BOOLEAN CompletionStatus = TRUE;


    InitializeObjectAttributes(
        &Event1ObjectAttributes,
        &UnicodeEvent1Name,
        0,
        NULL,
        NULL);

    DbgPrint("Se:     No Security Specified...                             Test\n");
    DbgPrint("Se:         No Inheritence...                                  ");
    Status = NtCreateEvent(
                 &Event1,
                 DELETE,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint(" **** Failed ****\n");
        CompletionStatus = FALSE;
    }
    ASSERT(NT_SUCCESS(Status));
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:         Dacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Sacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl Inheritence With Creator ID...                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl & Sacl Inheritence...                         ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Empty Security Descriptor Explicitly Specified...    Test\n");
    DbgPrint("Se:         No Inheritence...                                  ");

    RtlCreateSecurityDescriptor( Event1SecurityDescriptor, 1 );
    InitializeObjectAttributes(&Event1ObjectAttributes,
                               &UnicodeEvent1Name,
                               0,
                               NULL,
                               Event1SecurityDescriptor);
    Status = NtCreateEvent(
                 &Event1,
                 DELETE,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint(" **** Failed ****\n");
        CompletionStatus = FALSE;
    }
    ASSERT(NT_SUCCESS(Status));
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));




    DbgPrint("Se:         Dacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Sacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl & Sacl Inheritence...                         ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Explicit Dacl Specified...                           Test\n");
    DbgPrint("Se:         No Inheritence...                                  ");

    RtlCreateSecurityDescriptor( Event1SecurityDescriptor, 1 );
    RtlSetDaclSecurityDescriptor( Event1SecurityDescriptor, TRUE, TDacl, FALSE );

    InitializeObjectAttributes(&Event1ObjectAttributes,
                               &UnicodeEvent1Name,
                               0,
                               NULL,
                               Event1SecurityDescriptor);
    Status = NtCreateEvent(
                 &Event1,
                 DELETE,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    if (NT_SUCCESS(Status)) {
        DbgPrint("Succeeded.\n");
    } else {
        DbgPrint(" **** Failed ****\n");
        CompletionStatus = FALSE;
    }
    ASSERT(NT_SUCCESS(Status));
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:         Dacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Sacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl & Sacl Inheritence...                         ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Explicit Sacl Specified (W/Privilege)...             Test\n");
    DbgPrint("Se:         No Inheritence...                                  ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl & Sacl Inheritence...                         ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Default Dacl Specified...                            Test\n");
    DbgPrint("Se:         No Inheritence...                                  ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Sacl Inheritence...                                ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl & Sacl Inheritence...                         ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Default Sacl  (W/Privilege)...                       Test\n");
    DbgPrint("Se:         No Inheritence...                                  ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Dacl & Sacl Inheritence...                         ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Explicit Sacl (W/O Privilege)...                     Test\n");
    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:     Default Sacl (W/O Privilege)...                      Test\n");
    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Valid Owner Explicitly Specified...                  Test\n");
    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:     Invalid Owner Explicitly Specified...                Test\n");
    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Explicit Group Specified...                          Test\n");
    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");



    return CompletionStatus;

}

BOOLEAN
TestSeQuerySecurity()
//
//  Test:
//      No Security Descriptor
//          Query Owner
//          Query Group
//          Query  Dacl
//          Query Sacl (Privileged)
//          Query Sacl (Unprivileged - should be rejected)
//
//      Empty Security Descriptor
//          Query Owner
//          Query Group
//          Query  Dacl
//          Query Sacl (Privileged)
//          Query Sacl (Unprivileged - should be rejected)
//
//      Security Descriptor W/ Owner & Group
//          Query Owner
//          Query Group
//          Query  Dacl
//          Query Sacl (Privileged)
//          Query Sacl (Unprivileged - should be rejected)
//
//      Full Security Descriptor
//          Query Owner
//          Query Group
//          Query  Dacl
//          Query Sacl (Privileged)
//          Query Sacl (Unprivileged - should be rejected)
//
{

    BOOLEAN CompletionStatus = TRUE;

    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");

#if 0
    DbgPrint("Se:     No Security Descriptor... \n");
    DbgPrint("Se:         Query Owner... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query  Dacl... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Unprivileged)... ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Empty Security Descriptor... \n");
    DbgPrint("Se:         Query Owner... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query  Dacl... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Unprivileged)... ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Security Descriptor W/ Owner & Group... \n");
    DbgPrint("Se:         Query Owner... ");
    DbgPrint("  Not Implemented. \n");
    DbgPrint("Se:         Query Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query  Dacl... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Unprivileged)... ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Full Security Descriptor...\n");
    DbgPrint("Se:         Query Owner... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query  Dacl... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Query Sacl (Unprivileged)... ");
    DbgPrint("  Not Implemented.\n");
#endif //0

    return CompletionStatus;
}

BOOLEAN
TestSeSetSecurity()
//
//  Test:
//      No Security Descriptor
//          Set Valid Owner SID
//          Set Invalid Owner SID
//          Set Group
//          Set Dacl (explicitly granted by dacl)
//          Set Dacl (by virtue of ownership)
//          Set Dacl (invalid attempt)
//          Set Sacl (privileged)
//          Set Sacl (unprivileged - should be rejected)
//
//      Empty Security Descriptor
//          Set Valid Owner SID
//          Set Invalid Owner SID
//          Set Group
//          Set Dacl (explicitly granted by dacl)
//          Set Dacl (by virtue of ownership)
//          Set Dacl (invalid attempt)
//          Set Sacl (privileged)
//          Set Sacl (unprivileged - should be rejected)
//
//      Security Descriptor W/ Owner & Group Only
//          Set Valid Owner SID
//          Set Invalid Owner SID
//          Set Group
//          Set Dacl (explicitly granted by dacl)
//          Set Dacl (by virtue of ownership)
//          Set Dacl (invalid attempt)
//          Set Sacl (privileged)
//          Set Sacl (unprivileged - should be rejected)
//
//      Full Security Descriptor
//          Set Valid Owner SID
//          Set Invalid Owner SID
//          Set Group
//          Set Dacl (explicitly granted by dacl)
//          Set Dacl (by virtue of ownership)
//          Set Dacl (invalid attempt)
//          Set Sacl (privileged)
//          Set Sacl (unprivileged - should be rejected)
//
{

    BOOLEAN CompletionStatus = TRUE;

    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");
#if 0
    DbgPrint("Se:     No Security Descriptor...\n");
    DbgPrint("Se:         Set Valid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Invalid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (explicitly granted by dacl)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (by virtue of ownership)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (invalid attempt)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (unprivileged - should be rejected)... ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Empty Security Descriptor...\n");
    DbgPrint("Se:         Set Valid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Invalid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (explicitly granted by dacl)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (by virtue of ownership)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (invalid attempt)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (unprivileged - should be rejected)... ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Security Descriptor W/ Owner & Group Only...\n");
    DbgPrint("Se:         Set Valid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Invalid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (explicitly granted by dacl)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (by virtue of ownership)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (invalid attempt)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (unprivileged - should be rejected)... ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:     Full Security Descriptor...\n");
    DbgPrint("Se:         Set Valid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Invalid Owner SID... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Group... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (explicitly granted by dacl)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (by virtue of ownership)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Dacl (invalid attempt)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (privileged)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Set Sacl (unprivileged - should be rejected)... ");
    DbgPrint("  Not Implemented.\n");

#endif //0

    return CompletionStatus;

}

BOOLEAN
TestSeAccess()
//
//  Test:
//
//      Creation
//          No Access Requested (should be rejected)
//          Specific Access Requested
//              - Attempted Granted
//              - Attempt Ungranted
//          Access System Security
//
//       Open Existing
//          No Access Requested (should be rejected)
//          Specific Access Requested
//              - Attempted Granted
//              - Attempt Ungranted
//          Access System Security
//

{
    BOOLEAN CompletionStatus = TRUE;

    DbgPrint("                                                               ");
    DbgPrint("  Not Implemented.\n");
#if 0

    DbgPrint("Se:     Creation...\n");
    DbgPrint("Se:         No Access Requested (should be rejected)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Specific Access Requested... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:             - Attempted Granted... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:             - Attempt Ungranted... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Access System Security... ");
    DbgPrint("  Not Implemented.\n");

    DbgPrint("Se:      Open Existing...\n");
    DbgPrint("Se:         No Access Requested (should be rejected)... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Specific Access Requested... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:             - Attempted Granted... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:             - Attempt Ungranted... ");
    DbgPrint("  Not Implemented.\n");
    DbgPrint("Se:         Access System Security... ");
    DbgPrint("  Not Implemented.\n");
#endif //0

#if 0  //old code
// Without security descriptor
// Simple desired access mask...
//

    DbgPrint("Se:     Test1b... \n");         // Attempt ungranted access
    Status = NtSetEvent(
                 Event1,
                 NULL
                 );
    ASSERT(!NT_SUCCESS(Status));

    DbgPrint("Se:     Test1c... \n");         // Delete object
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));


    //
    // Without security descriptor
    // Simple desired access mask...
    //

    DbgPrint("Se:     Test2a... \n");         // unnamed object, specific access
    Status = NtCreateEvent(
                 &Event1,
                 (EVENT_MODIFY_STATE | STANDARD_DELETE),
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test2b... \n");         // Attempt granted specific access
    Status = NtSetEvent(
                 Event1,
                 NULL
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test2c... \n");         // Delete object


    //
    // Without security descriptor
    // Generic desired access mask...
    //

    DbgPrint("Se:     Test3a... \n");         // Unnamed object, generic mask
    Status = NtCreateEvent(
                 &Event1,
                 GENERIC_EXECUTE,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test3b... \n");         // Attempt implied granted access
    Status = NtSetEvent(
                 Event1,
                 NULL
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test3c... \n");         // Delete object
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));


    //
    // Without security descriptor
    // Empty desired access mask...
    //

    DbgPrint("Se:     Test4a... \n");         // Empty desired access
    Status = NtCreateEvent(
                 &Event1,
                 0,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    ASSERT(!NT_SUCCESS(Status));


    RtlCreateSecurityDescriptor( Event1SecurityDescriptor,
                                 SECURITY_DESCRIPTOR_REVISION);
    InitializeObjectAttributes(&Event1ObjectAttributes,
                               NULL, 0, NULL,
                               Event1SecurityDescriptor);
    DbgPrint("Se:     Empty Security Descriptor... \n");

    //
    // Without security descriptor
    // Simple desired access mask...
    //

    DbgPrint("Se:     Test1a... \n");         // Create unnamed object
    Status = NtCreateEvent(
                 &Event1,
                 STANDARD_DELETE,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test1b... \n");         // Attempt ungranted access
    Status = NtSetEvent(
                 Event1,
                 NULL
                 );
    ASSERT(!NT_SUCCESS(Status));

    DbgPrint("Se:     Test1c... \n");         // Delete object
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));


    //
    // Without security descriptor
    // Simple desired access mask...
    //

    DbgPrint("Se:     Test2a... \n");         // unnamed object, specific access
    Status = NtCreateEvent(
                 &Event1,
                 (EVENT_MODIFY_STATE | STANDARD_DELETE),
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test2b... \n");         // Attempt granted specific access
    Status = NtSetEvent(
                 Event1,
                 NULL
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test2c... \n");         // Delete object
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));


    //
    // Without security descriptor
    // Generic desired access mask...
    //

    DbgPrint("Se:     Test3a... \n");         // Unnamed object, generic mask
    Status = NtCreateEvent(
                 &Event1,
                 GENERIC_EXECUTE,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test3b... \n");         // Attempt implied granted access
    Status = NtSetEvent(
                 Event1,
                 NULL
                 );
    ASSERT(NT_SUCCESS(Status));

    DbgPrint("Se:     Test3c... \n");         // Delete object
    Status = NtClose(Event1);
    ASSERT(NT_SUCCESS(Status));


    //
    // Without security descriptor
    // Empty desired access mask...
    //

    DbgPrint("Se:     Test4a... \n");         // Empty desired access
    Status = NtCreateEvent(
                 &Event1,
                 0,
                 &Event1ObjectAttributes,
                 NotificationEvent,
                 FALSE
                 );
    ASSERT(!NT_SUCCESS(Status));
#endif // old code

    return CompletionStatus;
}

BOOLEAN
TSeAcc()
{
    BOOLEAN Result = TRUE;

    DbgPrint("Se:   Initialization...                                        ");
    TestSeInitialize();
    DbgPrint("Succeeded.\n");

    DbgPrint("Se:   Unnamed Object Creation Test...                      Suite\n");
    if (!TestSeUnnamedCreate()) {
        Result = FALSE;
    }
    DbgPrint("Se:   Named Object Creation Test...                        Suite\n");
    if (!TestSeNamedCreate()) {
        Result = FALSE;
    }
    DbgPrint("Se:   Query Object Security Descriptor Test...             Suite\n");
    if (!TestSeQuerySecurity()) {
        Result = FALSE;
    }
    DbgPrint("Se:   Set Object Security Descriptor Test...               Suite\n");
    if (!TestSeSetSecurity()) {
        Result = FALSE;
    }
    DbgPrint("Se:   Access Test...                                       Suite\n");
    if (!TestSeAccess()) {
        Result = FALSE;
    }

    DbgPrint("\n");
    DbgPrint("\n");
    DbgPrint("    ********************\n");
    DbgPrint("    **                **\n");

    if (Result = TRUE) {
        DbgPrint("    ** Test Succeeded **\n");
    } else {
        DbgPrint("    **  Test Failed   **\n");
    }

    DbgPrint("    **                **\n");
    DbgPrint("    ********************\n");
    DbgPrint("\n");
    DbgPrint("\n");

    return Result;
}

