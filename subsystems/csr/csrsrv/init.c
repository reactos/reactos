/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsys/csr/csrsrv/init.c
 * PURPOSE:         CSR Server DLL Initialization
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

HANDLE CsrObjectDirectory;
ULONG SessionId;
BOOLEAN CsrProfileControl;
UNICODE_STRING CsrDirectoryName;
HANDLE CsrHeap;
HANDLE BNOLinksDirectory;
HANDLE SessionObjectDirectory;
HANDLE DosDevicesDirectory;
HANDLE CsrInitializationEvent;
SYSTEM_BASIC_INFORMATION CsrNtSysInfo;

/* PRIVATE FUNCTIONS *********************************************************/

/*++
 * @name CsrPopulateDosDevicesDirectory
 *
 * The CsrPopulateDosDevicesDirectory routine uses the DOS Device Map from the
 * Kernel to populate the Dos Devices Object Directory for the session.
 *
 * @param TODO.
 *
 * @return TODO.
 *
 * @remarks TODO.
 *
 *--*/
NTSTATUS
NTAPI
CsrPopulateDosDevicesDirectory(IN HANDLE hDosDevicesDirectory,
                               IN PPROCESS_DEVICEMAP_INFORMATION DeviceMap)
{
    WCHAR SymLinkBuffer[0x1000];
    UNICODE_STRING GlobalString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hDirectory = 0;
    NTSTATUS Status;
    ULONG ReturnLength = 0;
    ULONG BufferLength = 0x4000;
    ULONG Context;
    POBJECT_DIRECTORY_INFORMATION QueryBuffer;
    HANDLE hSymLink;
    UNICODE_STRING LinkTarget;

    /* Initialize the Global String */
    RtlInitUnicodeString(&GlobalString, GLOBAL_ROOT);

    /* Initialize the Object Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &GlobalString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the directory */
    Status = NtOpenDirectoryObject(&hDirectory,
                                   DIRECTORY_QUERY,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return Status;

    /*  Allocate memory */
    QueryBuffer = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, 0x4000);
    if (!QueryBuffer) return STATUS_NO_MEMORY;

    /* Start query loop */
    while (TRUE)
    {
        /* Query the Directory */
        Status = NtQueryDirectoryObject(hDirectory,
                                        QueryBuffer,
                                        BufferLength,
                                        FALSE,
                                        FALSE,
                                        &Context,
                                        &ReturnLength);

        /* Check for the status */
        if (NT_SUCCESS(Status))
        {
            /* Make sure it has a name */
            if (!QueryBuffer->Name.Buffer[0]) continue;

            /* Check if it's actually a symbolic link */
            if (wcscmp(QueryBuffer->TypeName.Buffer, SYMLINK_NAME))
            {
                /* It is, open it */
                InitializeObjectAttributes(&ObjectAttributes,
                                           &QueryBuffer->Name,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           hDirectory);
                Status = NtOpenSymbolicLinkObject(&hSymLink,
                                                  SYMBOLIC_LINK_QUERY,
                                                  &ObjectAttributes);
                if (NT_SUCCESS(Status))
                {
                    /* Setup the Target String */
                    LinkTarget.Length = 0;
                    LinkTarget.MaximumLength = sizeof(SymLinkBuffer);
                    LinkTarget.Buffer = SymLinkBuffer;
                    
                    /* Query the target */
                    Status = NtQuerySymbolicLinkObject(hSymLink,
                                                       &LinkTarget,
                                                       &ReturnLength);

                    /* Close the handle */
                    NtClose(hSymLink);

                }
            }
        }
    }
}

/*++
 * @name CsrLoadServerDllFromCommandLine
 *
 * The CsrLoadServerDllFromCommandLine routine loads a Server DLL from the
 * CSRSS command-line in the registry.
 *
 * @param KeyValue
 *        Pointer to the specially formatted string for this Server DLL.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrLoadServerDllFromCommandLine(PCHAR KeyValue)
{
    PCHAR EntryPoint = NULL;
    ULONG DllIndex = 0;
    PCHAR ServerString = KeyValue;
    NTSTATUS Status;

    /* Loop the command line */
    while (*ServerString)
    {
        /* Check for the Entry Point */
        if ((*ServerString == ':') && (!EntryPoint))
        {
            /* Found it. Add a nullchar and save it */
            *ServerString++ = '\0';
            EntryPoint = ServerString;
        }

        /* Check for the Dll Index */
        if (*ServerString++ == ',')
        {
            /* Convert it to a ULONG */
            Status = RtlCharToInteger(ServerString, 10, &DllIndex);

            /* Add a null char if it was valid */
            if (NT_SUCCESS(Status)) ServerString[-1] = '\0';

            /* We're done here */
            break;
        }
    }

    /* We've got the name, entrypoint and index, load it */
    return CsrLoadServerDll(KeyValue, EntryPoint, DllIndex);
}

