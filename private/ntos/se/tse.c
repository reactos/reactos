//////////////////////////////////////////////////////////////////////////
// WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING  //
//                                                                      //
// This test file is not current with the security implementation.      //
// This file contains references to data types and APIs that do not     //
// exist.                                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

/*++
Copyright (c) 1989  Microsoft Corporation

Module Name:

    tse.c

Abstract:

    Test program for the SE subcomponent of the NTOS project

Author:

    Gary Kimura     (garyki)    20-Nov-1989

Revision History:

--*/

#include <stdio.h>

#include "sep.h"
#include <zwapi.h>

BOOLEAN SeTest();

#include "ttoken.c"

int
main(
    int argc,
    char *argv[]
    )
{
    VOID KiSystemStartup();

    TestFunction = SeTest;
    KiSystemStartup();
    return( 0 );
}


BOOLEAN
SeTest()
{
    BOOLEAN TestMakeSystemToken();
    BOOLEAN TestTokenCopy();
    BOOLEAN TestTokenSize();
    BOOLEAN TestDefaultObjectMethod();
    BOOLEAN TestCaptureSecurityDescriptor();
    BOOLEAN TestAssignSecurity();
    BOOLEAN TestAccessCheck();
    BOOLEAN TestGenerateMessage();

    DbgPrint("Start SeTest()\n");

    if (!TestMakeSystemToken()) {
        DbgPrint("TestMakeSystemToken() Error\n");
        return FALSE;
    }
    if (!TestTokenCopy()) {
        DbgPrint("TestTokenCopy() Error\n");
        return FALSE;
    }
    if (!TestTokenSize()) {
        DbgPrint("TestTokenSize() Error\n");
        return FALSE;
    }
    if (!TestDefaultObjectMethod()) {
        DbgPrint("TestDefaultObjectMethod() Error\n");
        return FALSE;
    }
    if (!TestCaptureSecurityDescriptor()) {
        DbgPrint("TestCaptureSecurityDescriptor() Error\n");
        return FALSE;
    }
    if (!TestAssignSecurity()) {
        DbgPrint("TestAssignSecurity() Error\n");
        return FALSE;
    }
    if (!TestAccessCheck()) {
        DbgPrint("TestAccessCheck() Error\n");
        return FALSE;
    }
    if (!TestGenerateMessage()) {
        DbgPrint("TestGenerateMessage() Error\n");
        return FALSE;
    }

    DbgPrint("End SeTest()\n");
    return TRUE;
}


BOOLEAN
TestMakeSystemToken()
{
    PACCESS_TOKEN Token;

    //
    //  Make the system token
    //

    Token = (PACCESS_TOKEN)SeMakeSystemToken( PagedPool );

    //
    //  and check its contents
    //

    if (!SepIsSystemToken( Token, ((ACCESS_TOKEN *)Token)->Size )) {
        DbgPrint("SepIsSystemToken Error\n");
        return FALSE;
    }

    ExFreePool( Token );

    return TRUE;
}


BOOLEAN
TestTokenCopy()
{
    PACCESS_TOKEN InToken;
    PACCESS_TOKEN OutToken;

    NTSTATUS Status;

    ULONG i;

    //
    //  Allocate Buffers, and build a token
    //

    InToken = (PACCESS_TOKEN)ExAllocatePool(PagedPool, 512);
    OutToken = (PACCESS_TOKEN)ExAllocatePool(PagedPool, 512);

    BuildToken( Fred, InToken );

    //
    //  Make a copy of the token
    //

    if (!NT_SUCCESS(Status = SeTokenCopy( InToken, OutToken, 512))) {
        DbgPrint("SeTokenCopy Error: %8lx\n", Status);
        return FALSE;
    }

    //
    //  check both tokens for equality
    //

    for (i = 0; i < ((ACCESS_TOKEN *)InToken)->Size; i += 1) {
        if (*((PUCHAR)InToken + 1) != *((PUCHAR)OutToken + 1)) {
            DbgPrint("Token copy error\n");
            return FALSE;
        }
    }

    ExFreePool( InToken );
    ExFreePool( OutToken );

    return TRUE;
}


BOOLEAN
TestTokenSize()
{
    PACCESS_TOKEN Token;

    //
    //  Allocate and build a token
    //

    Token = (PACCESS_TOKEN)ExAllocatePool(PagedPool, 512);

    BuildToken( Wilma, Token );

    //
    //  Get the token size
    //

    if (SeTokenSize(Token) != ((ACCESS_TOKEN *)Token)->Size) {
        DbgPrint("SeTokenSize error\n");
        return FALSE;
    }

    ExFreePool( Token );

    return TRUE;
}


