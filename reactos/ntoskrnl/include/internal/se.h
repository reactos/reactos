/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_SE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_SE_H

#ifndef AS_INVOKED

extern POBJECT_TYPE SepTokenObjectType;

/* SID Authorities */
extern SID_IDENTIFIER_AUTHORITY SeNullSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeWorldSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeLocalSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeCreatorSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeNtSidAuthority;

/* SIDs */
extern PSID SeNullSid;
extern PSID SeWorldSid;
extern PSID SeLocalSid;
extern PSID SeCreatorOwnerSid;
extern PSID SeCreatorGroupSid;
extern PSID SeCreatorOwnerServerSid;
extern PSID SeCreatorGroupServerSid;
extern PSID SeNtAuthoritySid;
extern PSID SeDialupSid;
extern PSID SeNetworkSid;
extern PSID SeBatchSid;
extern PSID SeInteractiveSid;
extern PSID SeServiceSid;
extern PSID SeAnonymousLogonSid;
extern PSID SePrincipalSelfSid;
extern PSID SeLocalSystemSid;
extern PSID SeAuthenticatedUserSid;
extern PSID SeRestrictedCodeSid;
extern PSID SeAliasAdminsSid;
extern PSID SeAliasUsersSid;
extern PSID SeAliasGuestsSid;
extern PSID SeAliasPowerUsersSid;
extern PSID SeAliasAccountOpsSid;
extern PSID SeAliasSystemOpsSid;
extern PSID SeAliasPrintOpsSid;
extern PSID SeAliasBackupOpsSid;

/* Privileges */
extern LUID SeCreateTokenPrivilege;
extern LUID SeAssignPrimaryTokenPrivilege;
extern LUID SeLockMemoryPrivilege;
extern LUID SeIncreaseQuotaPrivilege;
extern LUID SeUnsolicitedInputPrivilege;
extern LUID SeTcbPrivilege;
extern LUID SeSecurityPrivilege;
extern LUID SeTakeOwnershipPrivilege;
extern LUID SeLoadDriverPrivilege;
extern LUID SeCreatePagefilePrivilege;
extern LUID SeIncreaseBasePriorityPrivilege;
extern LUID SeSystemProfilePrivilege;
extern LUID SeSystemtimePrivilege;
extern LUID SeProfileSingleProcessPrivilege;
extern LUID SeCreatePermanentPrivilege;
extern LUID SeBackupPrivilege;
extern LUID SeRestorePrivilege;
extern LUID SeShutdownPrivilege;
extern LUID SeDebugPrivilege;
extern LUID SeAuditPrivilege;
extern LUID SeSystemEnvironmentPrivilege;
extern LUID SeChangeNotifyPrivilege;
extern LUID SeRemoteShutdownPrivilege;

/* DACLs */
extern PACL SePublicDefaultUnrestrictedDacl;
extern PACL SePublicOpenDacl;
extern PACL SePublicOpenUnrestrictedDacl;
extern PACL SeUnrestrictedDacl;


/* Functions */

BOOLEAN SeInit1(VOID);
BOOLEAN SeInit2(VOID);

VOID SepInitLuid(VOID);
VOID SepInitPrivileges(VOID);
BOOLEAN SepInitSecurityIDs(VOID);
BOOLEAN SepInitDACLs(VOID);
BOOLEAN SepInitSDs(VOID);

VOID SepInitializeTokenImplementation(VOID);

NTSTATUS SepCreateSystemProcessToken(struct _EPROCESS* Process);
NTSTATUS SepInitializeNewProcess(struct _EPROCESS* NewProcess,
								 struct _EPROCESS* ParentProcess);

NTSTATUS SeExchangePrimaryToken(struct _EPROCESS* Process,
				PIACCESS_TOKEN NewToken,
				PIACCESS_TOKEN* OldTokenP);

NTSTATUS SeCaptureLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Src,
					 ULONG PrivilegeCount,
					 KPROCESSOR_MODE PreviousMode,
					 PLUID_AND_ATTRIBUTES AllocatedMem,
					 ULONG AllocatedLength,
					 POOL_TYPE PoolType,
					 ULONG d,
					 PLUID_AND_ATTRIBUTES* Dest,
					 PULONG Length);

NTSTATUS STDCALL
RtlCopySidAndAttributesArray(ULONG Count,
			     PSID_AND_ATTRIBUTES_ARRAY Src,
			     ULONG SidAreaSize,
			     PSID_AND_ATTRIBUTES_ARRAY Dest,
			     PVOID SidArea,
			     PVOID* RemainingSidArea,
			     PULONG RemainingSidAreaSize);

#endif /* !AS_INVOKED */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_SE_H */

/* EOF */
