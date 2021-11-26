/*
* PROJECT:         ReactOS kernel-mode tests
* LICENSE:         GPLv2+ - See COPYING in the top level directory
* PURPOSE:         Kernel-Mode Test Suite Process Notification Routines test
* PROGRAMMER:      Constantine Belev (Moscow State Technical University)
*                  Denis Grishin (Moscow State Technical University)
*                  Egor Sinitsyn (Moscow State Technical University)
*/

#include <kmt_test.h>
#include <ntifs.h>

#define NDEBUG
#include <debug.h>

//------------------------------------------------------------------------------//
//      Testing Functions                                                       //
//------------------------------------------------------------------------------//

// Testing function for SQIT

void TestsSeQueryInformationToken(PACCESS_TOKEN Token)
{
    NTSTATUS Status;
    PVOID Buffer = NULL;
    PSID sid;
    PTOKEN_OWNER Towner;
    PTOKEN_DEFAULT_DACL TDefDacl;
    PTOKEN_GROUPS TGroups;
    ULONG GroupCount;
    PACL acl;
    PTOKEN_STATISTICS TStats;
    PTOKEN_TYPE TType;
    PTOKEN_USER TUser;
    BOOLEAN Flag;
    ULONG i;

    //----------------------------------------------------------------//
    // Testing SeQueryInformationToken with various args              //
    //----------------------------------------------------------------//

    ok(Token != NULL, "Token is not captured. Testing SQIT interrupted\n\n");

    if (Token == NULL) return;

    Status = SeQueryInformationToken(Token, TokenOwner, &Buffer);
    ok((Status == STATUS_SUCCESS), "SQIT with TokenOwner arg fails with status 0x%08X\n", Status);
    if (Status == STATUS_SUCCESS)
    {
        ok(Buffer != NULL, "Wrong. SQIT call was successful with TokenOwner arg. But Buffer == NULL\n");

        if (Buffer)
        {
            Towner = (TOKEN_OWNER *)Buffer;
            sid = Towner->Owner;
            ok((RtlValidSid(sid) == TRUE), "TokenOwner's SID is not a valid SID\n");
            ExFreePool(Buffer);
        }
    }

    //----------------------------------------------------------------//

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenDefaultDacl, &Buffer);
    ok(Status == STATUS_SUCCESS, "SQIT with TokenDefaultDacl fails with status 0x%08X\n", Status);
    if (Status == STATUS_SUCCESS)
    {
        ok(Buffer != NULL, "Wrong. SQIT call was successful with TokenDefaultDacl arg. But Buffer == NULL\n");
        if (Buffer)
        {
            TDefDacl = (PTOKEN_DEFAULT_DACL)Buffer;
            acl = TDefDacl->DefaultDacl;
            ok(((acl->AclRevision == ACL_REVISION || acl->AclRevision == ACL_REVISION_DS) == TRUE), "DACL is invalid\n");
            ExFreePool(Buffer);
        }
    }

    //----------------------------------------------------------------//

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenGroups, &Buffer);
    ok(Status == STATUS_SUCCESS, "SQIT with TokenGroups fails with status 0x%08X\n", Status);
    if (Status == STATUS_SUCCESS)
    {
        ok(Buffer != NULL, "Wrong. SQIT call was successful with TokenGroups arg. But Buffer == NULL\n");
        if (Buffer)
        {
            TGroups = (PTOKEN_GROUPS)Buffer;
            GroupCount = TGroups->GroupCount;
            Flag = TRUE;
            for (i = 0; i < GroupCount; i++)
            {
                sid = TGroups->Groups[i].Sid;
                if (!RtlValidSid(sid))
                {
                    Flag = FALSE;
                    break;
                }
            }
            ok((Flag == TRUE), "TokenGroup's SIDs are not valid\n");
            ExFreePool(Buffer);
        }
    }

    //----------------------------------------------------------------//

    // Call SQIT with TokenImpersonationLevel argument. Although our token
    // is not an impersonation token, the call will outright fail.

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenImpersonationLevel, &Buffer);
    ok(Status == STATUS_INVALID_INFO_CLASS, "SQIT with TokenImpersonationLevel must return STATUS_INVALID_INFO_CLASS but got 0x%08X\n", Status);
    ok(Buffer == NULL, "SQIT has failed to query the impersonation level but buffer is not NULL!\n");

    //----------------------------------------------------------------//

    // Call SQIT with the 4 classes (TokenOrigin, TokenGroupsAndPrivileges,
    // TokenRestrictedSids and TokenSandBoxInert) are not supported by
    // SeQueryInformationToken (only NtQueryInformationToken supports them).
    //

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenOrigin, &Buffer);
    ok(Status == STATUS_INVALID_INFO_CLASS, "SQIT with TokenOrigin failed with Status 0x%08X; expected STATUS_INVALID_INFO_CLASS\n", Status);
    ok(Buffer == NULL, "Wrong. SQIT call failed. But Buffer != NULL\n");

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenGroupsAndPrivileges, &Buffer);
    ok(Status == STATUS_INVALID_INFO_CLASS, "SQIT with TokenGroupsAndPrivileges failed with Status 0x%08X; expected STATUS_INVALID_INFO_CLASS\n", Status);
    ok(Buffer == NULL, "Wrong. SQIT call failed. But Buffer != NULL\n");

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenRestrictedSids, &Buffer);
    ok(Status == STATUS_INVALID_INFO_CLASS, "SQIT with TokenRestrictedSids failed with Status 0x%08X; expected STATUS_INVALID_INFO_CLASS\n", Status);
    ok(Buffer == NULL, "Wrong. SQIT call failed. But Buffer != NULL\n");

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenSandBoxInert, &Buffer);
    ok(Status == STATUS_INVALID_INFO_CLASS, "SQIT with TokenSandBoxInert failed with Status 0x%08X; expected STATUS_INVALID_INFO_CLASS\n", Status);
    ok(Buffer == NULL, "Wrong. SQIT call failed. But Buffer != NULL\n");

    //----------------------------------------------------------------//

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenStatistics, &Buffer);
    ok(Status == STATUS_SUCCESS, "SQIT with TokenStatistics fails with status 0x%08X\n", Status);
    if (Status == STATUS_SUCCESS)
    {
        ok(Buffer != NULL, "Wrong. SQIT call was successful with TokenStatistics arg. But Buffer == NULL\n");
        if (Buffer)
        {
            TStats = (PTOKEN_STATISTICS)Buffer;
            // just put 0 into 1st arg or use trace to print TokenStatistics
            ok(1, "print statistics:\n\tTokenID = %u_%d\n\tSecurityImperLevel = %d\n\tPrivCount = %d\n\tGroupCount = %d\n\n", TStats->TokenId.LowPart,
                TStats->TokenId.HighPart,
                TStats->ImpersonationLevel,
                TStats->PrivilegeCount,
                TStats->GroupCount
                );
            ExFreePool(Buffer);
        }
    } else {
        ok(Buffer == NULL, "Wrong. SQIT call failed. But Buffer != NULL\n");
    }

    //----------------------------------------------------------------//

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenType, &Buffer);
    ok(Status == STATUS_SUCCESS, "SQIT with TokenType fails with status 0x%08X\n", Status);
    if (Status == STATUS_SUCCESS)
    {
        ok(Buffer != NULL, "Wrong. SQIT call was successful with TokenType arg. But Buffer == NULL\n");
        if (Buffer)
        {
            TType = (PTOKEN_TYPE)Buffer;
            ok((*TType == TokenPrimary || *TType == TokenImpersonation), "TokenType in not a primary nor impersonation. FAILED\n");
            ExFreePool(Buffer);
        }
    }

    //----------------------------------------------------------------//

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenUser, &Buffer);
    ok(Status == STATUS_SUCCESS, "SQIT with TokenUser fails\n");
    if (Status == STATUS_SUCCESS)
    {
        ok(Buffer != NULL, "Wrong. SQIT call was successful with TokenUser arg. But Buffer == NULL\n");
        if (Buffer)
        {
            TUser = (PTOKEN_USER)Buffer;
            ok(RtlValidSid(TUser->User.Sid), "TokenUser has an invalid Sid\n");
            ExFreePool(Buffer);
        }
    }

    //----------------------------------------------------------------//

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenSandBoxInert, &Buffer);
    ok(Status != STATUS_SUCCESS, "SQIT must fail with wrong TOKEN_INFORMATION_CLASS arg\n");
}