BOOLEAN
TestDefaultObjectMethod()
{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_DESCRIPTOR Buffer;
    PACL Acl;
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR ObjectsSecurityDescriptor;
    ULONG Length;

    Acl = (PACL)ExAllocatePool( PagedPool, 1024 );
    Buffer = (PSECURITY_DESCRIPTOR)ExAllocatePool( PagedPool, 1024 );

    BuildAcl( Fred, Acl, 1024 );
    DiscretionarySecurityDescriptor( &SecurityDescriptor, Acl );

    ObjectsSecurityDescriptor = NULL;

    //
    //  Set the descriptor
    //

    if (!NT_SUCCESS(Status = SeDefaultObjectMethod( NULL,
                                                 SetSecurityDescriptor,
                                                 &SecurityDescriptor,
                                                 0,
                                                 NULL,
                                                 &ObjectsSecurityDescriptor,
                                                 PagedPool ))) {

        DbgPrint("SeDefaultObjectMethod setting error: %8lx\n", Status );
        return FALSE;
    }

    //
    //  Query the descriptor
    //

    if (!NT_SUCCESS(Status = SeDefaultObjectMethod( NULL,
                                                 QuerySecurityDescriptor,
                                                 Buffer,
                                                 AllAclInformation,
                                                 &Length,
                                                 &ObjectsSecurityDescriptor,
                                                 PagedPool ))) {

        DbgPrint("SeDefaultObjectMethod reading error: %8lx\n", Status );
        return FALSE;
    }

    //
    //  Replace the descriptor
    //

    BuildAcl( Wilma, Acl, 1024 );

    if (!NT_SUCCESS(Status = SeDefaultObjectMethod( NULL,
                                                 SetSecurityDescriptor,
                                                 &SecurityDescriptor,
                                                 AllAclInformation,
                                                 &Length,
                                                 &ObjectsSecurityDescriptor,
                                                 PagedPool ))) {

        DbgPrint("SeDefaultObjectMethod reading error: %8lx\n", Status );
        return FALSE;
    }

    //
    //  Delete the descriptor
    //

    if (!NT_SUCCESS(Status = SeDefaultObjectMethod( NULL,
                                                 DeleteSecurityDescriptor,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 &ObjectsSecurityDescriptor,
                                                 PagedPool ))) {

        DbgPrint("SeDefaultObjectMethod deleting error: %8lx\n", Status );
        return FALSE;
    }

    ExFreePool(Acl);
    ExFreePool(Buffer);

    return TRUE;
}


BOOLEAN
TestCaptureSecurityDescriptor()
{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Sacl;
    PACL Dacl;
    PSECURITY_DESCRIPTOR NewDescriptor;
    NTSTATUS Status;

    Sacl = (PACL)ExAllocatePool( PagedPool, 1024 );
    Dacl = (PACL)ExAllocatePool( PagedPool, 1024 );
    BuildAcl( Pebbles, Sacl, 1024 );
    BuildAcl( Barney, Dacl, 1024 );

    DiscretionarySecurityDescriptor( &SecurityDescriptor, Dacl );
    SecurityDescriptor.SecurityInformationClass = AllAclInformation;
    SecurityDescriptor.SystemAcl = Sacl;

    //
    //  Capture kernel mode and don't force
    //

    if (!NT_SUCCESS(Status = SeCaptureSecurityDescriptor( &SecurityDescriptor,
                                                       KernelMode,
                                                       PagedPool,
                                                       FALSE,
                                                       &NewDescriptor ))) {
        DbgPrint("SeCapture Error, KernelMode, FALSE : %8lx\n", Status );
        return FALSE;
    }

    //
    //  Capture kernel mode and force
    //

    if (!NT_SUCCESS(Status = SeCaptureSecurityDescriptor( &SecurityDescriptor,
                                                       KernelMode,
                                                       PagedPool,
                                                       TRUE,
                                                       &NewDescriptor ))) {
        DbgPrint("SeCapture Error, KernelMode, TRUE : %8lx\n", Status );
        return FALSE;
    } else {
        ExFreePool( NewDescriptor );
    }

    //
    //  Capture user mode
    //

    if (!NT_SUCCESS(Status = SeCaptureSecurityDescriptor( &SecurityDescriptor,
                                                       UserMode,
                                                       PagedPool,
                                                       TRUE,
                                                       &NewDescriptor ))) {
        DbgPrint("SeCapture Error, UserMode, FALSE : %8lx\n", Status );
        return FALSE;
    } else {
        ExFreePool( NewDescriptor );
    }

    return TRUE;
}

