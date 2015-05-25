/*
 * Unit tests for security functions
 *
 * Copyright (c) 2004 Mike McCormack
 * Copyright (c) 2011 Dmitry Timoshkov
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
#define WIN32_LEAN_AND_MEAN
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/winternl.h"
#include "aclapi.h"
#include "winnt.h"
#include "sddl.h"
#include "ntsecapi.h"
#include "lmcons.h"

#include <winsvc.h>

#include "wine/test.h"

#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#endif

/* PROCESS_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define PROCESS_ALL_ACCESS_NT4 (PROCESS_ALL_ACCESS & ~0xf000)
#define PROCESS_ALL_ACCESS_VISTA (PROCESS_ALL_ACCESS | 0xf000)

#ifndef EVENT_QUERY_STATE
#define EVENT_QUERY_STATE 0x0001
#endif

#ifndef SEMAPHORE_QUERY_STATE
#define SEMAPHORE_QUERY_STATE 0x0001
#endif

#ifndef THREAD_SET_LIMITED_INFORMATION
#define THREAD_SET_LIMITED_INFORMATION 0x0400
#define THREAD_QUERY_LIMITED_INFORMATION 0x0800
#endif

#define THREAD_ALL_ACCESS_NT4 (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff)
#define THREAD_ALL_ACCESS_VISTA (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xffff)

#define expect_eq(expr, value, type, format) { type ret_ = expr; ok((value) == ret_, #expr " expected " format "  got " format "\n", (value), (ret_)); }

static BOOL (WINAPI *pAddAccessAllowedAceEx)(PACL, DWORD, DWORD, DWORD, PSID);
static BOOL (WINAPI *pAddAccessDeniedAceEx)(PACL, DWORD, DWORD, DWORD, PSID);
static BOOL (WINAPI *pAddAuditAccessAceEx)(PACL, DWORD, DWORD, DWORD, PSID, BOOL, BOOL);
static VOID (WINAPI *pBuildTrusteeWithSidA)( PTRUSTEEA pTrustee, PSID pSid );
static VOID (WINAPI *pBuildTrusteeWithNameA)( PTRUSTEEA pTrustee, LPSTR pName );
static VOID (WINAPI *pBuildTrusteeWithObjectsAndNameA)( PTRUSTEEA pTrustee,
                                                          POBJECTS_AND_NAME_A pObjName,
                                                          SE_OBJECT_TYPE ObjectType,
                                                          LPSTR ObjectTypeName,
                                                          LPSTR InheritedObjectTypeName,
                                                          LPSTR Name );
static VOID (WINAPI *pBuildTrusteeWithObjectsAndSidA)( PTRUSTEEA pTrustee,
                                                         POBJECTS_AND_SID pObjSid,
                                                         GUID* pObjectGuid,
                                                         GUID* pInheritedObjectGuid,
                                                         PSID pSid );
static LPSTR (WINAPI *pGetTrusteeNameA)( PTRUSTEEA pTrustee );
static BOOL (WINAPI *pMakeSelfRelativeSD)( PSECURITY_DESCRIPTOR, PSECURITY_DESCRIPTOR, LPDWORD );
static BOOL (WINAPI *pConvertSidToStringSidA)( PSID pSid, LPSTR *str );
static BOOL (WINAPI *pConvertStringSidToSidA)( LPCSTR str, PSID pSid );
static BOOL (WINAPI *pCheckTokenMembership)(HANDLE, PSID, PBOOL);
static BOOL (WINAPI *pConvertStringSecurityDescriptorToSecurityDescriptorA)(LPCSTR, DWORD,
                                                                            PSECURITY_DESCRIPTOR*, PULONG );
static BOOL (WINAPI *pConvertStringSecurityDescriptorToSecurityDescriptorW)(LPCWSTR, DWORD,
                                                                            PSECURITY_DESCRIPTOR*, PULONG );
static BOOL (WINAPI *pConvertSecurityDescriptorToStringSecurityDescriptorA)(PSECURITY_DESCRIPTOR, DWORD,
                                                                            SECURITY_INFORMATION, LPSTR *, PULONG );
static BOOL (WINAPI *pGetFileSecurityA)(LPCSTR, SECURITY_INFORMATION,
                                          PSECURITY_DESCRIPTOR, DWORD, LPDWORD);
static BOOL (WINAPI *pSetFileSecurityA)(LPCSTR, SECURITY_INFORMATION,
                                          PSECURITY_DESCRIPTOR);
static DWORD (WINAPI *pGetNamedSecurityInfoA)(LPSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                              PSID*, PSID*, PACL*, PACL*,
                                              PSECURITY_DESCRIPTOR*);
static DWORD (WINAPI *pSetNamedSecurityInfoA)(LPSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                              PSID, PSID, PACL, PACL);
static PDWORD (WINAPI *pGetSidSubAuthority)(PSID, DWORD);
static PUCHAR (WINAPI *pGetSidSubAuthorityCount)(PSID);
static BOOL (WINAPI *pIsValidSid)(PSID);
static DWORD (WINAPI *pRtlAdjustPrivilege)(ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);
static BOOL (WINAPI *pCreateWellKnownSid)(WELL_KNOWN_SID_TYPE,PSID,PSID,DWORD*);
static BOOL (WINAPI *pDuplicateTokenEx)(HANDLE,DWORD,LPSECURITY_ATTRIBUTES,
                                        SECURITY_IMPERSONATION_LEVEL,TOKEN_TYPE,PHANDLE);

static NTSTATUS (WINAPI *pLsaQueryInformationPolicy)(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
static NTSTATUS (WINAPI *pLsaClose)(LSA_HANDLE);
static NTSTATUS (WINAPI *pLsaFreeMemory)(PVOID);
static NTSTATUS (WINAPI *pLsaOpenPolicy)(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
static NTSTATUS (WINAPI *pNtQueryObject)(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
static DWORD (WINAPI *pSetEntriesInAclW)(ULONG, PEXPLICIT_ACCESSW, PACL, PACL*);
static DWORD (WINAPI *pSetEntriesInAclA)(ULONG, PEXPLICIT_ACCESSA, PACL, PACL*);
static BOOL (WINAPI *pSetSecurityDescriptorControl)(PSECURITY_DESCRIPTOR, SECURITY_DESCRIPTOR_CONTROL,
                                                    SECURITY_DESCRIPTOR_CONTROL);
static DWORD (WINAPI *pGetSecurityInfo)(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                        PSID*, PSID*, PACL*, PACL*, PSECURITY_DESCRIPTOR*);
static DWORD (WINAPI *pSetSecurityInfo)(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                        PSID, PSID, PACL, PACL);
static NTSTATUS (WINAPI *pNtAccessCheck)(PSECURITY_DESCRIPTOR, HANDLE, ACCESS_MASK, PGENERIC_MAPPING,
                                         PPRIVILEGE_SET, PULONG, PULONG, NTSTATUS*);
static BOOL (WINAPI *pCreateRestrictedToken)(HANDLE, DWORD, DWORD, PSID_AND_ATTRIBUTES, DWORD,
                                             PLUID_AND_ATTRIBUTES, DWORD, PSID_AND_ATTRIBUTES, PHANDLE);
static BOOL (WINAPI *pGetAclInformation)(PACL,LPVOID,DWORD,ACL_INFORMATION_CLASS);
static BOOL (WINAPI *pGetAce)(PACL,DWORD,LPVOID*);
static NTSTATUS (WINAPI *pNtSetSecurityObject)(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
static NTSTATUS (WINAPI *pNtCreateFile)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)(LPCWSTR,PUNICODE_STRING,PWSTR*,CURDIR*);
static NTSTATUS (WINAPI *pRtlAnsiStringToUnicodeString)(PUNICODE_STRING,PCANSI_STRING,BOOLEAN);

static HMODULE hmod;
static int     myARGC;
static char**  myARGV;

struct strsid_entry
{
    const char *str;
    DWORD flags;
};
#define STRSID_OK     0
#define STRSID_OPT    1

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
    pNtAccessCheck = (void *)GetProcAddress( hntdll, "NtAccessCheck" );
    pNtSetSecurityObject = (void *)GetProcAddress(hntdll, "NtSetSecurityObject");
    pNtCreateFile = (void *)GetProcAddress(hntdll, "NtCreateFile");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlAnsiStringToUnicodeString = (void *)GetProcAddress(hntdll, "RtlAnsiStringToUnicodeString");

    hmod = GetModuleHandleA("advapi32.dll");
    pAddAccessAllowedAceEx = (void *)GetProcAddress(hmod, "AddAccessAllowedAceEx");
    pAddAccessDeniedAceEx = (void *)GetProcAddress(hmod, "AddAccessDeniedAceEx");
    pAddAuditAccessAceEx = (void *)GetProcAddress(hmod, "AddAuditAccessAceEx");
    pCheckTokenMembership = (void *)GetProcAddress(hmod, "CheckTokenMembership");
    pConvertStringSecurityDescriptorToSecurityDescriptorA =
        (void *)GetProcAddress(hmod, "ConvertStringSecurityDescriptorToSecurityDescriptorA" );
    pConvertStringSecurityDescriptorToSecurityDescriptorW =
        (void *)GetProcAddress(hmod, "ConvertStringSecurityDescriptorToSecurityDescriptorW" );
    pConvertSecurityDescriptorToStringSecurityDescriptorA =
        (void *)GetProcAddress(hmod, "ConvertSecurityDescriptorToStringSecurityDescriptorA" );
    pGetFileSecurityA = (void *)GetProcAddress(hmod, "GetFileSecurityA" );
    pSetFileSecurityA = (void *)GetProcAddress(hmod, "SetFileSecurityA" );
    pCreateWellKnownSid = (void *)GetProcAddress( hmod, "CreateWellKnownSid" );
    pGetNamedSecurityInfoA = (void *)GetProcAddress(hmod, "GetNamedSecurityInfoA");
    pSetNamedSecurityInfoA = (void *)GetProcAddress(hmod, "SetNamedSecurityInfoA");
    pGetSidSubAuthority = (void *)GetProcAddress(hmod, "GetSidSubAuthority");
    pGetSidSubAuthorityCount = (void *)GetProcAddress(hmod, "GetSidSubAuthorityCount");
    pIsValidSid = (void *)GetProcAddress(hmod, "IsValidSid");
    pMakeSelfRelativeSD = (void *)GetProcAddress(hmod, "MakeSelfRelativeSD");
    pSetEntriesInAclW = (void *)GetProcAddress(hmod, "SetEntriesInAclW");
    pSetEntriesInAclA = (void *)GetProcAddress(hmod, "SetEntriesInAclA");
    pSetSecurityDescriptorControl = (void *)GetProcAddress(hmod, "SetSecurityDescriptorControl");
    pGetSecurityInfo = (void *)GetProcAddress(hmod, "GetSecurityInfo");
    pSetSecurityInfo = (void *)GetProcAddress(hmod, "SetSecurityInfo");
    pCreateRestrictedToken = (void *)GetProcAddress(hmod, "CreateRestrictedToken");
    pConvertSidToStringSidA = (void *)GetProcAddress(hmod, "ConvertSidToStringSidA");
    pConvertStringSidToSidA = (void *)GetProcAddress(hmod, "ConvertStringSidToSidA");
    pGetAclInformation = (void *)GetProcAddress(hmod, "GetAclInformation");
    pGetAce = (void *)GetProcAddress(hmod, "GetAce");

    myARGC = winetest_get_mainargs( &myARGV );
}

static SECURITY_DESCRIPTOR* test_get_security_descriptor(HANDLE handle, int line)
{
    /* use HeapFree(GetProcessHeap(), 0, sd); when done */
    DWORD ret, length, needed;
    SECURITY_DESCRIPTOR *sd;

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(handle, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  NULL, 0, &needed);
    ok_(__FILE__, line)(!ret, "GetKernelObjectSecurity should fail\n");
    ok_(__FILE__, line)(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok_(__FILE__, line)(needed != 0xdeadbeef, "GetKernelObjectSecurity should return required buffer length\n");

    length = needed;
    sd = HeapAlloc(GetProcessHeap(), 0, length);

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetKernelObjectSecurity(handle, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                  sd, length, &needed);
    ok_(__FILE__, line)(ret, "GetKernelObjectSecurity error %d\n", GetLastError());
    ok_(__FILE__, line)(needed == length || needed == 0 /* file, pipe */, "GetKernelObjectSecurity should return %u instead of %u\n", length, needed);
    return sd;
}

static void test_owner_equal(HANDLE Handle, PSID expected, int line)
{
    BOOL res;
    SECURITY_DESCRIPTOR *queriedSD = NULL;
    PSID owner;
    BOOL owner_defaulted;

    queriedSD = test_get_security_descriptor( Handle, line );

    res = GetSecurityDescriptorOwner(queriedSD, &owner, &owner_defaulted);
    ok_(__FILE__, line)(res, "GetSecurityDescriptorOwner failed with error %d\n", GetLastError());

    ok_(__FILE__, line)(EqualSid(owner, expected), "Owner SIDs are not equal\n");
    ok_(__FILE__, line)(!owner_defaulted, "Defaulted is true\n");

    HeapFree(GetProcessHeap(), 0, queriedSD);
}

