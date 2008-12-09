/*
 * Unit tests for security functions
 *
 * Copyright (c) 2004 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "aclapi.h"
#include "winnt.h"
#include "sddl.h"
#include "ntsecapi.h"
#include "lmcons.h"

#include "wine/test.h"

/* copied from Wine winternl.h - not included in the Windows SDK */
typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllInformation,
    ObjectDataInformation
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_BASIC_INFORMATION {
    ULONG  Attributes;
    ACCESS_MASK  GrantedAccess;
    ULONG  HandleCount;
    ULONG  PointerCount;
    ULONG  PagedPoolUsage;
    ULONG  NonPagedPoolUsage;
    ULONG  Reserved[3];
    ULONG  NameInformationLength;
    ULONG  TypeInformationLength;
    ULONG  SecurityDescriptorLength;
    LARGE_INTEGER  CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

#define expect_eq(expr, value, type, format) { type ret = expr; ok((value) == ret, #expr " expected " format "  got " format "\n", (value), (ret)); }

static BOOL (WINAPI *pAddAccessAllowedAceEx)(PACL, DWORD, DWORD, DWORD, PSID);
static BOOL (WINAPI *pAddAccessDeniedAceEx)(PACL, DWORD, DWORD, DWORD, PSID);
static BOOL (WINAPI *pAddAuditAccessAceEx)(PACL, DWORD, DWORD, DWORD, PSID, BOOL, BOOL);
typedef VOID (WINAPI *fnBuildTrusteeWithSidA)( PTRUSTEEA pTrustee, PSID pSid );
typedef VOID (WINAPI *fnBuildTrusteeWithNameA)( PTRUSTEEA pTrustee, LPSTR pName );
typedef VOID (WINAPI *fnBuildTrusteeWithObjectsAndNameA)( PTRUSTEEA pTrustee,
                                                          POBJECTS_AND_NAME_A pObjName,
                                                          SE_OBJECT_TYPE ObjectType,
                                                          LPSTR ObjectTypeName,
                                                          LPSTR InheritedObjectTypeName,
                                                          LPSTR Name );
typedef VOID (WINAPI *fnBuildTrusteeWithObjectsAndSidA)( PTRUSTEEA pTrustee,
                                                         POBJECTS_AND_SID pObjSid,
                                                         GUID* pObjectGuid,
                                                         GUID* pInheritedObjectGuid,
                                                         PSID pSid );
typedef LPSTR (WINAPI *fnGetTrusteeNameA)( PTRUSTEEA pTrustee );
typedef BOOL (WINAPI *fnMakeSelfRelativeSD)( PSECURITY_DESCRIPTOR, PSECURITY_DESCRIPTOR, LPDWORD );
typedef BOOL (WINAPI *fnConvertSidToStringSidA)( PSID pSid, LPSTR *str );
typedef BOOL (WINAPI *fnConvertStringSidToSidA)( LPCSTR str, PSID pSid );
static BOOL (WINAPI *pConvertStringSecurityDescriptorToSecurityDescriptorA)(LPCSTR, DWORD,
                                                                            PSECURITY_DESCRIPTOR*, PULONG );
static BOOL (WINAPI *pConvertSecurityDescriptorToStringSecurityDescriptorA)(PSECURITY_DESCRIPTOR, DWORD,
                                                                            SECURITY_INFORMATION, LPSTR *, PULONG );
typedef BOOL (WINAPI *fnGetFileSecurityA)(LPCSTR, SECURITY_INFORMATION,
                                          PSECURITY_DESCRIPTOR, DWORD, LPDWORD);
static DWORD (WINAPI *pGetNamedSecurityInfoA)(LPSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                              PSID*, PSID*, PACL*, PACL*,
                                              PSECURITY_DESCRIPTOR*);
typedef DWORD (WINAPI *fnRtlAdjustPrivilege)(ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);
typedef BOOL (WINAPI *fnCreateWellKnownSid)(WELL_KNOWN_SID_TYPE,PSID,PSID,DWORD*);
typedef BOOL (WINAPI *fnDuplicateTokenEx)(HANDLE,DWORD,LPSECURITY_ATTRIBUTES,
                                        SECURITY_IMPERSONATION_LEVEL,TOKEN_TYPE,PHANDLE);

typedef NTSTATUS (WINAPI *fnLsaQueryInformationPolicy)(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
typedef NTSTATUS (WINAPI *fnLsaClose)(LSA_HANDLE);
typedef NTSTATUS (WINAPI *fnLsaFreeMemory)(PVOID);
typedef NTSTATUS (WINAPI *fnLsaOpenPolicy)(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
static NTSTATUS (WINAPI *pNtQueryObject)(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
static DWORD (WINAPI *pSetEntriesInAclW)(ULONG, PEXPLICIT_ACCESSW, PACL, PACL*);
static BOOL (WINAPI *pSetSecurityDescriptorControl)(PSECURITY_DESCRIPTOR, SECURITY_DESCRIPTOR_CONTROL,
                                                    SECURITY_DESCRIPTOR_CONTROL);
static DWORD (WINAPI *pGetSecurityInfo)(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                        PSID*, PSID*, PACL*, PACL*, PSECURITY_DESCRIPTOR*);

static HMODULE hmod;
static int     myARGC;
static char**  myARGV;

fnBuildTrusteeWithSidA   pBuildTrusteeWithSidA;
fnBuildTrusteeWithNameA  pBuildTrusteeWithNameA;
fnBuildTrusteeWithObjectsAndNameA pBuildTrusteeWithObjectsAndNameA;
fnBuildTrusteeWithObjectsAndSidA pBuildTrusteeWithObjectsAndSidA;
fnGetTrusteeNameA pGetTrusteeNameA;
fnMakeSelfRelativeSD pMakeSelfRelativeSD;
fnConvertSidToStringSidA pConvertSidToStringSidA;
fnConvertStringSidToSidA pConvertStringSidToSidA;
fnGetFileSecurityA pGetFileSecurityA;
fnRtlAdjustPrivilege pRtlAdjustPrivilege;
fnCreateWellKnownSid pCreateWellKnownSid;
fnDuplicateTokenEx pDuplicateTokenEx;
fnLsaQueryInformationPolicy pLsaQueryInformationPolicy;
fnLsaClose pLsaClose;
fnLsaFreeMemory pLsaFreeMemory;
fnLsaOpenPolicy pLsaOpenPolicy;

struct sidRef
{
    SID_IDENTIFIER_AUTHORITY auth;
    const char *refStr;
};

static void init(void)
{
    HMODULE hntdll;

    hntdll = GetModuleHandleA("ntdll.dll");
    pNtQueryObject = (void *)GetProcAddress( hntdll, "NtQueryObject" );

    hmod = GetModuleHandle("advapi32.dll");
    pAddAccessAllowedAceEx = (void *)GetProcAddress(hmod, "AddAccessAllowedAceEx");
    pAddAccessDeniedAceEx = (void *)GetProcAddress(hmod, "AddAccessDeniedAceEx");
    pAddAuditAccessAceEx = (void *)GetProcAddress(hmod, "AddAuditAccessAceEx");
    pConvertStringSecurityDescriptorToSecurityDescriptorA =
        (void *)GetProcAddress(hmod, "ConvertStringSecurityDescriptorToSecurityDescriptorA" );
    pConvertSecurityDescriptorToStringSecurityDescriptorA =
        (void *)GetProcAddress(hmod, "ConvertSecurityDescriptorToStringSecurityDescriptorA" );
    pCreateWellKnownSid = (fnCreateWellKnownSid)GetProcAddress( hmod, "CreateWellKnownSid" );
    pGetNamedSecurityInfoA = (void *)GetProcAddress(hmod, "GetNamedSecurityInfoA");
    pMakeSelfRelativeSD = (void *)GetProcAddress(hmod, "MakeSelfRelativeSD");
    pSetEntriesInAclW = (void *)GetProcAddress(hmod, "SetEntriesInAclW");
    pSetSecurityDescriptorControl = (void *)GetProcAddress(hmod, "SetSecurityDescriptorControl");
    pGetSecurityInfo = (void *)GetProcAddress(hmod, "GetSecurityInfo");

    myARGC = winetest_get_mainargs( &myARGV );
}

static void test_str_sid(const char *str_sid)
{
    PSID psid;
    char *temp;

    if (pConvertStringSidToSidA(str_sid, &psid))
    {
        if (pConvertSidToStringSidA(psid, &temp))
        {
            trace(" %s: %s\n", str_sid, temp);
            LocalFree(temp);
        }
        LocalFree(psid);
    }
    else
    {
        if (GetLastError() != ERROR_INVALID_SID)
            trace(" %s: couldn't be converted, returned %d\n", str_sid, GetLastError());
        else
            trace(" %s: couldn't be converted\n", str_sid);
    }
}

static void test_sid(void)
{
    struct sidRef refs[] = {
     { { {0x00,0x00,0x33,0x44,0x55,0x66} }, "S-1-860116326-1" },
     { { {0x00,0x00,0x01,0x02,0x03,0x04} }, "S-1-16909060-1"  },
     { { {0x00,0x00,0x00,0x01,0x02,0x03} }, "S-1-66051-1"     },
     { { {0x00,0x00,0x00,0x00,0x01,0x02} }, "S-1-258-1"       },
     { { {0x00,0x00,0x00,0x00,0x00,0x02} }, "S-1-2-1"         },
     { { {0x00,0x00,0x00,0x00,0x00,0x0c} }, "S-1-12-1"        },
    };
    const char noSubAuthStr[] = "S-1-5";
    unsigned int i;
    PSID psid = NULL;
    SID *pisid;
    BOOL r;
    LPSTR str = NULL;

    pConvertSidToStringSidA = (fnConvertSidToStringSidA)
                    GetProcAddress( hmod, "ConvertSidToStringSidA" );
    if( !pConvertSidToStringSidA )
        return;
    pConvertStringSidToSidA = (fnConvertStringSidToSidA)
                    GetProcAddress( hmod, "ConvertStringSidToSidA" );
    if( !pConvertStringSidToSidA )
        return;

    r = pConvertStringSidToSidA( NULL, NULL );
    ok( !r, "expected failure with NULL parameters\n" );
    if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED )
        return;
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %d\n",
     GetLastError() );

    r = pConvertStringSidToSidA( refs[0].refStr, NULL );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %d\n",
     GetLastError() );

    r = pConvertStringSidToSidA( NULL, &str );
    ok( !r && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected GetLastError() is ERROR_INVALID_PARAMETER, got %d\n",
     GetLastError() );

    r = pConvertStringSidToSidA( noSubAuthStr, &psid );
    ok( !r,
     "expected failure with no sub authorities\n" );
    ok( GetLastError() == ERROR_INVALID_SID,
     "expected GetLastError() is ERROR_INVALID_SID, got %d\n",
     GetLastError() );

    ok(pConvertStringSidToSidA("S-1-5-21-93476-23408-4576", &psid), "ConvertStringSidToSidA failed\n");
    pisid = (SID *)psid;
    ok(pisid->SubAuthorityCount == 4, "Invalid sub authority count - expected 4, got %d\n", pisid->SubAuthorityCount);
    ok(pisid->SubAuthority[0] == 21, "Invalid subauthority 0 - expceted 21, got %d\n", pisid->SubAuthority[0]);
    ok(pisid->SubAuthority[3] == 4576, "Invalid subauthority 0 - expceted 4576, got %d\n", pisid->SubAuthority[3]);
    LocalFree(str);

    for( i = 0; i < sizeof(refs) / sizeof(refs[0]); i++ )
    {
        PISID pisid;

        r = AllocateAndInitializeSid( &refs[i].auth, 1,1,0,0,0,0,0,0,0,
         &psid );
        ok( r, "failed to allocate sid\n" );
        r = pConvertSidToStringSidA( psid, &str );
        ok( r, "failed to convert sid\n" );
        if (r)
        {
            ok( !strcmp( str, refs[i].refStr ),
                "incorrect sid, expected %s, got %s\n", refs[i].refStr, str );
            LocalFree( str );
        }
        if( psid )
            FreeSid( psid );

        r = pConvertStringSidToSidA( refs[i].refStr, &psid );
        ok( r, "failed to parse sid string\n" );
        pisid = (PISID)psid;
        ok( pisid &&
         !memcmp( pisid->IdentifierAuthority.Value, refs[i].auth.Value,
         sizeof(refs[i].auth) ),
         "string sid %s didn't parse to expected value\n"
         "(got 0x%04x%08x, expected 0x%04x%08x)\n",
         refs[i].refStr,
         MAKEWORD( pisid->IdentifierAuthority.Value[1],
         pisid->IdentifierAuthority.Value[0] ),
         MAKELONG( MAKEWORD( pisid->IdentifierAuthority.Value[5],
         pisid->IdentifierAuthority.Value[4] ),
         MAKEWORD( pisid->IdentifierAuthority.Value[3],
         pisid->IdentifierAuthority.Value[2] ) ),
         MAKEWORD( refs[i].auth.Value[1], refs[i].auth.Value[0] ),
         MAKELONG( MAKEWORD( refs[i].auth.Value[5], refs[i].auth.Value[4] ),
         MAKEWORD( refs[i].auth.Value[3], refs[i].auth.Value[2] ) ) );
        if( psid )
            LocalFree( psid );
    }

    trace("String SIDs:\n");
    test_str_sid("AO");
    test_str_sid("RU");
    test_str_sid("AN");
    test_str_sid("AU");
    test_str_sid("BA");
    test_str_sid("BG");
    test_str_sid("BO");
    test_str_sid("BU");
    test_str_sid("CA");
    test_str_sid("CG");
    test_str_sid("CO");
    test_str_sid("DA");
    test_str_sid("DC");
    test_str_sid("DD");
    test_str_sid("DG");
    test_str_sid("DU");
    test_str_sid("EA");
    test_str_sid("ED");
    test_str_sid("WD");
    test_str_sid("PA");
    test_str_sid("IU");
    test_str_sid("LA");
    test_str_sid("LG");
    test_str_sid("LS");
    test_str_sid("SY");
    test_str_sid("NU");
    test_str_sid("NO");
    test_str_sid("NS");
    test_str_sid("PO");
    test_str_sid("PS");
    test_str_sid("PU");
    test_str_sid("RS");
    test_str_sid("RD");
    test_str_sid("RE");
    test_str_sid("RC");
    test_str_sid("SA");
    test_str_sid("SO");
    test_str_sid("SU");
}

static void test_trustee(void)
{
    GUID ObjectType = {0x12345678, 0x1234, 0x5678, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
    GUID InheritedObjectType = {0x23456789, 0x2345, 0x6786, {0x2, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
    GUID ZeroGuid;
    OBJECTS_AND_NAME_ oan;
    OBJECTS_AND_SID oas;
    TRUSTEE trustee;
    PSID psid;
    char szObjectTypeName[] = "ObjectTypeName";
    char szInheritedObjectTypeName[] = "InheritedObjectTypeName";
    char szTrusteeName[] = "szTrusteeName";
    SID_IDENTIFIER_AUTHORITY auth = { {0x11,0x22,0,0,0, 0} };

    memset( &ZeroGuid, 0x00, sizeof (ZeroGuid) );

    pBuildTrusteeWithSidA = (fnBuildTrusteeWithSidA)
                    GetProcAddress( hmod, "BuildTrusteeWithSidA" );
    pBuildTrusteeWithNameA = (fnBuildTrusteeWithNameA)
                    GetProcAddress( hmod, "BuildTrusteeWithNameA" );
    pBuildTrusteeWithObjectsAndNameA = (fnBuildTrusteeWithObjectsAndNameA)
                    GetProcAddress (hmod, "BuildTrusteeWithObjectsAndNameA" );
    pBuildTrusteeWithObjectsAndSidA = (fnBuildTrusteeWithObjectsAndSidA)
                    GetProcAddress (hmod, "BuildTrusteeWithObjectsAndSidA" );
    pGetTrusteeNameA = (fnGetTrusteeNameA)
                    GetProcAddress (hmod, "GetTrusteeNameA" );
    if( !pBuildTrusteeWithSidA || !pBuildTrusteeWithNameA ||
        !pBuildTrusteeWithObjectsAndNameA || !pBuildTrusteeWithObjectsAndSidA ||
        !pGetTrusteeNameA )
        return;

    if ( ! AllocateAndInitializeSid( &auth, 1, 42, 0,0,0,0,0,0,0,&psid ) )
    {
        trace( "failed to init SID\n" );
       return;
    }

    /* test BuildTrusteeWithSidA */
    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithSidA( &trustee, psid );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_SID, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == (LPSTR) psid, "ptstrName wrong\n" );

    /* test BuildTrusteeWithObjectsAndSidA (test 1) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oas, 0xff, sizeof(oas) );
    pBuildTrusteeWithObjectsAndSidA(&trustee, &oas, &ObjectType,
                                    &InheritedObjectType, psid);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oas, "ptstrName wrong\n");
 
    ok(oas.ObjectsPresent == (ACE_OBJECT_TYPE_PRESENT | ACE_INHERITED_OBJECT_TYPE_PRESENT), "ObjectsPresent wrong\n");
    ok(!memcmp(&oas.ObjectTypeGuid, &ObjectType, sizeof(GUID)), "ObjectTypeGuid wrong\n");
    ok(!memcmp(&oas.InheritedObjectTypeGuid, &InheritedObjectType, sizeof(GUID)), "InheritedObjectTypeGuid wrong\n");
    ok(oas.pSid == psid, "pSid wrong\n");

    /* test GetTrusteeNameA */
    ok(pGetTrusteeNameA(&trustee) == (LPSTR)&oas, "GetTrusteeName returned wrong value\n");

    /* test BuildTrusteeWithObjectsAndSidA (test 2) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oas, 0xff, sizeof(oas) );
    pBuildTrusteeWithObjectsAndSidA(&trustee, &oas, NULL,
                                    &InheritedObjectType, psid);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oas, "ptstrName wrong\n");
 
    ok(oas.ObjectsPresent == ACE_INHERITED_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(!memcmp(&oas.ObjectTypeGuid, &ZeroGuid, sizeof(GUID)), "ObjectTypeGuid wrong\n");
    ok(!memcmp(&oas.InheritedObjectTypeGuid, &InheritedObjectType, sizeof(GUID)), "InheritedObjectTypeGuid wrong\n");
    ok(oas.pSid == psid, "pSid wrong\n");

    FreeSid( psid );

    /* test BuildTrusteeWithNameA */
    memset( &trustee, 0xff, sizeof trustee );
    pBuildTrusteeWithNameA( &trustee, szTrusteeName );

    ok( trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok( trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, 
        "MultipleTrusteeOperation wrong\n");
    ok( trustee.TrusteeForm == TRUSTEE_IS_NAME, "TrusteeForm wrong\n");
    ok( trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok( trustee.ptstrName == szTrusteeName, "ptstrName wrong\n" );

    /* test BuildTrusteeWithObjectsAndNameA (test 1) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, szObjectTypeName,
                                     szInheritedObjectTypeName, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPTSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == (ACE_OBJECT_TYPE_PRESENT | ACE_INHERITED_OBJECT_TYPE_PRESENT), "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == szInheritedObjectTypeName, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");

    /* test GetTrusteeNameA */
    ok(pGetTrusteeNameA(&trustee) == (LPSTR)&oan, "GetTrusteeName returned wrong value\n");

    /* test BuildTrusteeWithObjectsAndNameA (test 2) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, NULL,
                                     szInheritedObjectTypeName, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == ACE_INHERITED_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == szInheritedObjectTypeName, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");

    /* test BuildTrusteeWithObjectsAndNameA (test 3) */
    memset( &trustee, 0xff, sizeof trustee );
    memset( &oan, 0xff, sizeof(oan) );
    pBuildTrusteeWithObjectsAndNameA(&trustee, &oan, SE_KERNEL_OBJECT, szObjectTypeName,
                                     NULL, szTrusteeName);

    ok(trustee.pMultipleTrustee == NULL, "pMultipleTrustee wrong\n");
    ok(trustee.MultipleTrusteeOperation == NO_MULTIPLE_TRUSTEE, "MultipleTrusteeOperation wrong\n");
    ok(trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_NAME, "TrusteeForm wrong\n");
    ok(trustee.TrusteeType == TRUSTEE_IS_UNKNOWN, "TrusteeType wrong\n");
    ok(trustee.ptstrName == (LPTSTR)&oan, "ptstrName wrong\n");
 
    ok(oan.ObjectsPresent == ACE_OBJECT_TYPE_PRESENT, "ObjectsPresent wrong\n");
    ok(oan.ObjectType == SE_KERNEL_OBJECT, "ObjectType wrong\n");
    ok(oan.InheritedObjectTypeName == NULL, "InheritedObjectTypeName wrong\n");
    ok(oan.ptstrName == szTrusteeName, "szTrusteeName wrong\n");
}
 
/* If the first isn't defined, assume none is */
#ifndef SE_MIN_WELL_KNOWN_PRIVILEGE
#define SE_MIN_WELL_KNOWN_PRIVILEGE       2L
#define SE_CREATE_TOKEN_PRIVILEGE         2L
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   3L
#define SE_LOCK_MEMORY_PRIVILEGE          4L
#define SE_INCREASE_QUOTA_PRIVILEGE       5L
#define SE_MACHINE_ACCOUNT_PRIVILEGE      6L
#define SE_TCB_PRIVILEGE                  7L
#define SE_SECURITY_PRIVILEGE             8L
#define SE_TAKE_OWNERSHIP_PRIVILEGE       9L
#define SE_LOAD_DRIVER_PRIVILEGE         10L
#define SE_SYSTEM_PROFILE_PRIVILEGE      11L
#define SE_SYSTEMTIME_PRIVILEGE          12L
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 13L
#define SE_INC_BASE_PRIORITY_PRIVILEGE   14L
#define SE_CREATE_PAGEFILE_PRIVILEGE     15L
#define SE_CREATE_PERMANENT_PRIVILEGE    16L
#define SE_BACKUP_PRIVILEGE              17L
#define SE_RESTORE_PRIVILEGE             18L
#define SE_SHUTDOWN_PRIVILEGE            19L
#define SE_DEBUG_PRIVILEGE               20L
#define SE_AUDIT_PRIVILEGE               21L
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE  22L
#define SE_CHANGE_NOTIFY_PRIVILLEGE      23L
#define SE_REMOTE_SHUTDOWN_PRIVILEGE     24L
#define SE_UNDOCK_PRIVILEGE              25L
#define SE_SYNC_AGENT_PRIVILEGE          26L
#define SE_ENABLE_DELEGATION_PRIVILEGE   27L
#define SE_MANAGE_VOLUME_PRIVILEGE       28L
#define SE_IMPERSONATE_PRIVILEGE         29L
#define SE_CREATE_GLOBAL_PRIVILEGE       30L
#define SE_MAX_WELL_KNOWN_PRIVILEGE      SE_CREATE_GLOBAL_PRIVILEGE
#endif /* ndef SE_MIN_WELL_KNOWN_PRIVILEGE */

static void test_allocateLuid(void)
{
    BOOL (WINAPI *pAllocateLocallyUniqueId)(PLUID);
    LUID luid1, luid2;
    BOOL ret;

    pAllocateLocallyUniqueId = (void*)GetProcAddress(hmod, "AllocateLocallyUniqueId");
    if (!pAllocateLocallyUniqueId) return;

    ret = pAllocateLocallyUniqueId(&luid1);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    ok(ret,
     "AllocateLocallyUniqueId failed: %d\n", GetLastError());
    ret = pAllocateLocallyUniqueId(&luid2);
    ok( ret,
     "AllocateLocallyUniqueId failed: %d\n", GetLastError());
    ok(luid1.LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE || luid1.HighPart != 0,
     "AllocateLocallyUniqueId returned a well-known LUID\n");
    ok(luid1.LowPart != luid2.LowPart || luid1.HighPart != luid2.HighPart,
     "AllocateLocallyUniqueId returned non-unique LUIDs\n");
    ret = pAllocateLocallyUniqueId(NULL);
    ok( !ret && GetLastError() == ERROR_NOACCESS,
     "AllocateLocallyUniqueId(NULL) didn't return ERROR_NOACCESS: %d\n",
     GetLastError());
}

static void test_lookupPrivilegeName(void)
{
    BOOL (WINAPI *pLookupPrivilegeNameA)(LPCSTR, PLUID, LPSTR, LPDWORD);
    char buf[MAX_PATH]; /* arbitrary, seems long enough */
    DWORD cchName = sizeof(buf);
    LUID luid = { 0, 0 };
    LONG i;
    BOOL ret;

    /* check whether it's available first */
    pLookupPrivilegeNameA = (void*)GetProcAddress(hmod, "LookupPrivilegeNameA");
    if (!pLookupPrivilegeNameA) return;
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    /* check with a short buffer */
    cchName = 0;
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    ret = pLookupPrivilegeNameA(NULL, &luid, NULL, &cchName);
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "LookupPrivilegeNameA didn't fail with ERROR_INSUFFICIENT_BUFFER: %d\n",
     GetLastError());
    ok(cchName == strlen("SeCreateTokenPrivilege") + 1,
     "LookupPrivilegeNameA returned an incorrect required length for\n"
     "SeCreateTokenPrivilege (got %d, expected %d)\n", cchName,
     lstrlenA("SeCreateTokenPrivilege") + 1);
    /* check a known value and its returned length on success */
    cchName = sizeof(buf);
    ok(pLookupPrivilegeNameA(NULL, &luid, buf, &cchName) &&
     cchName == strlen("SeCreateTokenPrivilege"),
     "LookupPrivilegeNameA returned an incorrect output length for\n"
     "SeCreateTokenPrivilege (got %d, expected %d)\n", cchName,
     (int)strlen("SeCreateTokenPrivilege"));
    /* check known values */
    for (i = SE_MIN_WELL_KNOWN_PRIVILEGE; i < SE_MAX_WELL_KNOWN_PRIVILEGE; i++)
    {
        luid.LowPart = i;
        cchName = sizeof(buf);
        ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
        ok( ret || GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
         "LookupPrivilegeNameA(0.%d) failed: %d\n", i, GetLastError());
    }
    /* check a bogus LUID */
    luid.LowPart = 0xdeadbeef;
    cchName = sizeof(buf);
    ret = pLookupPrivilegeNameA(NULL, &luid, buf, &cchName);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeNameA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %d\n",
     GetLastError());
    /* check on a bogus system */
    luid.LowPart = SE_CREATE_TOKEN_PRIVILEGE;
    cchName = sizeof(buf);
    ret = pLookupPrivilegeNameA("b0gu5.Nam3", &luid, buf, &cchName);
    ok( !ret && (GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
                 GetLastError() == RPC_S_INVALID_NET_ADDR) /* w2k8 */,
     "LookupPrivilegeNameA didn't fail with RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR: %d\n",
     GetLastError());
}

struct NameToLUID
{
    const char *name;
    DWORD lowPart;
};

static void test_lookupPrivilegeValue(void)
{
    static const struct NameToLUID privs[] = {
     { "SeCreateTokenPrivilege", SE_CREATE_TOKEN_PRIVILEGE },
     { "SeAssignPrimaryTokenPrivilege", SE_ASSIGNPRIMARYTOKEN_PRIVILEGE },
     { "SeLockMemoryPrivilege", SE_LOCK_MEMORY_PRIVILEGE },
     { "SeIncreaseQuotaPrivilege", SE_INCREASE_QUOTA_PRIVILEGE },
     { "SeMachineAccountPrivilege", SE_MACHINE_ACCOUNT_PRIVILEGE },
     { "SeTcbPrivilege", SE_TCB_PRIVILEGE },
     { "SeSecurityPrivilege", SE_SECURITY_PRIVILEGE },
     { "SeTakeOwnershipPrivilege", SE_TAKE_OWNERSHIP_PRIVILEGE },
     { "SeLoadDriverPrivilege", SE_LOAD_DRIVER_PRIVILEGE },
     { "SeSystemProfilePrivilege", SE_SYSTEM_PROFILE_PRIVILEGE },
     { "SeSystemtimePrivilege", SE_SYSTEMTIME_PRIVILEGE },
     { "SeProfileSingleProcessPrivilege", SE_PROF_SINGLE_PROCESS_PRIVILEGE },
     { "SeIncreaseBasePriorityPrivilege", SE_INC_BASE_PRIORITY_PRIVILEGE },
     { "SeCreatePagefilePrivilege", SE_CREATE_PAGEFILE_PRIVILEGE },
     { "SeCreatePermanentPrivilege", SE_CREATE_PERMANENT_PRIVILEGE },
     { "SeBackupPrivilege", SE_BACKUP_PRIVILEGE },
     { "SeRestorePrivilege", SE_RESTORE_PRIVILEGE },
     { "SeShutdownPrivilege", SE_SHUTDOWN_PRIVILEGE },
     { "SeDebugPrivilege", SE_DEBUG_PRIVILEGE },
     { "SeAuditPrivilege", SE_AUDIT_PRIVILEGE },
     { "SeSystemEnvironmentPrivilege", SE_SYSTEM_ENVIRONMENT_PRIVILEGE },
     { "SeChangeNotifyPrivilege", SE_CHANGE_NOTIFY_PRIVILLEGE },
     { "SeRemoteShutdownPrivilege", SE_REMOTE_SHUTDOWN_PRIVILEGE },
     { "SeUndockPrivilege", SE_UNDOCK_PRIVILEGE },
     { "SeSyncAgentPrivilege", SE_SYNC_AGENT_PRIVILEGE },
     { "SeEnableDelegationPrivilege", SE_ENABLE_DELEGATION_PRIVILEGE },
     { "SeManageVolumePrivilege", SE_MANAGE_VOLUME_PRIVILEGE },
     { "SeImpersonatePrivilege", SE_IMPERSONATE_PRIVILEGE },
     { "SeCreateGlobalPrivilege", SE_CREATE_GLOBAL_PRIVILEGE },
    };
    BOOL (WINAPI *pLookupPrivilegeValueA)(LPCSTR, LPCSTR, PLUID);
    int i;
    LUID luid;
    BOOL ret;

    /* check whether it's available first */
    pLookupPrivilegeValueA = (void*)GetProcAddress(hmod, "LookupPrivilegeValueA");
    if (!pLookupPrivilegeValueA) return;
    ret = pLookupPrivilegeValueA(NULL, "SeCreateTokenPrivilege", &luid);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;

    /* check a bogus system name */
    ret = pLookupPrivilegeValueA("b0gu5.Nam3", "SeCreateTokenPrivilege", &luid);
    ok( !ret && (GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
                GetLastError() == RPC_S_INVALID_NET_ADDR) /* w2k8 */,
     "LookupPrivilegeValueA didn't fail with RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR: %d\n",
     GetLastError());
    /* check a NULL string */
    ret = pLookupPrivilegeValueA(NULL, 0, &luid);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeValueA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %d\n",
     GetLastError());
    /* check a bogus privilege name */
    ret = pLookupPrivilegeValueA(NULL, "SeBogusPrivilege", &luid);
    ok( !ret && GetLastError() == ERROR_NO_SUCH_PRIVILEGE,
     "LookupPrivilegeValueA didn't fail with ERROR_NO_SUCH_PRIVILEGE: %d\n",
     GetLastError());
    /* check case insensitive */
    ret = pLookupPrivilegeValueA(NULL, "sEcREATEtOKENpRIVILEGE", &luid);
    ok( ret,
     "LookupPrivilegeValueA(NULL, sEcREATEtOKENpRIVILEGE, &luid) failed: %d\n",
     GetLastError());
    for (i = 0; i < sizeof(privs) / sizeof(privs[0]); i++)
    {
        /* Not all privileges are implemented on all Windows versions, so
         * don't worry if the call fails
         */
        if (pLookupPrivilegeValueA(NULL, privs[i].name, &luid))
        {
            ok(luid.LowPart == privs[i].lowPart,
             "LookupPrivilegeValueA returned an invalid LUID for %s\n",
             privs[i].name);
        }
    }
}

static void test_luid(void)
{
    test_allocateLuid();
    test_lookupPrivilegeName();
    test_lookupPrivilegeValue();
}

static void test_FileSecurity(void)
{
    char directory[MAX_PATH];
    DWORD retval, outSize;
    BOOL result;
    BYTE buffer[0x40];

    pGetFileSecurityA = (fnGetFileSecurityA)
                    GetProcAddress( hmod, "GetFileSecurityA" );
    if( !pGetFileSecurityA )
        return;

    retval = GetTempPathA(sizeof(directory), directory);
    if (!retval) {
        trace("GetTempPathA failed\n");
        return;
    }

    strcpy(directory, "\\Should not exist");

    SetLastError(NO_ERROR);
    result = pGetFileSecurityA( directory,OWNER_SECURITY_INFORMATION,buffer,0x40,&outSize);
    ok(!result, "GetFileSecurityA should fail for not existing directories/files\n"); 
    ok( (GetLastError() == ERROR_FILE_NOT_FOUND ) ||
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) , 
        "last error ERROR_FILE_NOT_FOUND / ERROR_CALL_NOT_IMPLEMENTED (98) "
        "expected, got %d\n", GetLastError());
}

static void test_AccessCheck(void)
{
    PSID EveryoneSid = NULL, AdminSid = NULL, UsersSid = NULL;
    PACL Acl = NULL;
    SECURITY_DESCRIPTOR *SecurityDescriptor = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    GENERIC_MAPPING Mapping = { KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS };
    ACCESS_MASK Access;
    BOOL AccessStatus;
    HANDLE Token;
    HANDLE ProcessToken;
    BOOL ret;
    DWORD PrivSetLen;
    PRIVILEGE_SET *PrivSet;
    BOOL res;
    HMODULE NtDllModule;
    BOOLEAN Enabled;
    DWORD err;

    NtDllModule = GetModuleHandle("ntdll.dll");
    if (!NtDllModule)
    {
        skip("not running on NT, skipping test\n");
        return;
    }
    pRtlAdjustPrivilege = (fnRtlAdjustPrivilege)
                          GetProcAddress(NtDllModule, "RtlAdjustPrivilege");
    if (!pRtlAdjustPrivilege)
    {
        skip("missing RtlAdjustPrivilege, skipping test\n");
        return;
    }

    Acl = HeapAlloc(GetProcessHeap(), 0, 256);
    res = InitializeAcl(Acl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("ACLs not implemented - skipping tests\n");
        HeapFree(GetProcessHeap(), 0, Acl);
        return;
    }
    ok(res, "InitializeAcl failed with error %d\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdminSid);
    ok(res, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &UsersSid);
    ok(res, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    SecurityDescriptor = HeapAlloc(GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);

    res = InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    ok(res, "InitializeSecurityDescriptor failed with error %d\n", GetLastError());

    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());

    PrivSetLen = FIELD_OFFSET(PRIVILEGE_SET, Privilege[16]);
    PrivSet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PrivSetLen);
    PrivSet->PrivilegeCount = 16;

    res = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &ProcessToken);
    ok(res, "OpenProcessToken failed with error %d\n", GetLastError());

    pRtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, FALSE, TRUE, &Enabled);

    res = DuplicateToken(ProcessToken, SecurityImpersonation, &Token);
    ok(res, "DuplicateToken failed with error %d\n", GetLastError());

    /* SD without owner/group */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0xdeadbeef;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_QUERY_VALUE, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_SECURITY_DESCR, "AccessCheck should have "
       "failed with ERROR_INVALID_SECURITY_DESCR, instead of %d\n", err);
    ok(Access == 0xdeadbeef && AccessStatus == 0xdeadbeef,
       "Access and/or AccessStatus were changed!\n");

    /* Set owner and group */
    res = SetSecurityDescriptorOwner(SecurityDescriptor, AdminSid, FALSE);
    ok(res, "SetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    res = SetSecurityDescriptorGroup(SecurityDescriptor, UsersSid, TRUE);
    ok(res, "SetSecurityDescriptorGroup failed with error %d\n", GetLastError());

    /* Generic access mask */
    SetLastError(0xdeadbeef);
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_GENERIC_NOT_MAPPED, "AccessCheck should have failed "
       "with ERROR_GENERIC_NOT_MAPPED, instead of %d\n", err);
    ok(Access == 0xdeadbeef && AccessStatus == 0xdeadbeef,
       "Access and/or AccessStatus were changed!\n");

    /* sd with no dacl present */
    ret = SetSecurityDescriptorDacl(SecurityDescriptor, FALSE, NULL, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %d\n",
        GetLastError());

    /* sd with NULL dacl */
    ret = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, NULL, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %d\n",
        GetLastError());

    /* sd with blank dacl */
    ret = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    err = GetLastError();
    ok(!AccessStatus && err == ERROR_ACCESS_DENIED, "AccessCheck should have failed "
       "with ERROR_ACCESS_DENIED, instead of %d\n", err);
    ok(!Access, "Should have failed to grant any access, got 0x%08x\n", Access);

    res = AddAccessAllowedAce(Acl, ACL_REVISION, KEY_READ, EveryoneSid);
    ok(res, "AddAccessAllowedAce failed with error %d\n", GetLastError());

    res = AddAccessDeniedAce(Acl, ACL_REVISION, KEY_SET_VALUE, AdminSid);
    ok(res, "AddAccessDeniedAce failed with error %d\n", GetLastError());

    /* sd with dacl */
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %d\n",
        GetLastError());

    ret = AccessCheck(SecurityDescriptor, Token, MAXIMUM_ALLOWED, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    ok(AccessStatus,
        "AccessCheck failed to grant any access with error %d\n",
        GetLastError());
    trace("AccessCheck with MAXIMUM_ALLOWED got Access 0x%08x\n", Access);

    /* Access denied by SD */
    SetLastError(0xdeadbeef);
    ret = AccessCheck(SecurityDescriptor, Token, KEY_WRITE, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    err = GetLastError();
    ok(!AccessStatus && err == ERROR_ACCESS_DENIED, "AccessCheck should have failed "
       "with ERROR_ACCESS_DENIED, instead of %d\n", err);
    ok(!Access, "Should have failed to grant any access, got 0x%08x\n", Access);

    SetLastError(0);
    PrivSet->PrivilegeCount = 16;
    ret = AccessCheck(SecurityDescriptor, Token, ACCESS_SYSTEM_SECURITY, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret && !AccessStatus && GetLastError() == ERROR_PRIVILEGE_NOT_HELD,
        "AccessCheck should have failed with ERROR_PRIVILEGE_NOT_HELD, instead of %d\n",
        GetLastError());

    ret = ImpersonateLoggedOnUser(Token);
    ok(ret, "ImpersonateLoggedOnUser failed with error %d\n", GetLastError());
    ret = pRtlAdjustPrivilege(SE_SECURITY_PRIVILEGE, TRUE, TRUE, &Enabled);
    if (!ret)
    {
        SetLastError(0);
        PrivSet->PrivilegeCount = 16;
        ret = AccessCheck(SecurityDescriptor, Token, ACCESS_SYSTEM_SECURITY, &Mapping,
                          PrivSet, &PrivSetLen, &Access, &AccessStatus);
        ok(ret && AccessStatus && GetLastError() == 0,
            "AccessCheck should have succeeded, error %d\n",
            GetLastError());
        ok(Access == ACCESS_SYSTEM_SECURITY,
            "Access should be equal to ACCESS_SYSTEM_SECURITY instead of 0x%08x\n",
            Access);
    }
    else
        trace("Couldn't get SE_SECURITY_PRIVILEGE (0x%08x), skipping ACCESS_SYSTEM_SECURITY test\n",
            ret);
    ret = RevertToSelf();
    ok(ret, "RevertToSelf failed with error %d\n", GetLastError());

    /* test INHERIT_ONLY_ACE */
    ret = InitializeAcl(Acl, 256, ACL_REVISION);
    ok(ret, "InitializeAcl failed with error %d\n", GetLastError());

    /* NT doesn't have AddAccessAllowedAceEx. Skipping this call/test doesn't influence
     * the next ones.
     */
    if (pAddAccessAllowedAceEx)
    {
        ret = pAddAccessAllowedAceEx(Acl, ACL_REVISION, INHERIT_ONLY_ACE, KEY_READ, EveryoneSid);
        ok(ret, "AddAccessAllowedAceEx failed with error %d\n", GetLastError());
    }
    else
        skip("AddAccessAllowedAceEx is not available\n");

    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    err = GetLastError();
    ok(!AccessStatus && err == ERROR_ACCESS_DENIED, "AccessCheck should have failed "
       "with ERROR_ACCESS_DENIED, instead of %d\n", err);
    ok(!Access, "Should have failed to grant any access, got 0x%08x\n", Access);

    CloseHandle(Token);

    res = DuplicateToken(ProcessToken, SecurityAnonymous, &Token);
    ok(res, "DuplicateToken failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = AccessCheck(SecurityDescriptor, Token, MAXIMUM_ALLOWED, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_BAD_IMPERSONATION_LEVEL, "AccessCheck should have failed "
       "with ERROR_BAD_IMPERSONATION_LEVEL, instead of %d\n", err);

    CloseHandle(Token);

    SetLastError(0xdeadbeef);
    ret = AccessCheck(SecurityDescriptor, ProcessToken, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NO_IMPERSONATION_TOKEN, "AccessCheck should have failed "
       "with ERROR_NO_IMPERSONATION_TOKEN, instead of %d\n", err);

    CloseHandle(ProcessToken);

    if (EveryoneSid)
        FreeSid(EveryoneSid);
    if (AdminSid)
        FreeSid(AdminSid);
    if (UsersSid)
        FreeSid(UsersSid);
    HeapFree(GetProcessHeap(), 0, Acl);
    HeapFree(GetProcessHeap(), 0, SecurityDescriptor);
    HeapFree(GetProcessHeap(), 0, PrivSet);
}

/* test GetTokenInformation for the various attributes */
static void test_token_attr(void)
{
    HANDLE Token, ImpersonationToken;
    DWORD Size;
    TOKEN_PRIVILEGES *Privileges;
    TOKEN_GROUPS *Groups;
    TOKEN_USER *User;
    BOOL ret;
    DWORD i, GLE;
    LPSTR SidString;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;

    /* cygwin-like use case */
    SetLastError(0xdeadbeef);
    ret = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("OpenProcessToken is not implemented\n");
        return;
    }
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());
    if (ret)
    {
        BYTE buf[1024];
        Size = sizeof(buf);
        ret = GetTokenInformation(Token, TokenUser,(void*)buf, Size, &Size);
        ok(ret, "GetTokenInformation failed with error %d\n", GetLastError());
        Size = sizeof(ImpersonationLevel);
        ret = GetTokenInformation(Token, TokenImpersonationLevel, &ImpersonationLevel, Size, &Size);
        GLE = GetLastError();
        ok(!ret && (GLE == ERROR_INVALID_PARAMETER), "GetTokenInformation(TokenImpersonationLevel) on primary token should have failed with ERROR_INVALID_PARAMETER instead of %d\n", GLE);
        CloseHandle(Token);
    }

    if(!pConvertSidToStringSidA)
    {
        skip("ConvertSidToStringSidA is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE, &Token);
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());

    /* groups */
    ret = GetTokenInformation(Token, TokenGroups, NULL, 0, &Size);
    Groups = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenGroups, Groups, Size, &Size);
    ok(ret, "GetTokenInformation(TokenGroups) failed with error %d\n", GetLastError());
    trace("TokenGroups:\n");
    for (i = 0; i < Groups->GroupCount; i++)
    {
        DWORD NameLength = 255;
        TCHAR Name[255];
        DWORD DomainLength = 255;
        TCHAR Domain[255];
        SID_NAME_USE SidNameUse;
        pConvertSidToStringSidA(Groups->Groups[i].Sid, &SidString);
        Name[0] = '\0';
        Domain[0] = '\0';
        ret = LookupAccountSid(NULL, Groups->Groups[i].Sid, Name, &NameLength, Domain, &DomainLength, &SidNameUse);
        if (ret)
            trace("\t%s, %s\\%s use: %d attr: 0x%08x\n", SidString, Domain, Name, SidNameUse, Groups->Groups[i].Attributes);
        else
            trace("\t%s, attr: 0x%08x LookupAccountSid failed with error %d\n", SidString, Groups->Groups[i].Attributes, GetLastError());
        LocalFree(SidString);
    }
    HeapFree(GetProcessHeap(), 0, Groups);

    /* user */
    ret = GetTokenInformation(Token, TokenUser, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());
    User = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenUser, User, Size, &Size);
    ok(ret,
        "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());

    pConvertSidToStringSidA(User->User.Sid, &SidString);
    trace("TokenUser: %s attr: 0x%08x\n", SidString, User->User.Attributes);
    LocalFree(SidString);
    HeapFree(GetProcessHeap(), 0, User);

    /* privileges */
    ret = GetTokenInformation(Token, TokenPrivileges, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenPrivileges) failed with error %d\n", GetLastError());
    Privileges = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenPrivileges, Privileges, Size, &Size);
    ok(ret,
        "GetTokenInformation(TokenPrivileges) failed with error %d\n", GetLastError());
    trace("TokenPrivileges:\n");
    for (i = 0; i < Privileges->PrivilegeCount; i++)
    {
        TCHAR Name[256];
        DWORD NameLen = sizeof(Name)/sizeof(Name[0]);
        LookupPrivilegeName(NULL, &Privileges->Privileges[i].Luid, Name, &NameLen);
        trace("\t%s, 0x%x\n", Name, Privileges->Privileges[i].Attributes);
    }
    HeapFree(GetProcessHeap(), 0, Privileges);

    ret = DuplicateToken(Token, SecurityAnonymous, &ImpersonationToken);
    ok(ret, "DuplicateToken failed with error %d\n", GetLastError());

    Size = sizeof(ImpersonationLevel);
    ret = GetTokenInformation(ImpersonationToken, TokenImpersonationLevel, &ImpersonationLevel, Size, &Size);
    ok(ret, "GetTokenInformation(TokenImpersonationLevel) failed with error %d\n", GetLastError());
    ok(ImpersonationLevel == SecurityAnonymous, "ImpersonationLevel should have been SecurityAnonymous instead of %d\n", ImpersonationLevel);

    CloseHandle(ImpersonationToken);
    CloseHandle(Token);
}