/*++
 * @name CsrpParseCommandLine
 *
 * The CsrpParseCommandLine routine parses the CSRSS command-line in the
 * registry and performs operations for each entry found.
 *
 * @param ArgumentCount
 *        Number of arguments on the command line.
 *
 * @param Arguments
 *        Array of arguments.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
FASTCALL
CsrpParseCommandLine(IN ULONG ArgumentCount,
                     IN PCHAR Arguments[])
{
    NTSTATUS Status;
    PCHAR ParameterName = NULL;
    PCHAR ParameterValue = NULL;
    ULONG i;

    /* Set the Defaults */
    CsrTotalPerProcessDataLength = 0;
    CsrObjectDirectory = 0;
    CsrMaxApiRequestThreads = 16;

    /* Save our Session ID, and create a Directory for it */
    SessionId = NtCurrentPeb()->SessionId;
    Status = CsrCreateSessionObjectDirectory(SessionId);
    if (NT_SUCCESS(Status))
    {
        DPRINT1("CSRSS: CsrCreateSessionObjectDirectory failed (%lx)\n",
                Status);

        /* It's not fatal if the SID is 0 */
        if (SessionId != 0) return Status;
    }

    /* Loop through every argument */
    for (i = 1; i < ArgumentCount; i++)
    {
        /* Split Name and Value */
        ParameterName = Arguments[i];
        ParameterValue = strchr(ParameterName, L'=');
        *ParameterValue++ = '\0';
        DPRINT("Name=%S, Value=%S\n", ParameterName, ParameterValue);

        /* Check for Object Directory */
        if (!_stricmp(ParameterName, "ObjectDirectory"))
        {
            CsrCreateObjectDirectory(ParameterValue);
        }
        else if(!_stricmp(ParameterName, "SubSystemType"))
        {
            /* Ignored */
            Status = STATUS_SUCCESS;
        }
        else if (!_stricmp(ParameterName, "MaxRequestThreads"))
        {
            Status = RtlCharToInteger(ParameterValue,
                                      0,
                                      &CsrMaxApiRequestThreads);
        }
        else if (!_stricmp(ParameterName, "RequestThreads"))
        {
            /* Ignored */
            Status = STATUS_SUCCESS;
        }
        else if (!_stricmp(ParameterName, "ProfileControl"))
        {
            CsrProfileControl = (!_stricmp(ParameterValue, "On")) ? TRUE : FALSE;
        }
        else if (!_stricmp(ParameterName, "SharedSection"))
        {
            /* Craete the Section */
            Status = CsrSrvCreateSharedSection(ParameterValue);

            /* Load us */
            Status = CsrLoadServerDll("CSRSS", NULL, CSR_SRV_SERVER);
        }
        else if (!_stricmp(ParameterName, "ServerDLL"))
        {
            /* Parse the Command-Line and load this DLL */
            Status = CsrLoadServerDllFromCommandLine(ParameterValue);
        }
        else if (!_stricmp(ParameterName, "Windows"))
        {
            /* Ignored */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Invalid parameter on the command line */
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    /* Return status */
    return Status;
}

/*++
 * @name CsrCreateObjectDirectory
 *
 * The CsrCreateObjectDirectory creates the Object Directory on the CSRSS
 * command-line from the registry.
 *
 * @param ObjectDirectory
 *        Pointer to the name of the Object Directory to create.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCreateObjectDirectory(IN PCHAR ObjectDirectory)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ANSI_STRING TempString;
    OBJECT_ATTRIBUTES DirectoryAttributes;

    DPRINT("CSRSRV:%s(%s) called\n", __FUNCTION__, ObjectDirectory);

    /* Convert the parameter to our Global Unicode name */
    RtlInitAnsiString(&TempString, ObjectDirectory);
    Status = RtlAnsiStringToUnicodeString(&CsrDirectoryName, &TempString, TRUE);

    /* Initialize the attributes for the Directory */
    InitializeObjectAttributes(&DirectoryAttributes,
                               &CsrDirectoryName,
                               OBJ_PERMANENT | OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create it */
    Status = NtCreateDirectoryObject(&CsrObjectDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &DirectoryAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: fatal: NtCreateDirectoryObject failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
    }

    /* Set the Security */
    Status = CsrSetDirectorySecurity(CsrObjectDirectory);

    /* Return */
    return Status;
}

/*++
 * @name CsrCreateLocalSystemSD
 *
 * The CsrCreateLocalSystemSD routine creates a Security Descriptor for
 * the local account with PORT_ALL_ACCESS.
 *
 * @param LocalSystemSd
 *        Pointer to a pointer to the security descriptor to create.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCreateLocalSystemSD(OUT PSECURITY_DESCRIPTOR *LocalSystemSd)
{
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID SystemSid;
    ULONG SidLength;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl;
    NTSTATUS Status;

    /* Initialize the System SID */
    RtlAllocateAndInitializeSid(&NtSidAuthority,
                                1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                &SystemSid);

    /* Get the length of the SID */
    SidLength = RtlLengthSid(SystemSid);

    /* Allocate a buffer for the Security Descriptor, with SID and DACL */
    SecurityDescriptor = RtlAllocateHeap(CsrHeap,
                                         0,
                                         SECURITY_DESCRIPTOR_MIN_LENGTH +
                                         sizeof(ACL) + SidLength +
                                         sizeof(ACCESS_ALLOWED_ACE));

    /* Set the pointer to the DACL */
    Dacl = (PACL)((ULONG_PTR)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);

    /* Now create the SD itself */
    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(CsrHeap, 0, SecurityDescriptor);
        return Status;
    }

    /* Create the DACL for it*/
    RtlCreateAcl(Dacl,
                 sizeof(ACL) + SidLength + sizeof(ACCESS_ALLOWED_ACE),
                 ACL_REVISION2);

    /* Create the ACE */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    PORT_ALL_ACCESS,
                                    SystemSid);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(CsrHeap, 0, SecurityDescriptor);
        return Status;
    }

    /* Clear the DACL in the SD */
    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(CsrHeap, 0, SecurityDescriptor);
        return Status;
    }

    /* Free the SID and return*/
    RtlFreeSid(SystemSid);
    *LocalSystemSd = SecurityDescriptor;
    return Status;
}