static void test_group_equal(HANDLE Handle, PSID expected, int line)
{
    BOOL res;
    SECURITY_DESCRIPTOR *queriedSD = NULL;
    PSID group;
    BOOL group_defaulted;

    queriedSD = test_get_security_descriptor( Handle, line );

    res = GetSecurityDescriptorGroup(queriedSD, &group, &group_defaulted);
    ok_(__FILE__, line)(res, "GetSecurityDescriptorGroup failed with error %d\n", GetLastError());

    ok_(__FILE__, line)(EqualSid(group, expected), "Group SIDs are not equal\n");
    ok_(__FILE__, line)(!group_defaulted, "Defaulted is true\n");

    HeapFree(GetProcessHeap(), 0, queriedSD);
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
    struct strsid_entry strsid_table[] = {
        {"AO", STRSID_OK},  {"RU", STRSID_OK},  {"AN", STRSID_OK},  {"AU", STRSID_OK},
        {"BA", STRSID_OK},  {"BG", STRSID_OK},  {"BO", STRSID_OK},  {"BU", STRSID_OK},
        {"CA", STRSID_OPT}, {"CG", STRSID_OK},  {"CO", STRSID_OK},  {"DA", STRSID_OPT},
        {"DC", STRSID_OPT}, {"DD", STRSID_OPT}, {"DG", STRSID_OPT}, {"DU", STRSID_OPT},
        {"EA", STRSID_OPT}, {"ED", STRSID_OK},  {"WD", STRSID_OK},  {"PA", STRSID_OPT},
        {"IU", STRSID_OK},  {"LA", STRSID_OK},  {"LG", STRSID_OK},  {"LS", STRSID_OK},
        {"SY", STRSID_OK},  {"NU", STRSID_OK},  {"NO", STRSID_OK},  {"NS", STRSID_OK},
        {"PO", STRSID_OK},  {"PS", STRSID_OK},  {"PU", STRSID_OK},  {"RS", STRSID_OPT},
        {"RD", STRSID_OK},  {"RE", STRSID_OK},  {"RC", STRSID_OK},  {"SA", STRSID_OPT},
        {"SO", STRSID_OK},  {"SU", STRSID_OK}};

    const char noSubAuthStr[] = "S-1-5";
    unsigned int i;
    PSID psid = NULL;
    SID *pisid;
    BOOL r;
    LPSTR str = NULL;

    if( !pConvertSidToStringSidA || !pConvertStringSidToSidA )
    {
        win_skip("ConvertSidToStringSidA or ConvertStringSidToSidA not available\n");
        return;
    }

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
    pisid = psid;
    ok(pisid->SubAuthorityCount == 4, "Invalid sub authority count - expected 4, got %d\n", pisid->SubAuthorityCount);
    ok(pisid->SubAuthority[0] == 21, "Invalid subauthority 0 - expected 21, got %d\n", pisid->SubAuthority[0]);
    ok(pisid->SubAuthority[3] == 4576, "Invalid subauthority 0 - expected 4576, got %d\n", pisid->SubAuthority[3]);
    LocalFree(str);
    LocalFree(psid);

    for( i = 0; i < sizeof(refs) / sizeof(refs[0]); i++ )
    {
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
        pisid = psid;
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

    /* string constant format not supported before XP */
    r = pConvertStringSidToSidA(strsid_table[0].str, &psid);
    if(!r)
    {
        win_skip("String constant format not supported\n");
        return;
    }
    LocalFree(psid);

    for(i = 0; i < sizeof(strsid_table) / sizeof(strsid_table[0]); i++)
    {
        char *temp;

        SetLastError(0xdeadbeef);
        r = pConvertStringSidToSidA(strsid_table[i].str, &psid);

        if (!(strsid_table[i].flags & STRSID_OPT))
        {
            ok(r, "%s: got %u\n", strsid_table[i].str, GetLastError());
        }

        if (r)
        {
            if ((winetest_debug > 1) && (pConvertSidToStringSidA(psid, &temp)))
            {
                trace(" %s: %s\n", strsid_table[i].str, temp);
                LocalFree(temp);
            }
            LocalFree(psid);
        }
        else
        {
            if (GetLastError() != ERROR_INVALID_SID)
                trace(" %s: couldn't be converted, returned %d\n", strsid_table[i].str, GetLastError());
            else
                trace(" %s: couldn't be converted\n", strsid_table[i].str);
        }
    }
}

static void test_trustee(void)
{
    GUID ObjectType = {0x12345678, 0x1234, 0x5678, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};
    GUID InheritedObjectType = {0x23456789, 0x2345, 0x6786, {0x2, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}};
    GUID ZeroGuid;
    OBJECTS_AND_NAME_A oan;
    OBJECTS_AND_SID oas;
    TRUSTEEA trustee;
    PSID psid;
    char szObjectTypeName[] = "ObjectTypeName";
    char szInheritedObjectTypeName[] = "InheritedObjectTypeName";
    char szTrusteeName[] = "szTrusteeName";
    SID_IDENTIFIER_AUTHORITY auth = { {0x11,0x22,0,0,0, 0} };

    memset( &ZeroGuid, 0x00, sizeof (ZeroGuid) );

    pBuildTrusteeWithSidA = (void *)GetProcAddress( hmod, "BuildTrusteeWithSidA" );
    pBuildTrusteeWithNameA = (void *)GetProcAddress( hmod, "BuildTrusteeWithNameA" );
    pBuildTrusteeWithObjectsAndNameA = (void *)GetProcAddress (hmod, "BuildTrusteeWithObjectsAndNameA" );
    pBuildTrusteeWithObjectsAndSidA = (void *)GetProcAddress (hmod, "BuildTrusteeWithObjectsAndSidA" );
    pGetTrusteeNameA = (void *)GetProcAddress (hmod, "GetTrusteeNameA" );
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
    ok( trustee.ptstrName == psid, "ptstrName wrong\n" );

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
    ok(trustee.ptstrName == (LPSTR)&oan, "ptstrName wrong\n");
 
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
    ok(trustee.ptstrName == (LPSTR)&oan, "ptstrName wrong\n");
 
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
#define SE_CHANGE_NOTIFY_PRIVILEGE       23L
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
    for (i = SE_MIN_WELL_KNOWN_PRIVILEGE; i <= SE_MAX_WELL_KNOWN_PRIVILEGE; i++)
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
     { "SeChangeNotifyPrivilege", SE_CHANGE_NOTIFY_PRIVILEGE },
     { "SeRemoteShutdownPrivilege", SE_REMOTE_SHUTDOWN_PRIVILEGE },
     { "SeUndockPrivilege", SE_UNDOCK_PRIVILEGE },
     { "SeSyncAgentPrivilege", SE_SYNC_AGENT_PRIVILEGE },
     { "SeEnableDelegationPrivilege", SE_ENABLE_DELEGATION_PRIVILEGE },
     { "SeManageVolumePrivilege", SE_MANAGE_VOLUME_PRIVILEGE },
     { "SeImpersonatePrivilege", SE_IMPERSONATE_PRIVILEGE },
     { "SeCreateGlobalPrivilege", SE_CREATE_GLOBAL_PRIVILEGE },
    };
    BOOL (WINAPI *pLookupPrivilegeValueA)(LPCSTR, LPCSTR, PLUID);
    unsigned int i;
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
    char wintmpdir [MAX_PATH];
    char path [MAX_PATH];
    char file [MAX_PATH];
    HANDLE fh, token;
    DWORD sdSize, retSize, rc, granted, priv_set_len;
    PRIVILEGE_SET priv_set;
    BOOL status;
    BYTE *sd;
    GENERIC_MAPPING mapping = { FILE_READ_DATA, FILE_WRITE_DATA, FILE_EXECUTE, FILE_ALL_ACCESS };
    const SECURITY_INFORMATION request = OWNER_SECURITY_INFORMATION
                                       | GROUP_SECURITY_INFORMATION
                                       | DACL_SECURITY_INFORMATION;

    if (!pGetFileSecurityA) {
        win_skip ("GetFileSecurity is not available\n");
        return;
    }

    if (!pSetFileSecurityA) {
        win_skip ("SetFileSecurity is not available\n");
        return;
    }

    if (!GetTempPathA (sizeof (wintmpdir), wintmpdir)) {
        win_skip ("GetTempPathA failed\n");
        return;
    }

    /* Create a temporary directory and in it a temporary file */
    strcat (strcpy (path, wintmpdir), "rary");
    SetLastError(0xdeadbeef);
    rc = CreateDirectoryA (path, NULL);
    ok (rc || GetLastError() == ERROR_ALREADY_EXISTS, "CreateDirectoryA "
        "failed for '%s' with %d\n", path, GetLastError());

    strcat (strcpy (file, path), "\\ess");
    SetLastError(0xdeadbeef);
    fh = CreateFileA (file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok (fh != INVALID_HANDLE_VALUE, "CreateFileA "
        "failed for '%s' with %d\n", file, GetLastError());
    CloseHandle (fh);

    /* For the temporary file ... */

    /* Get size needed */
    retSize = 0;
    SetLastError(0xdeadbeef);
    rc = pGetFileSecurityA (file, request, NULL, 0, &retSize);
    if (!rc && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) {
        win_skip("GetFileSecurityA is not implemented\n");
        goto cleanup;
    }
    ok (!rc, "GetFileSecurityA "
        "was expected to fail for '%s'\n", file);
    ok (GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetFileSecurityA "
        "returned %d; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
    ok (retSize > sizeof (SECURITY_DESCRIPTOR), "GetFileSecurityA returned size %d\n", retSize);

    sdSize = retSize;
    sd = HeapAlloc (GetProcessHeap (), 0, sdSize);

    /* Get security descriptor for real */
    retSize = -1;
    SetLastError(0xdeadbeef);
    rc = pGetFileSecurityA (file, request, sd, sdSize, &retSize);
    ok (rc, "GetFileSecurityA "
        "was not expected to fail '%s': %d\n", file, GetLastError());
    ok (retSize == sdSize ||
        broken(retSize == 0), /* NT4 */
        "GetFileSecurityA returned size %d; expected %d\n", retSize, sdSize);

    /* Use it to set security descriptor */
    SetLastError(0xdeadbeef);
    rc = pSetFileSecurityA (file, request, sd);
    ok (rc, "SetFileSecurityA "
        "was not expected to fail '%s': %d\n", file, GetLastError());

    HeapFree (GetProcessHeap (), 0, sd);

    /* Repeat for the temporary directory ... */

    /* Get size needed */
    retSize = 0;
    SetLastError(0xdeadbeef);
    rc = pGetFileSecurityA (path, request, NULL, 0, &retSize);
    ok (!rc, "GetFileSecurityA "
        "was expected to fail for '%s'\n", path);
    ok (GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetFileSecurityA "
        "returned %d; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
    ok (retSize > sizeof (SECURITY_DESCRIPTOR), "GetFileSecurityA returned size %d\n", retSize);

    sdSize = retSize;
    sd = HeapAlloc (GetProcessHeap (), 0, sdSize);

    /* Get security descriptor for real */
    retSize = -1;
    SetLastError(0xdeadbeef);
    rc = pGetFileSecurityA (path, request, sd, sdSize, &retSize);
    ok (rc, "GetFileSecurityA "
        "was not expected to fail '%s': %d\n", path, GetLastError());
    ok (retSize == sdSize ||
        broken(retSize == 0), /* NT4 */
        "GetFileSecurityA returned size %d; expected %d\n", retSize, sdSize);

    /* Use it to set security descriptor */
    SetLastError(0xdeadbeef);
    rc = pSetFileSecurityA (path, request, sd);
    ok (rc, "SetFileSecurityA "
        "was not expected to fail '%s': %d\n", path, GetLastError());
    HeapFree (GetProcessHeap (), 0, sd);

    /* Old test */
    strcpy (wintmpdir, "\\Should not exist");
    SetLastError(0xdeadbeef);
    rc = pGetFileSecurityA (wintmpdir, OWNER_SECURITY_INFORMATION, NULL, 0, &sdSize);
    ok (!rc, "GetFileSecurityA should fail for not existing directories/files\n");
    ok (GetLastError() == ERROR_FILE_NOT_FOUND,
        "last error ERROR_FILE_NOT_FOUND expected, got %d\n", GetLastError());

cleanup:
    /* Remove temporary file and directory */
    DeleteFileA(file);
    RemoveDirectoryA(path);

    /* Test file access permissions for a file with FILE_ATTRIBUTE_ARCHIVE */
    SetLastError(0xdeadbeef);
    rc = GetTempPathA(sizeof(wintmpdir), wintmpdir);
    ok(rc, "GetTempPath error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = GetTempFileNameA(wintmpdir, "tmp", 0, file);
    ok(rc, "GetTempFileName error %d\n", GetLastError());

    rc = GetFileAttributesA(file);
    rc &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_COMPRESSED);
    ok(rc == FILE_ATTRIBUTE_ARCHIVE, "expected FILE_ATTRIBUTE_ARCHIVE got %#x\n", rc);

    rc = GetFileSecurityA(file, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
                          NULL, 0, &sdSize);
    ok(!rc, "GetFileSecurity should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "expected ERROR_INSUFFICIENT_BUFFER got %d\n", GetLastError());
    ok(sdSize > sizeof(SECURITY_DESCRIPTOR), "got sd size %d\n", sdSize);

    sd = HeapAlloc(GetProcessHeap (), 0, sdSize);
    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA(file, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
                          sd, sdSize, &retSize);
    ok(rc, "GetFileSecurity error %d\n", GetLastError());
    ok(retSize == sdSize || broken(retSize == 0) /* NT4 */, "expected %d, got %d\n", sdSize, retSize);

    SetLastError(0xdeadbeef);
    rc = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token);
    ok(!rc, "OpenThreadToken should fail\n");
    ok(GetLastError() == ERROR_NO_TOKEN, "expected ERROR_NO_TOKEN, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = ImpersonateSelf(SecurityIdentification);
    ok(rc, "ImpersonateSelf error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token);
    ok(rc, "OpenThreadToken error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    rc = RevertToSelf();
    ok(rc, "RevertToSelf error %d\n", GetLastError());

    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_READ_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_READ_DATA, "expected FILE_READ_DATA, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_WRITE_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_WRITE_DATA, "expected FILE_WRITE_DATA, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_EXECUTE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_EXECUTE, "expected FILE_EXECUTE, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, DELETE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == DELETE, "expected DELETE, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_DELETE_CHILD, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_DELETE_CHILD, "expected FILE_DELETE_CHILD, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, 0x1ff, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == 0x1ff, "expected 0x1ff, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_ALL_ACCESS, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#x\n", granted);

    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, 0xffffffff, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(!rc, "AccessCheck should fail\n");
    ok(GetLastError() == ERROR_GENERIC_NOT_MAPPED, "expected ERROR_GENERIC_NOT_MAPPED, got %d\n", GetLastError());

    /* Test file access permissions for a file with FILE_ATTRIBUTE_READONLY */
    SetLastError(0xdeadbeef);
    fh = CreateFileA(file, FILE_READ_DATA, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok(fh != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = WriteFile(fh, "1", 1, &retSize, NULL);
    ok(!rc, "WriteFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    ok(retSize == 0, "expected 0, got %d\n", retSize);
    CloseHandle(fh);

    rc = GetFileAttributesA(file);
    rc &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_COMPRESSED);
todo_wine
    ok(rc == (FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY),
       "expected FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY got %#x\n", rc);

    SetLastError(0xdeadbeef);
    rc = SetFileAttributesA(file, FILE_ATTRIBUTE_ARCHIVE);
    ok(rc, "SetFileAttributes error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    rc = DeleteFileA(file);
    ok(rc, "DeleteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    fh = CreateFileA(file, FILE_READ_DATA, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok(fh != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = WriteFile(fh, "1", 1, &retSize, NULL);
    ok(!rc, "WriteFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    ok(retSize == 0, "expected 0, got %d\n", retSize);
    CloseHandle(fh);

    rc = GetFileAttributesA(file);
    rc &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_COMPRESSED);
    ok(rc == (FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY),
       "expected FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY got %#x\n", rc);

    retSize = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = GetFileSecurityA(file, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
                          sd, sdSize, &retSize);
    ok(rc, "GetFileSecurity error %d\n", GetLastError());
    ok(retSize == sdSize || broken(retSize == 0) /* NT4 */, "expected %d, got %d\n", sdSize, retSize);

    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_READ_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_READ_DATA, "expected FILE_READ_DATA, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_WRITE_DATA, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_WRITE_DATA, "expected FILE_WRITE_DATA, got %#x\n", granted);
}
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_EXECUTE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_EXECUTE, "expected FILE_EXECUTE, got %#x\n", granted);

    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, DELETE, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == DELETE, "expected DELETE, got %#x\n", granted);
}
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_DELETE_CHILD, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_DELETE_CHILD, "expected FILE_DELETE_CHILD, got %#x\n", granted);
}
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, 0x1ff, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == 0x1ff, "expected 0x1ff, got %#x\n", granted);
}
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    rc = AccessCheck(sd, token, FILE_ALL_ACCESS, &mapping, &priv_set, &priv_set_len, &granted, &status);
    ok(rc, "AccessCheck error %d\n", GetLastError());
todo_wine {
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#x\n", granted);
}
    SetLastError(0xdeadbeef);
    rc = DeleteFileA(file);
    ok(!rc, "DeleteFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    rc = SetFileAttributesA(file, FILE_ATTRIBUTE_ARCHIVE);
    ok(rc, "SetFileAttributes error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    rc = DeleteFileA(file);
    ok(rc, "DeleteFile error %d\n", GetLastError());

    CloseHandle(token);
    HeapFree(GetProcessHeap(), 0, sd);
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
    NTSTATUS ntret, ntAccessStatus;

    NtDllModule = GetModuleHandleA("ntdll.dll");
    if (!NtDllModule)
    {
        skip("not running on NT, skipping test\n");
        return;
    }
    pRtlAdjustPrivilege = (void *)GetProcAddress(NtDllModule, "RtlAdjustPrivilege");
    if (!pRtlAdjustPrivilege)
    {
        win_skip("missing RtlAdjustPrivilege, skipping test\n");
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
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, KEY_QUERY_VALUE, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_INVALID_SECURITY_DESCR, "AccessCheck should have "
       "failed with ERROR_INVALID_SECURITY_DESCR, instead of %d\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Set owner and group */
    res = SetSecurityDescriptorOwner(SecurityDescriptor, AdminSid, FALSE);
    ok(res, "SetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    res = SetSecurityDescriptorGroup(SecurityDescriptor, UsersSid, TRUE);
    ok(res, "SetSecurityDescriptorGroup failed with error %d\n", GetLastError());

    /* Generic access mask */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_GENERIC_NOT_MAPPED, "AccessCheck should have failed "
       "with ERROR_GENERIC_NOT_MAPPED, instead of %d\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Generic access mask - no privilegeset buffer */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      NULL, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %d\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Generic access mask - no returnlength */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      PrivSet, NULL, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %d\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* Generic access mask - no privilegeset buffer, no returnlength */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      NULL, NULL, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %d\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    /* sd with no dacl present */
    Access = AccessStatus = 0x1abe11ed;
    ret = SetSecurityDescriptorDacl(SecurityDescriptor, FALSE, NULL, FALSE);
    ok(ret, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    ret = AccessCheck(SecurityDescriptor, Token, KEY_READ, &Mapping,
                      PrivSet, &PrivSetLen, &Access, &AccessStatus);
    ok(ret, "AccessCheck failed with error %d\n", GetLastError());
    ok(AccessStatus && (Access == KEY_READ),
        "AccessCheck failed to grant access with error %d\n",
        GetLastError());

    /* sd with no dacl present - no privilegeset buffer */
    SetLastError(0xdeadbeef);
    Access = AccessStatus = 0x1abe11ed;
    ret = AccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                      NULL, &PrivSetLen, &Access, &AccessStatus);
    err = GetLastError();
    ok(!ret && err == ERROR_NOACCESS, "AccessCheck should have failed "
       "with ERROR_NOACCESS, instead of %d\n", err);
    ok(Access == 0x1abe11ed && AccessStatus == 0x1abe11ed,
       "Access and/or AccessStatus were changed!\n");

    if(pNtAccessCheck)
    {
       /* Generic access mask - no privilegeset buffer */
       SetLastError(0xdeadbeef);
       Access = ntAccessStatus = 0x1abe11ed;
       ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                              NULL, &PrivSetLen, &Access, &ntAccessStatus);
       err = GetLastError();
       ok(ntret == STATUS_ACCESS_VIOLATION,
          "NtAccessCheck should have failed with STATUS_ACCESS_VIOLATION, got %x\n", ntret);
       ok(err == 0xdeadbeef,
          "NtAccessCheck shouldn't set last error, got %d\n", err);
       ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
          "Access and/or AccessStatus were changed!\n");

      /* Generic access mask - no returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                             PrivSet, NULL, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_ACCESS_VIOLATION,
         "NtAccessCheck should have failed with STATUS_ACCESS_VIOLATION, got %x\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %d\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");

      /* Generic access mask - no privilegeset buffer, no returnlength */
      SetLastError(0xdeadbeef);
      Access = ntAccessStatus = 0x1abe11ed;
      ntret = pNtAccessCheck(SecurityDescriptor, Token, GENERIC_READ, &Mapping,
                             NULL, NULL, &Access, &ntAccessStatus);
      err = GetLastError();
      ok(ntret == STATUS_ACCESS_VIOLATION,
         "NtAccessCheck should have failed with STATUS_ACCESS_VIOLATION, got %x\n", ntret);
      ok(err == 0xdeadbeef,
         "NtAccessCheck shouldn't set last error, got %d\n", err);
      ok(Access == 0x1abe11ed && ntAccessStatus == 0x1abe11ed,
         "Access and/or AccessStatus were changed!\n");
    }
    else
       win_skip("NtAccessCheck unavailable. Skipping.\n");

    /* sd with NULL dacl */
    Access = AccessStatus = 0x1abe11ed;
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
    Access = AccessStatus = 0x1abe11ed;
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
    Access = AccessStatus = 0x1abe11ed;
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
        win_skip("AddAccessAllowedAceEx is not available\n");

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
    DWORD Size, Size2;
    TOKEN_PRIVILEGES *Privileges;
    TOKEN_GROUPS *Groups;
    TOKEN_USER *User;
    TOKEN_DEFAULT_DACL *Dacl;
    BOOL ret;
    DWORD i, GLE;
    LPSTR SidString;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    ACL *acl;

    /* cygwin-like use case */
    SetLastError(0xdeadbeef);
    ret = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("OpenProcessToken is not implemented\n");
        return;
    }
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());
    if (ret)
    {
        DWORD buf[256]; /* GetTokenInformation wants a dword-aligned buffer */
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
        win_skip("ConvertSidToStringSidA is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &Token);
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());

    /* groups */
    /* insufficient buffer length */
    SetLastError(0xdeadbeef);
    Size2 = 0;
    ret = GetTokenInformation(Token, TokenGroups, NULL, 0, &Size2);
    ok(Size2 > 1, "got %d\n", Size2);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "%d with error %d\n", ret, GetLastError());
    Size2 -= 1;
    Groups = HeapAlloc(GetProcessHeap(), 0, Size2);
    memset(Groups, 0xcc, Size2);
    Size = 0;
    ret = GetTokenInformation(Token, TokenGroups, Groups, Size2, &Size);
    ok(Size > 1, "got %d\n", Size);
    ok((!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER) || broken(ret) /* wow64 */,
        "%d with error %d\n", ret, GetLastError());
    if(!ret)
        ok(*((BYTE*)Groups) == 0xcc, "buffer altered\n");

    HeapFree(GetProcessHeap(), 0, Groups);

    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(Token, TokenGroups, NULL, 0, &Size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "GetTokenInformation(TokenGroups) %s with error %d\n",
        ret ? "succeeded" : "failed", GetLastError());
    Groups = HeapAlloc(GetProcessHeap(), 0, Size);
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(Token, TokenGroups, Groups, Size, &Size);
    ok(ret, "GetTokenInformation(TokenGroups) failed with error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef,
       "GetTokenInformation shouldn't have set last error to %d\n",
       GetLastError());
    trace("TokenGroups:\n");
    for (i = 0; i < Groups->GroupCount; i++)
    {
        DWORD NameLength = 255;
        CHAR Name[255];
        DWORD DomainLength = 255;
        CHAR Domain[255];
        SID_NAME_USE SidNameUse;
        Name[0] = '\0';
        Domain[0] = '\0';
        ret = LookupAccountSidA(NULL, Groups->Groups[i].Sid, Name, &NameLength, Domain, &DomainLength, &SidNameUse);
        if (ret)
        {
            pConvertSidToStringSidA(Groups->Groups[i].Sid, &SidString);
            trace("%s, %s\\%s use: %d attr: 0x%08x\n", SidString, Domain, Name, SidNameUse, Groups->Groups[i].Attributes);
            LocalFree(SidString);
        }
        else trace("attr: 0x%08x LookupAccountSid failed with error %d\n", Groups->Groups[i].Attributes, GetLastError());
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
        CHAR Name[256];
        DWORD NameLen = sizeof(Name)/sizeof(Name[0]);
        LookupPrivilegeNameA(NULL, &Privileges->Privileges[i].Luid, Name, &NameLen);
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

    /* default dacl */
    ret = GetTokenInformation(Token, TokenDefaultDacl, NULL, 0, &Size);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenDefaultDacl) failed with error %u\n", GetLastError());

    Dacl = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenDefaultDacl, Dacl, Size, &Size);
    ok(ret, "GetTokenInformation(TokenDefaultDacl) failed with error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetTokenInformation(Token, TokenDefaultDacl, NULL, 0);
    GLE = GetLastError();
    ok(!ret, "SetTokenInformation(TokenDefaultDacl) succeeded\n");
    ok(GLE == ERROR_BAD_LENGTH, "expected ERROR_BAD_LENGTH got %u\n", GLE);

    SetLastError(0xdeadbeef);
    ret = SetTokenInformation(Token, TokenDefaultDacl, NULL, Size);
    GLE = GetLastError();
    ok(!ret, "SetTokenInformation(TokenDefaultDacl) succeeded\n");
    ok(GLE == ERROR_NOACCESS, "expected ERROR_NOACCESS got %u\n", GLE);

    acl = Dacl->DefaultDacl;
    Dacl->DefaultDacl = NULL;

    ret = SetTokenInformation(Token, TokenDefaultDacl, Dacl, Size);
    ok(ret, "SetTokenInformation(TokenDefaultDacl) succeeded\n");

    Size2 = 0;
    Dacl->DefaultDacl = (ACL *)0xdeadbeef;
    ret = GetTokenInformation(Token, TokenDefaultDacl, Dacl, Size, &Size2);
    ok(ret, "GetTokenInformation(TokenDefaultDacl) failed with error %u\n", GetLastError());
    ok(Dacl->DefaultDacl == NULL, "expected NULL, got %p\n", Dacl->DefaultDacl);
    ok(Size2 == sizeof(TOKEN_DEFAULT_DACL) || broken(Size2 == 2*sizeof(TOKEN_DEFAULT_DACL)), /* WoW64 */
       "got %u expected sizeof(TOKEN_DEFAULT_DACL)\n", Size2);

    Dacl->DefaultDacl = acl;
    ret = SetTokenInformation(Token, TokenDefaultDacl, Dacl, Size);
    ok(ret, "SetTokenInformation(TokenDefaultDacl) failed with error %u\n", GetLastError());

    if (Size2 == sizeof(TOKEN_DEFAULT_DACL)) {
        ret = GetTokenInformation(Token, TokenDefaultDacl, Dacl, Size, &Size2);
        ok(ret, "GetTokenInformation(TokenDefaultDacl) failed with error %u\n", GetLastError());
    } else
        win_skip("TOKEN_DEFAULT_DACL size too small on WoW64\n");

    HeapFree(GetProcessHeap(), 0, Dacl);
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
        ret = LookupAccountSidA (NULL, sid, account, &acc_size, domain, &dom_size, &use);
        ok(ret || (!ret && (GetLastError() == ERROR_NONE_MAPPED)),
           "LookupAccountSid(%s) failed: %d\n", str_sid, GetLastError());
        if (ret)
            trace(" %s %s\\%s %d\n", str_sid, domain, account, use);
        else if (GetLastError() == ERROR_NONE_MAPPED)
            trace(" %s couldn't be mapped\n", str_sid);
        LocalFree(str_sid);
    }
}

static const struct well_known_sid_value
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
/* 74 */ {TRUE, "S-1-5-22"}, {FALSE, "S-1-5-21-12-23-34-45-56-521"}, {TRUE, "S-1-5-32-573"},
/* 77 */ {FALSE, "S-1-5-21-12-23-34-45-56-498"}, {TRUE, "S-1-5-32-574"}, {TRUE, "S-1-16-8448"}
};

static void test_CreateWellKnownSid(void)
{
    SID_IDENTIFIER_AUTHORITY ident = { SECURITY_NT_AUTHORITY };
    PSID domainsid, sid;
    DWORD size, error;
    BOOL ret;
    unsigned int i;

    if (!pCreateWellKnownSid)
    {
        win_skip("CreateWellKnownSid not available\n");
        return;
    }

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pCreateWellKnownSid(WinInteractiveSid, NULL, NULL, &size);
    error = GetLastError();
    ok(!ret, "CreateWellKnownSid succeeded\n");
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", error);
    ok(size, "expected size > 0\n");

    SetLastError(0xdeadbeef);
    ret = pCreateWellKnownSid(WinInteractiveSid, NULL, NULL, &size);
    error = GetLastError();
    ok(!ret, "CreateWellKnownSid succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", error);

    sid = HeapAlloc(GetProcessHeap(), 0, size);
    ret = pCreateWellKnownSid(WinInteractiveSid, NULL, sid, &size);
    ok(ret, "CreateWellKnownSid failed %u\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, sid);

    /* a domain sid usually have three subauthorities but we test that CreateWellKnownSid doesn't check it */
    AllocateAndInitializeSid(&ident, 6, SECURITY_NT_NON_UNIQUE, 12, 23, 34, 45, 56, 0, 0, &domainsid);

    for (i = 0; i < sizeof(well_known_sid_values)/sizeof(well_known_sid_values[0]); i++)
    {
        const struct well_known_sid_value *value = &well_known_sid_values[i];
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
                skip("Well known SIDs starting from %u are not implemented\n", i);
                break;
            }
        }

        cb = sizeof(sid_buffer);
        ok(pCreateWellKnownSid(i, value->without_domain ? NULL : domainsid, sid_buffer, &cb), "Couldn't create well known sid %u\n", i);
        expect_eq(GetSidLengthRequired(*GetSidSubAuthorityCount(sid_buffer)), cb, DWORD, "%d");
        ok(IsValidSid(sid_buffer), "The sid is not valid\n");
        ok(pConvertSidToStringSidA(sid_buffer, &str), "Couldn't convert SID to string\n");
        ok(strcmp(str, value->sid_string) == 0, "%d: SID mismatch - expected %s, got %s\n", i,
            value->sid_string, str);
        LocalFree(str);

        if (value->without_domain)
        {
            char buf2[SECURITY_MAX_SID_SIZE];
            cb = sizeof(buf2);
            ok(pCreateWellKnownSid(i, domainsid, buf2, &cb), "Couldn't create well known sid %u with optional domain\n", i);
            expect_eq(GetSidLengthRequired(*GetSidSubAuthorityCount(sid_buffer)), cb, DWORD, "%d");
            ok(memcmp(buf2, sid_buffer, cb) == 0, "SID create with domain is different than without (%u)\n", i);
        }
    }

    FreeSid(domainsid);
}