typedef union _MAX_SID
{
    SID sid;
    char max[SECURITY_MAX_SID_SIZE];
} MAX_SID;

static void test_sid_str(PSID * sid)
{
    char *str_sid;
    BOOL ret = pConvertSidToStringSidA(sid, &str_sid);
    ok(ret, "ConvertSidToStringSidA() failed: %d\n", GetLastError());
    if (ret)
    {
        char account[MAX_PATH], domain[MAX_PATH];
        SID_NAME_USE use;
        DWORD acc_size = MAX_PATH;
        DWORD dom_size = MAX_PATH;
        ret = LookupAccountSid(NULL, sid, account, &acc_size, domain, &dom_size, &use);
        ok(ret || (!ret && (GetLastError() == ERROR_NONE_MAPPED)),
           "LookupAccountSid(%s) failed: %d\n", str_sid, GetLastError());
        if (ret)
            trace(" %s %s\\%s %d\n", str_sid, domain, account, use);
        else if (GetLastError() == ERROR_NONE_MAPPED)
            trace(" %s couldn't be mapped\n", str_sid);
        LocalFree(str_sid);
    }
}

struct well_known_sid_value
{
    BOOL without_domain;
    const char *sid_string;
} well_known_sid_values[] = {
/*  0 */ {TRUE, "S-1-0-0"},  {TRUE, "S-1-1-0"},  {TRUE, "S-1-2-0"},  {TRUE, "S-1-3-0"},
/*  4 */ {TRUE, "S-1-3-1"},  {TRUE, "S-1-3-2"},  {TRUE, "S-1-3-3"},  {TRUE, "S-1-5"},
/*  8 */ {FALSE, "S-1-5-1"}, {TRUE, "S-1-5-2"},  {TRUE, "S-1-5-3"},  {TRUE, "S-1-5-4"},
/* 12 */ {TRUE, "S-1-5-6"},  {TRUE, "S-1-5-7"},  {TRUE, "S-1-5-8"},  {TRUE, "S-1-5-9"},
/* 16 */ {TRUE, "S-1-5-10"}, {TRUE, "S-1-5-11"}, {TRUE, "S-1-5-12"}, {TRUE, "S-1-5-13"},
/* 20 */ {TRUE, "S-1-5-14"}, {FALSE, NULL},      {TRUE, "S-1-5-18"}, {TRUE, "S-1-5-19"},
/* 24 */ {TRUE, "S-1-5-20"}, {TRUE, "S-1-5-32"},
/* 26 */ {FALSE, "S-1-5-32-544"}, {TRUE, "S-1-5-32-545"}, {TRUE, "S-1-5-32-546"},
/* 29 */ {TRUE, "S-1-5-32-547"},  {TRUE, "S-1-5-32-548"}, {TRUE, "S-1-5-32-549"},
/* 32 */ {TRUE, "S-1-5-32-550"},  {TRUE, "S-1-5-32-551"}, {TRUE, "S-1-5-32-552"},
/* 35 */ {TRUE, "S-1-5-32-554"},  {TRUE, "S-1-5-32-555"}, {TRUE, "S-1-5-32-556"},
/* 38 */ {FALSE, "S-1-5-21-12-23-34-45-56-500"}, {FALSE, "S-1-5-21-12-23-34-45-56-501"},
/* 40 */ {FALSE, "S-1-5-21-12-23-34-45-56-502"}, {FALSE, "S-1-5-21-12-23-34-45-56-512"},
/* 42 */ {FALSE, "S-1-5-21-12-23-34-45-56-513"}, {FALSE, "S-1-5-21-12-23-34-45-56-514"},
/* 44 */ {FALSE, "S-1-5-21-12-23-34-45-56-515"}, {FALSE, "S-1-5-21-12-23-34-45-56-516"},
/* 46 */ {FALSE, "S-1-5-21-12-23-34-45-56-517"}, {FALSE, "S-1-5-21-12-23-34-45-56-518"},
/* 48 */ {FALSE, "S-1-5-21-12-23-34-45-56-519"}, {FALSE, "S-1-5-21-12-23-34-45-56-520"},
/* 50 */ {FALSE, "S-1-5-21-12-23-34-45-56-553"},
/* Added in Windows Server 2003 */
/* 51 */ {TRUE, "S-1-5-64-10"},   {TRUE, "S-1-5-64-21"},   {TRUE, "S-1-5-64-14"},
/* 54 */ {TRUE, "S-1-5-15"},      {TRUE, "S-1-5-1000"},    {FALSE, "S-1-5-32-557"},
/* 57 */ {TRUE, "S-1-5-32-558"},  {TRUE, "S-1-5-32-559"},  {TRUE, "S-1-5-32-560"},
/* 60 */ {TRUE, "S-1-5-32-561"}, {TRUE, "S-1-5-32-562"},
/* Added in Windows Vista: */
/* 62 */ {TRUE, "S-1-5-32-568"},
/* 63 */ {TRUE, "S-1-5-17"},      {FALSE, "S-1-5-32-569"}, {TRUE, "S-1-16-0"},
/* 66 */ {TRUE, "S-1-16-4096"},   {TRUE, "S-1-16-8192"},   {TRUE, "S-1-16-12288"},
/* 69 */ {TRUE, "S-1-16-16384"},  {TRUE, "S-1-5-33"},      {TRUE, "S-1-3-4"},
/* 72 */ {FALSE, "S-1-5-21-12-23-34-45-56-571"},  {FALSE, "S-1-5-21-12-23-34-45-56-572"},
/* 74 */ {TRUE, "S-1-5-22"}, {FALSE, "S-1-5-21-12-23-34-45-56-521"}, {TRUE, "S-1-5-32-573"}
};

