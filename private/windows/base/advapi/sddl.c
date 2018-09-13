/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    sddl.c

Abstract:

    This module implements the Security Descriptor Definition Language support functions

Author:

    Mac McLain          (MacM)       Nov 07, 1997

Environment:

    User Mode

Revision History:

    Jin Huang           (JinHuang)  3/4/98   Fix validity flags (GetAceFlagsInTable)
    Jin Huang           (JinHuang)  3/10/98  Add SD controls (GetSDControlForString)
                                             Set SidsInitialized flag
                                             Skip any possible spaces in string
    Jin Huang           (JinHuang)  5/1/98   Fix memory leek, error checking
                                             improve performance
    Alaa Abdelhalim     (Alaa)      7/20/99  Initialize sbz2 field to 0 in LocalGetAclForString
                                             function.
--*/
#include "advapi.h"
#include <windef.h>
#include <stdio.h>
#include <wchar.h>
#include <sddl.h>
#include <ntseapi.h>
#include <seopaque.h>
#include <accctrl.h>
#include <rpcdce.h>
#include <ntlsa.h>
#include "sddlp.h"

//
// include and defines for ldap calls
//
#include <winldap.h>
#include <ntldap.h>

typedef LDAP * (LDAPAPI *PFN_LDAP_OPEN)( PCHAR, ULONG );
typedef ULONG (LDAPAPI *PFN_LDAP_UNBIND)( LDAP * );
typedef ULONG (LDAPAPI *PFN_LDAP_SEARCH)(LDAP *, PCHAR, ULONG, PCHAR, PCHAR *, ULONG,PLDAPControlA *, PLDAPControlA *, struct l_timeval *, ULONG, LDAPMessage **);
typedef LDAPMessage * (LDAPAPI *PFN_LDAP_FIRST_ENTRY)( LDAP *, LDAPMessage * );
typedef PCHAR * (LDAPAPI *PFN_LDAP_GET_VALUE)(LDAP *, LDAPMessage *, PCHAR );
typedef ULONG (LDAPAPI *PFN_LDAP_MSGFREE)( LDAPMessage * );
typedef ULONG (LDAPAPI *PFN_LDAP_VALUE_FREE)( PCHAR * );
typedef ULONG (LDAPAPI *PFN_LDAP_MAP_ERROR)( ULONG );

//
// To allow the defines to be used as Wide strings, redefine the TEXT macro
//
#ifdef TEXT
#undef TEXT
#endif
#define TEXT(quote) L##quote

//
// Local macros
//
#define STRING_GUID_LEN 36
#define STRING_GUID_SIZE  ( STRING_GUID_LEN * sizeof( WCHAR ) )
#define SDDL_LEN_TAG( tagdef )  ( sizeof( tagdef ) / sizeof( WCHAR ) - 1 )
#define SDDL_SIZE_TAG( tagdef )  ( wcslen( tagdef ) * sizeof( WCHAR ) )
#define SDDL_SIZE_SEP( sep ) (sizeof( WCHAR ) )

#define SDDL_VALID_DACL  0x00000001
#define SDDL_VALID_SACL  0x00000002

//
// This structure is used to do some lookups for mapping ACES
//
typedef struct _STRSD_KEY_LOOKUP {

    PWSTR Key;
    ULONG KeyLen;
    ULONG Value;
    ULONG ValidityFlags;

} STRSD_KEY_LOOKUP, *PSTRSD_KEY_LOOKUP;

typedef enum _STRSD_SID_TYPE {
    ST_DOMAIN_RELATIVE = 0,
    ST_WORLD,
    ST_LOCALSY,
    ST_LOCAL,
    ST_CREATOR,
    ST_NTAUTH,
    ST_BUILTIN,
    ST_ROOT_DOMAIN_RELATIVE
} STRSD_SID_TYPE;

//
// This structure is used to map account monikers to sids
//
typedef struct _STRSD_SID_LOOKUP {

    BOOLEAN Valid;
    PWSTR Key;
    ULONG KeyLen;
    PSID Sid;
    ULONG Rid;
    STRSD_SID_TYPE SidType;
    DWORD SidBuff[ sizeof( SID ) / sizeof( DWORD ) + 5];
} STRSD_SID_LOOKUP, *PSTRSD_SID_LOOKUP;


//
// Globally defined sids
//
/* JINHUANG: not used anywhere
DWORD PersonalSelfBuiltSid[sizeof(SID)/sizeof(DWORD) + 2];
DWORD AuthUserBuiltSid[sizeof(SID)/sizeof(DWORD) + 2];
DWORD CreatorOwnerBuiltSid[sizeof(SID)/sizeof(DWORD) + 2];
DWORD CreatorGroupBuiltSid[sizeof(SID)/sizeof(DWORD) + 2];
PSID  PersonalSelfSid = (PSID)PersonalSelfBuiltSid;
PSID  AuthUserSid = (PSID)AuthUserBuiltSid;
PSID  CreatorOwnerSid = (PSID)CreatorOwnerBuiltSid;
PSID  CreatorGroupSid = (PSID)CreatorGroupBuiltSid;
*/

CRITICAL_SECTION SddlSidLookupCritical;
static DWORD SidTableReinitializeInstance=0;

//    JINHUANG 3/26 BVT break for dcpromo
//
//    Some of the Valid fields were preset to TRUE with NULL Sid field. The SidLookup
//    table initialization is stopped if Status is not SUCCESS. So if error occurs,
//    for example, no domain info as in dcpromo, other SIDs will not be initialized
//    but the Valid fields are set to TRUE (with NULL SIDs).
//
//    changes: 1) preset Valid field to FALSE all all lookups and set the Valid to TRUE if
//                the SID is really initialized
//             2) do not stop the initialization process if an error occurs
//                if the Valid field is already TRUE (already initialized), skip the row
//
static STRSD_SID_LOOKUP  SidLookup[] = {
        { FALSE, SDDL_DOMAIN_ADMINISTRATORS, SDDL_LEN_TAG( SDDL_DOMAIN_ADMINISTRATORS ),
            NULL, DOMAIN_GROUP_RID_ADMINS, ST_DOMAIN_RELATIVE, 0 },
        { FALSE, SDDL_DOMAIN_GUESTS, SDDL_LEN_TAG( SDDL_DOMAIN_GUESTS ),
            NULL, DOMAIN_GROUP_RID_GUESTS, ST_DOMAIN_RELATIVE, 0 },
        { FALSE, SDDL_DOMAIN_USERS, SDDL_LEN_TAG( SDDL_DOMAIN_USERS ),
              NULL, DOMAIN_GROUP_RID_USERS, ST_DOMAIN_RELATIVE, 0 },
        { FALSE, SDDL_DOMAIN_DOMAIN_CONTROLLERS, SDDL_LEN_TAG( SDDL_DOMAIN_DOMAIN_CONTROLLERS ),
              NULL, DOMAIN_GROUP_RID_CONTROLLERS, ST_DOMAIN_RELATIVE, 0 },
        { FALSE, SDDL_DOMAIN_COMPUTERS, SDDL_LEN_TAG( SDDL_DOMAIN_COMPUTERS ),
              NULL, DOMAIN_GROUP_RID_COMPUTERS, ST_DOMAIN_RELATIVE, 0 },
        { FALSE, SDDL_SCHEMA_ADMINISTRATORS, SDDL_LEN_TAG( SDDL_SCHEMA_ADMINISTRATORS ),
              NULL, DOMAIN_GROUP_RID_SCHEMA_ADMINS, ST_ROOT_DOMAIN_RELATIVE, 0 },  // should be root domain only ST_DOMAIN_RELATIVE,
        { FALSE, SDDL_ENTERPRISE_ADMINS, SDDL_LEN_TAG( SDDL_ENTERPRISE_ADMINS ),
              NULL, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS, ST_ROOT_DOMAIN_RELATIVE, 0 }, // root domain only
        { FALSE, SDDL_CERT_SERV_ADMINISTRATORS, SDDL_LEN_TAG( SDDL_CERT_SERV_ADMINISTRATORS ),
              NULL, DOMAIN_GROUP_RID_CERT_ADMINS, ST_DOMAIN_RELATIVE, 0 },
        { FALSE, SDDL_ACCOUNT_OPERATORS, SDDL_LEN_TAG( SDDL_ACCOUNT_OPERATORS ),
              NULL, DOMAIN_ALIAS_RID_ACCOUNT_OPS, ST_BUILTIN, 0 },
        { FALSE, SDDL_BACKUP_OPERATORS, SDDL_LEN_TAG( SDDL_BACKUP_OPERATORS ),
              NULL, DOMAIN_ALIAS_RID_BACKUP_OPS, ST_BUILTIN, 0 },
        { FALSE, SDDL_PRINTER_OPERATORS, SDDL_LEN_TAG( SDDL_PRINTER_OPERATORS ),
              NULL, DOMAIN_ALIAS_RID_PRINT_OPS, ST_BUILTIN, 0 },
        { FALSE, SDDL_SERVER_OPERATORS, SDDL_LEN_TAG( SDDL_SERVER_OPERATORS ),
              NULL, DOMAIN_ALIAS_RID_SYSTEM_OPS, ST_BUILTIN, 0 },
        { FALSE, SDDL_REPLICATOR, SDDL_LEN_TAG( SDDL_REPLICATOR ),
              NULL, DOMAIN_ALIAS_RID_REPLICATOR, ST_BUILTIN, 0 },
        { FALSE, SDDL_RAS_SERVERS, SDDL_LEN_TAG( SDDL_RAS_SERVERS ),
              NULL, DOMAIN_ALIAS_RID_RAS_SERVERS, ST_DOMAIN_RELATIVE, 0 },  // ST_LOCAL
        { FALSE, SDDL_AUTHENTICATED_USERS, SDDL_LEN_TAG( SDDL_AUTHENTICATED_USERS ),
              NULL, SECURITY_AUTHENTICATED_USER_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_PERSONAL_SELF, SDDL_LEN_TAG( SDDL_PERSONAL_SELF ),
              NULL, SECURITY_PRINCIPAL_SELF_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_CREATOR_OWNER, SDDL_LEN_TAG( SDDL_CREATOR_OWNER ),
              NULL, SECURITY_CREATOR_OWNER_RID, ST_CREATOR, 0 },
        { FALSE, SDDL_CREATOR_GROUP, SDDL_LEN_TAG( SDDL_CREATOR_GROUP ),
              NULL, SECURITY_CREATOR_GROUP_RID, ST_CREATOR, 0 },
        { FALSE, SDDL_LOCAL_SYSTEM, SDDL_LEN_TAG( SDDL_LOCAL_SYSTEM ),
              NULL, SECURITY_LOCAL_SYSTEM_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_INTERACTIVE, SDDL_LEN_TAG( SDDL_INTERACTIVE ),
              NULL, SECURITY_INTERACTIVE_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_NETWORK, SDDL_LEN_TAG( SDDL_NETWORK ),
              NULL, SECURITY_NETWORK_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_SERVICE, SDDL_LEN_TAG( SDDL_SERVICE ),
              NULL, SECURITY_SERVICE_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_ENTERPRISE_DOMAIN_CONTROLLERS, SDDL_LEN_TAG( SDDL_ENTERPRISE_DOMAIN_CONTROLLERS ),
              NULL, SECURITY_SERVER_LOGON_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_RESTRICTED_CODE, SDDL_LEN_TAG( SDDL_RESTRICTED_CODE ),
              NULL, SECURITY_RESTRICTED_CODE_RID, ST_NTAUTH, 0 },
        { FALSE, SDDL_LOCAL_ADMIN, SDDL_LEN_TAG( SDDL_LOCAL_ADMIN ),
              NULL, DOMAIN_USER_RID_ADMIN, ST_LOCAL, 0 },
        { FALSE, SDDL_LOCAL_GUEST, SDDL_LEN_TAG( SDDL_LOCAL_GUEST ),
              NULL, DOMAIN_USER_RID_GUEST, ST_LOCAL, 0 },
        { FALSE, SDDL_BUILTIN_ADMINISTRATORS, SDDL_LEN_TAG( SDDL_BUILTIN_ADMINISTRATORS ),
              NULL, DOMAIN_ALIAS_RID_ADMINS, ST_BUILTIN, 0 },
        { FALSE, SDDL_BUILTIN_GUESTS, SDDL_LEN_TAG( SDDL_BUILTIN_GUESTS ),
              NULL, DOMAIN_ALIAS_RID_GUESTS, ST_BUILTIN, 0 },
        { FALSE, SDDL_BUILTIN_USERS, SDDL_LEN_TAG( SDDL_BUILTIN_USERS ),
              NULL, DOMAIN_ALIAS_RID_USERS, ST_BUILTIN, 0 },
        { FALSE, SDDL_POWER_USERS, SDDL_LEN_TAG( SDDL_POWER_USERS ),
              NULL, DOMAIN_ALIAS_RID_POWER_USERS, ST_BUILTIN, 0 },
        { FALSE, SDDL_EVERYONE, SDDL_LEN_TAG( SDDL_EVERYONE ),
              NULL, SECURITY_WORLD_RID, ST_WORLD, 0 },
        { FALSE, SDDL_GROUP_POLICY_ADMINS, SDDL_LEN_TAG( SDDL_GROUP_POLICY_ADMINS ),
              NULL, DOMAIN_GROUP_RID_POLICY_ADMINS, ST_DOMAIN_RELATIVE, 0 },
        { FALSE, SDDL_ALIAS_PREW2KCOMPACC, SDDL_LEN_TAG( SDDL_ALIAS_PREW2KCOMPACC ),
              NULL, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS, ST_BUILTIN, 0 }
    };

static DWORD RootDomSidBuf[sizeof(SID)/sizeof(DWORD)+5];
static BOOL RootDomInited=FALSE;

#define STRSD_REINITIALIZE_ENTER              1
#define STRSD_REINITIALIZE_LEAVE              2

BOOLEAN
InitializeSidLookupTable(
    IN BYTE InitFlag
    );

//
// Control Lookup table
//
static STRSD_KEY_LOOKUP ControlLookup[] = {
    { SDDL_PROTECTED, SDDL_LEN_TAG( SDDL_PROTECTED ), SE_DACL_PROTECTED, SDDL_VALID_DACL },
    { SDDL_AUTO_INHERIT_REQ, SDDL_LEN_TAG( SDDL_AUTO_INHERIT_REQ ), SE_DACL_AUTO_INHERIT_REQ, SDDL_VALID_DACL },
    { SDDL_AUTO_INHERITED, SDDL_LEN_TAG( SDDL_AUTO_INHERITED ), SE_DACL_AUTO_INHERITED, SDDL_VALID_DACL },
    { SDDL_PROTECTED, SDDL_LEN_TAG( SDDL_PROTECTED ), SE_SACL_PROTECTED, SDDL_VALID_SACL },
    { SDDL_AUTO_INHERIT_REQ, SDDL_LEN_TAG( SDDL_AUTO_INHERIT_REQ ), SE_SACL_AUTO_INHERIT_REQ, SDDL_VALID_SACL },
    { SDDL_AUTO_INHERITED, SDDL_LEN_TAG( SDDL_AUTO_INHERITED ), SE_SACL_AUTO_INHERITED, SDDL_VALID_SACL }
    };

//
// Local prototypes
//
BOOL
LocalConvertStringSidToSid(
    IN  PWSTR String,
    OUT PSID *SID,
    OUT PWSTR *End
    );

PSTRSD_SID_LOOKUP
LookupSidInTable(
    IN PWSTR String, OPTIONAL
    IN PSID Sid OPTIONAL,
    IN PSID RootDomainSid OPTIONAL,
    IN BOOLEAN DefaultToDomain,
    OUT PVOID *pSASid
    );

DWORD
LocalGetSidForString(
    IN  PWSTR String,
    OUT PSID *SID,
    OUT PWSTR *End,
    OUT PBOOLEAN FreeSid,
    IN  PSID RootDomainSid OPTIONAL,
    IN  BOOLEAN DefaultToDomain
    );

PSTRSD_KEY_LOOKUP
LookupAccessMaskInTable(
    IN PWSTR String, OPTIONAL
    IN ULONG AccessMask, OPTIONAL
    IN ULONG LookupFlags
    );


PSTRSD_KEY_LOOKUP
LookupAceTypeInTable(
    IN PWSTR String, OPTIONAL
    IN ULONG AceType, OPTIONAL
    IN ULONG LookupFlags
    );

PSTRSD_KEY_LOOKUP
LookupAceFlagsInTable(
    IN PWSTR String, OPTIONAL
    IN ULONG AceFlags OPTIONAL,
    IN ULONG LookupFlags
    );

DWORD
LocalGetStringForSid(
    IN  PSID Sid,
    OUT PWSTR *String,
    IN  PSID RootDomainSid OPTIONAL
    );