BOOLEAN
TestAssignSecurity()
{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Acl;

    PACCESS_TOKEN Token;

    GENERIC_MAPPING GenericMapping;

    PSECURITY_DESCRIPTOR NewDescriptor;

    NTSTATUS Status;

    Acl = (PACL)ExAllocatePool( PagedPool, 1024 );
    BuildAcl( Fred, Acl, 1024 );

    Token = (PACCESS_TOKEN)ExAllocatePool( PagedPool, 512 );
    BuildToken( Fred, Token );

    DiscretionarySecurityDescriptor( &SecurityDescriptor, Acl );

    //
    //  Kernel mode, non dir, and no new
    //

    NewDescriptor = NULL;
    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            FALSE,
                                            Token,
                                            &GenericMapping,
                                            KernelMode ))) {
        DbgPrint("SeAssign Error NoNew, NoDir, Kernel : %8lx\n", Status );
        return FALSE;
    }

    //
    //  Kernel mode, non dir, and new
    //

    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            FALSE,
                                            Token,
                                            &GenericMapping,
                                            KernelMode ))) {
        DbgPrint("SeAssign Error New, NoDir, Kernel : %8lx\n", Status );
        return FALSE;
    }

    //
    //  Kernel mode, dir, and no new
    //

    NewDescriptor = NULL;
    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            TRUE,
                                            Token,
                                            &GenericMapping,
                                            KernelMode ))) {
        DbgPrint("SeAssign Error NoNew, Dir, Kernel : %8lx\n", Status );
        return FALSE;
    }

    //
    //  Kernel mode, dir, and new
    //

    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            TRUE,
                                            Token,
                                            &GenericMapping,
                                            KernelMode ))) {
        DbgPrint("SeAssign Error New, Dir, Kernel : %8lx\n", Status );
        return FALSE;
    }


    //
    //  User mode, non dir, and no new
    //

    NewDescriptor = NULL;
    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            FALSE,
                                            Token,
                                            &GenericMapping,
                                            UserMode ))) {
        DbgPrint("SeAssign Error NoNew, NoDir, User : %8lx\n", Status );
        return FALSE;
    }

    //
    //  User mode, non dir, and new
    //

    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            FALSE,
                                            Token,
                                            &GenericMapping,
                                            UserMode ))) {
        DbgPrint("SeAssign Error New, NoDir, User : %8lx\n", Status );
        return FALSE;
    }

    //
    //  User mode, dir, and no new
    //

    NewDescriptor = NULL;
    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            TRUE,
                                            Token,
                                            &GenericMapping,
                                            UserMode ))) {
        DbgPrint("SeAssign Error NoNew, Dir, User : %8lx\n", Status );
        return FALSE;
    }

    //
    //  User mode, dir, and new
    //

    if (!NT_SUCCESS(Status = SeAssignSecurity( &SecurityDescriptor,
                                            &NewDescriptor,
                                            TRUE,
                                            Token,
                                            &GenericMapping,
                                            UserMode ))) {
        DbgPrint("SeAssign Error New, Dir, User : %8lx\n", Status );
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
TestAccessCheck()
{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Acl;

    PACCESS_TOKEN Token;

    Acl = (PACL)ExAllocatePool( PagedPool, 1024 );
    BuildAcl( Fred, Acl, 1024 );

    Token = (PACCESS_TOKEN)ExAllocatePool( PagedPool, 512 );
    BuildToken( Fred, Token );

    DiscretionarySecurityDescriptor( &SecurityDescriptor, Acl );

    //
    //  Test should be successful based on aces
    //

    if (!SeAccessCheck( &SecurityDescriptor,
                        Token,
                        0x00000001,
                        NULL,
                        UserMode )) {
        DbgPrint("SeAccessCheck Error should allow access\n");
        return FALSE;
    }

    //
    //  Test should be successful based on owner
    //

    if (!SeAccessCheck( &SecurityDescriptor,
                        Token,
                        READ_CONTROL,
                        &FredGuid,
                        UserMode )) {
        DbgPrint("SeAccessCheck Error should allow owner\n");
        return FALSE;
    }

    //
    //  Test should be unsuccessful based on aces
    //

    if (SeAccessCheck( &SecurityDescriptor,
                       Token,
                       0x0000000f,
                       &FredGuid,
                       UserMode )) {
        DbgPrint("SeAccessCheck Error shouldn't allow access\n");
        return FALSE;
    }

    //
    //  Test should be unsuccessful based on non owner
    //

    if (SeAccessCheck( &SecurityDescriptor,
                       Token,
                       READ_CONTROL,
                       &BarneyGuid,
                       UserMode )) {
        DbgPrint("SeAccessCheck Error shouldn't allow non owner access\n");
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
TestGenerateMessage()
{
    return TRUE;
}

