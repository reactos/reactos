/*
 *  SAM Server DLL
 *  Copyright (C) 2005 Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "samsrv.h"

#include <samsrv/samsrv.h>

/* GLOBALS *******************************************************************/

ENCRYPTED_NT_OWF_PASSWORD EmptyNtHash;
ENCRYPTED_LM_OWF_PASSWORD EmptyLmHash;
RTL_RESOURCE SampResource;


/* FUNCTIONS *****************************************************************/

static
NTSTATUS
SampInitHashes(VOID)
{
    UNICODE_STRING EmptyNtPassword = {0, 0, NULL};
    CHAR EmptyLmPassword[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
    NTSTATUS Status;

    /* Calculate the NT hash value of the empty password */
    Status = SystemFunction007(&EmptyNtPassword,
                               (LPBYTE)&EmptyNtHash);
    if (!NT_SUCCESS(Status))
    {
        ERR("Calculation of the empty NT hash failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Calculate the LM hash value of the empty password */
    Status = SystemFunction006(EmptyLmPassword,
                               (LPSTR)&EmptyLmHash);
    if (!NT_SUCCESS(Status))
    {
        ERR("Calculation of the empty LM hash failed (Status 0x%08lx)\n", Status);
    }

    return Status;
}


NTSTATUS
NTAPI
SamIConnect(IN PSAMPR_SERVER_NAME ServerName,
            OUT SAMPR_HANDLE *ServerHandle,
            IN ACCESS_MASK DesiredAccess,
            IN BOOLEAN Trusted)
{
    PSAM_DB_OBJECT ServerObject;
    NTSTATUS Status;

    TRACE("SamIConnect(%p %p %lx %ld)\n",
          ServerName, ServerHandle, DesiredAccess, Trusted);

    /* Map generic access rights */
    RtlMapGenericMask(&DesiredAccess,
                      pServerMapping);

    /* Open the Server Object */
    Status = SampOpenDbObject(NULL,
                              NULL,
                              L"SAM",
                              0,
                              SamDbServerObject,
                              DesiredAccess,
                              &ServerObject);
    if (NT_SUCCESS(Status))
    {
        ServerObject->Trusted = Trusted;
        *ServerHandle = (SAMPR_HANDLE)ServerObject;
    }

    TRACE("SamIConnect done (Status 0x%08lx)\n", Status);

    return Status;
}


NTSTATUS
NTAPI
SamIInitialize(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("SamIInitialize() called\n");

    Status = SampInitHashes();
    if (!NT_SUCCESS(Status))
        return Status;

    if (SampIsSetupRunning())
    {
        Status = SampInitializeRegistry();
        if (!NT_SUCCESS(Status))
            return Status;
    }

    RtlInitializeResource(&SampResource);

    /* Initialize the SAM database */
    Status = SampInitDatabase();
    if (!NT_SUCCESS(Status))
        return Status;

    /* Start the RPC server */
    SampStartRpcServer();

    return Status;
}


NTSTATUS
NTAPI
SampInitializeRegistry(VOID)
{
    TRACE("SampInitializeRegistry() called\n");

    SampInitializeSAM();

    return STATUS_SUCCESS;
}


VOID
NTAPI
SamIFreeVoid(PVOID Ptr)
{
    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_ALIAS_INFO_BUFFER(
    PSAMPR_ALIAS_INFO_BUFFER Ptr,
    ALIAS_INFORMATION_CLASS InformationClass)
{
    if (Ptr == NULL)
        return;

    switch (InformationClass)
    {
        case AliasGeneralInformation:
            if (Ptr->General.Name.Buffer != NULL)
                MIDL_user_free(Ptr->General.Name.Buffer);

            if (Ptr->General.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->General.AdminComment.Buffer);
            break;

        case AliasNameInformation:
            if (Ptr->Name.Name.Buffer != NULL)
                MIDL_user_free(Ptr->Name.Name.Buffer);
            break;

        case AliasAdminCommentInformation:
            if (Ptr->AdminComment.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->AdminComment.AdminComment.Buffer);
            break;

        default:
            FIXME("Unsupported information class: %lu\n", InformationClass);
            break;
    }

    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_DISPLAY_INFO_BUFFER(
    PSAMPR_DISPLAY_INFO_BUFFER Ptr,
    DOMAIN_DISPLAY_INFORMATION InformationClass)
{
    ULONG i;

    if (Ptr == NULL)
        return;

    switch (InformationClass)
    {
        case DomainDisplayUser:
            if (Ptr->UserInformation.Buffer != NULL)
            {
                for (i = 0; i < Ptr->UserInformation.EntriesRead; i++)
                {
                    if (Ptr->UserInformation.Buffer[i].AccountName.Buffer != NULL)
                        MIDL_user_free(Ptr->UserInformation.Buffer[i].AccountName.Buffer);

                    if (Ptr->UserInformation.Buffer[i].AdminComment.Buffer != NULL)
                        MIDL_user_free(Ptr->UserInformation.Buffer[i].AdminComment.Buffer);

                    if (Ptr->UserInformation.Buffer[i].FullName.Buffer != NULL)
                        MIDL_user_free(Ptr->UserInformation.Buffer[i].FullName.Buffer);
                }

                MIDL_user_free(Ptr->UserInformation.Buffer);
            }
            break;

        case DomainDisplayMachine:
            if (Ptr->MachineInformation.Buffer != NULL)
            {
                for (i = 0; i < Ptr->MachineInformation.EntriesRead; i++)
                {
                    if (Ptr->MachineInformation.Buffer[i].AccountName.Buffer != NULL)
                        MIDL_user_free(Ptr->MachineInformation.Buffer[i].AccountName.Buffer);

                    if (Ptr->MachineInformation.Buffer[i].AdminComment.Buffer != NULL)
                        MIDL_user_free(Ptr->MachineInformation.Buffer[i].AdminComment.Buffer);
                }

                MIDL_user_free(Ptr->MachineInformation.Buffer);
            }
            break;

        case DomainDisplayGroup:
            if (Ptr->GroupInformation.Buffer != NULL)
            {
                for (i = 0; i < Ptr->GroupInformation.EntriesRead; i++)
                {
                    if (Ptr->GroupInformation.Buffer[i].AccountName.Buffer != NULL)
                        MIDL_user_free(Ptr->GroupInformation.Buffer[i].AccountName.Buffer);

                    if (Ptr->GroupInformation.Buffer[i].AdminComment.Buffer != NULL)
                        MIDL_user_free(Ptr->GroupInformation.Buffer[i].AdminComment.Buffer);
                }

                MIDL_user_free(Ptr->GroupInformation.Buffer);
            }
            break;

        case DomainDisplayOemUser:
            if (Ptr->OemUserInformation.Buffer != NULL)
            {
                for (i = 0; i < Ptr->OemUserInformation.EntriesRead; i++)
                {
                    if (Ptr->OemUserInformation.Buffer[i].OemAccountName.Buffer != NULL)
                        MIDL_user_free(Ptr->OemUserInformation.Buffer[i].OemAccountName.Buffer);
                }

                MIDL_user_free(Ptr->OemUserInformation.Buffer);
            }
            break;

        case DomainDisplayOemGroup:
            if (Ptr->OemGroupInformation.Buffer != NULL)
            {
                for (i = 0; i < Ptr->OemGroupInformation.EntriesRead; i++)
                {
                    if (Ptr->OemGroupInformation.Buffer[i].OemAccountName.Buffer != NULL)
                        MIDL_user_free(Ptr->OemGroupInformation.Buffer[i].OemAccountName.Buffer);
                }

                MIDL_user_free(Ptr->OemGroupInformation.Buffer);
            }
            break;

        default:
            FIXME("Unsupported information class: %lu\n", InformationClass);
            break;
    }
}


VOID
NTAPI
SamIFree_SAMPR_DOMAIN_INFO_BUFFER(
    PSAMPR_DOMAIN_INFO_BUFFER Ptr,
    DOMAIN_INFORMATION_CLASS InformationClass)
{
    if (Ptr == NULL)
        return;

    switch (InformationClass)
    {
        case DomainPasswordInformation:
            break;

        case DomainGeneralInformation:
            if (Ptr->General.OemInformation.Buffer != NULL)
                MIDL_user_free(Ptr->General.OemInformation.Buffer);

            if (Ptr->General.DomainName.Buffer != NULL)
                MIDL_user_free(Ptr->General.DomainName.Buffer);

            if (Ptr->General.ReplicaSourceNodeName.Buffer != NULL)
                MIDL_user_free(Ptr->General.ReplicaSourceNodeName.Buffer);
            break;

        case DomainLogoffInformation:
            break;

        case DomainOemInformation:
            if (Ptr->Oem.OemInformation.Buffer != NULL)
                MIDL_user_free(Ptr->Oem.OemInformation.Buffer);
            break;

        case DomainNameInformation:
            if (Ptr->Name.DomainName.Buffer != NULL)
                MIDL_user_free(Ptr->Name.DomainName.Buffer);
            break;

        case DomainReplicationInformation:
            if (Ptr->Replication.ReplicaSourceNodeName.Buffer != NULL)
                MIDL_user_free(Ptr->Replication.ReplicaSourceNodeName.Buffer);
            break;

        case DomainServerRoleInformation:
            break;

        case DomainModifiedInformation:
            break;

        case DomainStateInformation:
            break;

        case DomainGeneralInformation2:
            if (Ptr->General2.I1.OemInformation.Buffer != NULL)
                MIDL_user_free(Ptr->General2.I1.OemInformation.Buffer);

            if (Ptr->General2.I1.DomainName.Buffer != NULL)
                MIDL_user_free(Ptr->General2.I1.DomainName.Buffer);

            if (Ptr->General2.I1.ReplicaSourceNodeName.Buffer != NULL)
                MIDL_user_free(Ptr->General2.I1.ReplicaSourceNodeName.Buffer);
            break;

        case DomainLockoutInformation:
            break;

        case DomainModifiedInformation2:
            break;

        default:
            FIXME("Unsupported information class: %lu\n", InformationClass);
            break;
    }

    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_ENUMERATION_BUFFER(PSAMPR_ENUMERATION_BUFFER Ptr)
{
    ULONG i;

    if (Ptr == NULL)
        return;

    if (Ptr->Buffer != NULL)
    {
        for (i = 0; i < Ptr->EntriesRead; i++)
        {
            if (Ptr->Buffer[i].Name.Buffer != NULL)
                MIDL_user_free(Ptr->Buffer[i].Name.Buffer);
        }

        MIDL_user_free(Ptr->Buffer);
    }

    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_GET_GROUPS_BUFFER(PSAMPR_GET_GROUPS_BUFFER Ptr)
{
    if (Ptr == NULL)
        return;

    if (Ptr->Groups != NULL)
        MIDL_user_free(Ptr->Groups);

    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_GET_MEMBERS_BUFFER(PSAMPR_GET_MEMBERS_BUFFER Ptr)
{
    if (Ptr == NULL)
        return;

    if (Ptr->Members != NULL)
        MIDL_user_free(Ptr->Members);

    if (Ptr->Attributes != NULL)
        MIDL_user_free(Ptr->Attributes);

    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_GROUP_INFO_BUFFER(
    PSAMPR_GROUP_INFO_BUFFER Ptr,
    GROUP_INFORMATION_CLASS InformationClass)
{
    if (Ptr == NULL)
        return;

    switch (InformationClass)
    {
        case GroupGeneralInformation:
            if (Ptr->General.Name.Buffer != NULL)
                MIDL_user_free(Ptr->General.Name.Buffer);

            if (Ptr->General.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->General.AdminComment.Buffer);
            break;

        case GroupNameInformation:
            if (Ptr->Name.Name.Buffer != NULL)
                MIDL_user_free(Ptr->Name.Name.Buffer);
            break;

        case GroupAttributeInformation:
            break;

        case GroupAdminCommentInformation:
            if (Ptr->AdminComment.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->AdminComment.AdminComment.Buffer);
            break;

        default:
            FIXME("Unsupported information class: %lu\n", InformationClass);
            break;
    }

    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_PSID_ARRAY(PSAMPR_PSID_ARRAY Ptr)
{
    if (Ptr == NULL)
        return;

    if (Ptr->Sids != NULL)
    {
        MIDL_user_free(Ptr->Sids);
    }
}


VOID
NTAPI
SamIFree_SAMPR_RETURNED_USTRING_ARRAY(PSAMPR_RETURNED_USTRING_ARRAY Ptr)
{
    ULONG i;

    if (Ptr == NULL)
        return;

    if (Ptr->Element != NULL)
    {
        for (i = 0; i < Ptr->Count; i++)
        {
            if (Ptr->Element[i].Buffer != NULL)
                MIDL_user_free(Ptr->Element[i].Buffer);
        }

        MIDL_user_free(Ptr->Element);
        Ptr->Element = NULL;
        Ptr->Count = 0;
    }
}


VOID
NTAPI
SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR(PSAMPR_SR_SECURITY_DESCRIPTOR Ptr)
{
    if (Ptr == NULL)
        return;

    if (Ptr->SecurityDescriptor != NULL)
        MIDL_user_free(Ptr->SecurityDescriptor);

    MIDL_user_free(Ptr);
}


VOID
NTAPI
SamIFree_SAMPR_ULONG_ARRAY(PSAMPR_ULONG_ARRAY Ptr)
{
    if (Ptr == NULL)
        return;

    if (Ptr->Element != NULL)
    {
        MIDL_user_free(Ptr->Element);
        Ptr->Element = NULL;
        Ptr->Count = 0;
    }
}


VOID
NTAPI
SamIFree_SAMPR_USER_INFO_BUFFER(PSAMPR_USER_INFO_BUFFER Ptr,
                                USER_INFORMATION_CLASS InformationClass)
{
    if (Ptr == NULL)
        return;

    switch (InformationClass)
    {
        case UserGeneralInformation:
            if (Ptr->General.UserName.Buffer != NULL)
                MIDL_user_free(Ptr->General.UserName.Buffer);

            if (Ptr->General.FullName.Buffer != NULL)
                MIDL_user_free(Ptr->General.FullName.Buffer);

            if (Ptr->General.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->General.AdminComment.Buffer);

            if (Ptr->General.UserComment.Buffer != NULL)
                MIDL_user_free(Ptr->General.UserComment.Buffer);
            break;

        case UserPreferencesInformation:
            if (Ptr->Preferences.UserComment.Buffer != NULL)
                MIDL_user_free(Ptr->Preferences.UserComment.Buffer);

            if (Ptr->Preferences.Reserved1.Buffer != NULL)
                MIDL_user_free(Ptr->Preferences.Reserved1.Buffer);
            break;

        case UserLogonInformation:
            if (Ptr->Logon.UserName.Buffer != NULL)
                MIDL_user_free(Ptr->Logon.UserName.Buffer);

            if (Ptr->Logon.FullName.Buffer != NULL)
                MIDL_user_free(Ptr->Logon.FullName.Buffer);

            if (Ptr->Logon.HomeDirectory.Buffer != NULL)
                MIDL_user_free(Ptr->Logon.HomeDirectory.Buffer);

            if (Ptr->Logon.HomeDirectoryDrive.Buffer != NULL)
                MIDL_user_free(Ptr->Logon.HomeDirectoryDrive.Buffer);

            if (Ptr->Logon.ScriptPath.Buffer != NULL)
                MIDL_user_free(Ptr->Logon.ScriptPath.Buffer);

            if (Ptr->Logon.ProfilePath.Buffer != NULL)
                MIDL_user_free(Ptr->Logon.ProfilePath.Buffer);

            if (Ptr->Logon.WorkStations.Buffer != NULL)
                MIDL_user_free(Ptr->Logon.WorkStations.Buffer);

            if (Ptr->Logon.LogonHours.LogonHours != NULL)
                MIDL_user_free(Ptr->Logon.LogonHours.LogonHours);
            break;

        case UserLogonHoursInformation:
            if (Ptr->LogonHours.LogonHours.LogonHours != NULL)
                MIDL_user_free(Ptr->LogonHours.LogonHours.LogonHours);
            break;

        case UserAccountInformation:
            if (Ptr->Account.UserName.Buffer != NULL)
                MIDL_user_free(Ptr->Account.UserName.Buffer);

            if (Ptr->Account.FullName.Buffer != NULL)
                MIDL_user_free(Ptr->Account.FullName.Buffer);

            if (Ptr->Account.HomeDirectory.Buffer != NULL)
                MIDL_user_free(Ptr->Account.HomeDirectory.Buffer);

            if (Ptr->Account.HomeDirectoryDrive.Buffer != NULL)
                MIDL_user_free(Ptr->Account.HomeDirectoryDrive.Buffer);

            if (Ptr->Account.ScriptPath.Buffer != NULL)
                MIDL_user_free(Ptr->Account.ScriptPath.Buffer);

            if (Ptr->Account.ProfilePath.Buffer != NULL)
                MIDL_user_free(Ptr->Account.ProfilePath.Buffer);

            if (Ptr->Account.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->Account.AdminComment.Buffer);

            if (Ptr->Account.WorkStations.Buffer != NULL)
                MIDL_user_free(Ptr->Account.WorkStations.Buffer);

            if (Ptr->Account.LogonHours.LogonHours != NULL)
                MIDL_user_free(Ptr->Account.LogonHours.LogonHours);
            break;

        case UserNameInformation:
            if (Ptr->Name.UserName.Buffer != NULL)
                MIDL_user_free(Ptr->Name.UserName.Buffer);

            if (Ptr->Name.FullName.Buffer != NULL)
                MIDL_user_free(Ptr->Name.FullName.Buffer);
            break;

        case UserAccountNameInformation:
            if (Ptr->AccountName.UserName.Buffer != NULL)
                MIDL_user_free(Ptr->AccountName.UserName.Buffer);
            break;

        case UserFullNameInformation:
            if (Ptr->FullName.FullName.Buffer != NULL)
                MIDL_user_free(Ptr->FullName.FullName.Buffer);
            break;

        case UserPrimaryGroupInformation:
            break;

        case UserHomeInformation:
            if (Ptr->Home.HomeDirectory.Buffer != NULL)
                MIDL_user_free(Ptr->Home.HomeDirectory.Buffer);

            if (Ptr->Home.HomeDirectoryDrive.Buffer != NULL)
                MIDL_user_free(Ptr->Home.HomeDirectoryDrive.Buffer);
            break;

        case UserScriptInformation:
            if (Ptr->Script.ScriptPath.Buffer != NULL)
                MIDL_user_free(Ptr->Script.ScriptPath.Buffer);

        case UserProfileInformation:
            if (Ptr->Profile.ProfilePath.Buffer != NULL)
                MIDL_user_free(Ptr->Profile.ProfilePath.Buffer);

        case UserAdminCommentInformation:
            if (Ptr->AdminComment.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->AdminComment.AdminComment.Buffer);
            break;

        case UserWorkStationsInformation:
            if (Ptr->WorkStations.WorkStations.Buffer != NULL)
                MIDL_user_free(Ptr->WorkStations.WorkStations.Buffer);
            break;

        case UserSetPasswordInformation:
            ERR("Information class UserSetPasswordInformation cannot be queried!\n");
            break;

        case UserControlInformation:
            break;

        case UserExpiresInformation:
            break;

        case UserInternal1Information:
            break;

        case UserInternal2Information:
            break;

        case UserParametersInformation:
            if (Ptr->Parameters.Parameters.Buffer != NULL)
                MIDL_user_free(Ptr->Parameters.Parameters.Buffer);
            break;

        case UserAllInformation:
            if (Ptr->All.UserName.Buffer != NULL)
                MIDL_user_free(Ptr->All.UserName.Buffer);

            if (Ptr->All.FullName.Buffer != NULL)
                MIDL_user_free(Ptr->All.FullName.Buffer);

            if (Ptr->All.HomeDirectory.Buffer != NULL)
                MIDL_user_free(Ptr->All.HomeDirectory.Buffer);

            if (Ptr->All.HomeDirectoryDrive.Buffer != NULL)
                MIDL_user_free(Ptr->All.HomeDirectoryDrive.Buffer);

            if (Ptr->All.ScriptPath.Buffer != NULL)
                MIDL_user_free(Ptr->All.ScriptPath.Buffer);

            if (Ptr->All.ProfilePath.Buffer != NULL)
                MIDL_user_free(Ptr->All.ProfilePath.Buffer);

            if (Ptr->All.AdminComment.Buffer != NULL)
                MIDL_user_free(Ptr->All.AdminComment.Buffer);

            if (Ptr->All.WorkStations.Buffer != NULL)
                MIDL_user_free(Ptr->All.WorkStations.Buffer);

            if (Ptr->All.UserComment.Buffer != NULL)
                MIDL_user_free(Ptr->All.UserComment.Buffer);

            if (Ptr->All.Parameters.Buffer != NULL)
                MIDL_user_free(Ptr->All.Parameters.Buffer);

            if (Ptr->All.LmOwfPassword.Buffer != NULL)
                MIDL_user_free(Ptr->All.LmOwfPassword.Buffer);

            if (Ptr->All.NtOwfPassword.Buffer != NULL)
                MIDL_user_free(Ptr->All.NtOwfPassword.Buffer);

            if (Ptr->All.PrivateData.Buffer != NULL)
                MIDL_user_free(Ptr->All.PrivateData.Buffer);

            if (Ptr->All.SecurityDescriptor.SecurityDescriptor != NULL)
                MIDL_user_free(Ptr->All.SecurityDescriptor.SecurityDescriptor);

            if (Ptr->All.LogonHours.LogonHours != NULL)
                MIDL_user_free(Ptr->All.LogonHours.LogonHours);
            break;

        default:
            FIXME("Unsupported information class: %lu\n", InformationClass);
            break;
    }

    MIDL_user_free(Ptr);
}

/* EOF */