static void test_LookupAccountSid(void)
{
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    CHAR accountA[MAX_PATH], domainA[MAX_PATH], usernameA[MAX_PATH];
    DWORD acc_sizeA, dom_sizeA, user_sizeA;
    DWORD real_acc_sizeA, real_dom_sizeA;
    WCHAR accountW[MAX_PATH], domainW[MAX_PATH];
    DWORD acc_sizeW, dom_sizeW;
    DWORD real_acc_sizeW, real_dom_sizeW;
    PSID pUsersSid = NULL;
    SID_NAME_USE use;
    BOOL ret;
    DWORD error, size, cbti = 0;
    MAX_SID  max_sid;
    CHAR *str_sidA;
    int i;
    HANDLE hToken;
    PTOKEN_USER ptiUser = NULL;

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
    LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeA == real_acc_sizeA + 1,
       "LookupAccountSidA() Expected acc_size = %u, got %u\n",
       real_acc_sizeA + 1, acc_sizeA);

    /* try a 0 sized account buffer */
    acc_sizeA = 0;
    dom_sizeA = MAX_PATH;
    LookupAccountSidA(NULL, pUsersSid, NULL, &acc_sizeA, domainA, &dom_sizeA, &use);
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
    LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeA == real_dom_sizeA + 1,
       "LookupAccountSidA() Expected dom_size = %u, got %u\n",
       real_dom_sizeA + 1, dom_sizeA);

    /* try a 0 sized domain buffer */
    dom_sizeA = 0;
    acc_sizeA = MAX_PATH;
    LookupAccountSidA(NULL, pUsersSid, accountA, &acc_sizeA, NULL, &dom_sizeA, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeA == real_dom_sizeA + 1,
       "LookupAccountSidA() Expected dom_size = %u, got %u\n",
       real_dom_sizeA + 1, dom_sizeA);

    real_acc_sizeW = MAX_PATH;
    real_dom_sizeW = MAX_PATH;
    ret = LookupAccountSidW(NULL, pUsersSid, accountW, &real_acc_sizeW, domainW, &real_dom_sizeW, &use);
    ok(ret, "LookupAccountSidW() Expected TRUE, got FALSE\n");

    /* try an invalid system name */
    real_acc_sizeA = MAX_PATH;
    real_dom_sizeA = MAX_PATH;
    ret = LookupAccountSidA("deepthought", pUsersSid, accountA, &real_acc_sizeA, domainA, &real_dom_sizeA, &use);
    ok(!ret, "LookupAccountSidA() Expected FALSE got TRUE\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE || GetLastError() == RPC_S_INVALID_NET_ADDR /* Vista */,
       "LookupAccountSidA() Expected RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR, got %u\n", GetLastError());

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
    LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(acc_sizeW == real_acc_sizeW + 1,
       "LookupAccountSidW() Expected acc_size = %u, got %u\n",
       real_acc_sizeW + 1, acc_sizeW);

    /* try a 0 sized account buffer */
    acc_sizeW = 0;
    dom_sizeW = MAX_PATH;
    LookupAccountSidW(NULL, pUsersSid, NULL, &acc_sizeW, domainW, &dom_sizeW, &use);
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
    LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, domainW, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeW == real_dom_sizeW + 1,
       "LookupAccountSidW() Expected dom_size = %u, got %u\n",
       real_dom_sizeW + 1, dom_sizeW);

    /* try a 0 sized domain buffer */
    dom_sizeW = 0;
    acc_sizeW = MAX_PATH;
    LookupAccountSidW(NULL, pUsersSid, accountW, &acc_sizeW, NULL, &dom_sizeW, &use);
    /* this can fail or succeed depending on OS version but the size will always be returned */
    ok(dom_sizeW == real_dom_sizeW + 1,
       "LookupAccountSidW() Expected dom_size = %u, got %u\n",
       real_dom_sizeW + 1, dom_sizeW);

    acc_sizeW = dom_sizeW = use = 0;
    SetLastError(0xdeadbeef);
    ret = LookupAccountSidW(NULL, pUsersSid, NULL, &acc_sizeW, NULL, &dom_sizeW, &use);
    error = GetLastError();
    ok(!ret, "LookupAccountSidW failed %u\n", GetLastError());
    ok(error == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %u\n", error);
    ok(acc_sizeW, "expected non-zero account size\n");
    ok(dom_sizeW, "expected non-zero domain size\n");
    ok(!use, "expected zero use %u\n", use);

    FreeSid(pUsersSid);

    /* Test LookupAccountSid with Sid retrieved from token information.
     This assumes this process is running under the account of the current user.*/
    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE, &hToken);
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());
    ret = GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti);
    ok(!ret, "GetTokenInformation failed with error %d\n", GetLastError());
    ptiUser = HeapAlloc(GetProcessHeap(), 0, cbti);
    if (GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti))
    {
        acc_sizeA = dom_sizeA = MAX_PATH;
        ret = LookupAccountSidA(NULL, ptiUser->User.Sid, accountA, &acc_sizeA, domainA, &dom_sizeA, &use);
        ok(ret, "LookupAccountSidA() Expected TRUE, got FALSE\n");
        user_sizeA = MAX_PATH;
        ret = GetUserNameA(usernameA , &user_sizeA);
        ok(ret, "GetUserNameA() Expected TRUE, got FALSE\n");
        ok(lstrcmpA(usernameA, accountA) == 0, "LookupAccountSidA() Expected account name: %s got: %s\n", usernameA, accountA );
    }
    HeapFree(GetProcessHeap(), 0, ptiUser);

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

        pLsaQueryInformationPolicy = (void *)GetProcAddress( hmod, "LsaQueryInformationPolicy");
        pLsaOpenPolicy = (void *)GetProcAddress( hmod, "LsaOpenPolicy");
        pLsaFreeMemory = (void *)GetProcAddress( hmod, "LsaFreeMemory");
        pLsaClose = (void *)GetProcAddress( hmod, "LsaClose");

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

static BOOL get_sid_info(PSID psid, LPSTR *user, LPSTR *dom)
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
    SetLastError(0xdeadbeef);
    return LookupAccountSidA(NULL, psid, account, &size, domain, &dom_size, &use);
}

static void check_wellknown_name(const char* name, WELL_KNOWN_SID_TYPE result)
{
    SID_IDENTIFIER_AUTHORITY ident = { SECURITY_NT_AUTHORITY };
    PSID domainsid = NULL;
    char wk_sid[SECURITY_MAX_SID_SIZE];
    DWORD cb;

    DWORD sid_size, domain_size;
    SID_NAME_USE sid_use;
    LPSTR domain, account, sid_domain, wk_domain, wk_account;
    PSID psid;
    BOOL ret ,ret2;

    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA(NULL, name, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, " %s Should have failed to lookup account name\n", name);
    psid = HeapAlloc(GetProcessHeap(),0,sid_size);
    domain = HeapAlloc(GetProcessHeap(),0,domain_size);
    ret = LookupAccountNameA(NULL, name, psid, &sid_size, domain, &domain_size, &sid_use);

    if (!result)
    {
        ok(!ret, " %s Should have failed to lookup account name\n",name);
        goto cleanup;
    }

    AllocateAndInitializeSid(&ident, 6, SECURITY_NT_NON_UNIQUE, 12, 23, 34, 45, 56, 0, 0, &domainsid);
    cb = sizeof(wk_sid);
    if (!pCreateWellKnownSid(result, domainsid, wk_sid, &cb))
    {
        win_skip("SID %i is not available on the system\n",result);
        goto cleanup;
    }

    ret2 = get_sid_info(wk_sid, &wk_account, &wk_domain);
    if (!ret2 && GetLastError() == ERROR_NONE_MAPPED)
    {
        win_skip("CreateWellKnownSid() succeeded but the account '%s' is not present (W2K)\n", name);
        goto cleanup;
    }

    get_sid_info(psid, &account, &sid_domain);

    ok(ret, "Failed to lookup account name %s\n",name);
    ok(sid_size != 0, "sid_size was zero\n");

    ok(EqualSid(psid,wk_sid),"(%s) Sids fail to match well known sid!\n",name);

    ok(!lstrcmpA(account, wk_account), "Expected %s , got %s\n", account, wk_account);
    ok(!lstrcmpA(domain, wk_domain), "Expected %s, got %s\n", wk_domain, domain);
    ok(sid_use == SidTypeWellKnownGroup , "Expected Use (5), got %d\n", sid_use);

cleanup:
    FreeSid(domainsid);
    HeapFree(GetProcessHeap(),0,psid);
    HeapFree(GetProcessHeap(),0,domain);
}

static void test_LookupAccountName(void)
{
    DWORD sid_size, domain_size, user_size;
    DWORD sid_save, domain_save;
    CHAR user_name[UNLEN + 1];
    CHAR computer_name[UNLEN + 1];
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
    ok(ret, "Failed to get user name : %d\n", GetLastError());

    /* get sizes */
    sid_size = 0;
    domain_size = 0;
    sid_use = 0xcafebabe;
    SetLastError(0xdeadbeef);
    ret = LookupAccountNameA(NULL, user_name, NULL, &sid_size, NULL, &domain_size, &sid_use);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("LookupAccountNameA is not implemented\n");
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
    ok(!lstrcmpA(account, user_name), "Expected %s, got %s\n", user_name, account);
    ok(!lstrcmpA(domain, sid_dom), "Expected %s, got %s\n", sid_dom, domain);
    ok(domain_size == domain_save - 1, "Expected %d, got %d\n", domain_save - 1, domain_size);
    ok(strlen(domain) == domain_size, "Expected %d, got %d\n", lstrlenA(domain), domain_size);
    ok(sid_use == SidTypeUser, "Expected SidTypeUser (%d), got %d\n", SidTypeUser, sid_use);
    domain_size = domain_save;
    sid_size = sid_save;

    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ret = LookupAccountNameA(NULL, "Everyone", psid, &sid_size, domain, &domain_size, &sid_use);
        get_sid_info(psid, &account, &sid_dom);
        ok(ret, "Failed to lookup account name\n");
        ok(sid_size != 0, "sid_size was zero\n");
        ok(!lstrcmpA(account, "Everyone"), "Expected Everyone, got %s\n", account);
        ok(!lstrcmpA(domain, sid_dom), "Expected %s, got %s\n", sid_dom, domain);
        ok(domain_size == 0, "Expected 0, got %d\n", domain_size);
        ok(strlen(domain) == domain_size, "Expected %d, got %d\n", lstrlenA(domain), domain_size);
        ok(sid_use == SidTypeWellKnownGroup, "Expected SidTypeWellKnownGroup (%d), got %d\n", SidTypeWellKnownGroup, sid_use);
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
    if (!ret && GetLastError() == ERROR_NONE_MAPPED)
        win_skip("NULL account name doesn't work on NT4\n");
    else
    {
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
        /* Using a fixed string will not work on different locales */
        ok(!lstrcmpA(account, domain),
           "Got %s for account and %s for domain, these should be the same\n",
           account, domain);
        ok(sid_use == SidTypeDomain, "Expected SidTypeDomain (%d), got %d\n", SidTypeDomain, sid_use);

        HeapFree(GetProcessHeap(), 0, psid);
        HeapFree(GetProcessHeap(), 0, domain);
    }

    /* try an invalid account name */
    SetLastError(0xdeadbeef);
    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA(NULL, "oogabooga", NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == ERROR_NONE_MAPPED ||
       broken(GetLastError() == ERROR_TRUSTED_RELATIONSHIP_FAILURE),
       "Expected ERROR_NONE_MAPPED, got %d\n", GetLastError());
    ok(sid_size == 0, "Expected 0, got %d\n", sid_size);
    ok(domain_size == 0, "Expected 0, got %d\n", domain_size);

    /* try an invalid system name */
    SetLastError(0xdeadbeef);
    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA("deepthought", NULL, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret, "Expected 0, got %d\n", ret);
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE || GetLastError() == RPC_S_INVALID_NET_ADDR /* Vista */,
       "Expected RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR, got %d\n", GetLastError());
    ok(sid_size == 0, "Expected 0, got %d\n", sid_size);
    ok(domain_size == 0, "Expected 0, got %d\n", domain_size);

    /* try with the computer name as the account name */
    domain_size = sizeof(computer_name);
    GetComputerNameA(computer_name, &domain_size);
    sid_size = 0;
    domain_size = 0;
    ret = LookupAccountNameA(NULL, computer_name, NULL, &sid_size, NULL, &domain_size, &sid_use);
    ok(!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
       GetLastError() == ERROR_NONE_MAPPED /* in a domain */ ||
       broken(GetLastError() == ERROR_TRUSTED_DOMAIN_FAILURE) ||
       broken(GetLastError() == ERROR_TRUSTED_RELATIONSHIP_FAILURE)),
       "LookupAccountNameA failed: %d\n", GetLastError());
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        psid = HeapAlloc(GetProcessHeap(), 0, sid_size);
        domain = HeapAlloc(GetProcessHeap(), 0, domain_size);
        ret = LookupAccountNameA(NULL, computer_name, psid, &sid_size, domain, &domain_size, &sid_use);
        ok(ret, "LookupAccountNameA failed: %d\n", GetLastError());
        ok(sid_use == SidTypeDomain ||
           (sid_use == SidTypeUser && ! strcmp(computer_name, user_name)), "expected SidTypeDomain for %s, got %d\n", computer_name, sid_use);
        HeapFree(GetProcessHeap(), 0, domain);
        HeapFree(GetProcessHeap(), 0, psid);
    }

    /* Well Known names */
    if (!pCreateWellKnownSid)
    {
        win_skip("CreateWellKnownSid not available\n");
        return;
    }

    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (skipping well known name creation tests)\n");
        return;
    }

    check_wellknown_name("LocalService", WinLocalServiceSid);
    check_wellknown_name("Local Service", WinLocalServiceSid);
    /* 2 spaces */
    check_wellknown_name("Local  Service", 0);
    check_wellknown_name("NetworkService", WinNetworkServiceSid);
    check_wellknown_name("Network Service", WinNetworkServiceSid);

    /* example of some names where the spaces are not optional */
    check_wellknown_name("Terminal Server User", WinTerminalServerSid);
    check_wellknown_name("TerminalServer User", 0);
    check_wellknown_name("TerminalServerUser", 0);
    check_wellknown_name("Terminal ServerUser", 0);

    check_wellknown_name("enterprise domain controllers",WinEnterpriseControllersSid);
    check_wellknown_name("enterprisedomain controllers", 0);
    check_wellknown_name("enterprise domaincontrollers", 0);
    check_wellknown_name("enterprisedomaincontrollers", 0);

    /* case insensitivity */
    check_wellknown_name("lOCAlServICE", WinLocalServiceSid);

    /* fully qualified account names */
    check_wellknown_name("NT AUTHORITY\\LocalService", WinLocalServiceSid);
    check_wellknown_name("nt authority\\Network Service", WinNetworkServiceSid);
    check_wellknown_name("nt authority test\\Network Service", 0);
    check_wellknown_name("Dummy\\Network Service", 0);
    check_wellknown_name("ntauthority\\Network Service", 0);
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
        win_skip("InitializeSecurityDescriptor is not implemented\n");
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
        BOOL res_; \
        DWORD err; \
        SetLastError( 0xdeadbeef ); \
        res_ = SetKernelObjectSecurity( o, i, SecurityDescriptor ); \
        err = GetLastError(); \
        if (e == ERROR_SUCCESS) \
            ok(res_, "SetKernelObjectSecurity failed with %d\n", err); \
        else \
            ok(!res_ && err == e, "SetKernelObjectSecurity should have failed " \
               "with %s, instead of %d\n", #e, err); \
    }while(0)