static void test_CreateWellKnownSid()
{
    SID_IDENTIFIER_AUTHORITY ident = { SECURITY_NT_AUTHORITY };
    PSID domainsid;
    int i;

    if (!pCreateWellKnownSid)
    {
        skip("CreateWellKnownSid not available\n");
        return;
    }

    /* a domain sid usually have three subauthorities but we test that CreateWellKnownSid doesn't check it */
    AllocateAndInitializeSid(&ident, 6, SECURITY_NT_NON_UNIQUE, 12, 23, 34, 45, 56, 0, 0, &domainsid);

    for (i = 0; i < sizeof(well_known_sid_values)/sizeof(well_known_sid_values[0]); i++)
    {
        struct well_known_sid_value *value = &well_known_sid_values[i];
        char sid_buffer[SECURITY_MAX_SID_SIZE];
        LPSTR str;
        DWORD cb;

        if (value->sid_string == NULL)
            continue;

        if (i > WinAccountRasAndIasServersSid)
        {
            /* These SIDs aren't implemented by all Windows versions - detect it and break the loop */
            cb = sizeof(sid_buffer);
            if (!pCreateWellKnownSid(i, domainsid, sid_buffer, &cb))
            {
                skip("Well known SIDs starting from %d are not implemented\n", i);
                break;
            }
        }

        cb = sizeof(sid_buffer);
        ok(pCreateWellKnownSid(i, value->without_domain ? NULL : domainsid, sid_buffer, &cb), "Couldn't create well known sid %d\n", i);
        expect_eq(GetSidLengthRequired(*GetSidSubAuthorityCount(sid_buffer)), cb, DWORD, "%d");
        ok(IsValidSid(sid_buffer), "The sid is not valid\n");
        ok(pConvertSidToStringSidA(sid_buffer, &str), "Couldn't convert SID to string\n");
        ok(strcmp(str, value->sid_string) == 0, "SID mismatch - expected %s, got %s\n",
            value->sid_string, str);
        LocalFree(str);

        if (value->without_domain)
        {
            char buf2[SECURITY_MAX_SID_SIZE];
            cb = sizeof(buf2);
            ok(pCreateWellKnownSid(i, domainsid, buf2, &cb), "Couldn't create well known sid %d with optional domain\n", i);
            expect_eq(GetSidLengthRequired(*GetSidSubAuthorityCount(sid_buffer)), cb, DWORD, "%d");
            ok(memcmp(buf2, sid_buffer, cb) == 0, "SID create with domain is different than without (%d)\n", i);
        }
    }
}

