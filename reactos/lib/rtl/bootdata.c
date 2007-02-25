/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Boot Data implementation
 * FILE:            lib/rtl/bootdata.c
 * PROGRAMMERS:     
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static SID_IDENTIFIER_AUTHORITY LocalSystemAuthority = {SECURITY_NT_AUTHORITY};

static NTSTATUS
RtlpSysVolCreateSecurityDescriptor(OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
                                   OUT PSID *SystemSid)
{
    PSECURITY_DESCRIPTOR AbsSD = NULL;
    PSID LocalSystemSid = NULL;
    PACL Dacl = NULL;
    ULONG DaclSize;
    NTSTATUS Status;

    /* create the local SYSTEM SID */
    Status = RtlAllocateAndInitializeSid(&LocalSystemAuthority,
                                         1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &LocalSystemSid);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* allocate and initialize the security descriptor */
    AbsSD = RtlpAllocateMemory(sizeof(SECURITY_DESCRIPTOR),
                               TAG('S', 'e', 'S', 'd'));
    if (AbsSD == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = RtlCreateSecurityDescriptor(AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* allocate and create the DACL */
    DaclSize = sizeof(ACL) + sizeof(ACE) +
               RtlLengthSid(LocalSystemSid);
    Dacl = RtlpAllocateMemory(DaclSize,
                              TAG('S', 'e', 'A', 'c'));
    if (Dacl == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = RtlCreateAcl(Dacl,
                          DaclSize,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlAddAccessAllowedAceEx(Dacl,
                                      ACL_REVISION,
                                      OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                                      STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                                      LocalSystemSid);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* set the DACL in the security descriptor */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          Dacl,
                                          FALSE);

    /* all done */
    if (NT_SUCCESS(Status))
    {
        *SecurityDescriptor = AbsSD;
        *SystemSid = LocalSystemSid;
    }
    else
    {
Cleanup:
        if (LocalSystemSid != NULL)
        {
            RtlFreeSid(LocalSystemSid);
        }

        if (Dacl != NULL)
        {
            RtlpFreeMemory(Dacl,
                           TAG('S', 'e', 'A', 'c'));
        }

        if (AbsSD != NULL)
        {
            RtlpFreeMemory(AbsSD,
                           TAG('S', 'e', 'S', 'd'));
        }
    }

    return Status;
}

static NTSTATUS
RtlpSysVolCheckOwnerAndSecurity(IN HANDLE DirectoryHandle,
                                IN PISECURITY_DESCRIPTOR SecurityDescriptor)
{
    PSECURITY_DESCRIPTOR RelSD = NULL;
    PSECURITY_DESCRIPTOR NewRelSD = NULL;
    PSECURITY_DESCRIPTOR AbsSD = NULL;
#ifdef _WIN64
    BOOLEAN AbsSDAllocated = FALSE;
#endif
    PSID AdminSid = NULL;
    PSID LocalSystemSid = NULL;
    ULONG DescriptorSize;
    ULONG AbsSDSize, RelSDSize = 0;
    PACL Dacl;
    BOOLEAN DaclPresent, DaclDefaulted;
    PSID OwnerSid;
    BOOLEAN OwnerDefaulted;
    ULONG AceIndex;
    PACE Ace = NULL;
    NTSTATUS Status;

    /* find out how much memory we need to allocate for the self-relative
       descriptor we're querying */
    Status = ZwQuerySecurityObject(DirectoryHandle,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   NULL,
                                   0,
                                   &DescriptorSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* looks like the FS doesn't support security... return success */
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    /* allocate enough memory for the security descriptor */
    RelSD = RtlpAllocateMemory(DescriptorSize,
                               TAG('S', 'e', 'S', 'd'));
    if (RelSD == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* query the self-relative security descriptor */
    Status = ZwQuerySecurityObject(DirectoryHandle,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   RelSD,
                                   DescriptorSize,
                                   &DescriptorSize);
    if (!NT_SUCCESS(Status))
    {
        /* FIXME - handle the case where someone else modified the owner and/or
                   DACL while we allocated memory. But that should be *very*
                   unlikely.... */
        goto Cleanup;
    }

    /* query the owner and DACL from the descriptor */
    Status = RtlGetOwnerSecurityDescriptor(RelSD,
                                           &OwnerSid,
                                           &OwnerDefaulted);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlGetDaclSecurityDescriptor(RelSD,
                                          &DaclPresent,
                                          &Dacl,
                                          &DaclDefaulted);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* create the Administrators SID */
    Status = RtlAllocateAndInitializeSid(&LocalSystemAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* create the local SYSTEM SID */
    Status = RtlAllocateAndInitializeSid(&LocalSystemAuthority,
                                         1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &LocalSystemSid);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* check if the Administrators are the owner and at least a not-NULL DACL
       is present */
    if (OwnerSid != NULL &&
        RtlEqualSid(OwnerSid,
                    AdminSid) &&
        DaclPresent && Dacl != NULL)
    {
        /* check the DACL for an Allowed ACE for the SYSTEM account */
        AceIndex = 0;
        do
        {
            Status = RtlGetAce(Dacl,
                               AceIndex++,
                               (PVOID*)&Ace);
            if (!NT_SUCCESS(Status))
            {
                Ace = NULL;
            }
            else if (Ace != NULL && Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
            {
                /* check if the the ACE is a set of allowed permissions for the
                   local SYSTEM account */
                if (RtlEqualSid((PSID)(Ace + 1),
                                LocalSystemSid))
                {
                    /* check if the ACE is inherited by noncontainer and
                       container objects, if not attempt to change that */
                    if (!(Ace->Header.AceFlags & OBJECT_INHERIT_ACE) ||
                        !(Ace->Header.AceFlags & CONTAINER_INHERIT_ACE))
                    {
                        Ace->Header.AceFlags |= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
                        Status = ZwSetSecurityObject(DirectoryHandle,
                                                     DACL_SECURITY_INFORMATION,
                                                     RelSD);
                    }
                    else
                    {
                        /* all done, we have access */
                        Status = STATUS_SUCCESS;
                    }

                    goto Cleanup;
                }
            }
        } while (Ace != NULL);
    }

    AbsSDSize = DescriptorSize;

    /* because we need to change any existing data we need to convert it to
       an absolute security descriptor first */
    Status = RtlSelfRelativeToAbsoluteSD2(RelSD,
                                          &AbsSDSize);
#ifdef _WIN64
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* this error code can only be returned on 64 bit builds because
           the size of an absolute security descriptor is greater than the
           size of a self-relative security descriptor */
        ASSERT(AbsSDSize > DescriptorSize);

        AbsSD = RtlpAllocateMemory(DescriptorSize,
                                   TAG('S', 'e', 'S', 'd'));
        if (AbsSD == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        AbsSDAllocated = TRUE;

        /* make a raw copy of the self-relative descriptor */
        RtlCopyMemory(AbsSD,
                      RelSD,
                      DescriptorSize);

        /* finally convert it */
        Status = RtlSelfRelativeToAbsoluteSD2(AbsSD,
                                              &AbsSDSize);
    }
    else
#endif
    {
        AbsSD = RelSD;
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* set the owner SID */
    Status = RtlSetOwnerSecurityDescriptor(AbsSD,
                                           AdminSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* set the DACL in the security descriptor */
    Status = RtlSetDaclSecurityDescriptor(AbsSD,
                                          TRUE,
                                          SecurityDescriptor->Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* convert it back to a self-relative descriptor, find out how much
       memory we need */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        goto Cleanup;
    }

    /* allocate enough memory for the new self-relative descriptor */
    NewRelSD = RtlpAllocateMemory(RelSDSize,
                                  TAG('S', 'e', 'S', 'd'));
    if (NewRelSD == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* convert the security descriptor to self-relative format */
    Status = RtlAbsoluteToSelfRelativeSD(AbsSD,
                                         NewRelSD,
                                         &RelSDSize);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        goto Cleanup;
    }

    /* finally attempt to change the security information */
    Status = ZwSetSecurityObject(DirectoryHandle,
                                 OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                 NewRelSD);

Cleanup:
    if (AdminSid != NULL)
    {
        RtlFreeSid(AdminSid);
    }

    if (LocalSystemSid != NULL)
    {
        RtlFreeSid(LocalSystemSid);
    }

    if (RelSD != NULL)
    {
        RtlpFreeMemory(RelSD,
                       TAG('S', 'e', 'S', 'd'));
    }

    if (NewRelSD != NULL)
    {
        RtlpFreeMemory(NewRelSD,
                       TAG('S', 'e', 'S', 'd'));
    }

#ifdef _WIN64
    if (AbsSDAllocated)
    {
        RtlpFreeMemory(AbsSD,
                       TAG('S', 'e', 'S', 'd'));
    }
#endif

    return Status;
}

static NTSTATUS
RtlpSysVolTakeOwnership(IN PUNICODE_STRING DirectoryPath,
                        IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    TOKEN_PRIVILEGES TokenPrivileges;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_DESCRIPTOR AbsSD;
    PSID AdminSid = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TokenEnabled = FALSE;
    HANDLE hToken = NULL;
    HANDLE hDirectory = NULL;
    NTSTATUS Status;

    Status = ZwOpenProcessToken(NtCurrentProcess(),
                                TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                                &hToken);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* attempt to enable the SE_TAKE_OWNERSHIP_PRIVILEGE privilege */
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid.LowPart = SE_TAKE_OWNERSHIP_PRIVILEGE;
    TokenPrivileges.Privileges[0].Luid.HighPart = 0;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    Status = ZwAdjustPrivilegesToken(hToken,
                                     FALSE,
                                     &TokenPrivileges,
                                     sizeof(TokenPrivileges),
                                     &TokenPrivileges,
                                     NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    TokenEnabled = (TokenPrivileges.PrivilegeCount != 0);

    /* open the directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               DirectoryPath,
                               0,
                               NULL,
                               SecurityDescriptor);

    Status = ZwOpenFile(&hDirectory,
                        SYNCHRONIZE | WRITE_OWNER,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* create the Administrators SID */
    Status = RtlAllocateAndInitializeSid(&LocalSystemAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* create the security descriptor */
    Status = RtlCreateSecurityDescriptor(&AbsSD,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlSetOwnerSecurityDescriptor(&AbsSD,
                                           AdminSid,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* attempt to take ownership */
    Status = ZwSetSecurityObject(hDirectory,
                                 OWNER_SECURITY_INFORMATION,
                                 &AbsSD);

Cleanup:
    if (TokenEnabled)
    {
        ZwAdjustPrivilegesToken(hToken,
                                FALSE,
                                &TokenPrivileges,
                                0,
                                NULL,
                                NULL);
    }

    if (AdminSid != NULL)
    {
        RtlFreeSid(AdminSid);
    }

    if (hDirectory != NULL)
    {
        ZwClose(hDirectory);
    }

    if (hToken != NULL)
    {
        ZwClose(hToken);
    }

    return Status;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlCreateSystemVolumeInformationFolder(
	IN PUNICODE_STRING VolumeRootPath
	)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hDirectory;
    UNICODE_STRING DirectoryName, NewPath;
    ULONG PathLen;
    PISECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PSID SystemSid = NULL;
    BOOLEAN AddSep = FALSE;
    NTSTATUS Status;

    PAGED_CODE_RTL();

    RtlInitUnicodeString(&DirectoryName,
                         L"System Volume Information");

    PathLen = VolumeRootPath->Length + DirectoryName.Length;

    /* make sure we don't overflow while appending the strings */
    if (PathLen > 0xFFFC)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (VolumeRootPath->Buffer[(VolumeRootPath->Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        AddSep = TRUE;
        PathLen += sizeof(WCHAR);
    }
    
    /* allocate the new string */
    NewPath.MaximumLength = (USHORT)PathLen + sizeof(WCHAR);
    NewPath.Buffer = RtlpAllocateStringMemory(NewPath.MaximumLength,
                                              TAG_USTR);
    if (NewPath.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* create the new path string */
    NewPath.Length = VolumeRootPath->Length;
    RtlCopyMemory(NewPath.Buffer,
                  VolumeRootPath->Buffer,
                  NewPath.Length);
    if (AddSep)
    {
        NewPath.Buffer[NewPath.Length / sizeof(WCHAR)] = L'\\';
        NewPath.Length += sizeof(WCHAR);
    }
    RtlCopyMemory(NewPath.Buffer + (NewPath.Length / sizeof(WCHAR)),
                  DirectoryName.Buffer,
                  DirectoryName.Length);
    NewPath.Length += DirectoryName.Length;
    NewPath.Buffer[NewPath.Length / sizeof(WCHAR)] = L'\0';

    ASSERT(NewPath.Length == PathLen);
    ASSERT(NewPath.Length == NewPath.MaximumLength - sizeof(WCHAR));

    /* create the security descriptor for the new directory */
    Status = RtlpSysVolCreateSecurityDescriptor(&SecurityDescriptor,
                                                &SystemSid);
    if (NT_SUCCESS(Status))
    {
        /* create or open the directory */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &NewPath,
                                   0,
                                   NULL,
                                   SecurityDescriptor);

        Status = ZwCreateFile(&hDirectory,
                              SYNCHRONIZE | WRITE_OWNER | WRITE_DAC | READ_CONTROL,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              FILE_OPEN_IF,
                              FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0);
        if (!NT_SUCCESS(Status))
        {
            Status = RtlpSysVolTakeOwnership(&NewPath,
                                             SecurityDescriptor);

            if (NT_SUCCESS(Status))
            {
                /* successfully took ownership, attempt to open it */
                Status = ZwCreateFile(&hDirectory,
                                      SYNCHRONIZE | WRITE_OWNER | WRITE_DAC | READ_CONTROL,
                                      &ObjectAttributes,
                                      &IoStatusBlock,
                                      NULL,
                                      FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      FILE_OPEN_IF,
                                      FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                      NULL,
                                      0);
            }
        }

        if (NT_SUCCESS(Status))
        {
            /* check security now and adjust it if neccessary */
            Status = RtlpSysVolCheckOwnerAndSecurity(hDirectory,
                                                     SecurityDescriptor);
            ZwClose(hDirectory);
        }

        /* free allocated memory */
        ASSERT(SecurityDescriptor != NULL);
        ASSERT(SecurityDescriptor->Dacl != NULL);

        RtlpFreeMemory(SecurityDescriptor->Dacl,
                       TAG('S', 'e', 'A', 'c'));
        RtlpFreeMemory(SecurityDescriptor,
                       TAG('S', 'e', 'S', 'd'));

        RtlFreeSid(SystemSid);
    }

    RtlpFreeStringMemory(NewPath.Buffer,
                         TAG_USTR);
    return Status;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlGetSetBootStatusData(
	HANDLE Filehandle,
	BOOLEAN WriteMode,
	DWORD DataClass,
	PVOID Buffer,
	ULONG BufferSize,
	DWORD DataClass2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlLockBootStatusData(
	HANDLE Filehandle
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlUnlockBootStatusData(
	HANDLE Filehandle
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