DWORD
LocalGetStringForControl(
    IN SECURITY_DESCRIPTOR_CONTROL ControlCode,
    IN ULONG LookupFlags,
    OUT PWSTR *ControlString
    );

DWORD
LocalGetSDControlForString (
    IN  PWSTR AclString,
    IN ULONG LookupFlags,
    OUT SECURITY_DESCRIPTOR_CONTROL *pControl,
    OUT PWSTR *End
    );

DWORD
LocalGetAclForString (
    IN  PWSTR AclString,
    IN  BOOLEAN ConvertAsDacl,
    OUT PACL *Acl,
    OUT PWSTR *End,
    IN  PSID RootDomainSid OPTIONAL,
    IN  BOOLEAN DefaultToDomain
    );

DWORD
LocalConvertAclToString(
    IN PACL Acl,
    IN BOOLEAN AclPresent,
    IN BOOLEAN ConvertAsDacl,
    OUT PWSTR *AclString,
    OUT PDWORD AclStringSize,
    IN PSID RootDomainSid OPTIONAL
    );

DWORD
LocalConvertSDToStringSD_Rev1(
    IN  PSID RootDomainSid OPTIONAL,
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  SECURITY_INFORMATION  SecurityInformation,
    OUT LPWSTR  *StringSecurityDescriptor,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    );

DWORD
LocalConvertStringSDToSD_Rev1(
    IN  PSID RootDomainSid OPTIONAL,
    IN  BOOLEAN DefaultToDomain,
    IN  LPCWSTR StringSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    );

BOOL
SddlpGetRootDomainSid(void);

//
// Exported functions
//

BOOL
APIENTRY
ConvertSidToStringSidA(
    IN  PSID     Sid,
    OUT LPSTR  *StringSid
    )
/*++

Routine Description:

    ANSI thunk to ConvertSidToStringSidW

--*/
{
    LPWSTR StringSidW = NULL;
    ULONG AnsiLen, WideLen;
    BOOL ReturnValue;

    if ( NULL == StringSid ) {
        //
        // invalid parameter
        //
        SetLastError( ERROR_INVALID_PARAMETER );
        return(FALSE);
    }

    ReturnValue = ConvertSidToStringSidW( Sid, &StringSidW );

    if ( ReturnValue ) {

        WideLen = wcslen( StringSidW ) + 1;

        AnsiLen = WideCharToMultiByte( CP_ACP,
                                       0,
                                       StringSidW,
                                       WideLen,
                                       *StringSid,
                                       0,
                                       NULL,
                                       NULL );

        if ( AnsiLen != 0 ) {

            *StringSid = LocalAlloc( LMEM_FIXED, AnsiLen );

            if ( *StringSid == NULL ) {

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                ReturnValue = FALSE;

            } else {

                AnsiLen = WideCharToMultiByte( CP_ACP,
                                               0,
                                               StringSidW,
                                               WideLen,
                                               *StringSid,
                                               AnsiLen,
                                               NULL,
                                               NULL );
                ASSERT( AnsiLen != 0 );

                if ( AnsiLen == 0 ) {

                    ReturnValue = FALSE;
                    //
                    // jinhuang: failed, free the buffer
                    //
                    LocalFree(*StringSid);
                    *StringSid = NULL;
                }
            }

        } else {

            ReturnValue = FALSE;
        }

    }

    //
    // jinhuang: free the wide buffer
    //
    if ( StringSidW ) {
        LocalFree(StringSidW);
    }

    if ( ReturnValue ) {
        SetLastError(ERROR_SUCCESS);
    }

    return( ReturnValue );

}


BOOL
APIENTRY
ConvertSidToStringSidW(
    IN  PSID     Sid,
    OUT LPWSTR  *StringSid
    )
/*++

Routine Description:

    This routine converts a SID into a string representation of a SID, suitable for framing or
    display

Arguments:

    Sid - SID to be converted.

    StringSid - Where the converted SID is returned.  Allocated via LocalAlloc and needs to
        be freed via LocalFree.


Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeStringSid;

    if ( NULL == Sid || NULL == StringSid ) {
        //
        // invalid parameter
        //
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    //
    // Convert using the Rtl functions
    //
    Status = RtlConvertSidToUnicodeString( &UnicodeStringSid, Sid, TRUE );

    if ( !NT_SUCCESS( Status ) ) {

        BaseSetLastNTError( Status );
        return( FALSE );
    }

    //
    // Convert it to the proper allocator
    //
    *StringSid = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                             UnicodeStringSid.Length + sizeof( WCHAR ) );

    if ( *StringSid == NULL ) {

        RtlFreeUnicodeString( &UnicodeStringSid );

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( FALSE );

    }

    RtlCopyMemory( *StringSid, UnicodeStringSid.Buffer, UnicodeStringSid.Length );
    RtlFreeUnicodeString( &UnicodeStringSid );

    SetLastError(ERROR_SUCCESS);
    return( TRUE );
}


BOOL
APIENTRY
ConvertStringSidToSidA(
    IN LPCSTR  StringSid,
    OUT PSID   *Sid
    )
/*++

Routine Description:

    ANSI thunk to ConvertStringSidToSidW

--*/
{
    UNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    BOOL Result;

    if ( NULL == StringSid || NULL == Sid ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return(FALSE);
    }

    RtlInitAnsiString( &AnsiString, StringSid );

    Status = RtlAnsiStringToUnicodeString( &Unicode,
                                           &AnsiString,
                                           TRUE);

    if ( !NT_SUCCESS( Status ) ) {

        BaseSetLastNTError( Status );

        return FALSE;

    }


    Result = ConvertStringSidToSidW( ( LPCWSTR )Unicode.Buffer, Sid );

    RtlFreeUnicodeString( &Unicode );

    if ( Result ) {
        SetLastError(ERROR_SUCCESS);
    }

    return( Result );
}


BOOL
APIENTRY
ConvertStringSidToSidW(
    IN LPCWSTR  StringSid,
    OUT PSID   *Sid
    )
/*++

Routine Description:

    This routine converts a stringized SID into a valid, functional SID

Arguments:

    StringSid - SID to be converted.

    Sid - Where the converted SID is returned.  Buffer is allocated via LocalAlloc and should
        be free via LocalFree.


Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

        ERROR_INVALID_PARAMETER - A NULL name was given

        ERROR_INVALID_SID - The format of the given sid was incorrect

--*/
{
    PWSTR End = NULL;
    BOOL ReturnValue = FALSE;

    if ( StringSid == NULL || Sid == NULL ) {

        SetLastError( ERROR_INVALID_PARAMETER );

    } else {

        ReturnValue = LocalConvertStringSidToSid( ( PWSTR )StringSid, Sid, &End );

        if ( ReturnValue == TRUE ) {

            if ( ( ULONG )( End - StringSid ) != wcslen( StringSid ) ) {

                SetLastError( ERROR_INVALID_SID );
                LocalFree( *Sid );
                *Sid = FALSE;
                ReturnValue = FALSE;

            } else {
                SetLastError(ERROR_SUCCESS);
            }

        }

    }

    return( ReturnValue );

}


BOOL
APIENTRY
ConvertStringSecurityDescriptorToSecurityDescriptorA(
    IN  LPCSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    )
/*++

Routine Description:

    ANSI thunk to ConvertStringSecurityDescriptorToSecurityDescriptorW

--*/
{
    UNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    BOOL Result;

    if ( NULL == StringSecurityDescriptor ||
         NULL == SecurityDescriptor ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    RtlInitAnsiString( &AnsiString, StringSecurityDescriptor );

    Status = RtlAnsiStringToUnicodeString( &Unicode,
                                           &AnsiString,
                                           TRUE);

    if ( !NT_SUCCESS( Status ) ) {

        BaseSetLastNTError( Status );

        return FALSE;

    }

    Result = ConvertStringSecurityDescriptorToSecurityDescriptorW( ( LPCWSTR )Unicode.Buffer,
                                                                   StringSDRevision,
                                                                   SecurityDescriptor,
                                                                   SecurityDescriptorSize);

    RtlFreeUnicodeString( &Unicode );

    if ( Result ) {
        SetLastError(ERROR_SUCCESS);
    }

    return( Result );
}



BOOL
APIENTRY
ConvertStringSecurityDescriptorToSecurityDescriptorW(
    IN  LPCWSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    )
/*++

Routine Description:

    This routine converts a stringized Security descriptor into a valid, functional security
    descriptor

    Ex:
    SD:[O:xyz][G:xyz][D: (Ace1)(Ace2)][S: (Ace3)(Ace4)]
          where some Ace is (OA;CIIO; DS_READ; OT: abc; IOT: abc; SID: xyz)

    So a possible Security descriptor may be (as all one long string):

    L"O:AOG:DAD:(A;IO;RPRWXRCWDWO;;;S-1-0-0)(OA;CI;RWX;af110080-1b13-11d0-af10-0020afd3606c;"
    L"a153d9e0-1b13-11d0-af10-0020afd3606c;AUS)(A;SAFA;0x7800003F;;;DA)(OA;FA;X;"
    L"954378e0-1b13-11d0-af10-0020afd3606c;880b12a0-1b13-11d0-af10-0020afd3606c;PO)"

    would build a security descriptor:

    Revision: 0x1
    Sbz1: 0x0
    Control: 0x8014
    Owner: S-1-5-32-548

    Group:S-1-5-32-544

    Dacl: Revision: 4
    AceCount: 2
    InUse: 84
    Free: 52
    Flags: 0
            Ace  0:
                Type: 0
                Flags: 0x1
                Size: 0x14
                Mask: 0xe00e0010
                S-1-0-0

            Ace  1:
                Type: 5
                Flags: 0x2
                Size: 0x38
                Mask: 0xe0000000
                af110080-1b13-11d0-af100020afd3606c
                a153d9e0-1b13-11d0-af100020afd3606c
                S-1-5-11


    sacl: Revision: 4
    AceCount: 2
    InUse: 92
    Free: 44
    Flags: 0
            Ace  0:
                Type: 2
                Flags: 0xc0
                Size: 0x18
                Mask: 0xe0000000
                S-1-5-32-544

            Ace  1:
                Type: 7
                Flags: 0x80
                Size: 0x3c
                Mask: 0x20000000
                954378e0-1b13-11d0-af100020afd3606c
                880b12a0-1b13-11d0-af100020afd3606c
                S-1-5-32-550
Arguments:

    StringSecurityDescriptor - Stringized security descriptor to be converted.

    StringSDRevision - String revision of the input string SD

    SecurityDescriptor - Where the converted SD is returned.  Buffer is allocated via
        LocalAlloc and should be free via LocalFree.  The returned security descriptor
        is always self relative

    SecurityDescriptorSize - OPTIONAL.  If non-NULL, the size of the converted security
        descriptor is returned here.


Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

        ERROR_INVALID_PARAMETER - A NULL input or output parameter was given

        ERROR_UNKNOWN_REVISION - An unsupported revision was given

--*/
{
    DWORD Err = ERROR_SUCCESS;

    //
    // Little elementary parameter checking...
    //
    if ( StringSecurityDescriptor == NULL || SecurityDescriptor == NULL ) {

        Err = ERROR_INVALID_PARAMETER;

    } else {

        switch ( StringSDRevision ) {
        case SDDL_REVISION_1:

            Err = LocalConvertStringSDToSD_Rev1( NULL,  // no domain sid is provided
                                                 FALSE, //TRUE, do not default to domain for EA/SA
                                                 StringSecurityDescriptor,
                                                 SecurityDescriptor,
                                                 SecurityDescriptorSize);
            break;

        default:

            Err = ERROR_UNKNOWN_REVISION;
            break;
        }

    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS );
}


BOOL
APIENTRY
ConvertSecurityDescriptorToStringSecurityDescriptorA(
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  DWORD RequestedStringSDRevision,
    IN  SECURITY_INFORMATION SecurityInformation,
    OUT LPSTR  *StringSecurityDescriptor,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    )
/*++

Routine Description:

    ANSI thunk to ConvertSecurityDescriptorToStringSecurityDescriptorW

--*/
{
    LPWSTR StringSecurityDescriptorW = NULL;
    ULONG AnsiLen, WideLen;
    BOOL ReturnValue ;

    if ( StringSecurityDescriptor == NULL ||
         0 == SecurityInformation ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    ReturnValue = ConvertSecurityDescriptorToStringSecurityDescriptorW(
                      SecurityDescriptor,
                      RequestedStringSDRevision,
                      SecurityInformation,
                      &StringSecurityDescriptorW,
                      &WideLen );

    if ( ReturnValue ) {

//  jinhuang: WindeLen is returned from previous call
//        WideLen = wcslen( StringSecurityDescriptorW ) + 1;

        AnsiLen = WideCharToMultiByte( CP_ACP,
                                       0,
                                       StringSecurityDescriptorW,
                                       WideLen+1,
                                       *StringSecurityDescriptor,
                                       0,
                                       NULL,
                                       NULL );

        if ( AnsiLen != 0 ) {

            *StringSecurityDescriptor = LocalAlloc( LMEM_FIXED, AnsiLen );

            if ( *StringSecurityDescriptor == NULL ) {

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                ReturnValue = FALSE;

            } else {

                AnsiLen = WideCharToMultiByte( CP_ACP,
                                               0,
                                               StringSecurityDescriptorW,
                                               WideLen+1,
                                               *StringSecurityDescriptor,
                                               AnsiLen,
                                               NULL,
                                               NULL );
                ASSERT( AnsiLen != 0 );

                if ( AnsiLen == 0 ) {

                    LocalFree(*StringSecurityDescriptor);
                    *StringSecurityDescriptor = NULL;

                    ReturnValue = FALSE;
                }

                //
                // jinhuang
                // output the length (optional)
                //
                if ( StringSecurityDescriptorLen ) {
                    *StringSecurityDescriptorLen = AnsiLen;
                }

            }

        } else {

            ReturnValue = FALSE;
        }

        //
        // jinhuang
        // StringSecurityDescriptorW should be freed
        //

        LocalFree(StringSecurityDescriptorW);

    }

    if ( ReturnValue ) {
        SetLastError(ERROR_SUCCESS);
    }

    return( ReturnValue );
}


BOOL
APIENTRY
ConvertSecurityDescriptorToStringSecurityDescriptorW(
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  DWORD RequestedStringSDRevision,
    IN  SECURITY_INFORMATION SecurityInformation,
    OUT LPWSTR  *StringSecurityDescriptor,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    )
/*++

Routine Description:

    This routine converts a security descriptor into a string version persuant to SDDL definition

Arguments:

    SecurityDescriptor - Security Descriptor to be converted.

    RequestedStringSDRevision - Requested revision of the output string security descriptor

    SecurityInformation - security information of which to be converted

    StringSecurityDescriptor - Where the converted SD is returned.  Buffer is allocated via
        LocalAlloc and should be free via LocalFree.

    StringSecurityDescriptorLen - the optional length of the converted SD

Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

        ERROR_INVALID_PARAMETER - A NULL input or output parameter was given

        ERROR_UNKNOWN_REVISION - An unsupported revision was given

--*/
{
    DWORD Err = ERROR_SUCCESS;

    //
    // A little parameter checking...
    //
    if ( SecurityDescriptor == NULL || StringSecurityDescriptor == NULL ||
         SecurityInformation == 0 ) {

        Err =  ERROR_INVALID_PARAMETER;

    } else {

        switch ( RequestedStringSDRevision ) {
        case SDDL_REVISION_1:

            Err = LocalConvertSDToStringSD_Rev1( NULL,  // root domain sid is not privided
                                                 SecurityDescriptor,
                                                 SecurityInformation,
                                                 StringSecurityDescriptor,
                                                 StringSecurityDescriptorLen );
            break;

        default:
            Err = ERROR_UNKNOWN_REVISION;
            break;
        }

    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS);
}



//
// Private functions
//
BOOL
LocalConvertStringSidToSid (
    IN  PWSTR       StringSid,
    OUT PSID       *Sid,
    OUT PWSTR      *End
    )
/*++

Routine Description:

    This routine will convert a string representation of a SID back into
    a sid.  The expected format of the string is:
                "S-1-5-32-549"
    If a string in a different format or an incorrect or incomplete string
    is given, the operation is failed.

    The returned sid must be free via a call to LocalFree


Arguments:

    StringSid - The string to be converted

    Sid - Where the created SID is to be returned

    End - Where in the string we stopped processing


Return Value:

    TRUE - Success.

    FALSE - Failure.  Additional information returned from GetLastError().  Errors set are:

            ERROR_SUCCESS indicates success

            ERROR_NOT_ENOUGH_MEMORY indicates a memory allocation for the ouput sid
                                    failed
            ERROR_INVALID_SID indicates that the given string did not represent a sid

--*/
{
    DWORD Err = ERROR_SUCCESS;
    UCHAR Revision, Subs;
    SID_IDENTIFIER_AUTHORITY IDAuth;
    PULONG SubAuth = NULL;
    PWSTR CurrEnd, Curr, Next;
    WCHAR Stub, *StubPtr = NULL;
    ULONG Index;
    INT gBase=10;
    INT lBase=10;
    ULONG Auto;

    if ( NULL == StringSid || NULL == Sid || NULL == End ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );

    }

//    if ( wcslen( StringSid ) < 2 || ( *StringSid != L'S' && *( StringSid + 1 ) != L'-' ) ) {

    //
    // no need to check length because StringSid is NULL
    // and if the first char is NULL, it won't access the second char
    //
    if ( (*StringSid != L'S' && *StringSid != L's') ||
         *( StringSid + 1 ) != L'-' ) {
        //
        // string sid should always start with S-
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }


    Curr = StringSid + 2;

    if ( (*Curr == L'0') &&
         ( *(Curr+1) == L'x' ||
           *(Curr+1) == L'X' ) ) {

        gBase = 16;
    }

    Revision = ( UCHAR )wcstol( Curr, &CurrEnd, gBase );

    if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
        //
        // no revision is provided, or invalid delimeter
        //
        SetLastError( ERROR_INVALID_SID );
        return( FALSE );
    }

    Curr = CurrEnd + 1;

    //
    // Count the number of characters in the indentifer authority...
    //
    Next = wcschr( Curr, L'-' );
/*
    Length = 6 doesn't mean each digit is a id authority value, could be 0x...

    if ( Next != NULL && (Next - Curr == 6) ) {

        for ( Index = 0; Index < 6; Index++ ) {

//            IDAuth.Value[Index] = (UCHAR)Next[Index];  what is this ???
            IDAuth.Value[Index] = (BYTE) (Curr[Index]-L'0');
        }

        Curr +=6;

    } else {
*/
        if ( (*Curr == L'0') &&
             ( *(Curr+1) == L'x' ||
               *(Curr+1) == L'X' ) ) {

            lBase = 16;
        } else {
            lBase = gBase;
        }

        Auto = wcstoul( Curr, &CurrEnd, lBase );

         if ( CurrEnd == Curr || *CurrEnd != L'-' || *(CurrEnd+1) == L'\0' ) {
             //
             // no revision is provided, or invalid delimeter
             //
             SetLastError( ERROR_INVALID_SID );
             return( FALSE );
         }

         IDAuth.Value[0] = IDAuth.Value[1] = 0;
         IDAuth.Value[5] = ( UCHAR )Auto & 0xFF;
         IDAuth.Value[4] = ( UCHAR )(( Auto >> 8 ) & 0xFF );
         IDAuth.Value[3] = ( UCHAR )(( Auto >> 16 ) & 0xFF );
         IDAuth.Value[2] = ( UCHAR )(( Auto >> 24 ) & 0xFF );
         Curr = CurrEnd;
//    }

    //
    // Now, count the number of sub auths, at least one sub auth is required
    //
    Subs = 0;
    Next = Curr;

    //
    // We'll have to count our sub authoritys one character at a time,
    // since there are several deliminators that we can have...
    //

    while ( Next ) {

        if ( *Next == L'-' && *(Next-1) != L'-') {

            //
            // do not allow two continuous '-'s
            // We've found one!
            //
            Subs++;

            if ( (*(Next+1) == L'0') &&
                 ( *(Next+2) == L'x' ||
                   *(Next+2) == L'X' ) ) {
                //
                // this is hex indicator
                //
                Next += 2;

            }

        } else if ( *Next == SDDL_SEPERATORC || *Next  == L'\0' ||
                    *Next == SDDL_ACE_ENDC || *Next == L' ' ||
                    ( *(Next+1) == SDDL_DELIMINATORC &&
                      (*Next == L'G' || *Next == L'O' || *Next == L'S')) ) {
            //
            // space is a terminator too
            //
            if ( *( Next - 1 ) == L'-' ) {
                //
                // shouldn't allow a SID terminated with '-'
                //
                Err = ERROR_INVALID_SID;
                Next--;

            } else {
                Subs++;
            }

            *End = Next;
            break;

        } else if ( !iswxdigit( *Next ) ) {

            Err = ERROR_INVALID_SID;
            *End = Next;
//            Subs++;
            break;

        } else {

            //
            // Note: SID is also used as a owner or group
            //
            // Some of the tags (namely 'D' for Dacl) fall under the category of iswxdigit, so
            // if the current character is a character we care about and the next one is a
            // delminiator, we'll quit
            //
            if ( *Next == L'D' && *( Next + 1 ) == SDDL_DELIMINATORC ) {

                //
                // We'll also need to temporarily truncate the string to this length so
                // we don't accidentally include the character in one of the conversions
                //
                Stub = *Next;
                StubPtr = Next;
                *StubPtr = UNICODE_NULL;
                *End = Next;
                Subs++;
                break;
            }

        }

        Next++;

    }

    if ( Err == ERROR_SUCCESS ) {

        if ( Subs != 0 ) Subs--;

        if ( Subs != 0 ) {

            Curr++;

            SubAuth = ( PULONG )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Subs * sizeof( ULONG ) );

            if ( SubAuth == NULL ) {

                Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                for ( Index = 0; Index < Subs; Index++ ) {

                    if ( (*Curr == L'0') &&
                         ( *(Curr+1) == L'x' ||
                           *(Curr+1) == L'X' ) ) {

                        lBase = 16;
                    } else {
                        lBase = gBase;
                    }

                    SubAuth[Index] = wcstoul( Curr, &CurrEnd, lBase );
                    Curr = CurrEnd + 1;
                }
            }

        } else {

            Err = ERROR_INVALID_SID;
        }
    }

    //
    // Now, create the SID
    //
    if ( Err == ERROR_SUCCESS ) {

        *Sid = ( PSID )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                   sizeof( SID ) + Subs * sizeof( ULONG ) );

        if ( *Sid == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            PISID ISid = ( PISID )*Sid;
            ISid->Revision = Revision;
            ISid->SubAuthorityCount = Subs;
            RtlCopyMemory( &( ISid->IdentifierAuthority ), &IDAuth,
                           sizeof( SID_IDENTIFIER_AUTHORITY ) );
            RtlCopyMemory( ISid->SubAuthority, SubAuth, Subs * sizeof( ULONG ) );
        }
    }

    LocalFree( SubAuth );

    //
    // Restore any character we may have stubbed out
    //
    if ( StubPtr ) {

        *StubPtr = Stub;
    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS );
}