//------------------------------------------------------------------------------//

//------------------------------------------------------------------------------//
//      Body of the main test                                                   //
//------------------------------------------------------------------------------//

START_TEST(SeQueryInfoToken)
{
    PACCESS_STATE AccessState;
    ACCESS_MASK AccessMask = MAXIMUM_ALLOWED;
    ACCESS_MASK DesiredAccess = MAXIMUM_ALLOWED;
    NTSTATUS Status = STATUS_SUCCESS;
    PAUX_ACCESS_DATA AuxData = NULL;
    PPRIVILEGE_SET NewPrivilegeSet;
    BOOLEAN Checker;
    PPRIVILEGE_SET Privileges = NULL;
    PSECURITY_SUBJECT_CONTEXT SubjectContext = NULL;
    PACCESS_TOKEN Token = NULL;
    PTOKEN_PRIVILEGES TPrivileges;
    PVOID Buffer;
    POBJECT_TYPE PsProcessType = NULL;
    PGENERIC_MAPPING GenericMapping;
    ULONG i;

    SubjectContext = ExAllocatePool(PagedPool, sizeof(SECURITY_SUBJECT_CONTEXT));

    SeCaptureSubjectContext(SubjectContext);
    SeLockSubjectContext(SubjectContext);
    Token = SeQuerySubjectContextToken(SubjectContext);

    // Testing SQIT with current Token
    TestsSeQueryInformationToken(Token);

    //----------------------------------------------------------------//
    //      Creating an ACCESS_STATE structure                        //
    //----------------------------------------------------------------//

    AccessState = ExAllocatePool(PagedPool, sizeof(ACCESS_STATE));
    PsProcessType = ExAllocatePool(PagedPool, sizeof(OBJECT_TYPE));
    AuxData = ExAllocatePool(PagedPool, 0xC8);
    GenericMapping = ExAllocatePool(PagedPool, sizeof(GENERIC_MAPPING));

    Status = SeCreateAccessState(AccessState,
                                 (PVOID)AuxData,
                                 DesiredAccess,
                                 GenericMapping
                                );

    ok((Status == STATUS_SUCCESS), "SeCreateAccessState failed with Status 0x%08X\n", Status);

    SeCaptureSubjectContext(&AccessState->SubjectSecurityContext);
    SeLockSubjectContext(&AccessState->SubjectSecurityContext);
    Token = SeQuerySubjectContextToken(&AccessState->SubjectSecurityContext);

    // Testing SQIT with AccessState Token
    TestsSeQueryInformationToken(Token);

    //----------------------------------------------------------------//
    //      Testing other functions                                   //
    //----------------------------------------------------------------//

    //----------------------------------------------------------------//
    //      Testing SeAppendPrivileges                                //
    //----------------------------------------------------------------//

    AuxData->PrivilegeSet->PrivilegeCount = 1;

    //  Testing SeAppendPrivileges. Must change PrivilegeCount to 2 (1 + 1)

    NewPrivilegeSet = ExAllocatePool(PagedPool, sizeof(PRIVILEGE_SET));
    NewPrivilegeSet->PrivilegeCount = 1;

    Status = SeAppendPrivileges(AccessState, NewPrivilegeSet);
    ok(Status == STATUS_SUCCESS, "SeAppendPrivileges failed\n");
    ok((AuxData->PrivilegeSet->PrivilegeCount == 2),"PrivelegeCount must be 2, but it is %d\n", AuxData->PrivilegeSet->PrivilegeCount);
    ExFreePool(NewPrivilegeSet);

    //----------------------------------------------------------------//

    // Testing SeAppendPrivileges. Must change PrivilegeCount to 6 (2 + 4)

    NewPrivilegeSet = ExAllocatePool(PagedPool, 4*sizeof(PRIVILEGE_SET));
    NewPrivilegeSet->PrivilegeCount = 4;

    Status = SeAppendPrivileges(AccessState, NewPrivilegeSet);
    ok(Status == STATUS_SUCCESS, "SeAppendPrivileges failed\n");
    ok((AuxData->PrivilegeSet->PrivilegeCount == 6),"PrivelegeCount must be 6, but it is %d\n", AuxData->PrivilegeSet->PrivilegeCount);
    ExFreePool(NewPrivilegeSet);

    //----------------------------------------------------------------//
    //      Testing SePrivilegeCheck                                  //
    //----------------------------------------------------------------//

    // KPROCESSOR_MODE is set to KernelMode ===> Always return TRUE
    ok(SePrivilegeCheck(AuxData->PrivilegeSet, &(AccessState->SubjectSecurityContext), KernelMode), "SePrivilegeCheck failed with KernelMode mode arg\n");
    // and call it again
    ok(SePrivilegeCheck(AuxData->PrivilegeSet, &(AccessState->SubjectSecurityContext), KernelMode), "SePrivilegeCheck failed with KernelMode mode arg\n");

    //----------------------------------------------------------------//

    // KPROCESSOR_MODE is set to UserMode. Expect false
    ok(!SePrivilegeCheck(AuxData->PrivilegeSet, &(AccessState->SubjectSecurityContext), UserMode), "SePrivilegeCheck unexpected success with UserMode arg\n");

    //----------------------------------------------------------------//

    //----------------------------------------------------------------//
    //      Testing SeFreePrivileges                                  //
    //----------------------------------------------------------------//

    Privileges = NULL;
    Checker = SeAccessCheck(
        AccessState->SecurityDescriptor,
        &AccessState->SubjectSecurityContext,
        FALSE,
        AccessState->OriginalDesiredAccess,
        AccessState->PreviouslyGrantedAccess,
        &Privileges,
        (PGENERIC_MAPPING)((PCHAR*)PsProcessType + 52),
        KernelMode,
        &AccessMask,
        &Status
        );
    ok(Checker, "Checker is NULL\n");
    ok((Privileges != NULL), "Privileges is NULL\n");
    if (Privileges)
    {
        trace("AuxData->PrivilegeSet->PrivilegeCount = %d ; Privileges->PrivilegeCount = %d\n",
              AuxData->PrivilegeSet->PrivilegeCount, Privileges->PrivilegeCount);
    }
    if (Privileges) SeFreePrivileges(Privileges);


    //----------------------------------------------------------------//
    //      Testing SePrivilegeCheck                                  //
    //----------------------------------------------------------------//
    // I'm trying to make success call of SePrivilegeCheck from UserMode
    // If we sets Privileges properly, can we expect true from SePrivilegeCheck?
    // answer: yes
    // This test demonstrates it

    Buffer = NULL;
    Status = SeQueryInformationToken(Token, TokenPrivileges, &Buffer);
    if (Status == STATUS_SUCCESS)
    {
        ok(Buffer != NULL, "Wrong. SQIT call was successful with TokenPrivileges arg. But Buffer == NULL\n");
        if (Buffer)
        {
            TPrivileges = (PTOKEN_PRIVILEGES)(Buffer);
            //trace("TPCount = %u\n\n", TPrivileges->PrivilegeCount);

            NewPrivilegeSet = ExAllocatePool(PagedPool, 14*sizeof(PRIVILEGE_SET));
            NewPrivilegeSet->PrivilegeCount = 14;

            ok((SeAppendPrivileges(AccessState, NewPrivilegeSet)) == STATUS_SUCCESS, "SeAppendPrivileges failed\n");
            ok((AuxData->PrivilegeSet->PrivilegeCount == 20),"PrivelegeCount must be 20, but it is %d\n", AuxData->PrivilegeSet->PrivilegeCount);
            ExFreePool(NewPrivilegeSet);
            for (i = 0; i < AuxData->PrivilegeSet->PrivilegeCount; i++)
            {
                AuxData->PrivilegeSet->Privilege[i].Attributes = TPrivileges->Privileges[i].Attributes;
                AuxData->PrivilegeSet->Privilege[i].Luid = TPrivileges->Privileges[i].Luid;
            }
            //trace("AccessState->privCount = %u\n\n", ((PAUX_ACCESS_DATA)(AccessState->AuxData))->PrivilegeSet->PrivilegeCount);

            ok(SePrivilegeCheck(AuxData->PrivilegeSet, &(AccessState->SubjectSecurityContext), UserMode), "SePrivilegeCheck fails in UserMode, but I wish it will success\n");
        }
    }

    // Call SeFreePrivileges again

    Privileges = NULL;
    Checker = SeAccessCheck(
        AccessState->SecurityDescriptor,
        &AccessState->SubjectSecurityContext,
        TRUE,
        AccessState->OriginalDesiredAccess,
        AccessState->PreviouslyGrantedAccess,
        &Privileges,
        (PGENERIC_MAPPING)((PCHAR*)PsProcessType + 52),
        KernelMode,
        &AccessMask,
        &Status
        );
    ok(Checker, "Checker is NULL\n");
    ok((Privileges != NULL), "Privileges is NULL\n");
    if (Privileges)
    {
        trace("AuxData->PrivilegeSet->PrivilegeCount = %d ; Privileges->PrivilegeCount = %d\n",
              AuxData->PrivilegeSet->PrivilegeCount, Privileges->PrivilegeCount);
    }
    if (Privileges) SeFreePrivileges(Privileges);

    //----------------------------------------------------------------//
    //      Missing for now                                           //
    //----------------------------------------------------------------//

    SeUnlockSubjectContext(&AccessState->SubjectSecurityContext);
    SeUnlockSubjectContext(SubjectContext);

    SeDeleteAccessState(AccessState);

    if (GenericMapping) ExFreePool(GenericMapping);
    if (PsProcessType) ExFreePool(PsProcessType);
    if (SubjectContext) ExFreePool(SubjectContext);
    if (AuxData) ExFreePool(AuxData);
    if (AccessState) ExFreePool(AccessState);
}
