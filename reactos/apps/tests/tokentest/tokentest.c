#define UNICODE
#define _UNICODE

#define ANONYMOUSUNIONS
#include <windows.h>

#define INCLUDE_THE_DDK_HEADERS
#ifdef INCLUDE_THE_DDK_HEADERS
#include <ddk/ntddk.h>
#define ROS_ACE_HEADER ACE_HEADER
#define ROS_ACE ACE

//
// Allocate the System Luid.  The first 1000 LUIDs are reserved.
// Use #999 here (0x3E7 = 999)
//

#define SYSTEM_LUID                     { 0x3E7, 0x0 }
#define ANONYMOUS_LOGON_LUID            { 0x3e6, 0x0 }
#define LOCALSERVICE_LUID               { 0x3e5, 0x0 }
#define NETWORKSERVICE_LUID             { 0x3e4, 0x0 }

#else
typedef LONG NTSTATUS, *PNTSTATUS;

typedef struct _UNICODE_STRING
{
   USHORT Length;
   USHORT MaximumLength;
   PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

typedef struct _ROS_ACE_HEADER
{
  CHAR AceType;
  CHAR AceFlags;
  USHORT AceSize;
  ACCESS_MASK AccessMask;
} ROS_ACE_HEADER, *PROS_ACE_HEADER;

typedef struct
{
  ACE_HEADER Header;
} ROS_ACE, *PROS_ACE;

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString (
	IN OUT	PUNICODE_STRING	String,
	IN	PSID		Sid,
	IN	BOOLEAN		AllocateString
	);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
	PACL Acl,
	ULONG AclSize,
	ULONG AclRevision);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce (
	PACL		Acl,
	ULONG		Revision,
	ACCESS_MASK	AccessMask,
	PSID		Sid
	);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce (
	PACL Acl,
	ULONG AceIndex,
	PROS_ACE *Ace
	);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
	OUT PLUID Luid
	);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateToken(
	OUT PHANDLE TokenHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TOKEN_TYPE TokenType,
	IN PLUID AuthenticationId,
	IN PLARGE_INTEGER ExpirationTime,
	IN PTOKEN_USER TokenUser,
	IN PTOKEN_GROUPS TokenGroups,
	IN PTOKEN_PRIVILEGES TokenPrivileges,
	IN PTOKEN_OWNER TokenOwner,
	IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
	IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
	IN PTOKEN_SOURCE TokenSource
	);
#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#endif
#include <stdio.h>