static void test_LookupAccountSid(void)
{
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    CHAR accountA[MAX_PATH], domainA[MAX_PATH];
    DWORD acc_sizeA, dom_sizeA;
    DWORD real_acc_sizeA, real_dom_sizeA;
    WCHAR accountW[MAX_PATH], domainW[MAX_PATH];
    DWORD acc_sizeW, dom_sizeW;
    DWORD real_acc_sizeW, real_dom_sizeW;
    PSID pUsersSid = NULL;
    SID_NAME_USE use;
    BOOL ret;
    DWORD size;
    MAX_SID  max_sid;
    CHAR *str_sidA;
    int i;

    /* native windows crashes if account size, domain size, or name use is NULL */

    ret = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &pUsersSid);
    ok(ret || (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED),
       "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    /* not running on NT so give up */
    if (!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
        return;

    real_acc_sizeA = MAX_PATH;
    real_dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &real_acc_sizeA, domainA, &real_dom_sizeA, &use);
    ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");

    /* try NULL account */
    acc_sizeA = MAX_PATH;
    dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, NULL, &acc_sizeA, domainA, &dom_sizeA, &use);
    ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");

    /* try NULL domain */
    acc_sizeA = MAX_PATH;
    dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, NULL, &dom_sizeA, &use);
    ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");

    /* try a small account buffer */
    acc_sizeA = 1;
    dom_sizeA = MAX_PATH;
    accountA[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    ok(!ret, "LookupAccountSidA() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidA() Expected ERROR_NOT_ENOUGH_MEMORY, got %u\n", GetLastError());

    /* try a 0 sized account buffer */
    acc_sizeA = 0;
    dom_sizeA = MAX_PATH;
    accountA[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeA == real_acc_sizeA + 1,
       "LookupAccountSidA() Expected acc_size = %u, got %u\n",
       real_acc_sizeA + 1, acc_sizeA);

    /* try a 0 sized account buffer */
    acc_sizeA = 0;
    dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, NULL, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeA == real_acc_sizeA + 1,
       "LookupAccountSid() Expected acc_size = %u, got %u\n",
       real_acc_sizeA + 1, acc_sizeA);

    /* try a small domain buffer */
    dom_sizeA = 1;
    acc_sizeA = MAX_PATH;
    accountA[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    ok(!ret, "LookupAccountSidA() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidA() Expected ERROR_NOT_ENOUGH_MEMORY, got %u\n", GetLastError());

    /* try a 0 sized domain buffer */
    dom_sizeA = 0;
    acc_sizeA = MAX_PATH;
    accountA[0] = 0;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeA == real_dom_sizeA + 1,
       "LookupAccountSidA() Expected dom_size = %u, got %u\n",
       real_dom_sizeA + 1, dom_sizeA);

    /* try a 0 sized domain buffer */
    dom_sizeA = 0;
    acc_sizeA = MAX_PATH;
    ret = LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, NULL, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeA == real_dom_sizeA + 1,
       "LookupAccountSidA() Expected dom_size = %u, got %u\n",
       real_dom_sizeA + 1, dom_sizeA);

    real_acc_sizeW = MAX_PATH;
    real_dom_sizeW = MAX_PATH;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &real_acc_sizeW, domainW, &real_dom_sizeW, &use);
    ok(ret, "LookupAccountSidW() Expected TRUE, got FALSE\n");

    /* native windows crashes if domainW or accountW is NULL */

    /* try a small account buffer */
    acc_sizeW = 1;
    dom_sizeW = MAX_PATH;
    accountW[0] = 0;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    ok(!ret, "LookupAccountSidW() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidW() Expected ERROR_NOT_ENOUGH_MEMORY, got %u\n", GetLastError());

    /* try a 0 sized account buffer */
    acc_sizeW = 0;
    dom_sizeW = MAX_PATH;
    accountW[0] = 0;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeW == real_acc_sizeW + 1,
       "LookupAccountSidW() Expected acc_size = %u, got %u\n",
       real_acc_sizeW + 1, acc_sizeW);

    /* try a 0 sized account buffer */
    acc_sizeW = 0;
    dom_sizeW = MAX_PATH;
    ret = LookupAccountSidW(NULL, pUsersSid, NULL, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeW == real_acc_sizeW + 1,
       "LookupAccountSidW() Expected acc_size = %u, got %u\n",
       real_acc_sizeW + 1, acc_sizeW);

    /* try a small domain buffer */
    dom_sizeW = 1;
    acc_sizeW = MAX_PATH;
    accountW[0] = 0;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    ok(!ret, "LookupAccountSidW() Expected FALSE got TRUE\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "LookupAccountSidW() Expected ERROR_NOT_ENOUGH_MEMORY, got %u\n", GetLastError());

    /* try a 0 sized domain buffer */
    dom_sizeW = 0;
    acc_sizeW = MAX_PATH;
    accountW[0] = 0;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeW == real_dom_sizeW + 1,
       "LookupAccountSidW() Expected dom_size = %u, got %u\n",
       real_dom_sizeW + 1, dom_sizeW);

    /* try a 0 sized domain buffer */
    dom_sizeW = 0;
    acc_sizeW = MAX_PATH;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, NULL, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeW == real_dom_sizeW + 1,
       "LookupAccountSidW() Expected dom_size = %u, got %u\n",
       real_dom_sizeW + 1, dom_sizeW);

    FreeSid(pUsersSid);

    if (pCreateWellKnownSid && pConvertSidToStringSidA)
    {
        trace("Well Known SIDs:\n");
        for (i = 0; i <= 60; i++)
        {
            size = SECURITY_MAX_SID_SIZE;
            if (pCreateWellKnownSid(i, NULL, &max_sid.sid, &size))
            {
                if (pConvertSidToStringSidA(&max_sid.sid, &str_sidA))
                {
                    acc_sizeA = MAX_PATH;
                    dom_sizeA = MAX_PATH;
                    if (LookupAccountSidA(NULL, &max_sid.sid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use))
                        trace(" %d: %s %s\\%s %d\n", i, str_sidA, domainA, accountA, use);
                    LocalFree(str_sidA);
                }
            }
            else
            {
                if (GetLastError() != ERROR_INVALID_PARAMETER)
                    trace(" CreateWellKnownSid(%d) failed: %d\n", i, GetLastError());
                else
                    trace(" %d: not supported\n", i);
            }
        }

        pLsaQueryInformationPolicy = (fnLsaQueryInformationPolicy)GetProcAddress( hmod, "LsaQueryInformationPolicy");
        pLsaOpenPolicy = (fnLsaOpenPolicy)GetProcAddress( hmod, "LsaOpenPolicy");
        pLsaFreeMemory = (fnLsaFreeMemory)GetProcAddress( hmod, "LsaFreeMemory");
        pLsaClose = (fnLsaClose)GetProcAddress( hmod, "LsaClose");

        if (pLsaQueryInformationPolicy && pLsaOpenPolicy && pLsaFreeMemory && pLsaClose)
        {
            NTSTATUS status;
            LSA_HANDLE handle;
            LSA_OBJECT_ATTRIBUTES object_attributes;

            ZeroMemory(&object_attributes, sizeof(object_attributes));
            object_attributes.Length = sizeof(object_attributes);

            status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_ALL_ACCESS, &handle);
            ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
               "LsaOpenPolicy(POLICY_ALL_ACCESS) returned 0x%08x\n", status);

            /* try a more restricted access mask if necessary */
            if (status == STATUS_ACCESS_DENIED) {
                trace("LsaOpenPolicy(POLICY_ALL_ACCESS) failed, trying POLICY_VIEW_LOCAL_INFORMATION\n");
                status = pLsaOpenPolicy( NULL, &object_attributes, POLICY_VIEW_LOCAL_INFORMATION, &handle);
                ok(status == STATUS_SUCCESS, "LsaOpenPolicy(POLICY_VIEW_LOCAL_INFORMATION) returned 0x%08x\n", status);
            }

            if (status == STATUS_SUCCESS)
            {
                PPOLICY_ACCOUNT_DOMAIN_INFO info;
                status = pLsaQueryInformationPolicy(handle, PolicyAccountDomainInformation, (PVOID*)&info);
                ok(status == STATUS_SUCCESS, "LsaQueryInformationPolicy() failed, returned 0x%08x\n", status);
                if (status == STATUS_SUCCESS)
                {
                    ok(info->DomainSid!=0, "LsaQueryInformationPolicy(PolicyAccountDomainInformation) missing SID\n");
                    if (info->DomainSid)
                    {
                        int count = *GetSidSubAuthorityCount(info->DomainSid);
                        CopySid(GetSidLengthRequired(count), &max_sid, info->DomainSid);
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_USER_RID_ADMIN;
                        max_sid.sid.SubAuthorityCount = count + 1;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_USER_RID_GUEST;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_USERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_GUESTS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_COMPUTERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_CONTROLLERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_CERT_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_SCHEMA_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_ENTERPRISE_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_GROUP_RID_POLICY_ADMINS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = DOMAIN_ALIAS_RID_RAS_SERVERS;
                        test_sid_str((PSID)&max_sid.sid);
                        max_sid.sid.SubAuthority[count] = 1000;	/* first user account */
                        test_sid_str((PSID)&max_sid.sid);
                    }

                    pLsaFreeMemory((LPVOID)info);
                }

                status = pLsaClose(handle);
                ok(status == STATUS_SUCCESS, "LsaClose() failed, returned 0x%08x\n", status);
            }
        }
    }
}