static void test_process_security(void)
{
    BOOL res;
    PTOKEN_USER user;
    PTOKEN_OWNER owner;
    PTOKEN_PRIMARY_GROUP group;
    PSID AdminSid = NULL, UsersSid = NULL, UserSid = NULL;
    PACL Acl = NULL, ThreadAcl = NULL;
    SECURITY_DESCRIPTOR *SecurityDescriptor = NULL, *ThreadSecurityDescriptor = NULL;
    char buffer[MAX_PATH], account[MAX_PATH], domain[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    SECURITY_ATTRIBUTES psa, tsa;
    HANDLE token, event;
    DWORD size, acc_size, dom_size, ret;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    PSID EveryoneSid = NULL;
    SID_NAME_USE use;

    Acl = HeapAlloc(GetProcessHeap(), 0, 256);
    res = InitializeAcl(Acl, 256, ACL_REVISION);
    if (!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ACLs not implemented - skipping tests\n");
        HeapFree(GetProcessHeap(), 0, Acl);
        return;
    }
    ok(res, "InitializeAcl failed with error %d\n", GetLastError());

    res = AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &EveryoneSid);
    ok(res, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    /* get owner from the token we might be running as a user not admin */
    res = OpenProcessToken( GetCurrentProcess(), MAXIMUM_ALLOWED, &token );
    ok(res, "OpenProcessToken failed with error %d\n", GetLastError());
    if (!res)
    {
        HeapFree(GetProcessHeap(), 0, Acl);
        return;
    }

    res = GetTokenInformation( token, TokenOwner, NULL, 0, &size );
    ok(!res, "Expected failure, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    owner = HeapAlloc(GetProcessHeap(), 0, size);
    res = GetTokenInformation( token, TokenOwner, owner, size, &size );
    ok(res, "GetTokenInformation failed with error %d\n", GetLastError());
    AdminSid = owner->Owner;
    test_sid_str(AdminSid);

    res = GetTokenInformation( token, TokenPrimaryGroup, NULL, 0, &size );
    ok(!res, "Expected failure, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    group = HeapAlloc(GetProcessHeap(), 0, size);
    res = GetTokenInformation( token, TokenPrimaryGroup, group, size, &size );
    ok(res, "GetTokenInformation failed with error %d\n", GetLastError());
    UsersSid = group->PrimaryGroup;
    test_sid_str(UsersSid);

    acc_size = sizeof(account);
    dom_size = sizeof(domain);
    ret = LookupAccountSidA( NULL, UsersSid, account, &acc_size, domain, &dom_size, &use );
    ok(ret, "LookupAccountSid failed with %d\n", ret);
    ok(use == SidTypeGroup, "expect SidTypeGroup, got %d\n", use);
    ok(!strcmp(account, "None"), "expect None, got %s\n", account);

    res = GetTokenInformation( token, TokenUser, NULL, 0, &size );
    ok(!res, "Expected failure, got %d\n", res);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    user = HeapAlloc(GetProcessHeap(), 0, size);
    res = GetTokenInformation( token, TokenUser, user, size, &size );
    ok(res, "GetTokenInformation failed with error %d\n", GetLastError());
    UserSid = user->User.Sid;
    test_sid_str(UserSid);
    ok(EqualPrefixSid(UsersSid, UserSid), "TokenPrimaryGroup Sid and TokenUser Sid don't match.\n");

    CloseHandle( token );
    if (!res)
    {
        HeapFree(GetProcessHeap(), 0, group);
        HeapFree(GetProcessHeap(), 0, owner);
        HeapFree(GetProcessHeap(), 0, user);
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

    event = CreateEventA( NULL, TRUE, TRUE, "test_event" );
    ok(event != NULL, "CreateEvent %d\n", GetLastError());

    SecurityDescriptor->Revision = 0;
    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_UNKNOWN_REVISION );
    SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;

    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_INVALID_SECURITY_DESCR );
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_INVALID_SECURITY_DESCR );
    CHECK_SET_SECURITY( event, SACL_SECURITY_INFORMATION, ERROR_ACCESS_DENIED );
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );
    /* NULL DACL is valid and means that everyone has access */
    SecurityDescriptor->Control |= SE_DACL_PRESENT;
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );

    /* Set owner and group and dacl */
    res = SetSecurityDescriptorOwner(SecurityDescriptor, AdminSid, FALSE);
    ok(res, "SetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, OWNER_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_owner_equal( event, AdminSid, __LINE__ );

    res = SetSecurityDescriptorGroup(SecurityDescriptor, EveryoneSid, FALSE);
    ok(res, "SetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_group_equal( event, EveryoneSid, __LINE__ );

    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );
    /* setting a dacl should not change the owner or group */
    test_owner_equal( event, AdminSid, __LINE__ );
    test_group_equal( event, EveryoneSid, __LINE__ );

    /* Test again with a different SID in case the previous SID also happens to
     * be the one that is incorrectly replacing the group. */
    res = SetSecurityDescriptorGroup(SecurityDescriptor, UsersSid, FALSE);
    ok(res, "SetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, GROUP_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_group_equal( event, UsersSid, __LINE__ );

    res = SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);
    ok(res, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    CHECK_SET_SECURITY( event, DACL_SECURITY_INFORMATION, ERROR_SUCCESS );
    test_group_equal( event, UsersSid, __LINE__ );

    sprintf(buffer, "%s tests/security.c test", myARGV[0]);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    psa.nLength = sizeof(psa);
    psa.lpSecurityDescriptor = SecurityDescriptor;
    psa.bInheritHandle = TRUE;

    ThreadSecurityDescriptor = HeapAlloc( GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH );
    res = InitializeSecurityDescriptor( ThreadSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );
    ok(res, "InitializeSecurityDescriptor failed with error %d\n", GetLastError());

    ThreadAcl = HeapAlloc( GetProcessHeap(), 0, 256 );
    res = InitializeAcl( ThreadAcl, 256, ACL_REVISION );
    ok(res, "InitializeAcl failed with error %d\n", GetLastError());
    res = AddAccessDeniedAce( ThreadAcl, ACL_REVISION, THREAD_SET_THREAD_TOKEN, AdminSid );
    ok(res, "AddAccessDeniedAce failed with error %d\n", GetLastError() );
    res = AddAccessAllowedAce( ThreadAcl, ACL_REVISION, THREAD_ALL_ACCESS, AdminSid );
    ok(res, "AddAccessAllowedAce failed with error %d\n", GetLastError());

    res = SetSecurityDescriptorOwner( ThreadSecurityDescriptor, AdminSid, FALSE );
    ok(res, "SetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    res = SetSecurityDescriptorGroup( ThreadSecurityDescriptor, UsersSid, FALSE );
    ok(res, "SetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    res = SetSecurityDescriptorDacl( ThreadSecurityDescriptor, TRUE, ThreadAcl, FALSE );
    ok(res, "SetSecurityDescriptorDacl failed with error %d\n", GetLastError());

    tsa.nLength = sizeof(tsa);
    tsa.lpSecurityDescriptor = ThreadSecurityDescriptor;
    tsa.bInheritHandle = TRUE;

    /* Doesn't matter what ACL say we should get full access for ourselves */
    res = CreateProcessA( NULL, buffer, &psa, &tsa, FALSE, 0, NULL, NULL, &startup, &info );
    ok(res, "CreateProcess with err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS2( info.hProcess, PROCESS_ALL_ACCESS_NT4,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );
    TEST_GRANTED_ACCESS2( info.hThread, THREAD_ALL_ACCESS_NT4,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );
    winetest_wait_child_process( info.hProcess );

    FreeSid(EveryoneSid);
    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( event );
    HeapFree(GetProcessHeap(), 0, group);
    HeapFree(GetProcessHeap(), 0, owner);
    HeapFree(GetProcessHeap(), 0, user);
    HeapFree(GetProcessHeap(), 0, Acl);
    HeapFree(GetProcessHeap(), 0, SecurityDescriptor);
    HeapFree(GetProcessHeap(), 0, ThreadAcl);
    HeapFree(GetProcessHeap(), 0, ThreadSecurityDescriptor);
}

static void test_process_security_child(void)
{
    HANDLE handle, handle1;
    BOOL ret;
    DWORD err;

    handle = OpenProcess( PROCESS_TERMINATE, FALSE, GetCurrentProcessId() );
    ok(handle != NULL, "OpenProcess(PROCESS_TERMINATE) with err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS( handle, PROCESS_TERMINATE );

    ret = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                           &handle1, 0, TRUE, DUPLICATE_SAME_ACCESS );
    ok(ret, "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS( handle1, PROCESS_TERMINATE );

    CloseHandle( handle1 );

    SetLastError( 0xdeadbeef );
    ret = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                           &handle1, PROCESS_ALL_ACCESS, TRUE, 0 );
    err = GetLastError();
    ok(!ret && err == ERROR_ACCESS_DENIED, "duplicating handle should have failed "
       "with STATUS_ACCESS_DENIED, instead of err:%d\n", err);

    CloseHandle( handle );

    /* These two should fail - they are denied by ACL */
    handle = OpenProcess( PROCESS_VM_READ, FALSE, GetCurrentProcessId() );
    ok(handle == NULL, "OpenProcess(PROCESS_VM_READ) should have failed\n");
    handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
    ok(handle == NULL, "OpenProcess(PROCESS_ALL_ACCESS) should have failed\n");

    /* Documented privilege elevation */
    ret = DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                           &handle, 0, TRUE, DUPLICATE_SAME_ACCESS );
    ok(ret, "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS2( handle, PROCESS_ALL_ACCESS_NT4,
                          STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL );

    CloseHandle( handle );

    /* Same only explicitly asking for all access rights */
    ret = DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                           &handle, PROCESS_ALL_ACCESS, TRUE, 0 );
    ok(ret, "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS2( handle, PROCESS_ALL_ACCESS_NT4,
                          PROCESS_ALL_ACCESS | PROCESS_QUERY_LIMITED_INFORMATION );
    ret = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(),
                           &handle1, PROCESS_VM_READ, TRUE, 0 );
    ok(ret, "duplicating handle err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS( handle1, PROCESS_VM_READ );
    CloseHandle( handle1 );
    CloseHandle( handle );

    /* Test thread security */
    handle = OpenThread( THREAD_TERMINATE, FALSE, GetCurrentThreadId() );
    ok(handle != NULL, "OpenThread(THREAD_TERMINATE) with err:%d\n", GetLastError());
    TEST_GRANTED_ACCESS( handle, PROCESS_TERMINATE );
    CloseHandle( handle );

    handle = OpenThread( THREAD_SET_THREAD_TOKEN, FALSE, GetCurrentThreadId() );
    ok(handle == NULL, "OpenThread(THREAD_SET_THREAD_TOKEN) should have failed\n");
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

    pDuplicateTokenEx = (void *)GetProcAddress(hmod, "DuplicateTokenEx");
    if( !pDuplicateTokenEx ) {
        win_skip("DuplicateTokenEx is not available\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = ImpersonateSelf(SecurityAnonymous);
    if(!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("ImpersonateSelf is not implemented\n");
        return;
    }
    ok(ret, "ImpersonateSelf(SecurityAnonymous) failed with error %d\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(!ret, "OpenThreadToken should have failed\n");
    error = GetLastError();
    ok(error == ERROR_CANT_OPEN_ANONYMOUS, "OpenThreadToken on anonymous token should have returned ERROR_CANT_OPEN_ANONYMOUS instead of %d\n", error);
    /* can't perform access check when opening object against an anonymous impersonation token */
    todo_wine {
    error = RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    ok(error == ERROR_INVALID_HANDLE || error == ERROR_CANT_OPEN_ANONYMOUS || error == ERROR_BAD_IMPERSONATION_LEVEL,
       "RegOpenKeyEx failed with %d\n", error);
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
    User = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenUser, User, Size, &Size);
    ok(ret, "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, User);

    /* PrivilegeCheck fails with SecurityAnonymous level */
    ret = GetTokenInformation(Token, TokenPrivileges, NULL, 0, &Size);
    error = GetLastError();
    ok(!ret && error == ERROR_INSUFFICIENT_BUFFER, "GetTokenInformation(TokenPrivileges) should have failed with ERROR_INSUFFICIENT_BUFFER instead of %d\n", error);
    Privileges = HeapAlloc(GetProcessHeap(), 0, Size);
    ret = GetTokenInformation(Token, TokenPrivileges, Privileges, Size, &Size);
    ok(ret, "GetTokenInformation(TokenPrivileges) failed with error %d\n", GetLastError());

    PrivilegeSet = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(PRIVILEGE_SET, Privilege[Privileges->PrivilegeCount]));
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
    error = RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    todo_wine {
    ok(error == ERROR_INVALID_HANDLE || error == ERROR_BAD_IMPERSONATION_LEVEL,
       "RegOpenKeyEx should have failed with ERROR_INVALID_HANDLE or ERROR_BAD_IMPERSONATION_LEVEL instead of %d\n", error);
    }
    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    ok(ret, "PrivilegeCheck for SecurityIdentification failed with error %d\n", GetLastError());
    CloseHandle(Token);
    RevertToSelf();

    ret = ImpersonateSelf(SecurityImpersonation);
    ok(ret, "ImpersonateSelf(SecurityImpersonation) failed with error %d\n", GetLastError());
    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, TRUE, &Token);
    ok(ret, "OpenThreadToken failed with error %d\n", GetLastError());
    error = RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hkey);
    ok(error == ERROR_SUCCESS, "RegOpenKeyEx should have succeeded instead of failing with %d\n", error);
    RegCloseKey(hkey);
    ret = PrivilegeCheck(Token, PrivilegeSet, &AccessGranted);
    ok(ret, "PrivilegeCheck for SecurityImpersonation failed with error %d\n", GetLastError());
    RevertToSelf();

    CloseHandle(Token);
    CloseHandle(ProcessToken);

    HeapFree(GetProcessHeap(), 0, PrivilegeSet);
}

static void test_SetEntriesInAclW(void)
{
    DWORD res;
    PSID EveryoneSid = NULL, UsersSid = NULL;
    PACL OldAcl = NULL, NewAcl;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    EXPLICIT_ACCESSW ExplicitAccess;
    static const WCHAR wszEveryone[] = {'E','v','e','r','y','o','n','e',0};
    static const WCHAR wszCurrentUser[] = { 'C','U','R','R','E','N','T','_','U','S','E','R','\0'};

    if (!pSetEntriesInAclW)
    {
        win_skip("SetEntriesInAclW is not available\n");
        return;
    }

    NewAcl = (PACL)0xdeadbeef;
    res = pSetEntriesInAclW(0, NULL, NULL, &NewAcl);
    if(res == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetEntriesInAclW is not implemented\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl == NULL ||
        broken(NewAcl != NULL), /* NT4 */
        "NewAcl=%p, expected NULL\n", NewAcl);
    LocalFree(NewAcl);

    OldAcl = HeapAlloc(GetProcessHeap(), 0, 256);
    res = InitializeAcl(OldAcl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ACLs not implemented - skipping tests\n");
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
    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = EveryoneSid;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = 0xDEADBEEF;
    ExplicitAccess.Trustee.pMultipleTrustee = (PVOID)0xDEADBEEF;
    res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.ptstrName = (LPWSTR)wszEveryone;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_BAD_FORM;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER ||
            broken(res == ERROR_NOT_SUPPORTED), /* NT4 */
            "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl == NULL ||
            broken(NewAcl != NULL), /* NT4 */
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER ||
            broken(res == ERROR_NOT_SUPPORTED), /* NT4 */
            "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl == NULL ||
            broken(NewAcl != NULL), /* NT4 */
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ExplicitAccess.grfAccessMode = SET_ACCESS;
        res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);
    }

    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ExplicitAccess.Trustee.ptstrName = (LPWSTR)wszCurrentUser;
    res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.grfAccessMode = REVOKE_ACCESS;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = UsersSid;
    res = pSetEntriesInAclW(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclW failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    FreeSid(UsersSid);
    FreeSid(EveryoneSid);
    HeapFree(GetProcessHeap(), 0, OldAcl);
}

static void test_SetEntriesInAclA(void)
{
    DWORD res;
    PSID EveryoneSid = NULL, UsersSid = NULL;
    PACL OldAcl = NULL, NewAcl;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
    EXPLICIT_ACCESSA ExplicitAccess;
    static const CHAR szEveryone[] = {'E','v','e','r','y','o','n','e',0};
    static const CHAR szCurrentUser[] = { 'C','U','R','R','E','N','T','_','U','S','E','R','\0'};

    if (!pSetEntriesInAclA)
    {
        win_skip("SetEntriesInAclA is not available\n");
        return;
    }

    NewAcl = (PACL)0xdeadbeef;
    res = pSetEntriesInAclA(0, NULL, NULL, &NewAcl);
    if(res == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetEntriesInAclA is not implemented\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %u\n", res);
    ok(NewAcl == NULL ||
        broken(NewAcl != NULL), /* NT4 */
        "NewAcl=%p, expected NULL\n", NewAcl);
    LocalFree(NewAcl);

    OldAcl = HeapAlloc(GetProcessHeap(), 0, 256);
    res = InitializeAcl(OldAcl, 256, ACL_REVISION);
    if(!res && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ACLs not implemented - skipping tests\n");
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
    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = EveryoneSid;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    ExplicitAccess.Trustee.pMultipleTrustee = NULL;
    ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    if (PRIMARYLANGID(GetSystemDefaultLangID()) != LANG_ENGLISH)
    {
        skip("Non-English locale (test with hardcoded 'Everyone')\n");
    }
    else
    {
        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.ptstrName = (LPSTR)szEveryone;
        res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %u\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_BAD_FORM;
        res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER ||
            broken(res == ERROR_NOT_SUPPORTED), /* NT4 */
            "SetEntriesInAclA failed: %u\n", res);
        ok(NewAcl == NULL ||
            broken(NewAcl != NULL), /* NT4 */
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ExplicitAccess.Trustee.MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
        res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_INVALID_PARAMETER ||
            broken(res == ERROR_NOT_SUPPORTED), /* NT4 */
            "SetEntriesInAclA failed: %u\n", res);
        ok(NewAcl == NULL ||
            broken(NewAcl != NULL), /* NT4 */
            "returned acl wasn't NULL: %p\n", NewAcl);

        ExplicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ExplicitAccess.grfAccessMode = SET_ACCESS;
        res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
        ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %u\n", res);
        ok(NewAcl != NULL, "returned acl was NULL\n");
        LocalFree(NewAcl);
    }

    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ExplicitAccess.Trustee.ptstrName = (LPSTR)szCurrentUser;
    res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    ExplicitAccess.grfAccessMode = REVOKE_ACCESS;
    ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess.Trustee.ptstrName = UsersSid;
    res = pSetEntriesInAclA(1, &ExplicitAccess, OldAcl, &NewAcl);
    ok(res == ERROR_SUCCESS, "SetEntriesInAclA failed: %u\n", res);
    ok(NewAcl != NULL, "returned acl was NULL\n");
    LocalFree(NewAcl);

    FreeSid(UsersSid);
    FreeSid(EveryoneSid);
    HeapFree(GetProcessHeap(), 0, OldAcl);
}

/* helper function for test_CreateDirectoryA */
static void get_nt_pathW(const char *name, UNICODE_STRING *nameW)
{
    UNICODE_STRING strW;
    ANSI_STRING str;
    NTSTATUS status;
    BOOLEAN ret;
    RtlInitAnsiString(&str, name);

    status = pRtlAnsiStringToUnicodeString(&strW, &str, TRUE);
    ok(!status, "RtlAnsiStringToUnicodeString failed with %08x\n", status);

    ret = pRtlDosPathNameToNtPathName_U(strW.Buffer, nameW, NULL, NULL);
    ok(ret, "RtlDosPathNameToNtPathName_U failed\n");

    RtlFreeUnicodeString(&strW);
}

static void test_inherited_dacl(PACL dacl, PSID admin_sid, PSID user_sid, DWORD flags, DWORD mask,
                                BOOL todo_count, BOOL todo_sid, BOOL todo_flags, int line)
{
    ACL_SIZE_INFORMATION acl_size;
    ACCESS_ALLOWED_ACE *ace;
    BOOL bret;

    bret = pGetAclInformation(dacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok_(__FILE__, line)(bret, "GetAclInformation failed\n");

    if (todo_count)
        todo_wine
        ok_(__FILE__, line)(acl_size.AceCount == 2,
            "GetAclInformation returned unexpected entry count (%d != 2)\n",
            acl_size.AceCount);
    else
        ok_(__FILE__, line)(acl_size.AceCount == 2,
            "GetAclInformation returned unexpected entry count (%d != 2)\n",
            acl_size.AceCount);

    if (acl_size.AceCount > 0)
    {
        bret = pGetAce(dacl, 0, (VOID **)&ace);
        ok_(__FILE__, line)(bret, "Failed to get Current User ACE\n");

        bret = EqualSid(&ace->SidStart, user_sid);
        if (todo_sid)
            todo_wine
            ok_(__FILE__, line)(bret, "Current User ACE != Current User SID\n");
        else
            ok_(__FILE__, line)(bret, "Current User ACE != Current User SID\n");

        if (todo_flags)
            todo_wine
            ok_(__FILE__, line)(((ACE_HEADER *)ace)->AceFlags == flags,
                "Current User ACE has unexpected flags (0x%x != 0x%x)\n",
                ((ACE_HEADER *)ace)->AceFlags, flags);
        else
            ok_(__FILE__, line)(((ACE_HEADER *)ace)->AceFlags == flags,
                "Current User ACE has unexpected flags (0x%x != 0x%x)\n",
                ((ACE_HEADER *)ace)->AceFlags, flags);

        ok_(__FILE__, line)(ace->Mask == mask,
            "Current User ACE has unexpected mask (0x%x != 0x%x)\n",
            ace->Mask, mask);
    }
    if (acl_size.AceCount > 1)
    {
        bret = pGetAce(dacl, 1, (VOID **)&ace);
        ok_(__FILE__, line)(bret, "Failed to get Administators Group ACE\n");

        bret = EqualSid(&ace->SidStart, admin_sid);
        if (todo_sid)
            todo_wine
            ok_(__FILE__, line)(bret, "Administators Group ACE != Administators Group SID\n");
        else
            ok_(__FILE__, line)(bret, "Administators Group ACE != Administators Group SID\n");

        if (todo_flags)
            todo_wine
            ok_(__FILE__, line)(((ACE_HEADER *)ace)->AceFlags == flags,
                "Administators Group ACE has unexpected flags (0x%x != 0x%x)\n",
                ((ACE_HEADER *)ace)->AceFlags, flags);
        else
            ok_(__FILE__, line)(((ACE_HEADER *)ace)->AceFlags == flags,
                "Administators Group ACE has unexpected flags (0x%x != 0x%x)\n",
                ((ACE_HEADER *)ace)->AceFlags, flags);

        ok_(__FILE__, line)(ace->Mask == mask,
            "Administators Group ACE has unexpected mask (0x%x != 0x%x)\n",
            ace->Mask, mask);
    }
}

static void test_CreateDirectoryA(void)
{
    char admin_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES], *user;
    DWORD sid_size = sizeof(admin_ptr), user_size;
    PSID admin_sid = (PSID) admin_ptr, user_sid;
    char sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = &sd;
    ACL_SIZE_INFORMATION acl_size;
    UNICODE_STRING tmpfileW;
    SECURITY_ATTRIBUTES sa;
    OBJECT_ATTRIBUTES attr;
    char tmpfile[MAX_PATH];
    char tmpdir[MAX_PATH];
    HANDLE token, hTemp;
    IO_STATUS_BLOCK io;
    struct _SID *owner;
    BOOL bret = TRUE;
    NTSTATUS status;
    DWORD error;
    PACL pDacl;

    if (!pGetSecurityInfo || !pGetNamedSecurityInfoA || !pCreateWellKnownSid)
    {
        win_skip("Required functions are not available\n");
        return;
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) bret = FALSE;
        else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) bret = FALSE;
    }
    if (!bret)
    {
        win_skip("Failed to get current user token\n");
        return;
    }
    bret = GetTokenInformation(token, TokenUser, NULL, 0, &user_size);
    ok(!bret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());
    user = HeapAlloc(GetProcessHeap(), 0, user_size);
    bret = GetTokenInformation(token, TokenUser, user, user_size, &user_size);
    ok(bret, "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());
    CloseHandle( token );
    user_sid = ((TOKEN_USER *)user)->User.Sid;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pCreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &sid_size);
    pDacl = HeapAlloc(GetProcessHeap(), 0, 100);
    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE,
                                  GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE,
                                  GENERIC_ALL, admin_sid);
    ok(bret, "Failed to add Administrator Group to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor.\n");

    GetTempPathA(MAX_PATH, tmpdir);
    lstrcatA(tmpdir, "Please Remove Me");
    bret = CreateDirectoryA(tmpdir, &sa);
    ok(bret == TRUE, "CreateDirectoryA(%s) failed err=%d\n", tmpdir, GetLastError());
    HeapFree(GetProcessHeap(), 0, pDacl);

    SetLastError(0xdeadbeef);
    error = pGetNamedSecurityInfoA(tmpdir, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSID*)&owner,
                                   NULL, &pDacl, NULL, &pSD);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("GetNamedSecurityInfoA is not implemented\n");
        goto done;
    }
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);
    test_inherited_dacl(pDacl, admin_sid, user_sid, OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE,
                        0x1f01ff, FALSE, FALSE, FALSE, __LINE__);
    LocalFree(pSD);

    /* Test inheritance of ACLs in CreateFile without security descriptor */
    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");

    hTemp = CreateFileA(tmpfile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    ok(hTemp != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError());

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "Failed to get permissions on file\n");
    test_inherited_dacl(pDacl, admin_sid, user_sid, INHERITED_ACE,
                        0x1f01ff, FALSE, FALSE, FALSE, __LINE__);
    LocalFree(pSD);
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in CreateFile with security descriptor -
     * When a security descriptor is set, then inheritance doesn't take effect */
    pSD = &sd;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = HeapAlloc(GetProcessHeap(), 0, sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor\n");

    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    hTemp = CreateFileA(tmpfile, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                        CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    ok(hTemp != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pDacl);

    error = pGetSecurityInfo(hTemp, SE_FILE_OBJECT,
                             OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                             (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %d\n", error);
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%d != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %d\n", error);
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%d != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in NtCreateFile without security descriptor */
    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");
    get_nt_pathW(tmpfile, &tmpfileW);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &tmpfileW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = pNtCreateFile(&hTemp, GENERIC_WRITE | DELETE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ, FILE_CREATE, FILE_DELETE_ON_CLOSE, NULL, 0);
    ok(!status, "NtCreateFile failed with %08x\n", status);
    RtlFreeUnicodeString(&tmpfileW);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "Failed to get permissions on file\n");
    test_inherited_dacl(pDacl, admin_sid, user_sid, INHERITED_ACE,
                        0x1f01ff, FALSE, FALSE, FALSE, __LINE__);
    LocalFree(pSD);
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in NtCreateFile with security descriptor -
     * When a security descriptor is set, then inheritance doesn't take effect */
    pSD = &sd;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = HeapAlloc(GetProcessHeap(), 0, sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor\n");

    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpfile");
    get_nt_pathW(tmpfile, &tmpfileW);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &tmpfileW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = pSD;
    attr.SecurityQualityOfService = NULL;

    status = pNtCreateFile(&hTemp, GENERIC_WRITE | DELETE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ, FILE_CREATE, FILE_DELETE_ON_CLOSE, NULL, 0);
    ok(!status, "NtCreateFile failed with %08x\n", status);
    RtlFreeUnicodeString(&tmpfileW);
    HeapFree(GetProcessHeap(), 0, pDacl);

    error = pGetSecurityInfo(hTemp, SE_FILE_OBJECT,
                             OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                             (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %d\n", error);
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    todo_wine
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%d != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %d\n", error);
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    todo_wine
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%d != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in CreateDirectory without security descriptor */
    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpdir");
    bret = CreateDirectoryA(tmpfile, NULL);
    ok(bret == TRUE, "CreateDirectoryA failed with error %u\n", GetLastError());

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "Failed to get permissions on file\n");
    test_inherited_dacl(pDacl, admin_sid, user_sid,
                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERITED_ACE,
                        0x1f01ff, FALSE, FALSE, FALSE, __LINE__);
    LocalFree(pSD);
    bret = RemoveDirectoryA(tmpfile);
    ok(bret == TRUE, "RemoveDirectoryA failed with error %u\n", GetLastError());

    /* Test inheritance of ACLs in CreateDirectory with security descriptor */
    pSD = &sd;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = HeapAlloc(GetProcessHeap(), 0, sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor\n");

    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpdir1");

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    bret = CreateDirectoryA(tmpfile, &sa);
    ok(bret == TRUE, "CreateDirectoryA failed with error %u\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pDacl);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %d\n", error);
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%d != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);

    SetLastError(0xdeadbeef);
    bret = RemoveDirectoryA(tmpfile);
    error = GetLastError();
    ok(bret == FALSE, "RemoveDirectoryA unexpected succeeded\n");
    ok(error == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %u\n", error);

    pSD = &sd;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = HeapAlloc(GetProcessHeap(), 0, 100);
    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor.\n");
    error = pSetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                                   NULL, pDacl, NULL);
    ok(error == ERROR_SUCCESS, "SetNamedSecurityInfoA failed with error %u\n", error);
    HeapFree(GetProcessHeap(), 0, pDacl);

    bret = RemoveDirectoryA(tmpfile);
    ok(bret == TRUE, "RemoveDirectoryA failed with error %u\n", GetLastError());

    /* Test inheritance of ACLs in NtCreateFile(..., FILE_DIRECTORY_FILE, ...) without security descriptor */
    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpdir");
    get_nt_pathW(tmpfile, &tmpfileW);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &tmpfileW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = pNtCreateFile(&hTemp, GENERIC_READ | DELETE, &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ, FILE_CREATE, FILE_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE, NULL, 0);
    ok(!status, "NtCreateFile failed with %08x\n", status);
    RtlFreeUnicodeString(&tmpfileW);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "Failed to get permissions on file\n");
    test_inherited_dacl(pDacl, admin_sid, user_sid,
                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERITED_ACE,
                        0x1f01ff, FALSE, FALSE, FALSE, __LINE__);
    LocalFree(pSD);
    CloseHandle(hTemp);

    /* Test inheritance of ACLs in NtCreateFile(..., FILE_DIRECTORY_FILE, ...) with security descriptor */
    pSD = &sd;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = HeapAlloc(GetProcessHeap(), 0, sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor\n");

    strcpy(tmpfile, tmpdir);
    lstrcatA(tmpfile, "/tmpdir2");
    get_nt_pathW(tmpfile, &tmpfileW);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &tmpfileW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = pSD;
    attr.SecurityQualityOfService = NULL;

    status = pNtCreateFile(&hTemp, GENERIC_READ | DELETE, &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ, FILE_CREATE, FILE_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE, NULL, 0);
    ok(!status, "NtCreateFile failed with %08x\n", status);
    RtlFreeUnicodeString(&tmpfileW);
    HeapFree(GetProcessHeap(), 0, pDacl);

    error = pGetSecurityInfo(hTemp, SE_FILE_OBJECT,
                             OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                             (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %d\n", error);
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    todo_wine
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%d != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
                                   OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                   (PSID *)&owner, NULL, &pDacl, NULL, &pSD);
    ok(error == ERROR_SUCCESS, "GetNamedSecurityInfo failed with error %d\n", error);
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    todo_wine
    ok(acl_size.AceCount == 0, "GetAclInformation returned unexpected entry count (%d != 0).\n",
                               acl_size.AceCount);
    LocalFree(pSD);
    CloseHandle(hTemp);

done:
    HeapFree(GetProcessHeap(), 0, user);
    bret = RemoveDirectoryA(tmpdir);
    ok(bret == TRUE, "RemoveDirectoryA should always succeed\n");
}

static void test_GetNamedSecurityInfoA(void)
{
    char admin_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES], *user;
    char system_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES];
    char users_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES];
    PSID admin_sid = (PSID) admin_ptr, users_sid = (PSID) users_ptr;
    PSID system_sid = (PSID) system_ptr, user_sid;
    DWORD sid_size = sizeof(admin_ptr), user_size;
    char invalid_path[] = "/an invalid file path";
    int users_ace_id = -1, admins_ace_id = -1, i;
    char software_key[] = "MACHINE\\Software";
    char sd[SECURITY_DESCRIPTOR_MIN_LENGTH+sizeof(void*)];
    SECURITY_DESCRIPTOR_CONTROL control;
    ACL_SIZE_INFORMATION acl_size;
    CHAR windows_dir[MAX_PATH];
    PSECURITY_DESCRIPTOR pSD;
    ACCESS_ALLOWED_ACE *ace;
    BOOL bret = TRUE, isNT4;
    char tmpfile[MAX_PATH];
    DWORD error, revision;
    BOOL owner_defaulted;
    BOOL group_defaulted;
    BOOL dacl_defaulted;
    HANDLE token, hTemp, h;
    PSID owner, group;
    BOOL dacl_present;
    PACL pDacl;
    BYTE flags;
    NTSTATUS status;

    if (!pSetNamedSecurityInfoA || !pGetNamedSecurityInfoA || !pCreateWellKnownSid)
    {
        win_skip("Required functions are not available\n");
        return;
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) bret = FALSE;
        else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) bret = FALSE;
    }
    if (!bret)
    {
        win_skip("Failed to get current user token\n");
        return;
    }
    bret = GetTokenInformation(token, TokenUser, NULL, 0, &user_size);
    ok(!bret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
        "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());
    user = HeapAlloc(GetProcessHeap(), 0, user_size);
    bret = GetTokenInformation(token, TokenUser, user, user_size, &user_size);
    ok(bret, "GetTokenInformation(TokenUser) failed with error %d\n", GetLastError());
    CloseHandle( token );
    user_sid = ((TOKEN_USER *)user)->User.Sid;

    bret = GetWindowsDirectoryA(windows_dir, MAX_PATH);
    ok(bret, "GetWindowsDirectory failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    error = pGetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,
        NULL, NULL, NULL, NULL, &pSD);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("GetNamedSecurityInfoA is not implemented\n");
        HeapFree(GetProcessHeap(), 0, user);
        return;
    }
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);

    bret = GetSecurityDescriptorControl(pSD, &control, &revision);
    ok(bret, "GetSecurityDescriptorControl failed with error %d\n", GetLastError());
    ok((control & (SE_SELF_RELATIVE|SE_DACL_PRESENT)) == (SE_SELF_RELATIVE|SE_DACL_PRESENT) ||
        broken((control & (SE_SELF_RELATIVE|SE_DACL_PRESENT)) == SE_DACL_PRESENT), /* NT4 */
        "control (0x%x) doesn't have (SE_SELF_RELATIVE|SE_DACL_PRESENT) flags set\n", control);
    ok(revision == SECURITY_DESCRIPTOR_REVISION1, "revision was %d instead of 1\n", revision);

    isNT4 = (control & (SE_SELF_RELATIVE|SE_DACL_PRESENT)) == SE_DACL_PRESENT;

    bret = GetSecurityDescriptorOwner(pSD, &owner, &owner_defaulted);
    ok(bret, "GetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    ok(owner != NULL, "owner should not be NULL\n");

    bret = GetSecurityDescriptorGroup(pSD, &group, &group_defaulted);
    ok(bret, "GetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    ok(group != NULL, "group should not be NULL\n");
    LocalFree(pSD);


    /* NULL descriptor tests */
    if(isNT4)
    {
        win_skip("NT4 does not support GetNamedSecutityInfo with a NULL descriptor\n");
        HeapFree(GetProcessHeap(), 0, user);
        return;
    }

    error = pGetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,
        NULL, NULL, NULL, NULL, NULL);
    ok(error==ERROR_INVALID_PARAMETER, "GetNamedSecurityInfo failed with error %d\n", error);

    pDacl = NULL;
    error = pGetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,
        NULL, NULL, &pDacl, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);
    ok(pDacl != NULL, "DACL should not be NULL\n");
    LocalFree(pSD);

    error = pGetNamedSecurityInfoA(windows_dir, SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,
        NULL, NULL, &pDacl, NULL, NULL);
    ok(error==ERROR_INVALID_PARAMETER, "GetNamedSecurityInfo failed with error %d\n", error);

    /* Test behavior of SetNamedSecurityInfo with an invalid path */
    SetLastError(0xdeadbeef);
    error = pSetNamedSecurityInfoA(invalid_path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                                   NULL, NULL, NULL);
    ok(error == ERROR_FILE_NOT_FOUND, "Unexpected error returned: 0x%x\n", error);
    ok(GetLastError() == 0xdeadbeef, "Expected last error to remain unchanged.\n");

    /* Create security descriptor information and test that it comes back the same */
    pSD = &sd;
    pDacl = HeapAlloc(GetProcessHeap(), 0, 100);
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pCreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &sid_size);
    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, admin_sid);
    ok(bret, "Failed to add Administrator Group to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor.\n");
    GetTempFileNameA(".", "foo", 0, tmpfile);
    hTemp = CreateFileA(tmpfile, WRITE_DAC|GENERIC_WRITE, FILE_SHARE_DELETE|FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    SetLastError(0xdeadbeef);
    error = pSetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                                   NULL, pDacl, NULL);
    HeapFree(GetProcessHeap(), 0, pDacl);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("SetNamedSecurityInfoA is not implemented\n");
        HeapFree(GetProcessHeap(), 0, user);
        CloseHandle(hTemp);
        return;
    }
    ok(!error, "SetNamedSecurityInfoA failed with error %d\n", error);
    SetLastError(0xdeadbeef);
    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                   NULL, NULL, &pDacl, NULL, &pSD);
    if (error != ERROR_SUCCESS && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("GetNamedSecurityInfoA is not implemented\n");
        HeapFree(GetProcessHeap(), 0, user);
        CloseHandle(hTemp);
        return;
    }
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);

    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    if (acl_size.AceCount > 0)
    {
        bret = pGetAce(pDacl, 0, (VOID **)&ace);
        ok(bret, "Failed to get Current User ACE.\n");
        bret = EqualSid(&ace->SidStart, user_sid);
        ok(bret, "Current User ACE != Current User SID.\n");
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Current User ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff,
           "Current User ACE has unexpected mask (0x%x != 0x1f01ff)\n", ace->Mask);
    }
    if (acl_size.AceCount > 1)
    {
        bret = pGetAce(pDacl, 1, (VOID **)&ace);
        ok(bret, "Failed to get Administators Group ACE.\n");
        bret = EqualSid(&ace->SidStart, admin_sid);
        ok(bret || broken(!bret) /* win2k */, "Administators Group ACE != Administators Group SID.\n");
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Administators Group ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff || broken(ace->Mask == GENERIC_ALL) /* win2k */,
           "Administators Group ACE has unexpected mask (0x%x != 0x1f01ff)\n", ace->Mask);
    }
    LocalFree(pSD);

    /* show that setting empty DACL is not removing all file permissions */
    pDacl = HeapAlloc(GetProcessHeap(), 0, sizeof(ACL));
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    error =  pSetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
            NULL, NULL, pDacl, NULL);
    ok(!error, "SetNamedSecurityInfoA failed with error %d\n", error);
    HeapFree(GetProcessHeap(), 0, pDacl);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
            NULL, NULL, &pDacl, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);

    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    if (acl_size.AceCount > 0)
    {
        bret = pGetAce(pDacl, 0, (VOID **)&ace);
        ok(bret, "Failed to get ACE.\n");
        ok(((ACE_HEADER *)ace)->AceFlags & INHERITED_ACE,
           "ACE has unexpected flags: 0x%x\n", ((ACE_HEADER *)ace)->AceFlags);
    }
    LocalFree(pSD);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    CloseHandle(h);

    /* test setting NULL DACL */
    error = pSetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL);
    ok(!error, "SetNamedSecurityInfoA failed with error %d\n", error);

    error = pGetNamedSecurityInfoA(tmpfile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                NULL, NULL, &pDacl, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);
    todo_wine ok(!pDacl, "pDacl != NULL\n");
    LocalFree(pSD);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    CloseHandle(h);

    /* NtSetSecurityObject doesn't inherit DACL entries */
    pSD = sd+sizeof(void*)-((ULONG_PTR)sd)%sizeof(void*);
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pDacl = HeapAlloc(GetProcessHeap(), 0, 100);
    bret = InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor.\n");
    status = pNtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %x\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    CloseHandle(h);

    pSetSecurityDescriptorControl(pSD, SE_DACL_AUTO_INHERIT_REQ, SE_DACL_AUTO_INHERIT_REQ);
    status = pNtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %x\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    CloseHandle(h);

    pSetSecurityDescriptorControl(pSD, SE_DACL_AUTO_INHERIT_REQ|SE_DACL_AUTO_INHERITED,
            SE_DACL_AUTO_INHERIT_REQ|SE_DACL_AUTO_INHERITED);
    status = pNtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %x\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    CloseHandle(h);

    /* test if DACL is properly mapped to permission */
    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = pAddAccessDeniedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor.\n");
    status = pNtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %x\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    CloseHandle(h);

    bret = InitializeAcl(pDacl, 100, ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = pAddAccessDeniedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor.\n");
    status = pNtSetSecurityObject(hTemp, DACL_SECURITY_INFORMATION, pSD);
    ok(status == ERROR_SUCCESS, "NtSetSecurityObject returned %x\n", status);

    h = CreateFileA(tmpfile, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_WRITE|FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
    ok(h == INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, pDacl);
    HeapFree(GetProcessHeap(), 0, user);
    CloseHandle(hTemp);

    /* Test querying the ownership of a built-in registry key */
    sid_size = sizeof(system_ptr);
    pCreateWellKnownSid(WinLocalSystemSid, NULL, system_sid, &sid_size);
    error = pGetNamedSecurityInfoA(software_key, SE_REGISTRY_KEY,
                                   OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION,
                                   NULL, NULL, NULL, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);

    bret = GetSecurityDescriptorOwner(pSD, &owner, &owner_defaulted);
    ok(bret, "GetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    ok(owner != NULL, "owner should not be NULL\n");
    ok(EqualSid(owner, admin_sid), "MACHINE\\Software owner SID != Administrators SID.\n");

    bret = GetSecurityDescriptorGroup(pSD, &group, &group_defaulted);
    ok(bret, "GetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    ok(group != NULL, "group should not be NULL\n");
    ok(EqualSid(group, admin_sid) || broken(EqualSid(group, system_sid)) /* before Win7 */
       || broken(((SID*)group)->SubAuthority[0] == SECURITY_NT_NON_UNIQUE) /* Vista */,
       "MACHINE\\Software group SID != Local System SID.\n");
    LocalFree(pSD);

    /* Test querying the DACL of a built-in registry key */
    sid_size = sizeof(users_ptr);
    pCreateWellKnownSid(WinBuiltinUsersSid, NULL, users_sid, &sid_size);
    error = pGetNamedSecurityInfoA(software_key, SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION,
                                   NULL, NULL, NULL, NULL, &pSD);
    ok(!error, "GetNamedSecurityInfo failed with error %d\n", error);

    bret = GetSecurityDescriptorDacl(pSD, &dacl_present, &pDacl, &dacl_defaulted);
    ok(bret, "GetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    ok(dacl_present, "DACL should be present\n");
    ok(pDacl && IsValidAcl(pDacl), "GetSecurityDescriptorDacl returned invalid DACL.\n");
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    ok(acl_size.AceCount != 0, "GetAclInformation returned no ACLs\n");
    for (i=0; i<acl_size.AceCount; i++)
    {
        bret = pGetAce(pDacl, i, (VOID **)&ace);
        ok(bret, "Failed to get ACE %d.\n", i);
        bret = EqualSid(&ace->SidStart, users_sid);
        if (bret) users_ace_id = i;
        bret = EqualSid(&ace->SidStart, admin_sid);
        if (bret) admins_ace_id = i;
    }
    ok(users_ace_id != -1 || broken(users_ace_id == -1) /* win2k */,
       "Bultin Users ACE not found.\n");
    if (users_ace_id != -1)
    {
        bret = pGetAce(pDacl, users_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Builtin Users ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE)
           || broken(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE|INHERITED_ACE)) /* w2k8 */,
           "Builtin Users ACE has unexpected flags (0x%x != 0x%x)\n", flags,
           INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE);
        ok(ace->Mask == GENERIC_READ, "Builtin Users ACE has unexpected mask (0x%x != 0x%x)\n",
                                      ace->Mask, GENERIC_READ);
    }
    ok(admins_ace_id != -1, "Bultin Admins ACE not found.\n");
    if (admins_ace_id != -1)
    {
        bret = pGetAce(pDacl, admins_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Builtin Admins ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == 0x0
           || broken(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE|INHERITED_ACE)) /* w2k8 */,
           "Builtin Admins ACE has unexpected flags (0x%x != 0x0)\n", flags);
        ok(ace->Mask == KEY_ALL_ACCESS || broken(ace->Mask == GENERIC_ALL) /* w2k8 */,
           "Builtin Admins ACE has unexpected mask (0x%x != 0x%x)\n", ace->Mask, KEY_ALL_ACCESS);
    }
    LocalFree(pSD);
}

static void test_ConvertStringSecurityDescriptor(void)
{
    BOOL ret;
    PSECURITY_DESCRIPTOR pSD;
    static const WCHAR Blank[] = { 0 };
    unsigned int i;
    static const struct
    {
        const char *sidstring;
        DWORD      revision;
        BOOL       ret;
        DWORD      GLE;
        DWORD      altGLE;
    } cssd[] =
    {
        { "D:(A;;GA;;;WD)",                  0xdeadbeef,      FALSE, ERROR_UNKNOWN_REVISION },
        /* test ACE string type */
        { "D:(A;;GA;;;WD)",                  SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;;WD)",                  SDDL_REVISION_1, TRUE },
        { "ERROR:(D;;GA;;;WD)",              SDDL_REVISION_1, FALSE, ERROR_INVALID_PARAMETER },
        /* test ACE string with spaces */
        { " D:(D;;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D: (D;;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:( D;;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D ;;GA;;;WD)",                SDDL_REVISION_1, FALSE, RPC_S_INVALID_STRING_UUID, ERROR_INVALID_ACL }, /* Vista+ */
        { "D:(D; ;GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;; GA;;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA ;;;WD)",                SDDL_REVISION_1, FALSE, ERROR_INVALID_ACL },
        { "D:(D;;GA; ;;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;; ;WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;; WD)",                SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;;WD )",                SDDL_REVISION_1, TRUE },
        /* test ACE string access rights */
        { "D:(A;;GA;;;WD)",                  SDDL_REVISION_1, TRUE },
        { "D:(A;;GRGWGX;;;WD)",              SDDL_REVISION_1, TRUE },
        { "D:(A;;RCSDWDWO;;;WD)",            SDDL_REVISION_1, TRUE },
        { "D:(A;;RPWPCCDCLCSWLODTCR;;;WD)",  SDDL_REVISION_1, TRUE },
        { "D:(A;;FAFRFWFX;;;WD)",            SDDL_REVISION_1, TRUE },
        { "D:(A;;KAKRKWKX;;;WD)",            SDDL_REVISION_1, TRUE },
        { "D:(A;;0xFFFFFFFF;;;WD)",          SDDL_REVISION_1, TRUE },
        { "S:(AU;;0xFFFFFFFF;;;WD)",         SDDL_REVISION_1, TRUE },
        /* test ACE string access right error case */
        { "D:(A;;ROB;;;WD)",                 SDDL_REVISION_1, FALSE, ERROR_INVALID_ACL },
        /* test behaviour with empty strings */
        { "",                                SDDL_REVISION_1, TRUE },
        /* test ACE string SID */
        { "D:(D;;GA;;;S-1-0-0)",             SDDL_REVISION_1, TRUE },
        { "D:(D;;GA;;;Nonexistent account)", SDDL_REVISION_1, FALSE, ERROR_INVALID_ACL, ERROR_INVALID_SID } /* W2K */
    };

    if (!pConvertStringSecurityDescriptorToSecurityDescriptorA)
    {
        win_skip("ConvertStringSecurityDescriptorToSecurityDescriptor is not available\n");
        return;
    }

    for (i = 0; i < sizeof(cssd)/sizeof(cssd[0]); i++)
    {
        DWORD GLE;

        SetLastError(0xdeadbeef);
        ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
            cssd[i].sidstring, cssd[i].revision, &pSD, NULL);
        GLE = GetLastError();
        ok(ret == cssd[i].ret, "(%02u) Expected %s (%d)\n", i, cssd[i].ret ? "success" : "failure", GLE);
        if (!cssd[i].ret)
            ok(GLE == cssd[i].GLE ||
               (cssd[i].altGLE && GLE == cssd[i].altGLE),
               "(%02u) Unexpected last error %d\n", i, GLE);
        if (ret)
            LocalFree(pSD);
    }

    /* test behaviour with NULL parameters */
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        NULL, 0xdeadbeef, &pSD, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %d\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorW(
        NULL, 0xdeadbeef, &pSD, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %d\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;ROB;;;WD)", 0xdeadbeef, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %d\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:(A;;ROB;;;WD)", SDDL_REVISION_1, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
        "ConvertStringSecurityDescriptorToSecurityDescriptor should have failed with ERROR_INVALID_PARAMETER instead of %d\n",
        GetLastError());

    /* test behaviour with empty strings */
    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorW(
        Blank, SDDL_REVISION_1, &pSD, NULL);
    ok(ret, "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %d\n", GetLastError());
    LocalFree(pSD);

    SetLastError(0xdeadbeef);
    ret = pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "D:P(A;;GRGW;;;BA)(A;;GRGW;;;S-1-5-21-0-0-0-1000)S:(ML;;NWNR;;;S-1-16-12288)", SDDL_REVISION_1, &pSD, NULL);
    ok(ret || broken(!ret && GetLastError() == ERROR_INVALID_DATATYPE) /* win2k */,
       "ConvertStringSecurityDescriptorToSecurityDescriptor failed with error %u\n", GetLastError());
    if (ret) LocalFree(pSD);
}