/*++
 * @name CsrGetDosDevicesSd
 *
 * The CsrGetDosDevicesSd creates a security descriptor for the DOS Devices
 * Object Directory.
 *
 * @param DosDevicesSd
 *        Pointer to the Security Descriptor to return.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks Depending on the DOS Devices Protection Mode (set in the registry),
 *          regular users may or may not have full access to the directory.
 *
 *--*/
NTSTATUS
NTAPI
CsrGetDosDevicesSd(OUT PSECURITY_DESCRIPTOR DosDevicesSd)
{
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY CreatorAuthority = {SECURITY_CREATOR_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID WorldSid, CreatorSid, AdminSid, SystemSid;
    UCHAR KeyValueBuffer[0x40];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
    UNICODE_STRING KeyName;
    ULONG ProtectionMode = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PACL Dacl;
    PACCESS_ALLOWED_ACE Ace;
    HANDLE hKey;
    NTSTATUS Status;
    ULONG ResultLength, SidLength;

    /* Create the SD */
    RtlCreateSecurityDescriptor(DosDevicesSd, SECURITY_DESCRIPTOR_REVISION);

    /* Initialize the System SID */
    RtlAllocateAndInitializeSid(&NtSidAuthority,
                                1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                &SystemSid);

    /* Initialize the World SID */
    RtlAllocateAndInitializeSid(&WorldAuthority,
                                1,
                                SECURITY_WORLD_RID,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                &WorldSid);

    /* Initialize the Admin SID */
    RtlAllocateAndInitializeSid(&NtSidAuthority,
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

    /* Initialize the Creator SID */
    RtlAllocateAndInitializeSid(&CreatorAuthority,
                                1,
                                SECURITY_CREATOR_OWNER_RID,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                &CreatorSid);

    /* Open the Session Manager Key */
    RtlInitUnicodeString(&KeyName, SM_REG_KEY);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    if (NT_SUCCESS(Status = NtOpenKey(&hKey,
                                      KEY_READ,
                                      &ObjectAttributes)))
    {
        /* Read the ProtectionMode. See http://support.microsoft.com/kb/q218473/ */
        RtlInitUnicodeString(&KeyName, L"ProtectionMode");
        Status = NtQueryValueKey(hKey,
                                 &KeyName,
                                 KeyValuePartialInformation,
                                 KeyValueBuffer,
                                 sizeof(KeyValueBuffer),
                                 &ResultLength);

        /* Make sure it's what we expect it to be */
        KeyValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
        if ((KeyValuePartialInfo->Type == REG_DWORD) && 
            (*(PULONG)KeyValuePartialInfo->Data != 0))
        {
            /* Save the Protection Mode */
            ProtectionMode = *(PULONG)KeyValuePartialInfo->Data;
        }

        /* Close the handle */
        NtClose(hKey);
    }

    /* Check the Protection Mode */
    if (ProtectionMode & 3)
    {
        /* Calculate SID Lengths */
        SidLength = RtlLengthSid(CreatorSid) + RtlLengthSid(SystemSid) +
                    RtlLengthSid(AdminSid);

        /* Allocate memory for the DACL */
        Dacl = RtlAllocateHeap(CsrHeap,
                               HEAP_ZERO_MEMORY,
                               sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) +
                               SidLength);

        /* Create it */
        Status = RtlCreateAcl(Dacl,
                              sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) +
                              SidLength,
                              ACL_REVISION2);

        /* Give full access to the System */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        SystemSid);

        /* Get the ACE back */
        Status = RtlGetAce(Dacl, 0, (PVOID*)&Ace);

        /* Add some flags to it for the Admin SID */
        Ace->Header.AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);

        /* Add the ACE to the Admin SID */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        AdminSid);

        /* Get the ACE back */
        Status = RtlGetAce(Dacl, 1, (PVOID*)&Ace);

        /* Add some flags to it for the Creator SID */
        Ace->Header.AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);

        /* Add the ACE to the Admin SID */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        CreatorSid);

        /* Get the ACE back */
        Status = RtlGetAce(Dacl, 2, (PVOID*)&Ace);

        /* Add some flags to it for the SD */
        Ace->Header.AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE |
                                INHERIT_ONLY_ACE);

        /* Set this DACL with the SD */
        Status = RtlSetDaclSecurityDescriptor(DosDevicesSd,
                                              TRUE,
                                              Dacl,
                                              FALSE);
    }
    else
    {
        /* Calculate SID Lengths */
        SidLength = RtlLengthSid(WorldSid) + RtlLengthSid(SystemSid);

        /* Allocate memory for the DACL */
        Dacl = RtlAllocateHeap(CsrHeap,
                               HEAP_ZERO_MEMORY,
                               sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) +
                               SidLength);

        /* Create it */
        Status = RtlCreateAcl(Dacl,
                              sizeof(ACL) + 3 * sizeof(ACCESS_ALLOWED_ACE) +
                              SidLength,
                              ACL_REVISION2);

        /* Give RWE access to the World */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_READ | GENERIC_WRITE |
                                        GENERIC_EXECUTE,
                                        WorldSid);

        /* Give full access to the System */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        SystemSid);

        /* Give full access to the World */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        WorldSid);

        /* Get the ACE back */
        Status = RtlGetAce(Dacl, 2, (PVOID*)&Ace);

        /* Add some flags to it for the SD */
        Ace->Header.AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE |
                                INHERIT_ONLY_ACE);

        /* Set this DACL with the SD */
        Status = RtlSetDaclSecurityDescriptor(DosDevicesSd,
                                              TRUE,
                                              Dacl,
                                              FALSE);
    }