static void get_sid_info(PSID psid, LPSTR *user, LPSTR *dom)
{
    static CHAR account[UNLEN + 1];
    static CHAR domain[UNLEN + 1];
    DWORD size, dom_size;
    SID_NAME_USE use;

    *user = account;
    *dom = domain;

    size = dom_size = UNLEN + 1;
    account[0] = '\0';
    domain[0] = '\0';
    LookupAccountSidA(NULL, psid, account, &size, domain, &dom_size, &use);
}

static void test_LookupAccountName(void)
{
    DWORD sid_size, domain_size, user_size;
    DWORD sid_save, domain_save;
    CHAR user_name[UNLEN + 1];
    SID_NAME_USE sid_use;
    LPSTR domain, account, sid_dom;
    PSID psid;
    BOOL ret;

    /* native crashes if (assuming all other parameters correct):
     *  - peUse is NULL
     *  - Sid is NULL and cbSid is > 0
     *  - cbSid or cchReferencedDomainName are NULL
     *  - ReferencedDomainName is NULL and cchReferencedDomainName is the correct size
     */

    user_size = UNLEN + 1;
    SetLastError(0xdeadbeef);
    ret = GetUserNameA(user_name, &user_size);
    if (!ret && (GetLastError() == ERROR_NOT_LOGGED_ON))
    {
        /* Probably on win9x where the user used 'Cancel' instead of properly logging in */
        skip("Cannot get the user name (win9x and not logged in properly)\n");
        return;
    }
    ok(ret, "Failed to get user name : %d\n", GetLastError());

    /* get sizes */
    sid_size = 0;
    domain_size = 0;
    sid_use = 0xcafebabe;
    SetLastError(0xdeadbeef);
    ret = LookupAccountNameA(NULL, user_name, NULL, &sid_size, NULL, &domain_size, &sid_use);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("LookupAccountNameA is not implemented\n");
        return;
    }
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(sid_size != 0, "Expected non-zero sid size\n");
    ok(domain_size != 0, "Expected non-zero domain size\n");
    ok(sid_use == 0xcafebabe, "Expected 0xcafebabe, got %d\n", sid_use);

    sid_save = sid_size;
    domain_save = domain_size;

    psid = HeapAlloc(GetProcessHeap(), 0, sid_size);
    domain = HeapAlloc(GetProcessHeap(), 0, domain_size);

    /* try valid account name */
    ret = LookupAccountNameA(NULL, user_name, psid, &sid_size, domain, &domain_size, &sid_use);
    get_sid_info(psid, &account, &sid_dom);
    ok(ret, "Failed to lookup account name\n");
    ok(sid_size == GetLengthSid(psid), "Expected %d, got %d\n", GetLengthSid(psid), sid_size);
    todo_wine
    {
        ok(!lstrcmp(account, user_name), "Expected %s, got %s\n", user_name, account);
        ok(!lstrcmp(domain, sid_dom), "Expected %s, got %s\n", sid_dom, domain);
        ok(domain_size == domain_save - 1, "Expected %d, got %d\n", domain_save - 1, domain_size);
        ok(lstrlen(domain) == domain_size, "Expected %d\n", lstrlen(domain));
        ok(sid_use == SidTypeUser, "Expected SidTypeUser, got %d\n", sid_use);
    }
    domain_size = domain_save;
    sid_size = sid_save;

    if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH)
    {
        skip("Non-english locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ret = LookupAccountNameA(NULL, "Everyone", psid, &sid_size, domain, &domain_size, &sid_use);
        get_sid_info(psid, &account, &sid_dom);
        ok(ret, "Failed to lookup account name\n");
        ok(sid_size != 0, "sid_size was zero\n");
        ok(!lstrcmp(account, "Everyone"), "Expected Everyone, got %s\n", account);
        todo_wine
        ok(!lstrcmp(domain, sid_dom), "Expected %s, got %s\n", sid_dom, domain);
        ok(domain_size == 0, "Expected 0, got %d\n", domain_size);
        todo_wine
        ok(lstrlen(domain) == domain_size, "Expected %d, got %d\n", lstrlen(domain), domain_size);
        ok(sid_use == SidTypeWellKnownGroup, "Expected SidTypeUser, got %d\n", sid_use);
        domain_size = domain_save;
    }

    /* NULL Sid with zero sid size */
    SetLastError(0xdeadbeef);
    sid_size = 0;
    ret = LookupAccountNameA(NULL, user_name, NULL, &sid_size, domain, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(sid_size == sid_save, "Expected %d, got %d\n", sid_save, sid_size);
    ok(domain_size == domain_save, "Expected %d, got %d\n", domain_save, domain_size);

    /* try cchReferencedDomainName - 1 */
    SetLastError(0xdeadbeef);
    domain_size--;
    ret = LookupAccountNameA(NULL, user_name, NULL, &sid_size, domain, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(sid_size == sid_save, "Expected %d, got %d\n", sid_save, sid_size);
    ok(domain_size == domain_save, "Expected %d, got %d\n", domain_save, domain_size);

    /* NULL ReferencedDomainName with zero domain name size */
    SetLastError(0xdeadbeef);
    domain_size = 0;
    ret = LookupAccountNameA(NULL, user_name, psid, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(sid_size == sid_save, "Expected %d, got %d\n", sid_save, sid_size);
    ok(domain_size == domain_save, "Expected %d, got %d\n", domain_save, domain_size);

    HeapFree(GetProcessHeap(), 0, psid);
    HeapFree(GetProcessHeap(), 0, domain);

    /* get sizes for NULL account name */
    sid_size = 0;
    domain_size = 0;
    sid_use = 0xcafebabe;
    SetLastError(0xdeadbeef);
    ret = LookupAccountNameA(NULL, NULL, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(sid_size != 0, "Expected non-zero sid size\n");
    ok(domain_size != 0, "Expected non-zero domain size\n");
    ok(sid_use == 0xcafebabe, "Expected 0xcafebabe, got %d\n", sid_use);

    psid = HeapAlloc(GetProcessHeap(), 0, sid_size);
    domain = HeapAlloc(GetProcessHeap(), 0, domain_size);

    /* try NULL account name */
    ret = LookupAccountNameA(NULL, NULL, psid, &sid_size, domain, &domain_size, &sid_use);
    get_sid_info(psid, &account, &sid_dom);
    ok(ret, "Failed to lookup account name\n");
    todo_wine
    {
        /* Using a fixed string will not work on different locales */
        ok(!lstrcmp(account, domain),
           "Got %s for account and %s for domain, these should be the same\n",
           account, domain);
        ok(sid_use == SidTypeDomain, "Expected SidTypeDomain, got %d\n", SidTypeDomain);
    }

    /* try an invalid account name */
    SetLastError(0xdeadbeef);
    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA(NULL, "oogabooga", NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    todo_wine
    {
        ok(GetLastError() == ERROR_NONE_MAPPED ||
           broken(GetLastError() == ERROR_TRUSTED_RELATIONSHIP_FAILURE),
           "Expected ERROR_NONE_MAPPED, got %d\n", GetLastError());
        ok(sid_size == 0, "Expected 0, got %d\n", sid_size);
        ok(domain_size == 0, "Expected 0, got %d\n", domain_size);
    }

    HeapFree(GetProcessHeap(), 0, psid);
    HeapFree(GetProcessHeap(), 0, domain);
}

static void test_security_descriptor(void)
{
    SECURITY_DESCRIPTOR sd;
    char buf[8192];
    DWORD size;
    BOOL isDefault, isPresent, ret;
    PACL pacl;
    PSID psid;

    SetLastError(0xdeadbeef);
    ret = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("InitializeSecurityDescriptor is not implemented\n");
        return;
    }

    ok(GetSecurityDescriptorOwner(&sd, &psid, &isDefault), "GetSecurityDescriptorOwner failed\n");
    expect_eq(psid, NULL, PSID, "%p");
    expect_eq(isDefault, FALSE, BOOL, "%d");
    sd.Control |= SE_DACL_PRESENT | SE_SACL_PRESENT;

    SetLastError(0xdeadbeef);
    size = 5;
    expect_eq(MakeSelfRelativeSD(&sd, buf, &size), FALSE, BOOL, "%d");
    expect_eq(GetLastError(), ERROR_INSUFFICIENT_BUFFER, DWORD, "%u");
    ok(size > 5, "Size not increased\n");
    if (size <= 8192)
    {
        expect_eq(MakeSelfRelativeSD(&sd, buf, &size), TRUE, BOOL, "%d");
        ok(GetSecurityDescriptorOwner(&sd, &psid, &isDefault), "GetSecurityDescriptorOwner failed\n");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
        ok(GetSecurityDescriptorGroup(&sd, &psid, &isDefault), "GetSecurityDescriptorGroup failed\n");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
        ok(GetSecurityDescriptorDacl(&sd, &isPresent, &pacl, &isDefault), "GetSecurityDescriptorDacl failed\n");
        expect_eq(isPresent, TRUE, BOOL, "%d");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
        ok(GetSecurityDescriptorSacl(&sd, &isPresent, &pacl, &isDefault), "GetSecurityDescriptorSacl failed\n");
        expect_eq(isPresent, TRUE, BOOL, "%d");
        expect_eq(psid, NULL, PSID, "%p");
        expect_eq(isDefault, FALSE, BOOL, "%d");
    }
}

#define TEST_GRANTED_ACCESS(a,b) test_granted_access(a,b,0,__LINE__)
#define TEST_GRANTED_ACCESS2(a,b,c) test_granted_access(a,b,c,__LINE__)
static void test_granted_access(HANDLE handle, ACCESS_MASK access,
                                ACCESS_MASK alt, int line)
{
    OBJECT_BASIC_INFORMATION obj_info;
    NTSTATUS status;

    if (!pNtQueryObject)
    {
        skip_(__FILE__, line)("Not NT platform - skipping tests\n");
        return;
    }

    status = pNtQueryObject( handle, ObjectBasicInformation, &obj_info,
                             sizeof(obj_info), NULL );
    ok_(__FILE__, line)(!status, "NtQueryObject with err: %08x\n", status);
    if (alt)
        ok_(__FILE__, line)(obj_info.GrantedAccess == access ||
            obj_info.GrantedAccess == alt, "Granted access should be 0x%08x "
            "or 0x%08x, instead of 0x%08x\n", access, alt, obj_info.GrantedAccess);
    else
        ok_(__FILE__, line)(obj_info.GrantedAccess == access, "Granted access should "
            "be 0x%08x, instead of 0x%08x\n", access, obj_info.GrantedAccess);
}

#define CHECK_SET_SECURITY(o,i,e) \
    do{ \
        BOOL res; \
        DWORD err; \
        SetLastError( 0xdeadbeef ); \
        res = SetKernelObjectSecurity( o, i, SecurityDescriptor ); \
        err = GetLastError(); \
        if (e == ERROR_SUCCESS) \
            ok(res, "SetKernelObjectSecurity failed with %d\n", err); \
        else \
            ok(!res && err == e, "SetKernelObjectSecurity should have failed " \
               "with %s, instead of %d\n", #e, err); \
    }while(0)

static void test_process_security(void)
{
    BOOL res;
    char owner[32], group[32];
    PSID AdminSid = NULL, UsersSid = NULL;
    PACL Acl = NULL;
    SECURITY_DESCRIPTOR *SecurityDescriptor = NULL;
    char buffer[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    SECURITY_ATTRIBUTES psa;
    HANDLE token, event;
    DWORD tmp;

    Acl = HeapAlloc(GetProcessHeap(), 0, 256);
    res = InitializeAcl(Acl, 256, ACL_REVISION);
    if (!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("ACLs not implemented - skipping tests\n");
        HeapFree(GetProcessHeap(), 0, Acl);
        return;
    }
    ok(res, "InitializeAcl failed with error %d\n", GetLastError());

    /* get owner from the token we might be running as a user not admin */
    res = OpenProcessToken( GetCurrentProcess(), MAXIMUM_ALLOWED, &token );
    ok(res, "OpenProcessToken failed with error %d\n", GetLastError());
    if (!res)
    {
        HeapFree(GetProcessHeap(), 0, Acl);
        return;
    }

    res = GetTokenInformation( token, TokenOwner, owner, sizeof(owner), &tmp );
    ok(res, "GetTokenInformation failed with error %d\n", GetLastError());
    AdminSid = ((TOKEN_OWNER*)owner)->Owner;
    res = GetTokenInformation( token, TokenPrimaryGroup, group, sizeof(group), &tmp );
    ok(res, "GetTokenInformation failed with error %d\n", GetLastError());
    UsersSid = ((TOKEN_PRIMARY_GROUP*)group)->PrimaryGroup;

    CloseHandle( token );
    if (!res)
    {
        HeapFree(GetProcessHeap(), 0, Acl);
        return;
    }

    res = AddAccessDeniedAce(Acl, ACL_REVISION, PROCESS_VM_READ, AdminSid);
    ok(res, "AddAccessDeniedAce failed with error %d\n", GetLastError());
    res = AddAccessAllowedAce(Acl, ACL_REVISION, PROCESS_ALL_ACCESS, AdminSid);
    ok(res, "AddAccessAllowedAce failed with error %d\n", GetLastError());

    SecurityDescriptor = HeapAlloc(GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);
    res = InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    ok(res, "InitializeSecurityDescriptor failed with error %d\n", GetLastError());

    event = CreateEvent( NULL, TRUE, TRUE, "test_event" );
    ok(event != NULL, "CreateEvent %d\n", GetLastError());

    SecurityDescriptor->Revision = 0;
    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_UNKNOWN_REVISION );
    SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;

    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_INVALID_SECURITY_DESCR );
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_INVALID_SECURITY_DESCR );
    CHECK_SET_SECURITY( event, SACL_SECURITY_INFORMATION, ERROR_ACCESS_DENIED );
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );
    /* NULL DACL is valid and means default DACL from token */
    SecurityDescriptor->Control |= SE_DACL_PRESENT;
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );

    /* Set owner and group and dacl */
    res = SetSecurityDescriptorOwner(SecurityDescriptor, AdminSid, FALSE);
    ok(res, "SetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_SUCCESS );
    res = SetSecurityDescriptorGroup(SecurityDescriptor, UsersSid, FALSE);
    ok(res, "SetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_SUCCESS );
    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );

    sprintf(buffer, "%s tests/security.c test", myARGV[0]);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    psa.nLength = sizeof(psa);
    psa.lpSecurityDescriptor = SecurityDescriptor;
    psa.bInheritHandle = TRUE;

    /* Doesn't matter what ACL say we should get full access for ourselves */
    ok(CreateProcessA( NULL, buffer, &psa, NULL, FALSE, 0, NULL, NULL, &startup, &info ),
        "CreateProcess with err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS2( info.hProcess, PROCESS_ALL_ACCESS,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );
    winetest_wait_child_process( info.hProcess );

    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( event );
    HeapFree(GetProcessHeap(), 0, Acl);
    HeapFree(GetProcessHeap(), 0, SecurityDescriptor);
}

static void test_process_security_child(void)
{
    HANDLE handle, handle1;
    BOOL ret;
    DWORD err;

    handle = OpenProcess( PROCESS_TERMINATE, FALSE, GetCurrentProcessId() );
    ok(handle != NULL, "OpenProcess(PROCESS_TERMINATE) with err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS( handle, PROCESS_TERMINATE );

    ok(DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                        &handle1, 0, TRUE, DUPLICATE_SAME_ACCESS ),
       "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS( handle1, PROCESS_TERMINATE );

    CloseHandle( handle1 );

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                           &handle1, PROCESS_ALL_ACCESS, TRUE, 0 );
    err = GetLastError();
    todo_wine
    ok(!ret && err == ERROR_ACCESS_DENIED, "duplicating handle should have failed "
       "with STATUS_ACCESS_DENIED, instead of err:%d\n", err);

    CloseHandle( handle );

    /* These two should fail - they are denied by ACL */
    handle = OpenProcess( PROCESS_VM_READ, FALSE, GetCurrentProcessId() );
    todo_wine
    ok(handle == NULL, "OpenProcess(PROCESS_VM_READ) should have failed\n");
    handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
    todo_wine
    ok(handle == NULL, "OpenProcess(PROCESS_ALL_ACCESS) should have failed\n");

    /* Documented privilege elevation */
    ok(DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                        &handle, 0, TRUE, DUPLICATE_SAME_ACCESS ),
       "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS2( handle, PROCESS_ALL_ACCESS,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );

    CloseHandle( handle );

    /* Same only explicitly asking for all access rights */
    ok(DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                        &handle, PROCESS_ALL_ACCESS, TRUE, 0 ),
       "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS2( handle, PROCESS_ALL_ACCESS,
                          PROCESS_ALL_ACCESS | PROCESS_QUERY_LIMITED_INFORMATION );
    ok(DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                        &handle1, PROCESS_VM_READ, TRUE, 0 ),
       "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS( handle1, PROCESS_VM_READ );
    CloseHandle( handle1 );
    CloseHandle( handle );
}

static void test_impersonation_level(void)
{
    HANDLE Token, ProcessToken;
    HANDLE Token2;
    DWORD Size;
    TOKEN_PRIVILEGES *Privileges;
    TOKEN_USER *User;
    PRIVILEGE_SET *PrivilegeSet;
    BOOL AccessGranted;
    BOOL ret;
    HKEY hkey;
    DWORD error;

    pDuplicateTokenEx = (fnDuplicateTokenEx) GetProcAddress(hmod, "DuplicateTokenEx");
    if( !pDuplicateTokenEx ) {
        skip("DuplicateTokenEx is not available\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = ImpersonateSelf(SecurityAnonymous);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("ImpersonateSelf is not implemented\n");
        return;
    }
    ok(ret, "ImpersonateSelf(SecurityAnonymous) failed with error %d\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(!ret, "OpenThreadToken should have failed\n");
    error = GetLastError();
    ok(error == ERROR_CANT_OPEN_ANONYMOUS, "OpenThreadToken on anonymous token should have returned ERROR_CANT_OPEN_ANONYMOUS instead of %d\n", error);
    /* can't perform access check when opening object against an anonymous impersonation token */
    todo_wine {
    error = RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    ok(error == ERROR_INVALID_HANDLE, "RegOpenKeyEx should have failed with ERROR_INVALID_HANDLE instead of %d\n", error);
    }
    RevertToSelf();

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &ProcessToken);
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());

    ret = pDuplicateTokenEx(ProcessToken,
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, NULL,
        SecurityAnonymous, TokenImpersonation, &Token);
    ok(ret, "DuplicateTokenEx failed with error %d\n", GetLastError());
    /* can't increase the impersonation level */
    ret = DuplicateToken(Token, SecurityIdentification, &Token2);
    error = GetLastError();
    ok(!ret && error == ERROR_BAD_IMPERSONATION_LEVEL,
        "Duplicating a token and increasing the impersonation level should have failed with ERROR_BAD_IMPERSONATION_LEVEL instead of %d\n", error);
    /* we can query anything from an anonymous token, including the user */
    ret = GetTokenInformation(Token, TokenUser, NULL, 0, &Size);
    error = GetLastError();
    ok(!ret && error == ERROR_INSUFFICIENT_BUFFER, "GetTokenInformation(TokenUser) should have failed with ERROR_INSUFFICIENT_BUFFER instead of %d\n", error);
    User = (TOKEN_USER *)HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenUser, User, Size, &Size);
    ok(ret, "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, User);

    /* PrivilegeCheck fails with SecurityAnonymous level */
    ret = GetTokenInformation(Token, TokenPrivileges, NULL, 0, &Size);
    error = GetLastError();
    ok(!ret && error == ERROR_INSUFFICIENT_BUFFER, "GetTokenInformation(TokenPrivileges) should have failed with ERROR_INSUFFICIENT_BUFFER instead of %d\n", error);
    Privileges = (TOKEN_PRIVILEGES *)HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenPrivileges, Privileges, Size, &Size);
    ok(ret, "GetTokenInformation(TokenPrivileges) failed with error %d\n", GetLastError());

    PrivilegeSet = (PRIVILEGE_SET *)HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(PRIVILEGE_SET, Privilege[Privileges->PrivilegeCount]));
    PrivilegeSet->PrivilegeCount = Privileges->PrivilegeCount;
    memcpy(PrivilegeSet->Privilege, Privileges->Privileges, PrivilegeSet->PrivilegeCount * sizeof(PrivilegeSet->Privilege[0]));
    PrivilegeSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
    HeapFree(GetProcessHeap(), 0, Privileges);

    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    error = GetLastError();
    ok(!ret && error == ERROR_BAD_IMPERSONATION_LEVEL, "PrivilegeCheck for SecurityAnonymous token should have failed with ERROR_BAD_IMPERSONATION_LEVEL instead of %d\n", error);

    CloseHandle(Token);

    ret = ImpersonateSelf(SecurityIdentification);
    ok(ret, "ImpersonateSelf(SecurityIdentification) failed with error %d\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(ret, "OpenThreadToken failed with error %d\n", GetLastError());

    /* can't perform access check when opening object against an identification impersonation token */
    error = RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    todo_wine {
    ok(error == ERROR_INVALID_HANDLE, "RegOpenKeyEx should have failed with ERROR_INVALID_HANDLE instead of %d\n", error);
    }
    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    ok(ret, "PrivilegeCheck for SecurityIdentification failed with error %d\n", GetLastError());
    CloseHandle(Token);
    RevertToSelf();

    ret = ImpersonateSelf(SecurityImpersonation);
    ok(ret, "ImpersonateSelf(SecurityImpersonation) failed with error %d\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(ret, "OpenThreadToken failed with error %d\n", GetLastError());
    error = RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    ok(error == ERROR_SUCCESS, "RegOpenKeyEx should have succeeded instead of failing with %d\n", error);
    RegCloseKey(hkey);
    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    ok(ret, "PrivilegeCheck for SecurityImpersonation failed with error %d\n", GetLastError());
    RevertToSelf();

    CloseHandle(Token);
    CloseHandle(ProcessToken);

    HeapFree(GetProcessHeap(), 0, PrivilegeSet);
}

static void test_SetEntriesInAcl(void)
{
    DWORD res;
    PSID EveryoneSid = NULL, UsersSid = NULL;
    PACL OldAcl = NULL, NewAcl;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    EXPLICIT_ACCESSW ExplicitAccess;
    static const WCHAR wszEveryone[] = {'E','v','e','r','y','o','n','e',0};

    if (!pSetEntriesInAclW)
    {
        skip("SetEntriesInAclW is not available\n");
        return;
    }

    NewAcl = (PACL)0xdeadbeef;
    res = pSetEntriesInAclW(0, NULL, NULL, &NewAcl);
    if(res == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("SetEntriesInAclW is not implemented\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl == NULL, "NewAcl=%p, expected NULL\n", NewAcl);

    OldAcl = HeapAlloc(GetProcessHeap(), 0, 256);
    res = InitializeAcl(OldAcl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("ACLs not implemented - skipping tests\n");
        HeapFree(GetProcessHeap(), 0, OldAcl);
        return;
    }
    ok(res, "InitializeAcl failed with error %d\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_USERS, 0, 0, 0, 0, 0, 0, &UsersSid);
    ok(res, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    res = AddAccessAllowedAce(OldAcl, ACL_REVISION, KEY_READ, UsersSid);
    ok(res, "AddAccessAllowedAce failed with error %d\n", GetLastError());

    ExplicitAccess.grfAccessPermissions = KEY_WRITE;
    ExplicitAccess.grfAccessMode = GRANT_ACCESS;
    ExplicitAccess.grfInheritance = NO_INHERITANCE;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    ExplicitAccess.Trustee.ptstrName = (LPWSTR)EveryoneSid;
    res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH)
    {
        skip("Non-english locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_USER;
        ExplicitAccess.Trustee.ptstrName = (LPWSTR)wszEveryone;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_BAD_FORM;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER, "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl == NULL, "returned acl wasn't NULL: %p\n", NewAcl);
        LocalFree(NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_USER;
        ExplicitAccess.Trustee.MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER, "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl == NULL, "returned acl wasn't NULL: %p\n", NewAcl);
        LocalFree(NewAcl);

        ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ExplicitAccess.grfAccessMode = SET_ACCESS;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);
    }

    ExplicitAccess.grfAccessMode = REVOKE_ACCESS;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = (LPWSTR)UsersSid;
    res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    LocalFree(UsersSid);
    LocalFree(EveryoneSid);
    HeapFree(GetProcessHeap(), 0, OldAcl);
}

static void test_GetNamedSecurityInfoA(void)
{
    PSECURITY_DESCRIPTOR pSecDesc;
    DWORD revision;
    SECURITY_DESCRIPTOR_CONTROL control;
    PSID owner;
    PSID group;
    BOOL owner_defaulted;
    BOOL group_defaulted;
    DWORD error;
    BOOL ret;
    CHAR windows_dir[MAX_PATH];

    if (!pGetNamedSecurityInfoA)
    {
        skip("GetNamedSecurityInfoA is not available\n");
        return;
    }

    ret = GetWindowsDirectoryA(windows_dir, MAX_PATH);
    ok(ret, "GetWindowsDirectory failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    error = pGetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
        NULL, NULL, NULL, NULL, &pSecDesc);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("GetNamedSecurityInfoA is not implemented\n");
        return;
    }
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);

    ret = GetSecurityDescriptorControl(pSecDesc, &control, &revision);
    ok(ret, "GetSecurityDescriptorControl failed with error %d\n", GetLastError());
    ok((control & (SE_SELF_RELATIVE|SE_DACL_PRESENT)) == (SE_SELF_RELATIVE|SE_DACL_PRESENT),
        "control (0x%x) doesn't have (SE_SELF_RELATIVE|SE_DACL_PRESENT) flags set\n", control);
    ok(revision == SECURITY_DESCRIPTOR_REVISION1, "revision was %d instead of 1\n", revision);
    ret = GetSecurityDescriptorOwner(pSecDesc, &owner, &owner_defaulted);
    ok(ret, "GetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    ok(owner != NULL, "owner should not be NULL\n");
    ret = GetSecurityDescriptorGroup(pSecDesc, &group, &group_defaulted);
    ok(ret, "GetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    ok(group != NULL, "group should not be NULL\n");
}

static void test_ConvertStringSecurityDescriptor(void)
{
    BOOL ret;
    PSECURITY_DESCRIPTOR pSD;

    if (!pConvertStringSecurityDescriptorToSecurityDescriptorA)
    {
        skip("ConvertStringSecurityDescriptorToSecurityDescriptor is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;GA;;;WD)", 0xdeadbeef, &pSD, NULL);
    ok(!ret && GetLastError() == ERROR_UNKNOWN_REVISION,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_UNKNOWN_REVISION instead of %d\n",
        GetLastError());

    /* test ACE string type */
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;GA;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(D;;GA;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "ERROR:(D;;GA;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %d\n",
        GetLastError());

    /* test ACE string access rights */
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;GA;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;GRGWGX;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;RCSDWDWO;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;RPWPCCDCLCSWLODTCR;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;FAFRFWFX;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;KAKRKWKX;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;0xFFFFFFFF;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "S:(AU;;0xFFFFFFFF;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);

    /* test ACE string access right error case */
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;ROB;;;WD)", SDDL_REVISION_1, &pSD, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_ACL,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_ACL instead of %d\n",
        GetLastError());

    /* test ACE string SID */
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(D;;GA;;;S-1-0-0)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(D;;GA;;;Nonexistent account)", SDDL_REVISION_1, &pSD, NULL);
    ok(!ret, "Expected failure, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_ACL || GetLastError() == ERROR_INVALID_SID,
       "Expected ERROR_INVALID_ACL or ERROR_INVALID_SID, got %d\n", GetLastError());
}

static void test_ConvertSecurityDescriptorToString()
{
    SECURITY_DESCRIPTOR desc;
    SECURITY_INFORMATION sec_info = OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION;
    LPSTR string;
    DWORD size;
    PSID psid, psid2;
    PACL pacl;
    char sid_buf[256];
    char acl_buf[8192];
    ULONG len;

    if (!pConvertSecurityDescriptorToStringSecurityDescriptorA)
    {
        skip("ConvertSecurityDescriptorToStringSecurityDescriptor is not available\n");
        return;
    }
    if (!pCreateWellKnownSid)
    {
        skip("CreateWellKnownSid is not available\n");
        return;
    }

/* It seems Windows XP adds an extra character to the length of the string for each ACE in an ACL. We
 * don't replicate this feature so we only test len >= strlen+1. */
#define CHECK_RESULT_AND_FREE(exp_str) \
    ok(strcmp(string, (exp_str)) == 0, "String mismatch (expected \"%s\", got \"%s\")\n", (exp_str), string); \
    ok(len >= (lstrlen(exp_str) + 1), "Length mismatch (expected %d, got %d)\n", lstrlen(exp_str) + 1, len); \
    LocalFree(string);

#define CHECK_ONE_OF_AND_FREE(exp_str1, exp_str2) \
    ok(strcmp(string, (exp_str1)) == 0 || strcmp(string, (exp_str2)) == 0, "String mismatch (expected\n\"%s\" or\n\"%s\", got\n\"%s\")\n", (exp_str1), (exp_str2), string); \
    ok(len >= (strlen(string) + 1), "Length mismatch (expected %d, got %d)\n", lstrlen(string) + 1, len); \
    LocalFree(string);

    InitializeSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("");

    size = 4096;
    pCreateWellKnownSid(WinLocalSid, NULL, sid_buf, &size);
    SetSecurityDescriptorOwner(&desc, (PSID)sid_buf, FALSE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:S-1-2-0");

    SetSecurityDescriptorOwner(&desc, (PSID)sid_buf, TRUE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:S-1-2-0");

    size = sizeof(sid_buf);
    pCreateWellKnownSid(WinLocalSystemSid, NULL, sid_buf, &size);
    SetSecurityDescriptorOwner(&desc, (PSID)sid_buf, TRUE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SY");

    pConvertStringSidToSidA("S-1-5-21-93476-23408-4576", &psid);
    SetSecurityDescriptorGroup(&desc, psid, TRUE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576");

    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, GROUP_SECURITY_INFORMATION, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("G:S-1-5-21-93476-23408-4576");

    pacl = (PACL)acl_buf;
    InitializeAcl(pacl, sizeof(acl_buf), ACL_REVISION);
    SetSecurityDescriptorDacl(&desc, TRUE, pacl, TRUE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:");

    SetSecurityDescriptorDacl(&desc, TRUE, pacl, FALSE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:");

    pConvertStringSidToSidA("S-1-5-6", &psid2);
    pAddAccessAllowedAceEx(pacl, ACL_REVISION, NO_PROPAGATE_INHERIT_ACE, 0xf0000000, psid2);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)");

    pAddAccessAllowedAceEx(pacl, ACL_REVISION, INHERIT_ONLY_ACE|INHERITED_ACE, 0x00000003, psid2);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)");

    pAddAccessDeniedAceEx(pacl, ACL_REVISION, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE, 0xffffffff, psid);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)");


    pacl = (PACL)acl_buf;
    InitializeAcl(pacl, sizeof(acl_buf), ACL_REVISION);
    SetSecurityDescriptorSacl(&desc, TRUE, pacl, FALSE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:S:");

    /* fails in win2k */
    SetSecurityDescriptorDacl(&desc, TRUE, NULL, FALSE);
    pAddAuditAccessAceEx(pacl, ACL_REVISION, VALID_INHERIT_FLAGS, KEY_READ|KEY_WRITE, psid2, TRUE, TRUE);
    if (pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len))
    {
        CHECK_ONE_OF_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)", /* XP */
            "O:SYG:S-1-5-21-93476-23408-4576D:NO_ACCESS_CONTROLS:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)" /* Vista */);
    }

    /* fails in win2k */
    pAddAuditAccessAceEx(pacl, ACL_REVISION, NO_PROPAGATE_INHERIT_ACE, FILE_GENERIC_READ|FILE_GENERIC_WRITE, psid2, TRUE, FALSE);
    if (pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len))
    {
        CHECK_ONE_OF_AND_FREE("O:SYG:S-1-5-21-93476-23408-4576D:S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)", /* XP */
            "O:SYG:S-1-5-21-93476-23408-4576D:NO_ACCESS_CONTROLS:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)" /* Vista */);
    }
}

static void test_PrivateObjectSecurity(void)
{
    SECURITY_INFORMATION sec_info = OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION;
    SECURITY_DESCRIPTOR_CONTROL ctrl;
    PSECURITY_DESCRIPTOR sec;
    DWORD dwDescSize;
    DWORD dwRevision;
    DWORD retSize;
    LPSTR string;
    ULONG len;
    PSECURITY_DESCRIPTOR buf;

    if (!pConvertStringSecurityDescriptorToSecurityDescriptorA)
    {
        skip("ConvertStringSecurityDescriptorToSecurityDescriptor is not available\n");
        return;
    }

    ok(pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)", SDDL_REVISION_1, &sec, &dwDescSize), "Creating descriptor failed\n");
    buf = HeapAlloc(GetProcessHeap(), 0, dwDescSize);
    DbgPrint("Received %p\n", sec);
    pSetSecurityDescriptorControl(sec, SE_DACL_PROTECTED, SE_DACL_PROTECTED);
    GetSecurityDescriptorControl(sec, &ctrl, &dwRevision);
    todo_wine expect_eq(ctrl, 0x9014, int, "%x");

    ok(GetPrivateObjectSecurity(sec, GROUP_SECURITY_INFORMATION, buf, dwDescSize, &retSize),
        "GetPrivateObjectSecurity failed (err=%u)\n", GetLastError());
    ok(retSize <= dwDescSize, "Buffer too small (%d vs %d)\n", retSize, dwDescSize);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("G:S-1-5-21-93476-23408-4576");
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl, 0x8000, int, "%x");

    ok(GetPrivateObjectSecurity(sec, GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, buf, dwDescSize, &retSize),
        "GetPrivateObjectSecurity failed (err=%u)\n", GetLastError());
    ok(retSize <= dwDescSize, "Buffer too small (%d vs %d)\n", retSize, dwDescSize);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed err=%u\n", GetLastError());
    CHECK_RESULT_AND_FREE("G:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)");
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl, 0x8004, int, "%x");

    ok(GetPrivateObjectSecurity(sec, sec_info, buf, dwDescSize, &retSize),
        "GetPrivateObjectSecurity failed (err=%u)\n", GetLastError());
    ok(retSize == dwDescSize, "Buffer too small (%d vs %d)\n", retSize, dwDescSize);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)");
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl, 0x8014, int, "%x");

    SetLastError(0xdeadbeef);
    ok(GetPrivateObjectSecurity(sec, sec_info, buf, 5, &retSize) == FALSE, "GetPrivateObjectSecurity should have failed\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected error ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());

    LocalFree(sec);
    HeapFree(GetProcessHeap(), 0, buf);
}
#undef CHECK_RESULT_AND_FREE
#undef CHECK_ONE_OF_AND_FREE

static void test_acls(void)
{
    char buffer[256];
    PACL pAcl = (PACL)buffer;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(ACL) - 1, ACL_REVISION);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("InitializeAcl is not implemented\n");
        return;
    }

    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "InitializeAcl with too small a buffer should have failed with ERROR_INSUFFICIENT_BUFFER instead of %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, 0xffffffff, ACL_REVISION);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "InitializeAcl with too large a buffer should have failed with ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION1);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "InitializeAcl(ACL_REVISION1) should have failed with ERROR_INVALID_PARAMETER instead of %d\n", GetLastError());

    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION2);
    ok(ret, "InitializeAcl(ACL_REVISION2) failed with error %d\n", GetLastError());

    ret = IsValidAcl(pAcl);
    ok(ret, "IsValidAcl failed with error %d\n", GetLastError());

    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION3);
    ok(ret, "InitializeAcl(ACL_REVISION3) failed with error %d\n", GetLastError());

    ret = IsValidAcl(pAcl);
    ok(ret, "IsValidAcl failed with error %d\n", GetLastError());

    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION4);
    ok(ret, "InitializeAcl(ACL_REVISION4) failed with error %d\n", GetLastError());

    ret = IsValidAcl(pAcl);
    ok(ret, "IsValidAcl failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(buffer), -1);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "InitializeAcl(-1) failed with error %d\n", GetLastError());
}