static void test_ConvertSecurityDescriptorToString(void)
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
        win_skip("ConvertSecurityDescriptorToStringSecurityDescriptor is not available\n");
        return;
    }
    if (!pCreateWellKnownSid)
    {
        win_skip("CreateWellKnownSid is not available\n");
        return;
    }

/* It seems Windows XP adds an extra character to the length of the string for each ACE in an ACL. We
 * don't replicate this feature so we only test len >= strlen+1. */
#define CHECK_RESULT_AND_FREE(exp_str) \
    ok(strcmp(string, (exp_str)) == 0, "String mismatch (expected \"%s\", got \"%s\")\n", (exp_str), string); \
    ok(len >= (strlen(exp_str) + 1), "Length mismatch (expected %d, got %d)\n", lstrlenA(exp_str) + 1, len); \
    LocalFree(string);

#define CHECK_ONE_OF_AND_FREE(exp_str1, exp_str2) \
    ok(strcmp(string, (exp_str1)) == 0 || strcmp(string, (exp_str2)) == 0, "String mismatch (expected\n\"%s\" or\n\"%s\", got\n\"%s\")\n", (exp_str1), (exp_str2), string); \
    ok(len >= (strlen(exp_str1) + 1) || len >= (strlen(exp_str2) + 1), "Length mismatch (expected %d or %d, got %d)\n", lstrlenA(exp_str1) + 1, lstrlenA(exp_str2) + 1, len); \
    LocalFree(string);

    InitializeSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("");

    size = 4096;
    pCreateWellKnownSid(WinLocalSid, NULL, sid_buf, &size);
    SetSecurityDescriptorOwner(&desc, sid_buf, FALSE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:S-1-2-0");

    SetSecurityDescriptorOwner(&desc, sid_buf, TRUE);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(&desc, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("O:S-1-2-0");

    size = sizeof(sid_buf);
    pCreateWellKnownSid(WinLocalSystemSid, NULL, sid_buf, &size);
    SetSecurityDescriptorOwner(&desc, sid_buf, TRUE);
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

    LocalFree(psid2);
    LocalFree(psid);
}

static void test_SetSecurityDescriptorControl (PSECURITY_DESCRIPTOR sec)
{
    SECURITY_DESCRIPTOR_CONTROL ref;
    SECURITY_DESCRIPTOR_CONTROL test;

    SECURITY_DESCRIPTOR_CONTROL const mutable
        = SE_DACL_AUTO_INHERIT_REQ | SE_SACL_AUTO_INHERIT_REQ
        | SE_DACL_AUTO_INHERITED   | SE_SACL_AUTO_INHERITED
        | SE_DACL_PROTECTED        | SE_SACL_PROTECTED
        | 0x00000040               | 0x00000080        /* not defined in winnt.h */
        ;
    SECURITY_DESCRIPTOR_CONTROL const immutable
        = SE_OWNER_DEFAULTED       | SE_GROUP_DEFAULTED
        | SE_DACL_PRESENT          | SE_DACL_DEFAULTED
        | SE_SACL_PRESENT          | SE_SACL_DEFAULTED
        | SE_RM_CONTROL_VALID      | SE_SELF_RELATIVE
        ;

    int     bit;
    DWORD   dwRevision;
    LPCSTR  fmt = "Expected error %s, got %u\n";

    GetSecurityDescriptorControl (sec, &ref, &dwRevision);

    /* The mutable bits are mutable regardless of the truth of
       SE_DACL_PRESENT and/or SE_SACL_PRESENT */

    /* Check call barfs if any bit-of-interest is immutable */
    for (bit = 0; bit < 16; ++bit)
    {
        SECURITY_DESCRIPTOR_CONTROL const bitOfInterest = 1 << bit;
        SECURITY_DESCRIPTOR_CONTROL setOrClear = ref & bitOfInterest;

        SECURITY_DESCRIPTOR_CONTROL ctrl;

        DWORD   dwExpect  = (bitOfInterest & immutable)
                          ?  ERROR_INVALID_PARAMETER  :  0xbebecaca;
        LPCSTR  strExpect = (bitOfInterest & immutable)
                          ? "ERROR_INVALID_PARAMETER" : "0xbebecaca";

        ctrl = (bitOfInterest & mutable) ? ref + bitOfInterest : ref;
        setOrClear ^= bitOfInterest;
        SetLastError (0xbebecaca);
        pSetSecurityDescriptorControl (sec, bitOfInterest, setOrClear);
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl(sec, &test, &dwRevision);
        expect_eq(test, ctrl, int, "%x");

        setOrClear ^= bitOfInterest;
        SetLastError (0xbebecaca);
        pSetSecurityDescriptorControl (sec, bitOfInterest, setOrClear);
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl (sec, &test, &dwRevision);
        expect_eq(test, ref, int, "%x");
    }

    /* Check call barfs if any bit-to-set is immutable
       even when not a bit-of-interest */
    for (bit = 0; bit < 16; ++bit)
    {
        SECURITY_DESCRIPTOR_CONTROL const bitsOfInterest = mutable;
        SECURITY_DESCRIPTOR_CONTROL setOrClear = ref & bitsOfInterest;

        SECURITY_DESCRIPTOR_CONTROL ctrl;

        DWORD   dwExpect  = ((1 << bit) & immutable)
                          ?  ERROR_INVALID_PARAMETER  :  0xbebecaca;
        LPCSTR  strExpect = ((1 << bit) & immutable)
                          ? "ERROR_INVALID_PARAMETER" : "0xbebecaca";

        ctrl = ((1 << bit) & immutable) ? test : ref | mutable;
        setOrClear ^= bitsOfInterest;
        SetLastError (0xbebecaca);
        pSetSecurityDescriptorControl (sec, bitsOfInterest, setOrClear | (1 << bit));
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl(sec, &test, &dwRevision);
        expect_eq(test, ctrl, int, "%x");

        ctrl = ((1 << bit) & immutable) ? test : ref | (1 << bit);
        setOrClear ^= bitsOfInterest;
        SetLastError (0xbebecaca);
        pSetSecurityDescriptorControl (sec, bitsOfInterest, setOrClear | (1 << bit));
        ok (GetLastError () == dwExpect, fmt, strExpect, GetLastError ());
        GetSecurityDescriptorControl(sec, &test, &dwRevision);
        expect_eq(test, ctrl, int, "%x");
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
    BOOL ret;

    if (!pConvertStringSecurityDescriptorToSecurityDescriptorA)
    {
        win_skip("ConvertStringSecurityDescriptorToSecurityDescriptor is not available\n");
        return;
    }

    ok(pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)"
          "(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)",
        SDDL_REVISION_1, &sec, &dwDescSize), "Creating descriptor failed\n");

    test_SetSecurityDescriptorControl(sec);

    LocalFree(sec);

    ok(pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SY"
        "G:S-1-5-21-93476-23408-4576",
        SDDL_REVISION_1, &sec, &dwDescSize), "Creating descriptor failed\n");

    test_SetSecurityDescriptorControl(sec);

    LocalFree(sec);

    ok(pConvertStringSecurityDescriptorToSecurityDescriptorA(
        "O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)", SDDL_REVISION_1, &sec, &dwDescSize), "Creating descriptor failed\n");
    buf = HeapAlloc(GetProcessHeap(), 0, dwDescSize);
    pSetSecurityDescriptorControl(sec, SE_DACL_PROTECTED, SE_DACL_PROTECTED);
    GetSecurityDescriptorControl(sec, &ctrl, &dwRevision);
    expect_eq(ctrl, 0x9014, int, "%x");

    ret = GetPrivateObjectSecurity(sec, GROUP_SECURITY_INFORMATION, buf, dwDescSize, &retSize);
    ok(ret, "GetPrivateObjectSecurity failed (err=%u)\n", GetLastError());
    ok(retSize <= dwDescSize, "Buffer too small (%d vs %d)\n", retSize, dwDescSize);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_RESULT_AND_FREE("G:S-1-5-21-93476-23408-4576");
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl, 0x8000, int, "%x");

    ret = GetPrivateObjectSecurity(sec, GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, buf, dwDescSize, &retSize);
    ok(ret, "GetPrivateObjectSecurity failed (err=%u)\n", GetLastError());
    ok(retSize <= dwDescSize, "Buffer too small (%d vs %d)\n", retSize, dwDescSize);
    ret = pConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len);
    ok(ret, "Conversion failed err=%u\n", GetLastError());
    CHECK_ONE_OF_AND_FREE("G:S-1-5-21-93476-23408-4576D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)",
        "G:S-1-5-21-93476-23408-4576D:P(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"); /* Win7 */
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl & (~ SE_DACL_PROTECTED), 0x8004, int, "%x");

    ret = GetPrivateObjectSecurity(sec, sec_info, buf, dwDescSize, &retSize);
    ok(ret, "GetPrivateObjectSecurity failed (err=%u)\n", GetLastError());
    ok(retSize == dwDescSize, "Buffer too small (%d vs %d)\n", retSize, dwDescSize);
    ok(pConvertSecurityDescriptorToStringSecurityDescriptorA(buf, SDDL_REVISION_1, sec_info, &string, &len), "Conversion failed\n");
    CHECK_ONE_OF_AND_FREE("O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)",
      "O:SY"
        "G:S-1-5-21-93476-23408-4576"
        "D:P(A;NP;GAGXGWGR;;;SU)(A;IOID;CCDC;;;SU)(D;OICI;0xffffffff;;;S-1-5-21-93476-23408-4576)"
        "S:(AU;OICINPIOIDSAFA;CCDCLCSWRPRC;;;SU)(AU;NPSA;0x12019f;;;SU)"); /* Win7 */
    GetSecurityDescriptorControl(buf, &ctrl, &dwRevision);
    expect_eq(ctrl & (~ SE_DACL_PROTECTED), 0x8014, int, "%x");

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
        win_skip("InitializeAcl is not implemented\n");
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

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(buffer), ACL_REVISION4);
    if (GetLastError() != ERROR_INVALID_PARAMETER)
    {
        ok(ret, "InitializeAcl(ACL_REVISION4) failed with error %d\n", GetLastError());

        ret = IsValidAcl(pAcl);
        ok(ret, "IsValidAcl failed with error %d\n", GetLastError());
    }
    else
        win_skip("ACL_REVISION4 is not implemented on NT4\n");

    SetLastError(0xdeadbeef);
    ret = InitializeAcl(pAcl, sizeof(buffer), -1);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "InitializeAcl(-1) failed with error %d\n", GetLastError());
}