PSTRSD_SID_LOOKUP
LookupSidInTable(
    IN PWSTR String OPTIONAL,
    IN PSID Sid OPTIONAL,
    IN PSID RootDomainSid OPTIONAL,
    IN BOOLEAN DefaultToDomain,
    IN PVOID *pSASid
    )
/*++

Routine Description:

    This routine will determine if the given sid or string sid exists in the lookup table.

    A pointer to the matching static lookup entry is returned.


Arguments:

    String - The string to be looked up

    Sid - The sid to be looked up.

Return Value:

    Lookup table entry if found

    NULL if not found

--*/
{
    BOOLEAN LookupSid = FALSE;
    DWORD i, SidCount = sizeof( SidLookup ) / sizeof( STRSD_SID_LOOKUP );
    PSTRSD_SID_LOOKUP MatchedEntry = NULL;
    DWORD DomainAdminIndex;

    BOOL  InitRootDomain;
    ULONG Rid;
    BOOL bIsSA = FALSE;

    if ( String == NULL && Sid == NULL ) {
        //
        // JINHUANG: if both string and sid are NULL
        // just return NULL
        //
        return((PSTRSD_SID_LOOKUP)NULL);
    }

    *pSASid = NULL;

    InitializeSidLookupTable(STRSD_REINITIALIZE_ENTER);

    InitRootDomain = FALSE;
    DomainAdminIndex = SidCount;

    if ( String == NULL ) {
        //
        // lookup on the Sid
        //
        LookupSid = TRUE;

        //
        // check if the RID is for Enterprise Admins
        //
        Rid = *( RtlSubAuthoritySid( Sid,
                                     *( RtlSubAuthorityCountSid(Sid) ) - 1 ) );

        if ( DOMAIN_GROUP_RID_ENTERPRISE_ADMINS == Rid ||
             DOMAIN_GROUP_RID_SCHEMA_ADMINS == Rid ) {

            InitRootDomain = TRUE;
            if ( DOMAIN_GROUP_RID_SCHEMA_ADMINS == Rid ) {
                bIsSA = TRUE;
            }
        }

    } else {

        if ( _wcsnicmp( String, SDDL_ENTERPRISE_ADMINS, SDDL_LEN_TAG( SDDL_ENTERPRISE_ADMINS ) ) == 0 ) {
            //
            // Enterprise admins string is requested
            //
            InitRootDomain = TRUE;
        } else if ( _wcsnicmp( String, SDDL_SCHEMA_ADMINISTRATORS, SDDL_LEN_TAG( SDDL_SCHEMA_ADMINISTRATORS ) ) == 0 ) {
            //
            // schema admin is requested
            //
            InitRootDomain = TRUE;
            bIsSA = TRUE;
        }
    }

    if ( InitRootDomain &&
         RootDomainSid == NULL &&
         DefaultToDomain == FALSE &&
         ( RootDomInited == FALSE ||
           !RtlValidSid( (PSID)RootDomSidBuf ) ) ) {

        //
        // get the root domain sid (using ldap calls)
        //

        SddlpGetRootDomainSid();

    }

    for ( i = 0; i < SidCount; i++ ) {

        //
        // If this is not an entry that has been initialized yet, skip it.
        //
        if ( SidLookup[ i ].Valid == FALSE ||
             SidLookup[ i ].Sid == NULL ) {

            if ( SidLookup[ i ].SidType == ST_ROOT_DOMAIN_RELATIVE &&
                 InitRootDomain ) {

                if ( RootDomainSid != NULL ) {

                    EnterCriticalSection(&SddlSidLookupCritical);

                    RtlCopyMemory( SidLookup[ i ].Sid, RootDomainSid,
                                   RtlLengthSid( RootDomainSid ) );
                    ( ( PISID )( SidLookup[ i ].Sid ) )->SubAuthorityCount++;
                    *( RtlSubAuthoritySid( SidLookup[ i ].Sid,
                                           *( RtlSubAuthorityCountSid( RootDomainSid ) ) ) ) =
                                           SidLookup[ i ].Rid;
                    SidLookup[ i ].Valid = TRUE;

                    LeaveCriticalSection(&SddlSidLookupCritical);

                } else if ( DefaultToDomain ) {
                    //
                    // should default EA to DA and SA to domain relative
                    //
                } else {

                    if ( RootDomInited && RtlValidSid( (PSID)RootDomSidBuf ) &&
                         ( ( SidLookup[ i ].Valid == FALSE ) ||
                           ( SidLookup[ i ].Sid == NULL ) ) ) {

                        EnterCriticalSection(&SddlSidLookupCritical);

                        RtlCopyMemory( SidLookup[ i ].Sid, (PSID)RootDomSidBuf,
                                       RtlLengthSid( (PSID)RootDomSidBuf ) );
                        ( ( PISID )( SidLookup[ i ].Sid ) )->SubAuthorityCount++;
                        *( RtlSubAuthoritySid( SidLookup[ i ].Sid,
                                               *( RtlSubAuthorityCountSid( (PSID)RootDomSidBuf ) ) ) ) =
                                               SidLookup[ i ].Rid;
                        SidLookup[ i ].Valid = TRUE;

                        LeaveCriticalSection(&SddlSidLookupCritical);
                    }

                }

            }

            if ( SidLookup[ i ].Valid == FALSE ||
                 SidLookup[ i ].Sid == NULL ) {
                continue;
            }
        }

        if ( LookupSid ) {

            if ( RtlEqualSid( Sid, SidLookup[ i ].Sid ) ) {

                break;
            }

        } else {

            //
            // check for the current key first
            //
            if ( _wcsnicmp( String, SidLookup[i].Key, SidLookup[i].KeyLen ) == 0 ) {

                break;

            } else if ( InitRootDomain && DefaultToDomain &&
                        (RootDomainSid == NULL) ) {

                //
                // looking for EA/SA, not found them,
                // EA needs to default to DA, SA needs to default to domain relative
                //
                if ( _wcsnicmp( SDDL_DOMAIN_ADMINISTRATORS, SidLookup[i].Key, SidLookup[i].KeyLen ) == 0 ) {
                    DomainAdminIndex = i;
//                    break;
                }

            }
        }
    }


    if ( i < SidCount ) {

        MatchedEntry = &SidLookup[ i ];

    } else if ( InitRootDomain && DefaultToDomain &&
                (RootDomainSid == NULL) &&
                ( DomainAdminIndex < SidCount ) ) {

        if ( bIsSA ) {
            //
            // default to domain relative sid
            //

            if ( LookupSid ) {
                *pSASid = (PVOID)Sid;

            } else if ( SidLookup[ DomainAdminIndex ].Sid ) {
                //
                // allocate buffer for domain relative SA sid
                // which means it's only valid on the root domain
                //

                i = RtlLengthSid( SidLookup[ DomainAdminIndex ].Sid );

                *pSASid = (PVOID)LocalAlloc( LMEM_FIXED, i+1 );

                if ( *pSASid != NULL ) {

                    RtlCopyMemory( (PSID)(*pSASid), SidLookup[ DomainAdminIndex ].Sid, i );

                    // replace the DA rid with SA rid
                    *( RtlSubAuthoritySid( (PSID)(*pSASid),
                                           *( RtlSubAuthorityCountSid( SidLookup[ DomainAdminIndex ].Sid )) - 1) ) =
                                           DOMAIN_GROUP_RID_SCHEMA_ADMINS;
                }
            }

        } else {

            //
            // default to the domain admin account
            //

            MatchedEntry = &SidLookup[ DomainAdminIndex ];
        }
    }

    InitializeSidLookupTable(STRSD_REINITIALIZE_LEAVE);

    return( MatchedEntry );
}


DWORD
LocalGetSidForString(
    IN  PWSTR String,
    OUT PSID *SID,
    OUT PWSTR *End,
    OUT PBOOLEAN FreeSid,
    IN  PSID RootDomainSid OPTIONAL,
    IN  BOOLEAN DefaultToDomain
    )