/* FIXME: failure cases! Fail: */
    /* Free the memory */
    RtlFreeHeap(CsrHeap, 0, Dacl);

/* FIXME: semi-failure cases! Quickie: */
    /* Free the SIDs */
    RtlFreeSid(SystemSid);
    RtlFreeSid(WorldSid);
    RtlFreeSid(AdminSid);
    RtlFreeSid(CreatorSid);

    /* Return */
    return Status;
}

/*++
 * @name CsrFreeDosDevicesSd
 *
 * The CsrFreeDosDevicesSd frees the security descriptor that was created
 * by CsrGetDosDevicesSd
 *
 * @param DosDevicesSd
 *        Pointer to the security descriptor to free.

 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrFreeDosDevicesSd(IN PSECURITY_DESCRIPTOR DosDevicesSd)
{
    PACL Dacl;
    BOOLEAN Present, Default;
    NTSTATUS Status;

    /* Get the DACL corresponding to this SD */
    Status = RtlGetDaclSecurityDescriptor(DosDevicesSd,
                                          &Present,
                                          &Dacl,
                                          &Default);

    /* Free it */
    if (NT_SUCCESS(Status) && Dacl) RtlFreeHeap(CsrHeap, 0, Dacl);
}

/*++
 * @name CsrCreateSessionObjectDirectory
 *
 * The CsrCreateSessionObjectDirectory routine creates the BaseNamedObjects,
 * Session and Dos Devices directories for the specified session.
 *
 * @param Session
 *        Session ID for which to create the directories.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCreateSessionObjectDirectory(IN ULONG Session)
{
    WCHAR SessionBuffer[512];
    WCHAR BnoBuffer[512];
    UNICODE_STRING SessionString;
    UNICODE_STRING BnoString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE BnoHandle;
    SECURITY_DESCRIPTOR DosDevicesSd;
    NTSTATUS Status;

    /* Generate the Session BNOLINKS Directory */
    swprintf(SessionBuffer, L"%ws\\BNOLINKS", SESSION_ROOT);
    RtlInitUnicodeString(&SessionString, SessionBuffer);

    /* Initialize the attributes for the Directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_PERMANENT | OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create it */
    Status = NtCreateDirectoryObject(&BNOLinksDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: fatal: NtCreateDirectoryObject failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }

    /* Now add the Session ID */
    swprintf(SessionBuffer, L"%ld", Session);
    RtlInitUnicodeString(&SessionString, SessionBuffer);

    /* Check if this is the first Session */
    if (Session)
    {
        /* Not the first, so the name will be slighly more complex */
        swprintf(BnoBuffer, L"%ws\\%ld\\BaseNamedObjects", SESSION_ROOT, Session);
    }
    else
    {
        /* Use the direct name */
        RtlCopyMemory(BnoBuffer, L"\\BaseNamedObjects", 36);
    }

    /* Create the Unicode String for the BNO SymLink */
    RtlInitUnicodeString(&BnoString, BnoBuffer);

    /* Initialize the attributes for the SymLink */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_PERMANENT | OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               BNOLinksDirectory,
                               NULL);

    /* Create it */
    Status = NtCreateSymbolicLinkObject(&BnoHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &BnoString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: fatal: NtCreateSymbolicLinkObject failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }

    /* Create the \DosDevices Security Descriptor */
    CsrGetDosDevicesSd(&DosDevicesSd);

    /* Now create a directory for this session */
    swprintf(SessionBuffer, L"%ws\\%ld", SESSION_ROOT, Session);
    RtlInitUnicodeString(&SessionString, SessionBuffer);

    /* Initialize the attributes for the Directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_PERMANENT | OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               0,
                               &DosDevicesSd);

    /* Create it */
    Status = NtCreateDirectoryObject(&SessionObjectDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: fatal: NtCreateDirectoryObject failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
        /* Release the Security Descriptor */
        CsrFreeDosDevicesSd(&DosDevicesSd);
        return Status;
    }

    /* Next, create a directory for this session's DOS Devices */
    /* Now create a directory for this session */
    RtlInitUnicodeString(&SessionString, L"DosDevices");

    /* Initialize the attributes for the Directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SessionString,
                               OBJ_PERMANENT | OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               0,
                               &DosDevicesSd);

    /* Create it */
    Status = NtCreateDirectoryObject(&DosDevicesDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: fatal: NtCreateDirectoryObject failed (Status=0x%08lx)\n",
                __FUNCTION__, Status);
    }

    /* Release the Security Descriptor */
    CsrFreeDosDevicesSd(&DosDevicesSd);

    /* Return */
    return Status;
}