static void test_GetSecurityInfo(void)
{
    char domain_users_ptr[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES];
    char b[sizeof(TOKEN_USER) + sizeof(SID) + sizeof(DWORD)*SID_MAX_SUB_AUTHORITIES];
    char admin_ptr[sizeof(SID)+sizeof(ULONG)*SID_MAX_SUB_AUTHORITIES], dacl[100];
    PSID domain_users_sid = (PSID) domain_users_ptr, domain_sid;
    SID_IDENTIFIER_AUTHORITY sia = { SECURITY_NT_AUTHORITY };
    int domain_users_ace_id = -1, admins_ace_id = -1, i;
    DWORD sid_size = sizeof(admin_ptr), l = sizeof(b);
    PSID admin_sid = (PSID) admin_ptr, user_sid;
    char sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
    BOOL owner_defaulted, group_defaulted;
    BOOL dacl_defaulted, dacl_present;
    ACL_SIZE_INFORMATION acl_size;
    PSECURITY_DESCRIPTOR pSD;
    ACCESS_ALLOWED_ACE *ace;
    HANDLE token, obj;
    PSID owner, group;
    BOOL bret = TRUE;
    PACL pDacl;
    BYTE flags;
    DWORD ret;

    if (!pGetSecurityInfo || !pSetSecurityInfo)
    {
        win_skip("[Get|Set]SecurityInfo is not available\n");
        return;
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &token))
    {
        if (GetLastError() != ERROR_NO_TOKEN) bret = FALSE;
        else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) bret = FALSE;
    }
    if (!bret)
    {
        win_skip("Failed to get current user token\n");
        return;
    }
    GetTokenInformation(token, TokenUser, b, l, &l);
    CloseHandle( token );
    user_sid = ((TOKEN_USER *)b)->User.Sid;

    /* Create something.  Files have lots of associated security info.  */
    obj = CreateFileA(myARGV[0], GENERIC_READ|WRITE_DAC, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (obj == INVALID_HANDLE_VALUE)
    {
        skip("Couldn't create an object for GetSecurityInfo test\n");
        return;
    }

    ret = pGetSecurityInfo(obj, SE_FILE_OBJECT,
                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          &owner, &group, &pDacl, NULL, &pSD);
    if (ret == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetSecurityInfo is not implemented\n");
        CloseHandle(obj);
        return;
    }
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %d\n", ret);
    ok(pSD != NULL, "GetSecurityInfo\n");
    ok(owner != NULL, "GetSecurityInfo\n");
    ok(group != NULL, "GetSecurityInfo\n");
    if (pDacl != NULL)
        ok(IsValidAcl(pDacl), "GetSecurityInfo\n");
    else
        win_skip("No ACL information returned\n");

    LocalFree(pSD);

    if (!pCreateWellKnownSid)
    {
        win_skip("NULL parameter test would crash on NT4\n");
        CloseHandle(obj);
        return;
    }

    /* If we don't ask for the security descriptor, Windows will still give us
       the other stuff, leaving us no way to free it.  */
    ret = pGetSecurityInfo(obj, SE_FILE_OBJECT,
                          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                          &owner, &group, &pDacl, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %d\n", ret);
    ok(owner != NULL, "GetSecurityInfo\n");
    ok(group != NULL, "GetSecurityInfo\n");
    if (pDacl != NULL)
        ok(IsValidAcl(pDacl), "GetSecurityInfo\n");
    else
        win_skip("No ACL information returned\n");

    /* Create security descriptor information and test that it comes back the same */
    pSD = &sd;
    pDacl = (PACL)&dacl;
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    pCreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &sid_size);
    bret = InitializeAcl(pDacl, sizeof(dacl), ACL_REVISION);
    ok(bret, "Failed to initialize ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, user_sid);
    ok(bret, "Failed to add Current User to ACL.\n");
    bret = pAddAccessAllowedAceEx(pDacl, ACL_REVISION, 0, GENERIC_ALL, admin_sid);
    ok(bret, "Failed to add Administrator Group to ACL.\n");
    bret = SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE);
    ok(bret, "Failed to add ACL to security desciptor.\n");
    ret = pSetSecurityInfo(obj, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                          NULL, NULL, pDacl, NULL);
    ok(ret == ERROR_SUCCESS, "SetSecurityInfo returned %d\n", ret);
    ret = pGetSecurityInfo(obj, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                          NULL, NULL, &pDacl, NULL, &pSD);
    ok(ret == ERROR_SUCCESS, "GetSecurityInfo returned %d\n", ret);
    ok(pDacl && IsValidAcl(pDacl), "GetSecurityInfo returned invalid DACL.\n");
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    if (acl_size.AceCount > 0)
    {
        bret = pGetAce(pDacl, 0, (VOID **)&ace);
        ok(bret, "Failed to get Current User ACE.\n");
        bret = EqualSid(&ace->SidStart, user_sid);
        ok(bret, "Current User ACE != Current User SID.\n");
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Current User ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff, "Current User ACE has unexpected mask (0x%x != 0x1f01ff)\n",
                                  ace->Mask);
    }
    if (acl_size.AceCount > 1)
    {
        bret = pGetAce(pDacl, 1, (VOID **)&ace);
        ok(bret, "Failed to get Administators Group ACE.\n");
        bret = EqualSid(&ace->SidStart, admin_sid);
        ok(bret, "Administators Group ACE != Administators Group SID.\n");
        ok(((ACE_HEADER *)ace)->AceFlags == 0,
           "Administators Group ACE has unexpected flags (0x%x != 0x0)\n", ((ACE_HEADER *)ace)->AceFlags);
        ok(ace->Mask == 0x1f01ff,
                     "Administators Group ACE has unexpected mask (0x%x != 0x1f01ff)\n", ace->Mask);
    }
    LocalFree(pSD);
    CloseHandle(obj);

    /* Obtain the "domain users" SID from the user SID */
    if (!AllocateAndInitializeSid(&sia, 4, *GetSidSubAuthority(user_sid, 0),
                                  *GetSidSubAuthority(user_sid, 1),
                                  *GetSidSubAuthority(user_sid, 2),
                                  *GetSidSubAuthority(user_sid, 3), 0, 0, 0, 0, &domain_sid))
    {
        win_skip("Failed to get current domain SID\n");
        return;
    }
    sid_size = sizeof(domain_users_ptr);
    pCreateWellKnownSid(WinAccountDomainUsersSid, domain_sid, domain_users_sid, &sid_size);
    FreeSid(domain_sid);

    /* Test querying the ownership of a process */
    ret = pGetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT,
                           OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION,
                           NULL, NULL, NULL, NULL, &pSD);
    ok(!ret, "GetNamedSecurityInfo failed with error %d\n", ret);

    bret = GetSecurityDescriptorOwner(pSD, &owner, &owner_defaulted);
    ok(bret, "GetSecurityDescriptorOwner failed with error %d\n", GetLastError());
    ok(owner != NULL, "owner should not be NULL\n");
    ok(EqualSid(owner, admin_sid) || EqualSid(owner, user_sid),
       "Process owner SID != Administrators SID.\n");

    bret = GetSecurityDescriptorGroup(pSD, &group, &group_defaulted);
    ok(bret, "GetSecurityDescriptorGroup failed with error %d\n", GetLastError());
    ok(group != NULL, "group should not be NULL\n");
    ok(EqualSid(group, domain_users_sid), "Process group SID != Domain Users SID.\n");
    LocalFree(pSD);

    /* Test querying the DACL of a process */
    ret = pGetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,
                                   NULL, NULL, NULL, NULL, &pSD);
    ok(!ret, "GetSecurityInfo failed with error %d\n", ret);

    bret = GetSecurityDescriptorDacl(pSD, &dacl_present, &pDacl, &dacl_defaulted);
    ok(bret, "GetSecurityDescriptorDacl failed with error %d\n", GetLastError());
    ok(dacl_present, "DACL should be present\n");
    ok(pDacl && IsValidAcl(pDacl), "GetSecurityDescriptorDacl returned invalid DACL.\n");
    bret = pGetAclInformation(pDacl, &acl_size, sizeof(acl_size), AclSizeInformation);
    ok(bret, "GetAclInformation failed\n");
    ok(acl_size.AceCount != 0, "GetAclInformation returned no ACLs\n");
    for (i=0; i<acl_size.AceCount; i++)
    {
        bret = pGetAce(pDacl, i, (VOID **)&ace);
        ok(bret, "Failed to get ACE %d.\n", i);
        bret = EqualSid(&ace->SidStart, domain_users_sid);
        if (bret) domain_users_ace_id = i;
        bret = EqualSid(&ace->SidStart, admin_sid);
        if (bret) admins_ace_id = i;
    }
    ok(domain_users_ace_id != -1 || broken(domain_users_ace_id == -1) /* win2k */,
       "Domain Users ACE not found.\n");
    if (domain_users_ace_id != -1)
    {
        bret = pGetAce(pDacl, domain_users_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Domain Users ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == (INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE),
           "Domain Users ACE has unexpected flags (0x%x != 0x%x)\n", flags,
           INHERIT_ONLY_ACE|CONTAINER_INHERIT_ACE);
        ok(ace->Mask == GENERIC_READ, "Domain Users ACE has unexpected mask (0x%x != 0x%x)\n",
                                      ace->Mask, GENERIC_READ);
    }
    ok(admins_ace_id != -1 || broken(admins_ace_id == -1) /* xp */,
       "Builtin Admins ACE not found.\n");
    if (admins_ace_id != -1)
    {
        bret = pGetAce(pDacl, admins_ace_id, (VOID **)&ace);
        ok(bret, "Failed to get Builtin Admins ACE.\n");
        flags = ((ACE_HEADER *)ace)->AceFlags;
        ok(flags == 0x0, "Builtin Admins ACE has unexpected flags (0x%x != 0x0)\n", flags);
        ok(ace->Mask == PROCESS_ALL_ACCESS || broken(ace->Mask == 0x1f0fff) /* win2k */,
           "Builtin Admins ACE has unexpected mask (0x%x != 0x%x)\n", ace->Mask, PROCESS_ALL_ACCESS);
    }
    LocalFree(pSD);
}