#define INITIAL_PRIV_ENABLED SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED
#define INITIAL_PRIV_DISABLED 0
LUID_AND_ATTRIBUTES InitialPrivilegeSet[] = 
{
	{ { 0x00000007, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeTcbPrivilege
	{ { 0x00000002, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeCreateTokenPrivilege
	{ { 0x00000009, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeTakeOwnershipPrivilege
	{ { 0x0000000f, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeCreatePagefilePrivilege
	{ { 0x00000004, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeLockMemoryPrivilege
	{ { 0x00000003, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeAssignPrimaryTokenPrivilege
	{ { 0x00000005, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeIncreaseQuotaPrivilege
	{ { 0x0000000e, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeIncreaseBasePriorityPrivilege
	{ { 0x00000010, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeCreatePermanentPrivilege
	{ { 0x00000014, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeDebugPrivilege
	{ { 0x00000015, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeAuditPrivilege
	{ { 0x00000008, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeSecurityPrivilege
	{ { 0x00000016, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeSystemEnvironmentPrivilege
	{ { 0x00000017, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeChangeNotifyPrivilege
	{ { 0x00000011, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeBackupPrivilege
	{ { 0x00000012, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeRestorePrivilege
	{ { 0x00000013, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeShutdownPrivilege
	{ { 0x0000000a, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeLoadDriverPrivilege
	{ { 0x0000000d, 0x00000000 }, INITIAL_PRIV_ENABLED  },	// SeProfileSingleProcessPrivilege
	{ { 0x0000000c, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeSystemtimePrivilege
	{ { 0x00000019, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeUndockPrivilege
	{ { 0x0000001c, 0x00000000 }, INITIAL_PRIV_DISABLED },	// SeManageVolumePrivilege
};

typedef struct _SID_2
{
	UCHAR  Revision;
	UCHAR  SubAuthorityCount;
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
	ULONG SubAuthority[2];
} SID_2;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
PrintSid(SID_AND_ATTRIBUTES* pSid, TOKEN_OWNER* pOwner, TOKEN_PRIMARY_GROUP* pPrimary)
{
	UNICODE_STRING scSid;

	RtlConvertSidToUnicodeString(&scSid, pSid->Sid, TRUE);
	printf("%wZ [", &scSid);
	LocalFree(scSid.Buffer);

	if ( EqualSid(pSid->Sid, pOwner->Owner) )
		printf("owner,");

	if ( EqualSid(pSid->Sid, pPrimary->PrimaryGroup) )
		printf("primary,");

	if ( pSid->Attributes & SE_GROUP_ENABLED )
	{
		if ( pSid->Attributes & SE_GROUP_ENABLED_BY_DEFAULT )
			printf("enabled-default,");
		else
			printf("enabled,");
	}

	if ( pSid->Attributes & SE_GROUP_LOGON_ID )
		printf("logon,");


	if ( pSid->Attributes & SE_GROUP_MANDATORY )
		printf("mandatory,");

	printf("]\n");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
DisplayTokenSids(TOKEN_USER* pUser,
				 TOKEN_GROUPS* pGroups,
				 TOKEN_OWNER* pOwner,
				 TOKEN_PRIMARY_GROUP* pPrimary)
{
	DWORD i;

	printf("\nSids:\n");
	PrintSid(&pUser->User, pOwner, pPrimary);
	printf("\nGroups:\n");
	for (i = 0; i < pGroups->GroupCount; i++)
		PrintSid(&pGroups->Groups[i], pOwner, pPrimary);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
DisplayTokenPrivileges(TOKEN_PRIVILEGES* pPriv)
{
	WCHAR buffer[256];
	DWORD i;

	printf("\nprivileges:\n");
	for (i = 0; i < pPriv->PrivilegeCount; i++)
	{
		DWORD cbName = sizeof(buffer) / sizeof(buffer[0]);
		LookupPrivilegeName(0, &pPriv->Privileges[i].Luid, buffer, &cbName);

		printf("%S{0x%08x, 0x%08x} [", buffer, pPriv->Privileges[i].Luid.HighPart, pPriv->Privileges[i].Luid.LowPart);

		if ( pPriv->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED )
			printf("enabled,");
		if ( pPriv->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT )
			printf("default,");
		if ( pPriv->Privileges[i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS )
			printf("used");

		printf("]\n");
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
DisplayDacl(PACL pAcl)
{
	DWORD i;
	NTSTATUS status;

	if ( ! pAcl )
	{
		printf("\nNo Default Dacl.\n");
		return;
	}

	printf("\nDacl:\n");
	for (i = 0; i < pAcl->AceCount; i++)
	{
		UNICODE_STRING scSid;
		ROS_ACE_HEADER* pAce;
		LPWSTR wszType = 0;
		PSID pSid;

		status = RtlGetAce(pAcl, i, (ROS_ACE**) &pAce);
		if ( ! NT_SUCCESS(status) )
		{
			printf("RtlGetAce(): status = 0x%08x\n", status);
			break;
		}

		pSid = (PSID) (pAce + 1);
		if ( pAce->AceType == ACCESS_ALLOWED_ACE_TYPE )
			wszType = L"allow";
		if ( pAce->AceType == ACCESS_DENIED_ACE_TYPE )
			wszType = L"deny ";

		status = RtlConvertSidToUnicodeString(&scSid, pSid, TRUE);
		if ( ! NT_SUCCESS(status) )
		{
			printf("RtlConvertSidToUnicodeString(): status = 0x%08x\n", status);
			break;
		}

		printf("%d.) %S %wZ 0x%08x\n", i, wszType, &scSid, pAce->AccessMask);
		LocalFree(scSid.Buffer);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PVOID
GetFromToken(HANDLE hToken, TOKEN_INFORMATION_CLASS tic)
{
	BOOL bResult;
    DWORD n;
	PBYTE p = 0;

    bResult = GetTokenInformation(hToken, tic, 0, 0, &n);
    if ( ! bResult && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return 0;

    p = (PBYTE) malloc(n);
    if ( ! GetTokenInformation(hToken, tic, p, n, &n) )
	{
		printf("GetFromToken() failed for TOKEN_INFORMATION_CLASS(%d): %d\n", tic, GetLastError());
		free(p);
		return 0;
	}
	
	return p;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
DisplayToken(HANDLE hTokenSource)
{
	TOKEN_USER*			 pTokenUser         = (PTOKEN_USER)			 GetFromToken(hTokenSource, TokenUser);
	TOKEN_GROUPS*		 pTokenGroups		= (PTOKEN_GROUPS)		 GetFromToken(hTokenSource, TokenGroups);
	TOKEN_OWNER*		 pTokenOwner		= (PTOKEN_OWNER)		 GetFromToken(hTokenSource, TokenOwner);
	TOKEN_PRIMARY_GROUP* pTokenPrimaryGroup = (PTOKEN_PRIMARY_GROUP) GetFromToken(hTokenSource, TokenPrimaryGroup);
	TOKEN_PRIVILEGES*	 pTokenPrivileges	= (PTOKEN_PRIVILEGES)	 GetFromToken(hTokenSource, TokenPrivileges);
	TOKEN_DEFAULT_DACL*	 pTokenDefaultDacl	= (PTOKEN_DEFAULT_DACL)	 GetFromToken(hTokenSource, TokenDefaultDacl);

	DisplayTokenSids(pTokenUser, pTokenGroups, pTokenOwner, pTokenPrimaryGroup);
	// DisplayTokenPrivileges(pTokenPrivileges);
	DisplayDacl(pTokenDefaultDacl->DefaultDacl);

	free(pTokenUser);
	free(pTokenGroups);
	free(pTokenOwner);
	free(pTokenPrimaryGroup);
	free(pTokenPrivileges);
	free(pTokenDefaultDacl);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
BOOL
EnablePrivilege(LPWSTR wszName)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES priv = {1, {0, 0, SE_PRIVILEGE_ENABLED}};
	BOOL bResult;

    LookupPrivilegeValue(0, wszName, &priv.Privileges[0].Luid);

    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);

    AdjustTokenPrivileges(hToken, FALSE, &priv, sizeof priv, 0, 0);
    bResult = GetLastError() == ERROR_SUCCESS;

    CloseHandle(hToken);
    return bResult;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NTSTATUS
CreateInitialSystemToken(HANDLE* phSystemToken)
{
	static SID   sidSystem			  = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID };
	static SID   sidEveryone		  = { 1, 1, SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID };
	static SID   sidAuthenticatedUser = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_AUTHENTICATED_USER_RID };
	static SID_2 sidAdministrators	  = { 1, 2, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS };
	static const int nGroupCount = 3;

	NTSTATUS status;
	ULONG uSize;
	DWORD i;

	TOKEN_USER			tkUser;
	TOKEN_OWNER			tkDefaultOwner;
	TOKEN_PRIMARY_GROUP	tkPrimaryGroup;

	TOKEN_GROUPS*		ptkGroups = 0;
	TOKEN_PRIVILEGES*	ptkPrivileges = 0;
	TOKEN_DEFAULT_DACL	tkDefaultDacl = { 0 };

	LARGE_INTEGER tkExpiration;

	LUID authId = SYSTEM_LUID;

	TOKEN_SOURCE source =
	{
		{ '*', '*', 'A', 'N', 'O', 'N', '*', '*' },
		{0, 0}
	};

	SECURITY_QUALITY_OF_SERVICE sqos =
	{
		sizeof(sqos),
		SecurityAnonymous,
		SECURITY_STATIC_TRACKING,
		FALSE
	};

	OBJECT_ATTRIBUTES oa =
	{
		sizeof(oa),
		0,
		0,
		0,
		0,
		&sqos
	};

	tkExpiration.QuadPart = -1;
	status = ZwAllocateLocallyUniqueId(&source.SourceIdentifier);
	if ( status != 0 )
		return status;

	tkUser.User.Sid = &sidSystem;
	tkUser.User.Attributes = 0;

	// Under WinXP (the only MS OS I've tested) ZwCreateToken()
	// squawks if we use sidAdministrators here -- though running
	// a progrem under AT and using the DisplayToken() function
	// shows that the system token does default ownership to
	// Administrator.

	// For now, default ownership to system, since that works
	tkDefaultOwner.Owner = &sidSystem;
	tkPrimaryGroup.PrimaryGroup = &sidSystem;

	uSize = sizeof(TOKEN_GROUPS) - sizeof(ptkGroups->Groups);
	uSize += sizeof(SID_AND_ATTRIBUTES) * nGroupCount;

	ptkGroups = (TOKEN_GROUPS*) malloc(uSize);
	ptkGroups->GroupCount = nGroupCount;

	ptkGroups->Groups[0].Sid = (SID*) &sidAdministrators;
	ptkGroups->Groups[0].Attributes = SE_GROUP_ENABLED;

	ptkGroups->Groups[1].Sid = &sidEveryone;
	ptkGroups->Groups[1].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;

	ptkGroups->Groups[2].Sid = &sidAuthenticatedUser;
	ptkGroups->Groups[2].Attributes = SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;

	uSize = sizeof(TOKEN_PRIVILEGES) - sizeof(ptkPrivileges->Privileges);
	uSize += sizeof(LUID_AND_ATTRIBUTES) * sizeof(InitialPrivilegeSet) / sizeof(InitialPrivilegeSet[0]);
	ptkPrivileges = (TOKEN_PRIVILEGES*) malloc(uSize);
	ptkPrivileges->PrivilegeCount = sizeof(InitialPrivilegeSet) / sizeof(InitialPrivilegeSet[0]);
	for (i = 0; i < ptkPrivileges->PrivilegeCount; i++)
	{
		ptkPrivileges->Privileges[i].Luid.HighPart = InitialPrivilegeSet[i].Luid.HighPart;
		ptkPrivileges->Privileges[i].Luid.LowPart = InitialPrivilegeSet[i].Luid.LowPart;
		ptkPrivileges->Privileges[i].Attributes = InitialPrivilegeSet[i].Attributes;
	}

	// Calculate the length needed for the ACL
	uSize = sizeof(ACL);
	uSize += sizeof(ACE_HEADER) + sizeof(sidSystem);
	uSize += sizeof(ACE_HEADER) + sizeof(sidAdministrators);
	uSize = (uSize & (~3)) + 8;
	tkDefaultDacl.DefaultDacl = (PACL) malloc(uSize);

	status = RtlCreateAcl(tkDefaultDacl.DefaultDacl, uSize, ACL_REVISION);
	if ( ! NT_SUCCESS(status) )
		printf("RtlCreateAcl() failed: 0x%08x\n", status);

	status = RtlAddAccessAllowedAce(tkDefaultDacl.DefaultDacl, ACL_REVISION, GENERIC_ALL, &sidSystem);
	if ( ! NT_SUCCESS(status) )
		printf("RtlAddAccessAllowedAce() failed: 0x%08x\n", status);

	status = RtlAddAccessAllowedAce(tkDefaultDacl.DefaultDacl, ACL_REVISION, GENERIC_READ|GENERIC_EXECUTE|READ_CONTROL, (PSID) &sidAdministrators);
	if ( ! NT_SUCCESS(status) )
		printf("RtlAddAccessAllowedAce() failed: 0x%08x\n", status);

	printf("Parameters being passed into ZwCreateToken:\n\n");
	DisplayTokenSids(&tkUser, ptkGroups, &tkDefaultOwner, &tkPrimaryGroup);
	DisplayDacl(tkDefaultDacl.DefaultDacl);

	printf("Calling ZwCreateToken()...\n");
	status = ZwCreateToken(phSystemToken,
						   TOKEN_ALL_ACCESS,
						   &oa,
						   TokenPrimary,
						   &authId,
						   &tkExpiration,
						   &tkUser,
						   ptkGroups,
						   ptkPrivileges,
						   &tkDefaultOwner,
						   &tkPrimaryGroup,
						   &tkDefaultDacl,
						   &source);

	// Cleanup
	free(ptkGroups);
	free(ptkPrivileges);
	free(tkDefaultDacl.DefaultDacl);

	return status;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int
main(int argc, char** argv[])
{
	NTSTATUS Status;
	HANDLE hSystemToken;
	CHAR buffer[512];
	HANDLE hOurToken;

        printf("Current process Token:\n");

        Status=ZwOpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_QUERY_SOURCE, &hOurToken);
	if ( NT_SUCCESS(Status) )
	{
	  DisplayToken(hOurToken);
	  CloseHandle(hOurToken);
	}
	else
	{
	  printf("ZwOpenProcessToken() failed: 0x%08x\n", Status);
	}

//#define ENABLE_PRIVILEGE
#ifdef ENABLE_PRIVILEGE
    EnablePrivilege(SE_CREATE_TOKEN_NAME);
#endif

	// Now do the other one
	Status = CreateInitialSystemToken(&hSystemToken);
	if ( NT_SUCCESS(Status) )
	{
		printf("System Token: 0x%08x\n", hSystemToken);
		DisplayToken(hSystemToken);
		CloseHandle(hSystemToken);
	}
	else
	{
		printf("CreateInitialSystemToken() return: 0x%08x\n", Status);
	}

	printf("press return");
	gets(buffer);

	return 0;
}