/*++
 * @name CsrSetProcessSecurity
 *
 * The CsrSetProcessSecurity routine protects access to the CSRSS process
 * from unauthorized tampering.
 *
 * @param None.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSetProcessSecurity(VOID)
{
    NTSTATUS Status;
    HANDLE hToken;
    ULONG ReturnLength;
    PTOKEN_USER TokenUserInformation;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl;

    /* Open our token */
    Status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_QUERY,
                                &hToken);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Token User Length */
    NtQueryInformationToken(hToken,
                            TokenUser,
                            NULL,
                            0,
                            &ReturnLength);

    /* Allocate space for it */
    TokenUserInformation = RtlAllocateHeap(CsrHeap,
                                           HEAP_ZERO_MEMORY,
                                           ReturnLength);

    /* Now query the data */
    Status = NtQueryInformationToken(hToken,
                                     TokenUser,
                                     TokenUserInformation,
                                     ReturnLength,
                                     &ReturnLength);

    /* Close the handle */
    NtClose(hToken);

    /* Make sure that we got the data */
    if (!NT_SUCCESS(Status))
    {
        /* FAil */
        RtlFreeHeap(CsrHeap, 0, TokenUserInformation);
        return Status;
    }

    /* Now check the SID Length */
    ReturnLength = RtlLengthSid(TokenUserInformation->User.Sid);

    /* Allocate a buffer for the Security Descriptor, with SID and DACL */
    SecurityDescriptor = RtlAllocateHeap(CsrHeap,
                                         HEAP_ZERO_MEMORY,
                                         SECURITY_DESCRIPTOR_MIN_LENGTH +
                                         sizeof(ACL) + ReturnLength +
                                         sizeof(ACCESS_ALLOWED_ACE));
    
    /* Set the pointer to the DACL */
    Dacl = (PACL)((ULONG_PTR)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);

    /* Now create the SD itself */
    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(CsrHeap, 0, SecurityDescriptor);
        RtlFreeHeap(CsrHeap, 0, TokenUserInformation);
        return Status;
    }

    /* Create the DACL for it*/
    RtlCreateAcl(Dacl,
                 sizeof(ACL) + ReturnLength + sizeof(ACCESS_ALLOWED_ACE),
                 ACL_REVISION2);

    /* Create the ACE */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    PROCESS_VM_READ | PROCESS_VM_WRITE |
                                    PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE |
                                    PROCESS_TERMINATE | PROCESS_SUSPEND_RESUME |
                                    PROCESS_QUERY_INFORMATION | READ_CONTROL,
                                    TokenUserInformation->User.Sid);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(CsrHeap, 0, SecurityDescriptor);
        RtlFreeHeap(CsrHeap, 0, TokenUserInformation);
        return Status;
    }

    /* Clear the DACL in the SD */
    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        RtlFreeHeap(CsrHeap, 0, SecurityDescriptor);
        RtlFreeHeap(CsrHeap, 0, TokenUserInformation);
        return Status;
    }

    /* Write the SD into the Process */
    Status = NtSetSecurityObject(NtCurrentProcess(),
                                 DACL_SECURITY_INFORMATION,
                                 SecurityDescriptor);

    /* Free the memory and return */
    RtlFreeHeap(CsrHeap, 0, SecurityDescriptor);
    RtlFreeHeap(CsrHeap, 0, TokenUserInformation);
    return Status;
}