static void test_GetSidSubAuthority(void)
{
    PSID psid = NULL;

    if (!pGetSidSubAuthority || !pConvertStringSidToSidA || !pIsValidSid || !pGetSidSubAuthorityCount)
    {
        win_skip("Some functions not available\n");
        return;
    }
    /* Note: on windows passing in an invalid index like -1, lets GetSidSubAuthority return 0x05000000 but
             still GetLastError returns ERROR_SUCCESS then. We don't test these unlikely cornercases here for now */
    ok(pConvertStringSidToSidA("S-1-5-21-93476-23408-4576",&psid),"ConvertStringSidToSidA failed\n");
    ok(pIsValidSid(psid),"Sid is not valid\n");
    SetLastError(0xbebecaca);
    ok(*pGetSidSubAuthorityCount(psid) == 4,"GetSidSubAuthorityCount gave %d expected 4\n",*pGetSidSubAuthorityCount(psid));
    ok(GetLastError() == 0,"GetLastError returned %d instead of 0\n",GetLastError());
    SetLastError(0xbebecaca);
    ok(*pGetSidSubAuthority(psid,0) == 21,"GetSidSubAuthority gave %d expected 21\n",*pGetSidSubAuthority(psid,0));
    ok(GetLastError() == 0,"GetLastError returned %d instead of 0\n",GetLastError());
    SetLastError(0xbebecaca);
    ok(*pGetSidSubAuthority(psid,1) == 93476,"GetSidSubAuthority gave %d expected 93476\n",*pGetSidSubAuthority(psid,1));
    ok(GetLastError() == 0,"GetLastError returned %d instead of 0\n",GetLastError());
    SetLastError(0xbebecaca);
    ok(pGetSidSubAuthority(psid,4) != NULL,"Expected out of bounds GetSidSubAuthority to return a non-NULL pointer\n");
    ok(GetLastError() == 0,"GetLastError returned %d instead of 0\n",GetLastError());
    LocalFree(psid);
}

static void test_CheckTokenMembership(void)
{
    PTOKEN_GROUPS token_groups;
    DWORD size;
    HANDLE process_token, token;
    BOOL is_member;
    BOOL ret;
    DWORD i;

    if (!pCheckTokenMembership)
    {
        win_skip("CheckTokenMembership is not available\n");
        return;
    }
    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &process_token);
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());

    ret = DuplicateToken(process_token, SecurityImpersonation, &token);
    ok(ret, "DuplicateToken failed with error %d\n", GetLastError());

    /* groups */
    ret = GetTokenInformation(token, TokenGroups, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "GetTokenInformation(TokenGroups) %s with error %d\n",
        ret ? "succeeded" : "failed", GetLastError());
    token_groups = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetTokenInformation(token, TokenGroups, token_groups, size, &size);
    ok(ret, "GetTokenInformation(TokenGroups) failed with error %d\n", GetLastError());

    for (i = 0; i < token_groups->GroupCount; i++)
    {
        if (token_groups->Groups[i].Attributes & SE_GROUP_ENABLED)
            break;
    }

    if (i == token_groups->GroupCount)
    {
        HeapFree(GetProcessHeap(), 0, token_groups);
        CloseHandle(token);
        skip("user not a member of any group\n");
        return;
    }

    is_member = FALSE;
    ret = pCheckTokenMembership(token, token_groups->Groups[i].Sid, &is_member);
    ok(ret, "CheckTokenMembership failed with error %d\n", GetLastError());
    ok(is_member, "CheckTokenMembership should have detected sid as member\n");

    is_member = FALSE;
    ret = pCheckTokenMembership(NULL, token_groups->Groups[i].Sid, &is_member);
    ok(ret, "CheckTokenMembership failed with error %d\n", GetLastError());
    ok(is_member, "CheckTokenMembership should have detected sid as member\n");

    is_member = TRUE;
    SetLastError(0xdeadbeef);
    ret = pCheckTokenMembership(process_token, token_groups->Groups[i].Sid, &is_member);
    ok(!ret && GetLastError() == ERROR_NO_IMPERSONATION_TOKEN,
        "CheckTokenMembership with process token %s with error %d\n",
        ret ? "succeeded" : "failed", GetLastError());
    ok(!is_member, "CheckTokenMembership should have cleared is_member\n");

    HeapFree(GetProcessHeap(), 0, token_groups);
    CloseHandle(token);
    CloseHandle(process_token);
}

static void test_EqualSid(void)
{
    PSID sid1, sid2;
    BOOL ret;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };

    SetLastError(0xdeadbeef);
    ret = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid1);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("AllocateAndInitializeSid is not implemented\n");
        return;
    }
    ok(ret, "AllocateAndInitializeSid failed with error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef,
       "AllocateAndInitializeSid shouldn't have set last error to %d\n",
       GetLastError());

    ret = AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
        0, 0, 0, 0, 0, 0, 0, &sid2);
    ok(ret, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EqualSid(sid1, sid2);
    ok(!ret, "World and domain admins sids shouldn't have been equal\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       broken(GetLastError() == 0xdeadbeef), /* NT4 */
       "EqualSid should have set last error to ERROR_SUCCESS instead of %d\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    sid2 = FreeSid(sid2);
    ok(!sid2, "FreeSid should have returned NULL instead of %p\n", sid2);
    ok(GetLastError() == 0xdeadbeef,
       "FreeSid shouldn't have set last error to %d\n",
       GetLastError());

    ret = AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid2);
    ok(ret, "AllocateAndInitializeSid failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EqualSid(sid1, sid2);
    ok(ret, "Same sids should have been equal\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       broken(GetLastError() == 0xdeadbeef), /* NT4 */
       "EqualSid should have set last error to ERROR_SUCCESS instead of %d\n",
       GetLastError());

    ((SID *)sid2)->Revision = 2;
    SetLastError(0xdeadbeef);
    ret = EqualSid(sid1, sid2);
    ok(!ret, "EqualSid with invalid sid should have returned FALSE\n");
    ok(GetLastError() == ERROR_SUCCESS ||
       broken(GetLastError() == 0xdeadbeef), /* NT4 */
       "EqualSid should have set last error to ERROR_SUCCESS instead of %d\n",
       GetLastError());
    ((SID *)sid2)->Revision = SID_REVISION;

    FreeSid(sid1);
    FreeSid(sid2);
}

static void test_GetUserNameA(void)
{
    char buffer[UNLEN + 1], filler[UNLEN + 1];
    DWORD required_len, buffer_len;
    BOOL ret;

    /* Test crashes on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        GetUserNameA(NULL, NULL);
    }

    SetLastError(0xdeadbeef);
    required_len = 0;
    ret = GetUserNameA(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(required_len != 0, "Outputted buffer length was %u\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    required_len = 1;
    ret = GetUserNameA(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(required_len != 0 && required_len != 1, "Outputted buffer length was %u\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());

    /* Tests crashes on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        required_len = UNLEN + 1;
        GetUserNameA(NULL, &required_len);

        SetLastError(0xdeadbeef);
        GetUserNameA(buffer, NULL);
    }

    memset(filler, 'x', sizeof(filler));

    /* Note that GetUserNameA on XP and newer outputs the number of bytes
     * required for a Unicode string, which affects a test in the next block. */
    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    required_len = 0;
    ret = GetUserNameA(buffer, &required_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)), "Output buffer was altered\n");
    ok(required_len != 0, "Outputted buffer length was %u\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len = required_len;
    ret = GetUserNameA(buffer, &buffer_len);
    ok(ret == TRUE, "GetUserNameA returned %d, last error %u\n", ret, GetLastError());
    ok(memcmp(buffer, filler, sizeof(filler)) != 0, "Output buffer was untouched\n");
    ok(buffer_len == required_len ||
       broken(buffer_len == required_len / sizeof(WCHAR)), /* XP+ */
       "Outputted buffer length was %u\n", buffer_len);

    /* Use the reported buffer size from the last GetUserNameA call and pass
     * a length that is one less than the required value. */
    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len--;
    ret = GetUserNameA(buffer, &buffer_len);
    ok(ret == FALSE, "GetUserNameA returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)), "Output buffer was untouched\n");
    ok(buffer_len == required_len, "Outputted buffer length was %u\n", buffer_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());
}

static void test_GetUserNameW(void)
{
    WCHAR buffer[UNLEN + 1], filler[UNLEN + 1];
    DWORD required_len, buffer_len;
    BOOL ret;

    /* Test crashes on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        GetUserNameW(NULL, NULL);
    }

    SetLastError(0xdeadbeef);
    required_len = 0;
    ret = GetUserNameW(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(required_len != 0, "Outputted buffer length was %u\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    required_len = 1;
    ret = GetUserNameW(NULL, &required_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(required_len != 0 && required_len != 1, "Outputted buffer length was %u\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());

    /* Tests crash on Windows. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        required_len = UNLEN + 1;
        GetUserNameW(NULL, &required_len);

        SetLastError(0xdeadbeef);
        GetUserNameW(buffer, NULL);
    }

    memset(filler, 'x', sizeof(filler));

    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    required_len = 0;
    ret = GetUserNameW(buffer, &required_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)), "Output buffer was altered\n");
    ok(required_len != 0, "Outputted buffer length was %u\n", required_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len = required_len;
    ret = GetUserNameW(buffer, &buffer_len);
    ok(ret == TRUE, "GetUserNameW returned %d, last error %u\n", ret, GetLastError());
    ok(memcmp(buffer, filler, sizeof(filler)) != 0, "Output buffer was untouched\n");
    ok(buffer_len == required_len, "Outputted buffer length was %u\n", buffer_len);

    /* GetUserNameW on XP and newer writes a truncated portion of the username string to the buffer. */
    SetLastError(0xdeadbeef);
    memcpy(buffer, filler, sizeof(filler));
    buffer_len--;
    ret = GetUserNameW(buffer, &buffer_len);
    ok(ret == FALSE, "GetUserNameW returned %d\n", ret);
    ok(!memcmp(buffer, filler, sizeof(filler)) ||
       broken(memcmp(buffer, filler, sizeof(filler)) != 0), /* XP+ */
       "Output buffer was altered\n");
    ok(buffer_len == required_len, "Outputted buffer length was %u\n", buffer_len);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Last error was %u\n", GetLastError());
}

static void test_CreateRestrictedToken(void)
{
    HANDLE process_token, token, r_token;
    PTOKEN_GROUPS token_groups, groups2;
    SID_AND_ATTRIBUTES sattr;
    SECURITY_IMPERSONATION_LEVEL level;
    TOKEN_TYPE type;
    BOOL is_member;
    DWORD size;
    BOOL ret;
    DWORD i, j;

    if (!pCreateRestrictedToken)
    {
        win_skip("CreateRestrictedToken is not available\n");
        return;
    }

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &process_token);
    ok(ret, "got error %d\n", GetLastError());

    ret = DuplicateTokenEx(process_token, TOKEN_DUPLICATE|TOKEN_ADJUST_GROUPS|TOKEN_QUERY,
        NULL, SecurityImpersonation, TokenImpersonation, &token);
    ok(ret, "got error %d\n", GetLastError());

    /* groups */
    ret = GetTokenInformation(token, TokenGroups, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "got %d with error %d\n", ret, GetLastError());
    token_groups = HeapAlloc(GetProcessHeap(), 0, size);
    ret = GetTokenInformation(token, TokenGroups, token_groups, size, &size);
    ok(ret, "got error %d\n", GetLastError());

    for (i = 0; i < token_groups->GroupCount; i++)
    {
        if (token_groups->Groups[i].Attributes & SE_GROUP_ENABLED)
            break;
    }

    if (i == token_groups->GroupCount)
    {
        HeapFree(GetProcessHeap(), 0, token_groups);
        CloseHandle(token);
        skip("User not a member of any group\n");
        return;
    }

    is_member = FALSE;
    ret = pCheckTokenMembership(token, token_groups->Groups[i].Sid, &is_member);
    ok(ret, "got error %d\n", GetLastError());
    ok(is_member, "not a member\n");

    /* disable a SID in new token */
    sattr.Sid = token_groups->Groups[i].Sid;
    sattr.Attributes = 0;
    r_token = NULL;
    ret = pCreateRestrictedToken(token, 0, 1, &sattr, 0, NULL, 0, NULL, &r_token);
    ok(ret, "got error %d\n", GetLastError());

    if (ret)
    {
        /* check if a SID is enabled */
        is_member = TRUE;
        ret = pCheckTokenMembership(r_token, token_groups->Groups[i].Sid, &is_member);
        ok(ret, "got error %d\n", GetLastError());
        todo_wine ok(!is_member, "not a member\n");

        ret = GetTokenInformation(r_token, TokenGroups, NULL, 0, &size);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d with error %d\n",
            ret, GetLastError());
        groups2 = HeapAlloc(GetProcessHeap(), 0, size);
        ret = GetTokenInformation(r_token, TokenGroups, groups2, size, &size);
        ok(ret, "got error %d\n", GetLastError());

        for (j = 0; j < groups2->GroupCount; j++)
        {
            if (EqualSid(groups2->Groups[j].Sid, token_groups->Groups[i].Sid))
                break;
        }

        todo_wine ok(groups2->Groups[j].Attributes & SE_GROUP_USE_FOR_DENY_ONLY,
            "got wrong attributes\n");
        todo_wine ok((groups2->Groups[j].Attributes & SE_GROUP_ENABLED) == 0,
            "got wrong attributes\n");

        HeapFree(GetProcessHeap(), 0, groups2);

        size = sizeof(type);
        ret = GetTokenInformation(r_token, TokenType, &type, size, &size);
        ok(ret, "got error %d\n", GetLastError());
        ok(type == TokenImpersonation, "got type %u\n", type);

        size = sizeof(level);
        ret = GetTokenInformation(r_token, TokenImpersonationLevel, &level, size, &size);
        ok(ret, "got error %d\n", GetLastError());
        ok(level == SecurityImpersonation, "got level %u\n", type);
    }

    HeapFree(GetProcessHeap(), 0, token_groups);
    CloseHandle(r_token);
    CloseHandle(token);
    CloseHandle(process_token);
}

static void validate_default_security_descriptor(SECURITY_DESCRIPTOR *sd)
{
    BOOL ret, present, defaulted;
    ACL *acl;
    void *sid;

    ret = IsValidSecurityDescriptor(sd);
    ok(ret, "security descriptor is not valid\n");

    present = -1;
    defaulted = -1;
    acl = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetSecurityDescriptorDacl(sd, &present, &acl, &defaulted);
    ok(ret, "GetSecurityDescriptorDacl error %d\n", GetLastError());
todo_wine
    ok(present == 1, "acl is not present\n");
todo_wine
    ok(acl != (void *)0xdeadbeef && acl != NULL, "acl pointer is not set\n");
    ok(defaulted == 0, "defaulted is set to TRUE\n");

    defaulted = -1;
    sid = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetSecurityDescriptorOwner(sd, &sid, &defaulted);
    ok(ret, "GetSecurityDescriptorOwner error %d\n", GetLastError());
todo_wine
    ok(sid != (void *)0xdeadbeef && sid != NULL, "sid pointer is not set\n");
    ok(defaulted == 0, "defaulted is set to TRUE\n");

    defaulted = -1;
    sid = (void *)0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetSecurityDescriptorGroup(sd, &sid, &defaulted);
    ok(ret, "GetSecurityDescriptorGroup error %d\n", GetLastError());
todo_wine
    ok(sid != (void *)0xdeadbeef && sid != NULL, "sid pointer is not set\n");
    ok(defaulted == 0, "defaulted is set to TRUE\n");
}

static void test_default_handle_security(HANDLE token, HANDLE handle, GENERIC_MAPPING *mapping)
{
    DWORD ret, granted, priv_set_len;
    BOOL status;
    PRIVILEGE_SET priv_set;
    SECURITY_DESCRIPTOR *sd;

    sd = test_get_security_descriptor(handle, __LINE__);
    validate_default_security_descriptor(sd);

    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, MAXIMUM_ALLOWED, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericAll, "expected all access %#x, got %#x\n", mapping->GenericAll, granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, 0, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %d\n", GetLastError());
    ok(status == 0 || broken(status == 1) /* NT4 */, "expected 0, got %d\n", status);
    ok(granted == 0 || broken(granted == mapping->GenericRead) /* NT4 */, "expected 0, got %#x\n", granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, ACCESS_SYSTEM_SECURITY, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %d\n", GetLastError());
    ok(status == 0, "expected 0, got %d\n", status);
    ok(granted == 0, "expected 0, got %#x\n", granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, mapping->GenericRead, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericRead, "expected read access %#x, got %#x\n", mapping->GenericRead, granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, mapping->GenericWrite, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericWrite, "expected write access %#x, got %#x\n", mapping->GenericWrite, granted);
}
    priv_set_len = sizeof(priv_set);
    granted = 0xdeadbeef;
    status = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = AccessCheck(sd, token, mapping->GenericExecute, mapping, &priv_set, &priv_set_len, &granted, &status);
todo_wine {
    ok(ret, "AccessCheck error %d\n", GetLastError());
    ok(status == 1, "expected 1, got %d\n", status);
    ok(granted == mapping->GenericExecute, "expected execute access %#x, got %#x\n", mapping->GenericExecute, granted);
}
    HeapFree(GetProcessHeap(), 0, sd);
}

static ACCESS_MASK get_obj_access(HANDLE obj)
{
    OBJECT_BASIC_INFORMATION info;
    NTSTATUS status;

    if (!pNtQueryObject) return 0;

    status = pNtQueryObject(obj, ObjectBasicInformation, &info, sizeof(info), NULL);
    ok(!status, "NtQueryObject error %#x\n", status);

    return info.GrantedAccess;
}

static void test_mutex_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE mutex, dup;
    GENERIC_MAPPING mapping = { STANDARD_RIGHTS_READ | MUTANT_QUERY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_WRITE | MUTEX_MODIFY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                STANDARD_RIGHTS_ALL | MUTEX_ALL_ACCESS };
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | MUTANT_QUERY_STATE },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | MUTANT_QUERY_STATE }
    };

    SetLastError(0xdeadbeef);
    mutex = OpenMutexA(0, FALSE, "WineTestMutex");
    ok(!mutex, "mutex should not exist\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    mutex = CreateMutexA(NULL, FALSE, "WineTestMutex");
    ok(mutex != 0, "CreateMutex error %d\n", GetLastError());

    access = get_obj_access(mutex);
    ok(access == MUTANT_ALL_ACCESS, "expected MUTANT_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), mutex, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

        CloseHandle(dup);

        SetLastError(0xdeadbeef);
        dup = OpenMutexA(0, FALSE, "WineTestMutex");
todo_wine
        ok(!dup, "OpenMutex should fail\n");
todo_wine
        ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %u\n", GetLastError());
    }

    test_default_handle_security(token, mutex, &mapping);

    CloseHandle (mutex);
}

static void test_event_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE event, dup;
    GENERIC_MAPPING mapping = { STANDARD_RIGHTS_READ | EVENT_QUERY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                                STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                STANDARD_RIGHTS_ALL | EVENT_ALL_ACCESS };
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | EVENT_QUERY_STATE },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | EVENT_QUERY_STATE | EVENT_MODIFY_STATE }
    };

    SetLastError(0xdeadbeef);
    event = OpenEventA(0, FALSE, "WineTestEvent");
    ok(!event, "event should not exist\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    event = CreateEventA(NULL, FALSE, FALSE, "WineTestEvent");
    ok(event != 0, "CreateEvent error %d\n", GetLastError());

    access = get_obj_access(event);
    ok(access == EVENT_ALL_ACCESS, "expected EVENT_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), event, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

        CloseHandle(dup);

        SetLastError(0xdeadbeef);
        dup = OpenEventA(0, FALSE, "WineTestEvent");
todo_wine
        ok(!dup, "OpenEvent should fail\n");
todo_wine
        ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %u\n", GetLastError());
    }

    test_default_handle_security(token, event, &mapping);

    CloseHandle(event);
}