/*++

Routine Description:

    This routine will determine which sid is an appropriate match for the
    given string, either as a sid moniker or as a string representation of a
    sid (ie: "DA or "S-1-0-0" )

    The returned sid must be free via a call to LocalFree if the *pFreeSid
    output parameter is TRUE.  If it's FALSE, no additional action needs to
    be taken


Arguments:

    String - The string to be converted

    Sid - Where the created SID is to be returned

    End - Where in the string we stopped processing

    FreeSid - Determines whether the returned SID needs to be freed via a
        call to LocalFree or not


Return Value:

    ERROR_SUCCESS - success

    ERROR_NON_MAPPED - An invalid format of the SID was given

--*/
{
    DWORD Err = ERROR_SUCCESS;
    PSTRSD_SID_LOOKUP MatchedEntry;
    PSID pSidSA=NULL;

    if ( String == NULL || SID == NULL || End == NULL || FreeSid == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Assume we'll return a well known sid
    //
    *FreeSid = FALSE;

//    if ( wcslen( String ) < 2 ) {
//  no need to do wcslen (expensive) because we know that String is not NULL
//  so just check for the first and second char
    if ( *String == L'\0' || *( String +1 ) == L'\0' ) {

        return( ERROR_NONE_MAPPED );
    }

    //
    // Set our end of string pointer
    //
    *End = String + 2;

    MatchedEntry = LookupSidInTable( String, NULL, RootDomainSid, DefaultToDomain,
                                     (PVOID *)&pSidSA);

    //
    // If we didn't find a match, try it as a sid string
    //
    if ( MatchedEntry == NULL ) {

        if ( pSidSA ) {
            //
            // this is schema admin lookup
            //
            *SID = pSidSA;
            *FreeSid = TRUE;

        } else {

            //
            // We assumed a known moniker, so we'll have to unset our end of string pointer.
            // Also, if it's a not a SID, the Convert routine will return the appropriate error.
            //
            *End -= 2;
            if ( LocalConvertStringSidToSid( String, SID, End) == FALSE ) {

                Err = GetLastError();
            }

            if ( Err == ERROR_SUCCESS && *SID != NULL ) {

                *FreeSid = TRUE;
            }
        }

    } else {

        //
        // If the entry that's been selected hasn't been initialized yet, do it now
        //
        *SID = MatchedEntry->Sid;
    }


    return(Err);
}


DWORD
LocalGetStringForSid(
    IN  PSID Sid,
    OUT PWSTR *String,
    IN  PSID RootDomainSid OPTIONAL
    )
/*++

Routine Description:

    This routine will determine which string represents a sid, either as a sid moniker or
    as a string representation of a sid (ie: "DA or "S-1-0-0" )

    The returned string must be free via a call to LocalFree


Arguments:

    Sid - Sid to be converted

    String - Where the mapped Sid is to be returned

Return Value:

    ERROR_SUCCESS - success

    ERROR_NON_MAPPED - An invalid format of the SID was given

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Err = ERROR_SUCCESS;
    PSTRSD_SID_LOOKUP MatchedEntry;
    PSID pSidSA=NULL;
    DWORD Len;

    if ( Sid == NULL || String == NULL ) {

        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Try to find a match in the lookup table
    //
    MatchedEntry = LookupSidInTable( NULL, Sid, RootDomainSid, FALSE, (PVOID *)&pSidSA );

    //
    // If a match was found, return it
    //
    if ( MatchedEntry || pSidSA ) {

        if ( MatchedEntry ) {
            Len = MatchedEntry->KeyLen;
        } else {
            Len = wcslen(SDDL_SCHEMA_ADMINISTRATORS);
        }

        *String = LocalAlloc( LMEM_FIXED, ( Len * sizeof( WCHAR ) ) + sizeof( WCHAR ) );
        if ( *String == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            if ( MatchedEntry ) {
                wcscpy( *String, MatchedEntry->Key );
            } else {
                wcscpy( *String, SDDL_SCHEMA_ADMINISTRATORS);
            }
        }

    } else {

        if ( ConvertSidToStringSidW( Sid, String ) == FALSE ) {

            Err = GetLastError();
        }

    }

    return(Err);
}

DWORD
LocalGetStringForControl(
    IN SECURITY_DESCRIPTOR_CONTROL ControlCode,
    IN ULONG LookupFlags,
    OUT PWSTR *ControlString
    )
{
    DWORD   i, ControlCount = sizeof( ControlLookup ) / sizeof( STRSD_KEY_LOOKUP );
    WCHAR Buffer[256];
    DWORD nOffset=0;


    if ( !ControlString ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *ControlString = NULL;

    for ( i = 0; i < ControlCount; i++ ) {

        //
        // If it doesn't match our lookup type, skip it.
        //
        if ( ( LookupFlags & ControlLookup[ i ].ValidityFlags ) != LookupFlags ) {

            continue;
        }

        if ( ControlCode & ControlLookup[ i ].Value ) {

            wcsncpy(Buffer+nOffset,
                    ControlLookup[ i ].Key,
                    ControlLookup[ i ].KeyLen );

            nOffset += ControlLookup[ i ].KeyLen;

        }
    }

    Buffer[nOffset] = L'\0';

    if ( nOffset ) {
        *ControlString = (PWSTR)LocalAlloc(0, (nOffset+1)*sizeof(WCHAR));

        if ( *ControlString ) {

            wcscpy(*ControlString, Buffer);

        } else {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return( ERROR_SUCCESS );
}


PSTRSD_KEY_LOOKUP
LookupAccessMaskInTable(
    IN PWSTR String, OPTIONAL
    IN ULONG AccessMask, OPTIONAL
    IN ULONG LookupFlags
    )
/*++

Routine Description:

    This routine will determine if the given access mask or string right exists in the lookup
    table.

    A pointer to the matching static lookup entry is returned.


Arguments:

    String - The string to be looked up

    AccessMask - The accessMask to be looked up.

    LookupFlags - Flags to use for lookup (Dacl or Sacl)

Return Value:

    Lookup table entry if found

    NULL if not found

--*/
{
    //
    // This is how the access mask is looked up.  Always have the multi-char
    // rights before the single char ones
    //
    static STRSD_KEY_LOOKUP  RightsLookup[] = {
        { SDDL_READ_PROPERTY, SDDL_LEN_TAG( SDDL_READ_PROPERTY ), ACTRL_DS_READ_PROP, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_WRITE_PROPERTY, SDDL_LEN_TAG( SDDL_WRITE_PROPERTY ), ACTRL_DS_WRITE_PROP, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_CREATE_CHILD, SDDL_LEN_TAG( SDDL_CREATE_CHILD ), ACTRL_DS_CREATE_CHILD, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_DELETE_CHILD, SDDL_LEN_TAG( SDDL_DELETE_CHILD ), ACTRL_DS_DELETE_CHILD, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_LIST_CHILDREN, SDDL_LEN_TAG( SDDL_LIST_CHILDREN ), ACTRL_DS_LIST, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_SELF_WRITE, SDDL_LEN_TAG( SDDL_SELF_WRITE ), ACTRL_DS_SELF, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_LIST_OBJECT, SDDL_LEN_TAG( SDDL_LIST_OBJECT ), ACTRL_DS_LIST_OBJECT, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_DELETE_TREE, SDDL_LEN_TAG( SDDL_DELETE_TREE ), ACTRL_DS_DELETE_TREE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_CONTROL_ACCESS, SDDL_LEN_TAG( SDDL_CONTROL_ACCESS ), ACTRL_DS_CONTROL_ACCESS, SDDL_VALID_DACL | SDDL_VALID_SACL },

        { SDDL_READ_CONTROL, SDDL_LEN_TAG( SDDL_READ_CONTROL ), READ_CONTROL, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_WRITE_DAC, SDDL_LEN_TAG( SDDL_WRITE_DAC ), WRITE_DAC, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_WRITE_OWNER, SDDL_LEN_TAG( SDDL_WRITE_OWNER ), WRITE_OWNER, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_STANDARD_DELETE, SDDL_LEN_TAG( SDDL_STANDARD_DELETE ), DELETE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_GENERIC_ALL, SDDL_LEN_TAG( SDDL_GENERIC_ALL ), GENERIC_ALL, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_GENERIC_READ, SDDL_LEN_TAG( SDDL_GENERIC_READ ), GENERIC_READ, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_GENERIC_WRITE, SDDL_LEN_TAG( SDDL_GENERIC_WRITE ), GENERIC_WRITE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_GENERIC_EXECUTE, SDDL_LEN_TAG( SDDL_GENERIC_EXECUTE ), GENERIC_EXECUTE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_FILE_ALL, SDDL_LEN_TAG( SDDL_FILE_ALL ), FILE_ALL_ACCESS, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_FILE_READ, SDDL_LEN_TAG( SDDL_FILE_READ ), FILE_GENERIC_READ, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_FILE_WRITE, SDDL_LEN_TAG( SDDL_FILE_WRITE ), FILE_GENERIC_WRITE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_FILE_EXECUTE, SDDL_LEN_TAG( SDDL_FILE_EXECUTE ), FILE_GENERIC_EXECUTE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_KEY_ALL, SDDL_LEN_TAG( SDDL_KEY_ALL ), KEY_ALL_ACCESS, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_KEY_READ, SDDL_LEN_TAG( SDDL_KEY_READ ), KEY_READ, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_KEY_WRITE, SDDL_LEN_TAG( SDDL_KEY_WRITE ), KEY_WRITE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_KEY_EXECUTE, SDDL_LEN_TAG( SDDL_KEY_EXECUTE ), KEY_EXECUTE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        };
    DWORD   i, RightsCount = sizeof(RightsLookup) / sizeof(STRSD_KEY_LOOKUP);
    PSTRSD_KEY_LOOKUP MatchedEntry = NULL;
    BOOLEAN LookupString = FALSE;

    if ( String ) {

        LookupString = TRUE;
    }

    for ( i = 0; i < RightsCount; i++ ) {

        //
        // If it doesn't match our lookup type, skip it.
        //
        if ( ( LookupFlags & RightsLookup[ i ].ValidityFlags ) != LookupFlags ) {

            continue;
        }

        if ( LookupString ) {

            if ( _wcsnicmp( String, RightsLookup[ i ].Key, RightsLookup[ i ].KeyLen ) == 0 ) {

                break;
            }

        } else {

            if ( AccessMask == RightsLookup[ i ].Value ) {

                break;
            }

        }

    }

    //
    // If a match was found, return it
    //
    if ( i < RightsCount ) {

        MatchedEntry = &RightsLookup[ i ];
    }


    return( MatchedEntry );

}


PSTRSD_KEY_LOOKUP
LookupAceTypeInTable(
    IN PWSTR String, OPTIONAL
    IN ULONG AceType, OPTIONAL
    IN ULONG LookupFlags
    )
/*++

Routine Description:

    This routine will determine if the given ace type or string type exists in the lookup
    table.

    A pointer to the matching static lookup entry is returned.


Arguments:

    String - The string to be looked up

    AceType - The ace type to be looked up.

    LookupFlags - Flags to use for lookup (Dacl or Sacl)

Return Value:

    Lookup table entry if found

    NULL if not found

--*/
{
    //
    // Lookup table
    //
    static STRSD_KEY_LOOKUP TypeLookup[] = {
        { SDDL_ACCESS_ALLOWED, SDDL_LEN_TAG( SDDL_ACCESS_ALLOWED ), ACCESS_ALLOWED_ACE_TYPE, SDDL_VALID_DACL },
        { SDDL_ACCESS_DENIED, SDDL_LEN_TAG( SDDL_ACCESS_DENIED ), ACCESS_DENIED_ACE_TYPE, SDDL_VALID_DACL },
        { SDDL_OBJECT_ACCESS_ALLOWED, SDDL_LEN_TAG( SDDL_OBJECT_ACCESS_ALLOWED ),
                                                                ACCESS_ALLOWED_OBJECT_ACE_TYPE, SDDL_VALID_DACL },
        { SDDL_OBJECT_ACCESS_DENIED, SDDL_LEN_TAG( SDDL_OBJECT_ACCESS_DENIED ),
                                                                ACCESS_DENIED_OBJECT_ACE_TYPE, SDDL_VALID_DACL },
        { SDDL_AUDIT, SDDL_LEN_TAG( SDDL_AUDIT ), SYSTEM_AUDIT_ACE_TYPE, SDDL_VALID_SACL },
        { SDDL_ALARM, SDDL_LEN_TAG( SDDL_ALARM ), SYSTEM_ALARM_ACE_TYPE, SDDL_VALID_SACL },
        { SDDL_OBJECT_AUDIT, SDDL_LEN_TAG( SDDL_OBJECT_AUDIT ), SYSTEM_AUDIT_OBJECT_ACE_TYPE, SDDL_VALID_SACL },
        { SDDL_OBJECT_ALARM, SDDL_LEN_TAG( SDDL_OBJECT_ALARM ), SYSTEM_ALARM_OBJECT_ACE_TYPE, SDDL_VALID_SACL }
        };
    DWORD   i, TypeCount = sizeof( TypeLookup ) / sizeof( STRSD_KEY_LOOKUP );
    PSTRSD_KEY_LOOKUP MatchedEntry = NULL;
    BOOLEAN LookupString = FALSE;

    if ( String ) {

        LookupString = TRUE;
    }

    for ( i = 0; i < TypeCount; i++ ) {

        //
        // If it doesn't match our lookup type, skip it.
        //
        if ( ( LookupFlags & TypeLookup[ i ].ValidityFlags ) != LookupFlags ) {

            continue;
        }

        if ( LookupString ) {

            if ( _wcsnicmp( String, TypeLookup[ i ].Key, TypeLookup[ i ].KeyLen ) == 0 ) {

                break;
            }

        } else {

            if ( AceType == TypeLookup[ i ].Value ) {

                break;
            }

        }

    }

    //
    // If a match was found, return it
    //
    if ( i < TypeCount ) {

        MatchedEntry = &TypeLookup[ i ];
    }


    return( MatchedEntry );
}



PSTRSD_KEY_LOOKUP
LookupAceFlagsInTable(
    IN PWSTR String, OPTIONAL
    IN ULONG AceFlags, OPTIONAL
    IN ULONG LookupFlags
    )
/*++

Routine Description:

    This routine will determine if the given ace flags or string flags exists in the lookup
    table.

    A pointer to the matching static lookup entry is returned.


Arguments:

    String - The string to be looked up

    AceFlags - The ace flags to be looked up.

    LookupFlags - Flags to use for lookup (Dacl or Sacl)

Return Value:

    Lookup table entry if found

    NULL if not found

--*/
{
    //
    // Lookup tables
    //
    static STRSD_KEY_LOOKUP  FlagLookup[] = {
        { SDDL_CONTAINER_INHERIT, SDDL_LEN_TAG( SDDL_CONTAINER_INHERIT ), CONTAINER_INHERIT_ACE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_OBJECT_INHERIT, SDDL_LEN_TAG( SDDL_OBJECT_INHERIT ), OBJECT_INHERIT_ACE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_NO_PROPAGATE, SDDL_LEN_TAG( SDDL_NO_PROPAGATE  ), NO_PROPAGATE_INHERIT_ACE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_INHERIT_ONLY, SDDL_LEN_TAG( SDDL_INHERIT_ONLY  ), INHERIT_ONLY_ACE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_INHERITED, SDDL_LEN_TAG( SDDL_INHERITED  ), INHERITED_ACE, SDDL_VALID_DACL | SDDL_VALID_SACL },
        { SDDL_AUDIT_SUCCESS, SDDL_LEN_TAG( SDDL_AUDIT_SUCCESS ), SUCCESSFUL_ACCESS_ACE_FLAG, SDDL_VALID_SACL },
        { SDDL_AUDIT_FAILURE, SDDL_LEN_TAG( SDDL_AUDIT_FAILURE ), FAILED_ACCESS_ACE_FLAG, SDDL_VALID_SACL }
        };
    DWORD   i, FlagCount = sizeof( FlagLookup ) / sizeof( STRSD_KEY_LOOKUP );
    PSTRSD_KEY_LOOKUP MatchedEntry = NULL;
    BOOLEAN LookupString = FALSE;

    if ( String ) {

        LookupString = TRUE;
    }


    for ( i = 0; i < FlagCount; i++ ) {

        //
        // If it doesn't match our lookup type, skip it.
        //
        if ( ( LookupFlags & FlagLookup[ i ].ValidityFlags ) != LookupFlags ) {

            continue;
        }

        if ( LookupString ) {

            if ( _wcsnicmp( String, FlagLookup[ i ].Key, FlagLookup[ i ].KeyLen ) == 0 ) {

                break;
            }

        } else {

            if ( AceFlags == FlagLookup[ i ].Value ) {

                break;
            }

        }

    }

    //
    // If a match was found, return it
    //
    if ( i < FlagCount ) {

        MatchedEntry = &FlagLookup[ i ];
    }


    return( MatchedEntry );
}


DWORD
LocalGetSDControlForString (
    IN  PWSTR ControlString,
    IN ULONG LookupFlags,
    OUT SECURITY_DESCRIPTOR_CONTROL *pControl,
    OUT PWSTR *End
    )
{

    DWORD   i, ControlCount = sizeof( ControlLookup ) / sizeof( STRSD_KEY_LOOKUP );
    PWSTR pCursor=ControlString;
    BOOL bFound;

    if ( !ControlString || !pControl || !End ) {

        return(ERROR_INVALID_PARAMETER);
    }

    *pControl = 0;

    while ( pCursor && *pCursor == L' ' ) {
        //
        // skip any blanks
        //
        pCursor++;
    }


    do {

        bFound = FALSE;

        for ( i = 0; i < ControlCount; i++ ) {

            //
            // If it doesn't match our lookup type, skip it.
            //
            if ( ( LookupFlags & ControlLookup[ i ].ValidityFlags ) != LookupFlags ) {

                continue;
            }

            if ( _wcsnicmp( pCursor,
                            ControlLookup[ i ].Key,
                            ControlLookup[ i ].KeyLen ) == 0 ) {

                *pControl |= ControlLookup[ i ].Value;

                pCursor += ControlLookup[ i ].KeyLen;

                while ( pCursor && *pCursor == L' ' ) {
                    //
                    // skip any blanks
                    //
                    pCursor++;
                }

                bFound = TRUE;

                break;  // break the for loop
            }
        }

    } while ( bFound );


    *End = pCursor;


    return( ERROR_SUCCESS );

}

DWORD
LocalGetAclForString(
    IN  PWSTR       AclString,
    IN  BOOLEAN     ConvertAsDacl,
    OUT PACL       *Acl,
    OUT PWSTR      *End,
    IN  PSID RootDomainSid OPTIONAL,
    IN  BOOLEAN DefaultToDomain
    )
/*++

Routine Description:

    This routine convert a string into an ACL.  The format of the aces is:

    Ace := ( Type; Flags; Rights; ObjGuid; IObjGuid; Sid;
    Type : = A | D | OA | OD        {Access, Deny, ObjectAccess, ObjectDeny}
    Flags := Flags Flag
    Flag : = CI | IO | NP | SA | FA {Container Inherit,Inherit Only, NoProp,
                                     SuccessAudit, FailAdit }
    Rights := Rights Right
    Right := DS_READ_PROPERTY |  blah blah
    Guid := String representation of a GUID (via RPC UuidToString)
    Sid := DA | PS | AO | PO | AU | S-* (Domain Admins, PersonalSelf, Acct Ops,
                                         PrinterOps, AuthenticatedUsers, or
                                         the string representation of a sid)
    The seperator is a ';'.

    The returned ACL must be free via a call to LocalFree


Arguments:

    AclString - The string to be converted

    ConvertAsDacl - Treat the input string as a dacl string

    ppSid - Where the created SID is to be returned

    End - Where in the string we stopped processing


Return Value:

    ERROR_SUCCESS indicates success
    ERROR_NOT_ENOUGH_MEMORY indicates a memory allocation for the ouput acl
                            failed
    ERROR_INVALID_PARAMETER The string does not represent an ACL


--*/
{
    DWORD Err = ERROR_SUCCESS;
    DWORD AclSize = 0, AclUsed = 0;
    ULONG Acls = 0, i, j;
    PWSTR Curr, MaskEnd;
    WCHAR ConvertGuidString[ STRING_GUID_LEN + 1];
    BOOLEAN FreeSid = FALSE;
    BOOL OpRes;
    PSTRSD_KEY_LOOKUP MatchedEntry;
    ULONG LookupFlags;
    PSID  SidPtr = NULL;


    if ( NULL == AclString || NULL == Acl || NULL == End ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( ConvertAsDacl ) {

        LookupFlags = SDDL_VALID_DACL;

    } else {

        LookupFlags = SDDL_VALID_SACL;
    }

    //
    // First, we'll have to go through and count the number of entries that
    // we have.  We'll do the by computing the length of this ACL (which is
    // delimited by either the end of the list or a ':' that seperates a key
    // from a value
    //
    *End = wcschr( AclString, SDDL_DELIMINATORC );

    if ( *End == AclString ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( *End == NULL ) {

        *End = AclString + wcslen( AclString );

    } else {

        ( *End )--;
    }

    //
    // Now, do the count
    //
    Curr = AclString;

    OpRes = 0;
//    while ( Curr != *End ) {
    while ( Curr < *End ) {

        if ( *Curr == SDDL_SEPERATORC ) {

            Acls++;

        } else if ( *Curr != L' ' ) {
            OpRes = 1;
        }

        Curr++;
    }

    //
    // Now, we've counted the total number of seperators.  Make sure we
    // have the right number.  (There is 5 seperators per ace)
    //
    if ( Acls % 5 == 0 ) {

        if ( Acls == 0 && OpRes ) {
            //
            // gabbage chars in between
            //
            Err = ERROR_INVALID_PARAMETER;

        } else {
            Acls = Acls / 5;
        }

    } else {

        Err = ERROR_INVALID_PARAMETER;
    }

    if (  Err == ERROR_SUCCESS && Acls == 0 ) {

        *Acl = LocalAlloc( LMEM_FIXED, sizeof( ACL ) );

        if ( *Acl == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            ( *Acl )->AclRevision = ACL_REVISION;
            ( *Acl )->Sbz1 = ( BYTE )0;
            ( *Acl )->AclSize = ( USHORT )sizeof( ACL ) ;
            ( *Acl )->AceCount = 0;
            ( *Acl )->Sbz2 = ( USHORT )0;

        }

        return( Err );
    }

    //
    // Ok now do the allocation.  We'll do a sort of worst case initial
    // allocation.  This saves us from having to process everything twice
    // (once to size, once to build).  If we determine later that we have
    // an acl that is not big enough, we allocate additional space.  The only
    // time that this reallocation should happen is if the input string
    // contains a lot of explicit SIDs.  Otherwise, the chosen buffer size
    // should be pretty close to the proper size
    //
    if ( Err == ERROR_SUCCESS ) {

        AclSize = sizeof( ACL ) + ( Acls * ( sizeof( ACCESS_ALLOWED_OBJECT_ACE ) +
                                            sizeof( SID ) + ( 6 * sizeof( ULONG ) ) ) );
        *Acl = ( PACL )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, AclSize );

        if ( *Acl == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            AclUsed = sizeof( ACL );

            //
            // We'll start initializing it...
            //
            ( *Acl )->AclRevision = ACL_REVISION;
            ( *Acl )->Sbz1        = ( BYTE )0;
            ( *Acl )->AclSize     = ( USHORT )AclSize;
            ( *Acl )->AceCount    = 0;
            ( *Acl )->Sbz2 = ( USHORT )0;

            //
            // Ok, now we'll go through and start building them all
            //
            Curr = AclString;

            for( i = 0; i < Acls; i++ ) {

                //
                // First, get the type..
                //
                UCHAR Type;
                UCHAR Flags = 0;
                USHORT Size;
                ACCESS_MASK Mask = 0;
                GUID *ObjId = NULL, ObjGuid;
                GUID *IObjId = NULL, IObjGuid;
                PWSTR  Next;
                DWORD AceSize = 0;

                //
                // skip any space before (
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }
                 //
                 // Skip any parens that may exist in the ace list
                 //
                if ( *Curr == SDDL_ACE_BEGINC ) {

                    Curr++;
                }
                 //
                 // skip any space after (
                 //
                 while(*Curr == L' ' ) {
                     Curr++;
                 }

                MatchedEntry = LookupAceTypeInTable( Curr, 0, LookupFlags );

                if ( MatchedEntry ) {

                    Type = ( UCHAR )MatchedEntry->Value;
                    Curr += MatchedEntry->KeyLen + 1;

                } else {

                    //
                    // Found an invalid type
                    //
                    Err = ERROR_INVALID_DATATYPE;
                    break;
                }

                //
                // If we have any object aces, set the acl revision to REVISION_DS
                //
                if ( Type >= ACCESS_MIN_MS_OBJECT_ACE_TYPE && Type <= ACCESS_MAX_MS_OBJECT_ACE_TYPE ) {

                    ( *Acl )->AclRevision = ACL_REVISION_DS;
                }

                //
                // skip any space before ;
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // Next, get the flags...
                //
                while ( Curr != *End ) {

                    if ( *Curr == SDDL_SEPERATORC ) {

                        Curr++;
                        break;
                    }

                    //
                    // Skip any blanks
                    //
                    while ( *Curr == L' ' ) {

                        Curr++;
                    }

                    MatchedEntry = LookupAceFlagsInTable( Curr, 0, LookupFlags );

                    if ( MatchedEntry ) {

                        Flags |= ( UCHAR )MatchedEntry->Value;
                        Curr += MatchedEntry->KeyLen;

                    } else {
                        //
                        // Found an invalid flag
                        //
                        Err = ERROR_INVALID_FLAGS;
                        break;
                    }
                }

                if ( Err != ERROR_SUCCESS ) {

                    break;
                }

                //
                // skip any space after ;
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // Now, get the access mask
                //
                while( TRUE ) {

                    if ( *Curr == SDDL_SEPERATORC ) {

                        Curr++;
                        break;
                    }

                    //
                    // Skip any blanks
                    //
                    while ( *Curr == L' ' ) {

                        Curr++;
                    }

                    MatchedEntry = LookupAccessMaskInTable( Curr, 0, LookupFlags );

                    if ( MatchedEntry ) {

                        Mask |= MatchedEntry->Value;
                        Curr += MatchedEntry->KeyLen;

                    } else {

                        //
                        // If the rights couldn't be looked up, see if it's a converted mask
                        //

                        Mask = wcstoul( Curr, &MaskEnd, 0 );

                        if ( MaskEnd != Curr ) {

                            Curr = MaskEnd;

                        } else {
                            //
                            // Found an invalid right
                            //
                            Err = ERROR_INVALID_ACL;
                            break;
                        }
                    }
                }

                if ( Err != ERROR_SUCCESS ) {

                    break;
                }

                //
                // If that worked, we'll get the ids
                //
                for ( j = 0; j < 2; j++ ) {

                    //
                    // skip any space before ;
                    //
                    while(*Curr == L' ' ) {
                        Curr++;
                    }

                    if ( *Curr != SDDL_SEPERATORC ) {

                        wcsncpy( ConvertGuidString, Curr, STRING_GUID_LEN );
                        ConvertGuidString[ STRING_GUID_LEN ] = UNICODE_NULL;

                        if ( j == 0 ) {


                            if ( UuidFromStringW( ConvertGuidString, &ObjGuid ) == RPC_S_OK ) {

                                ObjId = &ObjGuid;

                            } else {

                                Err = RPC_S_INVALID_STRING_UUID;
                                break;
                            }

                        } else {

                            if ( UuidFromStringW( ConvertGuidString, &IObjGuid ) == RPC_S_OK ) {

                                IObjId = &IObjGuid;

                            } else {

                                Err = RPC_S_INVALID_STRING_UUID;
                                break;
                            }
                        }

                        // success
                        Curr += STRING_GUID_LEN;
                        if ( *Curr != SDDL_SEPERATORC &&
                             *Curr != L' ' ) {

                            Err = RPC_S_INVALID_STRING_UUID;
                            break;
                        }

                    }

                    Curr++;
                }

                if ( Err != ERROR_SUCCESS ) {

                    break;
                }

                //
                // skip any space before ;
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // Finally, the SID
                //
                if ( ERROR_SUCCESS == Err ) {

                    PWSTR   End;
                    Err = LocalGetSidForString( Curr, &SidPtr, &End, &FreeSid,
                                                RootDomainSid, DefaultToDomain );

                    if ( Err == ERROR_SUCCESS ) {

                        if ( End == NULL ) {
                            Err = ERROR_INVALID_ACL;
                        } else {

                            while(*End == L' ' ) {
                                End++;
                            }
                            //
                            // a ace must be terminated by ')'
                            //
                            if ( *End != SDDL_ACE_ENDC ) {
                                Err = ERROR_INVALID_ACL;

                            } else {

                                Curr = End + 1;

                                if ( !SidPtr ) {
                                    Err = ERROR_INVALID_ACL;
                                }
                            }
                        }

                    }
                }

                //
                // Quit on an error
                //
                if ( Err != ERROR_SUCCESS ) {

                    break;
                }

                //
                // Now, we'll create the ace, and add it...
                //

                //
                // First, make sure we have the room for it
                //
                switch ( Type ) {
                case ACCESS_ALLOWED_ACE_TYPE:
                case ACCESS_DENIED_ACE_TYPE:
                case SYSTEM_AUDIT_ACE_TYPE:
                case SYSTEM_ALARM_ACE_TYPE:

                    AceSize = sizeof( ACCESS_ALLOWED_ACE );
                    break;

                case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                case ACCESS_DENIED_OBJECT_ACE_TYPE:
                case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                case SYSTEM_ALARM_OBJECT_ACE_TYPE:
                    AceSize = sizeof( KNOWN_OBJECT_ACE );

                    if ( ObjId ) {

                        AceSize += sizeof ( GUID );
                    }

                    if ( IObjId ) {

                        AceSize += sizeof ( GUID );
                    }
                    break;

                default:
                    Err = ERROR_INVALID_ACL;
                    break;

                }

                if ( Err != ERROR_SUCCESS ) {

                    break;
                }

                AceSize += RtlLengthSid( SidPtr )  - sizeof( ULONG );

                if (AceSize + AclUsed > AclSize)
                {
                    //
                    // We'll have to reallocate, since our buffer isn't
                    // big enough...
                    //
                    PACL  NewAcl;
                    DWORD NewSize = AclSize + ( ( Acls - i ) * AceSize );

                    NewAcl = ( PACL )LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                 NewSize );
                    if ( NewAcl == NULL ) {

                        LocalFree( *Acl );
                        *Acl = NULL;

                        if ( FreeSid == TRUE ) {

                            LocalFree( SidPtr );
                            SidPtr = NULL;

                            FreeSid = FALSE;
                        }

                        Err = ERROR_NOT_ENOUGH_MEMORY;
                        break;

                    } else {

                        memcpy( NewAcl, *Acl, AclSize );
                        NewAcl->AclSize = ( USHORT )NewSize;

                        LocalFree( *Acl );
                        *Acl = NewAcl;

                        AclSize = NewSize;
                    }

                }

                AclUsed += AceSize;

                SetLastError( ERROR_SUCCESS );

                switch (Type)
                {
                case SYSTEM_AUDIT_ACE_TYPE:
                    OpRes = AddAuditAccessAceEx( *Acl,
                                                 ACL_REVISION,
                                                 Flags &
                                                     ~(SUCCESSFUL_ACCESS_ACE_FLAG |
                                                       FAILED_ACCESS_ACE_FLAG),
                                                 Mask,
                                                 SidPtr,
                                                 Flags & SUCCESSFUL_ACCESS_ACE_FLAG,
                                                 Flags & FAILED_ACCESS_ACE_FLAG );
                    break;

                case ACCESS_ALLOWED_ACE_TYPE:
                    OpRes = AddAccessAllowedAceEx( *Acl,
                                                   ACL_REVISION,
                                                   Flags,
                                                   Mask,
                                                   SidPtr );

                    break;

                case ACCESS_DENIED_ACE_TYPE:
                    OpRes = AddAccessDeniedAceEx( *Acl,
                                                  ACL_REVISION,
                                                   Flags,
                                                  Mask,
                                                  SidPtr );

                    break;

                case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                    OpRes = AddAuditAccessObjectAce( *Acl,
                                                     ACL_REVISION_DS,
                                                     Flags,
                                                     Mask,
                                                     ObjId,
                                                     IObjId,
                                                     SidPtr,
                                                     Flags & SUCCESSFUL_ACCESS_ACE_FLAG,
                                                     Flags & FAILED_ACCESS_ACE_FLAG );
                    break;

                case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                    OpRes = AddAccessAllowedObjectAce( *Acl,
                                                       ACL_REVISION_DS,
                                                       Flags,
                                                       Mask,
                                                       ObjId,
                                                       IObjId,
                                                        SidPtr );
                    break;

                case ACCESS_DENIED_OBJECT_ACE_TYPE:
                    OpRes = AddAccessDeniedObjectAce( *Acl,
                                                      ACL_REVISION_DS,
                                                      Flags,
                                                      Mask,
                                                      ObjId,
                                                      IObjId,
                                                      SidPtr );

                    break;

                default:
                    SetLastError( ERROR_INVALID_DATATYPE );
                    OpRes = FALSE;
                    break;

                }

                if ( OpRes == FALSE ) {

                    Err = GetLastError();
                    break;
                }

                //
                // Clean up whatever memory we have to
                //
                if ( FreeSid == TRUE ) {

                    LocalFree( SidPtr );
                }

                SidPtr = NULL;

                if ( *Curr == SDDL_ACE_BEGINC ) {

                    Curr++;
                }

            }

            //
            // If something didn't work, clean up
            //
            if ( Err != ERROR_SUCCESS ) {

                LocalFree( *Acl );
                *Acl = NULL;

            } else {

                //
                // Set a more realistic acl size
                //
                ( *Acl )->AclSize = ( USHORT )AclUsed;
            }

            //
            // free any SIDs buffer used
            //
            if ( FreeSid && SidPtr ) {
                LocalFree(SidPtr);
                SidPtr = NULL;
            }

            FreeSid = FALSE;
        }
    }

    return(Err);
}


DWORD
LocalConvertAclToString(
    IN PACL Acl,
    IN BOOLEAN AclPresent,
    IN BOOLEAN ConvertAsDacl,
    OUT PWSTR *AclString,
    OUT PDWORD AclStringSize,
    IN PSID RootDomainSid OPTIONAL
    )
/*++

Routine Description:

    This routine convert an acl into a string.  The format of the aces is:

    Ace := ( Type; Flags; Rights; ObjGuid; IObjGuid; Sid;
    Type : = A | D | OA | OD        {Access, Deny, ObjectAccess, ObjectDeny}
    Flags := Flags Flag
    Flag : = CI | IO | NP | SA | FA {Container Inherit,Inherit Only, NoProp,
                                     SuccessAudit, FailAdit }
    Rights := Rights Right
    Right := DS_READ_PROPERTY |  blah blah
    Guid := String representation of a GUID (via RPC UuidToString)
    Sid := DA | PS | AO | PO | AU | S-* (Domain Admins, PersonalSelf, Acct Ops,
                                         PrinterOps, AuthenticatedUsers, or
                                         the string representation of a sid)
    The seperator is a ';'.

    The returned string must be free via a call to LocalFree


Arguments:

    Acl - The acl to be converted

    AclPresent - if TRUE, the acl is present, even if NULL

    AclString - Where the created acl string is to be returned

    ConvertAsDacl - Convert the given acl as the DACL, not the SACL

    AclStringLen - The size of the allocated string is returned here


Return Value:

    ERROR_SUCCESS - success

    ERROR_NOT_ENOUGH_MEMORY indicates a memory allocation for the ouput acl
                            failed

    ERROR_INVALID_PARAMETER The string does not represent an ACL

    ERROR_INVALID_ACL - An unexpected access mask was encountered or a NULL acl was encountered

--*/
{
    DWORD   Err = ERROR_SUCCESS;
    DWORD   AclSize = 0, MaskSize;
    PACE_HEADER AceHeader;
    ULONG i, j;
    PWSTR *SidStrings = NULL, Curr, Guid;
    BOOLEAN *SidFrees = NULL;
    UINT *pMaskAsString = NULL;
    PSTRSD_KEY_LOOKUP MatchedEntry;
    PSTRSD_SID_LOOKUP MatchedSidEntry;
    PKNOWN_ACE KnownAce;
    PKNOWN_OBJECT_ACE KnownObjectAce;
    ACCESS_MASK AccessMask;
    PSID Sid, pSidSA=NULL;
    GUID *Obj, *Inherit;
    ULONG LookupFlags;
    ULONG AceFlags[ ] = {
        OBJECT_INHERIT_ACE,
        CONTAINER_INHERIT_ACE,
        NO_PROPAGATE_INHERIT_ACE,
        INHERIT_ONLY_ACE,
        INHERITED_ACE,
        SUCCESSFUL_ACCESS_ACE_FLAG,
        FAILED_ACCESS_ACE_FLAG
        };


    if ( AclString == NULL || AclStringSize == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( !AclPresent ) {

        *AclString = NULL;
        *AclStringSize = 0;
        return( ERROR_SUCCESS );

    }

    //
    // Don't allow NULL acls
    //
    if ( Acl == NULL ) {

        return( ERROR_INVALID_ACL );
    }

    //
    // If the ace count is 0, then it's an empty acl, and we don't handle those...
    //
    if ( Acl->AceCount == 0 ) {

        *AclString = NULL;
        *AclStringSize = 0;
        return( ERROR_SUCCESS );

    }

    if ( ConvertAsDacl ) {

        LookupFlags = SDDL_VALID_DACL;

    } else {

        LookupFlags = SDDL_VALID_SACL;
    }

    //
    // Allocate buffers to hold sids that are looked up
    //
    SidStrings = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Acl->AceCount * sizeof( PWSTR ) );

    if ( SidStrings == NULL ) {

        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    SidFrees = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Acl->AceCount * sizeof( BOOLEAN ) );

    if ( SidFrees == NULL ) {

        LocalFree( SidStrings );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    pMaskAsString = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Acl->AceCount * sizeof( UINT ) );

    if ( pMaskAsString == NULL ) {

        LocalFree( SidStrings );
        LocalFree( SidFrees );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    AceHeader = ( PACE_HEADER )FirstAce( Acl );

    //
    // Size the acl, so we know how big a buffer to allocate
    //
    for(i = 0; i < Acl->AceCount && Err == ERROR_SUCCESS;
        i++, AceHeader = ( PACE_HEADER )NextAce( AceHeader ) ) {

        AclSize += sizeof( WCHAR );
        //
        // First, the type
        //
        MatchedEntry = LookupAceTypeInTable( NULL, ( ULONG )AceHeader->AceType, LookupFlags );

        if ( MatchedEntry ) {

            AclSize += SDDL_SIZE_TAG( MatchedEntry->Key ) + SDDL_SIZE_SEP( SDDL_SEPERATORC );

        } else {

            //
            // Found an invalid type
            //
            Err = ERROR_INVALID_ACL;
            break;
        }

        //
        // Next, process the ace flags
        //
        for ( j = 0; j < sizeof( AceFlags ) / sizeof( ULONG ); j++ ) {

            if ( ( ULONG )AceHeader->AceFlags & ( AceFlags[ j ] ) ) {

                MatchedEntry = LookupAceFlagsInTable( 0, AceFlags[ j ], LookupFlags );
                if ( MatchedEntry ) {

                    AclSize += SDDL_SIZE_TAG( MatchedEntry->Key );
                }
            }
        }

        if ( Err != ERROR_SUCCESS ) {

            break;

        } else {

            AclSize += SDDL_SIZE_SEP( SDDL_SEPERATORC );
        }

        //
        // Next, the rights and optionally the guids.  This gets done on a per ace type basis
        //
        switch ( AceHeader->AceType ) {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        case SYSTEM_ALARM_ACE_TYPE:
            KnownAce = ( PKNOWN_ACE )AceHeader;
            AccessMask = KnownAce->Mask;
            Sid = ( PSID )&KnownAce->SidStart;

            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:
            KnownObjectAce = ( PKNOWN_OBJECT_ACE )AceHeader;
            AccessMask = KnownObjectAce->Mask;
            Sid = RtlObjectAceSid( AceHeader );

            //
            // Deal with potential guids
            //
            if ( RtlObjectAceObjectType( AceHeader ) ) {

                AclSize += STRING_GUID_SIZE;
            }

            if ( RtlObjectAceInheritedObjectType( AceHeader ) ) {

                AclSize += STRING_GUID_SIZE;
            }

            break;

        default:
            Err = ERROR_INVALID_ACL;
            break;

        }

        //
        // Size the rights
        //
        if ( Err == ERROR_SUCCESS ) {

            MaskSize = 0;
            pMaskAsString[i] = 0;

            //
            // lookup for the exact value first
            //
            MatchedEntry = LookupAccessMaskInTable( 0, AccessMask, LookupFlags );

            if ( MatchedEntry ) {

                pMaskAsString[i] = 1;
                MaskSize += SDDL_SIZE_TAG( MatchedEntry->Key );

            } else {
                //
                // look for each bit
                //
                for ( j = 0; j < 32; j++ ) {

                    if ( AccessMask & ( 1 << j ) ) {

                        MatchedEntry = LookupAccessMaskInTable( 0, AccessMask & ( 1 << j ), LookupFlags );

                        if ( MatchedEntry ) {

                            MaskSize += SDDL_SIZE_TAG( MatchedEntry->Key );

                        } else {

                            //
                            // Found an invalid type.  We'll convert the whole thing to a string
                            //
                            pMaskAsString[i] = 2;
                            MaskSize = 10 * sizeof( WCHAR );
                            break;
                        }
                    }
                }
            }

            if ( Err != ERROR_SUCCESS ) {

                break;

            } else {

                AclSize += MaskSize;
                AclSize += SDDL_SIZE_SEP( SDDL_SEPERATORC );
            }
        }

        if ( Err != ERROR_SUCCESS ) {

            break;
        }

        //
        // Add in space for the guid seperators
        //
        AclSize += 2 * SDDL_SIZE_SEP( SDDL_SEPERATORC );

        //
        // Finally, lookup the sids
        //
        MatchedSidEntry = LookupSidInTable( NULL, Sid, RootDomainSid, FALSE, &pSidSA );

        //
        // If we didn't find a match, try it as a sid string
        //
        if ( MatchedSidEntry == NULL ) {

            if ( pSidSA ) {
                //
                // when sid lookup finds the sid of SA, pSidSA is assigned to Sid
                // so it doesn't need to be freed.
                //

                SidStrings[ i ] = LocalAlloc( LMEM_FIXED, (wcslen(SDDL_SCHEMA_ADMINISTRATORS)+1)*sizeof(TCHAR) );

                if ( SidStrings[ i ] == NULL ) {

                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    break;

                } else {
                    wcscpy( SidStrings[ i ], SDDL_SCHEMA_ADMINISTRATORS );

                    SidFrees[ i ] = TRUE;
                }

            } else {

                if ( ConvertSidToStringSidW( Sid, &SidStrings[ i ] ) == FALSE ) {

                    Err = GetLastError();
                    break;

                } else {

                    SidFrees[ i ] = TRUE;
                }
            }

        } else {

            //
            // If the entry that's been selected hasn't been initialized yet, do it now
            //
            SidStrings[ i ] = MatchedSidEntry->Key;
        }
        AclSize += SDDL_SIZE_TAG( SidStrings[ i ] );


        AclSize += sizeof( WCHAR );  // Closing paren
        AclSize += sizeof( WCHAR );  // Trailing NULL
    }

    //
    // If all of that worked, allocate the return buffer, and build the acl string
    //
    if ( AclSize == 0 ) {
        Err = ERROR_INVALID_ACL;
    }

    if ( Err == ERROR_SUCCESS ) {

        if ( AclSize % 2 != 0 ) {
            AclSize++;
        }

        *AclString = LocalAlloc( LMEM_FIXED, AclSize );

        if ( *AclString == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Build the acl
    //
    if ( Err == ERROR_SUCCESS ) {

        Curr = *AclString;

        AceHeader = ( PACE_HEADER )FirstAce( Acl );

        //
        // Size the acl, so we know how big a buffer to allocate
        //
        for(i = 0; i < Acl->AceCount && Err == ERROR_SUCCESS;
            i++, AceHeader = ( PACE_HEADER )NextAce( AceHeader ) ) {

            //
            // "("
            //
            *Curr = SDDL_ACE_BEGINC;
            Curr++;

            //
            // By the time we get down here, we've ensured that we can lookup all the values,
            // so there is no need to check for failure
            //

            //
            // First, the type, must find it
            // "T;"
            //
            MatchedEntry = LookupAceTypeInTable( NULL, ( ULONG )AceHeader->AceType, LookupFlags );
            if ( MatchedEntry ) {
                wcscpy( Curr, MatchedEntry->Key );
                Curr += MatchedEntry->KeyLen;
            }
            *Curr = SDDL_SEPERATORC;
            Curr++;

            //
            // Next, process the ace flags
            // "CIIO;"
            //
            for ( j = 0; j < sizeof( AceFlags ) / sizeof( ULONG ); j++ ) {

                if ( ( ULONG )AceHeader->AceFlags & ( AceFlags[ j ] ) ) {

                    MatchedEntry = LookupAceFlagsInTable( 0, AceFlags[ j ], LookupFlags );

                    if ( MatchedEntry ) {

                        wcscpy( Curr, MatchedEntry->Key );
                        Curr+= MatchedEntry->KeyLen;

                    }

                }
            }
            *Curr = SDDL_SEPERATORC;
            Curr++;

            //
            // Next, the rights and optionally the guids.  This gets done on a per ace type basis
            //
            Obj = NULL;
            Inherit = NULL;

            switch ( AceHeader->AceType ) {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
            case SYSTEM_ALARM_ACE_TYPE:
                KnownAce = ( PKNOWN_ACE )AceHeader;
                AccessMask = KnownAce->Mask;
                Sid = ( PSID )&KnownAce->SidStart;

                break;

            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            case SYSTEM_ALARM_OBJECT_ACE_TYPE:
                KnownObjectAce = ( PKNOWN_OBJECT_ACE )AceHeader;
                AccessMask = KnownObjectAce->Mask;
                Sid = RtlObjectAceSid( AceHeader );

                //
                // Deal with potential guids
                //
                Inherit = RtlObjectAceInheritedObjectType( AceHeader );
                Obj = RtlObjectAceObjectType( AceHeader );
                break;
            }

            //
            // Add the rights
            //
            if ( pMaskAsString[i] == 2 ) {

                wcscpy( Curr, L"0x");
                Curr += 2;
                _ultow( AccessMask, Curr, 16 );
                Curr += wcslen( Curr );

            } else if ( pMaskAsString[i] == 1 ) {

                //
                // lookup for the entire value first
                //
                MatchedEntry = LookupAccessMaskInTable( 0, AccessMask, LookupFlags );

                if ( MatchedEntry ) {

                    wcscpy( Curr, MatchedEntry->Key );
                    Curr += MatchedEntry->KeyLen;
                }

            } else { // 0

                for ( j = 0; j < 32; j++ ) {

                    if ( AccessMask & (1 << j) ) {

                        MatchedEntry = LookupAccessMaskInTable( 0, AccessMask & ( 1 << j ), LookupFlags );

                        if ( MatchedEntry ) {

                            wcscpy( Curr, MatchedEntry->Key );
                            Curr += MatchedEntry->KeyLen;

                        } // else shouldn't happen but if it happens
                          // it is too late to convert it into 0x format
                          // because the buffer is already allocated with smaller size.

                    }
                }
            }

            *Curr = SDDL_SEPERATORC;
            Curr++;


            //
            // Optional object guid
            //
            if ( Obj ) {

                Err = UuidToStringW( Obj, &Guid );

                if ( Err != ERROR_SUCCESS ) {

                    break;
                }

                wcscpy( Curr, Guid );
                Curr += wcslen( Guid );
                RpcStringFreeW( &Guid );

            }
            *Curr = SDDL_SEPERATORC;
            Curr++;

            if ( Inherit ) {

                Err = UuidToStringW( Inherit, &Guid );

                if ( Err != ERROR_SUCCESS ) {

                    break;
                }

                wcscpy( Curr, Guid );
                Curr += wcslen( Guid );
                RpcStringFreeW( &Guid );

            }
            *Curr = SDDL_SEPERATORC;
            Curr++;

            //
            // Finally, the sids
            //
            wcscpy( Curr, SidStrings[ i ] );
            Curr += wcslen( SidStrings[ i ] );

            *Curr = SDDL_ACE_ENDC;
            Curr++;
            *Curr = UNICODE_NULL;

        }
    }

    //
    // Free any allocated memory
    // jinhuang: should always free the allocated buffer
    //

//    if ( Err != ERROR_SUCCESS && SidStrings ) {

        for ( j = 0; j < Acl->AceCount; j++ ) {

            if ( SidFrees[ j ] ) {

                LocalFree( SidStrings[ j ] );
            }
        }
//    }

    LocalFree( SidStrings );
    LocalFree( SidFrees );
    LocalFree( pMaskAsString );

    if ( Err != ERROR_SUCCESS ) {

        LocalFree(*AclString);
        *AclString = NULL;
        *AclStringSize = 0;

    } else {

        *AclStringSize = AclSize;

    }

    return( Err );
}


DWORD
LocalConvertSDToStringSD_Rev1(
    IN  PSID RootDomainSid OPTIONAL,
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  SECURITY_INFORMATION  SecurityInformation,
    OUT LPWSTR  *StringSecurityDescriptor,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    )
/*++

Routine Description:

    This routine converts a security descriptor into a string version persuant to SDDL definition

Arguments:

    SecurityDescriptor - Security Descriptor to be converted.

    SecurityInformation - the security information of which component to be converted

    StringSecurityDescriptor - Where the converted SD is returned.  Buffer is allocated via
        LocalAlloc and should be free via LocalFree.

    StringSecurityDescriptorLen - optional length of the converted SD buffer.

Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

--*/
{
    DWORD Err = ERROR_SUCCESS;
    NTSTATUS Status=STATUS_SUCCESS;
    DWORD ReturnBufferSize = 0, AclSize;
    PSID Owner = NULL, Group = NULL;
    PACL Dacl = NULL, Sacl = NULL;
    BOOLEAN Defaulted, SaclPresent=FALSE, DaclPresent=FALSE;
    PWSTR OwnerString = NULL, GroupString = NULL, SaclString = NULL, DaclString = NULL;
    SECURITY_DESCRIPTOR_CONTROL ControlCode;
    ULONG Revision;
    PWSTR DaclControl=NULL, SaclControl=NULL;

    if ( SecurityDescriptor == NULL || StringSecurityDescriptor == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Get the relevant security descriptor parts
    // based on the SecurityInforamtion input parameter
    //
    if ( SecurityInformation & OWNER_SECURITY_INFORMATION ) {

        Status = RtlGetOwnerSecurityDescriptor( SecurityDescriptor, &Owner, &Defaulted );
    }

    if ( NT_SUCCESS( Status ) &&
         SecurityInformation & GROUP_SECURITY_INFORMATION ) {

        Status = RtlGetGroupSecurityDescriptor( SecurityDescriptor, &Group, &Defaulted );

    }

    if ( NT_SUCCESS( Status ) &&
         SecurityInformation & DACL_SECURITY_INFORMATION ) {

        Status = RtlGetDaclSecurityDescriptor( SecurityDescriptor, &DaclPresent, &Dacl, &Defaulted );
    }

    if ( NT_SUCCESS( Status ) &&
         SecurityInformation & SACL_SECURITY_INFORMATION ) {

        Status = RtlGetSaclSecurityDescriptor( SecurityDescriptor, &SaclPresent, &Sacl, &Defaulted );
    }

    if ( NT_SUCCESS( Status ) ) {

        Status = RtlGetControlSecurityDescriptor ( SecurityDescriptor, &ControlCode, &Revision);
    }

    if ( !NT_SUCCESS( Status ) ) {

        Err = RtlNtStatusToDosError( Status );
        return( Err );
    }

    //
    // make sure the SidLookup table is reinitialized
    //
    InitializeSidLookupTable(STRSD_REINITIALIZE_ENTER);

    //
    // Convert the owner and group, if they exist
    //
    if ( Owner ) {

        Err = LocalGetStringForSid( Owner,
                                    &OwnerString,
                                    RootDomainSid
                                  );

    }

    if ( Err == ERROR_SUCCESS && Group ) {

        Err = LocalGetStringForSid( Group,
                                    &GroupString,
                                    RootDomainSid );
    }

    //
    // JINHUANG 3/10/98
    // get DACL control string
    //
    if ( Err == ERROR_SUCCESS && ControlCode ) {

        Err = LocalGetStringForControl(ControlCode, SDDL_VALID_DACL, &DaclControl);
    }

    //
    // get SACL control string
    //
    if ( Err == ERROR_SUCCESS && ControlCode ) {

        Err = LocalGetStringForControl(ControlCode, SDDL_VALID_SACL, &SaclControl);
    }

    //
    // SACL first because the size of DACL is needed later
    //
    if ( Err == ERROR_SUCCESS && SaclPresent ) {


        Err = LocalConvertAclToString( Sacl,
                                       SaclPresent,
                                       FALSE,
                                       &SaclString,
                                       &AclSize,
                                       RootDomainSid );
        if ( Err == ERROR_SUCCESS ) {

            ReturnBufferSize += AclSize;
        }
    }

    //
    // Then, the DACL
    //
    if ( Err == ERROR_SUCCESS && DaclPresent ) {

        Err = LocalConvertAclToString( Dacl,
                                       DaclPresent,
                                       TRUE,
                                       &DaclString,
                                       &AclSize,
                                       RootDomainSid );

        if ( Err == ERROR_SUCCESS ) {

            ReturnBufferSize += AclSize;
        }
    }

    //
    // Now, if all of that worked, allocate and build the return string
    //
    if ( Err == ERROR_SUCCESS ) {

        if ( OwnerString ) {

            ReturnBufferSize += ( wcslen( OwnerString ) * sizeof( WCHAR ) ) +
                                SDDL_SIZE_TAG( SDDL_OWNER )  +
                                SDDL_SIZE_SEP( SDDL_DELIMINATORC );
        }

        if ( GroupString ) {

            ReturnBufferSize += ( wcslen( GroupString ) * sizeof( WCHAR ) ) +
                                SDDL_SIZE_TAG( SDDL_GROUP )  +
                                SDDL_SIZE_SEP( SDDL_DELIMINATORC );
        }

        if ( DaclPresent ) {

            ReturnBufferSize += SDDL_SIZE_TAG( SDDL_DACL )  +
                                SDDL_SIZE_SEP( SDDL_DELIMINATORC );

            if ( DaclControl ) {
                ReturnBufferSize += (wcslen( DaclControl ) * sizeof(WCHAR)) ;
            }
        }

        if ( SaclPresent ) {

            ReturnBufferSize += SDDL_SIZE_TAG( SDDL_SACL )  +
                                SDDL_SIZE_SEP( SDDL_DELIMINATORC );

            if ( SaclControl ) {
                ReturnBufferSize += (wcslen( SaclControl ) * sizeof(WCHAR)) ;
            }
        }


        *StringSecurityDescriptor = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                ReturnBufferSize + sizeof( WCHAR ) );

        if ( *StringSecurityDescriptor == NULL ) {

            Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            //
            // Build the string from the previously determined components.  Note that
            // if a component is not present, it is skipped.
            //
            DWORD dwOffset=0;

            if ( OwnerString ) {

                swprintf( *StringSecurityDescriptor, L"%ws%wc%ws",
                          SDDL_OWNER, SDDL_DELIMINATORC, OwnerString );

                dwOffset = wcslen(*StringSecurityDescriptor);
            }

            if ( GroupString ) {

                swprintf( *StringSecurityDescriptor + dwOffset,
                          L"%ws%wc%ws", SDDL_GROUP, SDDL_DELIMINATORC, GroupString );

                Revision = wcslen( *StringSecurityDescriptor + dwOffset ); // temp use
                dwOffset += Revision;

            }

            if ( DaclPresent ) {

                if ( DaclControl ) {
                    swprintf( *StringSecurityDescriptor + dwOffset,
                          L"%ws%wc%ws", SDDL_DACL, SDDL_DELIMINATORC, DaclControl );
                } else {
                    swprintf( *StringSecurityDescriptor + dwOffset,
                          L"%ws%wc", SDDL_DACL, SDDL_DELIMINATORC );
                }

                Revision = wcslen( *StringSecurityDescriptor + dwOffset );
                dwOffset += Revision;

                if ( DaclString ) {

                    wcscpy( *StringSecurityDescriptor + dwOffset, DaclString );

                    Revision = wcslen( *StringSecurityDescriptor + dwOffset ); // temp use
                    dwOffset += Revision;  // (AclSize/sizeof(WCHAR));
                }

            }

            if ( SaclPresent ) {

                if ( SaclControl ) {

                    swprintf( *StringSecurityDescriptor + dwOffset,
                              L"%ws%wc%ws", SDDL_SACL, SDDL_DELIMINATORC, SaclControl );
                } else {
                    swprintf( *StringSecurityDescriptor + dwOffset,
                              L"%ws%wc", SDDL_SACL, SDDL_DELIMINATORC );
                }

                Revision = wcslen( *StringSecurityDescriptor + dwOffset );
                dwOffset += Revision;

                if ( SaclString ) {

                    wcscpy( *StringSecurityDescriptor + dwOffset, SaclString);
                }

            }

            //
            // jinhuang
            // output the buffer size
            //

            if ( StringSecurityDescriptorLen ) {
                *StringSecurityDescriptorLen = ReturnBufferSize/sizeof(WCHAR);
            }
        }
    }


    //
    // Free any buffers that were allocated
    //
    LocalFree( OwnerString );
    LocalFree( GroupString );
    LocalFree( SaclString );
    LocalFree( DaclString );

    //
    // JINHUANG 3/10/98
    // it's ok to free them even if they are NULL
    //
    LocalFree( SaclControl );
    LocalFree( DaclControl );

    //
    // decrement the SidLookup instance
    //
    InitializeSidLookupTable(STRSD_REINITIALIZE_LEAVE);

    return( Err );
}




DWORD
LocalConvertStringSDToSD_Rev1(
    IN  PSID RootDomainSid OPTIONAL,
    IN  BOOLEAN DefaultToDomain,
    IN  LPCWSTR StringSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    )
/*++

Routine Description:

    This routine converts a revision 1 stringized Security descriptor into a valid, functional
    security descriptor

Arguments:

    StringSecurityDescriptor - Stringized security descriptor to be converted.

    SecurityDescriptor - Where the converted SD is returned.  Buffer is allocated via
        LocalAlloc and should be free via LocalFree.  The returned security descriptor
        is always self relative

    SecurityDescriptorSize - OPTIONAL.  If non-NULL, the size of the converted security
        descriptor is returned here.

    SecurityInformation - OPTIONAL. If non-NULL, the security information of the converted
        security descriptor is returned here.

Return Value:

    TRUE    -   Success
    FALSE   -   Failure

    Extended error status is available using GetLastError.

        ERROR_INVALID_PARAMETER - A NULL input or output parameter was given

        ERROR_UNKNOWN_REVISION - An unsupported revision was given

--*/
{
    DWORD Err = ERROR_SUCCESS;
    PSID Owner = NULL, Group = NULL;
    PACL Dacl  = NULL, Sacl  = NULL;
    SECURITY_INFORMATION SeInfo = 0;
    SECURITY_DESCRIPTOR SD;
    PWSTR Curr, End;
    NTSTATUS Status;
    BOOLEAN FreeOwner = FALSE, FreeGroup = FALSE;
    SID_IDENTIFIER_AUTHORITY UaspBuiltinAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY UaspCreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    ULONG SDSize = 0;
    BOOLEAN DaclPresent = FALSE, SaclPresent = FALSE;
    SECURITY_DESCRIPTOR_CONTROL DaclControl=0, SaclControl=0;


    if ( StringSecurityDescriptor == NULL || SecurityDescriptor == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( SecurityDescriptorSize ) {

        *SecurityDescriptorSize = 0;
    }

    //
    // make sure the SidLookup table is reinitialized
    //
    InitializeSidLookupTable(STRSD_REINITIALIZE_ENTER);

    //
    // Now, we'll just start parsing and building
    //
    Curr = ( PWSTR )StringSecurityDescriptor;

    while ( Err == ERROR_SUCCESS && Curr ) {

        switch( *Curr ) {

        //
        // Get the Owner sid
        //
        case L'O':

            Err = ERROR_INVALID_PARAMETER;

            if ( *(Curr+1) == SDDL_DELIMINATORC ) {

                Curr += 2;

                if ( Owner == NULL )  {

                    Err = LocalGetSidForString( Curr, &Owner, &End, &FreeOwner,
                                                RootDomainSid, DefaultToDomain );

                    if ( Err == ERROR_SUCCESS ) {

                        Curr = End;
                        SeInfo |= OWNER_SECURITY_INFORMATION;
                    }
                }
            }
            break;

        //
        // Get the Group sid
        //
        case L'G':

            Err = ERROR_INVALID_PARAMETER;

            if ( *(Curr+1) == SDDL_DELIMINATORC ) {

                Curr += 2;

                if ( Group == NULL ) {

                    Err = LocalGetSidForString( Curr, &Group, &End, &FreeGroup,
                                                RootDomainSid, DefaultToDomain );

                    if ( Err == ERROR_SUCCESS ) {

                        Curr = End;
                        SeInfo |= GROUP_SECURITY_INFORMATION;
                    }
                }
            }
            break;

        //
        // Next, the DAcl
        //
        case L'D':

            if ( *(Curr+1) == SDDL_DELIMINATORC ) {

                Curr += 2;

                if ( Dacl == NULL ) {

                    //
                    // JINHUANG: 3/10/98
                    // look for any security descriptor controls
                    //
                    if ( *Curr != SDDL_ACE_BEGINC ) {

                        Err = LocalGetSDControlForString( Curr,
                                                          SDDL_VALID_DACL,
                                                          &DaclControl,
                                                          &End );
                        if ( Err == ERROR_SUCCESS ) {
                            Curr = End;
                        }
                    }

                    if ( Err == ERROR_SUCCESS ) {

                        Err = LocalGetAclForString( Curr, TRUE, &Dacl, &End,
                                                    RootDomainSid, DefaultToDomain );

                        if ( Err == ERROR_SUCCESS ) {

                            Curr = End;
                            SeInfo |= DACL_SECURITY_INFORMATION;
                            DaclPresent = TRUE;
                        }
                    }

                } else {

                    Err = ERROR_INVALID_PARAMETER;
                }

            } else {

                Err = ERROR_INVALID_PARAMETER;
            }
            break;

        //
        // Finally, the SAcl
        //
        case L'S':

            if ( *(Curr+1) == SDDL_DELIMINATORC ) {

                Curr += 2;

                if ( Sacl == NULL ) {

                    //
                    // JINHUANG: 3/10/98
                    // look for any security descriptor controls
                    //
                    if ( *Curr != SDDL_ACE_BEGINC ) {

                        Err = LocalGetSDControlForString( Curr,
                                                          SDDL_VALID_SACL,
                                                          &SaclControl,
                                                          &End );
                        if ( Err == ERROR_SUCCESS ) {
                            Curr = End;
                        }
                    }

                    if ( Err == ERROR_SUCCESS ) {

                        Err = LocalGetAclForString( Curr, FALSE, &Sacl, &End,
                                                    RootDomainSid, DefaultToDomain );

                        if ( Err == ERROR_SUCCESS ) {

                            Curr = End;
                            SeInfo |= SACL_SECURITY_INFORMATION;
                            SaclPresent = TRUE;
                        }
                    }

                } else {

                    Err = ERROR_INVALID_PARAMETER;
                }

            } else {

                Err = ERROR_INVALID_PARAMETER;
            }
            break;

        //
        // It's a space, so ignore it
        //
        case L' ':
            Curr++;
            break;

        //
        // End of the string, so quit
        //
        case L'\0':
            Curr = NULL;
            break;

        //
        // Don't know what it is, so consider it an error
        //
        default:
            Err = ERROR_INVALID_PARAMETER;
            break;
        }
    }


    //
    // Ok, if we have the information we need, we'll create the security
    // descriptor
    //
    if ( Err == ERROR_SUCCESS ) {

        if ( InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION ) == FALSE ) {

            Err = GetLastError();
        }

        //
        // JINHUANG 3/10/98
        // set the security descriptor control
        //

        SD.Control |= (DaclControl | SaclControl);

        //
        // Now, add the owner and group
        //
        if ( Err == ERROR_SUCCESS && Owner != NULL ) {

            if ( SetSecurityDescriptorOwner(&SD, Owner, FALSE ) == FALSE ) {

                Err = GetLastError();
            }
        }

        if ( Err == ERROR_SUCCESS && Group != NULL ) {

            if ( SetSecurityDescriptorGroup( &SD, Group, FALSE ) == FALSE ) {

                Err = GetLastError();
            }
        }

        //
        // Then the DACL and SACL
        //
        if ( Err == ERROR_SUCCESS && DaclPresent ) {

            if ( SetSecurityDescriptorDacl( &SD, DaclPresent, Dacl, FALSE ) == FALSE ) {

                Err = GetLastError();
            }
        }

        if ( Err == ERROR_SUCCESS && SaclPresent ) {

            if ( SetSecurityDescriptorSacl( &SD, SaclPresent, Sacl, FALSE ) == FALSE ) {

                Err = GetLastError();
            }
        }

    }

    //
    // Finally, we'll allocate our buffer and return
    //
    if ( Err == ERROR_SUCCESS ) {

        MakeSelfRelativeSD( &SD, *SecurityDescriptor, &SDSize );

        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {

            *SecurityDescriptor = (PSECURITY_DESCRIPTOR) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                                                     SDSize );

            if ( *SecurityDescriptor == NULL ) {

                Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                if ( MakeSelfRelativeSD( &SD, *SecurityDescriptor, &SDSize ) == FALSE ) {

                    Err = GetLastError();
                    LocalFree( *SecurityDescriptor );
                    *SecurityDescriptor = NULL;

                }
            }

        } else {

            //
            // This should never happen
            //
            if ( GetLastError() == ERROR_SUCCESS ) {

                Err = ERROR_INSUFFICIENT_BUFFER;
            }
        }
    }

    //
    // Return the security descriptor size, if requested
    //
    if ( Err == ERROR_SUCCESS && SecurityDescriptorSize ) {

        *SecurityDescriptorSize = SDSize;
    }

    //
    // Finally, free any memory we may have allocated...
    //
    if ( FreeOwner == TRUE ) {

        LocalFree( Owner );

    }

    if ( FreeGroup == TRUE ) {

        LocalFree( Group );

    }

    LocalFree( Dacl );
    LocalFree( Sacl );

    //
    // make sure the SidLookup table is reinitialized
    //
    InitializeSidLookupTable(STRSD_REINITIALIZE_LEAVE);

    return( Err );
}

BOOLEAN
InitializeSidLookupTable(
    IN BYTE InitFlag
    )
{
    SID_IDENTIFIER_AUTHORITY UaspWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY UaspLocalAuthority = SECURITY_LOCAL_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY UaspCreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY UaspNtAuthority = SECURITY_NT_AUTHORITY;
    DWORD i;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status=STATUS_SUCCESS;
    LSA_HANDLE LsaPolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo = NULL;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;


    EnterCriticalSection(&SddlSidLookupCritical);

    switch ( InitFlag ) {
    case STRSD_REINITIALIZE_ENTER:

        SidTableReinitializeInstance++;

        if ( SidTableReinitializeInstance > 1 ) {
            //
            // there is another thread going, no need to reinitialize the table again
            //
            LeaveCriticalSection(&SddlSidLookupCritical);

            return TRUE;
        }
        break;

    case STRSD_REINITIALIZE_LEAVE:

        if ( SidTableReinitializeInstance > 1 ) {
            SidTableReinitializeInstance--;
        } else {
            SidTableReinitializeInstance = 0;
        }

        LeaveCriticalSection(&SddlSidLookupCritical);

        return TRUE;
        break;
    }

    //
    // open LSA to query domain and DNS information at once
    // because some domain/local relative SIDs will be initialized
    // if for some reason that the domain/DNS information can't be
    // queried, the relative SIDs will be invalid for this moment.
    //
    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

    Status = LsaOpenPolicy( NULL, &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaPolicyHandle );

    if ( NT_SUCCESS(Status) ) {

        Status = LsaQueryInformationPolicy( LsaPolicyHandle,
                                            PolicyDnsDomainInformation,
                                            ( PVOID * )&DnsDomainInfo );

        Status = LsaQueryInformationPolicy( LsaPolicyHandle,
                                            PolicyAccountDomainInformation,
                                            ( PVOID * )&DomainInfo );

        LsaClose( LsaPolicyHandle );
    }

    //
    // If the list of sids hasn't been built, do it now
    //
    // JINHUANG 3/26 BVT break,
    // see comments above always try to initialization table
    // but skip the ones already initialized for performance
    //
    for ( i = 0;
          i < sizeof( SidLookup ) / sizeof( STRSD_SID_LOOKUP ); i++ ) {

        if ( STRSD_REINITIALIZE_ENTER == InitFlag ) {
            SidLookup[ i ].Valid = FALSE;
        }

        if ( SidLookup[ i ].Valid == TRUE &&
             SidLookup[ i ].Sid != NULL ) {
            //
            // this one is already initialized
            //
            continue;
        }

        SidLookup[ i ].Sid = ( PSID )SidLookup[ i ].SidBuff;
        Status = STATUS_SUCCESS;

        switch ( SidLookup[ i ].SidType ) {
        case ST_DOMAIN_RELATIVE:

            if ( DnsDomainInfo != NULL && DnsDomainInfo->Sid != NULL ) {

                RtlCopyMemory( SidLookup[ i ].Sid, DnsDomainInfo->Sid,
                               RtlLengthSid( DnsDomainInfo->Sid ) );
                ( ( PISID )( SidLookup[ i ].Sid ) )->SubAuthorityCount++;
                *( RtlSubAuthoritySid( SidLookup[ i ].Sid,
                                       *( RtlSubAuthorityCountSid( DnsDomainInfo->Sid ) ) ) ) =
                                       SidLookup[ i ].Rid;
                SidLookup[ i ].Valid = TRUE;

            }

            break;

        case ST_ROOT_DOMAIN_RELATIVE:
            //
            // will be initialized on demand
            //
            break;

        case ST_WORLD:
            RtlInitializeSid( SidLookup[ i ].Sid, &UaspWorldAuthority, 1 );
            *( RtlSubAuthoritySid( SidLookup[ i ].Sid, 0 ) ) = SidLookup[ i ].Rid;
            SidLookup[ i ].Valid = TRUE;
            break;

        case ST_LOCALSY:
            RtlInitializeSid( SidLookup[ i ].Sid, &UaspLocalAuthority, 1 );
            *( RtlSubAuthoritySid( SidLookup[ i ].Sid, 0 ) ) = SidLookup[ i ].Rid;
            SidLookup[ i ].Valid = TRUE;
            break;

        case ST_LOCAL:
            if ( DomainInfo != NULL && DomainInfo->DomainSid ) {

                RtlCopyMemory( SidLookup[ i ].Sid, DomainInfo->DomainSid,
                               RtlLengthSid( DomainInfo->DomainSid ) );

                ( ( PISID )( SidLookup[ i ].Sid ) )->SubAuthorityCount++;
                *( RtlSubAuthoritySid( SidLookup[ i ].Sid,
                                       *( RtlSubAuthorityCountSid( DomainInfo->DomainSid ) ) ) ) =
                                       SidLookup[ i ].Rid;
                SidLookup[ i ].Valid = TRUE;
            }
            break;

        case ST_CREATOR:
            RtlInitializeSid( SidLookup[ i ].Sid, &UaspCreatorAuthority, 1 );
            *( RtlSubAuthoritySid( SidLookup[ i ].Sid, 0 ) ) = SidLookup[ i ].Rid;
            SidLookup[ i ].Valid = TRUE;
            break;

        case ST_NTAUTH:
            RtlInitializeSid( SidLookup[ i ].Sid, &UaspNtAuthority, 1 );
            *( RtlSubAuthoritySid( SidLookup[ i ].Sid, 0 ) ) = SidLookup[ i ].Rid;
            SidLookup[ i ].Valid = TRUE;
            break;

        case ST_BUILTIN:
            RtlInitializeSid( SidLookup[ i ].Sid, &UaspNtAuthority, 2 );
            *( RtlSubAuthoritySid( SidLookup[ i ].Sid, 0 ) ) = SECURITY_BUILTIN_DOMAIN_RID;
            *( RtlSubAuthoritySid( SidLookup[ i ].Sid, 1 ) ) = SidLookup[ i ].Rid;
            SidLookup[ i ].Valid = TRUE;
            break;

        }
    }

    LsaFreeMemory( DomainInfo );
    LsaFreeMemory( DnsDomainInfo );

    LeaveCriticalSection(&SddlSidLookupCritical);

    return TRUE;
}


BOOL
APIENTRY
ConvertStringSDToSDRootDomainA(
    IN  PSID RootDomainSid OPTIONAL,
    IN  LPCSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    )
{
    UNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    BOOL Result;

    if ( NULL == StringSecurityDescriptor ||
         NULL == SecurityDescriptor ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    RtlInitAnsiString( &AnsiString, StringSecurityDescriptor );

    Status = RtlAnsiStringToUnicodeString( &Unicode,
                                           &AnsiString,
                                           TRUE);

    if ( !NT_SUCCESS( Status ) ) {

        BaseSetLastNTError( Status );

        return FALSE;

    }

    Result = ConvertStringSDToSDRootDomainW( RootDomainSid,
                                           ( LPCWSTR )Unicode.Buffer,
                                           StringSDRevision,
                                           SecurityDescriptor,
                                           SecurityDescriptorSize);

    RtlFreeUnicodeString( &Unicode );

    if ( Result ) {
        SetLastError(ERROR_SUCCESS);
    }

    return( Result );

}

BOOL
APIENTRY
ConvertStringSDToSDRootDomainW(
    IN  PSID RootDomainSid OPTIONAL,
    IN  LPCWSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL
    )
{

    DWORD Err = ERROR_SUCCESS;

    //
    // Little elementary parameter checking...
    //
    if ( StringSecurityDescriptor == NULL || SecurityDescriptor == NULL ) {

        Err = ERROR_INVALID_PARAMETER;

    } else {

        switch ( StringSDRevision ) {
        case SDDL_REVISION_1:

            Err = LocalConvertStringSDToSD_Rev1( RootDomainSid,  // root domain sid
                                                 TRUE, // default to domain for EA/SA if root domain sid is not provided
                                                 StringSecurityDescriptor,
                                                 SecurityDescriptor,
                                                 SecurityDescriptorSize);
            break;

        default:

            Err = ERROR_UNKNOWN_REVISION;
            break;
        }

    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS );
}

BOOL
APIENTRY
ConvertSDToStringSDRootDomainA(
    IN  PSID RootDomainSid OPTIONAL,
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  DWORD RequestedStringSDRevision,
    IN  SECURITY_INFORMATION SecurityInformation,
    OUT LPSTR  *StringSecurityDescriptor OPTIONAL,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    )
{
    LPWSTR StringSecurityDescriptorW = NULL;
    ULONG AnsiLen, WideLen;
    BOOL ReturnValue ;

    if ( StringSecurityDescriptor == NULL ||
         0 == SecurityInformation ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    ReturnValue = ConvertSDToStringSDRootDomainW(
                      RootDomainSid,
                      SecurityDescriptor,
                      RequestedStringSDRevision,
                      SecurityInformation,
                      &StringSecurityDescriptorW,
                      &WideLen );

    if ( ReturnValue ) {

        AnsiLen = WideCharToMultiByte( CP_ACP,
                                       0,
                                       StringSecurityDescriptorW,
                                       WideLen+1,
                                       *StringSecurityDescriptor,
                                       0,
                                       NULL,
                                       NULL );

        if ( AnsiLen != 0 ) {

            *StringSecurityDescriptor = LocalAlloc( LMEM_FIXED, AnsiLen );

            if ( *StringSecurityDescriptor == NULL ) {

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                ReturnValue = FALSE;

            } else {

                AnsiLen = WideCharToMultiByte( CP_ACP,
                                               0,
                                               StringSecurityDescriptorW,
                                               WideLen+1,
                                               *StringSecurityDescriptor,
                                               AnsiLen,
                                               NULL,
                                               NULL );
                ASSERT( AnsiLen != 0 );

                if ( AnsiLen == 0 ) {

                    LocalFree(*StringSecurityDescriptor);
                    *StringSecurityDescriptor = NULL;

                    ReturnValue = FALSE;
                }

                //
                // jinhuang
                // output the length (optional)
                //
                if ( StringSecurityDescriptorLen ) {
                    *StringSecurityDescriptorLen = AnsiLen;
                }

            }

        } else {

            ReturnValue = FALSE;
        }

        LocalFree(StringSecurityDescriptorW);

    }

    if ( ReturnValue ) {
        SetLastError(ERROR_SUCCESS);
    }

    return( ReturnValue );

}

BOOL
APIENTRY
ConvertSDToStringSDRootDomainW(
    IN  PSID RootDomainSid OPTIONAL,
    IN  PSECURITY_DESCRIPTOR  SecurityDescriptor,
    IN  DWORD RequestedStringSDRevision,
    IN  SECURITY_INFORMATION SecurityInformation,
    OUT LPWSTR  *StringSecurityDescriptor OPTIONAL,
    OUT PULONG StringSecurityDescriptorLen OPTIONAL
    )
{

    DWORD Err = ERROR_SUCCESS;

    //
    // A little parameter checking...
    //
    if ( SecurityDescriptor == NULL || StringSecurityDescriptor == NULL ||
         SecurityInformation == 0 ) {

        Err =  ERROR_INVALID_PARAMETER;

    } else {

        switch ( RequestedStringSDRevision ) {
        case SDDL_REVISION_1:

            Err = LocalConvertSDToStringSD_Rev1( RootDomainSid,  // root domain sid
                                                 SecurityDescriptor,
                                                 SecurityInformation,
                                                 StringSecurityDescriptor,
                                                 StringSecurityDescriptorLen );
            break;

        default:
            Err = ERROR_UNKNOWN_REVISION;
            break;
        }

    }

    SetLastError( Err );

    return( Err == ERROR_SUCCESS);

}

BOOL
SddlpGetRootDomainSid(void)
{
    //
    // get root domain sid, save it in RootDomSidBuf (global)
    // this function is called within the critical section
    //
    // 1) ldap_open to the DC of interest.
    // 2) you do not need to ldap_connect - the following step works anonymously
    // 3) read the operational attribute rootDomainNamingContext and provide the
    //    operational control LDAP_SERVER_EXTENDED_DN_OID as defined in sdk\inc\ntldap.h.


    DWORD               Win32rc=NO_ERROR;
    BOOL                bRetValue=FALSE;

    HINSTANCE                   hLdapDll=NULL;
    PFN_LDAP_OPEN               pfnLdapOpen=NULL;
    PFN_LDAP_UNBIND             pfnLdapUnbind=NULL;
    PFN_LDAP_SEARCH             pfnLdapSearch=NULL;
    PFN_LDAP_FIRST_ENTRY        pfnLdapFirstEntry=NULL;
    PFN_LDAP_GET_VALUE          pfnLdapGetValue=NULL;
    PFN_LDAP_MSGFREE            pfnLdapMsgFree=NULL;
    PFN_LDAP_VALUE_FREE         pfnLdapValueFree=NULL;
    PFN_LDAP_MAP_ERROR          pfnLdapMapError=NULL;

    PLDAP                       phLdap=NULL;

    LDAPControlA    serverControls = { LDAP_SERVER_EXTENDED_DN_OID,
                                       { 0, (PCHAR) NULL },
                                       TRUE
                                     };
    LPSTR           Attribs[] = { LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT, NULL };

    PLDAPControlA   rServerControls[] = { &serverControls, NULL };
    PLDAPMessage    pMessage = NULL;
    LDAPMessage     *pEntry = NULL;
    PCHAR           *ppszValues=NULL;

    LPSTR           pSidStart, pSidEnd, pParse;
    BYTE            *pDest;
    BYTE            OneByte;

    hLdapDll = LoadLibraryA("wldap32.dll");

    if ( hLdapDll) {
        pfnLdapOpen = (PFN_LDAP_OPEN)GetProcAddress(hLdapDll,
                                                    "ldap_openA");
        pfnLdapUnbind = (PFN_LDAP_UNBIND)GetProcAddress(hLdapDll,
                                                      "ldap_unbind");
        pfnLdapSearch = (PFN_LDAP_SEARCH)GetProcAddress(hLdapDll,
                                                    "ldap_search_ext_sA");
        pfnLdapFirstEntry = (PFN_LDAP_FIRST_ENTRY)GetProcAddress(hLdapDll,
                                                      "ldap_first_entry");
        pfnLdapGetValue = (PFN_LDAP_GET_VALUE)GetProcAddress(hLdapDll,
                                                    "ldap_get_valuesA");
        pfnLdapMsgFree = (PFN_LDAP_MSGFREE)GetProcAddress(hLdapDll,
                                                      "ldap_msgfree");
        pfnLdapValueFree = (PFN_LDAP_VALUE_FREE)GetProcAddress(hLdapDll,
                                                    "ldap_value_freeA");
        pfnLdapMapError = (PFN_LDAP_MAP_ERROR)GetProcAddress(hLdapDll,
                                                      "LdapMapErrorToWin32");
    }

    if ( pfnLdapOpen == NULL ||
         pfnLdapUnbind == NULL ||
         pfnLdapSearch == NULL ||
         pfnLdapFirstEntry == NULL ||
         pfnLdapGetValue == NULL ||
         pfnLdapMsgFree == NULL ||
         pfnLdapValueFree == NULL ||
         pfnLdapMapError == NULL ) {

        Win32rc = ERROR_PROC_NOT_FOUND;

    } else {

        //
        // bind to ldap
        //
        phLdap = (*pfnLdapOpen)(NULL, LDAP_PORT);

        if ( phLdap == NULL ) {

            Win32rc = ERROR_FILE_NOT_FOUND;

        }
    }

    if ( NO_ERROR == Win32rc ) {
        //
        // now get the ldap handle,
        //

        Win32rc = (*pfnLdapSearch)(
                        phLdap,
                        "",
                        LDAP_SCOPE_BASE,
                        "(objectClass=*)",
                        Attribs,
                        0,
                        (PLDAPControlA *)&rServerControls,
                        NULL,
                        NULL,
                        10000,
                        &pMessage);

        if( Win32rc == NO_ERROR && pMessage ) {

            Win32rc = ERROR_SUCCESS;

            pEntry = (*pfnLdapFirstEntry)(phLdap, pMessage);

            if(pEntry == NULL) {

                Win32rc = (*pfnLdapMapError)( phLdap->ld_errno );

            } else {
                //
                // Now, we'll have to get the values
                //
                ppszValues = (*pfnLdapGetValue)(phLdap,
                                              pEntry,
                                              Attribs[0]);

                if( ppszValues == NULL) {

                    Win32rc = (*pfnLdapMapError)( phLdap->ld_errno );

                } else if ( ppszValues[0] && ppszValues[0][0] != '\0' ) {

                    //
                    // ppszValues[0] is the value to parse.
                    // The data will be returned as something like:

                    // <GUID=278676f8d753d211a61ad7e2dfa25f11>;<SID=010400000000000515000000828ba6289b0bc11e67c2ef7f>;DC=colinbrdom1,DC=nttest,DC=microsoft,DC=com

                    // Parse through this to find the <SID=xxxxxx> part.  Note that it may be missing, but the GUID= and trailer should not be.
                    // The xxxxx represents the hex nibbles of the SID.  Translate to the binary form and case to a SID.


                    pSidStart = strstr(ppszValues[0], "<SID=");

                    if ( pSidStart ) {
                        //
                        // find the end of this SID
                        //
                        pSidEnd = strchr(pSidStart, '>');

                        if ( pSidEnd ) {

                            EnterCriticalSection(&SddlSidLookupCritical);

                            pParse = pSidStart + 5;
                            pDest = (BYTE *)RootDomSidBuf;

                            while ( pParse < pSidEnd-1 ) {

                                if ( *pParse >= '0' && *pParse <= '9' ) {
                                    OneByte = (BYTE) ((*pParse - '0') * 16);
                                } else {
                                    OneByte = (BYTE) ( (tolower(*pParse) - 'a' + 10) * 16 );
                                }

                                if ( *(pParse+1) >= '0' && *(pParse+1) <= '9' ) {
                                    OneByte += (BYTE) ( *(pParse+1) - '0' ) ;
                                } else {
                                    OneByte += (BYTE) ( tolower(*(pParse+1)) - 'a' + 10 ) ;
                                }

                                *pDest = OneByte;
                                pDest++;
                                pParse += 2;
                            }

                            RootDomInited = TRUE;

                            LeaveCriticalSection(&SddlSidLookupCritical);

                            bRetValue = TRUE;

                        } else {
                            Win32rc = ERROR_OBJECT_NOT_FOUND;
                        }
                    } else {
                        Win32rc = ERROR_OBJECT_NOT_FOUND;
                    }

                    (*pfnLdapValueFree)(ppszValues);

                } else {
                    Win32rc = ERROR_OBJECT_NOT_FOUND;
                }
            }

            (*pfnLdapMsgFree)(pMessage);
        }

    }

    //
    // even though it's not binded, use unbind to close
    //
    if ( phLdap != NULL && pfnLdapUnbind )
        (*pfnLdapUnbind)(phLdap);

    if ( hLdapDll ) {
        FreeLibrary(hLdapDll);
    }

    SetLastError(Win32rc);

    return bRetValue;
}