/*++
 * @name CsrSetDirectorySecurity
 *
 * The CsrSetDirectorySecurity routine sets the security descriptor for the
 * specified Object Directory.
 *
 * @param ObjectDirectory
 *        Handle fo the Object Directory to protect.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrSetDirectorySecurity(IN HANDLE ObjectDirectory)
{
    /* FIXME: Implement */
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name CsrServerInitialization
 * @implemented NT4
 *
 * The CsrServerInitialization routine is the native (not Server) entrypoint
 * of this Server DLL. It serves as the entrypoint for csrss.
 *
 * @param ArgumentCount
 *        Number of arguments on the command line.
 *
 * @param Arguments
 *        Array of arguments from the command line.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrServerInitialization(ULONG ArgumentCount,
                        PCHAR Arguments[])
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i = 0;
    PVOID ProcessData;
    PCSR_SERVER_DLL ServerDll;

    DPRINT("CSRSRV: %s called\n", __FUNCTION__);

    /* Create the Init Event */
    Status = NtCreateEvent(&CsrInitializationEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    /* Cache System Basic Information so we don't always request it */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &CsrNtSysInfo,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);

    /* Save our Heap */
    CsrHeap = RtlGetProcessHeap();

    /* Set our Security Descriptor to protect the process */
    CsrSetProcessSecurity();

    /* Set up Session Support */
    Status = CsrInitializeNtSessions();
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: CsrInitializeSessions failed (Status=%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }

    /* Set up Process Support */
    Status = CsrInitializeProcesses();
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: CsrInitializeProcesses failed (Status=%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }

    /* Parse the command line */
    CsrpParseCommandLine(ArgumentCount, Arguments);

    /* All Server DLLs are now loaded, allocate a heap for the Root Process */
    ProcessData = RtlAllocateHeap(CsrHeap,
                                  HEAP_ZERO_MEMORY,
                                  CsrTotalPerProcessDataLength);

    /*
     * Our Root Process was never officially initalized, so write the data
     * for each Server DLL manually.
     */
    for(i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Get the current Server */
        ServerDll = CsrLoadedServerDll[i];

        /* Is it loaded, and does it have per process data? */
        if (ServerDll && ServerDll->SizeOfProcessData)
        {
            /* It does, give it part of our allocated heap */
            CsrRootProcess->ServerData[i] = ProcessData;

            /* Move to the next heap position */
            ProcessData = (PVOID)((ULONG_PTR)ProcessData +
                                  ServerDll->SizeOfProcessData);
        }
        else
        {
            /* Nothing for this Server DLL */
            CsrRootProcess->ServerData[i] = NULL;
        }
    }

    /* Now initialize the Root Process manually as well */
    for(i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Get the current Server */
        ServerDll = CsrLoadedServerDll[i];

        /* Is it loaded, and does it a callback for new processes? */
        if (ServerDll && ServerDll->NewProcessCallback)
        {
            /* Call the callback */
            (*ServerDll->NewProcessCallback)(NULL, CsrRootProcess);
        }
    }

    /* Now initialize our API Port */
    Status = CsrApiPortInitialize();
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: CsrApiPortInitialize failed (Status=%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }

    /* Initialize the API Port for SM communication */
    Status = CsrSbApiPortInitialize();
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: CsrSbApiPortInitialize failed (Status=%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }

    /* We're all set! Connect to SM! */
    Status = SmConnectToSm(&CsrSbApiPortName,
                           CsrSbApiPort,
                           IMAGE_SUBSYSTEM_WINDOWS_GUI,
                           &CsrSmApiPort);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("CSRSRV:%s: SmConnectToSm failed (Status=%08lx)\n",
                __FUNCTION__, Status);
        return Status;
    }

    /* Finito! Signal the event */
    NtSetEvent(CsrInitializationEvent, NULL);
    NtClose(CsrInitializationEvent);

    /* Have us handle Hard Errors */
    NtSetDefaultHardErrorPort(CsrApiPort);
    
    /* Return status */
    return Status;
}

/*++
 * @name CsrPopulateDosDevices
 * @implemented NT5.1
 *
 * The CsrPopulateDosDevices routine uses the DOS Device Map from the Kernel
 * to populate the Dos Devices Object Directory for the session.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrPopulateDosDevices(VOID)
{
    NTSTATUS Status;
    PROCESS_DEVICEMAP_INFORMATION OldDeviceMap;
    PROCESS_DEVICEMAP_INFORMATION NewDeviceMap;

    /* Query the Device Map */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &OldDeviceMap.Query,
                                       sizeof(PROCESS_DEVICEMAP_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status)) return;

    /* Set the new one */
    NewDeviceMap.Set.DirectoryHandle = DosDevicesDirectory;
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessDeviceMap,
                                     &NewDeviceMap,
                                     sizeof(ULONG));
    if (!NT_SUCCESS(Status)) return;

    /* Populate the Directory */
    CsrPopulateDosDevicesDirectory(DosDevicesDirectory, &OldDeviceMap);
}

BOOL
NTAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    /* We don't do much */
    return TRUE;
}

/* EOF */