static void test_semaphore_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE sem, dup;
    GENERIC_MAPPING mapping = { STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE,
                                STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE,
                                STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                STANDARD_RIGHTS_ALL | SEMAPHORE_ALL_ACCESS };
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | SEMAPHORE_QUERY_STATE | SEMAPHORE_MODIFY_STATE }
    };

    SetLastError(0xdeadbeef);
    sem = OpenSemaphoreA(0, FALSE, "WineTestSemaphore");
    ok(!sem, "semaphore should not exist\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    sem = CreateSemaphoreA(NULL, 0, 10, "WineTestSemaphore");
    ok(sem != 0, "CreateSemaphore error %d\n", GetLastError());

    access = get_obj_access(sem);
    ok(access == SEMAPHORE_ALL_ACCESS, "expected SEMAPHORE_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), sem, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    test_default_handle_security(token, sem, &mapping);

    CloseHandle(sem);
}

#define WINE_TEST_PIPE "\\\\.\\pipe\\WineTestPipe"
static void test_named_pipe_security(HANDLE token)
{
    DWORD ret, i, access;
    HANDLE pipe, file, dup;
    GENERIC_MAPPING mapping = { FILE_GENERIC_READ,
                                FILE_GENERIC_WRITE,
                                FILE_GENERIC_EXECUTE,
                                STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS };
    static const struct
    {
        int todo, generic, mapped;
    } map[] =
    {
        { 0, 0, 0 },
        { 1, GENERIC_READ, FILE_GENERIC_READ },
        { 1, GENERIC_WRITE, FILE_GENERIC_WRITE },
        { 1, GENERIC_EXECUTE, FILE_GENERIC_EXECUTE },
        { 1, GENERIC_ALL, STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS }
    };
    static const struct
    {
        DWORD open_mode;
        DWORD access;
    } creation_access[] =
    {
        { PIPE_ACCESS_INBOUND, FILE_GENERIC_READ },
        { PIPE_ACCESS_OUTBOUND, FILE_GENERIC_WRITE },
        { PIPE_ACCESS_DUPLEX, FILE_GENERIC_READ|FILE_GENERIC_WRITE },
        { PIPE_ACCESS_INBOUND|WRITE_DAC, FILE_GENERIC_READ|WRITE_DAC },
        { PIPE_ACCESS_INBOUND|WRITE_OWNER, FILE_GENERIC_READ|WRITE_OWNER }
        /* ACCESS_SYSTEM_SECURITY is also valid, but will fail with ERROR_PRIVILEGE_NOT_HELD */
    };

    /* Test the different security access options for pipes */
    for (i = 0; i < sizeof(creation_access)/sizeof(creation_access[0]); i++)
    {
        SetLastError(0xdeadbeef);
        pipe = CreateNamedPipeA(WINE_TEST_PIPE, creation_access[i].open_mode,
                                PIPE_TYPE_BYTE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES, 0, 0,
                                NMPWAIT_USE_DEFAULT_WAIT, NULL);
        ok(pipe != INVALID_HANDLE_VALUE, "CreateNamedPipe(0x%x) error %d\n",
                                         creation_access[i].open_mode, GetLastError());
        access = get_obj_access(pipe);
        ok(access == creation_access[i].access,
           "CreateNamedPipeA(0x%x) pipe expected access 0x%x (got 0x%x)\n",
           creation_access[i].open_mode, creation_access[i].access, access);
        CloseHandle(pipe);
    }

    SetLastError(0xdeadbeef);
    pipe = CreateNamedPipeA(WINE_TEST_PIPE, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                            PIPE_TYPE_BYTE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES,
                            0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(pipe != INVALID_HANDLE_VALUE, "CreateNamedPipe error %d\n", GetLastError());

    test_default_handle_security(token, pipe, &mapping);

    SetLastError(0xdeadbeef);
    file = CreateFileA(WINE_TEST_PIPE, FILE_ALL_ACCESS, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    access = get_obj_access(file);
    ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(file);
    CloseHandle(pipe);

    SetLastError(0xdeadbeef);
    file = CreateFileA("\\\\.\\pipe\\", FILE_ALL_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE || broken(file == INVALID_HANDLE_VALUE) /* before Vista */, "CreateFile error %d\n", GetLastError());

    if (file != INVALID_HANDLE_VALUE)
    {
        access = get_obj_access(file);
        ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#x\n", access);

        for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
        {
            SetLastError( 0xdeadbeef );
            ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                                  map[i].generic, FALSE, 0);
            ok(ret, "DuplicateHandle error %d\n", GetLastError());

            access = get_obj_access(dup);
            if (map[i].todo)
todo_wine
            ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            else
            ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

            CloseHandle(dup);
        }
    }

    CloseHandle(file);
}

static void test_file_security(HANDLE token)
{
    DWORD ret, i, access, bytes;
    HANDLE file, dup;
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, FILE_GENERIC_READ },
        { GENERIC_WRITE, FILE_GENERIC_WRITE },
        { GENERIC_EXECUTE, FILE_GENERIC_EXECUTE },
        { GENERIC_ALL, STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS }
    };
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    char buf[16];

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "tmp", 0, file_name);

    /* file */
    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    access = get_obj_access(file);
    ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    access = get_obj_access(file);
todo_wine
    ok(access == (FILE_READ_ATTRIBUTES | SYNCHRONIZE), "expected FILE_READ_ATTRIBUTES | SYNCHRONIZE, got %#x\n", access);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(file, buf, sizeof(buf), &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    ok(bytes == 0, "expected 0, got %u\n", bytes);

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    access = get_obj_access(file);
todo_wine
    ok(access == (FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES), "expected FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES, got %#x\n", access);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(file, buf, sizeof(buf), &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    ok(bytes == 0, "expected 0, got %u\n", bytes);

    CloseHandle(file);
    DeleteFileA(file_name);

    /* directory */
    SetLastError(0xdeadbeef);
    file = CreateFileA(temp_path, GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    access = get_obj_access(file);
    ok(access == FILE_ALL_ACCESS, "expected FILE_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(temp_path, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    access = get_obj_access(file);
todo_wine
    ok(access == (FILE_READ_ATTRIBUTES | SYNCHRONIZE), "expected FILE_READ_ATTRIBUTES | SYNCHRONIZE, got %#x\n", access);

    CloseHandle(file);

    SetLastError(0xdeadbeef);
    file = CreateFileA(temp_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    access = get_obj_access(file);
todo_wine
    ok(access == (FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES), "expected FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES, got %#x\n", access);

    CloseHandle(file);
}

static void test_filemap_security(void)
{
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    DWORD ret, i, access;
    HANDLE file, mapping, dup;
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | SECTION_QUERY | SECTION_MAP_READ },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE },
        { GENERIC_ALL, STANDARD_RIGHTS_REQUIRED | SECTION_ALL_ACCESS }
    };
    static const struct
    {
        int prot, mapped;
    } prot_map[] =
    {
        { 0, 0 },
        { PAGE_NOACCESS, 0 },
        { PAGE_READONLY, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ },
        { PAGE_READWRITE, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE },
        { PAGE_WRITECOPY, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ },
        { PAGE_EXECUTE, 0 },
        { PAGE_EXECUTE_READ, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE },
        { PAGE_EXECUTE_READWRITE, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE },
        { PAGE_EXECUTE_WRITECOPY, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE }
    };

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "tmp", 0, file_name);

    SetLastError(0xdeadbeef);
    file = CreateFileA(file_name, GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());
    SetFilePointer(file, 4096, NULL, FILE_BEGIN);
    SetEndOfFile(file);

    for (i = 0; i < sizeof(prot_map)/sizeof(prot_map[0]); i++)
    {
        SetLastError(0xdeadbeef);
        mapping = CreateFileMappingW(file, NULL, prot_map[i].prot, 0, 4096, NULL);
        if (prot_map[i].mapped)
        {
            if (!mapping)
            {
                /* NT4 and win2k don't support EXEC on file mappings */
                if (prot_map[i].prot == PAGE_EXECUTE_READ || prot_map[i].prot == PAGE_EXECUTE_READWRITE || prot_map[i].prot == PAGE_EXECUTE_WRITECOPY)
                {
                    win_skip("CreateFileMapping doesn't support PAGE_EXECUTE protection\n");
                    continue;
                }
            }
            ok(mapping != 0, "CreateFileMapping(%04x) error %d\n", prot_map[i].prot, GetLastError());
        }
        else
        {
            ok(!mapping, "CreateFileMapping(%04x) should fail\n", prot_map[i].prot);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
            continue;
        }

        access = get_obj_access(mapping);
        ok(access == prot_map[i].mapped, "%d: expected %#x, got %#x\n", i, prot_map[i].mapped, access);

        CloseHandle(mapping);
    }

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingW(file, NULL, PAGE_EXECUTE_READWRITE, 0, 4096, NULL);
    if (!mapping)
    {
        /* NT4 and win2k don't support EXEC on file mappings */
        win_skip("CreateFileMapping doesn't support PAGE_EXECUTE protection\n");
        CloseHandle(file);
        DeleteFileA(file_name);
        return;
    }
    ok(mapping != 0, "CreateFileMapping error %d\n", GetLastError());

    access = get_obj_access(mapping);
    ok(access == (STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE),
       "expected STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), mapping, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);

        CloseHandle(dup);
    }

    CloseHandle(mapping);
    CloseHandle(file);
    DeleteFileA(file_name);
}

static void test_thread_security(void)
{
    DWORD ret, i, access;
    HANDLE thread, dup;
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | THREAD_SET_INFORMATION | THREAD_SET_CONTEXT | THREAD_TERMINATE | THREAD_SUSPEND_RESUME | 0x4 },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, THREAD_ALL_ACCESS_NT4 }
    };

    SetLastError(0xdeadbeef);
    thread = CreateThread(NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(thread != 0, "CreateThread error %d\n", GetLastError());

    access = get_obj_access(thread);
    ok(access == THREAD_ALL_ACCESS_NT4 || access == THREAD_ALL_ACCESS_VISTA, "expected THREAD_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), thread, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        switch (map[i].generic)
        {
        case GENERIC_READ:
        case GENERIC_EXECUTE:
            ok(access == map[i].mapped ||
               access == (map[i].mapped | THREAD_QUERY_LIMITED_INFORMATION) /* Vista+ */ ||
               access == (map[i].mapped | THREAD_QUERY_LIMITED_INFORMATION | THREAD_RESUME) /* win8 */,
               "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        case GENERIC_WRITE:
todo_wine
            ok(access == map[i].mapped ||
               access == (map[i].mapped | THREAD_SET_LIMITED_INFORMATION) /* Vista+ */ ||
               access == (map[i].mapped | THREAD_SET_LIMITED_INFORMATION | THREAD_RESUME) /* win8 */,
               "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        case GENERIC_ALL:
            ok(access == map[i].mapped || access == THREAD_ALL_ACCESS_VISTA,
               "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        default:
            ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        }

        CloseHandle(dup);
    }

    TerminateThread(thread, 0);
    CloseHandle(thread);
}

static void test_process_access(void)
{
    DWORD ret, i, access;
    HANDLE process, dup;
    STARTUPINFOA sti;
    PROCESS_INFORMATION pi;
    char cmdline[] = "winver.exe";
    static const struct
    {
        int generic, mapped;
    } map[] =
    {
        { 0, 0 },
        { GENERIC_READ, STANDARD_RIGHTS_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ },
        { GENERIC_WRITE, STANDARD_RIGHTS_WRITE | PROCESS_SET_QUOTA | PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME |
                         PROCESS_VM_WRITE | PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION },
        { GENERIC_EXECUTE, STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE },
        { GENERIC_ALL, PROCESS_ALL_ACCESS_NT4 }
    };

    memset(&sti, 0, sizeof(sti));
    sti.cb = sizeof(sti);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sti, &pi);
    ok(ret, "CreateProcess() error %d\n", GetLastError());

    CloseHandle(pi.hThread);
    process = pi.hProcess;

    access = get_obj_access(process);
    ok(access == PROCESS_ALL_ACCESS_NT4 || access == PROCESS_ALL_ACCESS_VISTA, "expected PROCESS_ALL_ACCESS, got %#x\n", access);

    for (i = 0; i < sizeof(map)/sizeof(map[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = DuplicateHandle(GetCurrentProcess(), process, GetCurrentProcess(), &dup,
                              map[i].generic, FALSE, 0);
        ok(ret, "DuplicateHandle error %d\n", GetLastError());

        access = get_obj_access(dup);
        switch (map[i].generic)
        {
        case GENERIC_READ:
            ok(access == map[i].mapped || access == (map[i].mapped | PROCESS_QUERY_LIMITED_INFORMATION) /* Vista+ */,
               "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        case GENERIC_WRITE:
            ok(access == map[i].mapped ||
               access == (map[i].mapped | PROCESS_TERMINATE) /* before Vista */ ||
               access == (map[i].mapped | PROCESS_SET_LIMITED_INFORMATION) /* win8 */,
               "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        case GENERIC_EXECUTE:
            ok(access == map[i].mapped || access == (map[i].mapped | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE) /* Vista+ */,
               "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        case GENERIC_ALL:
            ok(access == map[i].mapped || access == PROCESS_ALL_ACCESS_VISTA,
               "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        default:
            ok(access == map[i].mapped, "%d: expected %#x, got %#x\n", i, map[i].mapped, access);
            break;
        }

        CloseHandle(dup);
    }

    TerminateProcess(process, 0);
    CloseHandle(process);
}

static BOOL validate_impersonation_token(HANDLE token, DWORD *token_type)
{
    DWORD ret, needed;
    TOKEN_TYPE type;
    SECURITY_IMPERSONATION_LEVEL sil;

    type = 0xdeadbeef;
    needed = 0;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenType, &type, sizeof(type), &needed);
    ok(ret, "GetTokenInformation error %d\n", GetLastError());
    ok(needed == sizeof(type), "GetTokenInformation should return required buffer length\n");
    ok(type == TokenPrimary || type == TokenImpersonation, "expected TokenPrimary or TokenImpersonation, got %d\n", type);

    *token_type = type;
    if (type != TokenImpersonation) return FALSE;

    needed = 0;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenImpersonationLevel, &sil, sizeof(sil), &needed);
    ok(ret, "GetTokenInformation error %d\n", GetLastError());
    ok(needed == sizeof(sil), "GetTokenInformation should return required buffer length\n");
    ok(sil == SecurityImpersonation, "expected SecurityImpersonation, got %d\n", sil);

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenDefaultDacl, NULL, 0, &needed);
    ok(!ret, "GetTokenInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed != 0xdeadbeef, "GetTokenInformation should return required buffer length\n");
    ok(needed > sizeof(TOKEN_DEFAULT_DACL), "GetTokenInformation returned empty default DACL\n");

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenOwner, NULL, 0, &needed);
    ok(!ret, "GetTokenInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed != 0xdeadbeef, "GetTokenInformation should return required buffer length\n");
    ok(needed > sizeof(TOKEN_OWNER), "GetTokenInformation returned empty token owner\n");

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = GetTokenInformation(token, TokenPrimaryGroup, NULL, 0, &needed);
    ok(!ret, "GetTokenInformation should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed != 0xdeadbeef, "GetTokenInformation should return required buffer length\n");
    ok(needed > sizeof(TOKEN_PRIMARY_GROUP), "GetTokenInformation returned empty token primary group\n");

    return TRUE;
}

static void test_kernel_objects_security(void)
{
    HANDLE token, process_token;
    DWORD ret, token_type;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &process_token);
    ok(ret, "OpenProcessToken error %d\n", GetLastError());

    ret = validate_impersonation_token(process_token, &token_type);
    ok(token_type == TokenPrimary, "expected TokenPrimary, got %d\n", token_type);
    ok(!ret, "access token should not be an impersonation token\n");

    ret = DuplicateToken(process_token, SecurityImpersonation, &token);
    ok(ret, "DuplicateToken error %d\n", GetLastError());

    ret = validate_impersonation_token(token, &token_type);
    ok(ret, "access token should be a valid impersonation token\n");
    ok(token_type == TokenImpersonation, "expected TokenImpersonation, got %d\n", token_type);

    test_mutex_security(token);
    test_event_security(token);
    test_named_pipe_security(token);
    test_semaphore_security(token);
    test_file_security(token);
    test_filemap_security();
    test_thread_security();
    test_process_access();
    /* FIXME: test other kernel object types */

    CloseHandle(process_token);
    CloseHandle(token);
}

static void test_TokenIntegrityLevel(void)
{
    TOKEN_MANDATORY_LABEL *tml;
    BYTE buffer[64];        /* using max. 28 byte in win7 x64 */
    HANDLE token;
    DWORD size;
    DWORD res;
    char *sidname = NULL;
    static SID medium_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                                                    {SECURITY_MANDATORY_HIGH_RID}};
    static SID high_level = {SID_REVISION, 1, {SECURITY_MANDATORY_LABEL_AUTHORITY},
                                                    {SECURITY_MANDATORY_MEDIUM_RID}};

    SetLastError(0xdeadbeef);
    res = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    ok(res, "got %d with %d (expected TRUE)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = GetTokenInformation(token, TokenIntegrityLevel, buffer, sizeof(buffer), &size);

    /* not supported before Vista */
    if (!res && ((GetLastError() == ERROR_INVALID_PARAMETER) || GetLastError() == ERROR_INVALID_FUNCTION))
    {
        win_skip("TokenIntegrityLevel not supported\n");
        CloseHandle(token);
        return;
    }

    ok(res, "got %u with %u (expected TRUE)\n", res, GetLastError());
    if (!res)
    {
        CloseHandle(token);
        return;
    }

    tml = (TOKEN_MANDATORY_LABEL*) buffer;
    ok(tml->Label.Attributes == (SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED),
        "got 0x%x (expected 0x%x)\n", tml->Label.Attributes, (SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED));

    SetLastError(0xdeadbeef);
    res = pConvertSidToStringSidA(tml->Label.Sid, &sidname);
    ok(res, "got %u and %u\n", res, GetLastError());

    ok(EqualSid(tml->Label.Sid, &medium_level) || EqualSid(tml->Label.Sid, &high_level),
        "got %s (expected 'S-1-16-8192' or 'S-1-16-12288')\n", sidname);

    LocalFree(sidname);
    CloseHandle(token);
}

static void test_default_dacl_owner_sid(void)
{
    HANDLE handle;
    BOOL ret, defaulted, present, found;
    DWORD size, index;
    SECURITY_DESCRIPTOR *sd;
    SECURITY_ATTRIBUTES sa;
    PSID owner;
    ACL *dacl;
    ACCESS_ALLOWED_ACE *ace;

    sd = HeapAlloc( GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH );
    ret = InitializeSecurityDescriptor( sd, SECURITY_DESCRIPTOR_REVISION );
    ok( ret, "error %u\n", GetLastError() );

    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = sd;
    sa.bInheritHandle       = FALSE;
    handle = CreateEventA( &sa, TRUE, TRUE, "test_event" );
    ok( handle != NULL, "error %u\n", GetLastError() );

    size = 0;
    ret = GetKernelObjectSecurity( handle, OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, NULL, 0, &size );
    ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "error %u\n", GetLastError() );

    sd = HeapAlloc( GetProcessHeap(), 0, size );
    ret = GetKernelObjectSecurity( handle, OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, sd, size, &size );
    ok( ret, "error %u\n", GetLastError() );

    owner = (void *)0xdeadbeef;
    defaulted = TRUE;
    ret = GetSecurityDescriptorOwner( sd, &owner, &defaulted );
    ok( ret, "error %u\n", GetLastError() );
    ok( owner != (void *)0xdeadbeef, "owner not set\n" );
    ok( !defaulted, "owner defaulted\n" );

    dacl = (void *)0xdeadbeef;
    present = FALSE;
    defaulted = TRUE;
    ret = GetSecurityDescriptorDacl( sd, &present, &dacl, &defaulted );
    ok( ret, "error %u\n", GetLastError() );
    ok( present, "dacl not present\n" );
    ok( dacl != (void *)0xdeadbeef, "dacl not set\n" );
    ok( !defaulted, "dacl defaulted\n" );

    index = 0;
    found = FALSE;
    while (pGetAce( dacl, index++, (void **)&ace ))
    {
        if (EqualSid( &ace->SidStart, owner )) found = TRUE;
    }
    ok( found, "owner sid not found in dacl\n" );

    HeapFree( GetProcessHeap(), 0, sa.lpSecurityDescriptor );
    HeapFree( GetProcessHeap(), 0, sd );
    CloseHandle( handle );
}

static void test_AdjustTokenPrivileges(void)
{
    TOKEN_PRIVILEGES tp, prev;
    HANDLE token;
    DWORD len;
    LUID luid;
    BOOL ret;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
        return;

    if (!LookupPrivilegeValueA(NULL, SE_BACKUP_NAME, &luid))
    {
        CloseHandle(token);
        return;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    len = 0xdeadbeef;
    ret = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, &len);
    ok(ret, "got %d\n", ret);
    ok(len == 0xdeadbeef, "got length %d\n", len);

    /* revert */
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = 0;
    AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &prev, NULL);

    CloseHandle(token);
}

static void test_AddAce(void)
{
    static SID const sidWorld = { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY} , { SECURITY_WORLD_RID } };

    char acl_buf[1024], ace_buf[256];
    ACCESS_ALLOWED_ACE *ace = (ACCESS_ALLOWED_ACE*)ace_buf;
    PACL acl = (PACL)acl_buf;
    BOOL ret;

    memset(ace, 0, sizeof(ace_buf));
    ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ace->Header.AceSize = sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+sizeof(SID);
    memcpy(&ace->SidStart, &sidWorld, sizeof(sidWorld));

    ret = InitializeAcl(acl, sizeof(acl_buf), ACL_REVISION2);
    ok(ret, "InitializeAcl failed: %d\n", GetLastError());

    ret = AddAce(acl, ACL_REVISION1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());
    ret = AddAce(acl, ACL_REVISION2, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());
    ret = AddAce(acl, ACL_REVISION3, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());
    ok(acl->AclRevision == ACL_REVISION3, "acl->AclRevision = %d\n", acl->AclRevision);
    ret = AddAce(acl, ACL_REVISION4, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());
    ok(acl->AclRevision == ACL_REVISION4, "acl->AclRevision = %d\n", acl->AclRevision);
    ret = AddAce(acl, ACL_REVISION1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());
    ok(acl->AclRevision == ACL_REVISION4, "acl->AclRevision = %d\n", acl->AclRevision);
    ret = AddAce(acl, ACL_REVISION2, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());

    ret = AddAce(acl, MIN_ACL_REVISION-1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());
    /* next test succeededs but corrupts ACL */
    ret = AddAce(acl, MAX_ACL_REVISION+1, MAXDWORD, ace, ace->Header.AceSize);
    ok(ret, "AddAce failed: %d\n", GetLastError());
    ok(acl->AclRevision == MAX_ACL_REVISION+1, "acl->AclRevision = %d\n", acl->AclRevision);
    SetLastError(0xdeadbeef);
    ret = AddAce(acl, ACL_REVISION1, MAXDWORD, ace, ace->Header.AceSize);
    ok(!ret, "AddAce succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %d\n", GetLastError());
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
    test_kernel_objects_security();
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
    test_SetEntriesInAclW();
    test_SetEntriesInAclA();
    test_CreateDirectoryA();
    test_GetNamedSecurityInfoA();
    test_ConvertStringSecurityDescriptor();
    test_ConvertSecurityDescriptorToString();
    test_PrivateObjectSecurity();
    test_acls();
    test_GetSecurityInfo();
    test_GetSidSubAuthority();
    test_CheckTokenMembership();
    test_EqualSid();
    test_GetUserNameA();
    test_GetUserNameW();
    test_CreateRestrictedToken();
    test_TokenIntegrityLevel();
    test_default_dacl_owner_sid();
    test_AdjustTokenPrivileges();
    test_AddAce();
}