static void test_GetSecurityInfo(void)
{
    HANDLE obj;
    PSECURITY_DESCRIPTOR sd;
    PSID owner, group;
    PACL dacl;
    DWORD ret;

    if (!pGetSecurityInfo)
    {
        win_skip("GetSecurityInfo is not available\n");
        return;
    }

    /* Create something.  Files have lots of associated security info.  */
    obj = CreateFile(myARGV[0], GENERIC_READ, FILE_SHARE_READ, NULL,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (obj == INVALID_HANDLE_VALUE)
    {
        skip("Couldn't create an object for GetSecurityInfo test\n");
        return;
    }

    ret = pGetSecurityInfo(obj, SE_FILE_OBJECT,
                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          &owner, &group, &dacl, NULL, &sd);
    if (ret == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetSecurityInfo is not implemented\n");
        CloseHandle(obj);
        return;
    }
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %d\n", ret);
    ok(sd != NULL, "GetSecurityInfo\n");
    ok(owner != NULL, "GetSecurityInfo\n");
    ok(group != NULL, "GetSecurityInfo\n");
    ok(dacl != NULL, "GetSecurityInfo\n");
    ok(IsValidAcl(dacl), "GetSecurityInfo\n");

    LocalFree(sd);

    /* If we don't ask for the security descriptor, Windows will still give us
       the other stuff, leaving us no way to free it.  */
    ret = pGetSecurityInfo(obj, SE_FILE_OBJECT,
                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          &owner, &group, &dacl, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %d\n", ret);
    ok(owner != NULL, "GetSecurityInfo\n");
    ok(group != NULL, "GetSecurityInfo\n");
    ok(dacl != NULL, "GetSecurityInfo\n");
    ok(IsValidAcl(dacl), "GetSecurityInfo\n");

    CloseHandle(obj);
}

START_TEST(security)
{
    init();
    if (!hmod) return;

    if (myARGC >= 3)
    {
        test_process_security_child();
        return;
    }
    test_sid();
    test_trustee();
    test_luid();
    test_CreateWellKnownSid();
    test_FileSecurity();
    test_AccessCheck();
    test_token_attr();
    test_LookupAccountSid();
    test_LookupAccountName();
    test_security_descriptor();
    test_process_security();
    test_impersonation_level();
    test_SetEntriesInAcl();
    test_GetNamedSecurityInfoA();
    test_ConvertStringSecurityDescriptor();
    test_ConvertSecurityDescriptorToString();
    test_PrivateObjectSecurity();
    test_acls();
    test_GetSecurityInfo();
}
